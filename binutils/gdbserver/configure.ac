dnl Autoconf configure script for GDB server.
dnl Copyright (C) 2000-2024 Free Software Foundation, Inc.
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
AC_CONFIG_MACRO_DIRS([../config])
AC_CONFIG_SRCDIR([server.cc])
AC_CONFIG_HEADERS(config.h:config.in, [echo > stamp-h])

AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CXX
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AM_PROG_INSTALL_STRIP

# Set build, build_cpu, build_vendor and build_os.
AC_CANONICAL_BUILD

# Set host, host_cpu, host_vendor, and host_os.
AC_CANONICAL_HOST

# Set target, target_cpu, target_vendor, and target_os.
AC_CANONICAL_TARGET

AC_PROG_INSTALL
AC_CHECK_TOOL(AR, ar)
AC_PROG_RANLIB

AC_ARG_PROGRAM

# We require a C++17 compiler.  Check if one is available, and if
# necessary, set CXX_DIALECT to some -std=xxx switch.
AX_CXX_COMPILE_STDCXX(17, , mandatory)

AC_HEADER_STDC

GDB_AC_COMMON
# This is set by GDB_AC_COMMON.
AC_SUBST(WIN32APILIBS)

GDB_AC_SELFTEST

ACX_NONCANONICAL_TARGET
ACX_NONCANONICAL_HOST

# Dependency checking.
ZW_CREATE_DEPDIR

dnl Set up for gettext.
ZW_GNU_GETTEXT_SISTER_DIR

# Create sub-directories for objects and dependencies.
CONFIG_SRC_SUBDIR="arch gdbsupport nat target"
AC_SUBST(CONFIG_SRC_SUBDIR)

AC_CONFIG_COMMANDS([gdbdepdir],[
  for subdir in ${CONFIG_SRC_SUBDIR}
  do
      $SHELL $ac_aux_dir/mkinstalldirs $subdir/$DEPDIR
  done],
  [ac_aux_dir=$ac_aux_dir DEPDIR=$DEPDIR CONFIG_SRC_SUBDIR="$CONFIG_SRC_SUBDIR"])

ZW_PROG_COMPILER_DEPENDENCIES([CC])

AC_CHECK_HEADERS(termios.h sys/reg.h string.h dnl
		 sys/procfs.h linux/elf.h dnl
		 fcntl.h signal.h sys/file.h dnl
		 sys/ioctl.h netinet/in.h sys/socket.h netdb.h dnl
		 netinet/tcp.h arpa/inet.h ws2tcpip.h)
AC_FUNC_FORK
AC_CHECK_FUNCS(pread pwrite pread64)

# Check the return and argument types of ptrace.
GDB_AC_PTRACE

# Check for UST
ustlibs=""
ustinc=""

AC_ARG_WITH(ust, [  --with-ust=PATH       Specify prefix directory for the installed UST package
                          Equivalent to --with-ust-include=PATH/include
                          plus --with-ust-lib=PATH/lib])
AC_ARG_WITH(ust_include, [  --with-ust-include=PATH Specify directory for installed UST include files])
AC_ARG_WITH(ust_lib, [  --with-ust-lib=PATH   Specify the directory for the installed UST library])

case $with_ust in
  no)
    ustlibs=
    ustinc=
    ;;
  "" | yes)
    ustlibs=" -lust "
    ustinc=""
    ;;
  *)
    ustlibs="-L$with_ust/lib -lust"
    ustinc="-I$with_ust/include "
    ;;
esac
if test "x$with_ust_include" != x; then
  ustinc="-I$with_ust_include "
fi
if test "x$with_ust_lib" != x; then
  ustlibs="-L$with_ust_lib -lust"
fi

if test "x$with_ust" != "xno"; then
  saved_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $ustinc"
  AC_MSG_CHECKING([for ust])
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
       [
#define CONFIG_UST_GDB_INTEGRATION
#include <ust/ust.h>
       ],
       []
     )],
    [AC_MSG_RESULT([yes]);
     AC_DEFINE(HAVE_UST, 1, [Define if UST is available])],
    [AC_MSG_RESULT([no])
     ustlibs= ; ustinc= ]
  )
  CFLAGS="$saved_CFLAGS"
fi

# Flags needed for UST
AC_SUBST(ustlibs)
AC_SUBST(ustinc)

