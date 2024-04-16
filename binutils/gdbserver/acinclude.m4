dnl NB: When possible, try to avoid explicit includes of ../config/ files.
dnl They're normally found by aclocal automatically and recorded in aclocal.m4.
dnl However, some are kept here explicitly to silence harmless warnings from
dnl aclocal when it finds AM_xxx macros via local search paths instead of
dnl system search paths.

dnl gdb/gdbserver/configure.in uses BFD_HAVE_SYS_PROCFS_TYPE.
m4_include(../bfd/bfd.m4)

# This get AM_GDB_COMPILER_TYPE.
m4_include(../gdbsupport/compiler-type.m4)

dnl This gets AM_GDB_WARNINGS.
m4_include(../gdbsupport/warning.m4)

dnl codeset.m4 is needed for common.m4, but not for
dnl anything else in gdbserver.
m4_include(../config/codeset.m4)
m4_include(../gdbsupport/common.m4)

dnl For libiberty_INIT.
m4_include(../gdbsupport/libiberty.m4)

dnl For GDB_AC_PTRACE.
m4_include(../gdbsupport/ptrace.m4)

m4_include(../gdb/ax_cxx_compile_stdcxx.m4)

dnl For GDB_AC_SELFTEST.
m4_include(../gdbsupport/selftest.m4)

dnl Check for existence of a type $1 in libthread_db.h
dnl Based on BFD_HAVE_SYS_PROCFS_TYPE in bfd/bfd.m4.

AC_DEFUN(
  [GDBSERVER_HAVE_THREAD_DB_TYPE],
  [AC_MSG_CHECKING([for $1 in thread_db.h])
   AC_CACHE_VAL(
     [gdbserver_cv_have_thread_db_type_$1],
     [AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([#include <thread_db.h>], [$1 avar])],
	[gdbserver_cv_have_thread_db_type_$1=yes],
	[gdbserver_cv_have_thread_db_type_$1=no]
      )]
   )
   if test $gdbserver_cv_have_thread_db_type_$1 = yes; then
     AC_DEFINE([HAVE_]translit($1, [a-z], [A-Z]), 1,
	       [Define if <thread_db.h> has $1.])
   fi
   AC_MSG_RESULT($gdbserver_cv_have_thread_db_type_$1)]
)
