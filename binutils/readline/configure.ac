dnl Autoconf configure script for the readline wrapper directory
dnl Copyright (C) 2019 Free Software Foundation, Inc.
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

AC_INIT([readline], [UNUSED-VERSION])
AC_CONFIG_SRCDIR([readline/readline.c])
AC_CONFIG_AUX_DIR(..)
AC_CONFIG_MACRO_DIRS([../config])
AM_INIT_AUTOMAKE([no-define no-dist foreign])
AM_MAINTAINER_MODE

AC_CONFIG_SUBDIRS([readline])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
