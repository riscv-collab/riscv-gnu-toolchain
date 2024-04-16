#                                                       -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# Copyright 2002-2024 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_INIT
AC_CONFIG_SRCDIR([gdb.base])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

ACX_NONCANONICAL_TARGET

# Enable gdbtk.
AC_ARG_ENABLE(gdbtk,
[  --enable-gtk            enable gdbtk graphical user interface (GUI)],,
  [if test -d $srcdir/../gdbtk && test -d $srcdir/gdb.gdbtk; then
    enable_gdbtk=yes
  else
    enable_gdbtk=no
  fi])
# We unconditionally disable gdbtk tests on selected platforms.
case $host_os in
  go32* | windows*)
    enable_gdbtk=no ;;
esac

# Add gdbtk tests when appropriate.
if test $enable_gdbtk = yes; then
   AC_CONFIG_SUBDIRS(gdb.gdbtk)
fi

GCC_ENABLE([libctf], [yes], [], [Handle .ctf type-info sections])
AC_SUBST(enable_libctf)

# Enable shared libraries.
AC_ARG_ENABLE(shared,
[  --enable-shared         build shared libraries [deault=yes]],,
  enable_shared=yes)

# If we have shared libraries, try to set RPATH_ENVVAR reasonably,
# such that we can find the shared libraries in the build tree.
if test $enable_shared = no; then
  # The variable `RPATH_ENVVAR' itself is not likely to be used on any
  # platform.
  RPATH_ENVVAR=RPATH_ENVVAR
else
  # The variable `LD_LIBRARY_PATH' is used on most platforms.
  RPATH_ENVVAR=LD_LIBRARY_PATH
  # The following exceptions are taken from Libtool 1.4.3.
  case $host_os in
    aix*)
      if test $host_cpu != ia64; then
        RPATH_ENVVAR=LIBPATH
      fi ;;
    darwin* | rhapsody*)
      RPATH_ENVVAR=DYLD_LIBRARY_PATH ;;
  esac
fi
AC_SUBST(RPATH_ENVVAR)

AC_CHECK_HEADERS(pthread.h)

AC_EXEEXT

if test "${build}" = "${host}" -a "${host}" = "${target}"; then
   case "${host}" in
     *gnu*)
       EXTRA_RULES=read1
       ;;
   esac
fi
AC_SUBST(EXTRA_RULES)

# Transform the name of some programs and generate the lib/pdtrace
# test tool.
AC_ARG_PROGRAM
GDB_AC_TRANSFORM(strip, STRIP_TRANSFORM_NAME)
GDB_AC_TRANSFORM(readelf, READELF_TRANSFORM_NAME)
GDB_AC_TRANSFORM(as, GAS_TRANSFORM_NAME)
GDB_AC_TRANSFORM(nm, NM_TRANSFORM_NAME)
AC_CONFIG_FILES([lib/pdtrace], [chmod +x lib/pdtrace])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
