AC_MSG_CHECKING([for sim ppc bitsize settings])
AC_ARG_ENABLE(sim-ppc-bitsize,
[AS_HELP_STRING([--enable-sim-ppc-bitsize=n], [Specify target bitsize (32 or 64).])],
[case "${enableval}" in
  32|64) sim_ppc_bitsize="-DWITH_TARGET_WORD_BITSIZE=$enableval";;
  *)	 AC_MSG_ERROR("--enable-sim-ppc-bitsize was given $enableval.  Expected 32 or 64");;
esac], [sim_ppc_bitsize=""])
AC_MSG_RESULT($sim_ppc_bitsize)

AC_MSG_CHECKING([for sim ppc decode mechanism])
AC_ARG_ENABLE(sim-ppc-decode-mechanism,
[AS_HELP_STRING([--enable-sim-ppc-decode-mechanism=which], [Specify the instruction decode mechanism.])],
[case "${enableval}" in
  yes|no)	AC_MSG_ERROR("No value supplied for --enable-sim-ppc-decode-mechanism=file");;
  array|switch|padded-switch|goto-switch)	sim_ppc_decode_mechanism="-T ${enableval}";;
  *)		AC_MSG_ERROR("File $enableval is not an opcode rules file");;
esac], [sim_ppc_decode_mechanism=""])
AC_MSG_RESULT($sim_ppc_decode_mechanism)

AC_MSG_CHECKING([for sim ppc default model])
AC_ARG_ENABLE(sim-ppc-default-model,
[AS_HELP_STRING([--enable-sim-ppc-default-model=which], [Specify default PowerPC to model.])],
[case "${enableval}" in
  yes|no)	AC_MSG_ERROR("No value supplied for --enable-sim-ppc-default-model=model");;
  *)		sim_ppc_default_model="-DWITH_DEFAULT_MODEL=${enableval}";;
esac], [sim_ppc_default_model=""])
AC_MSG_RESULT($sim_ppc_default_model)

AC_MSG_CHECKING([for sim ppc duplicate settings])
AC_ARG_ENABLE(sim-ppc-duplicate,
[AS_HELP_STRING([--enable-sim-ppc-duplicate], [Expand (duplicate) semantic functions.])],
[case "${enableval}" in
  yes)	sim_ppc_dup="-E";;
  no)	sim_ppc_dup="";;
  *)	AC_MSG_ERROR("--enable-sim-ppc-duplicate does not take a value");;
esac], [sim_ppc_dup="-E"])
AC_MSG_RESULT($sim_ppc_dup)

AC_MSG_CHECKING([for sim ppc filter rules])
AC_ARG_ENABLE(sim-ppc-filter,
[AS_HELP_STRING([--enable-sim-ppc-filter=rule], [Specify filter rules.])],
[case "${enableval}" in
  yes)	AC_MSG_ERROR("--enable-sim-ppc-filter must be specified with a rule to filter or no");;
  no)	sim_ppc_filter="";;
  *)	sim_ppc_filter="-F $enableval";;
esac], [sim_ppc_filter="-F 32,f,o"])
AC_MSG_RESULT($sim_ppc_filter)

AC_MSG_CHECKING([for sim ppc float settings])
AC_ARG_ENABLE(sim-ppc-float,
[AS_HELP_STRING([--enable-sim-ppc-float], [Specify whether the target has hard, soft, altivec or e500 floating point.])],
[case "${enableval}" in
  yes | hard)	sim_ppc_float="-DWITH_FLOATING_POINT=HARD_FLOATING_POINT";;
  no | soft)	sim_ppc_float="-DWITH_FLOATING_POINT=SOFT_FLOATING_POINT";;
  altivec)      sim_ppc_float="-DWITH_ALTIVEC" ; sim_ppc_filter="${sim_ppc_filter},av" ;;
  *spe*|*simd*) sim_ppc_float="-DWITH_E500" ; sim_ppc_filter="${sim_ppc_filter},e500" ;;
  *)		AC_MSG_ERROR("Unknown value $enableval passed to --enable-sim-ppc-float");;
esac],
[case "${target}" in
  *altivec*) sim_ppc_float="-DWITH_ALTIVEC" ; sim_ppc_filter="${sim_ppc_filter},av" ;;
  *spe*|*simd*)	sim_ppc_float="-DWITH_E500" ; sim_ppc_filter="${sim_ppc_filter},e500" ;;
  *) sim_ppc_float=""
esac])
AC_MSG_RESULT($sim_ppc_float)

AC_MSG_CHECKING([for sim ppc hardware settings])
hardware="cpu,memory,nvram,iobus,htab,disk,trace,register,vm,init,core,pal,com,eeprom,opic,glue,phb,ide,sem,shm"
AC_ARG_ENABLE(sim-ppc-hardware,
[AS_HELP_STRING([--enable-sim-ppc-hardware=list], [Specify the hardware to be included in the build.])],
[case "${enableval}" in
  yes)	;;
  no)	AC_MSG_ERROR("List of hardware must be specified for --enable-sim-ppc-hardware");;
  ,*)   hardware="${hardware}${enableval}";;
  *,)   hardware="${enableval}${hardware}";;
  *)	hardware="${enableval}"'';;
