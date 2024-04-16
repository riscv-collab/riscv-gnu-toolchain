/* Process record and replay target code for GNU/Linux.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

#ifndef LINUX_RECORD_H
#define LINUX_RECORD_H

struct linux_record_tdep
{
  /* The size of the type that will be used in a system call.  */
  int size_pointer;
  int size__old_kernel_stat;
  int size_tms;
  int size_loff_t;
  int size_flock;
  int size_oldold_utsname;
  int size_ustat;
  int size_old_sigaction;
  int size_old_sigset_t;
  int size_rlimit;
  int size_rusage;
  int size_timeval;
  int size_timezone;
  int size_old_gid_t;
  int size_old_uid_t;
  int size_fd_set;
  int size_old_dirent;
  int size_statfs;
  int size_statfs64;
  int size_sockaddr;
  int size_int;
  int size_long;
  int size_ulong;
  int size_msghdr;
  int size_itimerval;
  int size_stat;
  int size_old_utsname;
  int size_sysinfo;
  int size_msqid_ds;
  int size_shmid_ds;
  int size_new_utsname;
  int size_timex;
  int size_mem_dqinfo;
  int size_if_dqblk;
  int size_fs_quota_stat;
  int size_timespec;
  int size_pollfd;
  int size_NFS_FHSIZE;
  int size_knfsd_fh;
  int size_TASK_COMM_LEN;
  int size_sigaction;
  int size_sigset_t;
  int size_siginfo_t;
  int size_cap_user_data_t;
  int size_stack_t;
  int size_off_t;
  int size_stat64;
  int size_gid_t;
  int size_uid_t;
  int size_PAGE_SIZE;
  int size_flock64;
  int size_user_desc;
  int size_io_event;
  int size_iocb;
  int size_epoll_event;
  int size_itimerspec;
  int size_mq_attr;
  int size_termios;
  int size_termios2;
  int size_pid_t;
  int size_winsize;
  int size_serial_struct;
  int size_serial_icounter_struct;
  int size_hayes_esp_config;
  int size_size_t;
  int size_iovec;
  int size_time_t;

  /* The values of the second argument of system call "sys_ioctl".  */
  ULONGEST ioctl_TCGETS;
  ULONGEST ioctl_TCSETS;
  ULONGEST ioctl_TCSETSW;
  ULONGEST ioctl_TCSETSF;
  ULONGEST ioctl_TCGETA;
  ULONGEST ioctl_TCSETA;
  ULONGEST ioctl_TCSETAW;
  ULONGEST ioctl_TCSETAF;
  ULONGEST ioctl_TCSBRK;
  ULONGEST ioctl_TCXONC;
  ULONGEST ioctl_TCFLSH;
  ULONGEST ioctl_TIOCEXCL;
  ULONGEST ioctl_TIOCNXCL;
  ULONGEST ioctl_TIOCSCTTY;
  ULONGEST ioctl_TIOCGPGRP;
  ULONGEST ioctl_TIOCSPGRP;
  ULONGEST ioctl_TIOCOUTQ;
  ULONGEST ioctl_TIOCSTI;
  ULONGEST ioctl_TIOCGWINSZ;
  ULONGEST ioctl_TIOCSWINSZ;
  ULONGEST ioctl_TIOCMGET;
  ULONGEST ioctl_TIOCMBIS;
  ULONGEST ioctl_TIOCMBIC;
  ULONGEST ioctl_TIOCMSET;
  ULONGEST ioctl_TIOCGSOFTCAR;
  ULONGEST ioctl_TIOCSSOFTCAR;
  ULONGEST ioctl_FIONREAD;
  ULONGEST ioctl_TIOCINQ;
  ULONGEST ioctl_TIOCLINUX;
  ULONGEST ioctl_TIOCCONS;
  ULONGEST ioctl_TIOCGSERIAL;
  ULONGEST ioctl_TIOCSSERIAL;
  ULONGEST ioctl_TIOCPKT;
  ULONGEST ioctl_FIONBIO;
  ULONGEST ioctl_TIOCNOTTY;
  ULONGEST ioctl_TIOCSETD;
  ULONGEST ioctl_TIOCGETD;
  ULONGEST ioctl_TCSBRKP;
  ULONGEST ioctl_TIOCTTYGSTRUCT;
  ULONGEST ioctl_TIOCSBRK;
  ULONGEST ioctl_TIOCCBRK;
  ULONGEST ioctl_TIOCGSID;
  ULONGEST ioctl_TCGETS2;
  ULONGEST ioctl_TCSETS2;
  ULONGEST ioctl_TCSETSW2;
  ULONGEST ioctl_TCSETSF2;
  ULONGEST ioctl_TIOCGPTN;
  ULONGEST ioctl_TIOCSPTLCK;
  ULONGEST ioctl_FIONCLEX;
  ULONGEST ioctl_FIOCLEX;
  ULONGEST ioctl_FIOASYNC;
  ULONGEST ioctl_TIOCSERCONFIG;
  ULONGEST ioctl_TIOCSERGWILD;
  ULONGEST ioctl_TIOCSERSWILD;
  ULONGEST ioctl_TIOCGLCKTRMIOS;
  ULONGEST ioctl_TIOCSLCKTRMIOS;
  ULONGEST ioctl_TIOCSERGSTRUCT;
  ULONGEST ioctl_TIOCSERGETLSR;
  ULONGEST ioctl_TIOCSERGETMULTI;
  ULONGEST ioctl_TIOCSERSETMULTI;
  ULONGEST ioctl_TIOCMIWAIT;
  ULONGEST ioctl_TIOCGICOUNT;
  ULONGEST ioctl_TIOCGHAYESESP;
  ULONGEST ioctl_TIOCSHAYESESP;
  ULONGEST ioctl_FIOQSIZE;

  /* The values of the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  */
  int fcntl_F_GETLK;
  int fcntl_F_GETLK64;
  int fcntl_F_SETLK64;
  int fcntl_F_SETLKW64;

  /* The number of the registers that are used as the arguments of
     a system call.  */
  int arg1;
  int arg2;
  int arg3;
  int arg4;
  int arg5;
  int arg6;
  int arg7;
};

