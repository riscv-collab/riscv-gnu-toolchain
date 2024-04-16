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
dnl --enable-sim-stdio is for users of the simulator
dnl It determines if IO from the program is routed through STDIO (buffered)
AC_DEFUN([SIM_AC_OPTION_STDIO],
[dnl
AC_MSG_CHECKING([for sim stdio debug behavior])
sim_stdio="0"
AC_ARG_ENABLE(sim-stdio,
[AS_HELP_STRING([--enable-sim-stdio],
		[Specify whether to use stdio for console input/output])],
[case "${enableval}" in
  yes)	sim_stdio="DO_USE_STDIO";;
  no)	sim_stdio="DONT_USE_STDIO";;
  *)	AC_MSG_ERROR([Unknown value $enableval passed to --enable-sim-stdio]);;
esac])dnl
AC_DEFINE_UNQUOTED([WITH_STDIO], [$sim_stdio], [How to route I/O])
AC_MSG_RESULT($sim_stdio)
])
