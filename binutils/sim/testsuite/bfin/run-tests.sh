#!/bin/sh

usage() {
	cat <<-EOF
	Usage: $0 [-rs] [-rj <board>] [-rh <board ip>] [tests]

	If no tests are specified, all tests are processed.

	Options:
	  -rs          Run on simulator
	  -rj <board>  Run on board via JTAG
	  -rh <ip>     Run on board ip
	  -j <num>     Num jobs to run
	EOF
	exit ${1:-1}
}

: ${MAKE:=make}
boardip=
boardjtag=
run_sim=false
run_jtag=false
run_host=false
jobs=`getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1`
: $(( jobs += 1 ))
while [ -n "$1" ] ; do
	case $1 in
		-rs) run_sim=true;;
		-rj) boardjtag=$2; shift; run_jtag=true;;
		-rh) boardip=$2; shift; run_host=true;;
		-j)  jobs=$2; shift;;
		-*)  usage;;
		*)   break;;
	esac
	shift
done
${run_jtag} || ${run_host} || ${run_sim} || run_sim=true

if ${run_host} && [ -z "${boardip}" ] ; then
	usage
fi

cd "${0%/*}" || exit 1

dorsh() {
	# rsh sucks and does not pass up its exit status, so we have to:
	#  on board:
	#    - send all output to stdout
	#    - send exit status to stderr
	#  on host:
	#    - swap stdout and stderr
	#    - pass exit status to `exit`
	#    - send stderr back to stdout and up
	(exit \
		$(rsh -l root $boardip \
			'(/tmp/'$1') 2>&1; ret=$?; echo $ret 1>&2; [ $ret -eq 0 ] && rm -f /tmp/'$1 \
			3>&1 1>&2 2>&3) \
		2>&1) 2>&1
}

dojtag() {
	if grep -q CHECKREG ${1%.x} ; then
		echo "DBGA does not work via JTAG"
		exit 77
	fi

	cat <<-EOF > commands
		target remote localhost:2000
		load

		b *_pass
		commands
		exit 0
		end

		b *_fail
		commands
		exit 1
		end

		# we're executing at EVT1, so this doesn't really help ...
		set ((long *)0xFFE02000)[3] = _fail
		set ((long *)0xFFE02000)[5] = _fail

		c
	EOF
	bfin-elf-gdb -x commands "$1"
	ret=$?
	rm -f commands
	exit ${ret}
}

testit() {
	local name=$1 x=$2 y=`echo $2 | sed 's:\.[xX]$::'` out rsh_out addr
	shift; shift
	local fail=`grep xfail ${y}`
	if [ "${name}" = "HOST" -a ! -f $x ] ; then
		return
	fi
	printf '%-5s %-40s' ${name} ${x}
	out=`"$@" ${x} 2>&1`
	(pf "${out}")
	if [ $? -ne 0 ] ; then
		if [ "${name}" = "SIM" ] ; then
			tmp=`echo ${out} | awk '{print $3}' | sed 's/://'`
			tmp1=`expr index "${out}" "program stopped with signal 4"`
			if [ ${tmp1} -eq 1 ] ; then
				 printf 'illegal instruction\n'
			elif [ -n "${tmp}" ] ; then
				printf 'FAIL at line '
				addr=`echo $out | sed 's:^[A-Za-z ]*::' | sed 's:^0x[0-9][0-9] ::' | sed 's:^[A-Za-z ]*::' | awk '{print $1}'`
				bfin-elf-addr2line -e ${x} ${addr} | awk -F "/" '{print $NF}'
			fi
		elif [ "${name}" = "HOST" ] ; then
			rsh_out=`rsh -l root $boardip '/bin/dmesg -c | /bin/grep -e DBGA -e "FAULT "'`
			tmp=`echo ${rsh_out} | sed 's:\].*$::' | awk '{print $NF}' | awk -F ":" '{print $NF}'`
			if [ -n "${tmp}" ] ; then
				echo "${rsh_out}"
				printf 'FAIL at line '
				bfin-elf-addr2line -e ${x} $(echo ${rsh_out} | sed 's:\].*$::' | awk '{print $NF}') | awk -F "/" '{print $NF}'
			fi
		fi
		ret=$(( ret + 1 ))
		if [ -z "${fail}" ] ; then
			unexpected_fail=$(( unexpected_fail + 1 ))
			echo "!!!Expected Pass, but fail"
		fi
	else
		if [ ! -z "${fail}" ] ; then
			unexpected_pass=$(( unexpected_pass + 1 ))
			echo "!!!Expected fail, but pass"
		else
			expected_pass=$(( expected_pass + 1 ))
		fi
	fi
}

