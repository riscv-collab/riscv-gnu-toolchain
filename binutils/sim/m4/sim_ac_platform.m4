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
dnl Check for various platform settings.
AC_DEFUN([SIM_AC_PLATFORM],
[dnl
dnl Check for common headers.
dnl NB: You can assume C11 headers exist.
dnl NB: We use gnulib from ../gnulib/, so we don't probe headers it provides.
AC_CHECK_HEADERS_ONCE(m4_flatten([
  dlfcn.h
  fcntl.h
  fpu_control.h
  termios.h
  utime.h
  linux/if_tun.h
  linux/mii.h
  linux/types.h
  net/if.h
  netinet/in.h
  netinet/tcp.h
  sys/ioctl.h
  sys/mman.h
  sys/mount.h
  sys/param.h
  sys/resource.h
  sys/socket.h
  sys/statfs.h
  sys/termio.h
  sys/termios.h
  sys/types.h
  sys/vfs.h
]))
AC_HEADER_DIRENT

dnl NB: We use gnulib from ../gnulib/, so we don't probe functions it provides.
AC_CHECK_FUNCS_ONCE(m4_flatten([
  __setfpucw
  access
  aint
  anint
  cfgetispeed
  cfgetospeed
  cfsetispeed
  cfsetospeed
  chdir
  chmod
  dup
  dup2
  execv
  execve
  fcntl
  fork
  fstat
  fstatfs
  ftruncate
  getdirentries
  getegid
  geteuid
  getgid
  getpid
  getppid
  getrusage
  gettimeofday
  getuid
  ioctl
  kill
  link
  lseek
  lstat
  mkdir
  mmap
  munmap
  pipe
  posix_fallocate
  pread
  rmdir
  setregid
  setreuid
  setgid
  setuid
  sigaction
  sigprocmask
  sqrt
  stat
  strsignal
  symlink
  tcdrain
  tcflow
  tcflush
  tcgetattr
  tcgetpgrp
  tcsendbreak
  tcsetattr
  tcsetpgrp
  time
  truncate
  umask
  unlink
  utime
]))

AC_STRUCT_ST_BLKSIZE
AC_STRUCT_ST_BLOCKS
AC_STRUCT_ST_RDEV
AC_STRUCT_TIMEZONE

AC_CHECK_MEMBERS([[struct stat.st_dev], [struct stat.st_ino],
[struct stat.st_mode], [struct stat.st_nlink], [struct stat.st_uid],
[struct stat.st_gid], [struct stat.st_rdev], [struct stat.st_size],
[struct stat.st_blksize], [struct stat.st_blocks], [struct stat.st_atime],
[struct stat.st_mtime], [struct stat.st_ctime]], [], [],
[[#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/stat.h>]])

AC_CHECK_TYPES([__int128])
AC_CHECK_TYPES(socklen_t, [], [],
[#include <sys/types.h>
#include <sys/socket.h>
])

AC_CHECK_SIZEOF([void *])

dnl Check for struct statfs.
AC_CACHE_CHECK([for struct statfs],
  [sim_cv_struct_statfs],
  [AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif], [
  struct statfs s;
], [sim_cv_struct_statfs="yes"], [sim_cv_struct_statfs="no"])])
AS_IF([test x"sim_cv_struct_statfs" = x"yes"], [dnl
  AC_DEFINE(HAVE_STRUCT_STATFS, 1,
	    [Define if struct statfs is defined in <sys/mount.h>])
])

dnl Some System V related checks.
AC_CACHE_CHECK([if union semun defined],
  [sim_cv_has_union_semun],
  [AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>], [
  union semun arg;
], [sim_cv_has_union_semun="yes"], [sim_cv_has_union_semun="no"])])
AS_IF([test x"$sim_cv_has_union_semun" = x"yes"], [dnl
  AC_DEFINE(HAVE_UNION_SEMUN, 1,
	    [Define if union semun is defined in <sys/sem.h>])
])

AC_CACHE_CHECK([whether System V semaphores are supported],
  [sim_cv_sysv_sem],
  [AC_TRY_COMPILE([
  #include <sys/types.h>
  #include <sys/ipc.h>
  #include <sys/sem.h>
#ifndef HAVE_UNION_SEMUN
  union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
  };
#endif], [
  union semun arg;
  int id = semget(IPC_PRIVATE, 1, IPC_CREAT|0400);
  if (id == -1)
    return 1;
  arg.val = 0; /* avoid implicit type cast to union */
  if (semctl(id, 0, IPC_RMID, arg) == -1)
    return 1;
], [sim_cv_sysv_sem="yes"], [sim_cv_sysv_sem="no"])])
AS_IF([test x"$sim_cv_sysv_sem" = x"yes"], [dnl
  AC_DEFINE(HAVE_SYSV_SEM, 1, [Define if System V semaphores are supported])
])

