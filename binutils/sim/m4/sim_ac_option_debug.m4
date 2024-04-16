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
dnl --enable-sim-debug is for developers of the simulator
dnl the allowable values are work-in-progress
AC_DEFUN([SIM_AC_OPTION_DEBUG],
[dnl
AC_MSG_CHECKING([for sim debug setting])
sim_debug="0"
AC_ARG_ENABLE(sim-debug,
[AS_HELP_STRING([--enable-sim-debug=opts],
		[Enable debugging flags (for developers of the sim itself)])],
[case "${enableval}" in
  yes) sim_debug="7";;
  no)  sim_debug="0";;
  *)   sim_debug="($enableval)";;
esac])dnl
if test "$sim_debug" != "0"; then
  AC_DEFINE_UNQUOTED([DEBUG], [$sim_debug], [Sim debug setting])
fi
AC_DEFINE_UNQUOTED([WITH_DEBUG], [$sim_debug], [Sim debug setting])
AC_MSG_RESULT($sim_debug)
])
