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
dnl Specify the running environment.
dnl If the simulator invokes this in its configure.ac then without this option
dnl the default is the user environment and all are runtime selectable.
dnl If the simulator doesn't invoke this, only the user environment is
dnl supported.
dnl ??? Until there is demonstrable value in doing something more complicated,
dnl let's not.
AC_DEFUN([SIM_AC_OPTION_ENVIRONMENT],
[
AC_MSG_CHECKING([default sim environment setting])
sim_environment="ALL_ENVIRONMENT"
AC_ARG_ENABLE(sim-environment,
[AS_HELP_STRING([--enable-sim-environment=environment],
		[Specify mixed, user, virtual or operating environment])],
[case "${enableval}" in
  all | ALL)             sim_environment="ALL_ENVIRONMENT";;
  user | USER)           sim_environment="USER_ENVIRONMENT";;
  virtual | VIRTUAL)     sim_environment="VIRTUAL_ENVIRONMENT";;
  operating | OPERATING) sim_environment="OPERATING_ENVIRONMENT";;
  *)   AC_MSG_ERROR([Unknown value $enableval passed to --enable-sim-environment]);;
esac])dnl
AC_DEFINE_UNQUOTED([WITH_ENVIRONMENT], [$sim_environment], [Sim default environment])
AC_MSG_RESULT($sim_environment)
])
