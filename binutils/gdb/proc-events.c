/* Machine-independent support for Solaris /proc (process file system)

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

   Written by Michael Snyder at Cygnus Solutions.
   Based on work by Fred Fish, Stu Grossman, Geoff Noer, and others.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Pretty-print "events of interest".

   This module includes pretty-print routines for:
   * faults (hardware exceptions)
   * signals (software interrupts)
   * syscalls

   FIXME: At present, the syscall translation table must be
   initialized, which is not true of the other translation tables.  */

#include "defs.h"

#include <sys/types.h>
#include <sys/procfs.h>
#include <sys/syscall.h>
#include <sys/fault.h>

#include "proc-utils.h"

/* Much of the information used in the /proc interface, particularly
   for printing status information, is kept as tables of structures of
   the following form.  These tables can be used to map numeric values
   to their symbolic names and to a string that describes their
   specific use.  */

struct trans
{
  int value;                    /* The numeric value.  */
  const char *name;             /* The equivalent symbolic value.  */
  const char *desc;             /* Short description of value.  */
};


/* Pretty print syscalls.  */

/* Syscall translation table.  */

#define MAX_SYSCALLS 262	/* Pretty arbitrary.  */
static const char *syscall_table[MAX_SYSCALLS];

static void
init_syscall_table (void)
{
  syscall_table[SYS_accept] = "accept";
#ifdef SYS_access
  syscall_table[SYS_access] = "access";
#endif
  syscall_table[SYS_acct] = "acct";
  syscall_table[SYS_acctctl] = "acctctl";
  syscall_table[SYS_acl] = "acl";
#ifdef SYS_adi
  syscall_table[SYS_adi] = "adi";
#endif
  syscall_table[SYS_adjtime] = "adjtime";
  syscall_table[SYS_alarm] = "alarm";
  syscall_table[SYS_auditsys] = "auditsys";
  syscall_table[SYS_autofssys] = "autofssys";
  syscall_table[SYS_bind] = "bind";
  syscall_table[SYS_brand] = "brand";
  syscall_table[SYS_brk] = "brk";
  syscall_table[SYS_chdir] = "chdir";
#ifdef SYS_chmod
  syscall_table[SYS_chmod] = "chmod";
#endif
#ifdef SYS_chown
  syscall_table[SYS_chown] = "chown";
#endif
  syscall_table[SYS_chroot] = "chroot";
  syscall_table[SYS_cladm] = "cladm";
  syscall_table[SYS_clock_getres] = "clock_getres";
  syscall_table[SYS_clock_gettime] = "clock_gettime";
  syscall_table[SYS_clock_settime] = "clock_settime";
  syscall_table[SYS_close] = "close";
  syscall_table[SYS_connect] = "connect";
  syscall_table[SYS_context] = "context";
  syscall_table[SYS_corectl] = "corectl";
  syscall_table[SYS_cpc] = "cpc";
#ifdef SYS_creat
  syscall_table[SYS_creat] = "creat";
#endif
#ifdef SYS_creat64
  syscall_table[SYS_creat64] = "creat64";
#endif
  syscall_table[SYS_door] = "door";
#ifdef SYS_dup
  syscall_table[SYS_dup] = "dup";
#endif
#ifdef SYS_evsys
  syscall_table[SYS_evsys] = "evsys";
#endif
#ifdef SYS_evtrapret
  syscall_table[SYS_evtrapret] = "evtrapret";
#endif
  syscall_table[SYS_exacctsys] = "exacctsys";
#ifdef SYS_exec
  syscall_table[SYS_exec] = "exec";
#endif
  syscall_table[SYS_execve] = "execve";
  syscall_table[SYS_exit] = "exit";
#ifdef SYS_faccessat
  syscall_table[SYS_faccessat] = "faccessat";
#endif
  syscall_table[SYS_facl] = "facl";
  syscall_table[SYS_fchdir] = "fchdir";
#ifdef SYS_fchmod
  syscall_table[SYS_fchmod] = "fchmod";
#endif
#ifdef SYS_fchmodat
  syscall_table[SYS_fchmodat] = "fchmodat";
#endif
#ifdef SYS_fchown
  syscall_table[SYS_fchown] = "fchown";
#endif
#ifdef SYS_fchownat
  syscall_table[SYS_fchownat] = "fchownat";
#endif
  syscall_table[SYS_fchroot] = "fchroot";
  syscall_table[SYS_fcntl] = "fcntl";
  syscall_table[SYS_fdsync] = "fdsync";
#ifdef SYS_fork1
  syscall_table[SYS_fork1] = "fork1";
#endif
#ifdef SYS_forkall
  syscall_table[SYS_forkall] = "forkall";
#endif
#ifdef SYS_forksys
  syscall_table[SYS_forksys] = "forksys";
#endif
  syscall_table[SYS_fpathconf] = "fpathconf";
#ifdef SYS_frealpathat
  syscall_table[SYS_frealpathat] = "frealpathat";
#endif
#ifdef SYS_fsat
  syscall_table[SYS_fsat] = "fsat";
#endif
#ifdef SYS_fstat
  syscall_table[SYS_fstat] = "fstat";
#endif
#ifdef SYS_fstat64
  syscall_table[SYS_fstat64] = "fstat64";
#endif
#ifdef SYS_fstatat
  syscall_table[SYS_fstatat] = "fstatat";
#endif
#ifdef SYS_fstatat64
  syscall_table[SYS_fstatat64] = "fstatat64";
#endif
  syscall_table[SYS_fstatfs] = "fstatfs";
  syscall_table[SYS_fstatvfs] = "fstatvfs";
  syscall_table[SYS_fstatvfs64] = "fstatvfs64";
#ifdef SYS_fxstat
  syscall_table[SYS_fxstat] = "fxstat";
#endif
  syscall_table[SYS_getcwd] = "getcwd";
  syscall_table[SYS_getdents] = "getdents";
  syscall_table[SYS_getdents64] = "getdents64";
  syscall_table[SYS_getgid] = "getgid";
  syscall_table[SYS_getgroups] = "getgroups";
  syscall_table[SYS_getitimer] = "getitimer";
  syscall_table[SYS_getloadavg] = "getloadavg";
  syscall_table[SYS_getmsg] = "getmsg";
  syscall_table[SYS_getpagesizes] = "getpagesizes";
  syscall_table[SYS_getpeername] = "getpeername";
  syscall_table[SYS_getpid] = "getpid";
  syscall_table[SYS_getpmsg] = "getpmsg";
#ifdef SYS_getrandom
  syscall_table[SYS_getrandom] = "getrandom";
#endif
  syscall_table[SYS_getrlimit] = "getrlimit";
  syscall_table[SYS_getrlimit64] = "getrlimit64";
  syscall_table[SYS_getsockname] = "getsockname";
  syscall_table[SYS_getsockopt] = "getsockopt";
  syscall_table[SYS_gettimeofday] = "gettimeofday";
  syscall_table[SYS_getuid] = "getuid";
  syscall_table[SYS_gtty] = "gtty";
  syscall_table[SYS_hrtsys] = "hrtsys";
  syscall_table[SYS_inst_sync] = "inst_sync";
  syscall_table[SYS_install_utrap] = "install_utrap";
  syscall_table[SYS_ioctl] = "ioctl";
#ifdef SYS_issetugid
  syscall_table[SYS_issetugid] = "issetugid";
#endif
  syscall_table[SYS_kaio] = "kaio";
  syscall_table[SYS_kill] = "kill";
  syscall_table[SYS_labelsys] = "labelsys";
#ifdef SYS_lchown
  syscall_table[SYS_lchown] = "lchown";
#endif
  syscall_table[SYS_lgrpsys] = "lgrpsys";
#ifdef SYS_link
  syscall_table[SYS_link] = "link";
#endif
#ifdef SYS_linkat
  syscall_table[SYS_linkat] = "linkat";
#endif
  syscall_table[SYS_listen] = "listen";
  syscall_table[SYS_llseek] = "llseek";
  syscall_table[SYS_lseek] = "lseek";
#ifdef SYS_lstat
  syscall_table[SYS_lstat] = "lstat";
#endif
#ifdef SYS_lstat64
  syscall_table[SYS_lstat64] = "lstat64";
#endif
  syscall_table[SYS_lwp_cond_broadcast] = "lwp_cond_broadcast";
  syscall_table[SYS_lwp_cond_signal] = "lwp_cond_signal";
  syscall_table[SYS_lwp_cond_wait] = "lwp_cond_wait";
  syscall_table[SYS_lwp_continue] = "lwp_continue";
  syscall_table[SYS_lwp_create] = "lwp_create";
  syscall_table[SYS_lwp_detach] = "lwp_detach";
  syscall_table[SYS_lwp_exit] = "lwp_exit";
  syscall_table[SYS_lwp_info] = "lwp_info";
#ifdef SYS_lwp_kill
  syscall_table[SYS_lwp_kill] = "lwp_kill";
#endif
#ifdef SYS_lwp_mutex_lock
  syscall_table[SYS_lwp_mutex_lock] = "lwp_mutex_lock";
#endif
  syscall_table[SYS_lwp_mutex_register] = "lwp_mutex_register";
  syscall_table[SYS_lwp_mutex_timedlock] = "lwp_mutex_timedlock";
  syscall_table[SYS_lwp_mutex_trylock] = "lwp_mutex_trylock";
  syscall_table[SYS_lwp_mutex_unlock] = "lwp_mutex_unlock";
  syscall_table[SYS_lwp_mutex_wakeup] = "lwp_mutex_wakeup";
#ifdef SYS_lwp_name
  syscall_table[SYS_lwp_name] = "lwp_name";
#endif
  syscall_table[SYS_lwp_park] = "lwp_park";
  syscall_table[SYS_lwp_private] = "lwp_private";
  syscall_table[SYS_lwp_rwlock_sys] = "lwp_rwlock_sys";
  syscall_table[SYS_lwp_self] = "lwp_self";
  syscall_table[SYS_lwp_sema_post] = "lwp_sema_post";
  syscall_table[SYS_lwp_sema_timedwait] = "lwp_sema_timedwait";
  syscall_table[SYS_lwp_sema_trywait] = "lwp_sema_trywait";
#ifdef SYS_lwp_sema_wait
  syscall_table[SYS_lwp_sema_wait] = "lwp_sema_wait";
#endif
  syscall_table[SYS_lwp_sigmask] = "lwp_sigmask";
#ifdef SYS_lwp_sigqueue
  syscall_table[SYS_lwp_sigqueue] = "lwp_sigqueue";
#endif
  syscall_table[SYS_lwp_suspend] = "lwp_suspend";
  syscall_table[SYS_lwp_wait] = "lwp_wait";
#ifdef SYS_lxstat
  syscall_table[SYS_lxstat] = "lxstat";
#endif
  syscall_table[SYS_memcntl] = "memcntl";
#ifdef SYS_memsys
  syscall_table[SYS_memsys] = "memsys";
#endif
  syscall_table[SYS_mincore] = "mincore";
#ifdef SYS_mkdir
  syscall_table[SYS_mkdir] = "mkdir";
#endif
#ifdef SYS_mkdirat
  syscall_table[SYS_mkdirat] = "mkdirat";
#endif
#ifdef SYS_mknod
  syscall_table[SYS_mknod] = "mknod";
#endif
#ifdef SYS_mknodat
  syscall_table[SYS_mknodat] = "mknodat";
#endif
  syscall_table[SYS_mmap] = "mmap";
  syscall_table[SYS_mmap64] = "mmap64";
#ifdef SYS_mmapobj
  syscall_table[SYS_mmapobj] = "mmapobj";
#endif
  syscall_table[SYS_modctl] = "modctl";
  syscall_table[SYS_mount] = "mount";
  syscall_table[SYS_mprotect] = "mprotect";
  syscall_table[SYS_msgsys] = "msgsys";
  syscall_table[SYS_munmap] = "munmap";
  syscall_table[SYS_nanosleep] = "nanosleep";
  syscall_table[SYS_nfssys] = "nfssys";
  syscall_table[SYS_nice] = "nice";
  syscall_table[SYS_ntp_adjtime] = "ntp_adjtime";
  syscall_table[SYS_ntp_gettime] = "ntp_gettime";
#ifdef SYS_open
  syscall_table[SYS_open] = "open";
#endif
#ifdef SYS_open64
  syscall_table[SYS_open64] = "open64";
#endif
#ifdef SYS_openat
  syscall_table[SYS_openat] = "openat";
#endif
#ifdef SYS_openat64
  syscall_table[SYS_openat64] = "openat64";
#endif
  syscall_table[SYS_p_online] = "p_online";
  syscall_table[SYS_pathconf] = "pathconf";
  syscall_table[SYS_pause] = "pause";
  syscall_table[SYS_pcsample] = "pcsample";
  syscall_table[SYS_pgrpsys] = "pgrpsys";
  syscall_table[SYS_pipe] = "pipe";
#ifdef SYS_plock
  syscall_table[SYS_plock] = "plock";
#endif
#ifdef SYS_poll
  syscall_table[SYS_poll] = "poll";
#endif
  syscall_table[SYS_pollsys] = "pollsys";
  syscall_table[SYS_port] = "port";
  syscall_table[SYS_pread] = "pread";
  syscall_table[SYS_pread64] = "pread64";
  syscall_table[SYS_priocntlsys] = "priocntlsys";
  syscall_table[SYS_privsys] = "privsys";
#ifdef SYS_processor_bind
  syscall_table[SYS_processor_bind] = "processor_bind";
#endif
#ifdef SYS_processor_info
  syscall_table[SYS_processor_info] = "processor_info";
#endif
#ifdef SYS_processor_sys
  syscall_table[SYS_processor_sys] = "processor_sys";
#endif
  syscall_table[SYS_profil] = "profil";
  syscall_table[SYS_pset] = "pset";
  syscall_table[SYS_putmsg] = "putmsg";
  syscall_table[SYS_putpmsg] = "putpmsg";
  syscall_table[SYS_pwrite] = "pwrite";
  syscall_table[SYS_pwrite64] = "pwrite64";
  syscall_table[SYS_rctlsys] = "rctlsys";
  syscall_table[SYS_read] = "read";
#ifdef SYS_readlink
  syscall_table[SYS_readlink] = "readlink";
#endif
#ifdef SYS_readlinkat
  syscall_table[SYS_readlinkat] = "readlinkat";
#endif
  syscall_table[SYS_readv] = "readv";
  syscall_table[SYS_recv] = "recv";
  syscall_table[SYS_recvfrom] = "recvfrom";
#ifdef SYS_recvmmsg
  syscall_table[SYS_recvmmsg] = "recvmmsg";
#endif
  syscall_table[SYS_recvmsg] = "recvmsg";
#ifdef SYS_reflinkat
  syscall_table[SYS_reflinkat] = "reflinkat";
#endif
#ifdef SYS_rename
  syscall_table[SYS_rename] = "rename";
#endif
#ifdef SYS_renameat
  syscall_table[SYS_renameat] = "renameat";
#endif
  syscall_table[SYS_resolvepath] = "resolvepath";
#ifdef SYS_rmdir
  syscall_table[SYS_rmdir] = "rmdir";
#endif
  syscall_table[SYS_rpcsys] = "rpcsys";
  syscall_table[SYS_rusagesys] = "rusagesys";
  syscall_table[SYS_schedctl] = "schedctl";
#ifdef SYS_secsys
  syscall_table[SYS_secsys] = "secsys";
#endif
  syscall_table[SYS_semsys] = "semsys";
  syscall_table[SYS_send] = "send";
  syscall_table[SYS_sendfilev] = "sendfilev";
#ifdef SYS_sendmmsg
  syscall_table[SYS_sendmmsg] = "sendmmsg";
#endif
  syscall_table[SYS_sendmsg] = "sendmsg";
  syscall_table[SYS_sendto] = "sendto";
  syscall_table[SYS_setegid] = "setegid";
  syscall_table[SYS_seteuid] = "seteuid";
  syscall_table[SYS_setgid] = "setgid";
  syscall_table[SYS_setgroups] = "setgroups";
  syscall_table[SYS_setitimer] = "setitimer";
  syscall_table[SYS_setregid] = "setregid";
  syscall_table[SYS_setreuid] = "setreuid";
  syscall_table[SYS_setrlimit] = "setrlimit";
  syscall_table[SYS_setrlimit64] = "setrlimit64";
  syscall_table[SYS_setsockopt] = "setsockopt";
  syscall_table[SYS_setuid] = "setuid";
  syscall_table[SYS_sharefs] = "sharefs";
  syscall_table[SYS_shmsys] = "shmsys";
  syscall_table[SYS_shutdown] = "shutdown";
#ifdef SYS_sidsys
  syscall_table[SYS_sidsys] = "sidsys";
#endif
  syscall_table[SYS_sigaction] = "sigaction";
  syscall_table[SYS_sigaltstack] = "sigaltstack";
#ifdef SYS_signal
  syscall_table[SYS_signal] = "signal";
#endif
  syscall_table[SYS_signotify] = "signotify";
  syscall_table[SYS_sigpending] = "sigpending";
  syscall_table[SYS_sigprocmask] = "sigprocmask";
  syscall_table[SYS_sigqueue] = "sigqueue";
#ifdef SYS_sigresend
  syscall_table[SYS_sigresend] = "sigresend";
#endif
  syscall_table[SYS_sigsendsys] = "sigsendsys";
  syscall_table[SYS_sigsuspend] = "sigsuspend";
  syscall_table[SYS_sigtimedwait] = "sigtimedwait";
  syscall_table[SYS_so_socket] = "so_socket";
  syscall_table[SYS_so_socketpair] = "so_socketpair";
  syscall_table[SYS_sockconfig] = "sockconfig";
#ifdef SYS_sparc_fixalign
  syscall_table[SYS_sparc_fixalign] = "sparc_fixalign";
#endif
  syscall_table[SYS_sparc_utrap_install] = "sparc_utrap_install";
#ifdef SYS_spawn
  syscall_table[SYS_spawn] = "spawn";
#endif
#ifdef SYS_stat
  syscall_table[SYS_stat] = "stat";
#endif
#ifdef SYS_stat64
  syscall_table[SYS_stat64] = "stat64";
#endif
  syscall_table[SYS_statfs] = "statfs";
  syscall_table[SYS_statvfs] = "statvfs";
  syscall_table[SYS_statvfs64] = "statvfs64";
  syscall_table[SYS_stime] = "stime";
  syscall_table[SYS_stty] = "stty";
#ifdef SYS_symlink
  syscall_table[SYS_symlink] = "symlink";
#endif
#ifdef SYS_symlinkat
  syscall_table[SYS_symlinkat] = "symlinkat";
#endif
  syscall_table[SYS_sync] = "sync";
  syscall_table[SYS_syscall] = "syscall";
  syscall_table[SYS_sysconfig] = "sysconfig";
  syscall_table[SYS_sysfs] = "sysfs";
  syscall_table[SYS_sysi86] = "sysi86";
#ifdef SYS_syssun
  syscall_table[SYS_syssun] = "syssun";
#endif
#ifdef SYS_system_stats
  syscall_table[SYS_system_stats] = "system_stats";
#endif
  syscall_table[SYS_systeminfo] = "systeminfo";
  syscall_table[SYS_tasksys] = "tasksys";
  syscall_table[SYS_time] = "time";
  syscall_table[SYS_timer_create] = "timer_create";
  syscall_table[SYS_timer_delete] = "timer_delete";
  syscall_table[SYS_timer_getoverrun] = "timer_getoverrun";
  syscall_table[SYS_timer_gettime] = "timer_gettime";
  syscall_table[SYS_timer_settime] = "timer_settime";
  syscall_table[SYS_times] = "times";
  syscall_table[SYS_uadmin] = "uadmin";
  syscall_table[SYS_ucredsys] = "ucredsys";
  syscall_table[SYS_ulimit] = "ulimit";
  syscall_table[SYS_umask] = "umask";
#ifdef SYS_umount
  syscall_table[SYS_umount] = "umount";
#endif
  syscall_table[SYS_umount2] = "umount2";
  syscall_table[SYS_uname] = "uname";
#ifdef SYS_unlink
  syscall_table[SYS_unlink] = "unlink";
#endif
#ifdef SYS_unlinkat
  syscall_table[SYS_unlinkat] = "unlinkat";
#endif
#ifdef SYS_utime
  syscall_table[SYS_utime] = "utime";
#endif
#ifdef SYS_utimensat
  syscall_table[SYS_utimensat] = "utimensat";
#endif
#ifdef SYS_utimes
  syscall_table[SYS_utimes] = "utimes";
#endif
#ifdef SYS_utimesys
  syscall_table[SYS_utimesys] = "utimesys";
#endif
  syscall_table[SYS_utssys] = "utssys";
  syscall_table[SYS_uucopy] = "uucopy";
  syscall_table[SYS_uucopystr] = "uucopystr";
#ifdef SYS_uuidsys
  syscall_table[SYS_uuidsys] = "uuidsys";
#endif
#ifdef SYS_va_mask
  syscall_table[SYS_va_mask] = "va_mask";
#endif
  syscall_table[SYS_vfork] = "vfork";
  syscall_table[SYS_vhangup] = "vhangup";
#ifdef SYS_wait
  syscall_table[SYS_wait] = "wait";
#endif
#ifdef SYS_waitid
  syscall_table[SYS_waitid] = "waitid";
#endif
#ifdef SYS_waitsys
  syscall_table[SYS_waitsys] = "waitsys";
#endif
  syscall_table[SYS_write] = "write";
  syscall_table[SYS_writev] = "writev";
#ifdef SYS_xmknod
  syscall_table[SYS_xmknod] = "xmknod";
#endif
#ifdef SYS_xstat
  syscall_table[SYS_xstat] = "xstat";
#endif
  syscall_table[SYS_yield] = "yield";
  syscall_table[SYS_zone] = "zone";
}

