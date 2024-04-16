#! /bin/sh

# Copyright (C) 2018-2024 Free Software Foundation, Inc.
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

# Usage: update-freebsd.sh <path-to-syscall.h>
# Update the freebsd.xml file.
#
# FreeBSD uses the same list of system calls on all architectures.
# The list is defined in the sys/kern/syscalls.master file in the
# FreeBSD source tree.  This file is used as an input to generate
# several files that are also stored in FreeBSD's source tree.  This
# script parses one of those generated files (sys/sys/syscall.h)
# rather than syscalls.master as syscall.h is easier to parse.

if [ $# -ne 1 ]; then
   echo "Error: Path to syscall.h missing. Aborting."
   echo "Usage: update-freebsd.sh <path-to-syscall.h>"
   exit 1
fi

year=$(date +%Y)

cat > freebsd.xml.tmp <<EOF
<?xml version="1.0"?> <!-- THIS FILE IS GENERATED -*- buffer-read-only: t -*-  -->
<!-- vi:set ro: -->
<!-- Copyright (C) 2009-$year Free Software Foundation, Inc.

     Copying and distribution of this file, with or without modification,
     are permitted in any medium without royalty provided the copyright
     notice and this notice are preserved.  -->

<!DOCTYPE feature SYSTEM "gdb-syscalls.dtd">

<!-- This file was generated using the following file:

     /usr/src/sys/sys/syscall.h

     The file mentioned above belongs to the FreeBSD Kernel.  -->

<syscalls_info>
EOF

awk '
/MAXSYSCALL/ {
    next
}
/^#define/ {
    sub(/^SYS_/,"",$2);
    printf "  <syscall name=\"%s\" number=\"%s\"", $2, $3
    if (sub(/^freebsd[0-9]*_/,"",$2) != 0)
        printf " alias=\"%s\"", $2
    printf "/>\n"
}
/\/\* [0-9]* is obsolete [a-z_]* \*\// {
    printf "  <syscall name=\"%s\" number=\"%s\"/>\n", $5, $2
}
/\/\* [0-9]* is freebsd[0-9]* [a-z_]* \*\// {
    printf "  <syscall name=\"%s_%s\" number=\"%s\" alias=\"%s\"/>\n", $4, $5, $2, $5
}' "$1" >> freebsd.xml.tmp

cat >> freebsd.xml.tmp <<EOF
</syscalls_info>
EOF

../../move-if-change freebsd.xml.tmp freebsd.xml
