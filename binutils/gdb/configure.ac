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

AC_INIT
AC_CONFIG_MACRO_DIRS([.. ../config])
AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS(config.h:config.in, [echo > stamp-h])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CXX

AC_USE_SYSTEM_EXTENSIONS
ACX_LARGEFILE
AM_PROG_INSTALL_STRIP

AC_CONFIG_AUX_DIR(..)

# Set build, build_cpu, build_vendor and build_os.
AC_CANONICAL_BUILD

# Set host, host_cpu, host_vendor, and host_os.
AC_CANONICAL_HOST

# Set target, target_cpu, target_vendor, and target_os.
AC_CANONICAL_TARGET

ACX_NONCANONICAL_TARGET

AC_ARG_PROGRAM

# We require libtool to link with the in-tree libtool libraries
# the proper way.
LT_INIT
# ... and we need it soon, since it is used by some of the
# link tests in the configure script.
LT_OUTPUT

# We require a C++17 compiler.  Check if one is available, and if
# necessary, set CXX_DIALECT to some -std=xxx switch.
AX_CXX_COMPILE_STDCXX(17, , mandatory)

GDB_AC_COMMON

# Dependency checking.
ZW_CREATE_DEPDIR
ZW_PROG_COMPILER_DEPENDENCIES([CC])

# Since the first call to PKG_CHECK_MODULES may not happen (is guarded by
# a condition), we must call PKG_PROG_PKG_CONFIG explicitly to probe for
# pkg-config.
PKG_PROG_PKG_CONFIG

dnl List of object files and targets accumulated by configure.

CONFIG_OBS=
CONFIG_DEPS=
CONFIG_SRCS=
ENABLE_CFLAGS=

CONFIG_ALL=
CONFIG_CLEAN=
CONFIG_INSTALL=
CONFIG_UNINSTALL=

dnl Set up for gettext.
ZW_GNU_GETTEXT_SISTER_DIR

localedir='${datadir}/locale'
AC_SUBST(localedir)

if test x"$USE_NLS" = xyes; then
   CONFIG_ALL="$CONFIG_ALL all-po"
   CONFIG_CLEAN="$CONFIG_CLEAN clean-po"
   CONFIG_INSTALL="$CONFIG_INSTALL install-po"
   CONFIG_UNINSTALL="$CONFIG_UNINSTALL uninstall-po"
fi

PACKAGE=gdb
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of this package. ])
AC_SUBST(PACKAGE)

# We never need to detect it in this sub-configure.
# But preserve it for config.status --recheck.
AC_ARG_VAR(MAKEINFO,
  [Parent configure detects if it is of sufficient version.])
AC_ARG_VAR(MAKEINFOFLAGS,
  [Parameters for MAKEINFO.])

MAKEINFO_EXTRA_FLAGS=""
AC_CACHE_CHECK([whether $MAKEINFO supports @click], gdb_cv_have_makeinfo_click,
  [echo '@clicksequence{a @click{} b}' >conftest.texinfo
  if eval "$MAKEINFO conftest.texinfo >&5 2>&5"; then
    gdb_cv_have_makeinfo_click=yes
  else
    gdb_cv_have_makeinfo_click=no
  fi])
if test x"$gdb_cv_have_makeinfo_click" = xyes; then
  MAKEINFO_EXTRA_FLAGS="$MAKEINFO_EXTRA_FLAGS -DHAVE_MAKEINFO_CLICK"
fi
AC_SUBST(MAKEINFO_EXTRA_FLAGS)

GDB_AC_WITH_DIR(DEBUGDIR, separate-debug-dir,
    [look for global separate debug info in this path @<:@LIBDIR/debug@:>@],
    [${libdir}/debug])

AC_ARG_WITH(additional-debug-dirs,
	    AS_HELP_STRING([--with-additional-debug-dirs=PATHs],
			   [colon-separated list of additional directories to
			    search for separate debug info]),
	    [AC_DEFINE_UNQUOTED(ADDITIONAL_DEBUG_DIRS, "${withval}",
				Additional directories to look for separate
				debug info.)])

# We can't pass paths as command line arguments.
# Mingw32 tries to be clever and will convert the paths for us.
# For example -DBINDIR="/usr/local/bin" passed on the command line may get
# converted to -DBINDIR="E:/msys/mingw32/msys/1.0/local/bin".
# This breaks GDB's relocatable path conversions since paths passed in
# config.h would not get so translated, the path prefixes no longer match.
AC_DEFINE_DIR(BINDIR, bindir, [Directory of programs.])

# GDB's datadir relocation

GDB_AC_WITH_DIR(GDB_DATADIR, gdb-datadir,
    [look for global separate data files in this path @<:@DATADIR/gdb@:>@],
    [${datadir}/gdb])

AC_ARG_WITH(relocated-sources,
AS_HELP_STRING([--with-relocated-sources=PATH], [automatically relocate this path for source files]),
[reloc_srcdir="${withval}"
 AC_DEFINE_DIR(RELOC_SRCDIR, reloc_srcdir,
              [Relocated directory for source files. ])
])

AC_MSG_CHECKING([for default auto-load directory])
AC_ARG_WITH(auto-load-dir,
AS_HELP_STRING([--with-auto-load-dir=PATH],
  [directories from which to load auto-loaded scripts @<:@$debugdir:$datadir/auto-load@:>@]),,
  [with_auto_load_dir='$debugdir:$datadir/auto-load'])
escape_dir=`echo $with_auto_load_dir | sed -e 's/[[$]]datadir\>/\\\\\\\\\\\\&/g' -e 's/[[$]]debugdir\>/\\\\\\\\\\\\&/g'`
AC_DEFINE_DIR(AUTO_LOAD_DIR, escape_dir,
	      [Directories from which to load auto-loaded scripts.])
AC_MSG_RESULT([$with_auto_load_dir])

AC_MSG_CHECKING([for default auto-load safe-path])
AC_ARG_WITH(auto-load-safe-path,
AS_HELP_STRING([--with-auto-load-safe-path=PATH],
  [directories safe to hold auto-loaded files @<:@--with-auto-load-dir@:>@])
AS_HELP_STRING([--without-auto-load-safe-path],
               [do not restrict auto-loaded files locations]),
    [if test "$with_auto_load_safe_path" = "no"; then
     with_auto_load_safe_path="/"
     fi],
[with_auto_load_safe_path="$with_auto_load_dir"])
escape_dir=`echo $with_auto_load_safe_path | sed -e 's/[[$]]datadir\>/\\\\\\\\\\\\&/g' -e 's/[[$]]debugdir\>/\\\\\\\\\\\\&/g'`
AC_DEFINE_DIR(AUTO_LOAD_SAFE_PATH, escape_dir,
	      [Directories safe to hold auto-loaded files.])
AC_MSG_RESULT([$with_auto_load_safe_path])

AC_CONFIG_SUBDIRS(testsuite)

# Check whether to support alternative target configurations
AC_ARG_ENABLE(targets,
AS_HELP_STRING([--enable-targets=TARGETS], [alternative target configurations]),
[case "${enableval}" in
  yes | "") AC_MSG_ERROR(enable-targets option must specify target names or 'all')
            ;;
  no)       enable_targets= ;;
  *)        enable_targets=$enableval ;;
esac])

BFD_64_BIT

# Provide defaults for some variables set by the per-host and per-target
# configuration.
gdb_host_obs=posix-hdep.o

if test "${target}" = "${host}"; then
  gdb_native=yes
else
  gdb_native=no
fi

. $srcdir/configure.host

# Accumulate some settings from configure.tgt over all enabled targets

TARGET_OBS=
all_targets=
HAVE_NATIVE_GCORE_TARGET=

for targ_alias in `echo $target_alias $enable_targets | sed 's/,/ /g'`
do
  if test "$targ_alias" = "all"; then
    all_targets=true
  else
    # Canonicalize the secondary target names.
    result=`$ac_config_sub $targ_alias 2>/dev/null`
    if test -n "$result"; then
        targ=$result
    else
        targ=$targ_alias
    fi

    . ${srcdir}/configure.tgt

    AS_IF([test -z "${gdb_target_obs}"],
      [AC_MSG_ERROR([configuration ${targ} is unsupported.])])

    # Target-specific object files
    for i in ${gdb_target_obs}; do
        case " $TARGET_OBS " in
        *" ${i} "*) ;;
        *)
          TARGET_OBS="$TARGET_OBS ${i}"
          ;;
        esac
    done

    # Check whether this target needs 64-bit CORE_ADDR
    if test x${enable_64_bit_bfd} = xno; then
      . ${srcdir}/../bfd/config.bfd
    fi

    # Check whether this target is native and supports gcore.
    if test $gdb_native = yes -a "$targ_alias" = "$target_alias" \
       && $gdb_have_gcore; then
      HAVE_NATIVE_GCORE_TARGET=1
    fi
  fi
done

if test x${all_targets} = xtrue; then
  if test x${enable_64_bit_bfd} = xyes; then
    TARGET_OBS='$(ALL_TARGET_OBS) $(ALL_64_TARGET_OBS)'
  else
    TARGET_OBS='$(ALL_TARGET_OBS)'
  fi
fi

# AMD debugger API support.

AC_ARG_WITH([amd-dbgapi],
	    [AS_HELP_STRING([--with-amd-dbgapi],
			    [support for the amd-dbgapi target (yes / no / auto)])],
	    [GDB_CHECK_YES_NO_AUTO_VAL([$withval], [--with-amd-dbgapi])],
	    [with_amd_dbgapi=auto])

# If the user passes --without-amd-dbgapi but also explicitly enables a target
# that requires amd-dbgapi, it is an error.
if test "$with_amd_dbgapi" = no -a "$gdb_require_amd_dbgapi" = true; then
  AC_MSG_ERROR([an explicitly enabled target requires amd-dbgapi, but amd-dbgapi is explicitly disabled])
fi

# Look for amd-dbgapi if:
#
#   - a target architecture requiring it has explicitly been enabled, or
#   - --enable-targets=all was provided and the user did not explicitly disable
#     amd-dbgapi support
if test "$gdb_require_amd_dbgapi" = true \
     -o \( "$all_targets" = true -a "$with_amd_dbgapi" != no \); then
  # amd-dbgapi version 0.68 is part of ROCm 5.4.  There is no guarantee of API
  # stability until amd-dbgapi hits 1.0, but for convenience, still check for
  # greater or equal that version.  It can be handy when testing with a newer
  # version of the library.
  PKG_CHECK_MODULES([AMD_DBGAPI], [amd-dbgapi >= 0.68.0],
		    [has_amd_dbgapi=yes], [has_amd_dbgapi=no])

  if test "$has_amd_dbgapi" = "yes"; then
    AC_DEFINE(HAVE_AMD_DBGAPI, 1, [Define if amd-dbgapi is being linked in.])
    TARGET_OBS="$TARGET_OBS amd-dbgapi-target.o"

    # If --enable-targets=all was provided, use the list of all files depending
    # on amd-dbgapi that is hardcoded in the Makefile.  Else, the appropriate
    # architecture entry in configure.tgt will have added the files to
    # gdb_target_obs.
    if test "$all_targets" = true; then
      TARGET_OBS="$TARGET_OBS \$(ALL_AMD_DBGAPI_TARGET_OBS)"
    fi
  elif test "$gdb_require_amd_dbgapi" = true -o "$with_amd_dbgapi" = yes; then
    # amd-dbgapi was not found and...
    #
    #   - a target requiring it was explicitly enabled, or
    #   - the user explicitly wants to enable amd-dbgapi
    AC_MSG_ERROR([amd-dbgapi is required, but cannot find an appropriate version: $AMD_DBGAPI_PKG_ERRORS])
  fi
