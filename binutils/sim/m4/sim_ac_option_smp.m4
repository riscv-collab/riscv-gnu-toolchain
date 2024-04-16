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
AC_DEFUN([SIM_AC_OPTION_SMP], [dnl
AC_MSG_CHECKING([number of sim cpus to support])
dnl TODO: We should increase the default to use smp at some point.  When we do,
dnl the ppc/configure sim-smp option should be merged.  See the WITH_SMP check
dnl below for more cleanups too.
default_sim_smp="0"
sim_smp="$default_sim_smp"
AC_ARG_ENABLE(sim-smp,
[AS_HELP_STRING([--enable-sim-smp=n],
		[Specify number of processors to configure for (default 1)])],
[case "${enableval}" in
  yes)	sim_smp="5";;
  no)	sim_smp="0";;
  *)	sim_smp="$enableval";;
esac])dnl
IGEN_FLAGS_SMP="-N ${sim_smp}"
AC_SUBST(IGEN_FLAGS_SMP)
dnl NB: The ppc code uses a diff default because its smp works.  That is why
dnl we don't unconditionally enable WITH_SMP here.  Once we unify ppc, we can
dnl make this unconditional.
AS_VAR_IF([sim_smp], [0], [], [dnl
  AC_DEFINE_UNQUOTED([WITH_SMP], [$sim_smp], [Sim SMP settings])])
AC_MSG_RESULT($sim_smp)
])
