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
dnl --enable-sim-float is for developers of the simulator
dnl It specifies the presence of hardware floating point
dnl And optionally the bitsize of the floating point register.
dnl arg[1] specifies the presence (or absence) of floating point hardware
dnl arg[2] specifies the number of bits in a floating point register
AC_DEFUN([SIM_AC_OPTION_FLOAT],
[
default_sim_float="[$1]"
default_sim_float_bitsize="[$2]"
AC_ARG_ENABLE(sim-float,
[AS_HELP_STRING([--enable-sim-float],
		[Specify that the target processor has floating point hardware])],
[case "${enableval}" in
  yes | hard)	sim_float="-DWITH_FLOATING_POINT=HARD_FLOATING_POINT";;
  no | soft)	sim_float="-DWITH_FLOATING_POINT=SOFT_FLOATING_POINT";;
  32)           sim_float="-DWITH_FLOATING_POINT=HARD_FLOATING_POINT -DWITH_TARGET_FLOATING_POINT_BITSIZE=32";;
  64)           sim_float="-DWITH_FLOATING_POINT=HARD_FLOATING_POINT -DWITH_TARGET_FLOATING_POINT_BITSIZE=64";;
  *)		AC_MSG_ERROR("Unknown value $enableval passed to --enable-sim-float"); sim_float="";;
esac
if test x"$silent" != x"yes" && test x"$sim_float" != x""; then
  echo "Setting float flags = $sim_float" 6>&1
fi],[
sim_float=
if test x"${default_sim_float}" != x""; then
  sim_float="-DWITH_FLOATING_POINT=${default_sim_float}"
fi
if test x"${default_sim_float_bitsize}" != x""; then
  sim_float="$sim_float -DWITH_TARGET_FLOATING_POINT_BITSIZE=${default_sim_float_bitsize}"
fi
])dnl
])
AC_SUBST(sim_float)