AM_GDB_COMPILER_TYPE
AM_GDB_WARNINGS

dnl dladdr is glibc-specific.  It is used by thread-db.c but only for
dnl debugging messages.  It lives in -ldl which is handled below so we don't
dnl use AC_CHECK_LIB (or AC_SEARCH_LIBS) here.  Instead we just temporarily
dnl augment LIBS.
old_LIBS="$LIBS"
LIBS="$LIBS -ldl"
AC_CHECK_FUNCS(dladdr)
LIBS="$old_LIBS"

libiberty_INIT

AC_CHECK_DECLS([perror, vasprintf, vsnprintf])

AC_CHECK_TYPES(socklen_t, [], [],
[#include <sys/types.h>
#if HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#elif HAVE_WS2TCPIP_H
# include <ws2tcpip.h>
#endif
])

case "${target}" in
  *-android*)
    # Starting with NDK version 9, <elf.h> actually includes definitions
    # of Elf32_auxv_t and Elf64_auxv_t. But sadly, <elf.h> includes
    # <sys/exec_elf.h> which defines some of the ELF types incorrectly,
    # leading to conflicts with the defintions from <linux/elf.h>.
    # This makes it impossible for us to include both <elf.h> and
    # <linux/elf.h>, which means that, in practice, we do not have
    # access to Elf32_auxv_t and Elf64_auxv_t on this platform.
    # Therefore, do not try to auto-detect availability, as it would
    # get it wrong on this platform.
    ;;
  *)
    AC_CHECK_TYPES([Elf32_auxv_t, Elf64_auxv_t], [], [],
    #include <elf.h>
    )
esac

ACX_PKGVERSION([GDB])
ACX_BUGURL([http://www.gnu.org/software/gdb/bugs/])
AC_DEFINE_UNQUOTED([PKGVERSION], ["$PKGVERSION"], [Additional package description])
AC_DEFINE_UNQUOTED([REPORT_BUGS_TO], ["$REPORT_BUGS_TO"], [Bug reporting address])

# Check for various supplementary target information (beyond the
# triplet) which might affect the choices in configure.srv.
case "${target}" in
changequote(,)dnl
  i[34567]86-*-linux*)
changequote([,])dnl
    AC_CACHE_CHECK([if building for x86-64], [gdb_cv_i386_is_x86_64],
      	           [save_CPPFLAGS="$CPPFLAGS"
                    CPPFLAGS="$CPPFLAGS $CFLAGS"
                    AC_EGREP_CPP([got it], [
#if __x86_64__
got it
#endif
                 ], [gdb_cv_i386_is_x86_64=yes],
                    [gdb_cv_i386_is_x86_64=no])
                    CPPFLAGS="$save_CPPFLAGS"])
    ;;

  x86_64-*-linux*)
    AC_CACHE_CHECK([if building for x32], [gdb_cv_x86_is_x32],
		   [save_CPPFLAGS="$CPPFLAGS"
		    CPPFLAGS="$CPPFLAGS $CFLAGS"
		    AC_EGREP_CPP([got it], [
#if __x86_64__ && __ILP32__
got it
#endif
		 ], [gdb_cv_x86_is_x32=yes],
		    [gdb_cv_x86_is_x32=no])
		    CPPFLAGS="$save_CPPFLAGS"])
    ;;

  m68k-*-*)
    AC_CACHE_CHECK([if building for Coldfire], [gdb_cv_m68k_is_coldfire],
      	           [save_CPPFLAGS="$CPPFLAGS"
                    CPPFLAGS="$CPPFLAGS $CFLAGS"
                    AC_EGREP_CPP([got it], [
#ifdef __mcoldfire__
got it
#endif
                 ], [gdb_cv_m68k_is_coldfire=yes],
                    [gdb_cv_m68k_is_coldfire=no])
                    CPPFLAGS="$save_CPPFLAGS"])
    ;;
esac

. ${srcdir}/configure.srv

if test "${srv_mingwce}" = "yes"; then
  LIBS="$LIBS -lws2"
elif test "${srv_mingw}" = "yes"; then
  # WIN32APILIBS is set by GDB_AC_COMMON.
  LIBS="$LIBS $WIN32APILIBS"
fi