fi

AC_SUBST(TARGET_OBS)
AC_SUBST(HAVE_NATIVE_GCORE_TARGET)

# For other settings, only the main target counts.
gdb_sim=
gdb_osabi=
targ=$target; . ${srcdir}/configure.tgt

# Fetch the default architecture and default target vector from BFD.
targ=$target; . $srcdir/../bfd/config.bfd

# We only want the first architecture, so strip off the others if
# there is more than one.
targ_archs=`echo $targ_archs | sed 's/ .*//'`

if test "x$targ_archs" != x; then
  AC_DEFINE_UNQUOTED(DEFAULT_BFD_ARCH, $targ_archs,
    [Define to BFD's default architecture. ])
fi
if test "x$targ_defvec" != x; then
  AC_DEFINE_UNQUOTED(DEFAULT_BFD_VEC, $targ_defvec,
    [Define to BFD's default target vector. ])
fi

# Enable TUI.
AC_ARG_ENABLE(tui,
AS_HELP_STRING([--enable-tui],
	       [enable full-screen terminal user interface (TUI)]),
	       [GDB_CHECK_YES_NO_AUTO_VAL([$enableval], [--enable-tui])],
	       [enable_tui=auto])

# Enable gdbtk.
AC_ARG_ENABLE([gdbtk],
	      [AS_HELP_STRING([--enable-gdbtk], [enable gdbtk graphical user interface (GUI)])],
	      [GDB_CHECK_YES_NO_VAL([$enableval], [--enable-gdbtk])],
	      [if test -d "$srcdir/gdbtk"; then
		 enable_gdbtk=yes
	       else
		 enable_gdbtk=no
	       fi])
# We unconditionally disable gdbtk tests on selected platforms.
case $host_os in
  go32* | windows*)
    AC_MSG_WARN([gdbtk isn't supported on $host; disabling])
    enable_gdbtk=no ;;
esac

# Handle optional debuginfod support
AC_DEBUGINFOD

# Libunwind support for ia64.
AC_ARG_WITH(libunwind-ia64,
AS_HELP_STRING([--with-libunwind-ia64],
	       [use libunwind frame unwinding for ia64 targets]),,
	    [with_libunwind_ia64=auto])

# Backward compatibility option.
if test "${with_libunwind+set}" = set; then
  if test x"$with_libunwind_ia64" != xauto; then
    AC_MSG_ERROR(
      [option --with-libunwind is deprecated, use --with-libunwind-ia64])
  fi
  AC_MSG_WARN([option --with-libunwind is deprecated, use --with-libunwind-ia64])
  with_libunwind_ia64="$with_libunwind"
fi

case "$with_libunwind_ia64" in
  yes | no)
    ;;
  auto)
    AC_CHECK_HEADERS(libunwind-ia64.h)
    with_libunwind_ia64=$ac_cv_header_libunwind_ia64_h
    ;;
  *)
    AC_MSG_ERROR(
      [bad value $with_libunwind_ia64 for GDB --with-libunwind-ia64 option])
    ;;
esac

if test x"$with_libunwind_ia64" = xyes; then
  AC_CHECK_HEADERS(libunwind-ia64.h)
  if test x"$ac_cv_header_libunwind_ia64_h" != xyes; then
    AC_MSG_ERROR([GDB option --with-libunwind-ia64 requires libunwind-ia64.h])
  fi
  CONFIG_OBS="$CONFIG_OBS ia64-libunwind-tdep.o"
  CONFIG_DEPS="$CONFIG_DEPS ia64-libunwind-tdep.o"
  CONFIG_SRCS="$CONFIG_SRCS ia64-libunwind-tdep.c"
fi

opt_curses=no
AC_ARG_WITH(curses, AS_HELP_STRING([--with-curses], [use the curses library instead of the termcap library]), opt_curses=$withval)

prefer_curses=no
if test "$opt_curses" = "yes"; then
  prefer_curses=yes
fi

# Profiling support.
AC_ARG_ENABLE([profiling],
	      [AS_HELP_STRING([--enable-profiling], [enable profiling of GDB])],
	      [GDB_CHECK_YES_NO_VAL([$enableval], [--enable-profiling])],
	      [enable_profiling=no])

AC_CHECK_FUNCS(monstartup _mcleanup)
AC_CACHE_CHECK(
  [for _etext],
  [ac_cv_var__etext],
  [AC_LINK_IFELSE(
     [AC_LANG_PROGRAM(
	[#include <stdlib.h>
	 extern char _etext;],
	[free (&_etext);]
      )],
     [ac_cv_var__etext=yes],
     [ac_cv_var__etext=no]
   )]
)
if test "$ac_cv_var__etext" = yes; then
  AC_DEFINE(HAVE__ETEXT, 1,
            [Define to 1 if your system has the _etext variable. ])
fi
AC_CACHE_CHECK(
  [for etext],
  [ac_cv_var_etext],
  [AC_LINK_IFELSE(
     [AC_LANG_PROGRAM(
	[#include <stdlib.h>
	 extern char etext;],
	[free (&etext);]
      )],
     [ac_cv_var_etext=yes],
     [ac_cv_var_etext=no]
   )]
)
if test "$ac_cv_var_etext" = yes; then
  AC_DEFINE(HAVE_ETEXT, 1,
            [Define to 1 if your system has the etext variable. ])
fi
if test "$enable_profiling" = yes ; then
  if test "$ac_cv_func_monstartup" = no || test "$ac_cv_func__mcleanup" = no; then
    AC_MSG_ERROR(--enable-profiling requires monstartup and _mcleanup)
  fi
  PROFILE_CFLAGS=-pg
  OLD_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $PROFILE_CFLAGS"

  AC_CACHE_CHECK(
    [whether $CC supports -pg],
    [ac_cv_cc_supports_pg],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([], [int x;])],
       [ac_cv_cc_supports_pg=yes],
       [ac_cv_cc_supports_pg=no]
     )]
  )

  if test "$ac_cv_cc_supports_pg" = no; then
    AC_MSG_ERROR(--enable-profiling requires a compiler which supports -pg)
  fi

  CFLAGS="$OLD_CFLAGS"
fi

CODESIGN_CERT=
AC_ARG_ENABLE([codesign],
  AS_HELP_STRING([--enable-codesign=CERT],
                 [sign gdb with 'codesign -s CERT']),
  [CODESIGN_CERT=$enableval])
AC_SUBST([CODESIGN_CERT])

ACX_PKGVERSION([GDB])
ACX_BUGURL([https://www.gnu.org/software/gdb/bugs/])
AC_DEFINE_UNQUOTED([PKGVERSION], ["$PKGVERSION"], [Additional package description])
AC_DEFINE_UNQUOTED([REPORT_BUGS_TO], ["$REPORT_BUGS_TO"], [Bug reporting address])

# --------------------- #
# Checks for programs.  #
# --------------------- #

AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB
AC_PROG_YACC

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(DLLTOOL, dlltool)
AC_CHECK_TOOL(WINDRES, windres)

case $host_os in
  gnu*)
    # Needed for GNU Hurd hosts.
    AC_CHECK_TOOL(MIG, mig)
    if test x"$MIG" = x; then
      AC_MSG_ERROR([MIG not found but required for $host hosts])
    fi
    ;;
esac

# ---------------------- #
# Checks for libraries.  #
# ---------------------- #

# We might need to link with -lm; most simulators need it.
AC_CHECK_LIB(m, main)

# Some systems (e.g. Solaris) have `gethostbyname' in libnsl.
AC_SEARCH_LIBS(gethostbyname, nsl)

# Link in zlib/zstd if we can.  This allows us to read compressed debug
# sections.
AM_ZLIB
AC_ZSTD

AM_ICONV

# GDB may fork/exec the iconv program to get the list of supported character
# sets.  Allow the user to specify where to find it.
# There are several factors affecting the choice of option name:
# - There is already --with-libiconv-prefix but we can't use it, it specifies
#   the build-time location of libiconv files.
# - The program we need to find is iconv, which comes with glibc.  The user
#   doesn't necessarily have libiconv installed.  Therefore naming this
#   --with-libiconv-foo feels wrong.
# - We want the path to be relocatable, but GDB_AC_DEFINE_RELOCATABLE is
#   defined to work on directories not files (though it really doesn't know
#   the difference).
# - Calling this --with-iconv-prefix is perceived to cause too much confusion
#   with --with-libiconv-prefix.
# Putting these together is why the option name is --with-iconv-bin.

AC_ARG_WITH(iconv-bin,
AS_HELP_STRING([--with-iconv-bin=PATH], [specify where to find the iconv program]),
[iconv_bin="${withval}"
 AC_DEFINE_UNQUOTED([ICONV_BIN], ["${iconv_bin}"],
                    [Path of directory of iconv program.])
 GDB_AC_DEFINE_RELOCATABLE(ICONV_BIN, iconv, ${iconv_bin})
])

# For the TUI, we need enhanced curses functionality.
if test x"$enable_tui" != xno; then
  prefer_curses=yes
fi

curses_found=no
if test x"$prefer_curses" = xyes; then
  # FIXME: kettenis/20040905: We prefer ncurses over the vendor-supplied
  # curses library because the latter might not provide all the
  # functionality we need.  However, this leads to problems on systems
  # where the linker searches /usr/local/lib, but the compiler doesn't
  # search /usr/local/include, if ncurses is installed in /usr/local.  A
  # default installation of ncurses on alpha*-dec-osf* will lead to such
  # a situation.
  AC_SEARCH_LIBS(waddstr, [ncursesw ncurses cursesX curses],
                 [curses_found=yes
                  AC_DEFINE([HAVE_LIBCURSES], [1],
                            [Define to 1 if curses is enabled.])
                 ])
fi

# Check whether we should enable the TUI, but only do so if we really
# can.
if test x"$enable_tui" != xno; then
  if test "$curses_found" != no; then
    CONFIG_OBS="$CONFIG_OBS \$(SUBDIR_TUI_OBS)"
    CONFIG_DEPS="$CONFIG_DEPS \$(SUBDIR_TUI_DEPS)"
    CONFIG_SRCS="$CONFIG_SRCS \$(SUBDIR_TUI_SRCS)"
    ENABLE_CFLAGS="$ENABLE_CFLAGS \$(SUBDIR_TUI_CFLAGS)"
  else
    if test x"$enable_tui" = xyes; then
      AC_MSG_ERROR([no enhanced curses library found; disable TUI])
    else
      AC_MSG_WARN([no enhanced curses library found; disabling TUI])
    fi
  fi
fi

# Since GDB uses Readline, we need termcap functionality.  In many
# cases this will be provided by the curses library, but some systems
# have a separate termcap library, or no curses library at all.

case $host_os in
  cygwin*)
    if test -d "$srcdir/libtermcap"; then
      LIBS="../libtermcap/libtermcap.a $LIBS"
      ac_cv_search_tgetent="../libtermcap/libtermcap.a"
    fi ;;
  go32* | *djgpp*)
    ac_cv_search_tgetent="none required"
    ;;
