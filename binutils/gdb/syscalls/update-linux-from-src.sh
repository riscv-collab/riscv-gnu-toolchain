#!/bin/sh

# Copyright (C) 2022-2024 Free Software Foundation, Inc.
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

# Used to generate .xml.in files, like so:
# $ ./update-linux-from-src.sh ~/linux-stable.git

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
    f="$1"

    year=$(date +%Y)

    cat <<EOF
<?xml version="1.0"?>
<!-- Copyright (C) $start_date-$year Free Software Foundation, Inc.

     Copying and distribution of this file, with or without modification,
     are permitted in any medium without royalty provided the copyright
     notice and this notice are preserved.  -->

<!DOCTYPE feature SYSTEM "gdb-syscalls.dtd">

<!-- This file was generated using the following file:

     $f

     The file mentioned above belongs to the Linux Kernel.  -->


EOF

    echo '<syscalls_info>'
}


post ()
{
    echo '</syscalls_info>'
}

one ()
{
    f="$1"
    abi="$2"
    start_date="$3"
    offset="$4"

    pre "$f" "$start_date"

    grep -v "^#" "$d/$f" \
	| awk '{print $2, $3, $1}' \
	| grep -E "^$abi" \
	| grep -E -v " (reserved|unused)[0-9]+ " \
	| awk "{printf \"  <syscall name=\\\"%s\\\" number=\\\"%s\\\"/>\n\", \$2, \$3 + $offset}"

    post
}

for f in *.in; do
    start_date=2009
    offset=0

    case $f in
	amd64-linux.xml.in)
	    t="arch/x86/entry/syscalls/syscall_64.tbl"
	    abi="(common|64)"
	    ;;
	i386-linux.xml.in)
	    t="arch/x86/entry/syscalls/syscall_32.tbl"
	    abi=i386
	    ;;
	ppc64-linux.xml.in)
	    t="arch/powerpc/kernel/syscalls/syscall.tbl"
	    abi="(common|64|nospu)"
	    ;;
	ppc-linux.xml.in)
	    t="arch/powerpc/kernel/syscalls/syscall.tbl"
	    abi="(common|32|nospu)"
	    ;;
	s390-linux.xml.in)
	    t="arch/s390/kernel/syscalls/syscall.tbl"
	    abi="(common|32)"
	    ;;
	s390x-linux.xml.in)
	    t="arch/s390/kernel/syscalls/syscall.tbl"
	    abi="(common|64)"
	    ;;
	sparc64-linux.xml.in)
	    t="arch/sparc/kernel/syscalls/syscall.tbl"
	    abi="(common|64)"
	    start_date="2010"
	    ;;
	sparc-linux.xml.in)
	    t="arch/sparc/kernel/syscalls/syscall.tbl"
	    abi="(common|32)"
	    start_date="2010"
	    ;;
	mips-n32-linux.xml.in)
	    t="arch/mips/kernel/syscalls/syscall_n32.tbl"
	    abi="n32"
	    start_date="2011"
	    offset=6000
	    ;;
	mips-n64-linux.xml.in)
	    t="arch/mips/kernel/syscalls/syscall_n64.tbl"
	    abi="n64"
	    start_date="2011"
	    offset=5000
	    ;;
	mips-o32-linux.xml.in)
	    t="arch/mips/kernel/syscalls/syscall_o32.tbl"
	    abi="o32"
	    start_date="2011"
	    offset=4000
	    ;;
	bfin-linux.xml.in)
	    echo "Skipping $f, no longer supported"
	    continue
	    ;;
	aarch64-linux.xml.in)
	    echo "Skipping $f, no syscall.tbl"
	    continue
	    ;;
	arm-linux.xml.in)
	    echo "Skipping $f, use arm-linux.py instead"
	    continue
	    ;;
	linux-defaults.xml.in)
	    continue
	    ;;
	*)
	    echo "Don't know how to generate $f"
	    continue
	    ;;
    esac

    echo "Generating $f"
    one "$t" "$abi" "$start_date" "$offset" > "$f"

done
