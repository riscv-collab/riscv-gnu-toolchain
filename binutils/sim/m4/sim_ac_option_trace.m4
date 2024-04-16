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
dnl --enable-sim-trace is for users of the simulator
dnl The argument is either a bitmask of things to enable [exactly what is
dnl up to the simulator], or is a comma separated list of names of tracing
dnl elements to enable.  The latter is only supported on simulators that
dnl use WITH_TRACE.  Default to all tracing but internal debug.
AC_DEFUN([SIM_AC_OPTION_TRACE],
[dnl
AC_MSG_CHECKING([for sim trace settings])
sim_trace="~TRACE_debug"
AC_ARG_ENABLE(sim-trace,
[AS_HELP_STRING([--enable-sim-trace=opts],
		[Enable tracing of simulated programs])],
[case "${enableval}" in
  yes)	sim_trace="-1";;
  no)	sim_trace="0";;
  [[-0-9]]*)
	sim_trace="'(${enableval})'";;
  [[[:lower:]]]*)
	sim_trace=""
	for x in `echo "$enableval" | sed -e "s/,/ /g"`; do
	  if test x"$sim_trace" = x; then
	    sim_trace="(TRACE_$x"
	  else
	    sim_trace="${sim_trace}|TRACE_$x"
	  fi
	done
	sim_trace="$sim_trace)" ;;
esac])dnl
AC_DEFINE_UNQUOTED([WITH_TRACE], [$sim_trace], [Sim trace settings])
AC_MSG_RESULT($sim_trace)
])