esac

# These are the libraries checked by Readline.
AC_SEARCH_LIBS(tgetent, [termcap tinfow tinfo curses ncursesw ncurses])

if test "$ac_cv_search_tgetent" = no; then
  CONFIG_OBS="$CONFIG_OBS stub-termcap.o"
fi

AC_ARG_WITH([system-readline],
  [AS_HELP_STRING([--with-system-readline],
                  [use installed readline library])])

if test "$with_system_readline" = yes; then
  AC_CHECK_HEADERS(readline/readline.h, [readline_h=yes], [readline_h=no])
  if test "$readline_h" = "no"; then
    AC_MSG_ERROR([readline development packages are probably missing])
  fi
  AC_CACHE_CHECK(
    [whether system readline is new enough],
    [gdb_cv_readline_ok],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
	  [#include <stdio.h>
	   #include <readline/readline.h>],
	  [#if RL_VERSION_MAJOR < 7
	   # error "readline version 7 required"
	   #endif]
	)],
       [gdb_cv_readline_ok=yes],
       [gdb_cv_readline_ok=no]
     )]
  )
  if test "$gdb_cv_readline_ok" != yes; then
    AC_MSG_ERROR([system readline is not new enough])
  fi

  READLINE=-lreadline
  READLINE_DEPS=
  READLINE_CFLAGS=
  READLINE_TEXI_INCFLAG=
  READLINE_DOC_SOURCE_INCLUDES='$(READLINE_SYSTEM_DOC_INCLUDES)'
else
  READLINE='$(READLINE_DIR)/libreadline.a'
  READLINE_DEPS='$(READLINE)'
  READLINE_CFLAGS='-I$(READLINE_SRC)/..'
  READLINE_TEXI_INCFLAG='-I $(READLINE_DIR)'
  READLINE_DOC_SOURCE_INCLUDES='$(READLINE_INTREE_DOC_INCLUDES)'
fi
AC_SUBST(READLINE)
AC_SUBST(READLINE_DEPS)
AC_SUBST(READLINE_CFLAGS)
AC_SUBST(READLINE_TEXI_INCFLAG)
AC_SUBST(READLINE_DOC_SOURCE_INCLUDES)

# Generate jit-reader.h

# This is typedeffed to GDB_CORE_ADDR in jit-reader.h
TARGET_PTR=

AC_CHECK_SIZEOF(unsigned long long)
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(unsigned __int128)

if test "x${ac_cv_sizeof_unsigned_long}" = "x8"; then
  TARGET_PTR="unsigned long"
elif test "x${ac_cv_sizeof_unsigned_long_long}" = "x8"; then
  TARGET_PTR="unsigned long long"
elif test "x${ac_cv_sizeof_unsigned___int128}" = "x16"; then
  TARGET_PTR="unsigned __int128"
else
  TARGET_PTR="unsigned long"
fi

AC_SUBST(TARGET_PTR)
AC_CONFIG_FILES([jit-reader.h:jit-reader.in])

AC_SEARCH_LIBS(dlopen, dl)

GDB_AC_WITH_DIR([JIT_READER_DIR], [jit-reader-dir],
                [directory to load the JIT readers from],
                [${libdir}/gdb])

AC_ARG_WITH(expat,
  AS_HELP_STRING([--with-expat], [include expat support (auto/yes/no)]),
  [], [with_expat=auto])
AC_MSG_CHECKING([whether to use expat])
AC_MSG_RESULT([$with_expat])

if test "${with_expat}" = no; then
  AC_MSG_WARN([expat support disabled; some features may be unavailable.])
  HAVE_LIBEXPAT=no
else
  AC_LIB_HAVE_LINKFLAGS([expat], [], [#include "expat.h"],
			[XML_Parser p = XML_ParserCreate (0);])
  if test "$HAVE_LIBEXPAT" != yes; then
    if test "$with_expat" = yes; then
      AC_MSG_ERROR([expat is missing or unusable])
    else
      AC_MSG_WARN([expat is missing or unusable; some features may be unavailable.])
    fi
  else
    save_LIBS=$LIBS
    LIBS="$LIBS $LIBEXPAT"
    AC_CHECK_FUNCS(XML_StopParser)
    LIBS=$save_LIBS
  fi
fi

AC_ARG_VAR(GMPLIBS,[How to link GMP])
AC_ARG_VAR(GMPINC,[How to find GMP include files])

# --------------------- #
# Check for libpython.  #
# --------------------- #

dnl Utility to simplify finding libpython.
dnl $1 = the shell variable to assign the result to
dnl      If libpython is found we store $version here.
dnl $2 = additional flags to add to CPPFLAGS
dnl $3 = additional flags to add to LIBS

AC_DEFUN([AC_TRY_LIBPYTHON],
[
  define([have_libpython_var],$1)
  new_CPPFLAGS=$2
  new_LIBS=$3
  AC_MSG_CHECKING([for python])
  save_CPPFLAGS=$CPPFLAGS
  save_LIBS=$LIBS
  CPPFLAGS="$CPPFLAGS $new_CPPFLAGS"
  LIBS="$new_LIBS $LIBS"
  found_usable_python=no
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include "Python.h"]],
                                 [[
				    #if PY_MAJOR_VERSION != 3
				    # error "We only support Python 3"
				    #endif
				    Py_Initialize ();
				 ]])],
                 [have_libpython_var=yes
                  found_usable_python=yes
                  PYTHON_CPPFLAGS=$new_CPPFLAGS
                  PYTHON_LIBS=$new_LIBS])
  CPPFLAGS=$save_CPPFLAGS
  LIBS=$save_LIBS
  AC_MSG_RESULT([${found_usable_python}])
])

dnl There are several different values for --with-python:
dnl
dnl no -   Don't include python support.
dnl yes -  Include python support, error if it's missing.
dnl        If we find python in $PATH, use it to fetch configure options,
dnl        otherwise assume the compiler can find it with no help from us.
dnl auto - Same as "yes", but if python is missing from the system,
dnl        fall back to "no".
dnl /path/to/python/exec-prefix -
dnl        Use the python located in this directory.
dnl        If /path/to/python/exec-prefix/bin/python exists, use it to find
dnl        the compilation parameters.  Otherwise use
dnl        -I/path/to/python/exec-prefix/include,
dnl        -L/path/to/python/exec-prefix/lib.
dnl        NOTE: This case is historical.  It is what was done for 7.0/7.1
dnl        but is deprecated.
dnl /path/to/python/executable -
dnl        Run python-config.py with this version of python to fetch the
dnl        compilation parameters.
dnl        NOTE: This needn't be the real python executable.
dnl        In a cross-compilation scenario (build != host), this could be
dnl        a shell script that provides what python-config.py provides for
dnl        --ldflags, --includes, --exec-prefix.
dnl python-executable -
dnl        Find python-executable in $PATH, and then handle the same as
dnl        /path/to/python/executable.
dnl
dnl If a python program is specified, it is used to run python-config.py and
dnl is passed --ldflags, --includes, --exec-prefix.

AC_ARG_WITH(python,
  AS_HELP_STRING([--with-python@<:@=PYTHON@:>@], [include python support (auto/yes/no/<python-program>)]),
  [], [with_python=auto])
AC_MSG_CHECKING([whether to use python])
AC_MSG_RESULT([$with_python])

if test "${with_python}" = no; then
  AC_MSG_WARN([python support disabled; some features may be unavailable.])
  have_libpython=no
