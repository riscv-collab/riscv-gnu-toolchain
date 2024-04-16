#!/bin/sh

# GDB script to list of problems using awk.
#
# Copyright (C) 2002-2024 Free Software Foundation, Inc.
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

# Make certain that the script is not running in an internationalized
# environment.

LANG=C ; export LANG
LC_ALL=C ; export LC_ALL

# Permanent checks take the form:

#     Do not use XXXX, C++11 implies YYYY
#     Do not use XXXX, instead use YYYY''.

# and should never be removed.

# Temporary checks take the form:

#     Replace XXXX with YYYY

# and once they reach zero, can be eliminated.

# FIXME: It should be able to override this on the command line
error="regression"
warning="regression"
ari="regression eol code comment deprecated legacy obsolete gettext"
all="regression eol code comment deprecated legacy obsolete gettext deprecate internal gdbarch macro"
print_doc=0
print_idx=0

usage ()
{
    cat <<EOF 1>&2
Error: $1

Usage:
    $0 --print-doc --print-idx -Wall -Werror -WCATEGORY FILE ...
Options:
  --print-doc    Print a list of all potential problems, then exit.
  --print-idx    Include the problems IDX (index or key) in every message.
  --src=file     Write source lines to file.
  -Werror        Treat all problems as errors.
  -Wall          Report all problems.
  -Wari          Report problems that should be fixed in new code.
  -WCATEGORY     Report problems in the specifed category.  The category
                 can be prefixed with "no-".  Valid categories
                 are: ${all}
EOF
    exit 1
}


# Parse the various options
Woptions=
srclines=""
while test $# -gt 0
do
    case "$1" in
    -Wall ) Woptions="${all}" ;;
    -Wari ) Woptions="${ari}" ;;
    -Werror ) Werror=1 ;;
    -W* ) Woptions="${Woptions} `echo x$1 | sed -e 's/x-W//'`" ;;
    --print-doc ) print_doc=1 ;;
    --print-idx ) print_idx=1 ;;
    --src=* ) srclines="`echo $1 | sed -e 's/--src=/srclines=\"/'`\"" ;;
    -- ) shift ; break ;;
    - ) break ;;
    -* ) usage "$1: unknown option" ;;
    * ) break ;;
    esac
    shift
done
if test -n "$Woptions" ; then
    warning="$Woptions"
    error=
fi


# -Werror implies treating all warnings as errors.
if test -n "${Werror}" ; then
    error="${error} ${warning}"
fi


# Validate all errors and warnings.
for w in ${warning} ${error}
do
    case "$w" in
	no-*) w=`echo x$w | sed -e 's/xno-//'`;;
    esac

    case " ${all} " in
    *" ${w} "* ) ;;
    * ) usage "Unknown option -W${w}" ;;
    esac
done


# make certain that there is at least one file.
if test $# -eq 0 -a ${print_doc} = 0
then
    usage "Missing file."
fi


# Convert the errors/warnings into corresponding array entries.
for a in ${all}
do
    aris="${aris} ari_${a} = \"${a}\";"
done
for w in ${warning}
do
    val=1
    case "$w" in
	no-*) w=`echo x$w | sed -e 's/xno-//'`; val=0 ;;
    esac
    warnings="${warnings} warning[ari_${w}] = $val;"
done
for e in ${error}
do
    val=1
    case "$e" in
	no-*) e=`echo x$e | sed -e 's/xno-//'`; val=0 ;;
    esac
    errors="${errors} error[ari_${e}]  = $val;"
done

if [ "$AWK" = "" ] ; then
  AWK=awk
fi

${AWK} -- '
BEGIN {
    # NOTE, for a per-file begin use "FNR == 1".
    '"${aris}"'
    '"${errors}"'
    '"${warnings}"'
    '"${srclines}"'
    print_doc =  '$print_doc'
    print_idx =  '$print_idx'
    PWD = "'`pwd`'"
}

# Print the error message for BUG.  Append SUPLEMENT if non-empty.
function print_bug(file,line,prefix,category,bug,doc,supplement, suffix,idx) {
    if (print_idx) {
	idx = bug ": "
    } else {
	idx = ""
    }
    if (supplement) {
	suffix = " (" supplement ")"
    } else {
	suffix = ""
    }
    # ari.*.bug: <FILE>:<LINE>: <CATEGORY>: <BUG>: <DOC>
    print file ":" line ": " prefix category ": " idx doc suffix
    if (srclines != "") {
	print file ":" line ":" $0 >> srclines
    }
}

function fix(bug,file,count) {
    skip[bug, file] = count
    skipped[bug, file] = 0
}

function fail(bug,supplement) {
    if (doc[bug] == "") {
	print_bug("", 0, "internal: ", "internal", "internal", "Missing doc for bug " bug)
	exit
    }
    if (category[bug] == "") {
	print_bug("", 0, "internal: ", "internal", "internal", "Missing category for bug " bug)
	exit
    }

    if (ARI_OK == bug) {
	return
    }
    # Trim the filename down to just DIRECTORY/FILE so that it can be
    # robustly used by the FIX code.

    if (FILENAME ~ /^\//) {
	canonicalname = FILENAME
    } else {
        canonicalname = PWD "/" FILENAME
    }
    shortname = gensub (/^.*\/([^\\]*\/[^\\]*)$/, "\\1", 1, canonicalname)

    skipped[bug, shortname]++
    if (skip[bug, shortname] >= skipped[bug, shortname]) {
	# print FILENAME, FNR, skip[bug, FILENAME], skipped[bug, FILENAME], bug
	# Do nothing
    } else if (error[category[bug]]) {
	# ari.*.bug: <FILE>:<LINE>: <CATEGORY>: <BUG>: <DOC>
	print_bug(FILENAME, FNR, "", category[bug], bug, doc[bug], supplement)
    } else if (warning[category[bug]]) {
	# ari.*.bug: <FILE>:<LINE>: <CATEGORY>: <BUG>: <DOC>
	print_bug(FILENAME, FNR, "warning: ", category[bug], bug, doc[bug], supplement)
    }
}

