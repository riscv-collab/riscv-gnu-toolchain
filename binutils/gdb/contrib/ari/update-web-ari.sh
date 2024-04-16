#!/bin/sh -x

# GDB script to create GDB ARI web page.
#
# Copyright (C) 2001-2024 Free Software Foundation, Inc.
#
# This file is part of GDB.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# TODO: setjmp.h, setjmp and longjmp.

# Direct stderr into stdout but still hang onto stderr (/dev/fd/3)
exec 3>&2 2>&1
ECHO ()
{
#   echo "$@" | tee /dev/fd/3 1>&2
    echo "$@" 1>&2
    echo "$@" 1>&3
}

# Really mindless usage
if test $# -ne 4
then
    echo "Usage: $0 <snapshot/sourcedir> <tmpdir> <destdir> <project>" 1>&2
    exit 1
fi
snapshot=$1 ; shift
tmpdir=$1 ; shift
wwwdir=$1 ; shift
project=$1 ; shift

# Try to create destination directory if it doesn't exist yet
if [ ! -d ${wwwdir} ]
then
  mkdir -p ${wwwdir}
fi

# Fail if destination directory doesn't exist or is not writable
if [ ! -w ${wwwdir} -o ! -d ${wwwdir} ]
then
  echo ERROR: Can not write to directory ${wwwdir} >&2
  exit 2
fi

if [ ! -r ${snapshot} ]
then
    echo ERROR: Can not read snapshot file 1>&2
    exit 1
fi

# FILE formats
# ari.*.bug: <FILE>:<LINE>: <CATEGORY>: <BUG>: <DOC>
# ari.*.idx: <BUG>:<FILE>:<CATEGORY>
# ari.*.doc: <BUG>:<COUNT>:<CATEGORY>:<DOC>
# Where ``*'' is {source,warning,indent,doschk}

unpack_source_p=true
delete_source_p=true

check_warning_p=false # broken
check_indent_p=false # too slow, too many fail
check_source_p=true
check_doschk_p=true
check_werror_p=true

update_doc_p=true
update_web_p=true

if awk --version 2>&1 </dev/null | grep -i gnu > /dev/null
then
  AWK=awk
else
  AWK=gawk
fi
export AWK

# Set up a few cleanups
if ${delete_source_p}
then
    trap "cd /tmp; rm -rf ${tmpdir}; exit" 0 1 2 15
fi


# If the first parameter is a directory,
#we just use it as the extracted source
if [ -d ${snapshot} ]
then
  module=${project}
  srcdir=${snapshot}
  aridir=${srcdir}/${module}/contrib/ari
  unpack_source_p=false
  delete_source_p=false
  version_in=${srcdir}/${module}/version.in
else
  # unpack the tar-ball
  if ${unpack_source_p}
  then
    # Was it previously unpacked?
    if ${delete_source_p} || test ! -d ${tmpdir}/${module}*
    then
	/bin/rm -rf "${tmpdir}"
	/bin/mkdir -p ${tmpdir}
	if [ ! -d ${tmpdir} ]
	then
	    echo "Problem creating work directory"
	    exit 1
	fi
	cd ${tmpdir} || exit 1
	echo `date`: Unpacking tar-ball ...
	case ${snapshot} in
	    *.tar.bz2 ) bzcat ${snapshot} ;;
	    *.tar ) cat ${snapshot} ;;
	    * ) ECHO Bad file ${snapshot} ; exit 1 ;;
	esac | tar xf -
    fi
  fi

  module=`basename ${snapshot}`
  module=`basename ${module} .bz2`
  module=`basename ${module} .tar`
  srcdir=`echo ${tmpdir}/${module}*`
  aridir=${HOME}/ss
  version_in=${srcdir}/gdb/version.in
fi

if [ ! -r ${version_in} ]
then
    echo ERROR: missing version file 1>&2
    exit 1
fi

date=`sed -n -e 's/^.* BFD_VERSION_DATE \(.*\)$/\1/p' $srcdir/bfd/version.h`
version=`sed -e "s/DATE/$date/" < ${version_in}`

