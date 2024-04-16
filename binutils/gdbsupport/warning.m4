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

AC_DEFUN([AM_GDB_WARNINGS],[
AC_ARG_ENABLE(werror,
  AS_HELP_STRING([--enable-werror], [treat compile warnings as errors]),
  [case "${enableval}" in
     yes | y) ERROR_ON_WARNING="yes" ;;
     no | n)  ERROR_ON_WARNING="no" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --enable-werror) ;;
   esac])

# Enable -Werror by default when using gcc.  Turn it off for releases.
if test "${GCC}" = yes -a -z "${ERROR_ON_WARNING}" && $development; then
    ERROR_ON_WARNING=yes
fi

WERROR_CFLAGS=""
if test "${ERROR_ON_WARNING}" = yes ; then
    WERROR_CFLAGS="-Werror"
fi

# The options we'll try to enable.
build_warnings="-Wall -Wpointer-arith \
-Wno-unused -Wunused-value -Wunused-variable -Wunused-function \
-Wno-switch -Wno-char-subscripts \
-Wempty-body -Wunused-but-set-parameter -Wunused-but-set-variable \
-Wno-sign-compare -Wno-error=maybe-uninitialized \
-Wno-mismatched-tags \
-Wno-error=deprecated-register \
-Wsuggest-override \
-Wimplicit-fallthrough=5 \
-Wduplicated-cond \
-Wshadow=local \
-Wdeprecated-copy \
-Wdeprecated-copy-dtor \
-Wredundant-move \
-Wmissing-declarations \
-Wstrict-null-sentinel \
"

# The -Wmissing-prototypes flag will be accepted by GCC, but results
# in a warning being printed about the flag not being valid for C++,
# this is something to do with using ccache, and argument ordering.
if test "$GDB_COMPILER_TYPE" != gcc; then
  build_warnings="$build_warnings -Wmissing-prototypes"
fi

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
AC_ARG_ENABLE(gdb-build-warnings,
AS_HELP_STRING([--enable-gdb-build-warnings], [enable GDB specific build-time compiler warnings if gcc is used]),
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

# The set of warnings supported by a C++ compiler is not the same as
# of the C compiler.
AC_LANG_PUSH([C++])

WARN_CFLAGS=""
if test "x${build_warnings}" != x -a "x$GCC" = xyes
then
    AC_MSG_CHECKING(compiler warning flags)
    # Separate out the -Werror flag as some files just cannot be
    # compiled with it enabled.
    for w in ${build_warnings}; do
	# GCC does not complain about -Wno-unknown-warning.  Invert
	# and test -Wunknown-warning instead.
	case $w in
	-Wno-*)
		wtest=`echo $w | sed 's/-Wno-/-W/g'` ;;
        -Wformat-nonliteral)
		# gcc requires -Wformat before -Wformat-nonliteral
		# will work, so stick them together.
		w="-Wformat $w"
		wtest="$w"
		;;
	*)
		wtest=$w ;;
	esac

	case $w in
	-Werr*) WERROR_CFLAGS=-Werror ;;
	*)
	    # Check whether GCC accepts it.
	    saved_CFLAGS="$CFLAGS"
	    CFLAGS="$CFLAGS -Werror $wtest"
	    saved_CXXFLAGS="$CXXFLAGS"
	    CXXFLAGS="$CXXFLAGS -Werror $wtest"
	    if test "x$w" = "x-Wunused-variable"; then
	      # Check for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=38958,
	      # fixed in GCC 4.9.  This test is derived from the gdb
	      # source code that triggered this bug in GCC.
	      AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM(
		   [struct scoped_restore_base {};
		    struct scoped_restore_tmpl : public scoped_restore_base {
		      ~scoped_restore_tmpl() {}
		    };],
		   [const scoped_restore_base &b = scoped_restore_tmpl();]
		 )],
		[WARN_CFLAGS="${WARN_CFLAGS} $w"],
		[]
	      )
	    else
	      AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([], [])],
		[WARN_CFLAGS="${WARN_CFLAGS} $w"],
		[]
	      )
	    fi
	    CFLAGS="$saved_CFLAGS"
	    CXXFLAGS="$saved_CXXFLAGS"
	esac
    done
    AC_MSG_RESULT(${WARN_CFLAGS} ${WERROR_CFLAGS})
fi
AC_SUBST(WARN_CFLAGS)
AC_SUBST(WERROR_CFLAGS)

AC_LANG_POP([C++])
])
