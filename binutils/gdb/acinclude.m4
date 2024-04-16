dnl written by Rob Savoye <rob@cygnus.com> for Cygnus Support
dnl major rewriting for Tcl 7.5 by Don Libes <libes@nist.gov>

# Keep these includes in sync with the aclocal_m4_deps list in
# Makefile.in.

dnl NB: When possible, try to avoid explicit includes of ../config/ files.
dnl They're normally found by aclocal automatically and recorded in aclocal.m4.
dnl However, some are kept here explicitly to silence harmless warnings from
dnl aclocal when it finds AM_xxx macros via local search paths instead of
dnl system search paths.

m4_include(acx_configure_dir.m4)

# This gets GDB_AC_TRANSFORM.
m4_include(transform.m4)

# This get AM_GDB_COMPILER_TYPE.
m4_include(../gdbsupport/compiler-type.m4)

# This gets AM_GDB_WARNINGS.
m4_include(../gdbsupport/warning.m4)

# AM_GDB_UBSAN
m4_include(sanitize.m4)

# This gets GDB_AC_SELFTEST.
m4_include(../gdbsupport/selftest.m4)

dnl gdb/configure.in uses BFD_NEED_DECLARATION, so get its definition.
m4_include(../bfd/bfd.m4)

dnl For AM_LC_MESSAGES
m4_include([../config/lcmessage.m4])

dnl For AM_LANGINFO_CODESET.
m4_include([../config/codeset.m4])

dnl We need to explicitly include these before iconv.m4 to avoid warnings.
m4_include([../config/lib-ld.m4])
m4_include([../config/lib-prefix.m4])
m4_include([../config/lib-link.m4])
m4_include([../config/iconv.m4])

m4_include([../config/zlib.m4])
m4_include([../config/zstd.m4])

m4_include([../gdbsupport/common.m4])

dnl For libiberty_INIT.
m4_include(../gdbsupport/libiberty.m4)

dnl For GDB_AC_PTRACE.
m4_include(../gdbsupport/ptrace.m4)

m4_include(ax_cxx_compile_stdcxx.m4)

dnl written by Guido Draheim <guidod@gmx.de>, original by Alexandre Oliva 
dnl Version 1.3 (2001/03/02)
dnl source http://www.gnu.org/software/ac-archive/Miscellaneous/ac_define_dir.html

AC_DEFUN([AC_DEFINE_DIR], [
  test "x$prefix" = xNONE && prefix="$ac_default_prefix"
  test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
  ac_define_dir=`eval echo [$]$2`
  ac_define_dir=`eval echo [$]ac_define_dir`
  ifelse($3, ,
    AC_DEFINE_UNQUOTED($1, "$ac_define_dir"),
    AC_DEFINE_UNQUOTED($1, "$ac_define_dir", $3))
])

dnl See whether we need a declaration for a function.
dnl The result is highly dependent on the INCLUDES passed in, so make sure
dnl to use a different cache variable name in this macro if it is invoked
dnl in a different context somewhere else.
dnl gcc_AC_CHECK_DECL(SYMBOL,
dnl 	[ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, INCLUDES]]])
AC_DEFUN(
  [gcc_AC_CHECK_DECL],
  [AC_MSG_CHECKING([whether $1 is declared])
   AC_CACHE_VAL(
     [gcc_cv_have_decl_$1],
     [AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
	   [$4],
	   [#ifndef $1
	    char *(*pfn) = (char *(*)) $1 ;
	    #endif]
	 )],
	[eval "gcc_cv_have_decl_$1=yes"],
	[eval "gcc_cv_have_decl_$1=no"]
      )]
   )
if eval "test \"`echo '$gcc_cv_have_decl_'$1`\" = yes"; then
  AC_MSG_RESULT(yes) ; ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no) ; ifelse([$3], , :, [$3])
fi
])dnl

dnl Check multiple functions to see whether each needs a declaration.
dnl Arrange to define HAVE_DECL_<FUNCTION> to 0 or 1 as appropriate.
dnl gcc_AC_CHECK_DECLS(SYMBOLS,
dnl 	[ACTION-IF-NEEDED [, ACTION-IF-NOT-NEEDED [, INCLUDES]]])
AC_DEFUN([gcc_AC_CHECK_DECLS],
[for ac_func in $1
do
changequote(, )dnl
  ac_tr_decl=HAVE_DECL_`echo $ac_func | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
changequote([, ])dnl
gcc_AC_CHECK_DECL($ac_func,
  [AC_DEFINE_UNQUOTED($ac_tr_decl, 1) $2],
  [AC_DEFINE_UNQUOTED($ac_tr_decl, 0) $3],
dnl It is possible that the include files passed in here are local headers
dnl which supply a backup declaration for the relevant prototype based on
dnl the definition of (or lack of) the HAVE_DECL_ macro.  If so, this test
dnl will always return success.  E.g. see libiberty.h's handling of
dnl `basename'.  To avoid this, we define the relevant HAVE_DECL_ macro to
dnl 1 so that any local headers used do not provide their own prototype
dnl during this test.
#undef $ac_tr_decl
#define $ac_tr_decl 1
  $4
)
done
dnl Automatically generate config.h entries via autoheader.
if test x = y ; then
  patsubst(translit([$1], [a-z], [A-Z]), [\w+],
    [AC_DEFINE([HAVE_DECL_\&], 1,
      [Define to 1 if we found this declaration otherwise define to 0.])])dnl
fi
])

