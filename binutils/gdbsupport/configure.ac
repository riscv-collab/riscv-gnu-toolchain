dnl Autoconf configure script for GDB support library
dnl Copyright (C) 2020-2024 Free Software Foundation, Inc.
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

dnl Process this file with autoconf to produce a configure script.

AC_INIT([gdbsupport], 1.0)
AC_CONFIG_SRCDIR(common-defs.h)
AC_CONFIG_HEADER(config.h:config.in)

# Set build, build_cpu, build_vendor and build_os.
AC_CANONICAL_BUILD

# Set host, host_cpu, host_vendor, and host_os.
AC_CANONICAL_HOST

# Set target, target_cpu, target_vendor, and target_os.
AC_CANONICAL_TARGET

AM_MAINTAINER_MODE
AC_CONFIG_AUX_DIR(..)
AM_INIT_AUTOMAKE
AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

AC_USE_SYSTEM_EXTENSIONS
ACX_LARGEFILE

# We require a C++17 compiler.  Check if one is available, and if
# necessary, set CXX_DIALECT to some -std=xxx switch.
AX_CXX_COMPILE_STDCXX(17, , mandatory)

dnl Set up for gettext.
ZW_GNU_GETTEXT_SISTER_DIR

libiberty_INIT
GDB_AC_COMMON
GDB_AC_SELFTEST
AM_CONDITIONAL(SELFTEST, $enable_unittests)

AM_CONDITIONAL(HAVE_PIPE_OR_PIPE2,
   [test x$ac_cv_func_pipe = xyes -o x$ac_cv_func_pipe2 = xyes ])

# Check the return and argument types of ptrace.
GDB_AC_PTRACE

# Detect support warning flags.
AM_GDB_COMPILER_TYPE
AM_GDB_WARNINGS

# Set the 'development' global.
. $srcdir/../bfd/development.sh

if test "$development" = true ; then
   AC_DEFINE(DEVELOPMENT, 1,
	     [Define if development-mode features are enabled.])
fi

case ${host} in
  *mingw32*)
    AC_DEFINE(USE_WIN32API, 1,
              [Define if we should use the Windows API, instead of the
	       POSIX API.  On Windows, we use the Windows API when
	       building for MinGW, but the POSIX API when building
	       for Cygwin.])
    ;;
esac

AC_SUBST([CONFIG_STATUS_DEPENDENCIES], ['$(srcdir)/../bfd/development.sh'])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
