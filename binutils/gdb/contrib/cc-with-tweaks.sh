#!/usr/bin/env bash
# Wrapper around gcc to tweak the output in various ways when running
# the testsuite.

# Copyright (C) 2010-2024 Free Software Foundation, Inc.
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

# This program requires gdb and objcopy in addition to gcc.
# The default values are gdb from the build tree and objcopy from $PATH.
# They may be overridden by setting environment variables GDB and OBJCOPY
# respectively.  Note that GDB should contain the gdb binary as well as the
# -data-directory flag, e.g., "foo/gdb -data-directory foo/data-directory".
# We assume the current directory is either $obj/gdb or $obj/gdb/testsuite.
#
# Example usage:
#
# bash$ cd $objdir/gdb/testsuite
# bash$ runtest \
#   CC_FOR_TARGET="/bin/bash $srcdir/gdb/contrib/cc-with-tweaks.sh ARGS gcc" \
#   CXX_FOR_TARGET="/bin/bash $srcdir/gdb/contrib/cc-with-tweaks.sh ARGS g++"
#
# For documentation on Fission and dwp files:
#     http://gcc.gnu.org/wiki/DebugFission
#     http://gcc.gnu.org/wiki/DebugFissionDWP
# For documentation on index files: info -f gdb.info -n "Index Files"
# For information about 'dwz', see the announcement:
#     http://gcc.gnu.org/ml/gcc/2012-04/msg00686.html
# (More documentation is to come.)

# ARGS determine what is done.  They can be:
# -Z invoke objcopy --compress-debug-sections
# -z compress using dwz
# -m compress using dwz -m
# -i make an index (.gdb_index)
# -c make an index (currently .gdb_index) in a cache dir
# -n make a dwarf5 index (.debug_names)
# -p create .dwp files (Fission), you need to also use gcc option -gsplit-dwarf
# -l creates separate debuginfo files linked to using .gnu_debuglink
# If nothing is given, no changes are made

myname=cc-with-tweaks.sh
mydir=`dirname "$0"`

if [ -z "$GDB" ]
then
    if [ -f ./gdb ]
    then
	GDB="./gdb -data-directory data-directory"
    elif [ -f ../gdb ]
    then
	GDB="../gdb -data-directory ../data-directory"
    elif [ -f ../../gdb ]
    then
	GDB="../../gdb -data-directory ../../data-directory"
    else
	echo "$myname: unable to find usable gdb" >&2
	exit 1
    fi
fi

OBJCOPY=${OBJCOPY:-objcopy}
READELF=${READELF:-readelf}

DWZ=${DWZ:-dwz}
DWP=${DWP:-dwp}

# shellcheck disable=SC2206 # Allow word splitting.
STRIP_ARGS_STRIP_DEBUG=(${STRIP_ARGS_STRIP_DEBUG:---strip-debug})
# shellcheck disable=SC2206 # Allow word splitting.
STRIP_ARGS_KEEP_DEBUG=(${STRIP_ARGS_KEEP_DEBUG:---only-keep-debug})

have_link=unknown
next_is_output_file=no
output_file=a.out

want_index=false
index_options=""
want_index_cache=false
want_dwz=false
want_multi=false
want_dwp=false
want_objcopy_compress=false
want_gnu_debuglink=false

while [ $# -gt 0 ]; do
    case "$1" in
	-Z) want_objcopy_compress=true ;;
	-z) want_dwz=true ;;
	-i) want_index=true ;;
	-n) want_index=true; index_options=-dwarf-5;;
	-c) want_index_cache=true ;;
	-m) want_multi=true ;;
	-p) want_dwp=true ;;
	-l) want_gnu_debuglink=true ;;
	*) break ;;
    esac
    shift
done

if [ "$want_index" = true ]
then
    if [ -z "$GDB_ADD_INDEX" ]
    then
	if [ -f $mydir/gdb-add-index.sh ]
	then
	    GDB_ADD_INDEX="$mydir/gdb-add-index.sh"
	else
	    echo "$myname: unable to find usable contrib/gdb-add-index.sh" >&2
	    exit 1
	fi
    fi
fi

for arg in "$@"
do
    if [ "$next_is_output_file" = "yes" ]
    then
	output_file="$arg"
	next_is_output_file=no
	continue
    fi

    # Poor man's gcc argument parser.
    # We don't need to handle all arguments, we just need to know if we're
    # doing a link and what the output file is.
    # It's not perfect, but it seems to work well enough for the task at hand.
    case "$arg" in
    "-c") have_link=no ;;
    "-E") have_link=no ;;
    "-S") have_link=no ;;
    "-o") next_is_output_file=yes ;;
    esac
done

if [ "$next_is_output_file" = "yes" ]
then
    echo "$myname: Unable to find output file" >&2
    exit 1
fi

if [ "$have_link" = "no" ]
then
    "$@"
    exit $?
fi

output_dir="${output_file%/*}"
[ "$output_dir" = "$output_file" ] && output_dir="."

"$@"
rc=$?
[ $rc != 0 ] && exit $rc
if [ ! -f "$output_file" ]
then
    echo "$myname: Internal error: $output_file missing." >&2
    exit 1
fi

get_tmpdir ()
{
    subdir="$1"
    if [ "$subdir" = "" ]; then
	subdir=.tmp
    fi

    tmpdir=$(dirname "$output_file")/"$subdir"
    mkdir -p "$tmpdir"
}

if [ "$want_objcopy_compress" = true ]; then
    $OBJCOPY --compress-debug-sections "$output_file"
    rc=$?
    [ $rc != 0 ] && exit $rc