else
  case "${with_python}" in
  [[\\/]]* | ?:[[\\/]]*)
    if test -d "${with_python}"; then
      # Assume the python binary is ${with_python}/bin/python.
      python_prog="${with_python}/bin/python"
      python_prefix=
      # If python does not exit ${with_python}/bin, then try in
      # ${with_python}.  On Windows/MinGW, this is where the Python
      # executable is.
      if test ! -x "${python_prog}"; then
        python_prog="${with_python}/python"
        python_prefix=
      fi
      if test ! -x "${python_prog}"; then
        # Fall back to gdb 7.0/7.1 behaviour.
        python_prog=missing
        python_prefix=${with_python}
      fi
    elif test -x "${with_python}"; then
      # While we can't run python compiled for $host (unless host == build),
      # the user could write a script that provides the needed information,
      # so we support that.
      python_prog=${with_python}
      python_prefix=
    else
      AC_MSG_ERROR(invalid value for --with-python)
    fi
    ;;
  */*)
    # Disallow --with-python=foo/bar.
    AC_MSG_ERROR(invalid value for --with-python)
    ;;
  *)
    # The user has either specified auto, yes, or the name of the python
    # program assumed to be in $PATH.
    python_prefix=
    case "${with_python}" in
    yes | auto)
      if test "${build}" = "${host}"; then
        # Look first for 'python', then 'python3'.
        AC_PATH_PROGS(python_prog_path, [python python3], missing)
        if test "${python_prog_path}" = missing; then
          python_prog=missing
        else
          python_prog=${python_prog_path}
        fi
      else
        # Not much we can do except assume the cross-compiler will find the
        # right files.
        python_prog=missing
      fi
      ;;
    *)
      # While we can't run python compiled for $host (unless host == build),
      # the user could write a script that provides the needed information,
      # so we support that.
      python_prog="${with_python}"
      AC_PATH_PROG(python_prog_path, ${python_prog}, missing)
      if test "${python_prog_path}" = missing; then
	AC_MSG_ERROR(unable to find python program ${python_prog})
      fi
      ;;
    esac
  esac

  if test "${python_prog}" != missing; then
    # We have a python program to use, but it may be too old.
    # Don't flag an error for --with-python=auto (the default).
    have_python_config=yes
    python_includes=`${python_prog} ${srcdir}/python/python-config.py --includes`
    if test $? != 0; then
      have_python_config=failed
      if test "${with_python}" != auto; then
	AC_MSG_ERROR(failure running python-config --includes)
      fi
    fi
    python_libs=`${python_prog} ${srcdir}/python/python-config.py --ldflags`
    if test $? != 0; then
      have_python_config=failed
      if test "${with_python}" != auto; then
	AC_MSG_ERROR(failure running python-config --ldflags)
      fi
    fi
    python_prefix=`${python_prog} ${srcdir}/python/python-config.py --exec-prefix`
    if test $? != 0; then
      have_python_config=failed
      if test "${with_python}" != auto; then
	AC_MSG_ERROR(failure running python-config --exec-prefix)
      fi
    fi
  else
    # We do not have a python executable we can use to determine where
    # to find the Python headers and libs.  We cannot guess the include
    # path from the python_prefix either, because that include path
    # depends on the Python version.  So, there is nothing much we can
    # do except assume that the compiler will be able to find those files.
    python_includes=
    python_libs=
    have_python_config=no
  fi

  # If we have python-config, only try the configuration it provides.
  # Otherwise fallback on the old way of trying different versions of
  # python in turn.

  have_libpython=no
  if test "${have_python_config}" = yes; then
    AC_TRY_LIBPYTHON(have_libpython,
                     ${python_includes}, ${python_libs})
  fi

  if test "${have_libpython}" = no; then
    case "${with_python}" in
    yes)
      AC_MSG_ERROR([python is missing or unusable])
      ;;
    auto)
      AC_MSG_WARN([python is missing or unusable; some features may be unavailable.])
      ;;
    *)
      AC_MSG_ERROR([no usable python found at ${with_python}])
      ;;
    esac
  else
    if test -n "${python_prefix}"; then
      AC_DEFINE_UNQUOTED(WITH_PYTHON_PATH, "${python_prefix}",
                         [Define if --with-python provides a path, either directly or via python-config.py --exec-prefix.])
      GDB_AC_DEFINE_RELOCATABLE(PYTHON_PATH, python, ${python_prefix})
    fi
  fi
fi

dnl Use --with-python-libdir to control where GDB looks for the Python
dnl libraries.
dnl
dnl If this is not given then the default will be based on the value
dnl passed to --with-python, which is in the python_prefix variable.
dnl If the --with-python option wasn't given then the default value in
dnl python_prefix is based on running the 'gdb/python/python-config
dnl --exec-prefix' script.
AC_ARG_WITH(python-libdir,
  AS_HELP_STRING([--with-python-libdir@<:@=DIR@:>@], [search for python's libraries in DIR]),
  [],[
    # If no python libdir is specified then select one based on
    # python's prefix path.
    if test -n "${python_prefix}"; then
      with_python_libdir=${python_prefix}/lib
    fi
  ])

if test "${have_libpython}" != no; then
  AC_DEFINE(HAVE_PYTHON, 1, [Define if Python interpreter is being linked in.])
  CONFIG_OBS="$CONFIG_OBS \$(SUBDIR_PYTHON_OBS)"
  CONFIG_DEPS="$CONFIG_DEPS \$(SUBDIR_PYTHON_DEPS)"
  CONFIG_SRCS="$CONFIG_SRCS \$(SUBDIR_PYTHON_SRCS)"
  CONFIG_INSTALL="$CONFIG_INSTALL install-python"
  ENABLE_CFLAGS="$ENABLE_CFLAGS \$(SUBDIR_PYTHON_CFLAGS)"

  if test -n "${with_python_libdir}"; then
    AC_DEFINE_UNQUOTED(WITH_PYTHON_LIBDIR, "${with_python_libdir}",
		       [Directory containing Python's standard libraries from --with-python-libdir.])
    GDB_AC_DEFINE_RELOCATABLE(PYTHON_LIBDIR, [python lib], ${with_python_libdir})
  fi

  # Flags needed to compile Python code (taken from python-config --cflags).
  # We cannot call python-config directly because it will output whatever was
  # used when compiling the Python interpreter itself, including flags which
  # would make the python-related objects be compiled differently from the
  # rest of GDB (e.g., -O2 and -fPIC).
  if test "${GCC}" = yes; then
    tentative_python_cflags="-fno-strict-aliasing -fwrapv"
    # Python headers recommend -DNDEBUG, but it's unclear if that just
    # refers to building Python itself.  In release mode, though, it
    # doesn't hurt for the Python code in gdb to follow.
    $development || tentative_python_cflags="$tentative_python_cflags -DNDEBUG"
  fi

  if test "x${tentative_python_cflags}" != x; then
    AC_MSG_CHECKING(compiler flags for python code)
    for flag in ${tentative_python_cflags}; do
      # Check that the compiler accepts it
      saved_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS $flag"
      AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([], [])],
	[PYTHON_CFLAGS="${PYTHON_CFLAGS} $flag"],
	[]
      )
      CFLAGS="$saved_CFLAGS"
    done
    AC_MSG_RESULT(${PYTHON_CFLAGS})
  fi

  # On x64 Windows, Python's include headers, and pyconfig.h in
  # particular, rely on MS_WIN64 macro to detect that it's a 64bit
  # version of Windows.  Unfortunately, MS_WIN64 is only defined if
  # _MSC_VER, a Microsoft-specific macro, is defined.  So, when
  # building on x64 Windows with GCC, we define MS_WIN64 ourselves.
  # The issue was reported to the Python community, but still isn't
  # solved as of 2012-10-02 (http://bugs.python.org/issue4709).

  case "$gdb_host" in
    mingw64)
           if test "${GCC}" = yes; then
             CPPFLAGS="$CPPFLAGS -DMS_WIN64"
           fi
           ;;
  esac
else
  # Even if Python support is not compiled in, we need to have this file
  # included so that the "python" command, et.al., still exists.
  CONFIG_OBS="$CONFIG_OBS python/python.o"
  CONFIG_SRCS="$CONFIG_SRCS python/python.c"
fi

# Work around Python http://bugs.python.org/issue10112.  See also
# http://bugs.python.org/issue11410, otherwise -Wl,--dynamic-list has
# no effect.  Note that the only test after this that uses Python is
# the -rdynamic/-Wl,--dynamic-list test, and we do want that one to be
# run without -export-dynamic too.
PYTHON_LIBS=`echo $PYTHON_LIBS | sed -e 's/-Xlinker -export-dynamic//'`

AC_SUBST(PYTHON_CFLAGS)
AC_SUBST(PYTHON_CPPFLAGS)
AC_SUBST(PYTHON_LIBS)
AM_CONDITIONAL(HAVE_PYTHON, test "${have_libpython}" != no)

# -------------------- #
# Check for libguile.  #
# -------------------- #

dnl Utility to simplify finding libguile.
dnl $1 = pkg-config-program
dnl $2 = space-separate list of guile versions to try
dnl $3 = yes|no, indicating whether to flag errors or ignore them
dnl $4 = the shell variable to assign the result to
dnl      If libguile is found we store "yes" here.

AC_DEFUN([AC_TRY_LIBGUILE],
[
  pkg_config=$1
  guile_version_list=$2
  flag_errors=$3
  define([have_libguile_var],$4)
  found_usable_guile=checking
  AC_MSG_CHECKING([for usable guile from ${pkg_config}])
  for guile_version in ${guile_version_list}; do
    ${pkg_config} --exists ${guile_version} 2>/dev/null
    if test $? != 0; then
      continue
    fi
    dnl pkg-config says the package exists, so if we get an error now,
    dnl that's bad.
    new_CPPFLAGS=`${pkg_config} --cflags ${guile_version}`
    if test $? != 0; then
      AC_MSG_ERROR([failure running pkg-config --cflags ${guile_version}])
    fi
    new_LIBS=`${pkg_config} --libs ${guile_version}`
    if test $? != 0; then
      AC_MSG_ERROR([failure running pkg-config --libs ${guile_version}])
    fi
    dnl If we get this far, great.
    found_usable_guile=${guile_version}
    break
  done
  if test "${found_usable_guile}" = "checking"; then
    if test "${flag_errors}" = "yes"; then
      AC_MSG_ERROR([unable to find usable guile version from "${guile_version_list}"])
    else
      found_usable_guile=no
    fi
  fi
  dnl One final sanity check.
  dnl The user could have said --with-guile=python-2.7.
  if test "${found_usable_guile}" != no; then
    save_CPPFLAGS=$CPPFLAGS
    save_LIBS=$LIBS
    CPPFLAGS="$CPPFLAGS $new_CPPFLAGS"
    LIBS="$LIBS $new_LIBS"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include "libguile.h"]],
                                   [[scm_init_guile ();]])],
                   [have_libguile_var=yes
                    GUILE_CPPFLAGS=$new_CPPFLAGS
                    GUILE_LIBS=$new_LIBS],
                   [found_usable_guile=no])
    dnl scm_set_automatic_finalization_enabled added in Guile 2.2.
    AC_CHECK_FUNC(scm_set_automatic_finalization_enabled,
      AC_DEFINE(HAVE_GUILE_MANUAL_FINALIZATION, 1,
                [Define if Guile supports manual finalization.])
    )
    CPPFLAGS=$save_CPPFLAGS
    LIBS=$save_LIBS
    if test "${found_usable_guile}" = no; then
      if test "${flag_errors}" = yes; then
        AC_MSG_FAILURE([linking guile version ${guile_version} test program failed])
      fi
    fi
  fi
  AC_MSG_RESULT([${found_usable_guile}])
])

dnl There are several different values for --with-guile:
dnl
dnl no -   Don't include guile support.
dnl yes -  Include guile support, error if it's missing.
dnl        The pkg-config program must be in $PATH.
dnl auto - Same as "yes", but if guile is missing from the system,
dnl        fall back to "no".
dnl guile-version [guile-version-choice-2 ...] -
dnl        A space-separated list of guile package versions to try.
dnl        These are passed to pkg-config as-is.
dnl        E.g., guile-2.0 or guile-2.2-uninstalled
dnl        This requires making sure PKG_CONFIG_PATH is set appropriately.
dnl /path/to/pkg-config -
dnl        Use this pkg-config program.
dnl        NOTE: This needn't be the "real" pkg-config program.
dnl        It could be a shell script.  It is invoked as:
dnl        pkg-config --exists $version
dnl        pkg-config --cflags $version
dnl        pkg-config --libs $version
dnl        pkg-config --variable guild $version
dnl        The script will be called with $version having each value in
dnl        $try_guile_versions until --exists indicates success.

AC_ARG_WITH(guile,
  AS_HELP_STRING([--with-guile@<:@=GUILE@:>@], [include guile support (auto/yes/no/<guile-version>/<pkg-config-program>)]),
  [], [with_guile=auto])
AC_MSG_CHECKING([whether to use guile])
AC_MSG_RESULT([$with_guile])

dnl We check guile with pkg-config.
AC_PATH_PROG(pkg_config_prog_path, pkg-config, missing)

try_guile_versions="guile-3.0 guile-2.2 guile-2.0"
have_libguile=no
case "${with_guile}" in
no)
  AC_MSG_WARN([guile support disabled; some features will be unavailable.])
  ;;
auto)
  if test "${pkg_config_prog_path}" = "missing"; then
    AC_MSG_WARN([pkg-config not found, guile support disabled])
  else
    AC_TRY_LIBGUILE(${pkg_config_prog_path}, ${try_guile_versions}, no, have_libguile)
  fi
  ;;
yes)
  if test "${pkg_config_prog_path}" = "missing"; then
    AC_MSG_ERROR([pkg-config not found])
  fi
  AC_TRY_LIBGUILE(${pkg_config_prog_path}, ${try_guile_versions}, yes, have_libguile)
  ;;
[[\\/]]* | ?:[[\\/]]*)
  if test -x "${with_guile}"; then
    AC_TRY_LIBGUILE(${with_guile}, ${try_guile_versions}, yes, have_libguile)
  else
    AC_MSG_ERROR([Guile config program not executable: ${with_guile}])
  fi
  ;;
"" | */*)
  # Disallow --with=guile="" and --with-guile=foo/bar.
  AC_MSG_ERROR([invalid value for --with-guile])
  ;;
*)
  # A space separate list of guile versions to try, in order.
  if test "${pkg_config_prog_path}" = "missing"; then
    AC_MSG_ERROR([pkg-config not found])
  fi
  AC_TRY_LIBGUILE(${pkg_config_prog_path}, ${with_guile}, yes, have_libguile)
  ;;