dnl Find the location of the private Tcl headers
dnl When Tcl is installed, this is TCL_INCLUDE_SPEC/tcl-private/generic
dnl When Tcl is in the build tree, this is not needed.
dnl
dnl Note: you must use first use SC_LOAD_TCLCONFIG!
AC_DEFUN([CY_AC_TCL_PRIVATE_HEADERS], [
  AC_MSG_CHECKING([for Tcl private headers])
  private_dir=""
  dir=`echo ${TCL_INCLUDE_SPEC}/tcl-private/generic | sed -e s/-I//`
  if test -f ${dir}/tclInt.h ; then
    private_dir=${dir}
  fi

  if test x"${private_dir}" = x; then
    AC_MSG_ERROR(could not find private Tcl headers)
  else
    TCL_PRIVATE_INCLUDE="-I${private_dir}"
    AC_MSG_RESULT(${private_dir})
  fi
])

dnl Find the location of the private Tk headers
dnl When Tk is installed, this is TK_INCLUDE_SPEC/tk-private/generic
dnl When Tk is in the build tree, this not needed.
dnl
dnl Note: you must first use SC_LOAD_TKCONFIG
AC_DEFUN([CY_AC_TK_PRIVATE_HEADERS], [
  AC_MSG_CHECKING([for Tk private headers])
  private_dir=""
  dir=`echo ${TK_INCLUDE_SPEC}/tk-private/generic | sed -e s/-I//`
  if test -f ${dir}/tkInt.h; then
    private_dir=${dir}
  fi

  if test x"${private_dir}" = x; then
    AC_MSG_ERROR(could not find Tk private headers)
  else
    TK_PRIVATE_INCLUDE="-I${private_dir}"
    AC_MSG_RESULT(${private_dir})
  fi
])

dnl GDB_AC_DEFINE_RELOCATABLE([VARIABLE], [ARG-NAME], [SHELL-VARIABLE])
dnl For use in processing directory values for --with-foo.
dnl If the path in SHELL_VARIABLE is relative to the prefix, then the
dnl result is relocatable, then this will define the C macro
dnl VARIABLE_RELOCATABLE to 1; otherwise it is defined as 0.
AC_DEFUN([GDB_AC_DEFINE_RELOCATABLE], [
  if test "x$exec_prefix" = xNONE || test "x$exec_prefix" = 'x${prefix}'; then
     if test "x$prefix" = xNONE; then
     	test_prefix=/usr/local
     else
	test_prefix=$prefix
     fi
  else
     test_prefix=$exec_prefix
  fi
  value=0
  case [$3] in
     "${test_prefix}"|"${test_prefix}/"*|\
	'${exec_prefix}'|'${exec_prefix}/'*)
     value=1
     ;;
  esac
  AC_DEFINE_UNQUOTED([$1]_RELOCATABLE, $value, [Define if the $2 directory should be relocated when GDB is moved.])
])

dnl GDB_AC_WITH_DIR([VARIABLE], [ARG-NAME], [HELP], [DEFAULT])
dnl Add a new --with option that defines a directory.
dnl The result is stored in VARIABLE.  AC_DEFINE_DIR is called
dnl on this variable, as is AC_SUBST.
dnl ARG-NAME is the base name of the argument (without "--with").
dnl HELP is the help text to use.
dnl If the user's choice is relative to the prefix, then the
dnl result is relocatable, then this will define the C macro
dnl VARIABLE_RELOCATABLE to 1; otherwise it is defined as 0.
dnl DEFAULT is the default value, which is used if the user
dnl does not specify the argument.
AC_DEFUN([GDB_AC_WITH_DIR], [
  AC_ARG_WITH([$2], AS_HELP_STRING([--with-][$2][=PATH], [$3]), [
    [$1]=$withval], [[$1]=[$4]])
  AC_DEFINE_DIR([$1], [$1], [$3])
  AC_SUBST([$1])
  GDB_AC_DEFINE_RELOCATABLE([$1], [$2], ${ac_define_dir})
  ])