fi

if [ "$want_index" = true ]; then
    get_tmpdir
    mv "$output_file" "$tmpdir"
    output_dir=$(dirname "$output_file")

    # Copy .dwo file alongside, to fix gdb.dwarf2/fission-relative-dwo.exp.
    # Use copy instead of move to not break
    # rtf=gdb.dwarf2/fission-absolute-dwo.exp.
    dwo_pattern="$output_dir/*.dwo"
    for f in $dwo_pattern; do
	if [ "$f" = "$dwo_pattern" ]; then
	    break
	fi
	cp "$f" "$tmpdir"
    done

    tmpfile="$tmpdir/$(basename $output_file)"
    # Filter out these messages which would stop dejagnu testcase run:
    # echo "$myname: No index was created for $file" 1>&2
    # echo "$myname: [Was there no debuginfo? Was there already an index?]" 1>&2
    GDB=$GDB $GDB_ADD_INDEX $index_options "$tmpfile" 2>&1 \
	| grep -v "^${GDB_ADD_INDEX##*/}: " >&2
    rc=${PIPESTATUS[0]}
    mv "$tmpfile" "$output_file"
    rm -f "$tmpdir"/*.dwo
    [ $rc != 0 ] && exit $rc
fi

if [ "$want_index_cache" = true ]; then
    $GDB -q -batch \
	-ex "set index-cache directory $INDEX_CACHE_DIR" \
	-ex "set index-cache enabled on" \
	-ex "file $output_file"
    rc=$?
    [ $rc != 0 ] && exit $rc
fi

if [ "$want_dwz" = true ] || [ "$want_multi" = true ]; then
    # Require dwz version with PR dwz/24468 fixed.
    dwz_version_major_required=0
    dwz_version_minor_required=13
    dwz_version_line=$($DWZ --version 2>&1 | head -n 1)
    dwz_version=${dwz_version_line//dwz version /}
    dwz_version_major=${dwz_version//\.*/}
    dwz_version_minor=${dwz_version//*\./}
    if [ "$dwz_version_major" -lt "$dwz_version_major_required" ] \
	   || { [ "$dwz_version_major" -eq "$dwz_version_major_required" ] \
		    && [ "$dwz_version_minor" -lt "$dwz_version_minor_required" ]; }; then
	detected="$dwz_version_major.$dwz_version_minor"
	required="$dwz_version_major_required.$dwz_version_minor_required"
	echo "$myname: dwz version $detected detected, version $required or higher required"
	exit 1
    fi
fi

if [ "$want_dwz" = true ]; then
    # Validate dwz's result by checking if the executable was modified.
    cp "$output_file" "${output_file}.copy"
    $DWZ "$output_file" > /dev/null
    cmp "$output_file" "$output_file.copy" > /dev/null
    cmp_rc=$?
    rm -f "${output_file}.copy"

    case $cmp_rc in
    0)
	echo "$myname: dwz did not modify ${output_file}."
        exit 1
	;;
    1)
	# File was modified, great.
	;;
    *)
	# Other cmp error, it presumably has already printed something on
	# stderr.
	exit 1
	;;
    esac
elif [ "$want_multi" = true ]; then
    get_tmpdir
    dwz_file=$tmpdir/$(basename "$output_file").dwz
    # Remove the dwz output file if it exists, so we don't mistake it for a
    # new file in case dwz fails.
    rm -f "$dwz_file"

    cp $output_file ${output_file}.alt
    $DWZ -m "$dwz_file" "$output_file" ${output_file}.alt > /dev/null
    rm -f ${output_file}.alt

    # Validate dwz's work by checking if the expected output file exists.
    if [ ! -f "$dwz_file" ]; then
	echo "$myname: dwz file $dwz_file missing."
	exit 1
    fi
fi

if [ "$want_dwp" = true ]; then
    dwo_files=$($READELF -wi "${output_file}" | grep _dwo_name | \
	sed -e 's/^.*: //' | sort | uniq)
    rc=0
    if [ -n "$dwo_files" ]; then
	$DWP -o "${output_file}.dwp" ${dwo_files} > /dev/null
	rc=$?
	[ $rc != 0 ] && exit $rc
	rm -f ${dwo_files}
    fi
fi

if [ "$want_gnu_debuglink" = true ]; then
    # Based on gdb_gnu_strip_debug.

    # Gdb looks for the .gnu_debuglink file in the .debug subdirectory
    # of the directory of the executable.
    get_tmpdir .debug

    stripped_file="$tmpdir"/$(basename "$output_file").stripped
    debug_file="$tmpdir"/$(basename "$output_file").debug

    # Create stripped and debug versions of output_file.
    strip "${STRIP_ARGS_STRIP_DEBUG[@]}" "${output_file}" \
	  -o "${stripped_file}"
    rc=$?
    [ $rc != 0 ] && exit $rc
    strip "${STRIP_ARGS_KEEP_DEBUG[@]}" "${output_file}" \
	  -o "${debug_file}"
    rc=$?
    [ $rc != 0 ] && exit $rc

    # The .gnu_debuglink is supposed to contain no leading directories.
    link=$(basename "${debug_file}")

    (
	# Temporarily cd to tmpdir to allow objcopy to find $link
	cd "$tmpdir" || exit 1

	# Overwrite output_file with stripped version containing
	# .gnu_debuglink to debug_file.
	$OBJCOPY --add-gnu-debuglink="$link" "${stripped_file}" \
		"${output_file}"
	rc=$?
	[ $rc != 0 ] && exit $rc
    )
fi

exit $rc