esac

if test "${have_libguile}" != no; then
  dnl Get the name of the 'guild' program.
  case "${with_guile}" in
  [[\\/]]* | ?:[[\\/]]*)
    GDB_GUILE_PROGRAM_NAMES(["${with_guile}"], ["${guile_version}"])
    ;;
  *)
    GDB_GUILE_PROGRAM_NAMES(["${pkg_config_prog_path}"], ["${guile_version}"])
    ;;
  esac

  dnl Make sure guild can handle this host.
  GDB_TRY_GUILD([$srcdir/guile/lib/gdb/support.scm])
  dnl If not, disable guile support.
  if test "$ac_cv_guild_ok" = no; then
    have_libguile=no
    AC_MSG_WARN(disabling guile support, $GUILD fails compiling for $host)
  fi
fi

if test "${have_libguile}" != no; then
  AC_DEFINE(HAVE_GUILE, 1, [Define if Guile interpreter is being linked in.])
  CONFIG_OBS="$CONFIG_OBS \$(SUBDIR_GUILE_OBS)"
  CONFIG_DEPS="$CONFIG_DEPS \$(SUBDIR_GUILE_DEPS)"
  CONFIG_SRCS="$CONFIG_SRCS \$(SUBDIR_GUILE_SRCS)"
  CONFIG_INSTALL="$CONFIG_INSTALL install-guile"
  ENABLE_CFLAGS="$ENABLE_CFLAGS \$(SUBDIR_GUILE_CFLAGS)"

  dnl The 'scm_new_smob' function appeared in Guile 2.0.6.
  save_LIBS="$LIBS"
  save_CPPFLAGS="$CPPFLAGS"
  LIBS="$GUILE_LIBS"
  CPPFLAGS="$GUILE_CPPFLAGS"
  AC_CHECK_FUNCS([scm_new_smob])
  LIBS="$save_LIBS"
  CPPFLAGS="$save_CPPFLAGS"
else
  # Even if Guile support is not compiled in, we need to have these files
  # included.
  CONFIG_OBS="$CONFIG_OBS guile/guile.o"
  CONFIG_SRCS="$CONFIG_SRCS guile/guile.c"
fi
AC_SUBST(GUILE_CPPFLAGS)
AC_SUBST(GUILE_LIBS)
AM_CONDITIONAL(HAVE_GUILE, test "${have_libguile}" != no)

# ---------------------------- #
# Check for source highlight.  #
# ---------------------------- #

SRCHIGH_LIBS=
SRCHIGH_CFLAGS=

AC_ARG_ENABLE([source-highlight],
	      [AS_HELP_STRING([--enable-source-highlight],
			      [enable source-highlight for source listings])],
	      [GDB_CHECK_YES_NO_AUTO_VAL([$enableval], [--enable-source-highlight])],
	      [enable_source_highlight=auto])

if test "${enable_source_highlight}" != "no"; then
  AC_MSG_CHECKING([for the source-highlight library])
  if test "${pkg_config_prog_path}" = "missing"; then
    AC_MSG_RESULT([no - pkg-config not found])
    if test "${enable_source_highlight}" = "yes"; then
      AC_MSG_ERROR([pkg-config was not found in your system])
    fi
  else
    if ${pkg_config_prog_path} --exists source-highlight; then
      case "$LDFLAGS" in
        *static-libstdc*)
          AC_MSG_ERROR([source highlight is incompatible with -static-libstdc++; dnl
either use --disable-source-highlight or dnl
--without-static-standard-libraries])
          ;;
      esac

      srchigh_pkg_cflags=`${pkg_config_prog_path} --cflags source-highlight`
      srchigh_pkg_libs=`${pkg_config_prog_path} --libs source-highlight`

      # Now that we have found a source-highlight library, check if we can use
      # it.  In particular, we're trying to detect the situation that the
      # library is using the new libstdc++ library abi ( see
      # https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html )
      # while the compiler being used to compile gdb is using the old abi.
      # Such a situation will result in an undefined reference to
      # srchilite::SourceHighlight::SourceHighlight(std::string const&).
      # This situation can occur for instance when using a source highlight
      # library compiled with g++ 7.5.0 while building gdb with g++ 4.8.5.
      AC_LANG_PUSH(C++)
      save_CXXFLAGS="$CXXFLAGS"
      save_LIBS="$LIBS"
      CXXFLAGS="$CXXFLAGS $srchigh_pkg_cflags"
      LIBS="$LIBS $srchigh_pkg_libs"
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
          [#include <srchilite/sourcehighlight.h>],
          [std::string outlang = "esc.outlang";
           new srchilite::SourceHighlight (outlang);]
        )],
        [have_usable_source_highlight=yes],
        [have_usable_source_highlight=no]
      )
      CXXFLAGS="$save_CXXFLAGS"
      LIBS="$save_LIBS"
      AC_LANG_POP(C++)

      if test "${have_usable_source_highlight}" = "yes"; then
        AC_DEFINE([HAVE_SOURCE_HIGHLIGHT], 1,
                  [Define to 1 if the source-highlight library is available])
        AC_MSG_RESULT([yes])
        SRCHIGH_CFLAGS="$srchigh_pkg_cflags"
        SRCHIGH_LIBS="$srchigh_pkg_libs"
      else
        AC_MSG_RESULT([no])
        if test "${enable_source_highlight}" = "yes"; then
          AC_MSG_ERROR([source-highlight in your system could not be used])
        fi
      fi
    else
      AC_MSG_RESULT([no])
      if test "${enable_source_highlight}" = "yes"; then
        AC_MSG_ERROR([source-highlight was not found in your system])
      fi
    fi
  fi
fi
AC_SUBST(SRCHIGH_LIBS)
AC_SUBST(SRCHIGH_CFLAGS)

# ------------------------- #
# Checks for header files.  #
# ------------------------- #

AC_HEADER_STDC
# elf_hp.h is for HP/UX 64-bit shared library support.
AC_CHECK_HEADERS([nlist.h machine/reg.h \
                  thread_db.h \
		  sys/file.h sys/filio.h sys/ioctl.h sys/param.h \
		  sys/procctl.h sys/resource.h sys/ptrace.h ptrace.h \
		  sys/reg.h sys/debugreg.h \
		  termios.h elf_hp.h])
AC_CHECK_HEADERS(sys/user.h, [], [],
[#if HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
])

AC_CHECK_HEADERS(curses.h cursesX.h ncurses.h ncursesw/ncurses.h ncurses/ncurses.h ncurses/term.h)
AC_CHECK_HEADERS(term.h, [], [],
[#if HAVE_CURSES_H
# include <curses.h>
#endif
])

AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([ws2tcpip.h])
AC_CHECK_HEADERS([execinfo.h])

# ------------------------- #
# Checks for declarations.  #
# ------------------------- #

libiberty_INIT

AC_CHECK_DECLS([snprintf])
AM_LC_MESSAGES

# ------------------ #
# Checks for types.  #
# ------------------ #

AC_CHECK_TYPES(socklen_t, [], [],
[#include <sys/types.h>
#if HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#elif HAVE_WS2TCPIP_H
# include <ws2tcpip.h>
#endif
])

# ------------------------------------- #
# Checks for compiler characteristics.  #
# ------------------------------------- #

AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN

# ------------------------------ #
# Checks for library functions.  #
# ------------------------------ #

AC_CHECK_FUNCS([getuid getgid \
		pread pread64 pwrite resize_term \
		getpgid setsid \
		sigsetmask \
		ttrace wresize setlocale iconvlist libiconvlist btowc \
		setrlimit getrlimit posix_madvise waitpid \
		use_default_colors])
AM_LANGINFO_CODESET

# Check the return and argument types of ptrace.
GDB_AC_PTRACE

dnl AC_FUNC_SETPGRP does not work when cross compiling
dnl Instead, assume we will have a prototype for setpgrp if cross compiling.
if test "$cross_compiling" = no; then
  AC_FUNC_SETPGRP
else
  AC_CACHE_CHECK(
    [whether setpgrp takes no argument],
    [ac_cv_func_setpgrp_void],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
	  [#include <unistd.h>],
	  [if (setpgrp(1,1) == -1)
	     exit (0);
	   else
	     exit (1);]
	)],
       [ac_cv_func_setpgrp_void=no],
       [ac_cv_func_setpgrp_void=yes]
     )]
  )
if test "$ac_cv_func_setpgrp_void" = yes; then
  AC_DEFINE(SETPGRP_VOID, 1)
fi
fi

# Check if <sys/proc.h> defines `struct thread' with a td_pcb member.
AC_CHECK_MEMBERS([struct thread.td_pcb], [], [],
[#include <sys/param.h>
#include <sys/proc.h>
])

# See if <sys/lwp.h> defines `struct lwp`.
AC_CACHE_CHECK(
  [for struct lwp],
  [gdb_cv_struct_lwp],
  [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
	[#include <sys/param.h>
	 #define _KMEMUSER
	 #include <sys/lwp.h>],
	[struct lwp l;]
      )],
     [gdb_cv_struct_lwp=yes],
     [gdb_cv_struct_lwp=no]
   )]
)
if test "$gdb_cv_struct_lwp" = yes; then
  AC_DEFINE(HAVE_STRUCT_LWP, 1,
            [Define to 1 if your system has struct lwp.])
fi

# See if <machine/reg.h> degines `struct reg'.
AC_CACHE_CHECK(
  [for struct reg in machine/reg.h],
  [gdb_cv_struct_reg],
  [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
	[#include <sys/types.h>
	 #include <machine/reg.h>],
	[struct reg r;]
      )],
     [gdb_cv_struct_reg=yes],
     [gdb_cv_struct_reg=no]
   )]
)
if test "$gdb_cv_struct_reg" = yes; then
  AC_DEFINE(HAVE_STRUCT_REG, 1,
            [Define to 1 if your system has struct reg in <machine/reg.h>.])
fi