/* Prettyprint syscall NUM.  */

void
proc_prettyfprint_syscall (FILE *file, int num, int verbose)
{
  if (syscall_table[num])
    fprintf (file, "SYS_%s ", syscall_table[num]);
  else
    fprintf (file, "<Unknown syscall %d> ", num);
}

void
proc_prettyprint_syscall (int num, int verbose)
{
  proc_prettyfprint_syscall (stdout, num, verbose);
}

/* Prettyprint all syscalls in SYSSET.  */

void
proc_prettyfprint_syscalls (FILE *file, sysset_t *sysset, int verbose)
{
  int i;

  for (i = 0; i < MAX_SYSCALLS; i++)
    if (prismember (sysset, i))
      {
	proc_prettyfprint_syscall (file, i, verbose);
      }
  fprintf (file, "\n");
}

void
proc_prettyprint_syscalls (sysset_t *sysset, int verbose)
{
  proc_prettyfprint_syscalls (stdout, sysset, verbose);
}

/* Prettyprint signals.  */

/* Signal translation table, ordered ANSI-standard signals first,
   other signals second, with signals in each block ordered by their
   numerical values on a typical POSIX platform.  */

static struct trans signal_table[] = 
{
  { 0,      "<no signal>", "no signal" }, 

  /* SIGINT, SIGILL, SIGABRT, SIGFPE, SIGSEGV and SIGTERM
     are ANSI-standard signals and are always available.  */