# THIS HAS SUFFERED BIT ROT
if ${check_warning_p} && test -d "${srcdir}"
then
    echo `date`: Parsing compiler warnings 1>&2
    cat ${root}/ari.compile | $AWK '
BEGIN {
    FS=":";
}
/^[^:]*:[0-9]*: warning:/ {
  file = $1;
  #sub (/^.*\//, "", file);
  warning[file] += 1;
}
/^[^:]*:[0-9]*: error:/ {
  file = $1;
  #sub (/^.*\//, "", file);
  error[file] += 1;
}
END {
  for (file in warning) {
    print file ":warning:" level[file]
  }
  for (file in error) {
    print file ":error:" level[file]
  }
}
' > ${root}/ari.warning.bug
fi

# THIS HAS SUFFERED BIT ROT
if ${check_indent_p} && test -d "${srcdir}"
then
    printf "Analizing file indentation:" 1>&2
    ( cd "${srcdir}" && /bin/sh ${aridir}/gdb_find.sh ${project} | while read f
    do
	if /bin/sh ${aridir}/gdb_indent.sh < ${f} 2>/dev/null | cmp -s - ${f}
	then
	    :
	else
	    # ari.*.bug: <FILE>:<LINE>: <CATEGORY>: <BUG>: <DOC>
	    echo "${f}:0: info: indent: Indentation does not match GNU indent output"
	fi
    done ) > ${wwwdir}/ari.indent.bug
    echo ""
fi

if ${check_source_p} && test -d "${srcdir}"
then
    bugf=${wwwdir}/ari.source.bug
    oldf=${wwwdir}/ari.source.old
    srcf=${wwwdir}/ari.source.lines
    oldsrcf=${wwwdir}/ari.source.lines-old

    diff=${wwwdir}/ari.source.diff
    diffin=${diff}-in
    newf1=${bugf}1
    oldf1=${oldf}1
    oldpruned=${oldf1}-pruned
    newpruned=${newf1}-pruned

    cp -f ${bugf} ${oldf}
    cp -f ${srcf} ${oldsrcf}
    rm -f ${srcf}
    node=`uname -n`
    echo "`date`: Using source lines ${srcf}" 1>&2
    echo "`date`: Checking source code" 1>&2
    ( cd "${srcdir}" && /bin/sh ${aridir}/gdb_find.sh "${project}" | \
	xargs /bin/sh ${aridir}/gdb_ari.sh -Werror -Wall --print-idx --src=${srcf}
    ) > ${bugf}
    # Remove things we are not interested in to signal by email
    # gdbarch changes are not important here
    # Also convert ` into ' to avoid command substitution in script below
    sed -e "/.*: gdbarch:.*/d" -e "s:\`:':g" ${oldf} > ${oldf1}
    sed -e "/.*: gdbarch:.*/d" -e "s:\`:':g" ${bugf} > ${newf1}
    # Remove line number info so that code inclusion/deletion
    # has no impact on the result
    sed -e "s/\([^:]*\):\([^:]*\):\(.*\)/\1:0:\3/" ${oldf1} > ${oldpruned}
    sed -e "s/\([^:]*\):\([^:]*\):\(.*\)/\1:0:\3/" ${newf1} > ${newpruned}
    # Use diff without option to get normal diff output that
    # is reparsed after
    diff ${oldpruned} ${newpruned} > ${diffin}
    # Only keep new warnings
    sed -n -e "/^>.*/p" ${diffin} > ${diff}
    sedscript=${wwwdir}/sedscript
    script=${wwwdir}/script
    sed -n -e "s|\(^[0-9,]*\)a\(.*\)|echo \1a\2 \n \
	sed -n \'\2s:\\\\(.*\\\\):> \\\\1:p\' ${newf1}|p" \
	-e "s|\(^[0-9,]*\)d\(.*\)|echo \1d\2\n \
	sed -n \'\1s:\\\\(.*\\\\):< \\\\1:p\' ${oldf1}|p" \
	-e "s|\(^[0-9,]*\)c\(.*\)|echo \1c\2\n \
	sed -n \'\1s:\\\\(.*\\\\):< \\\\1:p\' ${oldf1} \n \
	sed -n \"\2s:\\\\(.*\\\\):> \\\\1:p\" ${newf1}|p" \
	${diffin} > ${sedscript}
    ${SHELL} ${sedscript} > ${wwwdir}/message
    sed -n \
	-e "s;\(.*\);echo \\\"\1\\\";p" \
	-e "s;.*< \([^:]*\):\([0-9]*\):.*;grep \"^\1:\2:\" ${oldsrcf};p" \
	-e "s;.*> \([^:]*\):\([0-9]*\):.*;grep \"^\1:\2:\" ${srcf};p" \
	${wwwdir}/message > ${script}
    ${SHELL} ${script} > ${wwwdir}/mail-message
    if [ "x${branch}" != "x" ]; then
	email_suffix="`date` in ${branch}"
    else
	email_suffix="`date`"
    fi