# See if <machine/reg.h> supports the %fs and %gs i386 segment registers.
# Older i386 BSD's don't have the r_fs and r_gs members of `struct reg'.
AC_CHECK_MEMBERS([struct reg.r_fs, struct reg.r_gs], [], [],
                 [#include <sys/types.h>
#include <machine/reg.h>])

# See if <sys/ptrace.h> provides the PTRACE_GETREGS request.
AC_MSG_CHECKING(for PTRACE_GETREGS)
AC_CACHE_VAL(
  [gdb_cv_have_ptrace_getregs],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([#include <sys/ptrace.h>], [PTRACE_GETREGS;])],
    [gdb_cv_have_ptrace_getregs=yes],
    [gdb_cv_have_ptrace_getregs=no]
   )]
)
AC_MSG_RESULT($gdb_cv_have_ptrace_getregs)
if test "$gdb_cv_have_ptrace_getregs" = yes; then
  AC_DEFINE(HAVE_PTRACE_GETREGS, 1,
  [Define if sys/ptrace.h defines the PTRACE_GETREGS request.])
fi

# See if <sys/ptrace.h> provides the PTRACE_GETFPXREGS request.
AC_MSG_CHECKING(for PTRACE_GETFPXREGS)
AC_CACHE_VAL(
  [gdb_cv_have_ptrace_getfpxregs],
  [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM([#include <sys/ptrace.h>], [PTRACE_GETFPXREGS;])],
     [gdb_cv_have_ptrace_getfpxregs=yes],
     [gdb_cv_have_ptrace_getfpxregs=no]
   )]
)
AC_MSG_RESULT($gdb_cv_have_ptrace_getfpxregs)
if test "$gdb_cv_have_ptrace_getfpxregs" = yes; then
  AC_DEFINE(HAVE_PTRACE_GETFPXREGS, 1,
  [Define if sys/ptrace.h defines the PTRACE_GETFPXREGS request.])
fi

# See if <sys/ptrace.h> provides the PT_GETDBREGS request.
AC_MSG_CHECKING(for PT_GETDBREGS)
AC_CACHE_VAL(
  [gdb_cv_have_pt_getdbregs],
  [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
	[#include <sys/types.h>
	 #include <sys/ptrace.h>],
	[PT_GETDBREGS;]
      )],
     [gdb_cv_have_pt_getdbregs=yes],
     [gdb_cv_have_pt_getdbregs=no]
   )]
)
AC_MSG_RESULT($gdb_cv_have_pt_getdbregs)
if test "$gdb_cv_have_pt_getdbregs" = yes; then
  AC_DEFINE(HAVE_PT_GETDBREGS, 1,
  [Define if sys/ptrace.h defines the PT_GETDBREGS request.])
fi

# See if <sys/ptrace.h> supports LWP names on FreeBSD
# Older FreeBSD versions don't have the pl_tdname member of
# `struct ptrace_lwpinfo'.
AC_CHECK_MEMBERS([struct ptrace_lwpinfo.pl_tdname], [], [],
                 [#include <sys/ptrace.h>])

# See if <sys/ptrace.h> supports syscall fields on FreeBSD.  The
# pl_syscall_code member of `struct ptrace_lwpinfo' was added in
# FreeBSD 10.3.
AC_CHECK_MEMBERS([struct ptrace_lwpinfo.pl_syscall_code], [], [],
                 [#include <sys/ptrace.h>])

# Check if the compiler supports the `long long' type.

AC_CACHE_CHECK([for long long support in compiler], gdb_cv_c_long_long,
               [AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
[[extern long long foo;]],
[[switch (foo & 2) { case 0: return 1; }]])],
                                  gdb_cv_c_long_long=yes,
                                  gdb_cv_c_long_long=no)])
if test "$gdb_cv_c_long_long" != yes; then
  # libdecnumber requires long long.
  AC_MSG_ERROR([Compiler must support long long for GDB.])
fi

# Check if the compiler and runtime support printing decfloats.

AC_CACHE_CHECK([for decfloat support in printf],
               gdb_cv_printf_has_decfloat,
               [AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],
[[char buf[64];
  _Decimal32 d32 = 1.2345df;
  _Decimal64 d64 = 1.2345dd;
  _Decimal128 d128 = 1.2345dl;
  sprintf (buf, "Decimal32: %H\nDecimal64: %D\nDecimal128: %DD", d32, d64, d128);
  return (strcmp ("Decimal32: 1.2345\nDecimal64: 1.2345\nDecimal128: 1.2345", buf));]])],
                              gdb_cv_printf_has_decfloat=yes,
                              gdb_cv_printf_has_decfloat=no,
                              gdb_cv_printf_has_decfloat=no)])
if test "$gdb_cv_printf_has_decfloat" = yes; then
  AC_DEFINE(PRINTF_HAS_DECFLOAT, 1,
            [Define to 1 if the "%H, %D and %DD" formats work to print decfloats.])
fi

# Check if the compiler supports the `long double' type.  We can't use
# AC_C_LONG_DOUBLE because that one does additional checks on the
# constants defined in <float.h> that fail on some systems,
# e.g. FreeBSD/i386 4.7 and OpenBSD/i386 3.6.

AC_CACHE_CHECK([for long double support in compiler], gdb_cv_c_long_double,
               [AC_COMPILE_IFELSE([AC_LANG_SOURCE([[long double foo;]])],
                                  gdb_cv_c_long_double=yes,
                                  gdb_cv_c_long_double=no)])
if test "$gdb_cv_c_long_double" = yes; then
  AC_DEFINE(HAVE_LONG_DOUBLE, 1,
           [Define to 1 if the compiler supports long double.])
fi

# Check if the compiler and runtime support printing long doubles.

AC_CACHE_CHECK([for long double support in printf],
               gdb_cv_printf_has_long_double,
               [AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],
[[char buf[16];
  long double f = 3.141592653;
  sprintf (buf, "%Lg", f);
  return (strncmp ("3.14159", buf, 7));]])],
                              gdb_cv_printf_has_long_double=yes,
                              gdb_cv_printf_has_long_double=no,
                              gdb_cv_printf_has_long_double=no)])
if test "$gdb_cv_printf_has_long_double" = yes; then
  AC_DEFINE(PRINTF_HAS_LONG_DOUBLE, 1,
            [Define to 1 if the "%Lg" format works to print long doubles.])
fi

# Check if the compiler and runtime support scanning long doubles.

AC_CACHE_CHECK([for long double support in scanf],
               gdb_cv_scanf_has_long_double,
               [AC_RUN_IFELSE([AC_LANG_PROGRAM(
[[#include <stdio.h>]],
[[char *buf = "3.141592653";
  long double f = 0;
  sscanf (buf, "%Lg", &f);
  return !(f > 3.14159 && f < 3.14160);]])],
                              gdb_cv_scanf_has_long_double=yes,
                              gdb_cv_scanf_has_long_double=no,
                              gdb_cv_scanf_has_long_double=no)])
if test "$gdb_cv_scanf_has_long_double" = yes; then
  AC_DEFINE(SCANF_HAS_LONG_DOUBLE, 1,
            [Define to 1 if the "%Lg" format works to scan long doubles.])
fi

case ${host_os} in
aix*)
  AC_CACHE_CHECK(
    [for -bbigtoc option], [gdb_cv_bigtoc],
    [SAVE_LDFLAGS=$LDFLAGS

     case $GCC in
       yes) gdb_cv_bigtoc=-Wl,-bbigtoc ;;
       *) gdb_cv_bigtoc=-bbigtoc ;;
     esac

     LDFLAGS=$LDFLAGS\ $gdb_cv_bigtoc
     AC_LINK_IFELSE(
       [AC_LANG_PROGRAM([], [int i;])],
       [],
       [gdb_cv_bigtoc=]
     )
     LDFLAGS="${SAVE_LDFLAGS}"]
  )
  CONFIG_LDFLAGS="${CONFIG_LDFLAGS} ${gdb_cv_bigtoc}"
  ;;
esac

AC_MSG_CHECKING(for the dynamic export flag)
dynamic_list=false
if test "${gdb_native}" = yes; then
   # The dynamically loaded libthread_db needs access to symbols in the gdb
   # executable.  Older GNU ld supports --export-dynamic but --dynamic-list
   # may not be supported there.
   old_LDFLAGS="$LDFLAGS"
   # Older GNU ld supports --export-dynamic but --dynamic-list it does not.
   RDYNAMIC="-Wl,--dynamic-list=${srcdir}/proc-service.list"
   LDFLAGS="$LDFLAGS $RDYNAMIC"
   if test "${have_libpython}" = no; then
     AC_LINK_IFELSE(
       [AC_LANG_PROGRAM([], [])],
       [dynamic_list=true],
       []
     )
   else
     # Workaround http://bugs.python.org/issue4434 where static
     # libpythonX.Y.a would get its symbols required for
     # pythonX.Y/lib-dynload/*.so modules hidden by -Wl,--dynamic-list.
     # Problem does not happen for the recommended libpythonX.Y.so linkage.

     # Note the workaround for Python
     # http://bugs.python.org/issue10112 earlier has removed
     # -export-dynamic from PYTHON_LIBS.  That's exactly what we want
     # here too, as otherwise it'd make this -Wl,--dynamic-list test
     # always pass.
     old_CFLAGS="$CFLAGS"
     CFLAGS="$CFLAGS $PYTHON_CFLAGS"
     old_LIBS="$LIBS"
     LIBS="$LIBS $PYTHON_LIBS"
     old_CPPFLAGS="$CPPFLAGS"
     CPPFLAGS="$CPPFLAGS $PYTHON_CPPFLAGS"
     AC_RUN_IFELSE(
       [AC_LANG_PROGRAM(
         [#include "Python.h"],
         [int err;
          Py_Initialize ();
          err = PyRun_SimpleString ("import ctypes\n");
          Py_Finalize ();
          return err == 0 ? 0 : 1;])],
       [dynamic_list=true], [], [true])
     LIBS="$old_LIBS"
     CFLAGS="$old_CFLAGS"
     CPPFLAGS="$old_CPPFLAGS"
   fi
   LDFLAGS="$old_LDFLAGS"
fi
if $dynamic_list; then
  found="-Wl,--dynamic-list"
  RDYNAMIC='-Wl,--dynamic-list=$(srcdir)/proc-service.list'
else
  found="-rdynamic"
  RDYNAMIC="-rdynamic"
fi
AC_SUBST(RDYNAMIC)
AC_MSG_RESULT($found)

AC_CACHE_CHECK(
  [whether execinfo.h backtrace is available],
  gdb_cv_execinfo_backtrace,
  [AC_LINK_IFELSE(
     [AC_LANG_PROGRAM(
        [
         #include <execinfo.h>
        ],
        [
         int f;
         void *b[[2]];
         f = backtrace (b, 2);
         backtrace_symbols_fd (b, f, 2);
        ])],
   [gdb_cv_execinfo_backtrace=yes],
   [gdb_cv_execinfo_backtrace=no])])
if test "$gdb_cv_execinfo_backtrace" = yes; then
  AC_DEFINE(HAVE_EXECINFO_BACKTRACE, 1,
            [Define to 1 if execinfo.h backtrace functions are available.])
fi

dnl For certain native configurations, we need to check whether thread
dnl support can be built in or not.
dnl
dnl Note that we only want this if we are both native (host == target),
dnl and not doing a canadian cross build (build == host).

if test "${build}" = "${host}" -a "${host}" = "${target}" ; then
   case ${host_os} in
   aix*)
      AC_MSG_CHECKING(for AiX thread debugging library)
      AC_CACHE_VAL(
	[gdb_cv_have_aix_thread_debug],
	[AC_COMPILE_IFELSE(
	   [AC_LANG_PROGRAM(
	      [#include <sys/pthdebug.h>],
	      [#ifndef PTHDB_VERSION_3
	       #error
	       #endif]
	    )],
	   [gdb_cv_have_aix_thread_debug=yes],
	   [gdb_cv_have_aix_thread_debug=no]
	 )]
      )
      AC_MSG_RESULT($gdb_cv_have_aix_thread_debug)
      if test "$gdb_cv_have_aix_thread_debug" = yes; then
         CONFIG_SRCS="${CONFIG_SRCS} aix-thread.c"
         CONFIG_OBS="${CONFIG_OBS} aix-thread.o"
         LIBS="$LIBS -lpthdebug"

         # Older versions of AIX do not provide the declaration for
         # the getthrds function (it appears that it was introduced
         # with AIX 6.x).
         AC_CHECK_DECLS(getthrds, [], [], [[#include <procinfo.h>]])
      fi
      ;;
   esac
   AC_SUBST(CONFIG_LDFLAGS)
fi

dnl See if we have a thread_db header file that has TD_NOTALLOC and
dnl other error codes.
if test "x$ac_cv_header_thread_db_h" = "xyes"; then
  AC_CACHE_CHECK(
    [whether <thread_db.h> has TD_NOTALLOC],
    [gdb_cv_thread_db_h_has_td_notalloc],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
	  [#include <thread_db.h>],
	  [int i = TD_NOTALLOC;]
	)],
       [gdb_cv_thread_db_h_has_td_notalloc=yes],
       [gdb_cv_thread_db_h_has_td_notalloc=no]
     )]
  )

  AC_CACHE_CHECK(
    [whether <thread_db.h> has TD_VERSION],
    [gdb_cv_thread_db_h_has_td_version],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
	  [#include <thread_db.h>],
	  [int i = TD_VERSION;]
	)],
       [gdb_cv_thread_db_h_has_td_version=yes],
       [gdb_cv_thread_db_h_has_td_version=no]
     )]
  )

  AC_CACHE_CHECK(
    [whether <thread_db.h> has TD_NOTLS],
    [gdb_cv_thread_db_h_has_td_notls],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
	  [#include <thread_db.h>],
	  [int i = TD_NOTLS;]
	)],
       [gdb_cv_thread_db_h_has_td_notls=yes],
       [gdb_cv_thread_db_h_has_td_notls=no]
     )]
  )
