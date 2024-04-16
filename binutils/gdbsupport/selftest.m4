dnl Copyright (C) 2018-2024 Free Software Foundation, Inc.
dnl
dnl This file is part of GDB.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

dnl GDB_AC_SELFTEST(ACTION-IF-ENABLED)
dnl
dnl Enable the unit/self tests if needed.  If they are enabled, AC_DEFINE
dnl the GDB_SELF_TEST macro, and execute ACTION-IF-ENABLED.

AC_DEFUN([GDB_AC_SELFTEST],[
# Check whether we will enable the inclusion of unit tests when
# compiling GDB.
#
# The default value of this option changes depending whether we're on
# development mode (in which case it's "true") or not (in which case
# it's "false").  The $development variable is set by the GDB_AC_COMMON
# macro, which must therefore be used before GDB_AC_SELFTEST.

AS_IF([test "x$development" != xtrue && test "x$development" != xfalse],
  [AC_MSG_ERROR([Invalid value for \$development, got "$development", expecting "true" or "false".])])

AC_ARG_ENABLE(unit-tests,
AS_HELP_STRING([--enable-unit-tests],
[Enable the inclusion of unit tests when compiling GDB]),
[case "${enableval}" in
  yes)  enable_unittests=true  ;;
  no)   enable_unittests=false ;;
  *)    AC_MSG_ERROR(
[bad value ${enableval} for --{enable,disable}-unit-tests option]) ;;
esac], [enable_unittests=$development])

if $enable_unittests; then
  AC_DEFINE(GDB_SELF_TEST, 1,
            [Define if self-testing features should be enabled])
  $1
fi
])
