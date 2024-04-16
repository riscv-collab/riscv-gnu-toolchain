#! /bin/sh
# runtest wrapper to reliably reproduce racy incomplete reads in the testsuite.

# Copyright (C) 2013-2024 Free Software Foundation, Inc.
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

# This tool exercises any incomplete reads handling in the testsuite by
# simulating read always returns just 1 character.
# Testsuite incompatibilities are tracked as GDB PR testsuite/12649.

# Example usage:
#
# bash$ cd $objdir/gdb/testsuite
# bash$ EXPECT=$srcdir/gdb/contrib/expect-read1.sh runtest
# or
# bash$ EXPECT=../contrib/expect-read1.sh runtest

C=`echo $0|sed 's/\.sh$/.c/'`
if ! test -e $C; then
  echo >&2 "$0: Cannot find 'srcdir/gdb/contrib/expect-read1.c' at '$C'."
  exit 2
fi
SO=/tmp/expect-read1.$$.so
rm -f $SO
CMD="${CC_FOR_TARGET:-gcc} -o $SO -Wall -fPIC -shared $C"
if ! $CMD; then
  echo >&2 "$0: Failed: $CMD"
  exit 2
fi
trap "rm -f $SO" EXIT
LD_PRELOAD=$SO expect "$@"
