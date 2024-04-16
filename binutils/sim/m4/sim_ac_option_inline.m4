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
dnl --enable-sim-inline is for users that wish to ramp up the simulator's
dnl performance by inlining functions.
dnl Default sims to no inlining.
AC_DEFUN([SIM_AC_OPTION_INLINE],
[dnl
AC_MSG_CHECKING([sim inline settings])
sim_inline="-DDEFAULT_INLINE=m4_ifblank([$1],[0],[$1])"
AC_ARG_ENABLE(sim-inline,
[AS_HELP_STRING([--enable-sim-inline=inlines],
		[Specify which functions should be inlined])],
[sim_inline=""
case "$enableval" in
  no)		sim_inline="-DDEFAULT_INLINE=0";;
  0)		sim_inline="-DDEFAULT_INLINE=0";;
  yes | 2)	sim_inline="-DDEFAULT_INLINE=ALL_C_INLINE";;
  1)		sim_inline="-DDEFAULT_INLINE=INLINE_LOCALS";;
  *) for x in `echo "$enableval" | sed -e "s/,/ /g"`; do
       new_flag=""
       case "$x" in
	 *_INLINE=*)	new_flag="-D$x";;
	 *=*)		new_flag=`echo "$x" | sed -e "s/=/_INLINE=/" -e "s/^/-D/"`;;
	 *_INLINE)	new_flag="-D$x=ALL_C_INLINE";;
	 *)		new_flag="-D$x""_INLINE=ALL_C_INLINE";;
       esac
       if test x"$sim_inline" = x""; then
	 sim_inline="$new_flag"
       else
	 sim_inline="$sim_inline $new_flag"
       fi
     done;;
esac
])dnl
AC_MSG_RESULT([$sim_inline])
SIM_INLINE=$sim_inline
AC_SUBST(SIM_INLINE)
])