  { SIGINT, "SIGINT", "Interrupt (rubout)" },
  { SIGILL, "SIGILL", "Illegal instruction" },	/* not reset when caught */
  { SIGABRT, "SIGABRT", "used by abort()" },	/* replaces SIGIOT */
  { SIGFPE, "SIGFPE", "Floating point exception" },
  { SIGSEGV, "SIGSEGV", "Segmentation violation" },
  { SIGTERM, "SIGTERM", "Software termination signal from kill" },

  /* All other signals need preprocessor conditionals.  */

  { SIGHUP, "SIGHUP", "Hangup" },
  { SIGQUIT, "SIGQUIT", "Quit (ASCII FS)" },
  { SIGTRAP, "SIGTRAP", "Trace trap" },		/* not reset when caught */
  { SIGIOT, "SIGIOT", "IOT instruction" },
  { SIGEMT, "SIGEMT", "EMT instruction" },
  { SIGKILL, "SIGKILL", "Kill" },	/* Solaris: cannot be caught/ignored */
  { SIGBUS, "SIGBUS", "Bus error" },
  { SIGSYS, "SIGSYS", "Bad argument to system call" },
  { SIGPIPE, "SIGPIPE", "Write to pipe with no one to read it" },
  { SIGALRM, "SIGALRM", "Alarm clock" },
  { SIGUSR1, "SIGUSR1", "User defined signal 1" },
  { SIGUSR2, "SIGUSR2", "User defined signal 2" },
  { SIGCHLD, "SIGCHLD", "Child status changed" },	/* Posix version */
  { SIGCLD, "SIGCLD", "Child status changed" },		/* Solaris version */
  { SIGPWR, "SIGPWR", "Power-fail restart" },
  { SIGWINCH, "SIGWINCH", "Window size change" },
  { SIGURG, "SIGURG", "Urgent socket condition" },
  { SIGPOLL, "SIGPOLL", "Pollable event" },
  { SIGIO, "SIGIO", "Socket I/O possible" },	/* alias for SIGPOLL */
  { SIGSTOP, "SIGSTOP", "Stop, not from tty" },	/* cannot be caught or
						   ignored */
  { SIGTSTP, "SIGTSTP", "User stop from tty" },
  { SIGCONT, "SIGCONT", "Stopped process has been continued" },
  { SIGTTIN, "SIGTTIN", "Background tty read attempted" },
  { SIGTTOU, "SIGTTOU", "Background tty write attempted" },
  { SIGVTALRM, "SIGVTALRM", "Virtual timer expired" },
  { SIGPROF, "SIGPROF", "Profiling timer expired" },
  { SIGXCPU, "SIGXCPU", "Exceeded CPU limit" },
  { SIGXFSZ, "SIGXFSZ", "Exceeded file size limit" },
  { SIGWAITING, "SIGWAITING", "Process's LWPs are blocked" },
  { SIGLWP, "SIGLWP", "Used by thread library" },
  { SIGFREEZE, "SIGFREEZE", "Used by CPR" },
  { SIGTHAW, "SIGTHAW", "Used by CPR" },
  { SIGCANCEL, "SIGCANCEL", "Used by libthread" },
  { SIGLOST, "SIGLOST", "Resource lost" },