dnl GDB_AC_CHECK_BFD([MESSAGE], [CV], [CODE], [HEADER])
dnl Check whether BFD provides a feature.
dnl MESSAGE is the "checking" message to display.
dnl CV is the name of the cache variable where the result is stored.
dnl The result will be "yes" or "no".
dnl CODE is some code to compile that checks for the feature.
dnl A link test is run.
dnl HEADER is the name of an extra BFD header to include.
AC_DEFUN([GDB_AC_CHECK_BFD], [
  OLD_CFLAGS=$CFLAGS
  OLD_LDFLAGS=$LDFLAGS
  OLD_LIBS=$LIBS
  OLD_CC=$CC
  # Put the old CFLAGS/LDFLAGS last, in case the user's (C|LD)FLAGS
  # points somewhere with bfd, with -I/foo/lib and -L/foo/lib.  We
  # always want our bfd.
  CFLAGS="-I${srcdir}/../include -I../bfd -I${srcdir}/../bfd $CFLAGS"
  LDFLAGS="-L../bfd -L../libiberty $LDFLAGS"
  # LTLIBINTL because we use libtool as CC below.
  intl="$(echo "$LTLIBINTL" | sed 's,\$[[{(]top_builddir[)}]]/,,')"
  LIBS="-lbfd -liberty $intl $LIBS"
  CC="./libtool --quiet --mode=link $CC"
  AC_CACHE_CHECK(
    [$1],
    [$2],
    [AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
	  [#include <stdlib.h>
	   #include <string.h>
	   #include "bfd.h"
	   #include "$4"],
	  [return $3;]
	)],
       [[$2]=yes],
       [[$2]=no]
     )]
  )
  CC=$OLD_CC
  CFLAGS=$OLD_CFLAGS
  LDFLAGS=$OLD_LDFLAGS
  LIBS=$OLD_LIBS])

dnl GDB_GUILE_PROGRAM_NAMES([PKG-CONFIG], [VERSION])
dnl
dnl Define and substitute 'GUILD' to contain the absolute file name of
dnl the 'guild' command for VERSION, using PKG-CONFIG.  (This is
dnl similar to Guile's 'GUILE_PROGS' macro.)
AC_DEFUN([GDB_GUILE_PROGRAM_NAMES], [
  AC_CACHE_CHECK([for the absolute file name of the 'guild' command],
    [ac_cv_guild_program_name],
    [ac_cv_guild_program_name="`$1 --variable guild $2`"

     # In Guile up to 2.0.11 included, guile-2.0.pc would not define
     # the 'guild' and 'bindir' variables.  In that case, try to guess
     # what the program name is, at the risk of getting it wrong if
     # Guile was configured with '--program-suffix' or similar.
     if test "x$ac_cv_guild_program_name" = "x"; then
       guile_exec_prefix="`$1 --variable exec_prefix $2`"
       ac_cv_guild_program_name="$guile_exec_prefix/bin/guild"
     fi
  ])

  if ! "$ac_cv_guild_program_name" --version >&AS_MESSAGE_LOG_FD 2>&AS_MESSAGE_LOG_FD; then
    AC_MSG_ERROR(['$ac_cv_guild_program_name' appears to be unusable])
  fi

  GUILD="$ac_cv_guild_program_name"
  AC_SUBST([GUILD])
])

dnl GDB_GUILD_TARGET_FLAG
dnl
dnl Compute the value of GUILD_TARGET_FLAG.
dnl For native builds this is empty.
dnl For cross builds this is --target=<host>.
AC_DEFUN([GDB_GUILD_TARGET_FLAG], [
  if test "$cross_compiling" = no; then
    GUILD_TARGET_FLAG=
  else
    GUILD_TARGET_FLAG="--target=$host"
  fi
  AC_SUBST(GUILD_TARGET_FLAG)
])

dnl GDB_TRY_GUILD([SRC-FILE])
dnl
dnl We precompile the .scm files and install them with gdb, so make sure
dnl guild works for this host.
dnl The .scm files are precompiled for several reasons:
dnl 1) To silence Guile during gdb startup (Guile's auto-compilation output
dnl    is unnecessarily verbose).
dnl 2) Make gdb developers see compilation errors/warnings during the build,
dnl    and not leave it to later when the user runs gdb.
dnl 3) As a convenience for the user, so that one copy of the files is built
dnl    instead of one copy per user.
dnl
dnl Make sure guild can handle this host by trying to compile SRC-FILE, and
dnl setting ac_cv_guild_ok to yes or no.
dnl Note that guild can handle cross-compilation.
dnl It could happen that guild can't handle the host, but guile would still
dnl work.  For the time being we're conservative, and if guild doesn't work
dnl we punt.
AC_DEFUN([GDB_TRY_GUILD], [
  AC_REQUIRE([GDB_GUILD_TARGET_FLAG])
  AC_CACHE_CHECK([whether guild supports this host],
    [ac_cv_guild_ok],
    [echo "$ac_cv_guild_program_name compile $GUILD_TARGET_FLAG -o conftest.go $1" >&AS_MESSAGE_LOG_FD
     if "$ac_cv_guild_program_name" compile $GUILD_TARGET_FLAG -o conftest.go "$1" >&AS_MESSAGE_LOG_FD 2>&AS_MESSAGE_LOG_FD; then
       ac_cv_guild_ok=yes
     else
       ac_cv_guild_ok=no
     fi])
])