fi




if ${check_doschk_p} && test -d "${srcdir}"
then
    echo "`date`: Checking for doschk" 1>&2
    rm -f "${wwwdir}"/ari.doschk.*
    fnchange_lst="${srcdir}"/gdb/config/djgpp/fnchange.lst
    fnchange_awk="${wwwdir}"/ari.doschk.awk
    doschk_in="${wwwdir}"/ari.doschk.in
    doschk_out="${wwwdir}"/ari.doschk.out
    doschk_bug="${wwwdir}"/ari.doschk.bug
    doschk_char="${wwwdir}"/ari.doschk.char

    # Transform fnchange.lst into fnchange.awk.  The program DJTAR
    # does a textual substitution of each file name using the list.
    # Generate an awk script that does the equivalent - matches an
    # exact line and then outputs the replacement.

    sed -e 's;@[^@]*@[/]*\([^ ]*\) @[^@]*@[/]*\([^ ]*\);\$0 == "\1" { print "\2"\; next\; };' \
	< "${fnchange_lst}" > "${fnchange_awk}"
    echo '{ print }' >> "${fnchange_awk}"

    # Do the raw analysis - transform the list of files into the DJGPP
    # equivalents putting it in the .in file
    ( cd "${srcdir}" && find * \
	-name '*.info-[0-9]*' -prune \
	-o -name tcl -prune \
	-o -name itcl -prune \
	-o -name tk -prune \
	-o -name libgui -prune \
	-o -name tix -prune \
	-o -name dejagnu -prune \
	-o -name expect -prune \
	-o -type f -print ) \
    | $AWK -f ${fnchange_awk} > ${doschk_in}

    # Start with a clean slate
    rm -f ${doschk_bug}

    # Check for any invalid characters.
    grep '[\+\,\;\=\[\]\|\<\>\\\"\:\?\*]' < ${doschk_in} > ${doschk_char}
    # ari.*.bug: <FILE>:<LINE>: <CATEGORY>: <BUG>: <DOC>
    sed < ${doschk_char} >> ${doschk_bug} \
	-e s'/$/:0: dos: DOSCHK: Invalid DOS character/'

    # Magic to map ari.doschk.out to ari.doschk.bug goes here
    doschk < ${doschk_in} > ${doschk_out}
    cat ${doschk_out} | $AWK >> ${doschk_bug} '
BEGIN {
    state = 1;
    invalid_dos = state++; bug[invalid_dos] = "invalid DOS file name";  category[invalid_dos] = "dos";
    same_dos = state++;    bug[same_dos]    = "DOS 8.3";                category[same_dos] = "dos";
    same_sysv = state++;   bug[same_sysv]   = "SysV";
    long_sysv = state++;   bug[long_sysv]   = "long SysV";
    internal = state++;    bug[internal]    = "internal doschk";        category[internal] = "internal";
    state = 0;
}
/^$/ { state = 0; next; }
/^The .* not valid DOS/     { state = invalid_dos; next; }
/^The .* same DOS/          { state = same_dos; next; }
/^The .* same SysV/         { state = same_sysv; next; }
/^The .* too long for SysV/ { state = long_sysv; next; }
/^The .* /                  { state = internal; next; }

