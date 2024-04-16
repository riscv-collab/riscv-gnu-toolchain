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
dnl --enable-sim-endian={yes,no,big,little} is for simulators
dnl that support both big and little endian targets.
AC_DEFUN([SIM_AC_OPTION_ENDIAN],
[dnl
AC_MSG_CHECKING([whether to force sim endianness])
sim_endian=
AC_ARG_ENABLE(sim-endian,
[AS_HELP_STRING([--enable-sim-endian=endian],
		[Specify target byte endian orientation])],
[case "${enableval}" in
  b*|B*) sim_endian="BFD_ENDIAN_BIG";;
  l*|L*) sim_endian="BFD_ENDIAN_LITTLE";;
  yes | no) ;;
  *)	 AC_MSG_ERROR("Unknown value $enableval for --enable-sim-endian");;
esac])dnl
AC_DEFINE_UNQUOTED([WITH_TARGET_BYTE_ORDER],
		   [${sim_endian:-BFD_ENDIAN_UNKNOWN}], [Sim endian settings])
AC_MSG_RESULT([${sim_alignment:-no}])
])dnl
