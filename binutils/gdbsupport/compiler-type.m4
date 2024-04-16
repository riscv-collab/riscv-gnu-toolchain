dnl Autoconf configure script for GDB, the GNU debugger.
dnl Copyright (C) 2022-2024 Free Software Foundation, Inc.
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

# Sets up GDB_COMPILER_TYPE to either 'gcc', 'clang', or 'unknown'.
# The autoconf compiler check will set GCC=yes for clang as well as
# gcc, it's really more of a "is gcc like" check.
#
# By contrast, this will set the GDB_COMPILER_TYPE to 'gcc' only for
# versions of gcc.
#
# There's no reason why this can't be extended to identify other
# compiler types if needed in the future, users of this variable
# should therefore avoid relying on the 'unknown' value, instead
# checks should be written in terms of the known compiler types.
AC_DEFUN([AM_GDB_COMPILER_TYPE],[

  AC_CACHE_CHECK([the compiler type],
                 [gdb_cv_compiler_type],
 [gdb_cv_compiler_type=unknown
  if test "$gdb_cv_compiler_type" = unknown; then
     AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([],
                         [
                          #if !defined __GNUC__ || defined __clang__
                          #error not gcc
                          #endif
                         ])],
        [gdb_cv_compiler_type=gcc], [])
  fi

  if test "$gdb_cv_compiler_type" = unknown; then
     AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([],
                         [
                          #ifndef __clang__
                          #error not clang
                          #endif
                         ])],
        [gdb_cv_compiler_type=clang], [])
  fi
 ])

 GDB_COMPILER_TYPE="$gdb_cv_compiler_type"
])