esac])
sim_ppc_hw_src=`echo $hardware | sed -e 's/,/.c hw_/g' -e 's/^/hw_/' -e s'/$/.c/'`
sim_ppc_hw_obj=`echo $sim_ppc_hw_src | sed -e 's/\.c/.o/g'`
AC_MSG_RESULT($hardware)

AC_MSG_CHECKING([for sim ppc icache settings])
AC_ARG_ENABLE(sim-ppc-icache,
[AS_HELP_STRING([--enable-sim-ppc-icache=size], [Specify instruction-decode cache size and type.])],
[icache="-R"
 case "${enableval}" in
  yes)		icache="1024"; sim_ppc_icache="-I $icache";;
  no)		sim_ppc_icache="-R";;
  *) icache=1024
     sim_ppc_icache="-"
     for x in `echo "${enableval}" | sed -e "s/,/ /g"`; do
       case "$x" in
         define)	sim_ppc_icache="${sim_ppc_icache}R";;
         semantic)	sim_ppc_icache="${sim_ppc_icache}C";;
	 insn)		sim_ppc_icache="${sim_ppc_icache}S";;
	 0*|1*|2*|3*|4*|5*|6*|7*|8*|9*)	icache=$x;;
         *)		AC_MSG_ERROR("Unknown value $x for --enable-sim-ppc-icache");;
       esac
     done
     sim_ppc_icache="${sim_ppc_icache}I $icache";;
esac], [sim_ppc_icache="-CSRI 1024"])
AC_MSG_RESULT($sim_ppc_icache)

AC_MSG_CHECKING([for sim ppc jump settings])
AC_ARG_ENABLE(sim-ppc-jump,
[AS_HELP_STRING([--enable-sim-ppc-jump], [Jump between semantic code (instead of call/return).])],
[case "${enableval}" in
  yes)	sim_ppc_jump="-J";;
  no)	sim_ppc_jump="";;
  *)	AC_MSG_ERROR("--enable-sim-ppc-jump does not take a value");;
esac], [sim_ppc_jump=""])
AC_MSG_RESULT($sim_ppc_jump)

AC_MSG_CHECKING([for sim ppc source debug line numbers])
AC_ARG_ENABLE(sim-ppc-line-nr,
[AS_HELP_STRING([--enable-sim-ppc-line-nr=opts], [Generate extra CPP code that references source rather than generated code])],
[case "${enableval}" in
  yes)	sim_ppc_line_nr="";;
  no)	sim_ppc_line_nr="-L";;
  *)	AC_MSG_ERROR("--enable-sim-ppc-line-nr does not take a value");;
esac], [sim_ppc_line_nr=""])
AC_MSG_RESULT($sim_ppc_line_nr)

AC_MSG_CHECKING([for sim ppc model])
AC_ARG_ENABLE(sim-ppc-model,
[AS_HELP_STRING([--enable-sim-ppc-model=which], [Specify PowerPC to model.])],
[case "${enableval}" in
  yes|no)	AC_MSG_ERROR("No value supplied for --enable-sim-ppc-model=model");;
  *)		sim_ppc_model="-DWITH_MODEL=${enableval}";;
esac], [sim_ppc_model=""])
AC_MSG_RESULT($sim_ppc_model)

AC_MSG_CHECKING([for sim ppc model issue])
AC_ARG_ENABLE(sim-ppc-model-issue,
[AS_HELP_STRING([--enable-sim-ppc-model-issue], [Specify whether to simulate model specific actions])],
[case "${enableval}" in
  yes)	sim_ppc_model_issue="-DWITH_MODEL_ISSUE=MODEL_ISSUE_PROCESS";;
  no)	sim_ppc_model_issue="-DWITH_MODEL_ISSUE=MODEL_ISSUE_IGNORE";;
  *)	AC_MSG_ERROR("--enable-sim-ppc-model-issue does not take a value");;
esac], [sim_ppc_model_issue=""])
AC_MSG_RESULT($sim_ppc_model_issue)

AC_MSG_CHECKING([for sim ppc event monitoring])
AC_ARG_ENABLE(sim-ppc-monitor,
[AS_HELP_STRING([--enable-sim-ppc-monitor=mon], [Specify whether to enable monitoring events.])],
[case "${enableval}" in
  yes)		sim_ppc_monitor="-DWITH_MON='MONITOR_INSTRUCTION_ISSUE | MONITOR_LOAD_STORE_UNIT'";;
  no)		sim_ppc_monitor="-DWITH_MON=0";;
  instruction)	sim_ppc_monitor="-DWITH_MON=MONITOR_INSTRUCTION_ISSUE";;
  memory)	sim_ppc_monitor="-DWITH_MON=MONITOR_LOAD_STORE_UNIT";;
  *)		AC_MSG_ERROR("Unknown value $enableval passed to --enable-sim-ppc-mon");;
