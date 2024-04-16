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
AC_DEFUN([SIM_AC_OPTION_RESERVED_BITS],
[dnl
AC_MSG_CHECKING([whether to check reserved bits in instruction])
AC_ARG_ENABLE(sim-reserved-bits,
[AS_HELP_STRING([--enable-sim-reserved-bits],
		[Specify whether to check reserved bits in instruction])],
[case "${enableval}" in
yes|no) ;;
*) AC_MSG_ERROR("--enable-sim-reserved-bits does not take a value");;
esac])
if test "x${enable_sim_reserved_bits}" != xno; then
  sim_reserved_bits=1
  AC_MSG_RESULT([yes])
else
  sim_reserved_bits=0
  AC_MSG_RESULT([no])
fi
AC_DEFINE_UNQUOTED([WITH_RESERVED_BITS], [$sim_reserved_bits], [Sim reserved bits setting])
])
