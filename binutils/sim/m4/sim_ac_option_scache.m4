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
AC_DEFUN([SIM_AC_OPTION_SCACHE],
[dnl
AC_MSG_CHECKING([for sim cache size])
sim_scache="16384"
AC_ARG_ENABLE(sim-scache,
[AS_HELP_STRING([--enable-sim-scache=size],
		[Specify simulator execution cache size])],
[case "${enableval}" in
  yes)	;;
  no)	sim_scache="0";;
  [[0-9]]*) sim_scache="${enableval}";;
  *)	AC_MSG_ERROR("Bad value $enableval passed to --enable-sim-scache");;
esac])
AC_DEFINE_UNQUOTED([WITH_SCACHE], [$sim_scache], [Sim cache szie])
AC_MSG_RESULT($sim_scache)
])