FNR == 1 {
    seen[FILENAME] = 1
    if (match(FILENAME, "\\.[ly]$")) {
      # FILENAME is a lex or yacc source
      is_yacc_or_lex = 1
    }
    else {
      is_yacc_or_lex = 0
    }
}
END {
    if (print_idx) {
	idx = bug ": "
    } else {
	idx = ""
    }
    # Did we do only a partial skip?
    for (bug_n_file in skip) {
	split (bug_n_file, a, SUBSEP)
	bug = a[1]
	file = a[2]
	if (seen[file] && (skipped[bug_n_file] < skip[bug_n_file])) {
	    # ari.*.bug: <FILE>:<LINE>: <CATEGORY>: <BUG>: <DOC>
	    b = file " missing " bug
	    print_bug(file, 0, "", "internal", file " missing " bug, "Expecting " skip[bug_n_file] " occurances of bug " bug " in file " file ", only found " skipped[bug_n_file])
	}
    }
}


# Skip OBSOLETE lines
/(^|[^_[:alnum:]])OBSOLETE([^_[:alnum:]]|$)/ { next; }

# Skip ARI lines

BEGIN {
    ARI_OK = ""
}

/\/\* ARI:[[:space:]]*(.*)[[:space:]]*\*\// {
    ARI_OK = gensub(/^.*\/\* ARI:[[:space:]]*(.*[^[:space:]])[[:space:]]*\*\/.*$/, "\\1", 1, $0)
    # print "ARI line found \"" $0 "\""
    # print "ARI_OK \"" ARI_OK "\""
}
! /\/\* ARI:[[:space:]]*(.*)[[:space:]]*\*\// {
    ARI_OK = ""
}


# SNIP - Strip out comments - SNIP

FNR == 1 {
    comment_p = 0
}
comment_p && /\*\// { gsub (/^([^\*]|\*+[^\/\*])*\*+\//, " "); comment_p = 0; }
comment_p { next; }
!comment_p { gsub (/\/\*([^\*]|\*+[^\/\*])*\*+\//, " "); }
!comment_p && /(^|[^"])\/\*/ { gsub (/\/\*.*$/, " "); comment_p = 1; }


BEGIN { doc["_ markup"] = "\
All messages should be marked up with _."
    category["_ markup"] = ari_gettext
}
/^[^"]*[[:space:]](warning|error|error_no_arg|query|perror_with_name)[[:space:]]*\([^_\(a-z]/ {
    if (! /\("%s"/) {
	fail("_ markup")
    }
}

BEGIN { doc["trailing new line"] = "\
A message should not have a trailing new line"
    category["trailing new line"] = ari_gettext
}
/(^|[^_[:alnum:]])(warning|error)[[:space:]]*\(_\(".*\\n"\)[\),]/ {
    fail("trailing new line")
}

# Include files for which GDB has a custom version.

BEGIN { doc["assert.h"] = "\
Do not include assert.h, instead include \"gdb_assert.h\"";
    category["assert.h"] = ari_regression
    fix("assert.h", "gdb/gdb_assert.h", 0) # it does not use it
}
/^#[[:space:]]*include[[:space:]]+.assert\.h./ {
    fail("assert.h")
}

BEGIN { doc["regex.h"] = "\
Do not include regex.h, instead include gdb_regex.h"
    category["regex.h"] = ari_regression
    fix("regex.h", "gdb/gdb_regex.h", 1)
}
/^#[[:space:]]*include[[:space:]]*.regex\.h./ {
    fail("regex.h")
}

BEGIN { doc["xregex.h"] = "\
Do not include xregex.h, instead include gdb_regex.h"
    category["xregex.h"] = ari_regression
    fix("xregex.h", "gdb/gdb_regex.h", 1)
}
/^#[[:space:]]*include[[:space:]]*.xregex\.h./ {
    fail("xregex.h")
}

BEGIN { doc["gnu-regex.h"] = "\
Do not include gnu-regex.h, instead include gdb_regex.h"
    category["gnu-regex.h"] = ari_regression
}
/^#[[:space:]]*include[[:space:]]*.gnu-regex\.h./ {
    fail("gnu regex.h")
}

BEGIN { doc["wait.h"] = "\
Do not include wait.h or sys/wait.h, instead include gdb_wait.h"
    fix("wait.h", "gdbsupport/gdb_wait.h", 2);
    category["wait.h"] = ari_regression
}
/^#[[:space:]]*include[[:space:]]*.wait\.h./ \
|| /^#[[:space:]]*include[[:space:]]*.sys\/wait\.h./ {
    fail("wait.h")
}

BEGIN { doc["vfork.h"] = "\
Do not include vfork.h, instead include gdb_vfork.h"
    fix("vfork.h", "gdb/gdb_vfork.h", 1);
    category["vfork.h"] = ari_regression
}
/^#[[:space:]]*include[[:space:]]*.vfork\.h./ {
    fail("vfork.h")
}

