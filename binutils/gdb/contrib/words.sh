#!/bin/sh

# Copyright (C) 2019-2024 Free Software Foundation, Inc.
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

# This script intends to facilitate spell checking of source/doc files.
# It:
# - transforms the files into a list of lowercase words
# - prefixes each word with the frequency
# - filters out words within a frequency range
# - sorts the words, longest first
#
# If '-c' is passed as option, it operates on the C comments only, rather than
# on the entire file.
#
# For:
# ...
# $ files=$(find gdb -type f -name "*.c" -o -name "*.h")
# $ ./gdb/contrib/words.sh -c $files
# ...
# it generates a list of ~15000 words prefixed with frequency.
#
# This could be used to generate a dictionary that is kept as part of the
# sources, against which new code can be checked, generating a warning or
# error.  The hope is that misspellings would trigger this frequently, and rare
# words rarely, otherwise the burden of updating the dictionary would be too
# much.
#
# And for:
# ...
# $ files=$(find gdb -type f -name "*.c" -o -name "*.h")
# $ ./gdb/contrib/words.sh -c -f 1 $files
# ...
# it generates a list of ~5000 words with frequency 1.
#
# This can be used to scan for misspellings manually.
#

minfreq=
maxfreq=
c=false
while [ $# -gt 0 ]; do
    case "$1" in
	-c)
	    c=true
	    shift
	    ;;
	--freq|-f)
	    minfreq=$2
	    maxfreq=$2
	    shift 2
	    ;;
	--min)
	    minfreq=$2
	    if [ "$maxfreq" = "" ]; then
		maxfreq=0
	    fi
	    shift 2
	    ;;
	--max)
	    maxfreq=$2
	    if [ "$minfreq" = "" ]; then
		minfreq=0
	    fi
	    shift 2
	    ;;
	*)
	    break;
	    ;;
    esac
done

if [ "$minfreq" = "" ] && [ "$maxfreq" = "" ]; then
    minfreq=0
    maxfreq=0
fi

awkfile=$(mktemp)
trap 'rm -f "$awkfile"' EXIT

cat > "$awkfile" <<EOF
BEGIN {
    in_comment=0
}

// {
    line=\$0
}

/\/\*/ {
    in_comment=1
    sub(/.*\/\*/, "", line)
}

/\*\// {
    sub(/\*\/.*/, "", line)
    in_comment=0
    print line
    next
}

// {
    if (in_comment) {
	print line
    }
}
EOF

# Stabilize sort.
export LC_ALL=C

if $c; then
    awk \
	-f "$awkfile" \
	-- "$@"
else
    cat "$@"
fi \
    | sed \
	  -e 's/[!"?;:%^$~#{}`&=@,. \t\/_()|<>\+\*-]/\n/g' \
	  -e 's/\[/\n/g' \
	  -e 's/\]/\n/g' \
	  -e "s/'/\n/g" \
	  -e 's/[0-9][0-9]*/\n/g' \
	  -e 's/[ \t]*//g' \
    | tr '[:upper:]' '[:lower:]' \
    | sort \
    | uniq -c \
    | awk "{ if (($minfreq == 0 || $minfreq <= \$1) \
                 && ($maxfreq == 0 || \$1 <= $maxfreq)) { print \$0; } }" \
    | awk '{ print length($0) " " $0; }' \
    | sort -n -r \
    | cut -d ' ' -f 2-
