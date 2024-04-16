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
dnl --enable-sim-profile
dnl The argument is either a bitmask of things to enable [exactly what is
dnl up to the simulator], or is a comma separated list of names of profiling
dnl elements to enable.  The latter is only supported on simulators that
dnl use WITH_PROFILE.
AC_DEFUN([SIM_AC_OPTION_PROFILE],
[dnl
AC_MSG_CHECKING([for sim profile settings])
profile="1"
sim_profile="-1"
AC_ARG_ENABLE(sim-profile,
[AS_HELP_STRING([--enable-sim-profile=opts], [Enable profiling flags])],
[case "${enableval}" in
  yes)	profile="1" sim_profile="-1";;
  no)	profile="0" sim_profile="0";;
  [[-0-9]]*)
	profile="(${enableval})" sim_profile="(${enableval})";;
  [[a-z]]*)
    profile="1"
	sim_profile=""
	for x in `echo "$enableval" | sed -e "s/,/ /g"`; do
	  if test x"$sim_profile" = x; then
	    sim_profile="(PROFILE_$x"
	  else
	    sim_profile="${sim_profile}|PROFILE_$x"
	  fi
	done
	sim_profile="$sim_profile)" ;;
esac])dnl
AC_DEFINE_UNQUOTED([PROFILE], [$profile], [Sim profile settings])
AC_DEFINE_UNQUOTED([WITH_PROFILE], [$sim_profile], [Sim profile settings])
AC_MSG_RESULT($sim_profile)
])
