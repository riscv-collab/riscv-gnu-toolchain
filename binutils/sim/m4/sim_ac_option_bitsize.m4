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
dnl --enable-sim-bitsize is for developers of the simulator
dnl It specifies the number of BITS in the target.
dnl arg[1] is the number of bits in a word
dnl arg[2] is the number assigned to the most significant bit
dnl arg[3] is the number of bits in an address
dnl arg[4] is the number of bits in an OpenFirmware cell.
dnl FIXME: this information should be obtained from bfd/archure
AC_DEFUN([SIM_AC_OPTION_BITSIZE],
wire_word_bitsize="[$1]"
wire_word_msb="[$2]"
wire_address_bitsize="[$3]"
wire_cell_bitsize="[$4]"
[AC_ARG_ENABLE(sim-bitsize,
[AS_HELP_STRING([--enable-sim-bitsize=N], [Specify target bitsize (32 or 64)])],
[sim_bitsize=
case "${enableval}" in
  64,63 | 64,63,* ) sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=64 -DWITH_TARGET_WORD_MSB=63";;
  32,31 | 32,31,* ) sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=31";;
  64,0 | 64,0,* ) sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=0";;
  32,0 | 64,0,* ) sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=0";;
  32) if test x"$wire_word_msb" != x -a x"$wire_word_msb" != x0; then
        sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=31"
      else
        sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=0"
      fi ;;
  64) if test x"$wire_word_msb" != x -a x"$wire_word_msb" != x0; then
        sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=64 -DWITH_TARGET_WORD_MSB=63"
      else
        sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=64 -DWITH_TARGET_WORD_MSB=0"
      fi ;;
  *)  AC_MSG_ERROR("--enable-sim-bitsize was given $enableval.  Expected 32 or 64") ;;
esac
# address bitsize
tmp=`echo "${enableval}" | sed -e "s/^[[0-9]]*,*[[0-9]]*,*//"`
case x"${tmp}" in
  x ) ;;
  x32 | x32,* ) sim_bitsize="${sim_bitsize} -DWITH_TARGET_ADDRESS_BITSIZE=32" ;;
  x64 | x64,* ) sim_bitsize="${sim_bitsize} -DWITH_TARGET_ADDRESS_BITSIZE=64" ;;
  * ) AC_MSG_ERROR("--enable-sim-bitsize was given address size $enableval.  Expected 32 or 64") ;;
esac
# cell bitsize
tmp=`echo "${enableval}" | sed -e "s/^[[0-9]]*,*[[0-9*]]*,*[[0-9]]*,*//"`
case x"${tmp}" in
  x ) ;;
  x32 | x32,* ) sim_bitsize="${sim_bitsize} -DWITH_TARGET_CELL_BITSIZE=32" ;;
  x64 | x64,* ) sim_bitsize="${sim_bitsize} -DWITH_TARGET_CELL_BITSIZE=64" ;;
  * ) AC_MSG_ERROR("--enable-sim-bitsize was given cell size $enableval.  Expected 32 or 64") ;;
esac
if test x"$silent" != x"yes" && test x"$sim_bitsize" != x""; then
  echo "Setting bitsize flags = $sim_bitsize" 6>&1
fi],
[sim_bitsize=""
if test x"$wire_word_bitsize" != x; then
  sim_bitsize="$sim_bitsize -DWITH_TARGET_WORD_BITSIZE=$wire_word_bitsize"
fi
if test x"$wire_word_msb" != x; then
  sim_bitsize="$sim_bitsize -DWITH_TARGET_WORD_MSB=$wire_word_msb"
fi
if test x"$wire_address_bitsize" != x; then
  sim_bitsize="$sim_bitsize -DWITH_TARGET_ADDRESS_BITSIZE=$wire_address_bitsize"
fi
if test x"$wire_cell_bitsize" != x; then
  sim_bitsize="$sim_bitsize -DWITH_TARGET_CELL_BITSIZE=$wire_cell_bitsize"
fi])dnl
])
AC_SUBST(sim_bitsize)
