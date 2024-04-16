dnl Copyright (C) 1997-2024 Free Software Foundation, Inc.
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
dnl
dnl Conditionally compile in assertion statements.
AC_DEFUN([SIM_AC_OPTION_ASSERT],
[
AC_MSG_CHECKING([whether to enable sim asserts])
sim_assert="1"
AC_ARG_ENABLE(sim-assert,
[AS_HELP_STRING([--enable-sim-assert],
		[Specify whether to perform random assertions])],
[case "${enableval}" in
  yes)	sim_assert="1";;
  no)	sim_assert="0";;
  *)	AC_MSG_ERROR([--enable-sim-assert does not take a value]);;
esac])dnl
AC_DEFINE_UNQUOTED([WITH_ASSERT], [$sim_assert], [Sim assert settings])
AC_MSG_RESULT($sim_assert)
])
