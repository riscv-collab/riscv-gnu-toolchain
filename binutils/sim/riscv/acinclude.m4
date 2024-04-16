dnl Copyright (C) 2022-2024 Free Software Foundation, Inc.
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
dnl NB: This file is included in sim/configure, so keep settings namespaced.
AC_MSG_CHECKING([riscv bitsize])
SIM_RISCV_BITSIZE=64
AS_CASE([$target],
	[riscv32*], [SIM_RISCV_BITSIZE=32])
AC_MSG_RESULT([$SIM_RISCV_BITSIZE])
AC_SUBST(SIM_RISCV_BITSIZE)