NF == 0 { next }

NF == 3 { name = $1 ; file = $3 }
NF == 1 { file = $1 }
NF > 3 && $2 == "-" { file = $1 ; name = gensub(/^.* - /, "", 1) }

state == same_dos {
    # ari.*.bug: <FILE>:<LINE>: <CATEGORY>: <BUG>: <DOC>
    print  file ":0: " category[state] ": " \
	name " " bug[state] " " " dup: " \
	" DOSCHK - the names " name " and " file " resolve to the same" \
	" file on a " bug[state] \
	" system.<br>For DOS, this can be fixed by modifying the file" \
	" fnchange.lst."
    next
}
state == invalid_dos {
    # ari.*.bug: <FILE>:<LINE>: <SEVERITY>: <CATEGORY>: <DOC>
    print file ":0: " category[state] ": "  name ": DOSCHK - " name
    next
}
state == internal {
    # ari.*.bug: <FILE>:<LINE>: <SEVERITY>: <CATEGORY>: <DOC>
    print file ":0: " category[state] ": "  bug[state] ": DOSCHK - a " \
	bug[state] " problem"
}
'
fi



if ${check_werror_p} && test -d "${srcdir}"
then
    echo "`date`: Checking Makefile.in for non- -Werror rules"
    rm -f ${wwwdir}/ari.werror.*
    cat "${srcdir}/${project}/Makefile.in" | $AWK > ${wwwdir}/ari.werror.bug '
BEGIN {
    count = 0
    cont_p = 0
    full_line = ""
}
/^[-_[:alnum:]]+\.o:/ {
    file = gensub(/.o:.*/, "", 1) ".c"
}

/[^\\]\\$/ { gsub (/\\$/, ""); full_line = full_line $0; cont_p = 1; next; }
cont_p { $0 = full_line $0; cont_p = 0; full_line = ""; }

/\$\(COMPILE\.pre\)/ {
    print file " has  line " $0
    if (($0 !~ /\$\(.*ERROR_CFLAGS\)/) && ($0 !~ /\$\(INTERNAL_CFLAGS\)/)) {
	# ari.*.bug: <FILE>:<LINE>: <CATEGORY>: <BUG>: <DOC>
	print "'"${project}"'/" file ":0: info: Werror: The file is not being compiled with -Werror"
    }
}
'
fi


# From the warnings, generate the doc and indexed bug files
if ${update_doc_p}
then
    cd ${wwwdir}
    rm -f ari.doc ari.idx ari.doc.bug
    # Generate an extra file containing all the bugs that the ARI can detect.
    /bin/sh ${aridir}/gdb_ari.sh -Werror -Wall --print-idx --print-doc >> ari.doc.bug
    cat ari.*.bug | $AWK > ari.idx '
BEGIN {
    FS=": *"
}
{
    # ari.*.bug: <FILE>:<LINE>: <CATEGORY>: <BUG>: <DOC>
    file = $1
    line = $2
    category = $3
    bug = $4
    if (! (bug in cat)) {
	cat[bug] = category
	# strip any trailing .... (supplement)
	doc[bug] = gensub(/ \([^\)]*\)$/, "", 1, $5)
	count[bug] = 0
    }
    if (file != "") {
	count[bug] += 1
	# ari.*.idx: <BUG>:<FILE>:<CATEGORY>
	print bug ":" file ":" category
    }
    # Also accumulate some categories as obsolete
    if (category == "deprecated") {
	# ari.*.idx: <BUG>:<FILE>:<CATEGORY>
	if (file != "") {
	    print category ":" file ":" "obsolete"
	}
	#count[category]++
	#doc[category] = "Contains " category " code"
    }
}
END {
    i = 0;
    for (bug in count) {
	# ari.*.doc: <BUG>:<COUNT>:<CATEGORY>:<DOC>
	print bug ":" count[bug] ":" cat[bug] ":" doc[bug] >> "ari.doc"
    }
}
'
fi


