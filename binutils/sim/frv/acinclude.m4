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
dnl NB: This file is included in sim/configure, so keep settings namespaced.
dnl
dnl Enable making unknown traps dump out registers
AC_MSG_CHECKING([whether sim frv should dump cpu state on unknown traps])
AC_ARG_ENABLE(sim-frv-trapdump,
[AS_HELP_STRING([--enable-sim-frv-trapdump],
		[Make unknown traps dump the registers])],
[case "${enableval}" in
yes|no) ;;
*) AC_MSG_ERROR("Unknown value $enableval passed to --enable-sim-trapdump");;
esac])
if test "x${enable_sim_frv_trapdump}" = xyes; then
  SIM_FRV_TRAPDUMP_FLAGS="-DTRAPDUMP=1"
  AC_MSG_RESULT([yes])
else
  SIM_FRV_TRAPDUMP_FLAGS="-DTRAPDUMP=0"
  AC_MSG_RESULT([no])
fi
AC_SUBST(SIM_FRV_TRAPDUMP_FLAGS)