fi
if test "x$gdb_cv_thread_db_h_has_td_notalloc" = "xyes"; then
  AC_DEFINE(THREAD_DB_HAS_TD_NOTALLOC, 1,
            [Define if <thread_db.h> has the TD_NOTALLOC error code.])
fi
if test "x$gdb_cv_thread_db_h_has_td_version" = "xyes"; then
  AC_DEFINE(THREAD_DB_HAS_TD_VERSION, 1,
            [Define if <thread_db.h> has the TD_VERSION error code.])
fi
if test "x$gdb_cv_thread_db_h_has_td_notls" = "xyes"; then
  AC_DEFINE(THREAD_DB_HAS_TD_NOTLS, 1,
            [Define if <thread_db.h> has the TD_NOTLS error code.])
fi

dnl Set the host's .gdbinit filename.
case $host_os in
  go32* | *djgpp*)
    gdbinit=gdb.ini
    ;;
  *)
    gdbinit=.gdbinit
    ;;
esac
AC_DEFINE_UNQUOTED(GDBINIT,"$gdbinit",[The .gdbinit filename.])

dnl Set the host's .gdbearlyinit filename
AC_DEFINE_UNQUOTED(GDBEARLYINIT,".gdbearlyinit",[The .gdbearlyinit filename.])

dnl Handle optional features that can be enabled.

# Support for --with-sysroot is a copy of GDB_AC_WITH_DIR,
# except that the argument to --with-sysroot is optional.
# --with-sysroot (or --with-sysroot=yes) sets the default sysroot path.
if test "x$with_sysroot" = xyes; then
  with_sysroot="${exec_prefix}/${target_alias}/sys-root"
fi
AC_ARG_WITH(sysroot,
  AS_HELP_STRING([--with-sysroot@<:@=DIR@:>@],
                 [search for usr/lib et al within DIR]),
  [TARGET_SYSTEM_ROOT=$withval], [TARGET_SYSTEM_ROOT=])
AC_DEFINE_DIR(TARGET_SYSTEM_ROOT, TARGET_SYSTEM_ROOT,
              [search for usr/lib et al within DIR])
AC_SUBST(TARGET_SYSTEM_ROOT)
GDB_AC_DEFINE_RELOCATABLE(TARGET_SYSTEM_ROOT, sysroot, ${ac_define_dir})

GDB_AC_WITH_DIR(SYSTEM_GDBINIT, system-gdbinit,
    [automatically load a system-wide gdbinit file],
    [])
GDB_AC_WITH_DIR(SYSTEM_GDBINIT_DIR, system-gdbinit-dir,
    [automatically load system-wide gdbinit files from this directory],
    [])

AM_GDB_COMPILER_TYPE
AM_GDB_WARNINGS
AM_GDB_UBSAN

# In the Cygwin environment, we need some additional flags.
AC_CACHE_CHECK([for cygwin], gdb_cv_os_cygwin,
[AC_EGREP_CPP(^lose$, [
#if defined (__CYGWIN__) || defined (__CYGWIN32__)
lose
#endif],[gdb_cv_os_cygwin=yes],[gdb_cv_os_cygwin=no])])


dnl Figure out which of the many generic ser-*.c files the _host_ supports.
SER_HARDWIRE="ser-base.o ser-unix.o ser-pipe.o ser-tcp.o"
case ${host} in
  *go32* ) SER_HARDWIRE=ser-go32.o ;;
  *djgpp* ) SER_HARDWIRE=ser-go32.o ;;
  *mingw32*) SER_HARDWIRE="ser-base.o ser-tcp.o ser-mingw.o" ;;
  *) SER_HARDWIRE="$SER_HARDWIRE ser-uds.o" ;;
esac
AC_SUBST(SER_HARDWIRE)

# libreadline needs libuser32.a in a cygwin environment
WIN32LIBS=
if test x"$gdb_cv_os_cygwin" = xyes; then
    WIN32LIBS="-luser32"
    case "${target}" in
	*cygwin*) WIN32LIBS="$WIN32LIBS -limagehlp"
	;;
    esac
fi

# The ser-tcp.c module requires sockets.
# Note that WIN32APILIBS is set by GDB_AC_COMMON.
WIN32LIBS="$WIN32LIBS $WIN32APILIBS"

# Add ELF support to GDB, but only if BFD includes ELF support.
GDB_AC_CHECK_BFD([for ELF support in BFD], gdb_cv_var_elf,
                 [bfd_get_elf_phdr_upper_bound (NULL)], elf-bfd.h)
if test "$gdb_cv_var_elf" = yes; then
  CONFIG_OBS="$CONFIG_OBS elfread.o stap-probe.o dtrace-probe.o \
		gcore-elf.o elf-none-tdep.o"
  AC_DEFINE(HAVE_ELF, 1,
	    [Define if ELF support should be included.])
  # -ldl is provided by bfd/Makfile.am (LIBDL) <PLUGINS>.
  if test "$plugins" = "yes"; then
    AC_SEARCH_LIBS(dlopen, dl)
  fi
fi

# Add macho support to GDB, but only if BFD includes it.
GDB_AC_CHECK_BFD([for Mach-O support in BFD], gdb_cv_var_macho,
                 [bfd_mach_o_lookup_command (NULL, 0, NULL)], mach-o.h)
if test "$gdb_cv_var_macho" = yes; then
  CONFIG_OBS="$CONFIG_OBS machoread.o"
fi

# Add any host-specific objects to GDB.
CONFIG_OBS="${CONFIG_OBS} ${gdb_host_obs}"

# If building on ELF, look for lzma support for embedded compressed debug info.
if test "$gdb_cv_var_elf" = yes; then
  AC_ARG_WITH(lzma,
    AS_HELP_STRING([--with-lzma], [support lzma compression (auto/yes/no)]),
    [], [with_lzma=auto])
  AC_MSG_CHECKING([whether to use lzma])
  AC_MSG_RESULT([$with_lzma])

  if test "${with_lzma}" != no; then
    AC_LIB_HAVE_LINKFLAGS([lzma], [], [#include "lzma.h"],
			  [lzma_index_iter iter;
			   lzma_index_iter_init (&iter, 0);
			   lzma_mf_is_supported (LZMA_MF_HC3);])
    if test "$HAVE_LIBLZMA" != yes; then
      if test "$with_lzma" = yes; then
        AC_MSG_ERROR([missing liblzma for --with-lzma])
      fi
    fi
  fi
fi

LIBGUI="../libgui/src/libgui.a"
GUI_CFLAGS_X="-I${srcdir}/../libgui/src"
AC_SUBST(LIBGUI)
AC_SUBST(GUI_CFLAGS_X)

WIN32LDAPP=
AC_SUBST(WIN32LIBS)
AC_SUBST(WIN32LDAPP)

case "${host}" in
*-*-cygwin* | *-*-mingw* )
    configdir="win"
    ;;
*)
    configdir="unix"
    ;;
esac

GDBTKLIBS=
if test "${enable_gdbtk}" = "yes"; then

    # Gdbtk must have an absolute path to srcdir in order to run
    # properly when not installed.
    here=`pwd`
    cd ${srcdir}
    GDBTK_SRC_DIR=`pwd`
    cd $here

    SC_PATH_TCLCONFIG

    # If $no_tk is nonempty, then we can't do Tk, and there is no
    # point to doing Tcl.
    SC_PATH_TKCONFIG

    if test -z "${no_tcl}" -a -z "${no_tk}"; then
	SC_LOAD_TCLCONFIG

        # Check for in-tree tcl
        here=`pwd`
        cd ${srcdir}/..
        topdir=`pwd`
        cd ${here}

        intree="no"
        if test "${TCL_SRC_DIR}" = "${topdir}/tcl"; then
          intree="yes"
        fi

        # Find Tcl private headers
        if test x"${intree}" = xno; then
          CY_AC_TCL_PRIVATE_HEADERS
	  TCL_INCLUDE="${TCL_INCLUDE_SPEC} ${TCL_PRIVATE_INCLUDE}"
          TCL_LIBRARY="${TCL_LIB_SPEC}"
          TCL_DEPS=""
        else
          # If building tcl in the same src tree, private headers
          # are not needed, but we need to be sure to use the right
          # headers library
	  TCL_INCLUDE="-I${TCL_SRC_DIR}/generic"
          TCL_LIBRARY="${TCL_BUILD_LIB_SPEC}"
          TCL_DEPS="../tcl/${configdir}${TCL_LIB_FILE}"
        fi
	AC_SUBST(TCL_INCLUDE)
	AC_SUBST(TCL_LIBRARY)
        AC_SUBST(TCL_DEPS)

	SC_LOAD_TKCONFIG

        # Check for in-tree Tk
        intree="no"
        if test "${TK_SRC_DIR}" = "${topdir}/tk"; then
          intree="yes"
        fi

        # Find Tk private headers
        if test x"${intree}" = xno; then
          CY_AC_TK_PRIVATE_HEADERS
	  TK_INCLUDE="${TK_INCLUDE_SPEC} ${TK_PRIVATE_INCLUDE}"
	  TK_LIBRARY=${TK_LIB_SPEC}
          TK_DEPS=""
        else
          TK_INCLUDE="-I${TK_SRC_DIR}/generic"
          TK_LIBRARY="${TK_BUILD_LIB_SPEC}"
          TK_DEPS="../tk/${configdir}/${TK_LIB_FILE}"
        fi
	AC_SUBST(TK_INCLUDE)
	AC_SUBST(TK_LIBRARY)
	AC_SUBST(TK_DEPS)
	AC_SUBST(TK_XINCLUDES)

        ENABLE_CFLAGS="${ENABLE_CFLAGS} \$(SUBDIR_GDBTK_CFLAGS)"

	# Include some libraries that Tcl and Tk want.
	TCL_LIBS='$(LIBGUI) $(TK) $(TCL) $(X11_LDFLAGS) $(X11_LIBS)'
	# Yes, the ordering seems wrong here.  But it isn't.
	# TK_LIBS is the list of libraries that need to be linked
	# after Tcl/Tk.  Note that this isn't put into LIBS.  If it
	# were in LIBS then any link tests after this point would
	# try to include things like `$(LIBGUI)', which wouldn't work.
	GDBTKLIBS="${TCL_LIBS} ${TK_LIBS}"

        CONFIG_OBS="${CONFIG_OBS} \$(SUBDIR_GDBTK_OBS)"
        CONFIG_DEPS="${CONFIG_DEPS} \$(SUBDIR_GDBTK_DEPS)"
        CONFIG_SRCS="${CONFIG_SRCS} \$(SUBDIR_GDBTK_SRCS)"
	CONFIG_ALL="${CONFIG_ALL} all-gdbtk"
	CONFIG_CLEAN="${CONFIG_CLEAN} clean-gdbtk"
	CONFIG_INSTALL="${CONFIG_INSTALL} install-gdbtk"
	CONFIG_UNINSTALL="${CONFIG_UNINSTALL} uninstall-gdbtk"

	if test x"$gdb_cv_os_cygwin" = xyes; then
	  WIN32LIBS="${WIN32LIBS} -lshell32 -lgdi32 -lcomdlg32 -ladvapi32"
	  WIN32LDAPP="-Wl,--subsystem,console"
	  CONFIG_OBS="${CONFIG_OBS} gdbres.o"
	fi

        AC_CONFIG_SUBDIRS(gdbtk)
    fi