BEGIN { doc["error not internal-warning"] = "\
Do not use error(\"internal-warning\"), instead use internal_warning"
    category["error not internal-warning"] = ari_regression
}
/error.*\"[Ii]nternal.warning/ {
    fail("error not internal-warning")
}

BEGIN { doc["%p"] = "\
Do not use printf(\"%p\"), instead use printf(\"%s\",paddr()) to dump a \
target address, or host_address_to_string() for a host address"
    category["%p"] = ari_code
}
# Allow gdb %p extensions, but not other uses of %p.
/%p[^[\]sF]/ && !/%prec/ {
    fail("%p")
}

BEGIN { doc["%ll"] = "\
Do not use printf(\"%ll\"), instead use printf(\"%s\",phex()) to dump a \
`long long'\'' value"
    category["%ll"] = ari_code
}
# Allow %ll in scanf
/%[0-9]*ll/ && !/scanf \(.*%[0-9]*ll/ {
    fail("%ll")
}


# SNIP - Strip out strings - SNIP

# Test on top.c, scm-valprint.c, remote-rdi.c, ada-lang.c
FNR == 1 {
    string_p = 0
    trace_string = 0
}
# Strip escaped characters.
{ gsub(/\\./, "."); }
# Strip quoted quotes.
{ gsub(/'\''.'\''/, "'\''.'\''"); }
# End of multi-line string
string_p && /\"/ {
    if (trace_string) print "EOS:" FNR, $0;
    gsub (/^[^\"]*\"/, "'\''");
    string_p = 0;
}
# Middle of multi-line string, discard line.
string_p {
    if (trace_string) print "MOS:" FNR, $0;
    $0 = ""
}
# Strip complete strings from the middle of the line
!string_p && /\"[^\"]*\"/ {
    if (trace_string) print "COS:" FNR, $0;
    gsub (/\"[^\"]*\"/, "'\''");
}
# Start of multi-line string
BEGIN { doc["multi-line string"] = "\
Multi-line string must have the newline escaped"
    category["multi-line string"] = ari_regression
}
!string_p && /\"/ {
    if (trace_string) print "SOS:" FNR, $0;
    if (/[^\\]$/) {
	fail("multi-line string")
    }
    gsub (/\"[^\"]*$/, "'\''");
    string_p = 1;
}
# { print }

# Multi-line string
string_p &&

# Accumulate continuation lines
FNR == 1 {
    cont_p = 0
}
!cont_p { full_line = ""; }
/[^\\]\\$/ { gsub (/\\$/, ""); full_line = full_line $0; cont_p = 1; next; }
cont_p { $0 = full_line $0; cont_p = 0; full_line = ""; }


BEGIN { doc["__FUNCTION__"] = "\
Do not use __FUNCTION__, C++11 does not support this macro"
    category["__FUNCTION__"] = ari_regression
}
/(^|[^_[:alnum:]])__FUNCTION__([^_[:alnum:]]|$)/ {
    fail("__FUNCTION__")
}

BEGIN { doc["__CYGWIN32__"] = "\
Do not use __CYGWIN32__, instead use __CYGWIN__ or, better, an explicit \
autoconf tests"
    category["__CYGWIN32__"] = ari_regression
}
/(^|[^_[:alnum:]])__CYGWIN32__([^_[:alnum:]]|$)/ {
    fail("__CYGWIN32__")
}

BEGIN { doc["PTR"] = "\
Do not use PTR, C++11 implies `void *'\''"
    category["PTR"] = ari_regression
    #fix("PTR", "gdb/utils.c", 6)
}
/(^|[^_[:alnum:]])PTR([^_[:alnum:]]|$)/ {
    fail("PTR")
}

BEGIN { doc["UCASE function"] = "\
Function name is uppercase."
    category["UCASE function"] = ari_code
    possible_UCASE = 0
    UCASE_full_line = ""
}
(possible_UCASE) {
    if (ARI_OK == "UCASE function") {
	possible_UCASE = 0
    }
    # Closing brace found?
    else if (UCASE_full_line ~ \
	/^[A-Z][[:alnum:]_]*[[:space:]]*\([^()]*\).*$/) {
	if ((UCASE_full_line ~ \
	    /^[A-Z][[:alnum:]_]*[[:space:]]*\([^()]*\)[[:space:]]*$/) \
	    && ($0 ~ /^\{/) && (is_yacc_or_lex == 0)) {
	    store_FNR = FNR
	    FNR = possible_FNR
	    store_0 = $0;
	    $0 = UCASE_full_line;
	    fail("UCASE function")
	    FNR = store_FNR
	    $0 = store_0;
	}
	possible_UCASE = 0
	UCASE_full_line = ""
    } else {
	UCASE_full_line = UCASE_full_line $0;
    }
}
/^[A-Z][[:alnum:]_]*[[:space:]]*\([^()]*(|\))[[:space:]]*$/ {
    possible_UCASE = 1
    if (ARI_OK == "UCASE function") {
	possible_UCASE = 0
    }
    possible_FNR = FNR
    UCASE_full_line = $0
}


BEGIN { doc["editCase function"] = "\
Function name starts lower case but has uppercased letters."
    category["editCase function"] = ari_code
    possible_editCase = 0
    editCase_full_line = ""
}
(possible_editCase) {
    if (ARI_OK == "editCase function") {
	possible_editCase = 0
    }
    # Closing brace found?
    else if (editCase_full_line ~ \
/^[a-z][a-z0-9_]*[A-Z][a-z0-9A-Z_]*[[:space:]]*\([^()]*\).*$/) {
	if ((editCase_full_line ~ \
/^[a-z][a-z0-9_]*[A-Z][a-z0-9A-Z_]*[[:space:]]*\([^()]*\)[[:space:]]*$/) \
	    && ($0 ~ /^\{/) && (is_yacc_or_lex == 0)) {
	    store_FNR = FNR
	    FNR = possible_FNR
	    store_0 = $0;
	    $0 = editCase_full_line;
	    fail("editCase function")
	    FNR = store_FNR
	    $0 = store_0;
	}
	possible_editCase = 0
	editCase_full_line = ""
    } else {
	editCase_full_line = editCase_full_line $0;
    }
}
/^[a-z][a-z0-9_]*[A-Z][a-z0-9A-Z_]*[[:space:]]*\([^()]*(|\))[[:space:]]*$/ {
    possible_editCase = 1
    if (ARI_OK == "editCase function") {
        possible_editCase = 0
    }
    possible_FNR = FNR
    editCase_full_line = $0
}

# Only function implementation should be on first column
BEGIN { doc["function call in first column"] = "\
Function name in first column should be restricted to function implementation"
    category["function call in first column"] = ari_code
}
/^[a-z][a-z0-9_]*[[:space:]]*\((|[^*][^()]*)\)[[:space:]]*[^ \t]+/ {
    fail("function call in first column")
}


BEGIN { doc["hash"] = "\
Do not use ` #...'\'', instead use `#...'\''(some compilers only correctly \
parse a C preprocessor directive when `#'\'' is the first character on \
the line)"
    category["hash"] = ari_regression
}
/^[[:space:]]+#/ {
    fail("hash")
}

BEGIN { doc["OP eol"] = "\
Do not use &&, or || at the end of a line"
    category["OP eol"] = ari_code
}
# * operator needs a special treatment as it can be a
# valid end of line for a pointer type definition
# Only catch case where an assignment or an opening brace is present
/(\|\||\&\&|==|!=|[[:space:]][+\-\/])[[:space:]]*$/ \
|| /(\(|=)[[:space:]].*[[:space:]]\*[[:space:]]*$/ {
    fail("OP eol")
}

BEGIN { doc["strerror"] = "\
Do not use strerror(), instead use safe_strerror()"
    category["strerror"] = ari_regression
    fix("strerror", "gdb/gdb_string.h", 1)
    fix("strerror", "gdb/gdbsupport/mingw-strerror.c", 1)
    fix("strerror", "gdb/gdbsupport/posix-strerror.c", 1)
}
/(^|[^_[:alnum:]])strerror[[:space:]]*\(/ {
    fail("strerror")
}

BEGIN { doc["long long"] = "\
Do not use `long long'\'', instead use LONGEST"
    category["long long"] = ari_code
}
/(^|[^_[:alnum:]])long[[:space:]]+long([^_[:alnum:]]|$)/ {
    fail("long long")
}

BEGIN { doc["ATTR_FORMAT"] = "\
Do not use ATTR_FORMAT, use ATTRIBUTE_PRINTF instead"
    category["ATTR_FORMAT"] = ari_regression
}
/(^|[^_[:alnum:]])ATTR_FORMAT([^_[:alnum:]]|$)/ {
    fail("ATTR_FORMAT")
}

BEGIN { doc["ATTR_NORETURN"] = "\
Do not use ATTR_NORETURN, use ATTRIBUTE_NORETURN instead"
    category["ATTR_NORETURN"] = ari_regression
}
/(^|[^_[:alnum:]])ATTR_NORETURN([^_[:alnum:]]|$)/ {
    fail("ATTR_NORETURN")
}

BEGIN { doc["NORETURN"] = "\
Do not use NORETURN, use ATTRIBUTE_NORETURN instead"
    category["NORETURN"] = ari_regression
}
/(^|[^_[:alnum:]])NORETURN([^_[:alnum:]]|$)/ {
    fail("NORETURN")
}


# General problems

# Commented out, but left inside sources, just in case.
# BEGIN { doc["inline"] = "\
# Do not use the inline attribute; \
# since the compiler generally ignores this, better algorithm selection \
# is needed to improved performance"
#    category["inline"] = ari_code
# }
# /(^|[^_[:alnum:]])inline([^_[:alnum:]]|$)/ {
#     fail("inline")
# }

# This test is obsolete as this type
# has been deprecated and finally suppressed from GDB sources
#BEGIN { doc["obj_private"] = "\
#Replace obj_private with objfile_data"
#    category["obj_private"] = ari_obsolete
#}
#/(^|[^_[:alnum:]])obj_private([^_[:alnum:]]|$)/ {
#    fail("obj_private")
#}

BEGIN { doc["abort"] = "\
Do not use abort, instead use internal_error; GDB should never abort"
    category["abort"] = ari_regression
}
/(^|[^_[:alnum:]])abort[[:space:]]*\(/ {
    fail("abort")
}

BEGIN { doc["basename"] = "\
Do not use basename, instead use lbasename"
    category["basename"] = ari_regression
}
/(^|[^_[:alnum:]])basename[[:space:]]*\(/ {
    fail("basename")
}

BEGIN { doc["assert"] = "\
Do not use assert, instead use gdb_assert or internal_error; assert \
calls abort and GDB should never call abort"
    category["assert"] = ari_regression
}
/(^|[^_[:alnum:]])assert[[:space:]]*\(/ {
    fail("assert")
}

BEGIN { doc["TARGET_HAS_HARDWARE_WATCHPOINTS"] = "\
Replace TARGET_HAS_HARDWARE_WATCHPOINTS with nothing, not needed"
    category["TARGET_HAS_HARDWARE_WATCHPOINTS"] = ari_regression
}
/(^|[^_[:alnum:]])TARGET_HAS_HARDWARE_WATCHPOINTS([^_[:alnum:]]|$)/ {
    fail("TARGET_HAS_HARDWARE_WATCHPOINTS")
}

BEGIN { doc["ADD_SHARED_SYMBOL_FILES"] = "\
Replace ADD_SHARED_SYMBOL_FILES with nothing, not needed?"
    category["ADD_SHARED_SYMBOL_FILES"] = ari_regression
}
/(^|[^_[:alnum:]])ADD_SHARED_SYMBOL_FILES([^_[:alnum:]]|$)/ {
    fail("ADD_SHARED_SYMBOL_FILES")
}

BEGIN { doc["SOLIB_ADD"] = "\
Replace SOLIB_ADD with nothing, not needed?"
    category["SOLIB_ADD"] = ari_regression
}
/(^|[^_[:alnum:]])SOLIB_ADD([^_[:alnum:]]|$)/ {
    fail("SOLIB_ADD")
}

BEGIN { doc["SOLIB_CREATE_INFERIOR_HOOK"] = "\
Replace SOLIB_CREATE_INFERIOR_HOOK with nothing, not needed?"
    category["SOLIB_CREATE_INFERIOR_HOOK"] = ari_regression
}
/(^|[^_[:alnum:]])SOLIB_CREATE_INFERIOR_HOOK([^_[:alnum:]]|$)/ {
    fail("SOLIB_CREATE_INFERIOR_HOOK")
}

BEGIN { doc["SOLIB_LOADED_LIBRARY_PATHNAME"] = "\
Replace SOLIB_LOADED_LIBRARY_PATHNAME with nothing, not needed?"
    category["SOLIB_LOADED_LIBRARY_PATHNAME"] = ari_regression
}
/(^|[^_[:alnum:]])SOLIB_LOADED_LIBRARY_PATHNAME([^_[:alnum:]]|$)/ {
    fail("SOLIB_LOADED_LIBRARY_PATHNAME")
}

BEGIN { doc["REGISTER_U_ADDR"] = "\
Replace REGISTER_U_ADDR with nothing, not needed?"
    category["REGISTER_U_ADDR"] = ari_regression
}
/(^|[^_[:alnum:]])REGISTER_U_ADDR([^_[:alnum:]]|$)/ {
    fail("REGISTER_U_ADDR")
}

BEGIN { doc["PROCESS_LINENUMBER_HOOK"] = "\
Replace PROCESS_LINENUMBER_HOOK with nothing, not needed?"
    category["PROCESS_LINENUMBER_HOOK"] = ari_regression
}
/(^|[^_[:alnum:]])PROCESS_LINENUMBER_HOOK([^_[:alnum:]]|$)/ {
    fail("PROCESS_LINENUMBER_HOOK")
}

BEGIN { doc["PC_SOLIB"] = "\
Replace PC_SOLIB with nothing, not needed?"
    category["PC_SOLIB"] = ari_regression
}
/(^|[^_[:alnum:]])PC_SOLIB([^_[:alnum:]]|$)/ {
    fail("PC_SOLIB")
}

BEGIN { doc["IN_SOLIB_DYNSYM_RESOLVE_CODE"] = "\
Replace IN_SOLIB_DYNSYM_RESOLVE_CODE with nothing, not needed?"
    category["IN_SOLIB_DYNSYM_RESOLVE_CODE"] = ari_regression
}
/(^|[^_[:alnum:]])IN_SOLIB_DYNSYM_RESOLVE_CODE([^_[:alnum:]]|$)/ {
    fail("IN_SOLIB_DYNSYM_RESOLVE_CODE")
}

BEGIN { doc["GCC_COMPILED_FLAG_SYMBOL"] = "\
Replace GCC_COMPILED_FLAG_SYMBOL with nothing, not needed?"
    category["GCC_COMPILED_FLAG_SYMBOL"] = ari_deprecate
}
/(^|[^_[:alnum:]])GCC_COMPILED_FLAG_SYMBOL([^_[:alnum:]]|$)/ {
    fail("GCC_COMPILED_FLAG_SYMBOL")
}

BEGIN { doc["GCC2_COMPILED_FLAG_SYMBOL"] = "\
Replace GCC2_COMPILED_FLAG_SYMBOL with nothing, not needed?"
    category["GCC2_COMPILED_FLAG_SYMBOL"] = ari_deprecate
}
/(^|[^_[:alnum:]])GCC2_COMPILED_FLAG_SYMBOL([^_[:alnum:]]|$)/ {
    fail("GCC2_COMPILED_FLAG_SYMBOL")
}

BEGIN { doc["FUNCTION_EPILOGUE_SIZE"] = "\
Replace FUNCTION_EPILOGUE_SIZE with nothing, not needed?"
    category["FUNCTION_EPILOGUE_SIZE"] = ari_regression
}
/(^|[^_[:alnum:]])FUNCTION_EPILOGUE_SIZE([^_[:alnum:]]|$)/ {
    fail("FUNCTION_EPILOGUE_SIZE")
}

BEGIN { doc["HAVE_VFORK"] = "\
Do not use HAVE_VFORK, instead include \"gdb_vfork.h\" and call vfork() \
unconditionally"
    category["HAVE_VFORK"] = ari_regression
}
/(^|[^_[:alnum:]])HAVE_VFORK([^_[:alnum:]]|$)/ {
    fail("HAVE_VFORK")
}

BEGIN { doc["bcmp"] = "\
Do not use bcmp(), C++11 implies memcmp()"
    category["bcmp"] = ari_regression
}
/(^|[^_[:alnum:]])bcmp[[:space:]]*\(/ {
    fail("bcmp")
}

BEGIN { doc["setlinebuf"] = "\
Do not use setlinebuf(), C++11 implies setvbuf()"
    category["setlinebuf"] = ari_regression
}
/(^|[^_[:alnum:]])setlinebuf[[:space:]]*\(/ {
    fail("setlinebuf")
}

BEGIN { doc["bcopy"] = "\
Do not use bcopy(), C++11 implies memcpy() and memmove()"
    category["bcopy"] = ari_regression
}
/(^|[^_[:alnum:]])bcopy[[:space:]]*\(/ {
    fail("bcopy")
}

BEGIN { doc["get_frame_base"] = "\
Replace get_frame_base with get_frame_id, get_frame_base_address, \
get_frame_locals_address, or get_frame_args_address."
    category["get_frame_base"] = ari_obsolete
}
/(^|[^_[:alnum:]])get_frame_base([^_[:alnum:]]|$)/ {
    fail("get_frame_base")
}

BEGIN { doc["floatformat_to_double"] = "\
Do not use floatformat_to_double() from libierty, \
instead use floatformat_to_doublest()"
    category["floatformat_to_double"] = ari_regression
}
/(^|[^_[:alnum:]])floatformat_to_double[[:space:]]*\(/ {
    fail("floatformat_to_double")
}

BEGIN { doc["floatformat_from_double"] = "\
Do not use floatformat_from_double() from libierty, \
instead use host_float_ops<T>::from_target()"
    category["floatformat_from_double"] = ari_regression
}
/(^|[^_[:alnum:]])floatformat_from_double[[:space:]]*\(/ {
    fail("floatformat_from_double")
}

BEGIN { doc["BIG_ENDIAN"] = "\
Do not use BIG_ENDIAN, instead use BFD_ENDIAN_BIG"
    category["BIG_ENDIAN"] = ari_regression
}
/(^|[^_[:alnum:]])BIG_ENDIAN([^_[:alnum:]]|$)/ {
    fail("BIG_ENDIAN")
}

BEGIN { doc["LITTLE_ENDIAN"] = "\
Do not use LITTLE_ENDIAN, instead use BFD_ENDIAN_LITTLE";
    category["LITTLE_ENDIAN"] = ari_regression
}
/(^|[^_[:alnum:]])LITTLE_ENDIAN([^_[:alnum:]]|$)/ {
    fail("LITTLE_ENDIAN")
}

BEGIN { doc["BIG_ENDIAN"] = "\
Do not use BIG_ENDIAN, instead use BFD_ENDIAN_BIG"
    category["BIG_ENDIAN"] = ari_regression
}
/(^|[^_[:alnum:]])BIG_ENDIAN([^_[:alnum:]]|$)/ {
    fail("BIG_ENDIAN")
}

BEGIN { doc["sec_ptr"] = "\
Instead of sec_ptr, use struct bfd_section";
    category["sec_ptr"] = ari_regression
}
/(^|[^_[:alnum:]])sec_ptr([^_[:alnum:]]|$)/ {
    fail("sec_ptr")
}

BEGIN { doc["frame_unwind_unsigned_register"] = "\
Replace frame_unwind_unsigned_register with frame_unwind_register_unsigned"
    category["frame_unwind_unsigned_register"] = ari_regression
}
/(^|[^_[:alnum:]])frame_unwind_unsigned_register([^_[:alnum:]]|$)/ {
    fail("frame_unwind_unsigned_register")
}

BEGIN { doc["frame_register_read"] = "\
Replace frame_register_read() with get_frame_register(), or \
possibly introduce a new method safe_get_frame_register()"
    category["frame_register_read"] = ari_obsolete
}
/(^|[^_[:alnum:]])frame_register_read([^_[:alnum:]]|$)/ {
    fail("frame_register_read")
}

BEGIN { doc["read_register"] = "\
Replace read_register() with regcache_read() et.al."
    category["read_register"] = ari_regression
}
/(^|[^_[:alnum:]])read_register([^_[:alnum:]]|$)/ {
    fail("read_register")
}

BEGIN { doc["write_register"] = "\
Replace write_register() with regcache_read() et.al."
    category["write_register"] = ari_regression
}
/(^|[^_[:alnum:]])write_register([^_[:alnum:]]|$)/ {
    fail("write_register")
}

function report(name) {
    # Drop any trailing _P.
    name = gensub(/(_P|_p)$/, "", 1, name)
    # Convert to lower case
    name = tolower(name)
    # Split into category and bug
    cat = gensub(/^([[:alpha:]]+)_([_[:alnum:]]*)$/, "\\1", 1, name)
    bug = gensub(/^([[:alpha:]]+)_([_[:alnum:]]*)$/, "\\2", 1, name)
    # Report it
    name = cat " " bug
    doc[name] = "Do not use " cat " " bug ", see declaration for details"
    category[name] = cat
    fail(name)
}

/(^|[^_[:alnum:]])(DEPRECATED|deprecated|set_gdbarch_deprecated|LEGACY|legacy|set_gdbarch_legacy)_/ {
    line = $0
    # print "0 =", $0
    while (1) {
	name = gensub(/^(|.*[^_[:alnum:]])((DEPRECATED|deprecated|LEGACY|legacy)_[_[:alnum:]]*)(.*)$/, "\\2", 1, line)
	line = gensub(/^(|.*[^_[:alnum:]])((DEPRECATED|deprecated|LEGACY|legacy)_[_[:alnum:]]*)(.*)$/, "\\1 \\4", 1, line)
	# print "name =", name, "line =", line
	if (name == line) break;
	report(name)
    }
}

# Count the number of times each architecture method is set
/(^|[^_[:alnum:]])set_gdbarch_[_[:alnum:]]*([^_[:alnum:]]|$)/ {
    name = gensub(/^.*set_gdbarch_([_[:alnum:]]*).*$/, "\\1", 1, $0)
    doc["set " name] = "\
Call to set_gdbarch_" name
    category["set " name] = ari_gdbarch
    fail("set " name)
}

# Count the number of times each tm/xm/nm macro is defined or undefined
/^#[[:space:]]*(undef|define)[[:space:]]+[[:alnum:]_]+.*$/ \
&& !/^#[[:space:]]*(undef|define)[[:space:]]+[[:alnum:]_]+_H($|[[:space:]])/ \
&& FILENAME ~ /(^|\/)config\/(|[^\/]*\/)(tm-|xm-|nm-).*\.h$/ {
    basename = gensub(/(^|.*\/)([^\/]*)$/, "\\2", 1, FILENAME)
    type = gensub(/^(tm|xm|nm)-.*\.h$/, "\\1", 1, basename)
    name = gensub(/^#[[:space:]]*(undef|define)[[:space:]]+([[:alnum:]_]+).*$/, "\\2", 1, $0)
    if (type == basename) {
        type = "macro"
    }
    doc[type " " name] = "\
Do not define macros such as " name " in a tm, nm or xm file, \
in fact do not provide a tm, nm or xm file"
    category[type " " name] = ari_macro
    fail(type " " name)
}

BEGIN { doc["deprecated_registers"] = "\
Replace deprecated_registers with nothing, they have reached \
end-of-life"
    category["deprecated_registers"] = ari_eol
}
/(^|[^_[:alnum:]])deprecated_registers([^_[:alnum:]]|$)/ {
    fail("deprecated_registers")
}

BEGIN { doc["read_pc"] = "\
Replace READ_PC() with frame_pc_unwind; \
at present the inferior function call code still uses this"
    category["read_pc"] = ari_deprecate
}
/(^|[^_[:alnum:]])read_pc[[:space:]]*\(/ || \
/(^|[^_[:alnum:]])set_gdbarch_read_pc[[:space:]]*\(/ || \
/(^|[^_[:alnum:]])TARGET_READ_PC[[:space:]]*\(/ {
    fail("read_pc")
}

BEGIN { doc["write_pc"] = "\
Replace write_pc() with get_frame_base_address or get_frame_id; \
at present the inferior function call code still uses this when doing \
a DECR_PC_AFTER_BREAK"
    category["write_pc"] = ari_deprecate
}
/(^|[^_[:alnum:]])write_pc[[:space:]]*\(/ || \
/(^|[^_[:alnum:]])TARGET_WRITE_PC[[:space:]]*\(/ {
    fail("write_pc")
}

BEGIN { doc["generic_target_write_pc"] = "\
Replace generic_target_write_pc with a per-architecture implementation, \
this relies on PC_REGNUM which is being eliminated"
    category["generic_target_write_pc"] = ari_regression
}
/(^|[^_[:alnum:]])generic_target_write_pc([^_[:alnum:]]|$)/ {
    fail("generic_target_write_pc")
}

BEGIN { doc["read_sp"] = "\
Replace read_sp() with frame_sp_unwind"
    category["read_sp"] = ari_regression
}
/(^|[^_[:alnum:]])read_sp[[:space:]]*\(/ || \
/(^|[^_[:alnum:]])set_gdbarch_read_sp[[:space:]]*\(/ || \
/(^|[^_[:alnum:]])TARGET_READ_SP[[:space:]]*\(/ {
    fail("read_sp")
}

BEGIN { doc["register_cached"] = "\
Replace register_cached() with nothing, does not have a regcache parameter"
    category["register_cached"] = ari_regression
}
/(^|[^_[:alnum:]])register_cached[[:space:]]*\(/ {
    fail("register_cached")
}

BEGIN { doc["set_register_cached"] = "\
Replace set_register_cached() with nothing, does not have a regcache parameter"
    category["set_register_cached"] = ari_regression
}
/(^|[^_[:alnum:]])set_register_cached[[:space:]]*\(/ {
    fail("set_register_cached")
}

# Print functions: Use versions that either check for buffer overflow
# or safely allocate a fresh buffer.

BEGIN { doc["sprintf"] = "\
Do not use sprintf, instead use xsnprintf or xstrprintf"
    category["sprintf"] = ari_code
}
/(^|[^_[:alnum:]])sprintf[[:space:]]*\(/ {
    fail("sprintf")
}

BEGIN { doc["vsprintf"] = "\
Do not use vsprintf(), instead use xstrvprintf"
    category["vsprintf"] = ari_regression
}
/(^|[^_[:alnum:]])vsprintf[[:space:]]*\(/ {
    fail("vsprintf")
}

BEGIN { doc["asprintf"] = "\
Do not use asprintf(), instead use xstrprintf()"
    category["asprintf"] = ari_regression
}
/(^|[^_[:alnum:]])asprintf[[:space:]]*\(/ {
    fail("asprintf")
}

BEGIN { doc["vasprintf"] = "\
Do not use vasprintf(), instead use xstrvprintf"
    fix("vasprintf", "gdbsupport/common-utils.c", 1)
    category["vasprintf"] = ari_regression
}
/(^|[^_[:alnum:]])vasprintf[[:space:]]*\(/ {
    fail("vasprintf")
}

BEGIN { doc["printf_vma"] = "\
Do not use printf_vma, instead use paddress or phex_nz"
    category["printf_vma"] = ari_code
}
/(^|[^_[:alnum:]])printf_vma[[:space:]]*\(/ {
    fail("printf_vma")
}

BEGIN { doc["sprintf_vma"] = "\
Do not use sprintf_vma, instead use paddress or phex_nz"
    category["sprintf_vma"] = ari_code
}
/(^|[^_[:alnum:]])sprintf_vma[[:space:]]*\(/ {
    fail("sprintf_vma")
}

# More generic memory operations

BEGIN { doc["bzero"] = "\
Do not use bzero(), instead use memset()"
    category["bzero"] = ari_regression
}
/(^|[^_[:alnum:]])bzero[[:space:]]*\(/ {
    fail("bzero")
}

BEGIN { doc["strdup"] = "\
Do not use strdup(), instead use xstrdup()";
    category["strdup"] = ari_regression
}
/(^|[^_[:alnum:]])strdup[[:space:]]*\(/ {
    fail("strdup")
}

BEGIN { doc["strsave"] = "\
Do not use strsave(), instead use xstrdup() et.al."
    category["strsave"] = ari_regression
}
/(^|[^_[:alnum:]])strsave[[:space:]]*\(/ {
    fail("strsave")
}

# String compare functions

BEGIN { doc["strnicmp"] = "\
Do not use strnicmp(), instead use strncasecmp()"
    category["strnicmp"] = ari_regression
}
/(^|[^_[:alnum:]])strnicmp[[:space:]]*\(/ {
    fail("strnicmp")
}

# Typedefs that are either redundant or can be reduced to `struct
# type *''.
# Must be placed before if assignment otherwise ARI exceptions
# are not handled correctly.

BEGIN { doc["d_namelen"] = "\
Do not use dirent.d_namelen, instead use NAMELEN"
    category["d_namelen"] = ari_regression
}
/(^|[^_[:alnum:]])d_namelen([^_[:alnum:]]|$)/ {
    fail("d_namelen")
}

BEGIN { doc["strlen d_name"] = "\
Do not use strlen dirent.d_name, instead use NAMELEN"
    category["strlen d_name"] = ari_regression
}
/(^|[^_[:alnum:]])strlen[[:space:]]*\(.*[^_[:alnum:]]d_name([^_[:alnum:]]|$)/ {
    fail("strlen d_name")
}

BEGIN { doc["generic_use_struct_convention"] = "\
Replace generic_use_struct_convention with nothing, \
EXTRACT_STRUCT_VALUE_ADDRESS is a predicate"
    category["generic_use_struct_convention"] = ari_regression
}
/(^|[^_[:alnum:]])generic_use_struct_convention([^_[:alnum:]]|$)/ {
    fail("generic_use_struct_convention")
}

BEGIN { doc["if assignment"] = "\
An IF statement'\''s expression contains an assignment (the GNU coding \
standard discourages this)"
    category["if assignment"] = ari_code
}
BEGIN { doc["if clause more than 50 lines"] = "\
An IF statement'\''s expression expands over 50 lines"
    category["if clause more than 50 lines"] = ari_code
}
#
# Accumulate continuation lines
FNR == 1 {
    in_if = 0
}

/(^|[^_[:alnum:]])if / {
    in_if = 1;
    if_brace_level = 0;
    if_cont_p = 0;
    if_count = 0;
    if_brace_end_pos = 0;
    if_full_line = "";
}
(in_if)  {
    # We want everything up to closing brace of same level
    if_count++;
    if (if_count > 50) {
	print "multiline if: " if_full_line $0
	fail("if clause more than 50 lines")
	if_brace_level = 0;
	if_full_line = "";
    } else {
	if (if_count == 1) {
	    i = index($0,"if ");
	} else {
	    i = 1;
	}
	for (i=i; i <= length($0); i++) {
	    char = substr($0,i,1);
	    if (char == "(") { if_brace_level++; }
	    if (char == ")") {
		if_brace_level--;
		if (!if_brace_level) {
		    if_brace_end_pos = i;
		    after_if = substr($0,i+1,length($0));
		    # Do not parse what is following
		    break;
		}
	    }
	}
	if (if_brace_level == 0) {
	    $0 = substr($0,1,i);
	    in_if = 0;
	} else {
	    if_full_line = if_full_line $0;
	    if_cont_p = 1;
	    next;
	}
    }
}
# if we arrive here, we need to concatenate, but we are at brace level 0

(if_brace_end_pos) {
    $0 = if_full_line substr($0,1,if_brace_end_pos);
    if (if_count > 1) {
	# print "IF: multi line " if_count " found at " FILENAME ":" FNR " \"" $0 "\""
    }
    if_cont_p = 0;
    if_full_line = "";
}
/(^|[^_[:alnum:]])if .* = / {
    # print "fail in if " $0
    fail("if assignment")
}
(if_brace_end_pos) {
    $0 = $0 after_if;
    if_brace_end_pos = 0;
    in_if = 0;
}

# Printout of all found bug

BEGIN {
    if (print_doc) {
	for (bug in doc) {
	    fail(bug)
	}
	exit
    }
}' "$@"

