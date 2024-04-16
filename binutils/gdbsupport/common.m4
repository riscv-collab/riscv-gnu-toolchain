dnl Autoconf configure snippets for common.
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

dnl Invoke configury needed by the files in 'common'.
AC_DEFUN([GDB_AC_COMMON], [
  # Set the 'development' global.
  . $srcdir/../bfd/development.sh

  AC_HEADER_STDC
  AC_FUNC_ALLOCA

  WIN32APILIBS=
  case ${host} in
    *mingw32*)
      AC_DEFINE(USE_WIN32API, 1,
		[Define if we should use the Windows API, instead of the
		 POSIX API.  On Windows, we use the Windows API when
		 building for MinGW, but the POSIX API when building
		 for Cygwin.])
      WIN32APILIBS="-lws2_32"
      ;;
  esac

  dnl Note that this requires codeset.m4, which is included
  dnl by the users of common.m4.
  AM_LANGINFO_CODESET

  AC_CHECK_HEADERS(linux/perf_event.h locale.h memory.h signal.h dnl
		   sys/resource.h sys/socket.h dnl
		   sys/un.h sys/wait.h dnl
		   thread_db.h wait.h dnl
		   termios.h dnl
		   dlfcn.h dnl
		   linux/elf.h proc_service.h dnl
		   poll.h sys/poll.h sys/select.h)

  AC_FUNC_MMAP
  AC_FUNC_FORK
  # Some systems (e.g. Solaris) have `socketpair' in libsocket.
  AC_SEARCH_LIBS(socketpair, socket)
  AC_CHECK_FUNCS([fdwalk getrlimit pipe pipe2 poll socketpair sigaction \
		  ptrace64 sbrk setns sigaltstack sigprocmask \
		  setpgid setpgrp getrusage getauxval sigtimedwait])

  # This is needed for RHEL 5 and uclibc-ng < 1.0.39.
  # These did not define ADDR_NO_RANDOMIZE in sys/personality.h,
  # only in linux/personality.h.
  AC_CHECK_DECLS([ADDR_NO_RANDOMIZE],,, [#include <sys/personality.h>])

  AC_CHECK_DECLS([strstr])

  # ----------------------- #
  # Checks for structures.  #
  # ----------------------- #

  AC_CHECK_MEMBERS([struct stat.st_blocks, struct stat.st_blksize])

  # On FreeBSD we need libutil for the kinfo_get* functions.  On
  # GNU/kFreeBSD systems, FreeBSD libutil is renamed to libutil-freebsd.
  # Figure out which one to use.
  AC_SEARCH_LIBS(kinfo_getfile, util util-freebsd)

  # Define HAVE_KINFO_GETFILE if kinfo_getfile is available.
  AC_CHECK_FUNCS(kinfo_getfile)

  # ----------------------- #
  # Check for threading.    #
  # ----------------------- #

  AC_ARG_ENABLE(threading,
    AS_HELP_STRING([--enable-threading], [include support for parallel processing of data (yes/no)]),
    [case "$enableval" in
    yes) want_threading=yes ;;
    no) want_threading=no ;;
    *) AC_MSG_ERROR([bad value $enableval for threading]) ;;
    esac],
    [want_threading=yes])

  # Check for std::thread.  This does not work on some platforms, like
  # mingw and DJGPP.
  AC_LANG_PUSH([C++])
  AX_PTHREAD([threads=yes], [threads=no])
  save_LIBS="$LIBS"
  LIBS="$PTHREAD_LIBS $LIBS"
  save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$PTHREAD_CFLAGS $save_CXXFLAGS"
  AC_CACHE_CHECK([for std::thread],
		 gdb_cv_cxx_std_thread,
		 [AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
  dnl NOTE: this must be kept in sync with common-defs.h.
  [[#if defined (__MINGW32__) || defined (__CYGWIN__)
    # ifdef _WIN32_WINNT
    #  if _WIN32_WINNT < 0x0501
    #   undef _WIN32_WINNT
    #   define _WIN32_WINNT 0x0501
    #  endif
    # else
    #  define _WIN32_WINNT 0x0501
    # endif
    #endif	/* __MINGW32__ || __CYGWIN__ */
    #include <thread>
    void callback() { }]],
  [[std::thread t(callback);]])],
				gdb_cv_cxx_std_thread=yes,
				gdb_cv_cxx_std_thread=no)])

  if test "$threads" = "yes"; then
    # This check must be here, while LIBS includes any necessary
    # threading library.
    AC_CHECK_FUNCS([pthread_sigmask pthread_setname_np])
  fi
  LIBS="$save_LIBS"
  CXXFLAGS="$save_CXXFLAGS"

  if test "$want_threading" = "yes"; then
    if test "$gdb_cv_cxx_std_thread" = "yes"; then
      AC_DEFINE(CXX_STD_THREAD, 1,
		[Define to 1 if std::thread works.])
    fi
  fi
  AC_LANG_POP

  dnl Check if sigsetjmp is available.  Using AC_CHECK_FUNCS won't
  dnl do since sigsetjmp might only be defined as a macro.
  AC_CACHE_CHECK(
    [for sigsetjmp],
    [gdb_cv_func_sigsetjmp],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
          [#include <setjmp.h>],
          [sigjmp_buf env;
           while (! sigsetjmp (env, 1))
             siglongjmp (env, 1);]
        )],
       [gdb_cv_func_sigsetjmp=yes],
       [gdb_cv_func_sigsetjmp=no]
     )]
  )
  if test "$gdb_cv_func_sigsetjmp" = "yes"; then
    AC_DEFINE(HAVE_SIGSETJMP, 1, [Define if sigsetjmp is available. ])
  fi

  AC_ARG_WITH(intel_pt,
    AS_HELP_STRING([--with-intel-pt], [include Intel Processor Trace support (auto/yes/no)]),
    [], [with_intel_pt=auto])
  AC_MSG_CHECKING([whether to use intel pt])
  AC_MSG_RESULT([$with_intel_pt])

  if test "${with_intel_pt}" = no; then
    AC_MSG_WARN([Intel Processor Trace support disabled; some features may be unavailable.])
    HAVE_LIBIPT=no
  else
    AC_PREPROC_IFELSE([AC_LANG_SOURCE([[
  #include <linux/perf_event.h>
  #ifndef PERF_ATTR_SIZE_VER5
  # error
  #endif
    ]])], [perf_event=yes], [perf_event=no])
    if test "$perf_event" != yes; then
      if test "$with_intel_pt" = yes; then
	AC_MSG_ERROR([linux/perf_event.h missing or too old])
      else
	AC_MSG_WARN([linux/perf_event.h missing or too old; some features may be unavailable.])
      fi
    fi

    AC_LIB_HAVE_LINKFLAGS([ipt], [], [#include "intel-pt.h"], [pt_insn_alloc_decoder (0);])
    if test "$HAVE_LIBIPT" != yes; then
      if test "$with_intel_pt" = yes; then
	AC_MSG_ERROR([libipt is missing or unusable])
      else
	AC_MSG_WARN([libipt is missing or unusable; some features may be unavailable.])
      fi
    else
      save_LIBS=$LIBS
      LIBS="$LIBS $LIBIPT"
      AC_CHECK_FUNCS(pt_insn_event)
      AC_CHECK_MEMBERS([struct pt_insn.enabled, struct pt_insn.resynced], [], [],
		       [#include <intel-pt.h>])
      LIBS=$save_LIBS
    fi
  fi

  # Check if the compiler and runtime support printing long longs.

  AC_CACHE_CHECK([for long long support in printf],
		 gdb_cv_printf_has_long_long,
		 [AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],
  [[char buf[32];
    long long l = 0;
    l = (l << 16) + 0x0123;
    l = (l << 16) + 0x4567;
    l = (l << 16) + 0x89ab;
    l = (l << 16) + 0xcdef;
    sprintf (buf, "0x%016llx", l);
    return (strcmp ("0x0123456789abcdef", buf));]])],
				gdb_cv_printf_has_long_long=yes,
				gdb_cv_printf_has_long_long=no,
				gdb_cv_printf_has_long_long=no)])
  if test "$gdb_cv_printf_has_long_long" = yes; then
    AC_DEFINE(PRINTF_HAS_LONG_LONG, 1,
	      [Define to 1 if the "%ll" format works to print long longs.])
  fi

  BFD_SYS_PROCFS_H
  if test "$ac_cv_header_sys_procfs_h" = yes; then
    BFD_HAVE_SYS_PROCFS_TYPE(gregset_t)
    BFD_HAVE_SYS_PROCFS_TYPE(fpregset_t)
    BFD_HAVE_SYS_PROCFS_TYPE(prgregset_t)
    BFD_HAVE_SYS_PROCFS_TYPE(prfpregset_t)
    BFD_HAVE_SYS_PROCFS_TYPE(prgregset32_t)
    BFD_HAVE_SYS_PROCFS_TYPE(lwpid_t)
    BFD_HAVE_SYS_PROCFS_TYPE(psaddr_t)
    BFD_HAVE_SYS_PROCFS_TYPE(elf_fpregset_t)
  fi

  dnl xxhash support
  # Check for xxhash
  AC_ARG_WITH(xxhash,
    AS_HELP_STRING([--with-xxhash], [use libxxhash for hashing (faster) (auto/yes/no)]),
    [], [with_xxhash=auto])

  if test "x$with_xxhash" != "xno"; then
    AC_LIB_HAVE_LINKFLAGS([xxhash], [],
			  [#include <xxhash.h>],
			  [XXH32("foo", 3, 0);
			  ])
    if test "$HAVE_LIBXXHASH" != yes; then
      if test "$with_xxhash" = yes; then
	AC_MSG_ERROR([xxhash is missing or unusable])
      fi
    fi
    if test "x$with_xxhash" = "xauto"; then
      with_xxhash="$HAVE_LIBXXHASH"
    fi
  fi

  AC_MSG_CHECKING([whether to use xxhash])
  AC_MSG_RESULT([$with_xxhash])
])

dnl Check that the provided value ($1) is either "yes" or "no".  If not,
dnl emit an error message mentionning the configure option $2, and abort
dnl the script.
AC_DEFUN([GDB_CHECK_YES_NO_VAL],
	 [
	   case $1 in
	     yes | no)
	       ;;
	     *)
	       AC_MSG_ERROR([bad value $1 for $2])
	       ;;
	   esac
	  ])

dnl Check that the provided value ($1) is either "yes", "no" or "auto".  If not,
dnl emit an error message mentionning the configure option $2, and abort
dnl the script.
AC_DEFUN([GDB_CHECK_YES_NO_AUTO_VAL],
	 [
	   case $1 in
	     yes | no | auto)
	       ;;
	     *)
	       AC_MSG_ERROR([bad value $1 for $2])
	       ;;
	   esac
	  ])