fi

AC_SUBST(X_CFLAGS)
AC_SUBST(X_LDFLAGS)
AC_SUBST(X_LIBS)
AC_SUBST(GDBTKLIBS)
AC_SUBST(GDBTK_CFLAGS)
AC_SUBST(GDBTK_SRC_DIR)

AC_PATH_X

# Unlike the sim directory, whether a simulator is linked is controlled by
# presence of a gdb_sim definition in the target configure.tgt entry.
# This code just checks for a few cases where we'd like to ignore those
# definitions, even when they're present in the '.mt' file.  These cases
# are when --disable-sim is specified, or if the simulator directory is
# not part of the source tree.
#
AC_ARG_ENABLE(sim,
AS_HELP_STRING([--enable-sim], [link gdb with simulator]),
[echo "enable_sim = $enable_sim";
 echo "enableval = ${enableval}";
 case "${enableval}" in
  yes) ignore_sim=false ;;
  no)  ignore_sim=true ;;
  *)   ignore_sim=false ;;
 esac],
[ignore_sim=false])

if test ! -d "${srcdir}/../sim"; then
  ignore_sim=true
fi

SIM=
SIM_OBS=
if test "${ignore_sim}" = "false"; then
  if test x"${gdb_sim}" != x ; then
    SIM="${gdb_sim}"
    SIM_OBS="remote-sim.o"

    # Some tdep code should only be compiled in when the ppc sim is
    # built.  PR sim/13418.
    case $target in
      powerpc*-*-*)
	AC_DEFINE(WITH_PPC_SIM, 1, [Define if the PPC simulator is being linked in.])
	;;
    esac
  fi
fi
AC_SUBST(SIM)
AC_SUBST(SIM_OBS)

AC_SUBST(ENABLE_CFLAGS)
AC_SUBST(PROFILE_CFLAGS)

AC_SUBST(CONFIG_OBS)
AC_SUBST(CONFIG_DEPS)
AC_SUBST(CONFIG_SRCS)
AC_SUBST(CONFIG_ALL)
AC_SUBST(CONFIG_CLEAN)
AC_SUBST(CONFIG_INSTALL)
AC_SUBST(CONFIG_UNINSTALL)

# List of host floatformats.
AC_DEFINE_UNQUOTED(GDB_HOST_FLOAT_FORMAT,$gdb_host_float_format,[Host float floatformat])
AC_DEFINE_UNQUOTED(GDB_HOST_DOUBLE_FORMAT,$gdb_host_double_format,[Host double floatformat])
AC_DEFINE_UNQUOTED(GDB_HOST_LONG_DOUBLE_FORMAT,$gdb_host_long_double_format,[Host long double floatformat])

# target_subdir is used by the testsuite to find the target libraries.
target_subdir=
if test "${host}" != "${target}"; then
    target_subdir="${target_alias}/"
fi
AC_SUBST(target_subdir)

# Import nat definitions.
nat_makefile_frag=/dev/null
if test "${gdb_native}" = "yes"; then
  . ${srcdir}/configure.nat
  nativefile=$NAT_FILE
fi

AC_SUBST(NAT_FILE)
AC_SUBST(NATDEPFILES)
AC_SUBST(NAT_CDEPS)
AC_SUBST(LOADLIBES)
AC_SUBST(MH_CFLAGS)
AC_SUBST(XM_CLIBS)
AC_SUBST(NAT_GENERATED_FILES)
AC_SUBST(HAVE_NATIVE_GCORE_HOST)
AC_SUBST_FILE(nat_makefile_frag)

if test x"${gdb_osabi}" != x ; then
    AC_DEFINE_UNQUOTED(GDB_OSABI_DEFAULT, $gdb_osabi,
		       [Define to the default OS ABI for this configuration.])
fi

# Setup possible use of libbacktrace.
AC_ARG_ENABLE([libbacktrace],
	      [AS_HELP_STRING([--enable-libbacktrace],
			      [use libbacktrace to write a backtrace after a fatal signal.])],
	      [GDB_CHECK_YES_NO_VAL([$enableval], [--enable-libbacktrace])],
	      [enable_libbacktrace=yes])

if test "${enable_libbacktrace}" = "yes"; then
  LIBBACKTRACE_INC="-I$srcdir/../libbacktrace/ -I../libbacktrace/"
  LIBBACKTRACE_LIB=../libbacktrace/libbacktrace.la
  AC_DEFINE(HAVE_LIBBACKTRACE, 1, [Define if libbacktrace is being used.])
else
  LIBBACKTRACE_INC=
  LIBBACKTRACE_LIB=
fi

AC_SUBST(LIBBACKTRACE_INC)
AC_SUBST(LIBBACKTRACE_LIB)

# Check for babeltrace and babeltrace-ctf
AC_ARG_WITH(babeltrace,
  AS_HELP_STRING([--with-babeltrace], [include babeltrace support (auto/yes/no)]),
  [], [with_babeltrace=auto])
AC_MSG_CHECKING([whether to use babeltrace])
AC_MSG_RESULT([$with_babeltrace])

if test "x$with_babeltrace" = "xno"; then
  AC_MSG_WARN([babletrace support disabled; GDB is unable to read CTF data.])
else
  # Append -Werror to CFLAGS so that configure can catch the warning
  # "assignment from incompatible pointer type", which is related to
  # the babeltrace change from 1.0.3 to 1.1.0.  Babeltrace 1.1.0 works
  # in GDB, while babeltrace 1.0.3 is broken.
  # AC_LIB_HAVE_LINKFLAGS may modify CPPFLAGS in it, so it should be
  # safe to save and restore CFLAGS here.
  saved_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS -Werror"
  AC_LIB_HAVE_LINKFLAGS([babeltrace], [babeltrace-ctf],
			[#include <babeltrace/babeltrace.h>
			 #include <babeltrace/ctf/events.h>
			 #include <babeltrace/ctf/iterator.h>],
			[struct bt_iter_pos *pos = bt_iter_get_pos (bt_ctf_get_iter (NULL));
			struct bt_ctf_event *event = NULL;
			const struct bt_definition *scope;

			pos->type = BT_SEEK_BEGIN;
			bt_iter_set_pos (bt_ctf_get_iter (NULL), pos);
			scope = bt_ctf_get_top_level_scope (event,
			      				   BT_STREAM_EVENT_HEADER);
		        bt_ctf_get_uint64 (bt_ctf_get_field (event, scope, "id"));
			])
  CFLAGS=$saved_CFLAGS

  if test "$HAVE_LIBBABELTRACE" != yes; then
     if test "$with_babeltrace" = yes; then
       AC_MSG_ERROR([babeltrace is missing or unusable])
     else
       AC_MSG_WARN([babeltrace is missing or unusable; GDB is unable to read CTF data.])
     fi
  fi
fi

GCC_ENABLE([libctf], [yes], [], [Handle .ctf type-info sections])
if test "${enable_libctf}" = yes; then
  AC_DEFINE(ENABLE_LIBCTF, 1, [Handle .ctf type-info sections])
  LIBCTF="../libctf/libctf.la"
  CTF_DEPS="../libctf/libctf.la"
else
  LIBCTF=
  CTF_DEPS=
fi
AC_SUBST(LIBCTF)
AC_SUBST(CTF_DEPS)

# If nativefile (NAT_FILE) is not set in configure.nat, we link to an
# empty version.

NM_H=
rm -f nm.h
if test "${nativefile}" != ""; then
    case "${nativefile}" in
      nm-*.h ) GDB_NM_FILE="config/${gdb_host_cpu}/${nativefile}" ;;
      * ) GDB_NM_FILE="${nativefile}"
    esac
    AC_CONFIG_LINKS([nm.h:$GDB_NM_FILE], [echo > stamp-nmh],
                    [GDB_NM_FILE=$GDB_NM_FILE])
    AC_DEFINE_UNQUOTED(GDB_NM_FILE, "${GDB_NM_FILE}", [nativefile])
    NM_H=nm.h
fi
AC_SUBST(GDB_NM_FILE)
AC_SUBST(NM_H)

dnl Add dependency for xsltproc if building with maintainer-mode enabled.
AC_PATH_PROGS(XSLTPROC, xsltproc, missing)
if test "x$USE_MAINTAINER_MODE" = xyes; then
  if test "${XSLTPROC}" = missing; then
    AC_MSG_ERROR(unable to find xsltproc.  maintainer-mode requires xsltproc.)
  fi
fi
AC_SUBST(XSLTPROC)

dnl Check for exe extension set on certain hosts (e.g. Win32)
AC_EXEEXT

dnl  Detect the character set used by this host.
dnl  At the moment, we just assume it's UTF-8.
AC_DEFINE(GDB_DEFAULT_HOST_CHARSET, "UTF-8",
          [Define to be a string naming the default host character set.])

GDB_AC_SELFTEST([
  CONFIG_OBS="$CONFIG_OBS \$(SELFTESTS_OBS)"
  CONFIG_SRCS="$CONFIG_SRCS \$(SELFTESTS_SRCS)"
])

GDB_AC_TRANSFORM([gdb], [GDB_TRANSFORM_NAME])
GDB_AC_TRANSFORM([gcore], [GCORE_TRANSFORM_NAME])
AC_CONFIG_FILES([gcore], [chmod +x gcore])
AC_CONFIG_FILES([Makefile gdb-gdb.gdb gdb-gdb.py doc/Makefile data-directory/Makefile])

AC_OUTPUT