  /* FIXME: add real-time signals.  */
};

/* Prettyprint signal number SIGNO.  */

void
proc_prettyfprint_signal (FILE *file, int signo, int verbose)
{
  int i;

  for (i = 0; i < sizeof (signal_table) / sizeof (signal_table[0]); i++)
    if (signo == signal_table[i].value)
      {
	fprintf (file, "%s", signal_table[i].name);
	if (verbose)
	  fprintf (file, ": %s\n", signal_table[i].desc);
	else
	  fprintf (file, " ");
	return;
      }
  fprintf (file, "Unknown signal %d%c", signo, verbose ? '\n' : ' ');
}

void
proc_prettyprint_signal (int signo, int verbose)
{
  proc_prettyfprint_signal (stdout, signo, verbose);
}

/* Prettyprint all signals in SIGSET.  */

void
proc_prettyfprint_signalset (FILE *file, sigset_t *sigset, int verbose)
{
  int i;

  /* Loop over all signal numbers from 0 to NSIG, using them as the
     index to prismember.  The signal table had better not contain
     aliases, for if it does they will both be printed.  */

  for (i = 0; i < NSIG; i++)
    if (prismember (sigset, i))
      proc_prettyfprint_signal (file, i, verbose);

  if (!verbose)
    fprintf (file, "\n");
}