esac], [sim_ppc_monitor=""])
AC_MSG_RESULT($sim_ppc_monitor)

AC_MSG_CHECKING([for sim ppc opcode lookup settings])
AC_ARG_ENABLE(sim-ppc-opcode,
[AS_HELP_STRING([--enable-sim-ppc-opcode=which], [Override default opcode lookup.])],
[case "${enableval}" in
  yes|no)	AC_MSG_ERROR("No value supplied for --enable-sim-ppc-opcode=file");;
  *)		if test -f "${srcdir}/${enableval}"; then
		  sim_ppc_opcode="${enableval}"
		elif test -f "${srcdir}/dc-${enableval}"; then
		  sim_ppc_opcode="dc-${enableval}"
		else
		  AC_MSG_ERROR("File $enableval is not an opcode rules file")
		fi;;
esac], [sim_ppc_opcode="dc-complex"])
AC_MSG_RESULT($sim_ppc_opcode)

AC_MSG_CHECKING([for sim ppc smp settings])
AC_ARG_ENABLE(sim-ppc-smp,
[AS_HELP_STRING([--enable-sim-ppc-smp=n], [Specify number of processors to configure for.])],
[case "${enableval}" in
  yes)	sim_ppc_smp="-DWITH_SMP=5" ; sim_ppc_igen_smp="-N 5";;
  no)	sim_ppc_smp="-DWITH_SMP=0" ; sim_ppc_igen_smp="-N 0";;
  *)	sim_ppc_smp="-DWITH_SMP=$enableval" ; sim_ppc_igen_smp="-N $enableval";;
esac], [sim_ppc_smp="-DWITH_SMP=5" ; sim_ppc_igen_smp="-N 5"])
AC_MSG_RESULT($sim_ppc_smp)

AC_MSG_CHECKING([for sim ppc switch table settings])
AC_ARG_ENABLE(sim-ppc-switch,
[AS_HELP_STRING([--enable-sim-ppc-switch], [Use a switch instead of a table for instruction call.])],
[case "${enableval}" in
  yes)	sim_ppc_switch="-DWITH_SPREG_SWITCH_TABLE";;
  no)	sim_ppc_switch="";;
  *)	AC_MSG_ERROR("--enable-sim-ppc-switch does not take a value");;
esac], [sim_ppc_switch=""])
AC_MSG_RESULT($sim_ppc_switch)

AC_MSG_CHECKING([for sim ppc timebase])
AC_ARG_ENABLE(sim-ppc-timebase,
[AS_HELP_STRING([--enable-sim-ppc-timebase], [Specify whether the PPC timebase is supported.])],
[case "${enableval}" in
  yes)	sim_ppc_timebase="-DWITH_TIME_BASE=1";;
  no)	sim_ppc_timebase="-DWITH_TIME_BASE=0";;
  *)	AC_MSG_ERROR("--enable-sim-ppc-timebase does not take a value");;
esac], [sim_ppc_timebase=""])
AC_MSG_RESULT($sim_ppc_timebase)

AC_MSG_CHECKING([for sim ppc xor endian settings])
AC_ARG_ENABLE(sim-ppc-xor-endian,
[AS_HELP_STRING([--enable-sim-ppc-xor-endian=n], [Specify number bytes involved in PowerPC XOR bi-endian mode (default 8).])],
[case "${enableval}" in
  yes)	sim_ppc_xor_endian="-DWITH_XOR_ENDIAN=8";;
  no)	sim_ppc_xor_endian="-DWITH_XOR_ENDIAN=0";;
  *)	sim_ppc_xor_endian="-DWITH_XOR_ENDIAN=$enableval";;
esac], [sim_ppc_xor_endian=""])
AC_MSG_RESULT($sim_ppc_xor_endian)

AC_SUBST(sim_ppc_line_nr)
AC_SUBST(sim_ppc_opcode)
AC_SUBST(sim_ppc_switch)
AC_SUBST(sim_ppc_dup)
AC_SUBST(sim_ppc_decode_mechanism)
AC_SUBST(sim_ppc_jump)
AC_SUBST(sim_ppc_filter)
AC_SUBST(sim_ppc_icache)
AC_SUBST(sim_ppc_hw_src)
AC_SUBST(sim_ppc_hw_obj)
AC_SUBST(sim_ppc_xor_endian)
AC_SUBST(sim_ppc_smp)
AC_SUBST(sim_ppc_igen_smp)
AC_SUBST(sim_ppc_bitsize)
AC_SUBST(sim_ppc_timebase)
AC_SUBST(sim_ppc_float)
AC_SUBST(sim_ppc_monitor)
AC_SUBST(sim_ppc_model)
AC_SUBST(sim_ppc_default_model)
AC_SUBST(sim_ppc_model_issue)