# print_toc BIAS MIN_COUNT CATEGORIES TITLE

# Print a table of contents containing the bugs CATEGORIES.  If the
# BUG count >= MIN_COUNT print it in the table-of-contents.  If
# MIN_COUNT is non -ve, also include a link to the table.Adjust the
# printed BUG count by BIAS.

all=

print_toc ()
{
    bias="$1" ; shift
    min_count="$1" ; shift

    all=" $all $1 "
    categories=""
    for c in $1; do
	categories="${categories} categories[\"${c}\"] = 1 ;"
    done
    shift

    title="$@" ; shift

    echo "<p>" >> ${newari}
    echo "<a name=${title}>" | tr '[A-Z]' '[a-z]' >> ${newari}
    echo "<h3>${title}</h3>" >> ${newari}
    cat >> ${newari} # description

    cat >> ${newari} <<EOF
<p>
<table>
<tr><th align=left>BUG</th><th>Total</th><th align=left>Description</th></tr>
EOF
    # ari.*.doc: <BUG>:<COUNT>:<CATEGORY>:<DOC>
    cat ${wwwdir}/ari.doc \
    | sort -t: +1rn -2 +0d \
    | $AWK >> ${newari} '
BEGIN {
    FS=":"
    '"$categories"'
    MIN_COUNT = '${min_count}'
    BIAS = '${bias}'
    total = 0
    nr = 0
}
{
    # ari.*.doc: <BUG>:<COUNT>:<CATEGORY>:<DOC>
    bug = $1
    count = $2
    category = $3
    doc = $4
    if (count < MIN_COUNT) next
    if (!(category in categories)) next
    nr += 1
    total += count
    printf "<tr>"
    printf "<th align=left valign=top><a name=\"%s\">", bug
    printf "%s", gensub(/_/, " ", "g", bug)
    printf "</a></th>"
    printf "<td align=right valign=top>"
    if (count > 0 && MIN_COUNT >= 0) {
	printf "<a href=\"#,%s\">%d</a></td>", bug, count + BIAS
    } else {
	printf "%d", count + BIAS
    }
    printf "</td>"
    printf "<td align=left valign=top>%s</td>", doc
    printf "</tr>"
    print ""
}
END {
    print "<tr><th align=right valign=top>" nr "</th><th align=right valign=top>" total "</th><td></td></tr>"
}
'
cat >> ${newari} <<EOF
</table>
<p>
EOF
}


