dnl Bits libiberty clients must do on their autoconf step.
dnl
dnl Copyright (C) 2012-2024 Free Software Foundation, Inc.
dnl
dnl This file is free software; you can redistribute it and/or modify
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
dnl along with this program; see the file COPYING3.  If not see
dnl <http://www.gnu.org/licenses/>.
dnl
dnl
dnl Checks for declarations ansidecl.h and libiberty.h themselves
dnl check with HAVE_DECL_XXX, etc.
AC_DEFUN([libiberty_INIT],
[dnl
  dnl Check for presence and size of long long.
  AC_CHECK_TYPES([long long], [AC_CHECK_SIZEOF(long long)])

  AC_CHECK_DECLS([basename(char *)])
  AC_CHECK_DECLS_ONCE([ffs, asprintf, vasprintf, snprintf, vsnprintf])
  AC_CHECK_DECLS_ONCE([strtol, strtoul, strtoll, strtoull])
  AC_CHECK_DECLS_ONCE([strverscmp])
])