/* Enum that defines the gdb-canonical set of Linux syscall identifiers.
   Different architectures will have different sets of syscall ids, and
   each must provide a mapping from their set to this one.  */

enum gdb_syscall {
  /* An unknown GDB syscall, not a real syscall.  */
  gdb_sys_no_syscall = -1,

  gdb_sys_restart_syscall = 0,
  gdb_sys_exit = 1,
  gdb_sys_fork = 2,
  gdb_sys_read = 3,
  gdb_sys_write = 4,
  gdb_sys_open = 5,
  gdb_sys_close = 6,
  gdb_sys_waitpid = 7,
  gdb_sys_creat = 8,
  gdb_sys_link = 9,
  gdb_sys_unlink = 10,
  gdb_sys_execve = 11,
  gdb_sys_chdir = 12,
  gdb_sys_time = 13,
  gdb_sys_mknod = 14,
  gdb_sys_chmod = 15,
  gdb_sys_lchown16 = 16,
  gdb_sys_ni_syscall17 = 17,
  gdb_sys_stat = 18,
  gdb_sys_lseek = 19,
  gdb_sys_getpid = 20,
  gdb_sys_mount = 21,
  gdb_sys_oldumount = 22,
  gdb_sys_setuid16 = 23,
  gdb_sys_getuid16 = 24,
  gdb_sys_stime = 25,
  gdb_sys_ptrace = 26,
  gdb_sys_alarm = 27,
  gdb_sys_fstat = 28,
  gdb_sys_pause = 29,
  gdb_sys_utime = 30,
  gdb_sys_ni_syscall31 = 31,
  gdb_sys_ni_syscall32 = 32,
  gdb_sys_access = 33,
  gdb_sys_nice = 34,
  gdb_sys_ni_syscall35 = 35,
  gdb_sys_sync = 36,
  gdb_sys_kill = 37,
  gdb_sys_rename = 38,
  gdb_sys_mkdir = 39,
  gdb_sys_rmdir = 40,
  gdb_sys_dup = 41,
  gdb_sys_pipe = 42,
  gdb_sys_times = 43,
  gdb_sys_ni_syscall44 = 44,
  gdb_sys_brk = 45,
  gdb_sys_setgid16 = 46,
  gdb_sys_getgid16 = 47,
  gdb_sys_signal = 48,
  gdb_sys_geteuid16 = 49,
  gdb_sys_getegid16 = 50,
  gdb_sys_acct = 51,
  gdb_sys_umount = 52,
  gdb_sys_ni_syscall53 = 53,
  gdb_sys_ioctl = 54,
  gdb_sys_fcntl = 55,
  gdb_sys_ni_syscall56 = 56,
  gdb_sys_setpgid = 57,
  gdb_sys_ni_syscall58 = 58,
  gdb_sys_olduname = 59,
  gdb_sys_umask = 60,
  gdb_sys_chroot = 61,
  gdb_sys_ustat = 62,
  gdb_sys_dup2 = 63,
  gdb_sys_getppid = 64,
  gdb_sys_getpgrp = 65,
  gdb_sys_setsid = 66,
  gdb_sys_sigaction = 67,
  gdb_sys_sgetmask = 68,
  gdb_sys_ssetmask = 69,
  gdb_sys_setreuid16 = 70,
  gdb_sys_setregid16 = 71,
  gdb_sys_sigsuspend = 72,
  gdb_sys_sigpending = 73,
  gdb_sys_sethostname = 74,
  gdb_sys_setrlimit = 75,
  gdb_sys_old_getrlimit = 76,
  gdb_sys_getrusage = 77,
  gdb_sys_gettimeofday = 78,
  gdb_sys_settimeofday = 79,
  gdb_sys_getgroups16 = 80,
  gdb_sys_setgroups16 = 81,
  gdb_old_select = 82,
  gdb_sys_symlink = 83,
  gdb_sys_lstat = 84,
  gdb_sys_readlink = 85,
  gdb_sys_uselib = 86,
  gdb_sys_swapon = 87,
  gdb_sys_reboot = 88,
  gdb_old_readdir = 89,
  gdb_old_mmap = 90,
  gdb_sys_munmap = 91,
  gdb_sys_truncate = 92,
  gdb_sys_ftruncate = 93,
  gdb_sys_fchmod = 94,
  gdb_sys_fchown16 = 95,
  gdb_sys_getpriority = 96,
  gdb_sys_setpriority = 97,
  gdb_sys_ni_syscall98 = 98,
  gdb_sys_statfs = 99,
  gdb_sys_fstatfs = 100,
  gdb_sys_ioperm = 101,
  gdb_sys_socketcall = 102,
  gdb_sys_syslog = 103,
  gdb_sys_setitimer = 104,
  gdb_sys_getitimer = 105,
  gdb_sys_newstat = 106,
  gdb_sys_newlstat = 107,
  gdb_sys_newfstat = 108,
  gdb_sys_uname = 109,
  gdb_sys_iopl = 110,
  gdb_sys_vhangup = 111,
  gdb_sys_ni_syscall112 = 112,
  gdb_sys_vm86old = 113,
  gdb_sys_wait4 = 114,
  gdb_sys_swapoff = 115,
  gdb_sys_sysinfo = 116,
  gdb_sys_ipc = 117,
  gdb_sys_fsync = 118,
  gdb_sys_sigreturn = 119,
  gdb_sys_clone = 120,
  gdb_sys_setdomainname = 121,
  gdb_sys_newuname = 122,
  gdb_sys_modify_ldt = 123,
  gdb_sys_adjtimex = 124,
  gdb_sys_mprotect = 125,
  gdb_sys_sigprocmask = 126,
  gdb_sys_ni_syscall127 = 127,
  gdb_sys_init_module = 128,
  gdb_sys_delete_module = 129,
  gdb_sys_ni_syscall130 = 130,
  gdb_sys_quotactl = 131,
  gdb_sys_getpgid = 132,
  gdb_sys_fchdir = 133,
  gdb_sys_bdflush = 134,
  gdb_sys_sysfs = 135,
  gdb_sys_personality = 136,
  gdb_sys_ni_syscall137 = 137,
  gdb_sys_setfsuid16 = 138,
  gdb_sys_setfsgid16 = 139,
  gdb_sys_llseek = 140,
  gdb_sys_getdents = 141,
  gdb_sys_select = 142,
  gdb_sys_flock = 143,
  gdb_sys_msync = 144,
  gdb_sys_readv = 145,
  gdb_sys_writev = 146,
  gdb_sys_getsid = 147,
  gdb_sys_fdatasync = 148,
  gdb_sys_sysctl = 149,
  gdb_sys_mlock = 150,
  gdb_sys_munlock = 151,
  gdb_sys_mlockall = 152,
  gdb_sys_munlockall = 153,
  gdb_sys_sched_setparam = 154,
  gdb_sys_sched_getparam = 155,
  gdb_sys_sched_setscheduler = 156,
  gdb_sys_sched_getscheduler = 157,
  gdb_sys_sched_yield = 158,
  gdb_sys_sched_get_priority_max = 159,
  gdb_sys_sched_get_priority_min = 160,
  gdb_sys_sched_rr_get_interval = 161,
  gdb_sys_nanosleep = 162,
  gdb_sys_mremap = 163,
  gdb_sys_setresuid16 = 164,
  gdb_sys_getresuid16 = 165,
  gdb_sys_vm86 = 166,
  gdb_sys_ni_syscall167 = 167,
  gdb_sys_poll = 168,
  gdb_sys_nfsservctl = 169,
  gdb_sys_setresgid16 = 170,
  gdb_sys_getresgid16 = 171,
  gdb_sys_prctl = 172,
  gdb_sys_rt_sigreturn = 173,
  gdb_sys_rt_sigaction = 174,
  gdb_sys_rt_sigprocmask = 175,
  gdb_sys_rt_sigpending = 176,
  gdb_sys_rt_sigtimedwait = 177,
  gdb_sys_rt_sigqueueinfo = 178,
  gdb_sys_rt_sigsuspend = 179,
  gdb_sys_pread64 = 180,
  gdb_sys_pwrite64 = 181,
  gdb_sys_chown16 = 182,
  gdb_sys_getcwd = 183,
  gdb_sys_capget = 184,
  gdb_sys_capset = 185,
  gdb_sys_sigaltstack = 186,
  gdb_sys_sendfile = 187,
  gdb_sys_ni_syscall188 = 188,
  gdb_sys_ni_syscall189 = 189,
  gdb_sys_vfork = 190,
  gdb_sys_getrlimit = 191,
  gdb_sys_mmap2 = 192,
  gdb_sys_truncate64 = 193,
  gdb_sys_ftruncate64 = 194,
  gdb_sys_stat64 = 195,
  gdb_sys_lstat64 = 196,
  gdb_sys_fstat64 = 197,
  gdb_sys_lchown = 198,
  gdb_sys_getuid = 199,
  gdb_sys_getgid = 200,
  gdb_sys_geteuid = 201,
  gdb_sys_getegid = 202,
  gdb_sys_setreuid = 203,
  gdb_sys_setregid = 204,
  gdb_sys_getgroups = 205,
  gdb_sys_setgroups = 206,
  gdb_sys_fchown = 207,
  gdb_sys_setresuid = 208,
  gdb_sys_getresuid = 209,
  gdb_sys_setresgid = 210,
  gdb_sys_getresgid = 211,
  gdb_sys_chown = 212,
  gdb_sys_setuid = 213,
  gdb_sys_setgid = 214,
  gdb_sys_setfsuid = 215,
  gdb_sys_setfsgid = 216,
  gdb_sys_pivot_root = 217,
  gdb_sys_mincore = 218,
  gdb_sys_madvise = 219,
  gdb_sys_getdents64 = 220,
  gdb_sys_fcntl64 = 221,
  gdb_sys_ni_syscall222 = 222,
  gdb_sys_ni_syscall223 = 223,
  gdb_sys_gettid = 224,
  gdb_sys_readahead = 225,
  gdb_sys_setxattr = 226,
  gdb_sys_lsetxattr = 227,
  gdb_sys_fsetxattr = 228,
  gdb_sys_getxattr = 229,
  gdb_sys_lgetxattr = 230,
  gdb_sys_fgetxattr = 231,
  gdb_sys_listxattr = 232,
  gdb_sys_llistxattr = 233,
  gdb_sys_flistxattr = 234,
  gdb_sys_removexattr = 235,
  gdb_sys_lremovexattr = 236,
  gdb_sys_fremovexattr = 237,
  gdb_sys_tkill = 238,
  gdb_sys_sendfile64 = 239,
  gdb_sys_futex = 240,
  gdb_sys_sched_setaffinity = 241,
  gdb_sys_sched_getaffinity = 242,
  gdb_sys_set_thread_area = 243,
  gdb_sys_get_thread_area = 244,
  gdb_sys_io_setup = 245,
  gdb_sys_io_destroy = 246,
  gdb_sys_io_getevents = 247,
  gdb_sys_io_submit = 248,
  gdb_sys_io_cancel = 249,
  gdb_sys_fadvise64 = 250,
  gdb_sys_ni_syscall251 = 251,
  gdb_sys_exit_group = 252,
  gdb_sys_lookup_dcookie = 253,
  gdb_sys_epoll_create = 254,
  gdb_sys_epoll_ctl = 255,
  gdb_sys_epoll_wait = 256,
  gdb_sys_remap_file_pages = 257,
  gdb_sys_set_tid_address = 258,
  gdb_sys_timer_create = 259,
  gdb_sys_timer_settime = 260,
  gdb_sys_timer_gettime = 261,
  gdb_sys_timer_getoverrun = 262,
  gdb_sys_timer_delete = 263,
  gdb_sys_clock_settime = 264,
  gdb_sys_clock_gettime = 265,
  gdb_sys_clock_getres = 266,
  gdb_sys_clock_nanosleep = 267,
  gdb_sys_statfs64 = 268,
  gdb_sys_fstatfs64 = 269,
  gdb_sys_tgkill = 270,
  gdb_sys_utimes = 271,
  gdb_sys_fadvise64_64 = 272,
  gdb_sys_ni_syscall273 = 273,
  gdb_sys_mbind = 274,
  gdb_sys_get_mempolicy = 275,
  gdb_sys_set_mempolicy = 276,
  gdb_sys_mq_open = 277,
  gdb_sys_mq_unlink = 278,
  gdb_sys_mq_timedsend = 279,
  gdb_sys_mq_timedreceive = 280,
  gdb_sys_mq_notify = 281,
  gdb_sys_mq_getsetattr = 282,
  gdb_sys_kexec_load = 283,
  gdb_sys_waitid = 284,
  gdb_sys_ni_syscall285 = 285,
  gdb_sys_add_key = 286,
  gdb_sys_request_key = 287,
  gdb_sys_keyctl = 288,
  gdb_sys_ioprio_set = 289,
  gdb_sys_ioprio_get = 290,
  gdb_sys_inotify_init = 291,
  gdb_sys_inotify_add_watch = 292,
  gdb_sys_inotify_rm_watch = 293,
  gdb_sys_migrate_pages = 294,
  gdb_sys_openat = 295,
  gdb_sys_mkdirat = 296,
  gdb_sys_mknodat = 297,
  gdb_sys_fchownat = 298,
  gdb_sys_futimesat = 299,
  gdb_sys_fstatat64 = 300,
  gdb_sys_unlinkat = 301,
  gdb_sys_renameat = 302,
  gdb_sys_linkat = 303,
  gdb_sys_symlinkat = 304,
  gdb_sys_readlinkat = 305,
  gdb_sys_fchmodat = 306,
  gdb_sys_faccessat = 307,
  gdb_sys_pselect6 = 308,
  gdb_sys_ppoll = 309,
  gdb_sys_unshare = 310,
  gdb_sys_set_robust_list = 311,
  gdb_sys_get_robust_list = 312,
  gdb_sys_splice = 313,
  gdb_sys_sync_file_range = 314,
  gdb_sys_tee = 315,
  gdb_sys_vmsplice = 316,
  gdb_sys_move_pages = 317,
  gdb_sys_getcpu = 318,
  gdb_sys_epoll_pwait = 319,
  gdb_sys_fallocate = 324,
  gdb_sys_eventfd2 = 328,
  gdb_sys_epoll_create1 = 329,
  gdb_sys_dup3 = 330,
  gdb_sys_pipe2 = 331,
  gdb_sys_inotify_init1 = 332,
  gdb_sys_getrandom = 355,
  gdb_sys_statx = 383,
  gdb_sys_socket = 500,
  gdb_sys_connect = 501,
  gdb_sys_accept = 502,
  gdb_sys_sendto = 503,
  gdb_sys_recvfrom = 504,
  gdb_sys_sendmsg = 505,
  gdb_sys_recvmsg = 506,
  gdb_sys_shutdown = 507,
  gdb_sys_bind = 508,
  gdb_sys_listen = 509,
  gdb_sys_getsockname = 510,
  gdb_sys_getpeername = 511,
  gdb_sys_socketpair = 512,
  gdb_sys_setsockopt = 513,
  gdb_sys_getsockopt = 514,
  gdb_sys_recv = 515,
  gdb_sys_shmget = 520,
  gdb_sys_shmat = 521,
  gdb_sys_shmctl = 522,
  gdb_sys_semget = 523,
  gdb_sys_semop = 524,
  gdb_sys_semctl = 525,
  gdb_sys_shmdt = 527,
  gdb_sys_msgget = 528,
  gdb_sys_msgsnd = 529,
  gdb_sys_msgrcv = 530,
  gdb_sys_msgctl = 531,
  gdb_sys_semtimedop = 532,
  gdb_sys_newfstatat = 540,
};

/* Record a linux syscall.  */

extern int record_linux_system_call (enum gdb_syscall num, 
				     struct regcache *regcache,
				     struct linux_record_tdep *tdep);

#endif /* LINUX_RECORD_H */