void
proc_prettyprint_signalset (sigset_t *sigset, int verbose)
{
  proc_prettyfprint_signalset (stdout, sigset, verbose);
}


/* Prettyprint faults.  */

/* Fault translation table.  */

static struct trans fault_table[] =
{
  { FLTILL, "FLTILL", "Illegal instruction" },
  { FLTPRIV, "FLTPRIV", "Privileged instruction" },
  { FLTBPT, "FLTBPT", "Breakpoint trap" },
  { FLTTRACE, "FLTTRACE", "Trace trap" },
  { FLTACCESS, "FLTACCESS", "Memory access fault" },
  { FLTBOUNDS, "FLTBOUNDS", "Memory bounds violation" },
  { FLTIOVF, "FLTIOVF", "Integer overflow" },
  { FLTIZDIV, "FLTIZDIV", "Integer zero divide" },
  { FLTFPE, "FLTFPE", "Floating-point exception" },
  { FLTSTACK, "FLTSTACK", "Unrecoverable stack fault" },
  { FLTPAGE, "FLTPAGE", "Recoverable page fault" },
  { FLTWATCH, "FLTWATCH", "User watchpoint" },
};

/* Work horse.  Accepts an index into the fault table, prints it
   pretty.  */

static void
prettyfprint_faulttable_entry (FILE *file, int i, int verbose)
{
  fprintf (file, "%s", fault_table[i].name);
  if (verbose)
    fprintf (file, ": %s\n", fault_table[i].desc);
  else
    fprintf (file, " ");
}

