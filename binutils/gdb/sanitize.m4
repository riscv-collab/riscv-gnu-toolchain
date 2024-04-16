dnl Sanitization-related configure macro for GDB
dnl Copyright (C) 2018-2024 Free Software Foundation, Inc.
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

AC_DEFUN([AM_GDB_UBSAN],[
AC_ARG_ENABLE(ubsan,
  AS_HELP_STRING([--enable-ubsan],
                 [enable undefined behavior sanitizer (auto/yes/no)]),
  [],enable_ubsan=no)
if test "x$enable_ubsan" = xauto; then
  if $development; then
    enable_ubsan=yes
  fi
fi
AC_LANG_PUSH([C++])
if test "x$enable_ubsan" = xyes; then
  AC_MSG_CHECKING(whether -fsanitize=undefined is accepted)
  saved_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -fsanitize=undefined -fno-sanitize-recover=undefined"
  dnl A link check is required because it is possible to install gcc
  dnl without libubsan, leading to link failures when compiling with
  dnl -fsanitize=undefined.
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([], [])],
    [enable_ubsan=yes],
    [enable_ubsan=no]
  )
  CXXFLAGS="$saved_CXXFLAGS"
  AC_MSG_RESULT($enable_ubsan)
  if test "x$enable_ubsan" = xyes; then
    WARN_CFLAGS="$WARN_CFLAGS -fsanitize=undefined -fno-sanitize-recover=undefined"
    CONFIG_LDFLAGS="$CONFIG_LDFLAGS -fsanitize=undefined"
  fi
fi
AC_LANG_POP([C++])
])
