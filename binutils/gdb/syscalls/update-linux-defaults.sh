#!/bin/sh

# Copyright (C) 2023-2024 Free Software Foundation, Inc.
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

# Used to generate linux-defaults.xml.in, like so:
# $ ./update-linux-defaults.sh ~/strace.git

if [ $# -lt 1 ]; then
    echo "dir argument needed"
    exit 1
fi

d="$1"
shift

if [ ! -d "$d" ]; then
    echo "cannot find $d"
    exit 1
fi

pre ()
{
    year=$(date +%Y)

    cat <<EOF
<?xml version="1.0"?>
<!-- Copyright (C) 2009-$year Free Software Foundation, Inc.

     Copying and distribution of this file, with or without modification,
     are permitted in any medium without royalty provided the copyright
     notice and this notice are preserved.  -->

<!-- This file was generated using the sources from strace.  -->
EOF

    echo '<syscalls_defaults>'
}


post ()
{
    echo '</syscalls_defaults>'
}

generate ()
{
    pre

    grep -rn -E "T[A-Z][,|]" "$d/src/linux/" \
	| sed -e 's/\(T[A-Z][,|].*\)/\x03&/' -e 's/.*\x03//' \
	      -e 's/,[ \t]*SEN[ \t]*(/, SEN(/g' \
	| grep ", SEN(" \
	| sed -e 's/\(.*\"\).*/\1/g' \
	      -e 's/#64\"/\"/g' \
	| awk '{print $3 " " $1}' \
	| sort -u \
	| sed -e 's/|/,/g' \
	      -e 's/TD,/descriptor,/g' \
	      -e 's/TF,/file,/g' \
	      -e 's/TI,/ipc,/g' \
	      -e 's/TM,/memory,/g' \
	      -e 's/TN,/network,/g' \
	      -e 's/TP,/process,/g' \
	      -e 's/TS,/signal,/g' \
	      -e 's/[A-Z]\+,//g' \
	| grep -v '" $' \
	| sed 's/,$//g' \
	| awk "{printf \"  <syscall name=%s groups=\\\"%s\\\"/>\n\", \$1, \$2}"

    post
}

f=linux-defaults.xml.in

echo "Generating $f"
generate > "$f"
