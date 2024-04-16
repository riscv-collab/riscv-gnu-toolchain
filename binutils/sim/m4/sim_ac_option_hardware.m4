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
dnl --enable-sim-hardware is for users of the simulator
AC_DEFUN([SIM_AC_OPTION_HARDWARE],
[dnl
AC_MSG_CHECKING([for sim hardware settings])
AC_ARG_ENABLE(sim-hardware,
  [AS_HELP_STRING([--enable-sim-hardware],
		  [Whether to enable hardware/device simulation])],
  ,[enable_sim_hardware="yes"])
sim_hw_sockser=
if test "$enable_sim_hardware" = no; then
  sim_hw_cflags="-DWITH_HW=0"
elif test "$enable_sim_hardware" = yes; then
  sim_hw_cflags="-DWITH_HW=1"
  dnl TODO: We don't add dv-sockser to sim_hw as it is not a "real" device
  dnl that you instatiate.  Instead, other code will call into it directly.
  dnl At some point, we should convert it over.
  sim_hw_sockser="dv-sockser.o"
  sim_hw_cflags="$sim_hw_cflags -DHAVE_DV_SOCKSER"
else
  AC_MSG_ERROR([unknown argument "$enable_sim_hardware"])
fi
AM_CONDITIONAL([SIM_ENABLE_HW], [test "$enable_sim_hardware" = "yes"])
AC_MSG_RESULT(${enable_sim_hardware})
SIM_HW_CFLAGS=$sim_hw_cflags
AC_SUBST(SIM_HW_CFLAGS)
SIM_HW_SOCKSER=$sim_hw_sockser
AC_SUBST(SIM_HW_SOCKSER)
])
