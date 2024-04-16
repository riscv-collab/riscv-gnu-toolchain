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
dnl --enable-build-warnings is for developers of the simulator.
dnl it enables extra GCC specific warnings.
AC_DEFUN([SIM_AC_OPTION_WARNINGS], [dnl
AC_ARG_ENABLE(werror,
  AS_HELP_STRING([--enable-werror], [treat compile warnings as errors]),
  [case "${enableval}" in
     yes | y) ERROR_ON_WARNING="yes" ;;
     no | n)  ERROR_ON_WARNING="no" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --enable-werror) ;;
   esac])

dnl Enable -Werror by default when using gcc.  Turn it off for releases.
if test "${GCC}" = yes -a -z "${ERROR_ON_WARNING}" && $development; then
  ERROR_ON_WARNING=yes
fi

WERROR_CFLAGS=""
if test "${ERROR_ON_WARNING}" = yes ; then
  WERROR_CFLAGS="-Werror"
fi

dnl The options we'll try to enable.
dnl NB: Kept somewhat in sync with gdbsupport/warnings.m4.
build_warnings="-Wall -Wpointer-arith
-Wno-unused -Wunused-value -Wunused-variable -Wunused-function
-Wno-switch -Wno-char-subscripts
-Wempty-body -Wunused-but-set-parameter -Wunused-but-set-variable
-Wno-sign-compare -Wno-error=maybe-uninitialized
dnl C++ -Wno-mismatched-tags
-Wno-error=deprecated-register
dnl C++ -Wsuggest-override
-Wimplicit-fallthrough=5
-Wduplicated-cond
-Wshadow=local
dnl C++ -Wdeprecated-copy
dnl C++ -Wdeprecated-copy-dtor
dnl C++ -Wredundant-move
-Wmissing-declarations
dnl C++ -Wstrict-null-sentinel
"
dnl Some extra warnings we use in the sim.
build_warnings="$build_warnings
-Wdeclaration-after-statement
-Wdeprecated-non-prototype
-Wimplicit-function-declaration
-Wimplicit-int
-Wincompatible-function-pointer-types
-Wincompatible-pointer-types
-Wint-conversion
-Wmisleading-indentation
-Wmissing-parameter-type
-Wmissing-prototypes
-Wold-style-declaration
-Wold-style-definition
-Wpointer-sign
-Wreturn-mismatch
-Wreturn-type
-Wshift-negative-value
-Wstrict-prototypes
dnl The cgen virtual insn logic involves enum conversions.
dnl Disable until we can figure out how to make this work.
-Wno-enum-conversion
"
build_build_warnings="
dnl TODO Fix the sh/gencode.c which triggers a ton of these warnings.
-Wno-missing-braces
dnl TODO Figure out the igen code that triggers warnings w/FORTIFY_SOURCE.
-Wno-stringop-truncation
dnl Fixing this requires ATTRIBUTE_FALLTHROUGH support at build time, but we
dnl don't have gnulib there (yet).
-Wno-implicit-fallthrough
dnl TODO Enable this after cleaning up code.
-Wno-shadow=local
"

case "${host}" in
  *-*-mingw32*)
    # Enable -Wno-format by default when using gcc on mingw since many
    # GCC versions complain about %I64.
    build_warnings="$build_warnings -Wno-format" ;;
  *-*-solaris*)
    # Solaris 11.4 <python2.7/ceval.h> uses #pragma no_inline that GCC
    # doesn't understand.
    build_warnings="$build_warnings -Wno-unknown-pragmas"
    # Solaris 11 <unistd.h> marks vfork deprecated.
    build_warnings="$build_warnings -Wno-deprecated-declarations" ;;
  *)
    # Note that gcc requires -Wformat for -Wformat-nonliteral to work,
    # but there's a special case for this below.
    build_warnings="$build_warnings -Wformat-nonliteral" ;;
esac

AC_ARG_ENABLE(build-warnings,
AS_HELP_STRING([--enable-build-warnings], [enable build-time compiler warnings if gcc is used]),
[case "${enableval}" in
  yes)	;;
  no)	build_warnings="-w";;
  ,*)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        build_warnings="${build_warnings} ${t}";;
  *,)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        build_warnings="${t} ${build_warnings}";;
  *)    build_warnings=`echo "${enableval}" | sed -e "s/,/ /g"`;;
esac
if test x"$silent" != x"yes" && test x"$build_warnings" != x""; then
  echo "Setting compiler warning flags = $build_warnings" 6>&1
