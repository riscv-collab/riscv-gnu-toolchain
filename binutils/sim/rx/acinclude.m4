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
AC_MSG_CHECKING([whether sim rx should be cycle accurate])
AC_ARG_ENABLE(sim-rx-cycle-accurate,
[AS_HELP_STRING([--disable-sim-rx-cycle-accurate],
		[Disable cycle accurate simulation (faster runtime)])],
[case "${enableval}" in
yes | no) ;;
*)	AC_MSG_ERROR(bad value ${enableval} given for --enable-sim-rx-cycle-accurate option) ;;
esac])
if test "x${enable_sim_rx_cycle_accurate}" != xno; then
  SIM_RX_CYCLE_ACCURATE_FLAGS="-DCYCLE_ACCURATE"
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi
AC_SUBST(SIM_RX_CYCLE_ACCURATE_FLAGS)
