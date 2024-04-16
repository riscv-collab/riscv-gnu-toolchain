dnl Autoconf configure script for GDB, the GNU debugger.
dnl Copyright (C) 1995-2024 Free Software Foundation, Inc.
dnl
dnl This file is part of GDB.
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

AC_INIT([libgnu], [UNUSED-VERSION])
AC_CONFIG_SRCDIR([import/memmem.c])
AC_CONFIG_HEADER(config.h:config.in)
AC_CONFIG_MACRO_DIRS([import/m4])
AC_CONFIG_MACRO_DIRS([../config])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS
# Needs to run before gl_EARLY so it can override AC_SYS_LARGEFILE included
# there.
ACX_LARGEFILE
gl_EARLY
AM_PROG_CC_STDC

AC_CONFIG_AUX_DIR(..)
AC_CANONICAL_SYSTEM

gl_INIT

AM_INIT_AUTOMAKE([no-define no-dist foreign])

AM_SILENT_RULES([yes])

# --------------------- #
# Checks for programs.  #
# --------------------- #

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB

AC_CHECK_TOOL(AR, ar)

# ---------------------- #
# Checks for libraries.  #
# ---------------------- #

AC_CONFIG_FILES(Makefile.gnulib.inc)

AC_OUTPUT(Makefile import/Makefile,
[
case x$CONFIG_HEADERS in
xconfig.h:config.in)
echo > stamp-h ;;
esac
])

exit 0
