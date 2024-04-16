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
AC_DEFUN([SIM_AC_OPTION_XOR_ENDIAN],
[
AC_MSG_CHECKING([for xor endian support])
default_sim_xor_endian="ifelse([$1],,8,[$1])"
sim_xor_endian="$default_sim_xor_endian"
AC_ARG_ENABLE(sim-xor-endian,
[AS_HELP_STRING([--enable-sim-xor-endian=n],
		[Specify number bytes involved in XOR bi-endian mode (default ${default_sim_xor_endian})])],
[case "${enableval}" in
  yes)	sim_xor_endian="8";;
  no)	sim_xor_endian="0";;
  *)	sim_xor_endian="$enableval";;
esac])dnl
AC_DEFINE_UNQUOTED([WITH_XOR_ENDIAN], [$sim_xor_endian], [Sim XOR endian settings])
AC_MSG_RESULT($sim_smp)
])