print_table ()
{
    categories=""
    for c in $1; do
	categories="${categories} categories[\"${c}\"] = 1 ;"
    done
    # Remember to prune the dir prefix from projects files
    # ari.*.idx: <BUG>:<FILE>:<CATEGORY>
    cat ${wwwdir}/ari.idx | $AWK >> ${newari} '
function qsort (table,
		middle, tmp, left, nr_left, right, nr_right, result) {
    middle = ""
    for (middle in table) { break; }
    nr_left = 0;
    nr_right = 0;
    for (tmp in table) {
	if (tolower(tmp) < tolower(middle)) {
	    nr_left++
	    left[tmp] = tmp
	} else if (tolower(tmp) > tolower(middle)) {
	    nr_right++
	    right[tmp] = tmp
	}
    }
    #print "qsort " nr_left " " middle " " nr_right > "/dev/stderr"
    result = ""
    if (nr_left > 0) {
	result = qsort(left) SUBSEP
    }
    result = result middle
    if (nr_right > 0) {
	result = result SUBSEP qsort(right)
    }
    return result
}
function print_heading (nb_file, where, bug_i) {
    print ""
    print "<tr border=1>"
    print "<th align=left>File " nb_file "</th>"
    print "<th align=left><em>Total</em></th>"
    print "<th></th>"
    for (bug_i = 1; bug_i <= nr_bug; bug_i++) {
	bug = i2bug[bug_i];
	printf "<th>"
	# The title names are offset by one.  Otherwize, when the browser
	# jumps to the name it leaves out half the relevant column.
	#printf "<a name=\",%s\">&nbsp;</a>", bug
	printf "<a name=\",%s\">&nbsp;</a>", i2bug[bug_i-1]
	printf "<a href=\"#%s\">", bug
	printf "%s", gensub(/_/, " ", "g", bug)
	printf "</a>\n"
	printf "</th>\n"
    }
    #print "<th></th>"
    printf "<th><a name=\"%s,\">&nbsp;</a></th>\n", i2bug[bug_i-1]
    print "<th align=left><em>Total</em></th>"
    print "<th align=left>File " nb_file "</th>"
    print "</tr>"
}
function print_totals (where, bug_i) {
    print "<th align=left><em>Totals</em></th>"
    printf "<th align=right>"
    printf "<em>%s</em>", total
    printf "&gt;"
    printf "</th>\n"
    print "<th></th>";
    for (bug_i = 1; bug_i <= nr_bug; bug_i++) {
	bug = i2bug[bug_i];
	printf "<th align=right>"
	printf "<em>"
	printf "<a href=\"#%s\">%d</a>", bug, bug_total[bug]
	printf "</em>";
	printf "<a href=\"#%s,%s\">^</a>", prev_file[bug, where], bug
	printf "<a href=\"#%s,%s\">v</a>", next_file[bug, where], bug
	printf "<a name=\"%s,%s\">&nbsp;</a>", where, bug
	printf "</th>";
	print ""
    }
    print "<th></th>"
    printf "<th align=right>"
    printf "<em>%s</em>", total
    printf "&lt;"
    printf "</th>\n"
    print "<th align=left><em>Totals</em></th>"
    print "</tr>"
}
BEGIN {
    FS = ":"
    '"${categories}"'
    nr_file = 0;
    nr_bug = 0;
}
{
    # ari.*.idx: <BUG>:<FILE>:<CATEGORY>
    bug = $1
    file = $2
    category = $3
    # Interested in this
    if (!(category in categories)) next
    # Totals
    db[bug, file] += 1
    bug_total[bug] += 1
    file_total[file] += 1
    total += 1
}
END {

    # Sort the files and bugs creating indexed lists.
    nr_bug = split(qsort(bug_total), i2bug, SUBSEP);
    nr_file = split(qsort(file_total), i2file, SUBSEP);

    # Dummy entries for first/last
    i2file[0] = 0
    i2file[-1] = -1
    i2bug[0] = 0
    i2bug[-1] = -1

    # Construct a cycle of next/prev links.  The file/bug "0" and "-1"
    # are used to identify the start/end of the cycle.  Consequently,
    # prev(0) = -1 (prev of start is the end) and next(-1) = 0 (next
    # of end is the start).

    # For all the bugs, create a cycle that goes to the prev / next file.
    for (bug_i = 1; bug_i <= nr_bug; bug_i++) {
	bug = i2bug[bug_i]
	prev = 0
	prev_file[bug, 0] = -1
	next_file[bug, -1] = 0
	for (file_i = 1; file_i <= nr_file; file_i++) {
	    file = i2file[file_i]
	    if ((bug, file) in db) {
		prev_file[bug, file] = prev
		next_file[bug, prev] = file
		prev = file
	    }
	}
	prev_file[bug, -1] = prev
	next_file[bug, prev] = -1
    }

    # For all the files, create a cycle that goes to the prev / next bug.
    for (file_i = 1; file_i <= nr_file; file_i++) {
	file = i2file[file_i]
	prev = 0
	prev_bug[file, 0] = -1
	next_bug[file, -1] = 0
	for (bug_i = 1; bug_i <= nr_bug; bug_i++) {
	    bug = i2bug[bug_i]
	    if ((bug, file) in db) {
		prev_bug[file, bug] = prev
		next_bug[file, prev] = bug
		prev = bug
	    }
	}
	prev_bug[file, -1] = prev
	next_bug[file, prev] = -1
    }

    print "<table border=1 cellspacing=0>"
    print "<tr></tr>"
    print_heading(nr_file, 0);
    print "<tr></tr>"
    print_totals(0);
    print "<tr></tr>"

    for (file_i = 1; file_i <= nr_file; file_i++) {
	file = i2file[file_i];
	pfile = gensub(/^'${project}'\//, "", 1, file)
	print ""
	print "<tr>"
	print "<th align=left><a name=\"" file ",\">" pfile "</a></th>"
	printf "<th align=right>"
	printf "%s", file_total[file]
	printf "<a href=\"#%s,%s\">&gt;</a>", file, next_bug[file, 0]
	printf "</th>\n"
	print "<th></th>"
	for (bug_i = 1; bug_i <= nr_bug; bug_i++) {
	    bug = i2bug[bug_i];
	    if ((bug, file) in db) {
		printf "<td align=right>"
		printf "<a href=\"#%s\">%d</a>", bug, db[bug, file]
		printf "<a href=\"#%s,%s\">^</a>", prev_file[bug, file], bug
		printf "<a href=\"#%s,%s\">v</a>", next_file[bug, file], bug
		printf "<a name=\"%s,%s\">&nbsp;</a>", file, bug
		printf "</td>"
		print ""
	    } else {
		print "<td>&nbsp;</td>"
		#print "<td></td>"
	    }
	}
	print "<th></th>"
	printf "<th align=right>"
	printf "%s", file_total[file]
	printf "<a href=\"#%s,%s\">&lt;</a>", file, prev_bug[file, -1]
	printf "</th>\n"
	print "<th align=left>" pfile "</th>"
	print "</tr>"
    }

    print "<tr></tr>"
    print_totals(-1)
    print "<tr></tr>"
    print_heading(nr_file, -1);
    print "<tr></tr>"
    print ""
    print "</table>"
    print ""
}
'
}


# Make the scripts available
cp ${aridir}/gdb_*.sh ${wwwdir}

nb_files=`cd "${srcdir}" && /bin/sh ${aridir}/gdb_find.sh "${project}" | wc -l`

echo "Total number of tested files is $nb_files"

if [ "x$debug_awk" = "x" ]
then
  debug_awk=0
fi

# Compute the ARI index - ratio of zero vs non-zero problems.
indexes=`${AWK} -v debug=${debug_awk} -v nr="$nb_files" '
BEGIN {
    FS=":"
}
{
    # ari.*.doc: <BUG>:<COUNT>:<CATEGORY>:<DOC>
    bug = $1; count = $2; category = $3; doc = $4

    # legacy type error have at least one entry,
    #corresponding to the declaration.
    if (bug ~ /^legacy /) legacy++
    # Idem for deprecated_XXX symbols/functions.
    if (bug ~ /^deprecated /) deprecated++

    if (category !~ /^gdbarch$/) {
	bugs += count
	nrtests += 1
    }
    if (count == 0) {
	oks++
    }
}
END {
    if (debug == 1) {
      print "nb files: " nr
      print "tests/oks: " nrtests "/" oks
      print "bugs/tests: " bugs "/" nrtests
      print "bugs/oks: " bugs "/" oks
      print bugs "/ (" oks "+" legacy "+" deprecated ")"
    }
    # This value should be as low as possible 
    print bugs / ( oks + legacy + deprecated )
}
' ${wwwdir}/ari.doc`

# Merge, generating the ARI tables.
if ${update_web_p}
then
    echo "Create the ARI table" 1>&2
    oldari=${wwwdir}/old.html
    ari=${wwwdir}/index.html
    newari=${wwwdir}/new.html
    rm -f ${newari} ${newari}.gz
    cat <<EOF >> ${newari}
<html>
<head>
<title>A.R. Index for GDB version ${version}</title>
</head>
<body>

<center><h2>A.R. Index for GDB version ${version}<h2></center>

<!-- body, update above using ../index.sh -->

<!-- Navigation.  This page contains the following anchors.
"BUG": The definition of the bug.
"FILE,BUG": The row/column containing FILEs BUG count
"0,BUG", "-1,BUG": The top/bottom total for BUGs column.
"FILE,O", "FILE,-1": The left/right total for FILEs row.
",BUG": The top title for BUGs column.
"FILE,": The left title for FILEs row.
-->

<center><h3>${indexes}</h3></center>
<center><h3>You can not take this seriously!</h3></center>

<center>
Also available:
<a href="../gdb/ari/">most recent branch</a>
|
<a href="../gdb/current/ari/">current</a>
|
<a href="../gdb/download/ari/">last release</a>
</center>

<center>
Last updated: `date -u`
</center>
EOF

    print_toc 0 1 "internal regression" Critical <<EOF
Things previously eliminated but returned.  This should always be empty.
EOF

    print_table "regression code comment obsolete gettext"

    print_toc 0 0 code Code <<EOF
Coding standard problems, portability problems, readability problems.
EOF

    print_toc 0 0 comment Comments <<EOF
Problems concerning comments in source files.
EOF

    print_toc 0 0 gettext GetText <<EOF
Gettext related problems.
EOF

    print_toc 0 -1 dos DOS 8.3 File Names <<EOF
File names with problems on 8.3 file systems.
EOF

    print_toc -2 -1 deprecated Deprecated <<EOF
Mechanisms that have been replaced with something better, simpler,
cleaner; or are no longer required by core-GDB.  New code should not
use deprecated mechanisms.  Existing code, when touched, should be
updated to use non-deprecated mechanisms.  See obsolete and deprecate.
(The declaration and definition are hopefully excluded from count so
zero should indicate no remaining uses).
EOF

    print_toc 0 0 obsolete Obsolete <<EOF
Mechanisms that have been replaced, but have not yet been marked as
such (using the deprecated_ prefix).  See deprecate and deprecated.
EOF

    print_toc 0 -1 deprecate Deprecate <<EOF
Mechanisms that are a candidate for being made obsolete.  Once core
GDB no longer depends on these mechanisms and/or there is a
replacement available, these mechanims can be deprecated (adding the
deprecated prefix) obsoleted (put into category obsolete) or deleted.
See obsolete and deprecated.
EOF

    print_toc -2 -1 legacy Legacy <<EOF
Methods used to prop up targets using targets that still depend on
deprecated mechanisms. (The method's declaration and definition are
hopefully excluded from count).
EOF

    print_toc -2 -1 gdbarch Gdbarch <<EOF
Count of calls to the gdbarch set methods.  (Declaration and
definition hopefully excluded from count).
EOF

    print_toc 0 -1 macro Macro <<EOF
Breakdown of macro definitions (and #undef) in configuration files.
EOF

    print_toc 0 0 regression Fixed <<EOF
Problems that have been expunged from the source code.
EOF

    # Check for invalid categories
    for a in $all; do
	alls="$alls all[$a] = 1 ;"
    done
    cat ari.*.doc | $AWK >> ${newari} '
BEGIN {
    FS = ":"
    '"$alls"'
}
{
    # ari.*.doc: <BUG>:<COUNT>:<CATEGORY>:<DOC>
    bug = $1
    count = $2
    category = $3
    doc = $4
    if (!(category in all)) {
	print "<b>" category "</b>: no documentation<br>"
    }
}
'

    cat >> ${newari} <<EOF
<center>
Input files:
`( cd ${wwwdir} && ls ari.*.bug ari.idx ari.doc ) | while read f
do
    echo "<a href=\"${f}\">${f}</a>"
done`
</center>

<center>
Scripts:
`( cd ${wwwdir} && ls *.sh ) | while read f
do
    echo "<a href=\"${f}\">${f}</a>"
done`
</center>

<!-- /body, update below using ../index.sh -->
</body>
</html>
EOF

    for i in . .. ../..; do
	x=${wwwdir}/${i}/index.sh
	if test -x $x; then
	    $x ${newari}
	    break
	fi
    done

    gzip -c -v -9 ${newari} > ${newari}.gz

    cp ${ari} ${oldari}
    cp ${ari}.gz ${oldari}.gz
    cp ${newari} ${ari}
    cp ${newari}.gz ${ari}.gz

fi # update_web_p

# ls -l ${wwwdir}

exit 0