if test "${srv_linux_usrregs}" = "yes"; then
  AC_DEFINE(HAVE_LINUX_USRREGS, 1,
	    [Define if the target supports PTRACE_PEEKUSR for register ]
	    [access.])
fi

if test "${srv_linux_regsets}" = "yes"; then
  AC_DEFINE(HAVE_LINUX_REGSETS, 1,
	    [Define if the target supports register sets.])

  AC_MSG_CHECKING(for PTRACE_GETREGS)
  AC_CACHE_VAL(
    [gdbsrv_cv_have_ptrace_getregs],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([#include <sys/ptrace.h>], [PTRACE_GETREGS;])],
       [gdbsrv_cv_have_ptrace_getregs=yes],
       [gdbsrv_cv_have_ptrace_getregs=no]
     )]
  )
  AC_MSG_RESULT($gdbsrv_cv_have_ptrace_getregs)
  if test "${gdbsrv_cv_have_ptrace_getregs}" = "yes"; then
    AC_DEFINE(HAVE_PTRACE_GETREGS, 1,
	      [Define if the target supports PTRACE_GETREGS for register ]
	      [access.])
  fi

  AC_MSG_CHECKING(for PTRACE_GETFPXREGS)
  AC_CACHE_VAL(
    [gdbsrv_cv_have_ptrace_getfpxregs],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([#include <sys/ptrace.h>], [PTRACE_GETFPXREGS;])],
       [gdbsrv_cv_have_ptrace_getfpxregs=yes],
       [gdbsrv_cv_have_ptrace_getfpxregs=no]
     )]
  )
  AC_MSG_RESULT($gdbsrv_cv_have_ptrace_getfpxregs)
  if test "${gdbsrv_cv_have_ptrace_getfpxregs}" = "yes"; then
    AC_DEFINE(HAVE_PTRACE_GETFPXREGS, 1,
	      [Define if the target supports PTRACE_GETFPXREGS for extended ]
	      [register access.])
  fi
fi

if test "${srv_linux_btrace}" = "yes"; then
  AC_DEFINE(HAVE_LINUX_BTRACE, 1,
	    [Define if the target supports branch tracing.])
fi

dnl Some systems (e.g., Android) have lwpid_t defined in libthread_db.h.
if test "$bfd_cv_have_sys_procfs_type_lwpid_t" != yes; then
  GDBSERVER_HAVE_THREAD_DB_TYPE(lwpid_t)
fi

dnl Some systems (e.g., Android) have psaddr_t defined in libthread_db.h.
if test "$bfd_cv_have_sys_procfs_type_psaddr_t" != yes; then
  GDBSERVER_HAVE_THREAD_DB_TYPE(psaddr_t)
fi

dnl Check for libdl, but do not add it to LIBS as only gdbserver
dnl needs it (and gdbreplay doesn't).
old_LIBS="$LIBS"
AC_CHECK_LIB(dl, dlopen)
LIBS="$old_LIBS"

srv_thread_depfiles=
srv_libs=

if test "$srv_linux_thread_db" = "yes"; then
  if test "$ac_cv_lib_dl_dlopen" = "yes"; then
    srv_libs="-ldl"
    AC_MSG_CHECKING(for the dynamic export flag)
    old_LDFLAGS="$LDFLAGS"
    # Older GNU ld supports --export-dynamic but --dynamic-list may not be
    # supported there.
    RDYNAMIC="-Wl,--dynamic-list=${srcdir}/proc-service.list"
    LDFLAGS="$LDFLAGS $RDYNAMIC"
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([],[])],
      [found="-Wl,--dynamic-list"
       RDYNAMIC='-Wl,--dynamic-list=$(srcdir)/proc-service.list'],
      [RDYNAMIC="-rdynamic"
       LDFLAGS="$old_LDFLAGS $RDYNAMIC"
       AC_LINK_IFELSE(
	 [AC_LANG_PROGRAM([], [])],
	 [found="-rdynamic"],
	 [found="no"
	  RDYNAMIC=""]
       )]
    )
    AC_SUBST(RDYNAMIC)
    LDFLAGS="$old_LDFLAGS"
    AC_MSG_RESULT($found)
  else
    srv_libs="-lthread_db"
  fi

  srv_thread_depfiles="thread-db.o proc-service.o"
  AC_DEFINE(USE_THREAD_DB, 1, [Define if we should use libthread_db.])
  AC_CACHE_CHECK(
    [for TD_VERSION],
    [gdbsrv_cv_have_td_version],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([#include <thread_db.h>], [TD_VERSION;])],
       [gdbsrv_cv_have_td_version=yes],
       [gdbsrv_cv_have_td_version=no]
     )]
  )
  if test "$gdbsrv_cv_have_td_version" = yes; then
    AC_DEFINE(HAVE_TD_VERSION, 1, [Define if TD_VERSION is available.])
  fi
