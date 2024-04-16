#!/bin/sh

# A shell script to run the test suite on the DJGPP version of GDB.

#  Copyright (C) 2000-2024 Free Software Foundation, Inc.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

ORIGDIR=`pwd`
GDB=${ORIGDIR}/../gdb.exe
SUBDIRS=`find $ORIGDIR -type d ! -ipath $ORIGDIR`

for d in $SUBDIRS
do
  cd $d
  echo "Running tests in $d..."
  for f in *.out
  do
    test -f $f || break
    base=`basename $f .out`
    if test "${base}" = "dbx" ; then
	options=-dbx
    else
	options=
    fi
    $GDB ${options} < ${base}.in 2>&1 \
      | sed -e '/GNU gdb /s/ [.0-9][.0-9]*//' \
            -e '/^Copyright/s/[12][0-9][0-9][0-9]/XYZZY/g' \
            -e '/Starting program: /s|[A-z]:/.*/||' \
            -e '/main (/s/=0x[0-9a-f][0-9a-f]*/=XYZ/g' \
      > ${base}.tst
    if diff --binary -u ${base}.out ${base}.tst ; then
      rm -f ${base}.tst
    fi
  done
done