pf() {
	local ret=$?
	if [ ${ret} -eq 0 ] ; then
		echo "PASS"
	elif [ ${ret} -eq 77 ] ; then
		echo "SKIP $*"
	else
		echo "FAIL! $*"
		exit 1
	fi
}

[ $# -eq 0 ] && set -- *.[Ss]
bins_hw=$( (${run_sim} || ${run_jtag}) && printf '%s.x ' "$@")
if ${run_host} ; then
	for files in "$@" ; do
		tmp=`grep -e CYCLES -e TESTSET -e CLI -e STI -e RTX -e RTI -e SEQSTAT $files -l`
		if [ -z "${tmp}" ] ; then
			bins_host=`echo "${bins_host} ${files}.X"`
		else
			echo "skipping ${files}, since it isn't userspace friendly"
		fi
	done
fi
if [ -n "${bins_hw}" ] ; then
	bins_all="${bins_hw}"
fi

if [ -n "${bins_host}" ] ; then
	bins_all="${bins_all} ${bins_host}"
fi

if [ -z "${bins_all}" ] ; then
	exit
fi

printf 'Compiling tests: '
${MAKE} -s -j ${bins_all}
pf

if ${run_jtag} ; then
	printf 'Setting up gdbproxy (see gdbproxy.log): '
	killall -q bfin-gdbproxy
	bfin-gdbproxy -q bfin --reset --board=${boardjtag} --init-sdram >gdbproxy.log 2>&1 &
	t=0
	while [ ${t} -lt 5 ] ; do
		if netstat -nap 2>&1 | grep -q ^tcp.*:2000.*gdbproxy ; then
			break
		else
			: $(( t += 1 ))
			sleep 1
		fi
	done
	pf
fi

if ${run_host} ; then
	printf 'Uploading tests to board "%s": ' "${boardip}"
	rcp ${bins_host} root@${boardip}:/tmp/
	pf
	rsh -l root $boardip '/bin/dmesg -c' > /dev/null
fi

SIM="../../../bfin/run"
if [ ! -x ${SIM} ] ; then
	SIM="bfin-elf-run"
fi
echo "Using sim: ${SIM}"

ret=0
unexpected_fail=0
unexpected_pass=0
expected_pass=0
pids=()
for s in "$@" ; do
	(
	out=$(
	${run_sim}  && testit SIM  ${s}.x ${SIM} `sed -n '/^# sim:/s|^[^:]*:||p' ${s}`
	${run_jtag} && testit JTAG ${s}.x dojtag
	${run_host} && testit HOST ${s}.X dorsh
	)
	case $out in
	*PASS*) ;;
	*) echo "$out" ;;
	esac
	) &
	pids+=( $! )
	if [[ ${#pids[@]} -gt ${jobs} ]] ; then
		wait ${pids[0]}
		pids=( ${pids[@]:1} )
	fi
done
wait

killall -q bfin-gdbproxy
if [ ${ret} -eq 0 ] ; then
	rm -f gdbproxy.log
#	${MAKE} -s clean &
	exit 0
else
	echo number of failures ${ret}
	if [ ${unexpected_pass} -gt 0 ] ; then
		echo "Unexpected passes: ${unexpected_pass}"
	fi
	if [ ${unexpected_fail} -gt 0 ] ; then
		echo "Unexpected fails: ${unexpected_fail}"
	fi
	if [ ${expected_pass} -gt 0 ] ; then
		echo "passes : ${expected_pass}"
	fi
	exit 1
fi