AC_CACHE_CHECK([whether System V shared memory is supported],
  [sim_cv_sysv_shm],
  [AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>], [
  int id = shmget(IPC_PRIVATE, 1, IPC_CREAT|0400);
  if (id == -1)
    return 1;
  if (shmctl(id, IPC_RMID, 0) == -1)
    return 1;
], [sim_cv_sysv_shm="yes"], [sim_cv_sysv_shm="no"])])
AS_IF([test x"$sim_cv_sysv_shm" = x"yes"], [dnl
  AC_DEFINE(HAVE_SYSV_SHM, 1, [Define if System V shared memory is supported])
])

dnl Figure out what type of termio/termios support there is
AC_CACHE_CHECK([for struct termios],
  [sim_cv_termios_struct],
  [AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/termios.h>], [
  static struct termios x;
  x.c_iflag = 0;
  x.c_oflag = 0;
  x.c_cflag = 0;
  x.c_lflag = 0;
  x.c_cc[NCCS] = 0;
], [sim_cv_termios_struct="yes"], [sim_cv_termios_struct="no"])])
if test $sim_cv_termios_struct = yes; then
  AC_DEFINE([HAVE_TERMIOS_STRUCTURE], 1, [Define if struct termios exists.])
fi

if test "$sim_cv_termios_struct" = "yes"; then
  AC_CACHE_VAL([sim_cv_termios_cline])
  AC_CHECK_MEMBER(
    [struct termios.c_line],
    [sim_cv_termios_cline="yes"],
    [sim_cv_termios_cline="no"], [
#include <sys/types.h>
#include <sys/termios.h>
])
  if test $sim_cv_termios_cline = yes; then
    AC_DEFINE([HAVE_TERMIOS_CLINE], 1, [Define if struct termios has c_line.])
  fi
else
  sim_cv_termios_cline=no
fi

if test "$sim_cv_termios_struct" != "yes"; then
  AC_CACHE_CHECK([for struct termio],
    [sim_cv_termio_struct],
    [AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/termio.h>], [
  static struct termio x;
  x.c_iflag = 0;
  x.c_oflag = 0;
  x.c_cflag = 0;
  x.c_lflag = 0;
  x.c_cc[NCC] = 0;
], [sim_cv_termio_struct="yes"], [sim_cv_termio_struct="no"])])
  if test $sim_cv_termio_struct = yes; then
    AC_DEFINE([HAVE_TERMIO_STRUCTURE], 1, [Define if struct termio exists.])
  fi
else
  sim_cv_termio_struct=no
fi

if test "$sim_cv_termio_struct" = "yes"; then
  AC_CACHE_VAL([sim_cv_termio_cline])
  AC_CHECK_MEMBER(
    [struct termio.c_line],
    [sim_cv_termio_cline="yes"],
    [sim_cv_termio_cline="no"], [
#include <sys/types.h>
#include <sys/termio.h>
])
  if test $sim_cv_termio_cline = yes; then
    AC_DEFINE([HAVE_TERMIO_CLINE], 1, [Define if struct termio has c_line.])
  fi