fi])dnl
AC_ARG_ENABLE(sim-build-warnings,
AS_HELP_STRING([--enable-sim-build-warnings], [enable SIM specific build-time compiler warnings if gcc is used]),
[case "${enableval}" in
  yes)	;;
  no)	build_warnings="-w";;
  ,*)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        build_warnings="${build_warnings} ${t}";;
  *,)   t=`echo "${enableval}" | sed -e "s/,/ /g"`
        build_warnings="${t} ${build_warnings}";;
  *)    build_warnings=`echo "${enableval}" | sed -e "s/,/ /g"`;;
esac
if test x"$silent" != x"yes" && test x"$build_warnings" != x""; then
  echo "Setting GDB specific compiler warning flags = $build_warnings" 6>&1
fi])dnl
WARN_CFLAGS=""
BUILD_WARN_CFLAGS=""
if test "x${build_warnings}" != x -a "x$GCC" = xyes
then
AC_DEFUN([_SIM_TEST_ALL_WARNING_FLAGS], [dnl
    AC_MSG_CHECKING(compiler warning flags)
    # Separate out the -Werror flag as some files just cannot be
    # compiled with it enabled.
    for w in ${build_warnings}; do
	case $w in
	-Werr*) WERROR_CFLAGS=-Werror ;;
	*) _SIM_TEST_WARNING_FLAG($w, [WARN_CFLAGS="${WARN_CFLAGS} $w"]) ;;
	esac
    done
    AC_MSG_RESULT(${WARN_CFLAGS} ${WERROR_CFLAGS})
])

    dnl Test the host flags.
    _SIM_TEST_ALL_WARNING_FLAGS

    dnl Test the build flags.
    AS_IF([test "x$cross_compiling" = "xno"], [dnl
	SAVE_WARN_CFLAGS=$WARN_CFLAGS
	build_warnings=$build_build_warnings
	_SIM_TEST_ALL_WARNING_FLAGS
	BUILD_WARN_CFLAGS=$WARN_CFLAGS
	WARN_CFLAGS=$SAVE_WARN_CFLAGS
	BUILD_WERROR_CFLAGS=$WERROR_CFLAGS
    ])

    dnl Test individual flags to export to dedicated variables.
    m4_map([_SIM_EXPORT_WARNING_FLAG], m4_split(m4_normalize([
	-Wno-shadow=local
	-Wno-unused-but-set-variable
    ])))dnl
fi
])
dnl Test a warning flag $1 and execute $2 if it passes, else $3.
AC_DEFUN([_SIM_TEST_WARNING_FLAG], [dnl
  dnl GCC does not complain about -Wno-unknown-warning.  Invert
  dnl and test -Wunknown-warning instead.
  w="$1"
  case $w in
  -Wno-*)
    wtest=`echo $w | sed 's/-Wno-/-W/g'` ;;
  -Wformat-nonliteral)
    dnl gcc requires -Wformat before -Wformat-nonliteral
    dnl will work, so stick them together.
    w="-Wformat $w"
    wtest="$w"
    ;;
  *)
    wtest=$w ;;
  esac

  dnl Check whether GCC accepts it.
  saved_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -Werror $wtest"
  AC_TRY_COMPILE([],[],$2,$3)
  CFLAGS="$saved_CFLAGS"
])
dnl Export variable $1 to $2 for use in makefiles.
AC_DEFUN([_SIM_EXPORT_WARNING], [dnl
  AS_VAR_SET($1, $2)
  AC_SUBST($1)
])
dnl Test if $1 is a known warning flag, and export a variable for makefiles.
dnl If $1=-Wfoo, then SIM_CFLAG_WFOO will be set to -Wfoo if it's supported.
AC_DEFUN([_SIM_EXPORT_WARNING_FLAG], [dnl
  AC_MSG_CHECKING([whether $1 is supported])
  _SIM_TEST_WARNING_FLAG($1, [dnl
    _SIM_EXPORT_WARNING([SIM_CFLAG]m4_toupper(m4_translit($1, [-= ], [__])), $1)
    AC_MSG_RESULT(yes)
  ], [AC_MSG_RESULT(no)])
])
AC_SUBST(WARN_CFLAGS)
AC_SUBST(WERROR_CFLAGS)
AC_SUBST(BUILD_WARN_CFLAGS)
AC_SUBST(BUILD_WERROR_CFLAGS)
