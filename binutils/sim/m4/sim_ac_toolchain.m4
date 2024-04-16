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
dnl Setup the toolchain variables.
AC_DEFUN([SIM_AC_TOOLCHAIN],
[dnl
AC_CANONICAL_SYSTEM
AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([AC_PROG_CPP])
AC_C_BIGENDIAN
AC_ARG_PROGRAM
AC_PROG_INSTALL

AC_CHECK_TOOL(DTC, dtc)

dnl Setup toolchain settings for build-time tools..
if test "x$cross_compiling" = "xno"; then
  : "${AR_FOR_BUILD:=\$(AR)}"
  : "${CC_FOR_BUILD:=\$(CC)}"
  : "${RANLIB_FOR_BUILD:=\$(RANLIB)}"
  : "${CFLAGS_FOR_BUILD:=\$(CFLAGS)}"
  : "${CPPFLAGS_FOR_BUILD:=\$(CPPFLAGS)}"
  : "${LDFLAGS_FOR_BUILD:=\$(LDFLAGS)}"
else
  : "${AR_FOR_BUILD:=ar}"
  : "${CC_FOR_BUILD:=gcc}"
  : "${RANLIB_FOR_BUILD:=ranlib}"
  : "${CFLAGS_FOR_BUILD:=-g -O}"
  : "${CPPFLAGS_FOR_BUILD:=}"
  : "${LDLFAGS_FOR_BUILD:=}"
fi
AC_SUBST(AR_FOR_BUILD)
AC_SUBST(CC_FOR_BUILD)
AC_SUBST(RANLIB_FOR_BUILD)
AC_SUBST(CFLAGS_FOR_BUILD)
AC_SUBST(CPPFLAGS_FOR_BUILD)
AC_SUBST(LDFLAGS_FOR_BUILD)

AC_SUBST(CFLAGS)
AC_CHECK_TOOL(AR, ar)
AC_PROG_RANLIB
PKG_PROG_PKG_CONFIG

# Require C11 or newer.  Autoconf-2.70 provides ac_cv_prog_cc_c11 when using
# AC_PROG_CC, but we're still using Autoconf-2.69, and the newest it understands
# is C99.  So handle it ourselves.
m4_version_prereq([2.70], [AC_MSG_ERROR([clean this up!])], [:])
C_DIALECT=
AC_MSG_CHECKING([whether C11 is supported by default])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#if !defined __STDC_VERSION__ || __STDC_VERSION__ < 201112L
# error "C11 support not found"
#endif
])], [AC_MSG_RESULT([yes])], [
  AC_MSG_RESULT([no])
  AC_MSG_CHECKING([for -std=c11 support])
  ac_save_CC="$CC"
  CC="$CC -std=c11"
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#if !defined __STDC_VERSION__ || __STDC_VERSION__ < 201112L
# error "C11 support not found"
#endif
])], [
  AC_MSG_RESULT([yes])
  CC="$ac_save_CC"
  C_DIALECT="-std=c11"
], [AC_MSG_ERROR([C11 is required])])])
AC_SUBST(C_DIALECT)
])
dnl

AC_DEFUN([SIM_AC_CHECK_TOOLCHAIN_FOR_PRIMARY_TARGET],
[dnl
GCC_TARGET_TOOL([cc], [CC_FOR_TARGET], [CC], [${target_alias}-gcc])
GCC_TARGET_TOOL([as], [AS_FOR_TARGET], [AS], [\$(abs_builddir)/../gas/as-new])
GCC_TARGET_TOOL([ld], [LD_FOR_TARGET], [LD], [\$(abs_builddir)/../ld/ld-new])
])

SIM_TOOLCHAIN_VARS=
AC_SUBST(SIM_TOOLCHAIN_VARS)
AC_DEFUN([_SIM_AC_TOOLCHAIN_FOR_TARGET],
[dnl
AC_REQUIRE([SIM_AC_CHECK_TOOLCHAIN_FOR_PRIMARY_TARGET])
AC_ARG_VAR(AS_FOR_TARGET_$2, [Assembler for $1 tests])
AC_ARG_VAR(LD_FOR_TARGET_$2, [Linker for $1 tests])
AC_ARG_VAR(CC_FOR_TARGET_$2, [C compiler for $1 tests])
m4_bmatch($1, [example-], [dnl
  : "${AS_FOR_TARGET_$2:=\$(AS_FOR_TARGET)}"
  : "${LD_FOR_TARGET_$2:=\$(LD_FOR_TARGET)}"
  : "${CC_FOR_TARGET_$2:=\$(CC)}"
], [dnl
  AS_IF([test "$SIM_PRIMARY_TARGET" = "$1"], [dnl
    : "${AS_FOR_TARGET_$2:=\$(AS_FOR_TARGET)}"
    : "${LD_FOR_TARGET_$2:=\$(LD_FOR_TARGET)}"
    : "${CC_FOR_TARGET_$2:=\$(CC_FOR_TARGET)}"
  ])
])
AS_VAR_APPEND([SIM_TOOLCHAIN_VARS], [" AS_FOR_TARGET_$2 LD_FOR_TARGET_$2 CC_FOR_TARGET_$2"])
])
AC_DEFUN([SIM_AC_TOOLCHAIN_FOR_TARGET],
[_SIM_AC_TOOLCHAIN_FOR_TARGET($1, m4_toupper(m4_translit($1, [-], [_])))])