fi

AC_ARG_WITH(libthread-db,
AS_HELP_STRING([--with-libthread-db=PATH], [use given libthread_db directly]),
[srv_libthread_db_path="${withval}"
  srv_libs="$srv_libthread_db_path"
])

if test "$srv_libs" != "" -a "$srv_libs" != "-ldl"; then
  AC_DEFINE(USE_LIBTHREAD_DB_DIRECTLY, 1, [Define if we should use libthread_db directly.])
fi

if test "$srv_xmlfiles" != ""; then
  srv_xmlbuiltin="xml-builtin.o"
  AC_DEFINE(USE_XML, 1, [Define if an XML target description is available.])

  tmp_xmlfiles=$srv_xmlfiles
  srv_xmlfiles=""
  for f in $tmp_xmlfiles; do
    srv_xmlfiles="$srv_xmlfiles \$(XML_DIR)/$f"
  done
fi

GDBSERVER_DEPFILES="$srv_regobj $srv_tgtobj $srv_thread_depfiles"
GDBSERVER_LIBS="$srv_libs"

dnl Check whether the target supports __sync_*_compare_and_swap.
AC_CACHE_CHECK(
  [whether the target supports __sync_*_compare_and_swap],
  [gdbsrv_cv_have_sync_builtins],
  [AC_LINK_IFELSE(
     [AC_LANG_PROGRAM(
	[],
	[int foo, bar;
	 bar = __sync_val_compare_and_swap(&foo, 0, 1);]
      )],
     [gdbsrv_cv_have_sync_builtins=yes],
     [gdbsrv_cv_have_sync_builtins=no]
   )]
)
if test "$gdbsrv_cv_have_sync_builtins" = yes; then
  AC_DEFINE(HAVE_SYNC_BUILTINS, 1,
    [Define to 1 if the target supports __sync_*_compare_and_swap])
fi

dnl Check for -fvisibility=hidden support in the compiler.
saved_cflags="$CFLAGS"
CFLAGS="$CFLAGS -fvisibility=hidden"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
		        [gdbsrv_cv_have_visibility_hidden=yes],
	        	[gdbsrv_cv_have_visibility_hidden=no])
CFLAGS="$saved_cflags"

IPA_DEPFILES=""
extra_libraries=""

# check whether to enable the inprocess agent
if test "$ipa_obj" != "" \
   -a "$gdbsrv_cv_have_sync_builtins" = yes \
   -a "$gdbsrv_cv_have_visibility_hidden" = yes; then
   have_ipa=true
else
   have_ipa=false
fi

AC_ARG_ENABLE(inprocess-agent,
AS_HELP_STRING([--enable-inprocess-agent], [inprocess agent]),
[case "$enableval" in
  yes) want_ipa=true ;;
  no) want_ipa=false ;;
  *) AC_MSG_ERROR([bad value $enableval for inprocess-agent]) ;;
esac],
[want_ipa=$have_ipa])

if $want_ipa ; then
   if $have_ipa ; then
     IPA_DEPFILES="$ipa_obj"
     extra_libraries="$extra_libraries libinproctrace.so"
   else
     AC_MSG_ERROR([inprocess agent not supported for this target])
   fi
fi

AC_SUBST(GDBSERVER_DEPFILES)
AC_SUBST(GDBSERVER_LIBS)
AC_SUBST(srv_xmlbuiltin)
AC_SUBST(srv_xmlfiles)
AC_SUBST(IPA_DEPFILES)
AC_SUBST(extra_libraries)

GNULIB=../gnulib/import

GNULIB_STDINT_H=
if test x"$STDINT_H" != x; then
  GNULIB_STDINT_H=$GNULIB/$STDINT_H
fi
AC_SUBST(GNULIB_STDINT_H)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