else
  sim_cv_termio_cline=no
fi

dnl Types used by common code
AC_TYPE_GETGROUPS
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_TYPE_UID_T

LT_INIT

dnl Libraries.
AC_SEARCH_LIBS([bind], [socket])
AC_SEARCH_LIBS([gethostbyname], [nsl])
AC_SEARCH_LIBS([fabs], [m])
AC_SEARCH_LIBS([log2], [m])

AC_SEARCH_LIBS([dlopen], [dl])
if test "${ac_cv_search_dlopen}" = "none required" || test "${ac_cv_lib_dl_dlopen}" = "yes"; then
  PKG_CHECK_MODULES(SDL, sdl2, [dnl
    SDL_CFLAGS="${SDL_CFLAGS} -DHAVE_SDL=2"
  ], [
    PKG_CHECK_MODULES(SDL, sdl, [dnl
      SDL_CFLAGS="${SDL_CFLAGS} -DHAVE_SDL=1"
    ], [:])
  ])
  dnl If we use SDL, we need dlopen support.
  AS_IF([test -n "$SDL_CFLAGS"], [dnl
    AS_IF([test "$ac_cv_search_dlopen" = no], [dnl
      AC_MSG_WARN([SDL support requires dlopen support])
    ])
  ])
else
  SDL_CFLAGS=
fi
dnl We dlopen the libs at runtime, so never pass down SDL_LIBS.
SDL_LIBS=
AC_SUBST(SDL_CFLAGS)

dnl In the Cygwin environment, we need some additional flags.
AC_CACHE_CHECK([for cygwin], sim_cv_os_cygwin,
[AC_EGREP_CPP(lose, [
#ifdef __CYGWIN__
lose
#endif],[sim_cv_os_cygwin=yes],[sim_cv_os_cygwin=no])])

dnl Keep in sync with gdb's configure.ac list.
AC_SEARCH_LIBS(tgetent, [termcap tinfo curses ncurses],
  [TERMCAP_LIB=$ac_cv_search_tgetent], [TERMCAP_LIB=""])
if test x$sim_cv_os_cygwin = xyes; then
  TERMCAP_LIB="${TERMCAP_LIB} -luser32"
fi
AC_SUBST(TERMCAP_LIB)

dnl We prefer the in-tree readline.  Top-level dependencies make sure
dnl src/readline (if it's there) is configured before src/sim.
if test -r ../readline/Makefile; then
  READLINE_LIB=../readline/readline/libreadline.a
  READLINE_CFLAGS='-I$(READLINE_SRC)/..'
else
  AC_CHECK_LIB(readline, readline, READLINE_LIB=-lreadline,
	       AC_ERROR([the required "readline" library is missing]), $TERMCAP_LIB)
  READLINE_CFLAGS=
fi
AC_SUBST(READLINE_LIB)
AC_SUBST(READLINE_CFLAGS)

dnl Determine whether we have a known getopt prototype in unistd.h
dnl to make sure that we have correct getopt declaration on
dnl include/getopt.h.  The purpose of this is to sync with other Binutils
dnl components and this logic is copied from ld/configure.ac.
AC_MSG_CHECKING(for a known getopt prototype in unistd.h)
AC_CACHE_VAL(sim_cv_decl_getopt_unistd_h,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <unistd.h>], [extern int getopt (int, char *const*, const char *);])],
sim_cv_decl_getopt_unistd_h=yes, sim_cv_decl_getopt_unistd_h=no)])
AC_MSG_RESULT($sim_cv_decl_getopt_unistd_h)
if test $sim_cv_decl_getopt_unistd_h = yes; then
  AC_DEFINE([HAVE_DECL_GETOPT], 1,
	    [Is the prototype for getopt in <unistd.h> in the expected format?])
fi
])