/* Prettyprint hardware fault number FAULTNO.  */

void
proc_prettyfprint_fault (FILE *file, int faultno, int verbose)
{
  int i;

  for (i = 0; i < ARRAY_SIZE (fault_table); i++)
    if (faultno == fault_table[i].value)
      {
	prettyfprint_faulttable_entry (file, i, verbose);
	return;
      }

  fprintf (file, "Unknown hardware fault %d%c", 
	   faultno, verbose ? '\n' : ' ');
}

void
proc_prettyprint_fault (int faultno, int verbose)
{
  proc_prettyfprint_fault (stdout, faultno, verbose);
}

/* Prettyprint all faults in FLTSET.  */

void
proc_prettyfprint_faultset (FILE *file, fltset_t *fltset, int verbose)
{
  int i;

  /* Loop through the fault table, using the value field as the index
     to prismember.  The fault table had better not contain aliases,
     for if it does they will both be printed.  */

  for (i = 0; i < ARRAY_SIZE (fault_table); i++)
    if (prismember (fltset, fault_table[i].value))
      prettyfprint_faulttable_entry (file, i, verbose);

  if (!verbose)
    fprintf (file, "\n");
}

void
proc_prettyprint_faultset (fltset_t *fltset, int verbose)
{
  proc_prettyfprint_faultset (stdout, fltset, verbose);
}

/* TODO: actions, holds...  */

void
proc_prettyprint_actionset (struct sigaction *actions, int verbose)
{
}

void _initialize_proc_events ();
void
_initialize_proc_events ()
{
  init_syscall_table ();
}
