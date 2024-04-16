#if 0 /* Auto generated: sh ./targ-linux.h

sed -n '1,/^#endif/p' targ-linux.h
echo

echo '#include <sys/syscall.h>' | \
bfin-uclinux-gcc -E -dD -P - | \
sed -r -n \
    -e '1istatic CB_TARGET_DEFS_MAP cb_linux_syscall_map[] = {' \
    -e '$i\ \ { 0, -1, -1 }\n};' \
    -e '/#define __NR_/{s:^.* __NR_(.*) (.*):#ifdef CB_SYS_\1\n# define TARGET_LINUX_SYS_\1 \2\n  { "\1", CB_SYS_\1, TARGET_LINUX_SYS_\1 },\n#endif:;p;}'
echo

echo '#include <errno.h>' | \
bfin-uclinux-gcc -E -dD -P - | \
sed -r -n \
    -e '1istatic CB_TARGET_DEFS_MAP cb_linux_errno_map[] = {' \
    -e '$i\ \ { 0, 0, 0 }\n};' \
    -e '/#define E.* [0-9]/{s:^.* (E.*) (.*):#ifdef \1\n# define TARGET_LINUX_\1 \2\n  { "\1", \1, TARGET_LINUX_\1 },\n#endif:;p;}'
echo

echo '#include <fcntl.h>' | \
bfin-uclinux-gcc -E -dD -P - | \
sed -r -n \
    -e '1istatic CB_TARGET_DEFS_MAP cb_linux_open_map[] = {' \
    -e '$i\ \ { 0, -1, -1 }\n};' \
    -e '/#define O.* [0-9]/{s:^.* (O_.*) (.*):#ifdef \1\n# define TARGET_LINUX_\1 \2\n  { "\1", \1, TARGET_LINUX_\1 },\n#endif:;p;}'
echo

# XXX: nothing uses this ?
echo '#include <signal.h>' | \
bfin-uclinux-gcc -E -dD -P - | \
grep -v SIGSTKSZ | \
sed -r -n \
    -e '1istatic CB_TARGET_DEFS_MAP cb_linux_signal_map[] = {' \
    -e '$i\ \ { 0, -1, -1 }\n};' \
    -e '/#define SIG.* [0-9]+$/{s:^.* (SIG.*) (.*):#ifdef \1\n# define TARGET_LINUX_\1 \2\n  { "\1", \1, TARGET_LINUX_\1 },\n#endif:;p;}'

exit 0
*/
#endif

static CB_TARGET_DEFS_MAP cb_linux_syscall_map[] =
{
#ifdef CB_SYS_restart_syscall
# define TARGET_LINUX_SYS_restart_syscall 0
  { "restart_syscall", CB_SYS_restart_syscall, TARGET_LINUX_SYS_restart_syscall },
#endif
#ifdef CB_SYS_exit
# define TARGET_LINUX_SYS_exit 1
  { "exit", CB_SYS_exit, TARGET_LINUX_SYS_exit },
#endif
#ifdef CB_SYS_fork
# define TARGET_LINUX_SYS_fork 2
  { "fork", CB_SYS_fork, TARGET_LINUX_SYS_fork },
#endif
#ifdef CB_SYS_read
# define TARGET_LINUX_SYS_read 3
  { "read", CB_SYS_read, TARGET_LINUX_SYS_read },
#endif
#ifdef CB_SYS_write
# define TARGET_LINUX_SYS_write 4
  { "write", CB_SYS_write, TARGET_LINUX_SYS_write },
#endif
#ifdef CB_SYS_open
# define TARGET_LINUX_SYS_open 5
  { "open", CB_SYS_open, TARGET_LINUX_SYS_open },
#endif
#ifdef CB_SYS_close
# define TARGET_LINUX_SYS_close 6
  { "close", CB_SYS_close, TARGET_LINUX_SYS_close },
#endif
#ifdef CB_SYS_creat
# define TARGET_LINUX_SYS_creat 8
  { "creat", CB_SYS_creat, TARGET_LINUX_SYS_creat },
#endif
#ifdef CB_SYS_link
# define TARGET_LINUX_SYS_link 9
  { "link", CB_SYS_link, TARGET_LINUX_SYS_link },
#endif
#ifdef CB_SYS_unlink
# define TARGET_LINUX_SYS_unlink 10
  { "unlink", CB_SYS_unlink, TARGET_LINUX_SYS_unlink },
#endif
#ifdef CB_SYS_execve
# define TARGET_LINUX_SYS_execve 11
  { "execve", CB_SYS_execve, TARGET_LINUX_SYS_execve },
#endif
#ifdef CB_SYS_chdir
# define TARGET_LINUX_SYS_chdir 12
  { "chdir", CB_SYS_chdir, TARGET_LINUX_SYS_chdir },
#endif
#ifdef CB_SYS_time
# define TARGET_LINUX_SYS_time 13
  { "time", CB_SYS_time, TARGET_LINUX_SYS_time },
#endif
#ifdef CB_SYS_mknod
# define TARGET_LINUX_SYS_mknod 14
  { "mknod", CB_SYS_mknod, TARGET_LINUX_SYS_mknod },
#endif
#ifdef CB_SYS_chmod
# define TARGET_LINUX_SYS_chmod 15
  { "chmod", CB_SYS_chmod, TARGET_LINUX_SYS_chmod },
#endif
#ifdef CB_SYS_chown
# define TARGET_LINUX_SYS_chown 16
  { "chown", CB_SYS_chown, TARGET_LINUX_SYS_chown },
#endif
#ifdef CB_SYS_lseek
# define TARGET_LINUX_SYS_lseek 19
  { "lseek", CB_SYS_lseek, TARGET_LINUX_SYS_lseek },
#endif
#ifdef CB_SYS_getpid
# define TARGET_LINUX_SYS_getpid 20
  { "getpid", CB_SYS_getpid, TARGET_LINUX_SYS_getpid },
#endif
#ifdef CB_SYS_mount
# define TARGET_LINUX_SYS_mount 21
  { "mount", CB_SYS_mount, TARGET_LINUX_SYS_mount },
#endif
#ifdef CB_SYS_setuid
# define TARGET_LINUX_SYS_setuid 23
  { "setuid", CB_SYS_setuid, TARGET_LINUX_SYS_setuid },
#endif
#ifdef CB_SYS_getuid
# define TARGET_LINUX_SYS_getuid 24
  { "getuid", CB_SYS_getuid, TARGET_LINUX_SYS_getuid },
#endif
#ifdef CB_SYS_stime
# define TARGET_LINUX_SYS_stime 25
  { "stime", CB_SYS_stime, TARGET_LINUX_SYS_stime },
#endif
#ifdef CB_SYS_ptrace
# define TARGET_LINUX_SYS_ptrace 26
  { "ptrace", CB_SYS_ptrace, TARGET_LINUX_SYS_ptrace },
#endif
#ifdef CB_SYS_alarm
# define TARGET_LINUX_SYS_alarm 27
  { "alarm", CB_SYS_alarm, TARGET_LINUX_SYS_alarm },
#endif
#ifdef CB_SYS_pause
# define TARGET_LINUX_SYS_pause 29
  { "pause", CB_SYS_pause, TARGET_LINUX_SYS_pause },
#endif
#ifdef CB_SYS_access
# define TARGET_LINUX_SYS_access 33
  { "access", CB_SYS_access, TARGET_LINUX_SYS_access },
#endif
#ifdef CB_SYS_nice
# define TARGET_LINUX_SYS_nice 34
  { "nice", CB_SYS_nice, TARGET_LINUX_SYS_nice },
#endif
#ifdef CB_SYS_sync
# define TARGET_LINUX_SYS_sync 36
  { "sync", CB_SYS_sync, TARGET_LINUX_SYS_sync },
#endif
#ifdef CB_SYS_kill
# define TARGET_LINUX_SYS_kill 37
  { "kill", CB_SYS_kill, TARGET_LINUX_SYS_kill },
#endif
#ifdef CB_SYS_rename
# define TARGET_LINUX_SYS_rename 38
  { "rename", CB_SYS_rename, TARGET_LINUX_SYS_rename },
#endif
#ifdef CB_SYS_mkdir
# define TARGET_LINUX_SYS_mkdir 39
  { "mkdir", CB_SYS_mkdir, TARGET_LINUX_SYS_mkdir },
#endif
#ifdef CB_SYS_rmdir
# define TARGET_LINUX_SYS_rmdir 40
  { "rmdir", CB_SYS_rmdir, TARGET_LINUX_SYS_rmdir },
#endif
#ifdef CB_SYS_dup
# define TARGET_LINUX_SYS_dup 41
  { "dup", CB_SYS_dup, TARGET_LINUX_SYS_dup },
#endif
#ifdef CB_SYS_pipe
# define TARGET_LINUX_SYS_pipe 42
  { "pipe", CB_SYS_pipe, TARGET_LINUX_SYS_pipe },
#endif
#ifdef CB_SYS_times
# define TARGET_LINUX_SYS_times 43
  { "times", CB_SYS_times, TARGET_LINUX_SYS_times },
#endif
#ifdef CB_SYS_brk
# define TARGET_LINUX_SYS_brk 45
  { "brk", CB_SYS_brk, TARGET_LINUX_SYS_brk },
#endif
#ifdef CB_SYS_setgid
# define TARGET_LINUX_SYS_setgid 46
  { "setgid", CB_SYS_setgid, TARGET_LINUX_SYS_setgid },
#endif
#ifdef CB_SYS_getgid
# define TARGET_LINUX_SYS_getgid 47
  { "getgid", CB_SYS_getgid, TARGET_LINUX_SYS_getgid },
#endif
#ifdef CB_SYS_geteuid
# define TARGET_LINUX_SYS_geteuid 49
  { "geteuid", CB_SYS_geteuid, TARGET_LINUX_SYS_geteuid },
#endif
#ifdef CB_SYS_getegid
# define TARGET_LINUX_SYS_getegid 50
  { "getegid", CB_SYS_getegid, TARGET_LINUX_SYS_getegid },
#endif
#ifdef CB_SYS_acct
# define TARGET_LINUX_SYS_acct 51
  { "acct", CB_SYS_acct, TARGET_LINUX_SYS_acct },
#endif
#ifdef CB_SYS_umount2
# define TARGET_LINUX_SYS_umount2 52
  { "umount2", CB_SYS_umount2, TARGET_LINUX_SYS_umount2 },
#endif
#ifdef CB_SYS_ioctl
# define TARGET_LINUX_SYS_ioctl 54
  { "ioctl", CB_SYS_ioctl, TARGET_LINUX_SYS_ioctl },
#endif
#ifdef CB_SYS_fcntl
# define TARGET_LINUX_SYS_fcntl 55
  { "fcntl", CB_SYS_fcntl, TARGET_LINUX_SYS_fcntl },
#endif
#ifdef CB_SYS_setpgid
# define TARGET_LINUX_SYS_setpgid 57
  { "setpgid", CB_SYS_setpgid, TARGET_LINUX_SYS_setpgid },
#endif
#ifdef CB_SYS_umask
# define TARGET_LINUX_SYS_umask 60
  { "umask", CB_SYS_umask, TARGET_LINUX_SYS_umask },
#endif
#ifdef CB_SYS_chroot
# define TARGET_LINUX_SYS_chroot 61
  { "chroot", CB_SYS_chroot, TARGET_LINUX_SYS_chroot },
#endif
#ifdef CB_SYS_ustat
# define TARGET_LINUX_SYS_ustat 62
  { "ustat", CB_SYS_ustat, TARGET_LINUX_SYS_ustat },
#endif
#ifdef CB_SYS_dup2
# define TARGET_LINUX_SYS_dup2 63
  { "dup2", CB_SYS_dup2, TARGET_LINUX_SYS_dup2 },
#endif
#ifdef CB_SYS_getppid
# define TARGET_LINUX_SYS_getppid 64
  { "getppid", CB_SYS_getppid, TARGET_LINUX_SYS_getppid },
#endif
#ifdef CB_SYS_getpgrp
# define TARGET_LINUX_SYS_getpgrp 65
  { "getpgrp", CB_SYS_getpgrp, TARGET_LINUX_SYS_getpgrp },
#endif
#ifdef CB_SYS_setsid
# define TARGET_LINUX_SYS_setsid 66
  { "setsid", CB_SYS_setsid, TARGET_LINUX_SYS_setsid },
#endif
#ifdef CB_SYS_sgetmask
# define TARGET_LINUX_SYS_sgetmask 68
  { "sgetmask", CB_SYS_sgetmask, TARGET_LINUX_SYS_sgetmask },
#endif
#ifdef CB_SYS_ssetmask
# define TARGET_LINUX_SYS_ssetmask 69
  { "ssetmask", CB_SYS_ssetmask, TARGET_LINUX_SYS_ssetmask },
#endif
#ifdef CB_SYS_setreuid
# define TARGET_LINUX_SYS_setreuid 70
  { "setreuid", CB_SYS_setreuid, TARGET_LINUX_SYS_setreuid },
#endif
#ifdef CB_SYS_setregid
# define TARGET_LINUX_SYS_setregid 71
  { "setregid", CB_SYS_setregid, TARGET_LINUX_SYS_setregid },
#endif
#ifdef CB_SYS_sethostname
# define TARGET_LINUX_SYS_sethostname 74
  { "sethostname", CB_SYS_sethostname, TARGET_LINUX_SYS_sethostname },
#endif
#ifdef CB_SYS_setrlimit
# define TARGET_LINUX_SYS_setrlimit 75
  { "setrlimit", CB_SYS_setrlimit, TARGET_LINUX_SYS_setrlimit },
#endif
#ifdef CB_SYS_getrusage
# define TARGET_LINUX_SYS_getrusage 77
  { "getrusage", CB_SYS_getrusage, TARGET_LINUX_SYS_getrusage },
#endif
#ifdef CB_SYS_gettimeofday
# define TARGET_LINUX_SYS_gettimeofday 78
  { "gettimeofday", CB_SYS_gettimeofday, TARGET_LINUX_SYS_gettimeofday },
#endif
#ifdef CB_SYS_settimeofday
# define TARGET_LINUX_SYS_settimeofday 79
  { "settimeofday", CB_SYS_settimeofday, TARGET_LINUX_SYS_settimeofday },
#endif
#ifdef CB_SYS_getgroups
# define TARGET_LINUX_SYS_getgroups 80
  { "getgroups", CB_SYS_getgroups, TARGET_LINUX_SYS_getgroups },
#endif
#ifdef CB_SYS_setgroups
# define TARGET_LINUX_SYS_setgroups 81
  { "setgroups", CB_SYS_setgroups, TARGET_LINUX_SYS_setgroups },
#endif
#ifdef CB_SYS_symlink
# define TARGET_LINUX_SYS_symlink 83
  { "symlink", CB_SYS_symlink, TARGET_LINUX_SYS_symlink },
#endif
#ifdef CB_SYS_readlink
# define TARGET_LINUX_SYS_readlink 85
  { "readlink", CB_SYS_readlink, TARGET_LINUX_SYS_readlink },
#endif
#ifdef CB_SYS_reboot
# define TARGET_LINUX_SYS_reboot 88
  { "reboot", CB_SYS_reboot, TARGET_LINUX_SYS_reboot },
#endif
#ifdef CB_SYS_munmap
# define TARGET_LINUX_SYS_munmap 91
  { "munmap", CB_SYS_munmap, TARGET_LINUX_SYS_munmap },
#endif
#ifdef CB_SYS_truncate
# define TARGET_LINUX_SYS_truncate 92
  { "truncate", CB_SYS_truncate, TARGET_LINUX_SYS_truncate },
#endif
#ifdef CB_SYS_ftruncate
# define TARGET_LINUX_SYS_ftruncate 93
  { "ftruncate", CB_SYS_ftruncate, TARGET_LINUX_SYS_ftruncate },
#endif
#ifdef CB_SYS_fchmod
# define TARGET_LINUX_SYS_fchmod 94
  { "fchmod", CB_SYS_fchmod, TARGET_LINUX_SYS_fchmod },
#endif
#ifdef CB_SYS_fchown
# define TARGET_LINUX_SYS_fchown 95
  { "fchown", CB_SYS_fchown, TARGET_LINUX_SYS_fchown },
#endif
#ifdef CB_SYS_getpriority
# define TARGET_LINUX_SYS_getpriority 96
  { "getpriority", CB_SYS_getpriority, TARGET_LINUX_SYS_getpriority },
#endif
#ifdef CB_SYS_setpriority
# define TARGET_LINUX_SYS_setpriority 97
  { "setpriority", CB_SYS_setpriority, TARGET_LINUX_SYS_setpriority },
#endif
#ifdef CB_SYS_statfs
# define TARGET_LINUX_SYS_statfs 99
  { "statfs", CB_SYS_statfs, TARGET_LINUX_SYS_statfs },
#endif
#ifdef CB_SYS_fstatfs
# define TARGET_LINUX_SYS_fstatfs 100
  { "fstatfs", CB_SYS_fstatfs, TARGET_LINUX_SYS_fstatfs },
#endif
#ifdef CB_SYS_syslog
# define TARGET_LINUX_SYS_syslog 103
  { "syslog", CB_SYS_syslog, TARGET_LINUX_SYS_syslog },
#endif
#ifdef CB_SYS_setitimer
# define TARGET_LINUX_SYS_setitimer 104
  { "setitimer", CB_SYS_setitimer, TARGET_LINUX_SYS_setitimer },
#endif
#ifdef CB_SYS_getitimer
# define TARGET_LINUX_SYS_getitimer 105
  { "getitimer", CB_SYS_getitimer, TARGET_LINUX_SYS_getitimer },
#endif
#ifdef CB_SYS_stat
# define TARGET_LINUX_SYS_stat 106
  { "stat", CB_SYS_stat, TARGET_LINUX_SYS_stat },
#endif
#ifdef CB_SYS_lstat
# define TARGET_LINUX_SYS_lstat 107
  { "lstat", CB_SYS_lstat, TARGET_LINUX_SYS_lstat },
#endif
#ifdef CB_SYS_fstat
# define TARGET_LINUX_SYS_fstat 108
  { "fstat", CB_SYS_fstat, TARGET_LINUX_SYS_fstat },
#endif
#ifdef CB_SYS_vhangup
# define TARGET_LINUX_SYS_vhangup 111
  { "vhangup", CB_SYS_vhangup, TARGET_LINUX_SYS_vhangup },
#endif
#ifdef CB_SYS_wait4
# define TARGET_LINUX_SYS_wait4 114
  { "wait4", CB_SYS_wait4, TARGET_LINUX_SYS_wait4 },
#endif
#ifdef CB_SYS_sysinfo
# define TARGET_LINUX_SYS_sysinfo 116
  { "sysinfo", CB_SYS_sysinfo, TARGET_LINUX_SYS_sysinfo },
#endif
#ifdef CB_SYS_fsync
# define TARGET_LINUX_SYS_fsync 118
  { "fsync", CB_SYS_fsync, TARGET_LINUX_SYS_fsync },
#endif
#ifdef CB_SYS_clone
# define TARGET_LINUX_SYS_clone 120
  { "clone", CB_SYS_clone, TARGET_LINUX_SYS_clone },
#endif
#ifdef CB_SYS_setdomainname
# define TARGET_LINUX_SYS_setdomainname 121
  { "setdomainname", CB_SYS_setdomainname, TARGET_LINUX_SYS_setdomainname },
#endif
#ifdef CB_SYS_uname
# define TARGET_LINUX_SYS_uname 122
  { "uname", CB_SYS_uname, TARGET_LINUX_SYS_uname },
#endif
#ifdef CB_SYS_adjtimex
# define TARGET_LINUX_SYS_adjtimex 124
  { "adjtimex", CB_SYS_adjtimex, TARGET_LINUX_SYS_adjtimex },
#endif
#ifdef CB_SYS_mprotect
# define TARGET_LINUX_SYS_mprotect 125
  { "mprotect", CB_SYS_mprotect, TARGET_LINUX_SYS_mprotect },
#endif
#ifdef CB_SYS_init_module
# define TARGET_LINUX_SYS_init_module 128
  { "init_module", CB_SYS_init_module, TARGET_LINUX_SYS_init_module },
#endif
#ifdef CB_SYS_delete_module
# define TARGET_LINUX_SYS_delete_module 129
  { "delete_module", CB_SYS_delete_module, TARGET_LINUX_SYS_delete_module },
#endif
#ifdef CB_SYS_quotactl
# define TARGET_LINUX_SYS_quotactl 131
  { "quotactl", CB_SYS_quotactl, TARGET_LINUX_SYS_quotactl },
#endif
#ifdef CB_SYS_getpgid
# define TARGET_LINUX_SYS_getpgid 132
  { "getpgid", CB_SYS_getpgid, TARGET_LINUX_SYS_getpgid },
#endif
#ifdef CB_SYS_fchdir
# define TARGET_LINUX_SYS_fchdir 133
  { "fchdir", CB_SYS_fchdir, TARGET_LINUX_SYS_fchdir },
#endif
#ifdef CB_SYS_bdflush
# define TARGET_LINUX_SYS_bdflush 134
  { "bdflush", CB_SYS_bdflush, TARGET_LINUX_SYS_bdflush },
#endif
#ifdef CB_SYS_personality
# define TARGET_LINUX_SYS_personality 136
  { "personality", CB_SYS_personality, TARGET_LINUX_SYS_personality },
#endif
#ifdef CB_SYS_setfsuid
# define TARGET_LINUX_SYS_setfsuid 138
  { "setfsuid", CB_SYS_setfsuid, TARGET_LINUX_SYS_setfsuid },
#endif
#ifdef CB_SYS_setfsgid
# define TARGET_LINUX_SYS_setfsgid 139
  { "setfsgid", CB_SYS_setfsgid, TARGET_LINUX_SYS_setfsgid },
#endif
#ifdef CB_SYS__llseek
# define TARGET_LINUX_SYS__llseek 140
  { "_llseek", CB_SYS__llseek, TARGET_LINUX_SYS__llseek },
#endif
#ifdef CB_SYS_getdents
# define TARGET_LINUX_SYS_getdents 141
  { "getdents", CB_SYS_getdents, TARGET_LINUX_SYS_getdents },
#endif
#ifdef CB_SYS_flock
# define TARGET_LINUX_SYS_flock 143
  { "flock", CB_SYS_flock, TARGET_LINUX_SYS_flock },
#endif
#ifdef CB_SYS_readv
# define TARGET_LINUX_SYS_readv 145
  { "readv", CB_SYS_readv, TARGET_LINUX_SYS_readv },
#endif
#ifdef CB_SYS_writev
# define TARGET_LINUX_SYS_writev 146
  { "writev", CB_SYS_writev, TARGET_LINUX_SYS_writev },
#endif
#ifdef CB_SYS_getsid
# define TARGET_LINUX_SYS_getsid 147
  { "getsid", CB_SYS_getsid, TARGET_LINUX_SYS_getsid },
#endif
#ifdef CB_SYS_fdatasync
# define TARGET_LINUX_SYS_fdatasync 148
  { "fdatasync", CB_SYS_fdatasync, TARGET_LINUX_SYS_fdatasync },
#endif
#ifdef CB_SYS__sysctl
# define TARGET_LINUX_SYS__sysctl 149
  { "_sysctl", CB_SYS__sysctl, TARGET_LINUX_SYS__sysctl },
#endif
#ifdef CB_SYS_sched_setparam
# define TARGET_LINUX_SYS_sched_setparam 154
  { "sched_setparam", CB_SYS_sched_setparam, TARGET_LINUX_SYS_sched_setparam },
#endif
#ifdef CB_SYS_sched_getparam
# define TARGET_LINUX_SYS_sched_getparam 155
  { "sched_getparam", CB_SYS_sched_getparam, TARGET_LINUX_SYS_sched_getparam },
#endif
#ifdef CB_SYS_sched_setscheduler
# define TARGET_LINUX_SYS_sched_setscheduler 156
  { "sched_setscheduler", CB_SYS_sched_setscheduler, TARGET_LINUX_SYS_sched_setscheduler },
#endif
#ifdef CB_SYS_sched_getscheduler
# define TARGET_LINUX_SYS_sched_getscheduler 157
  { "sched_getscheduler", CB_SYS_sched_getscheduler, TARGET_LINUX_SYS_sched_getscheduler },
#endif
#ifdef CB_SYS_sched_yield
# define TARGET_LINUX_SYS_sched_yield 158
  { "sched_yield", CB_SYS_sched_yield, TARGET_LINUX_SYS_sched_yield },
#endif
#ifdef CB_SYS_sched_get_priority_max
# define TARGET_LINUX_SYS_sched_get_priority_max 159
  { "sched_get_priority_max", CB_SYS_sched_get_priority_max, TARGET_LINUX_SYS_sched_get_priority_max },
#endif
#ifdef CB_SYS_sched_get_priority_min
# define TARGET_LINUX_SYS_sched_get_priority_min 160
  { "sched_get_priority_min", CB_SYS_sched_get_priority_min, TARGET_LINUX_SYS_sched_get_priority_min },
#endif
#ifdef CB_SYS_sched_rr_get_interval
# define TARGET_LINUX_SYS_sched_rr_get_interval 161
  { "sched_rr_get_interval", CB_SYS_sched_rr_get_interval, TARGET_LINUX_SYS_sched_rr_get_interval },
#endif
#ifdef CB_SYS_nanosleep
# define TARGET_LINUX_SYS_nanosleep 162
  { "nanosleep", CB_SYS_nanosleep, TARGET_LINUX_SYS_nanosleep },
#endif
#ifdef CB_SYS_mremap
# define TARGET_LINUX_SYS_mremap 163
  { "mremap", CB_SYS_mremap, TARGET_LINUX_SYS_mremap },
#endif
#ifdef CB_SYS_setresuid
# define TARGET_LINUX_SYS_setresuid 164
  { "setresuid", CB_SYS_setresuid, TARGET_LINUX_SYS_setresuid },
#endif
#ifdef CB_SYS_getresuid
# define TARGET_LINUX_SYS_getresuid 165
  { "getresuid", CB_SYS_getresuid, TARGET_LINUX_SYS_getresuid },
#endif
#ifdef CB_SYS_nfsservctl
# define TARGET_LINUX_SYS_nfsservctl 169
  { "nfsservctl", CB_SYS_nfsservctl, TARGET_LINUX_SYS_nfsservctl },
#endif
#ifdef CB_SYS_setresgid
# define TARGET_LINUX_SYS_setresgid 170
  { "setresgid", CB_SYS_setresgid, TARGET_LINUX_SYS_setresgid },
#endif
#ifdef CB_SYS_getresgid
# define TARGET_LINUX_SYS_getresgid 171
  { "getresgid", CB_SYS_getresgid, TARGET_LINUX_SYS_getresgid },
#endif
#ifdef CB_SYS_prctl
# define TARGET_LINUX_SYS_prctl 172
  { "prctl", CB_SYS_prctl, TARGET_LINUX_SYS_prctl },
#endif
#ifdef CB_SYS_rt_sigreturn
# define TARGET_LINUX_SYS_rt_sigreturn 173
  { "rt_sigreturn", CB_SYS_rt_sigreturn, TARGET_LINUX_SYS_rt_sigreturn },
#endif
#ifdef CB_SYS_rt_sigaction
# define TARGET_LINUX_SYS_rt_sigaction 174
  { "rt_sigaction", CB_SYS_rt_sigaction, TARGET_LINUX_SYS_rt_sigaction },
#endif
#ifdef CB_SYS_rt_sigprocmask
# define TARGET_LINUX_SYS_rt_sigprocmask 175
  { "rt_sigprocmask", CB_SYS_rt_sigprocmask, TARGET_LINUX_SYS_rt_sigprocmask },
#endif
#ifdef CB_SYS_rt_sigpending
# define TARGET_LINUX_SYS_rt_sigpending 176
  { "rt_sigpending", CB_SYS_rt_sigpending, TARGET_LINUX_SYS_rt_sigpending },
#endif
#ifdef CB_SYS_rt_sigtimedwait
# define TARGET_LINUX_SYS_rt_sigtimedwait 177
  { "rt_sigtimedwait", CB_SYS_rt_sigtimedwait, TARGET_LINUX_SYS_rt_sigtimedwait },
#endif
#ifdef CB_SYS_rt_sigqueueinfo
# define TARGET_LINUX_SYS_rt_sigqueueinfo 178
  { "rt_sigqueueinfo", CB_SYS_rt_sigqueueinfo, TARGET_LINUX_SYS_rt_sigqueueinfo },
#endif
#ifdef CB_SYS_rt_sigsuspend
# define TARGET_LINUX_SYS_rt_sigsuspend 179
  { "rt_sigsuspend", CB_SYS_rt_sigsuspend, TARGET_LINUX_SYS_rt_sigsuspend },
#endif
#ifdef CB_SYS_pread
# define TARGET_LINUX_SYS_pread 180
  { "pread", CB_SYS_pread, TARGET_LINUX_SYS_pread },
#endif
#ifdef CB_SYS_pwrite
# define TARGET_LINUX_SYS_pwrite 181
  { "pwrite", CB_SYS_pwrite, TARGET_LINUX_SYS_pwrite },
#endif
#ifdef CB_SYS_lchown
# define TARGET_LINUX_SYS_lchown 182
  { "lchown", CB_SYS_lchown, TARGET_LINUX_SYS_lchown },
#endif
#ifdef CB_SYS_getcwd
# define TARGET_LINUX_SYS_getcwd 183
  { "getcwd", CB_SYS_getcwd, TARGET_LINUX_SYS_getcwd },
#endif
#ifdef CB_SYS_capget
# define TARGET_LINUX_SYS_capget 184
  { "capget", CB_SYS_capget, TARGET_LINUX_SYS_capget },
#endif
#ifdef CB_SYS_capset
# define TARGET_LINUX_SYS_capset 185
  { "capset", CB_SYS_capset, TARGET_LINUX_SYS_capset },
#endif
#ifdef CB_SYS_sigaltstack
# define TARGET_LINUX_SYS_sigaltstack 186
  { "sigaltstack", CB_SYS_sigaltstack, TARGET_LINUX_SYS_sigaltstack },
#endif
#ifdef CB_SYS_sendfile
# define TARGET_LINUX_SYS_sendfile 187
  { "sendfile", CB_SYS_sendfile, TARGET_LINUX_SYS_sendfile },
#endif
#ifdef CB_SYS_vfork
# define TARGET_LINUX_SYS_vfork 190
  { "vfork", CB_SYS_vfork, TARGET_LINUX_SYS_vfork },
#endif
#ifdef CB_SYS_getrlimit
# define TARGET_LINUX_SYS_getrlimit 191
  { "getrlimit", CB_SYS_getrlimit, TARGET_LINUX_SYS_getrlimit },
#endif
#ifdef CB_SYS_mmap2
# define TARGET_LINUX_SYS_mmap2 192
  { "mmap2", CB_SYS_mmap2, TARGET_LINUX_SYS_mmap2 },
#endif
#ifdef CB_SYS_truncate64
# define TARGET_LINUX_SYS_truncate64 193
  { "truncate64", CB_SYS_truncate64, TARGET_LINUX_SYS_truncate64 },
#endif
#ifdef CB_SYS_ftruncate64
# define TARGET_LINUX_SYS_ftruncate64 194
  { "ftruncate64", CB_SYS_ftruncate64, TARGET_LINUX_SYS_ftruncate64 },
#endif
#ifdef CB_SYS_stat64
# define TARGET_LINUX_SYS_stat64 195
  { "stat64", CB_SYS_stat64, TARGET_LINUX_SYS_stat64 },
#endif
#ifdef CB_SYS_lstat64
# define TARGET_LINUX_SYS_lstat64 196
  { "lstat64", CB_SYS_lstat64, TARGET_LINUX_SYS_lstat64 },
#endif
#ifdef CB_SYS_fstat64
# define TARGET_LINUX_SYS_fstat64 197
  { "fstat64", CB_SYS_fstat64, TARGET_LINUX_SYS_fstat64 },
#endif
#ifdef CB_SYS_chown32
# define TARGET_LINUX_SYS_chown32 198
  { "chown32", CB_SYS_chown32, TARGET_LINUX_SYS_chown32 },
#endif
#ifdef CB_SYS_getuid32
# define TARGET_LINUX_SYS_getuid32 199
  { "getuid32", CB_SYS_getuid32, TARGET_LINUX_SYS_getuid32 },
#endif
#ifdef CB_SYS_getgid32
# define TARGET_LINUX_SYS_getgid32 200
  { "getgid32", CB_SYS_getgid32, TARGET_LINUX_SYS_getgid32 },
#endif
#ifdef CB_SYS_geteuid32
# define TARGET_LINUX_SYS_geteuid32 201
  { "geteuid32", CB_SYS_geteuid32, TARGET_LINUX_SYS_geteuid32 },
#endif
#ifdef CB_SYS_getegid32
# define TARGET_LINUX_SYS_getegid32 202
  { "getegid32", CB_SYS_getegid32, TARGET_LINUX_SYS_getegid32 },
#endif
#ifdef CB_SYS_setreuid32
# define TARGET_LINUX_SYS_setreuid32 203
  { "setreuid32", CB_SYS_setreuid32, TARGET_LINUX_SYS_setreuid32 },
#endif
#ifdef CB_SYS_setregid32
# define TARGET_LINUX_SYS_setregid32 204
  { "setregid32", CB_SYS_setregid32, TARGET_LINUX_SYS_setregid32 },
#endif
#ifdef CB_SYS_getgroups32
# define TARGET_LINUX_SYS_getgroups32 205
  { "getgroups32", CB_SYS_getgroups32, TARGET_LINUX_SYS_getgroups32 },
#endif
#ifdef CB_SYS_setgroups32
# define TARGET_LINUX_SYS_setgroups32 206
  { "setgroups32", CB_SYS_setgroups32, TARGET_LINUX_SYS_setgroups32 },
#endif
#ifdef CB_SYS_fchown32
# define TARGET_LINUX_SYS_fchown32 207
  { "fchown32", CB_SYS_fchown32, TARGET_LINUX_SYS_fchown32 },
#endif
#ifdef CB_SYS_setresuid32
# define TARGET_LINUX_SYS_setresuid32 208
  { "setresuid32", CB_SYS_setresuid32, TARGET_LINUX_SYS_setresuid32 },
#endif
#ifdef CB_SYS_getresuid32
# define TARGET_LINUX_SYS_getresuid32 209
  { "getresuid32", CB_SYS_getresuid32, TARGET_LINUX_SYS_getresuid32 },
#endif
#ifdef CB_SYS_setresgid32
# define TARGET_LINUX_SYS_setresgid32 210
  { "setresgid32", CB_SYS_setresgid32, TARGET_LINUX_SYS_setresgid32 },
#endif
#ifdef CB_SYS_getresgid32
# define TARGET_LINUX_SYS_getresgid32 211
  { "getresgid32", CB_SYS_getresgid32, TARGET_LINUX_SYS_getresgid32 },
#endif
#ifdef CB_SYS_lchown32
# define TARGET_LINUX_SYS_lchown32 212
  { "lchown32", CB_SYS_lchown32, TARGET_LINUX_SYS_lchown32 },
#endif
#ifdef CB_SYS_setuid32
# define TARGET_LINUX_SYS_setuid32 213
  { "setuid32", CB_SYS_setuid32, TARGET_LINUX_SYS_setuid32 },
#endif
#ifdef CB_SYS_setgid32
# define TARGET_LINUX_SYS_setgid32 214
  { "setgid32", CB_SYS_setgid32, TARGET_LINUX_SYS_setgid32 },
#endif
#ifdef CB_SYS_setfsuid32
# define TARGET_LINUX_SYS_setfsuid32 215
  { "setfsuid32", CB_SYS_setfsuid32, TARGET_LINUX_SYS_setfsuid32 },
#endif
#ifdef CB_SYS_setfsgid32
# define TARGET_LINUX_SYS_setfsgid32 216
  { "setfsgid32", CB_SYS_setfsgid32, TARGET_LINUX_SYS_setfsgid32 },
#endif
#ifdef CB_SYS_pivot_root
# define TARGET_LINUX_SYS_pivot_root 217
  { "pivot_root", CB_SYS_pivot_root, TARGET_LINUX_SYS_pivot_root },
#endif
#ifdef CB_SYS_getdents64
# define TARGET_LINUX_SYS_getdents64 220
  { "getdents64", CB_SYS_getdents64, TARGET_LINUX_SYS_getdents64 },
#endif
#ifdef CB_SYS_fcntl64
# define TARGET_LINUX_SYS_fcntl64 221
  { "fcntl64", CB_SYS_fcntl64, TARGET_LINUX_SYS_fcntl64 },
#endif
#ifdef CB_SYS_gettid
# define TARGET_LINUX_SYS_gettid 224
  { "gettid", CB_SYS_gettid, TARGET_LINUX_SYS_gettid },
#endif
#ifdef CB_SYS_readahead
# define TARGET_LINUX_SYS_readahead 225
  { "readahead", CB_SYS_readahead, TARGET_LINUX_SYS_readahead },
#endif
#ifdef CB_SYS_setxattr
# define TARGET_LINUX_SYS_setxattr 226
  { "setxattr", CB_SYS_setxattr, TARGET_LINUX_SYS_setxattr },
#endif
#ifdef CB_SYS_lsetxattr
# define TARGET_LINUX_SYS_lsetxattr 227
  { "lsetxattr", CB_SYS_lsetxattr, TARGET_LINUX_SYS_lsetxattr },
#endif
#ifdef CB_SYS_fsetxattr
# define TARGET_LINUX_SYS_fsetxattr 228
  { "fsetxattr", CB_SYS_fsetxattr, TARGET_LINUX_SYS_fsetxattr },
#endif
#ifdef CB_SYS_getxattr
# define TARGET_LINUX_SYS_getxattr 229
  { "getxattr", CB_SYS_getxattr, TARGET_LINUX_SYS_getxattr },
#endif
#ifdef CB_SYS_lgetxattr
# define TARGET_LINUX_SYS_lgetxattr 230
  { "lgetxattr", CB_SYS_lgetxattr, TARGET_LINUX_SYS_lgetxattr },
#endif
#ifdef CB_SYS_fgetxattr
# define TARGET_LINUX_SYS_fgetxattr 231
  { "fgetxattr", CB_SYS_fgetxattr, TARGET_LINUX_SYS_fgetxattr },
#endif
#ifdef CB_SYS_listxattr
# define TARGET_LINUX_SYS_listxattr 232
  { "listxattr", CB_SYS_listxattr, TARGET_LINUX_SYS_listxattr },
#endif
#ifdef CB_SYS_llistxattr
# define TARGET_LINUX_SYS_llistxattr 233
  { "llistxattr", CB_SYS_llistxattr, TARGET_LINUX_SYS_llistxattr },
#endif
#ifdef CB_SYS_flistxattr
# define TARGET_LINUX_SYS_flistxattr 234
  { "flistxattr", CB_SYS_flistxattr, TARGET_LINUX_SYS_flistxattr },
#endif
#ifdef CB_SYS_removexattr
# define TARGET_LINUX_SYS_removexattr 235
  { "removexattr", CB_SYS_removexattr, TARGET_LINUX_SYS_removexattr },
#endif
#ifdef CB_SYS_lremovexattr
# define TARGET_LINUX_SYS_lremovexattr 236
  { "lremovexattr", CB_SYS_lremovexattr, TARGET_LINUX_SYS_lremovexattr },
#endif
#ifdef CB_SYS_fremovexattr
# define TARGET_LINUX_SYS_fremovexattr 237
  { "fremovexattr", CB_SYS_fremovexattr, TARGET_LINUX_SYS_fremovexattr },
#endif
#ifdef CB_SYS_tkill
# define TARGET_LINUX_SYS_tkill 238
  { "tkill", CB_SYS_tkill, TARGET_LINUX_SYS_tkill },
#endif
#ifdef CB_SYS_sendfile64
# define TARGET_LINUX_SYS_sendfile64 239
  { "sendfile64", CB_SYS_sendfile64, TARGET_LINUX_SYS_sendfile64 },
#endif
#ifdef CB_SYS_futex
# define TARGET_LINUX_SYS_futex 240
  { "futex", CB_SYS_futex, TARGET_LINUX_SYS_futex },
#endif
#ifdef CB_SYS_sched_setaffinity
# define TARGET_LINUX_SYS_sched_setaffinity 241
  { "sched_setaffinity", CB_SYS_sched_setaffinity, TARGET_LINUX_SYS_sched_setaffinity },
#endif
#ifdef CB_SYS_sched_getaffinity
# define TARGET_LINUX_SYS_sched_getaffinity 242
  { "sched_getaffinity", CB_SYS_sched_getaffinity, TARGET_LINUX_SYS_sched_getaffinity },
#endif
#ifdef CB_SYS_io_setup
# define TARGET_LINUX_SYS_io_setup 245
  { "io_setup", CB_SYS_io_setup, TARGET_LINUX_SYS_io_setup },
#endif
#ifdef CB_SYS_io_destroy
# define TARGET_LINUX_SYS_io_destroy 246
  { "io_destroy", CB_SYS_io_destroy, TARGET_LINUX_SYS_io_destroy },
#endif
#ifdef CB_SYS_io_getevents
# define TARGET_LINUX_SYS_io_getevents 247
  { "io_getevents", CB_SYS_io_getevents, TARGET_LINUX_SYS_io_getevents },
#endif
#ifdef CB_SYS_io_submit
# define TARGET_LINUX_SYS_io_submit 248
  { "io_submit", CB_SYS_io_submit, TARGET_LINUX_SYS_io_submit },
#endif
#ifdef CB_SYS_io_cancel
# define TARGET_LINUX_SYS_io_cancel 249
  { "io_cancel", CB_SYS_io_cancel, TARGET_LINUX_SYS_io_cancel },
#endif
#ifdef CB_SYS_exit_group
# define TARGET_LINUX_SYS_exit_group 252
  { "exit_group", CB_SYS_exit_group, TARGET_LINUX_SYS_exit_group },
#endif
#ifdef CB_SYS_lookup_dcookie
# define TARGET_LINUX_SYS_lookup_dcookie 253
  { "lookup_dcookie", CB_SYS_lookup_dcookie, TARGET_LINUX_SYS_lookup_dcookie },
#endif
#ifdef CB_SYS_bfin_spinlock
# define TARGET_LINUX_SYS_bfin_spinlock 254
  { "bfin_spinlock", CB_SYS_bfin_spinlock, TARGET_LINUX_SYS_bfin_spinlock },
#endif
#ifdef CB_SYS_epoll_create
# define TARGET_LINUX_SYS_epoll_create 255
  { "epoll_create", CB_SYS_epoll_create, TARGET_LINUX_SYS_epoll_create },
#endif
#ifdef CB_SYS_epoll_ctl
# define TARGET_LINUX_SYS_epoll_ctl 256
  { "epoll_ctl", CB_SYS_epoll_ctl, TARGET_LINUX_SYS_epoll_ctl },
#endif
#ifdef CB_SYS_epoll_wait
# define TARGET_LINUX_SYS_epoll_wait 257
  { "epoll_wait", CB_SYS_epoll_wait, TARGET_LINUX_SYS_epoll_wait },
#endif
#ifdef CB_SYS_set_tid_address
# define TARGET_LINUX_SYS_set_tid_address 259
  { "set_tid_address", CB_SYS_set_tid_address, TARGET_LINUX_SYS_set_tid_address },
#endif
#ifdef CB_SYS_timer_create
# define TARGET_LINUX_SYS_timer_create 260
  { "timer_create", CB_SYS_timer_create, TARGET_LINUX_SYS_timer_create },
#endif
#ifdef CB_SYS_timer_settime
# define TARGET_LINUX_SYS_timer_settime 261
  { "timer_settime", CB_SYS_timer_settime, TARGET_LINUX_SYS_timer_settime },
#endif
#ifdef CB_SYS_timer_gettime
# define TARGET_LINUX_SYS_timer_gettime 262
  { "timer_gettime", CB_SYS_timer_gettime, TARGET_LINUX_SYS_timer_gettime },
#endif
#ifdef CB_SYS_timer_getoverrun
# define TARGET_LINUX_SYS_timer_getoverrun 263
  { "timer_getoverrun", CB_SYS_timer_getoverrun, TARGET_LINUX_SYS_timer_getoverrun },
#endif
#ifdef CB_SYS_timer_delete
# define TARGET_LINUX_SYS_timer_delete 264
  { "timer_delete", CB_SYS_timer_delete, TARGET_LINUX_SYS_timer_delete },
#endif
#ifdef CB_SYS_clock_settime
# define TARGET_LINUX_SYS_clock_settime 265
  { "clock_settime", CB_SYS_clock_settime, TARGET_LINUX_SYS_clock_settime },
#endif
#ifdef CB_SYS_clock_gettime
# define TARGET_LINUX_SYS_clock_gettime 266
  { "clock_gettime", CB_SYS_clock_gettime, TARGET_LINUX_SYS_clock_gettime },
#endif
#ifdef CB_SYS_clock_getres
# define TARGET_LINUX_SYS_clock_getres 267
  { "clock_getres", CB_SYS_clock_getres, TARGET_LINUX_SYS_clock_getres },
#endif
#ifdef CB_SYS_clock_nanosleep
# define TARGET_LINUX_SYS_clock_nanosleep 268
  { "clock_nanosleep", CB_SYS_clock_nanosleep, TARGET_LINUX_SYS_clock_nanosleep },
#endif
#ifdef CB_SYS_statfs64
# define TARGET_LINUX_SYS_statfs64 269
  { "statfs64", CB_SYS_statfs64, TARGET_LINUX_SYS_statfs64 },
#endif
#ifdef CB_SYS_fstatfs64
# define TARGET_LINUX_SYS_fstatfs64 270
  { "fstatfs64", CB_SYS_fstatfs64, TARGET_LINUX_SYS_fstatfs64 },
#endif
#ifdef CB_SYS_tgkill
# define TARGET_LINUX_SYS_tgkill 271
  { "tgkill", CB_SYS_tgkill, TARGET_LINUX_SYS_tgkill },
#endif
#ifdef CB_SYS_utimes
# define TARGET_LINUX_SYS_utimes 272
  { "utimes", CB_SYS_utimes, TARGET_LINUX_SYS_utimes },
#endif
#ifdef CB_SYS_fadvise64_64
# define TARGET_LINUX_SYS_fadvise64_64 273
  { "fadvise64_64", CB_SYS_fadvise64_64, TARGET_LINUX_SYS_fadvise64_64 },
#endif
#ifdef CB_SYS_mq_open
# define TARGET_LINUX_SYS_mq_open 278
  { "mq_open", CB_SYS_mq_open, TARGET_LINUX_SYS_mq_open },
#endif
#ifdef CB_SYS_mq_unlink
# define TARGET_LINUX_SYS_mq_unlink 279
  { "mq_unlink", CB_SYS_mq_unlink, TARGET_LINUX_SYS_mq_unlink },
#endif
#ifdef CB_SYS_mq_timedsend
# define TARGET_LINUX_SYS_mq_timedsend 280
  { "mq_timedsend", CB_SYS_mq_timedsend, TARGET_LINUX_SYS_mq_timedsend },
#endif
#ifdef CB_SYS_mq_timedreceive
# define TARGET_LINUX_SYS_mq_timedreceive 281
  { "mq_timedreceive", CB_SYS_mq_timedreceive, TARGET_LINUX_SYS_mq_timedreceive },
#endif
#ifdef CB_SYS_mq_notify
# define TARGET_LINUX_SYS_mq_notify 282
  { "mq_notify", CB_SYS_mq_notify, TARGET_LINUX_SYS_mq_notify },
#endif
#ifdef CB_SYS_mq_getsetattr
# define TARGET_LINUX_SYS_mq_getsetattr 283
  { "mq_getsetattr", CB_SYS_mq_getsetattr, TARGET_LINUX_SYS_mq_getsetattr },
#endif
#ifdef CB_SYS_kexec_load
# define TARGET_LINUX_SYS_kexec_load 284
  { "kexec_load", CB_SYS_kexec_load, TARGET_LINUX_SYS_kexec_load },
#endif
#ifdef CB_SYS_waitid
# define TARGET_LINUX_SYS_waitid 285
  { "waitid", CB_SYS_waitid, TARGET_LINUX_SYS_waitid },
#endif
#ifdef CB_SYS_add_key
# define TARGET_LINUX_SYS_add_key 286
  { "add_key", CB_SYS_add_key, TARGET_LINUX_SYS_add_key },
#endif
#ifdef CB_SYS_request_key
# define TARGET_LINUX_SYS_request_key 287
  { "request_key", CB_SYS_request_key, TARGET_LINUX_SYS_request_key },
#endif
#ifdef CB_SYS_keyctl
# define TARGET_LINUX_SYS_keyctl 288
  { "keyctl", CB_SYS_keyctl, TARGET_LINUX_SYS_keyctl },
#endif
#ifdef CB_SYS_ioprio_set
# define TARGET_LINUX_SYS_ioprio_set 289
  { "ioprio_set", CB_SYS_ioprio_set, TARGET_LINUX_SYS_ioprio_set },
#endif
#ifdef CB_SYS_ioprio_get
# define TARGET_LINUX_SYS_ioprio_get 290
  { "ioprio_get", CB_SYS_ioprio_get, TARGET_LINUX_SYS_ioprio_get },
#endif
#ifdef CB_SYS_inotify_init
# define TARGET_LINUX_SYS_inotify_init 291
  { "inotify_init", CB_SYS_inotify_init, TARGET_LINUX_SYS_inotify_init },
#endif
#ifdef CB_SYS_inotify_add_watch
# define TARGET_LINUX_SYS_inotify_add_watch 292
  { "inotify_add_watch", CB_SYS_inotify_add_watch, TARGET_LINUX_SYS_inotify_add_watch },
#endif
#ifdef CB_SYS_inotify_rm_watch
# define TARGET_LINUX_SYS_inotify_rm_watch 293
  { "inotify_rm_watch", CB_SYS_inotify_rm_watch, TARGET_LINUX_SYS_inotify_rm_watch },
#endif
#ifdef CB_SYS_openat
# define TARGET_LINUX_SYS_openat 295
  { "openat", CB_SYS_openat, TARGET_LINUX_SYS_openat },
#endif
#ifdef CB_SYS_mkdirat
# define TARGET_LINUX_SYS_mkdirat 296
  { "mkdirat", CB_SYS_mkdirat, TARGET_LINUX_SYS_mkdirat },
#endif
#ifdef CB_SYS_mknodat
# define TARGET_LINUX_SYS_mknodat 297
  { "mknodat", CB_SYS_mknodat, TARGET_LINUX_SYS_mknodat },
#endif
#ifdef CB_SYS_fchownat
# define TARGET_LINUX_SYS_fchownat 298
  { "fchownat", CB_SYS_fchownat, TARGET_LINUX_SYS_fchownat },
#endif
#ifdef CB_SYS_futimesat
# define TARGET_LINUX_SYS_futimesat 299
  { "futimesat", CB_SYS_futimesat, TARGET_LINUX_SYS_futimesat },
#endif
#ifdef CB_SYS_fstatat64
# define TARGET_LINUX_SYS_fstatat64 300
  { "fstatat64", CB_SYS_fstatat64, TARGET_LINUX_SYS_fstatat64 },
#endif
#ifdef CB_SYS_unlinkat
# define TARGET_LINUX_SYS_unlinkat 301
  { "unlinkat", CB_SYS_unlinkat, TARGET_LINUX_SYS_unlinkat },
#endif
#ifdef CB_SYS_renameat
# define TARGET_LINUX_SYS_renameat 302
  { "renameat", CB_SYS_renameat, TARGET_LINUX_SYS_renameat },
#endif
#ifdef CB_SYS_linkat
# define TARGET_LINUX_SYS_linkat 303
  { "linkat", CB_SYS_linkat, TARGET_LINUX_SYS_linkat },
#endif
#ifdef CB_SYS_symlinkat
# define TARGET_LINUX_SYS_symlinkat 304
  { "symlinkat", CB_SYS_symlinkat, TARGET_LINUX_SYS_symlinkat },
#endif
#ifdef CB_SYS_readlinkat
# define TARGET_LINUX_SYS_readlinkat 305
  { "readlinkat", CB_SYS_readlinkat, TARGET_LINUX_SYS_readlinkat },
#endif
#ifdef CB_SYS_fchmodat
# define TARGET_LINUX_SYS_fchmodat 306
  { "fchmodat", CB_SYS_fchmodat, TARGET_LINUX_SYS_fchmodat },
#endif
#ifdef CB_SYS_faccessat
# define TARGET_LINUX_SYS_faccessat 307
  { "faccessat", CB_SYS_faccessat, TARGET_LINUX_SYS_faccessat },
#endif
#ifdef CB_SYS_pselect6
# define TARGET_LINUX_SYS_pselect6 308
  { "pselect6", CB_SYS_pselect6, TARGET_LINUX_SYS_pselect6 },
#endif
#ifdef CB_SYS_ppoll
# define TARGET_LINUX_SYS_ppoll 309
  { "ppoll", CB_SYS_ppoll, TARGET_LINUX_SYS_ppoll },
#endif
#ifdef CB_SYS_unshare
# define TARGET_LINUX_SYS_unshare 310
  { "unshare", CB_SYS_unshare, TARGET_LINUX_SYS_unshare },
#endif
#ifdef CB_SYS_sram_alloc
# define TARGET_LINUX_SYS_sram_alloc 311
  { "sram_alloc", CB_SYS_sram_alloc, TARGET_LINUX_SYS_sram_alloc },
#endif
#ifdef CB_SYS_sram_free
# define TARGET_LINUX_SYS_sram_free 312
  { "sram_free", CB_SYS_sram_free, TARGET_LINUX_SYS_sram_free },
#endif
#ifdef CB_SYS_dma_memcpy
# define TARGET_LINUX_SYS_dma_memcpy 313
  { "dma_memcpy", CB_SYS_dma_memcpy, TARGET_LINUX_SYS_dma_memcpy },
#endif
#ifdef CB_SYS_accept
# define TARGET_LINUX_SYS_accept 314
  { "accept", CB_SYS_accept, TARGET_LINUX_SYS_accept },
#endif
#ifdef CB_SYS_bind
# define TARGET_LINUX_SYS_bind 315
  { "bind", CB_SYS_bind, TARGET_LINUX_SYS_bind },
#endif
#ifdef CB_SYS_connect
# define TARGET_LINUX_SYS_connect 316
  { "connect", CB_SYS_connect, TARGET_LINUX_SYS_connect },
#endif
#ifdef CB_SYS_getpeername
# define TARGET_LINUX_SYS_getpeername 317
  { "getpeername", CB_SYS_getpeername, TARGET_LINUX_SYS_getpeername },
#endif
#ifdef CB_SYS_getsockname
# define TARGET_LINUX_SYS_getsockname 318
  { "getsockname", CB_SYS_getsockname, TARGET_LINUX_SYS_getsockname },
#endif
#ifdef CB_SYS_getsockopt
# define TARGET_LINUX_SYS_getsockopt 319
  { "getsockopt", CB_SYS_getsockopt, TARGET_LINUX_SYS_getsockopt },
#endif
#ifdef CB_SYS_listen
# define TARGET_LINUX_SYS_listen 320
  { "listen", CB_SYS_listen, TARGET_LINUX_SYS_listen },
#endif
#ifdef CB_SYS_recv
# define TARGET_LINUX_SYS_recv 321
  { "recv", CB_SYS_recv, TARGET_LINUX_SYS_recv },
#endif
#ifdef CB_SYS_recvfrom
# define TARGET_LINUX_SYS_recvfrom 322
  { "recvfrom", CB_SYS_recvfrom, TARGET_LINUX_SYS_recvfrom },
#endif
#ifdef CB_SYS_recvmsg
# define TARGET_LINUX_SYS_recvmsg 323
  { "recvmsg", CB_SYS_recvmsg, TARGET_LINUX_SYS_recvmsg },
#endif
#ifdef CB_SYS_send
# define TARGET_LINUX_SYS_send 324
  { "send", CB_SYS_send, TARGET_LINUX_SYS_send },
#endif
#ifdef CB_SYS_sendmsg
# define TARGET_LINUX_SYS_sendmsg 325
  { "sendmsg", CB_SYS_sendmsg, TARGET_LINUX_SYS_sendmsg },
#endif
#ifdef CB_SYS_sendto
# define TARGET_LINUX_SYS_sendto 326
  { "sendto", CB_SYS_sendto, TARGET_LINUX_SYS_sendto },
#endif
#ifdef CB_SYS_setsockopt
# define TARGET_LINUX_SYS_setsockopt 327
  { "setsockopt", CB_SYS_setsockopt, TARGET_LINUX_SYS_setsockopt },
#endif
#ifdef CB_SYS_shutdown
# define TARGET_LINUX_SYS_shutdown 328
  { "shutdown", CB_SYS_shutdown, TARGET_LINUX_SYS_shutdown },
#endif
#ifdef CB_SYS_socket
# define TARGET_LINUX_SYS_socket 329
  { "socket", CB_SYS_socket, TARGET_LINUX_SYS_socket },
#endif
#ifdef CB_SYS_socketpair
# define TARGET_LINUX_SYS_socketpair 330
  { "socketpair", CB_SYS_socketpair, TARGET_LINUX_SYS_socketpair },
#endif
#ifdef CB_SYS_semctl
# define TARGET_LINUX_SYS_semctl 331
  { "semctl", CB_SYS_semctl, TARGET_LINUX_SYS_semctl },
#endif
#ifdef CB_SYS_semget
# define TARGET_LINUX_SYS_semget 332
  { "semget", CB_SYS_semget, TARGET_LINUX_SYS_semget },
#endif
#ifdef CB_SYS_semop
# define TARGET_LINUX_SYS_semop 333
  { "semop", CB_SYS_semop, TARGET_LINUX_SYS_semop },
#endif
#ifdef CB_SYS_msgctl
# define TARGET_LINUX_SYS_msgctl 334
  { "msgctl", CB_SYS_msgctl, TARGET_LINUX_SYS_msgctl },
#endif
#ifdef CB_SYS_msgget
# define TARGET_LINUX_SYS_msgget 335
  { "msgget", CB_SYS_msgget, TARGET_LINUX_SYS_msgget },
#endif
#ifdef CB_SYS_msgrcv
# define TARGET_LINUX_SYS_msgrcv 336
  { "msgrcv", CB_SYS_msgrcv, TARGET_LINUX_SYS_msgrcv },
#endif
#ifdef CB_SYS_msgsnd
# define TARGET_LINUX_SYS_msgsnd 337
  { "msgsnd", CB_SYS_msgsnd, TARGET_LINUX_SYS_msgsnd },
#endif
#ifdef CB_SYS_shmat
# define TARGET_LINUX_SYS_shmat 338
  { "shmat", CB_SYS_shmat, TARGET_LINUX_SYS_shmat },
#endif
#ifdef CB_SYS_shmctl
# define TARGET_LINUX_SYS_shmctl 339
  { "shmctl", CB_SYS_shmctl, TARGET_LINUX_SYS_shmctl },
#endif
#ifdef CB_SYS_shmdt
# define TARGET_LINUX_SYS_shmdt 340
  { "shmdt", CB_SYS_shmdt, TARGET_LINUX_SYS_shmdt },
#endif
#ifdef CB_SYS_shmget
# define TARGET_LINUX_SYS_shmget 341
  { "shmget", CB_SYS_shmget, TARGET_LINUX_SYS_shmget },
#endif
#ifdef CB_SYS_splice
# define TARGET_LINUX_SYS_splice 342
  { "splice", CB_SYS_splice, TARGET_LINUX_SYS_splice },
#endif
#ifdef CB_SYS_sync_file_range
# define TARGET_LINUX_SYS_sync_file_range 343
  { "sync_file_range", CB_SYS_sync_file_range, TARGET_LINUX_SYS_sync_file_range },
#endif
#ifdef CB_SYS_tee
# define TARGET_LINUX_SYS_tee 344
  { "tee", CB_SYS_tee, TARGET_LINUX_SYS_tee },
#endif
#ifdef CB_SYS_vmsplice
# define TARGET_LINUX_SYS_vmsplice 345
  { "vmsplice", CB_SYS_vmsplice, TARGET_LINUX_SYS_vmsplice },
#endif
#ifdef CB_SYS_epoll_pwait
# define TARGET_LINUX_SYS_epoll_pwait 346
  { "epoll_pwait", CB_SYS_epoll_pwait, TARGET_LINUX_SYS_epoll_pwait },
#endif
#ifdef CB_SYS_utimensat
# define TARGET_LINUX_SYS_utimensat 347
  { "utimensat", CB_SYS_utimensat, TARGET_LINUX_SYS_utimensat },
#endif
#ifdef CB_SYS_signalfd
# define TARGET_LINUX_SYS_signalfd 348
  { "signalfd", CB_SYS_signalfd, TARGET_LINUX_SYS_signalfd },
#endif
#ifdef CB_SYS_timerfd_create
# define TARGET_LINUX_SYS_timerfd_create 349
  { "timerfd_create", CB_SYS_timerfd_create, TARGET_LINUX_SYS_timerfd_create },
#endif
#ifdef CB_SYS_eventfd
# define TARGET_LINUX_SYS_eventfd 350
  { "eventfd", CB_SYS_eventfd, TARGET_LINUX_SYS_eventfd },
#endif
#ifdef CB_SYS_pread64
# define TARGET_LINUX_SYS_pread64 351
  { "pread64", CB_SYS_pread64, TARGET_LINUX_SYS_pread64 },
#endif
#ifdef CB_SYS_pwrite64
# define TARGET_LINUX_SYS_pwrite64 352
  { "pwrite64", CB_SYS_pwrite64, TARGET_LINUX_SYS_pwrite64 },
#endif
#ifdef CB_SYS_fadvise64
# define TARGET_LINUX_SYS_fadvise64 353
  { "fadvise64", CB_SYS_fadvise64, TARGET_LINUX_SYS_fadvise64 },
#endif
#ifdef CB_SYS_set_robust_list
# define TARGET_LINUX_SYS_set_robust_list 354
  { "set_robust_list", CB_SYS_set_robust_list, TARGET_LINUX_SYS_set_robust_list },
#endif
#ifdef CB_SYS_get_robust_list
# define TARGET_LINUX_SYS_get_robust_list 355
  { "get_robust_list", CB_SYS_get_robust_list, TARGET_LINUX_SYS_get_robust_list },
#endif
#ifdef CB_SYS_fallocate
# define TARGET_LINUX_SYS_fallocate 356
  { "fallocate", CB_SYS_fallocate, TARGET_LINUX_SYS_fallocate },
#endif
#ifdef CB_SYS_semtimedop
# define TARGET_LINUX_SYS_semtimedop 357
  { "semtimedop", CB_SYS_semtimedop, TARGET_LINUX_SYS_semtimedop },
#endif
#ifdef CB_SYS_timerfd_settime
# define TARGET_LINUX_SYS_timerfd_settime 358
  { "timerfd_settime", CB_SYS_timerfd_settime, TARGET_LINUX_SYS_timerfd_settime },
#endif
#ifdef CB_SYS_timerfd_gettime
# define TARGET_LINUX_SYS_timerfd_gettime 359
  { "timerfd_gettime", CB_SYS_timerfd_gettime, TARGET_LINUX_SYS_timerfd_gettime },
#endif
#ifdef CB_SYS_signalfd4
# define TARGET_LINUX_SYS_signalfd4 360
  { "signalfd4", CB_SYS_signalfd4, TARGET_LINUX_SYS_signalfd4 },
#endif
#ifdef CB_SYS_eventfd2
# define TARGET_LINUX_SYS_eventfd2 361
  { "eventfd2", CB_SYS_eventfd2, TARGET_LINUX_SYS_eventfd2 },
#endif
#ifdef CB_SYS_epoll_create1
# define TARGET_LINUX_SYS_epoll_create1 362
  { "epoll_create1", CB_SYS_epoll_create1, TARGET_LINUX_SYS_epoll_create1 },
#endif
#ifdef CB_SYS_dup3
# define TARGET_LINUX_SYS_dup3 363
  { "dup3", CB_SYS_dup3, TARGET_LINUX_SYS_dup3 },
#endif
#ifdef CB_SYS_pipe2
# define TARGET_LINUX_SYS_pipe2 364
  { "pipe2", CB_SYS_pipe2, TARGET_LINUX_SYS_pipe2 },
#endif
#ifdef CB_SYS_inotify_init1
# define TARGET_LINUX_SYS_inotify_init1 365
  { "inotify_init1", CB_SYS_inotify_init1, TARGET_LINUX_SYS_inotify_init1 },
#endif
#ifdef CB_SYS_preadv
# define TARGET_LINUX_SYS_preadv 366
  { "preadv", CB_SYS_preadv, TARGET_LINUX_SYS_preadv },
#endif
#ifdef CB_SYS_pwritev
# define TARGET_LINUX_SYS_pwritev 367
  { "pwritev", CB_SYS_pwritev, TARGET_LINUX_SYS_pwritev },
#endif
#ifdef CB_SYS_rt_tgsigqueueinfo
# define TARGET_LINUX_SYS_rt_tgsigqueueinfo 368
  { "rt_tgsigqueueinfo", CB_SYS_rt_tgsigqueueinfo, TARGET_LINUX_SYS_rt_tgsigqueueinfo },
#endif
#ifdef CB_SYS_perf_event_open
# define TARGET_LINUX_SYS_perf_event_open 369
  { "perf_event_open", CB_SYS_perf_event_open, TARGET_LINUX_SYS_perf_event_open },
#endif
#ifdef CB_SYS_recvmmsg
# define TARGET_LINUX_SYS_recvmmsg 370
  { "recvmmsg", CB_SYS_recvmmsg, TARGET_LINUX_SYS_recvmmsg },
#endif
#ifdef CB_SYS_fanotify_init
# define TARGET_LINUX_SYS_fanotify_init 371
  { "fanotify_init", CB_SYS_fanotify_init, TARGET_LINUX_SYS_fanotify_init },
#endif
#ifdef CB_SYS_fanotify_mark
# define TARGET_LINUX_SYS_fanotify_mark 372
  { "fanotify_mark", CB_SYS_fanotify_mark, TARGET_LINUX_SYS_fanotify_mark },
#endif
#ifdef CB_SYS_prlimit64
# define TARGET_LINUX_SYS_prlimit64 373
  { "prlimit64", CB_SYS_prlimit64, TARGET_LINUX_SYS_prlimit64 },
#endif
#ifdef CB_SYS_cacheflush
# define TARGET_LINUX_SYS_cacheflush 374
  { "cacheflush", CB_SYS_cacheflush, TARGET_LINUX_SYS_cacheflush },
#endif
#ifdef CB_SYS_syscall
# define TARGET_LINUX_SYS_syscall 375
  { "syscall", CB_SYS_syscall, TARGET_LINUX_SYS_syscall },
#endif
  { 0, -1, -1 }
};

static CB_TARGET_DEFS_MAP cb_linux_errno_map[] =
{
#ifdef EPERM
# define TARGET_LINUX_EPERM 1
  { "EPERM", EPERM, TARGET_LINUX_EPERM },
#endif
#ifdef ENOENT
# define TARGET_LINUX_ENOENT 2
  { "ENOENT", ENOENT, TARGET_LINUX_ENOENT },
#endif
#ifdef ESRCH
# define TARGET_LINUX_ESRCH 3
  { "ESRCH", ESRCH, TARGET_LINUX_ESRCH },
#endif
#ifdef EINTR
# define TARGET_LINUX_EINTR 4
  { "EINTR", EINTR, TARGET_LINUX_EINTR },
#endif
#ifdef EIO
# define TARGET_LINUX_EIO 5
  { "EIO", EIO, TARGET_LINUX_EIO },
#endif
#ifdef ENXIO
# define TARGET_LINUX_ENXIO 6
  { "ENXIO", ENXIO, TARGET_LINUX_ENXIO },
#endif
#ifdef E2BIG
# define TARGET_LINUX_E2BIG 7
  { "E2BIG", E2BIG, TARGET_LINUX_E2BIG },
#endif
#ifdef ENOEXEC
# define TARGET_LINUX_ENOEXEC 8
  { "ENOEXEC", ENOEXEC, TARGET_LINUX_ENOEXEC },
#endif
#ifdef EBADF
# define TARGET_LINUX_EBADF 9
  { "EBADF", EBADF, TARGET_LINUX_EBADF },
#endif
#ifdef ECHILD
# define TARGET_LINUX_ECHILD 10
  { "ECHILD", ECHILD, TARGET_LINUX_ECHILD },
#endif
#ifdef EAGAIN
# define TARGET_LINUX_EAGAIN 11
  { "EAGAIN", EAGAIN, TARGET_LINUX_EAGAIN },
#endif
#ifdef ENOMEM
# define TARGET_LINUX_ENOMEM 12
  { "ENOMEM", ENOMEM, TARGET_LINUX_ENOMEM },
#endif
#ifdef EACCES
# define TARGET_LINUX_EACCES 13
  { "EACCES", EACCES, TARGET_LINUX_EACCES },
#endif
#ifdef EFAULT
# define TARGET_LINUX_EFAULT 14
  { "EFAULT", EFAULT, TARGET_LINUX_EFAULT },
#endif
#ifdef ENOTBLK
# define TARGET_LINUX_ENOTBLK 15
  { "ENOTBLK", ENOTBLK, TARGET_LINUX_ENOTBLK },
#endif
#ifdef EBUSY
# define TARGET_LINUX_EBUSY 16
  { "EBUSY", EBUSY, TARGET_LINUX_EBUSY },
#endif
#ifdef EEXIST
# define TARGET_LINUX_EEXIST 17
  { "EEXIST", EEXIST, TARGET_LINUX_EEXIST },
#endif
#ifdef EXDEV
# define TARGET_LINUX_EXDEV 18
  { "EXDEV", EXDEV, TARGET_LINUX_EXDEV },
#endif
#ifdef ENODEV
# define TARGET_LINUX_ENODEV 19
  { "ENODEV", ENODEV, TARGET_LINUX_ENODEV },
#endif
#ifdef ENOTDIR
# define TARGET_LINUX_ENOTDIR 20
  { "ENOTDIR", ENOTDIR, TARGET_LINUX_ENOTDIR },
#endif
#ifdef EISDIR
# define TARGET_LINUX_EISDIR 21
  { "EISDIR", EISDIR, TARGET_LINUX_EISDIR },
#endif
#ifdef EINVAL
# define TARGET_LINUX_EINVAL 22
  { "EINVAL", EINVAL, TARGET_LINUX_EINVAL },
#endif
#ifdef ENFILE
# define TARGET_LINUX_ENFILE 23
  { "ENFILE", ENFILE, TARGET_LINUX_ENFILE },
#endif
#ifdef EMFILE
# define TARGET_LINUX_EMFILE 24
  { "EMFILE", EMFILE, TARGET_LINUX_EMFILE },
#endif
#ifdef ENOTTY
# define TARGET_LINUX_ENOTTY 25
  { "ENOTTY", ENOTTY, TARGET_LINUX_ENOTTY },
#endif
#ifdef ETXTBSY
# define TARGET_LINUX_ETXTBSY 26
  { "ETXTBSY", ETXTBSY, TARGET_LINUX_ETXTBSY },
#endif
#ifdef EFBIG
# define TARGET_LINUX_EFBIG 27
  { "EFBIG", EFBIG, TARGET_LINUX_EFBIG },
#endif
#ifdef ENOSPC
# define TARGET_LINUX_ENOSPC 28
  { "ENOSPC", ENOSPC, TARGET_LINUX_ENOSPC },
#endif
#ifdef ESPIPE
# define TARGET_LINUX_ESPIPE 29
  { "ESPIPE", ESPIPE, TARGET_LINUX_ESPIPE },
#endif
#ifdef EROFS
# define TARGET_LINUX_EROFS 30
  { "EROFS", EROFS, TARGET_LINUX_EROFS },
#endif
#ifdef EMLINK
# define TARGET_LINUX_EMLINK 31
  { "EMLINK", EMLINK, TARGET_LINUX_EMLINK },
#endif
#ifdef EPIPE
# define TARGET_LINUX_EPIPE 32
  { "EPIPE", EPIPE, TARGET_LINUX_EPIPE },
#endif
#ifdef EDOM
# define TARGET_LINUX_EDOM 33
  { "EDOM", EDOM, TARGET_LINUX_EDOM },
#endif
#ifdef ERANGE
# define TARGET_LINUX_ERANGE 34
  { "ERANGE", ERANGE, TARGET_LINUX_ERANGE },
#endif
#ifdef EDEADLK
# define TARGET_LINUX_EDEADLK 35
  { "EDEADLK", EDEADLK, TARGET_LINUX_EDEADLK },
#endif
#ifdef ENAMETOOLONG
# define TARGET_LINUX_ENAMETOOLONG 36
  { "ENAMETOOLONG", ENAMETOOLONG, TARGET_LINUX_ENAMETOOLONG },
#endif
#ifdef ENOLCK
# define TARGET_LINUX_ENOLCK 37
  { "ENOLCK", ENOLCK, TARGET_LINUX_ENOLCK },
#endif
#ifdef ENOSYS
# define TARGET_LINUX_ENOSYS 38
  { "ENOSYS", ENOSYS, TARGET_LINUX_ENOSYS },
#endif
#ifdef ENOTEMPTY
# define TARGET_LINUX_ENOTEMPTY 39
  { "ENOTEMPTY", ENOTEMPTY, TARGET_LINUX_ENOTEMPTY },
#endif
#ifdef ELOOP
# define TARGET_LINUX_ELOOP 40
  { "ELOOP", ELOOP, TARGET_LINUX_ELOOP },
#endif
#ifdef ENOMSG
# define TARGET_LINUX_ENOMSG 42
  { "ENOMSG", ENOMSG, TARGET_LINUX_ENOMSG },
#endif
#ifdef EIDRM
# define TARGET_LINUX_EIDRM 43
  { "EIDRM", EIDRM, TARGET_LINUX_EIDRM },
#endif
#ifdef ECHRNG
# define TARGET_LINUX_ECHRNG 44
  { "ECHRNG", ECHRNG, TARGET_LINUX_ECHRNG },
#endif
#ifdef EL2NSYNC
# define TARGET_LINUX_EL2NSYNC 45
  { "EL2NSYNC", EL2NSYNC, TARGET_LINUX_EL2NSYNC },
#endif
#ifdef EL3HLT
# define TARGET_LINUX_EL3HLT 46
  { "EL3HLT", EL3HLT, TARGET_LINUX_EL3HLT },
#endif
#ifdef EL3RST
# define TARGET_LINUX_EL3RST 47
  { "EL3RST", EL3RST, TARGET_LINUX_EL3RST },
#endif
#ifdef ELNRNG
# define TARGET_LINUX_ELNRNG 48
  { "ELNRNG", ELNRNG, TARGET_LINUX_ELNRNG },
#endif
#ifdef EUNATCH
# define TARGET_LINUX_EUNATCH 49
  { "EUNATCH", EUNATCH, TARGET_LINUX_EUNATCH },
#endif
#ifdef ENOCSI
# define TARGET_LINUX_ENOCSI 50
  { "ENOCSI", ENOCSI, TARGET_LINUX_ENOCSI },
#endif
#ifdef EL2HLT
# define TARGET_LINUX_EL2HLT 51
  { "EL2HLT", EL2HLT, TARGET_LINUX_EL2HLT },
#endif
#ifdef EBADE
# define TARGET_LINUX_EBADE 52
  { "EBADE", EBADE, TARGET_LINUX_EBADE },
#endif
#ifdef EBADR
# define TARGET_LINUX_EBADR 53
  { "EBADR", EBADR, TARGET_LINUX_EBADR },
#endif
#ifdef EXFULL
# define TARGET_LINUX_EXFULL 54
  { "EXFULL", EXFULL, TARGET_LINUX_EXFULL },
#endif
#ifdef ENOANO
# define TARGET_LINUX_ENOANO 55
  { "ENOANO", ENOANO, TARGET_LINUX_ENOANO },
#endif
#ifdef EBADRQC
# define TARGET_LINUX_EBADRQC 56
  { "EBADRQC", EBADRQC, TARGET_LINUX_EBADRQC },
#endif
#ifdef EBADSLT
# define TARGET_LINUX_EBADSLT 57
  { "EBADSLT", EBADSLT, TARGET_LINUX_EBADSLT },
#endif
#ifdef EBFONT
# define TARGET_LINUX_EBFONT 59
  { "EBFONT", EBFONT, TARGET_LINUX_EBFONT },
#endif
#ifdef ENOSTR
# define TARGET_LINUX_ENOSTR 60
  { "ENOSTR", ENOSTR, TARGET_LINUX_ENOSTR },
#endif
#ifdef ENODATA
# define TARGET_LINUX_ENODATA 61
  { "ENODATA", ENODATA, TARGET_LINUX_ENODATA },
#endif
#ifdef ETIME
# define TARGET_LINUX_ETIME 62
  { "ETIME", ETIME, TARGET_LINUX_ETIME },
#endif
#ifdef ENOSR
# define TARGET_LINUX_ENOSR 63
  { "ENOSR", ENOSR, TARGET_LINUX_ENOSR },
#endif
#ifdef ENONET
# define TARGET_LINUX_ENONET 64
  { "ENONET", ENONET, TARGET_LINUX_ENONET },
#endif
#ifdef ENOPKG
# define TARGET_LINUX_ENOPKG 65
  { "ENOPKG", ENOPKG, TARGET_LINUX_ENOPKG },
#endif
#ifdef EREMOTE
# define TARGET_LINUX_EREMOTE 66
  { "EREMOTE", EREMOTE, TARGET_LINUX_EREMOTE },
#endif
#ifdef ENOLINK
# define TARGET_LINUX_ENOLINK 67
  { "ENOLINK", ENOLINK, TARGET_LINUX_ENOLINK },
#endif
#ifdef EADV
# define TARGET_LINUX_EADV 68
  { "EADV", EADV, TARGET_LINUX_EADV },
#endif
#ifdef ESRMNT
# define TARGET_LINUX_ESRMNT 69
  { "ESRMNT", ESRMNT, TARGET_LINUX_ESRMNT },
#endif
#ifdef ECOMM
# define TARGET_LINUX_ECOMM 70
  { "ECOMM", ECOMM, TARGET_LINUX_ECOMM },
#endif
#ifdef EPROTO
# define TARGET_LINUX_EPROTO 71
  { "EPROTO", EPROTO, TARGET_LINUX_EPROTO },
#endif
#ifdef EMULTIHOP
# define TARGET_LINUX_EMULTIHOP 72
  { "EMULTIHOP", EMULTIHOP, TARGET_LINUX_EMULTIHOP },
#endif
#ifdef EDOTDOT
# define TARGET_LINUX_EDOTDOT 73
  { "EDOTDOT", EDOTDOT, TARGET_LINUX_EDOTDOT },
#endif
#ifdef EBADMSG
# define TARGET_LINUX_EBADMSG 74
  { "EBADMSG", EBADMSG, TARGET_LINUX_EBADMSG },
#endif
#ifdef EOVERFLOW
# define TARGET_LINUX_EOVERFLOW 75
  { "EOVERFLOW", EOVERFLOW, TARGET_LINUX_EOVERFLOW },
#endif
#ifdef ENOTUNIQ
# define TARGET_LINUX_ENOTUNIQ 76
  { "ENOTUNIQ", ENOTUNIQ, TARGET_LINUX_ENOTUNIQ },
#endif
#ifdef EBADFD
# define TARGET_LINUX_EBADFD 77
  { "EBADFD", EBADFD, TARGET_LINUX_EBADFD },
#endif
#ifdef EREMCHG
# define TARGET_LINUX_EREMCHG 78
  { "EREMCHG", EREMCHG, TARGET_LINUX_EREMCHG },
#endif
#ifdef ELIBACC
# define TARGET_LINUX_ELIBACC 79
  { "ELIBACC", ELIBACC, TARGET_LINUX_ELIBACC },
#endif
#ifdef ELIBBAD
# define TARGET_LINUX_ELIBBAD 80
  { "ELIBBAD", ELIBBAD, TARGET_LINUX_ELIBBAD },
#endif
#ifdef ELIBSCN
# define TARGET_LINUX_ELIBSCN 81
  { "ELIBSCN", ELIBSCN, TARGET_LINUX_ELIBSCN },
#endif
#ifdef ELIBMAX
# define TARGET_LINUX_ELIBMAX 82
  { "ELIBMAX", ELIBMAX, TARGET_LINUX_ELIBMAX },
#endif
#ifdef ELIBEXEC
# define TARGET_LINUX_ELIBEXEC 83
  { "ELIBEXEC", ELIBEXEC, TARGET_LINUX_ELIBEXEC },
#endif
#ifdef EILSEQ
# define TARGET_LINUX_EILSEQ 84
  { "EILSEQ", EILSEQ, TARGET_LINUX_EILSEQ },
#endif
#ifdef ERESTART
# define TARGET_LINUX_ERESTART 85
  { "ERESTART", ERESTART, TARGET_LINUX_ERESTART },
#endif
#ifdef ESTRPIPE
# define TARGET_LINUX_ESTRPIPE 86
  { "ESTRPIPE", ESTRPIPE, TARGET_LINUX_ESTRPIPE },
#endif
#ifdef EUSERS
# define TARGET_LINUX_EUSERS 87
  { "EUSERS", EUSERS, TARGET_LINUX_EUSERS },
#endif
#ifdef ENOTSOCK
# define TARGET_LINUX_ENOTSOCK 88
  { "ENOTSOCK", ENOTSOCK, TARGET_LINUX_ENOTSOCK },
#endif
#ifdef EDESTADDRREQ
# define TARGET_LINUX_EDESTADDRREQ 89
  { "EDESTADDRREQ", EDESTADDRREQ, TARGET_LINUX_EDESTADDRREQ },
#endif
#ifdef EMSGSIZE
# define TARGET_LINUX_EMSGSIZE 90
  { "EMSGSIZE", EMSGSIZE, TARGET_LINUX_EMSGSIZE },
#endif
#ifdef EPROTOTYPE
# define TARGET_LINUX_EPROTOTYPE 91
  { "EPROTOTYPE", EPROTOTYPE, TARGET_LINUX_EPROTOTYPE },
#endif
#ifdef ENOPROTOOPT
# define TARGET_LINUX_ENOPROTOOPT 92
  { "ENOPROTOOPT", ENOPROTOOPT, TARGET_LINUX_ENOPROTOOPT },
#endif
#ifdef EPROTONOSUPPORT
# define TARGET_LINUX_EPROTONOSUPPORT 93
  { "EPROTONOSUPPORT", EPROTONOSUPPORT, TARGET_LINUX_EPROTONOSUPPORT },
#endif
#ifdef ESOCKTNOSUPPORT
# define TARGET_LINUX_ESOCKTNOSUPPORT 94
  { "ESOCKTNOSUPPORT", ESOCKTNOSUPPORT, TARGET_LINUX_ESOCKTNOSUPPORT },
#endif
#ifdef EOPNOTSUPP
# define TARGET_LINUX_EOPNOTSUPP 95
  { "EOPNOTSUPP", EOPNOTSUPP, TARGET_LINUX_EOPNOTSUPP },
#endif
#ifdef EPFNOSUPPORT
# define TARGET_LINUX_EPFNOSUPPORT 96
  { "EPFNOSUPPORT", EPFNOSUPPORT, TARGET_LINUX_EPFNOSUPPORT },
#endif
#ifdef EAFNOSUPPORT
# define TARGET_LINUX_EAFNOSUPPORT 97
  { "EAFNOSUPPORT", EAFNOSUPPORT, TARGET_LINUX_EAFNOSUPPORT },
#endif
#ifdef EADDRINUSE
# define TARGET_LINUX_EADDRINUSE 98
  { "EADDRINUSE", EADDRINUSE, TARGET_LINUX_EADDRINUSE },
#endif
#ifdef EADDRNOTAVAIL
# define TARGET_LINUX_EADDRNOTAVAIL 99
  { "EADDRNOTAVAIL", EADDRNOTAVAIL, TARGET_LINUX_EADDRNOTAVAIL },
#endif
#ifdef ENETDOWN
# define TARGET_LINUX_ENETDOWN 100
  { "ENETDOWN", ENETDOWN, TARGET_LINUX_ENETDOWN },
#endif
#ifdef ENETUNREACH
# define TARGET_LINUX_ENETUNREACH 101
  { "ENETUNREACH", ENETUNREACH, TARGET_LINUX_ENETUNREACH },
#endif
#ifdef ENETRESET
# define TARGET_LINUX_ENETRESET 102
  { "ENETRESET", ENETRESET, TARGET_LINUX_ENETRESET },
#endif
#ifdef ECONNABORTED
# define TARGET_LINUX_ECONNABORTED 103
  { "ECONNABORTED", ECONNABORTED, TARGET_LINUX_ECONNABORTED },
#endif
#ifdef ECONNRESET
# define TARGET_LINUX_ECONNRESET 104
  { "ECONNRESET", ECONNRESET, TARGET_LINUX_ECONNRESET },
#endif
#ifdef ENOBUFS
# define TARGET_LINUX_ENOBUFS 105
  { "ENOBUFS", ENOBUFS, TARGET_LINUX_ENOBUFS },
#endif
#ifdef EISCONN
# define TARGET_LINUX_EISCONN 106
  { "EISCONN", EISCONN, TARGET_LINUX_EISCONN },
#endif
#ifdef ENOTCONN
# define TARGET_LINUX_ENOTCONN 107
  { "ENOTCONN", ENOTCONN, TARGET_LINUX_ENOTCONN },
#endif
#ifdef ESHUTDOWN
# define TARGET_LINUX_ESHUTDOWN 108
  { "ESHUTDOWN", ESHUTDOWN, TARGET_LINUX_ESHUTDOWN },
#endif
#ifdef ETOOMANYREFS
# define TARGET_LINUX_ETOOMANYREFS 109
  { "ETOOMANYREFS", ETOOMANYREFS, TARGET_LINUX_ETOOMANYREFS },
#endif
#ifdef ETIMEDOUT
# define TARGET_LINUX_ETIMEDOUT 110
  { "ETIMEDOUT", ETIMEDOUT, TARGET_LINUX_ETIMEDOUT },
#endif
#ifdef ECONNREFUSED
# define TARGET_LINUX_ECONNREFUSED 111
  { "ECONNREFUSED", ECONNREFUSED, TARGET_LINUX_ECONNREFUSED },
#endif
#ifdef EHOSTDOWN
# define TARGET_LINUX_EHOSTDOWN 112
  { "EHOSTDOWN", EHOSTDOWN, TARGET_LINUX_EHOSTDOWN },
#endif
#ifdef EHOSTUNREACH
# define TARGET_LINUX_EHOSTUNREACH 113
  { "EHOSTUNREACH", EHOSTUNREACH, TARGET_LINUX_EHOSTUNREACH },
#endif
#ifdef EALREADY
# define TARGET_LINUX_EALREADY 114
  { "EALREADY", EALREADY, TARGET_LINUX_EALREADY },
#endif
#ifdef EINPROGRESS
# define TARGET_LINUX_EINPROGRESS 115
  { "EINPROGRESS", EINPROGRESS, TARGET_LINUX_EINPROGRESS },
#endif
#ifdef ESTALE
# define TARGET_LINUX_ESTALE 116
  { "ESTALE", ESTALE, TARGET_LINUX_ESTALE },
#endif
#ifdef EUCLEAN
# define TARGET_LINUX_EUCLEAN 117
  { "EUCLEAN", EUCLEAN, TARGET_LINUX_EUCLEAN },
#endif
#ifdef ENOTNAM
# define TARGET_LINUX_ENOTNAM 118
  { "ENOTNAM", ENOTNAM, TARGET_LINUX_ENOTNAM },
#endif
#ifdef ENAVAIL
# define TARGET_LINUX_ENAVAIL 119
  { "ENAVAIL", ENAVAIL, TARGET_LINUX_ENAVAIL },
#endif
#ifdef EISNAM
# define TARGET_LINUX_EISNAM 120
  { "EISNAM", EISNAM, TARGET_LINUX_EISNAM },
#endif
#ifdef EREMOTEIO
# define TARGET_LINUX_EREMOTEIO 121
  { "EREMOTEIO", EREMOTEIO, TARGET_LINUX_EREMOTEIO },
#endif
#ifdef EDQUOT
# define TARGET_LINUX_EDQUOT 122
  { "EDQUOT", EDQUOT, TARGET_LINUX_EDQUOT },
#endif
#ifdef ENOMEDIUM
# define TARGET_LINUX_ENOMEDIUM 123
  { "ENOMEDIUM", ENOMEDIUM, TARGET_LINUX_ENOMEDIUM },
#endif
#ifdef EMEDIUMTYPE
# define TARGET_LINUX_EMEDIUMTYPE 124
  { "EMEDIUMTYPE", EMEDIUMTYPE, TARGET_LINUX_EMEDIUMTYPE },
#endif
#ifdef ECANCELED
# define TARGET_LINUX_ECANCELED 125
  { "ECANCELED", ECANCELED, TARGET_LINUX_ECANCELED },
#endif
#ifdef EOWNERDEAD
# define TARGET_LINUX_EOWNERDEAD 130
  { "EOWNERDEAD", EOWNERDEAD, TARGET_LINUX_EOWNERDEAD },
#endif
#ifdef ENOTRECOVERABLE
# define TARGET_LINUX_ENOTRECOVERABLE 131
  { "ENOTRECOVERABLE", ENOTRECOVERABLE, TARGET_LINUX_ENOTRECOVERABLE },
#endif
  { 0, 0, 0 }
};

static CB_TARGET_DEFS_MAP cb_linux_open_map[] =
{
#ifdef O_ACCMODE
# define TARGET_LINUX_O_ACCMODE 0003
  { "O_ACCMODE", O_ACCMODE, TARGET_LINUX_O_ACCMODE },
#endif
#ifdef O_RDONLY
# define TARGET_LINUX_O_RDONLY 00
  { "O_RDONLY", O_RDONLY, TARGET_LINUX_O_RDONLY },
#endif
#ifdef O_WRONLY
# define TARGET_LINUX_O_WRONLY 01
  { "O_WRONLY", O_WRONLY, TARGET_LINUX_O_WRONLY },
#endif
#ifdef O_RDWR
# define TARGET_LINUX_O_RDWR 02
  { "O_RDWR", O_RDWR, TARGET_LINUX_O_RDWR },
#endif
#ifdef O_CREAT
# define TARGET_LINUX_O_CREAT 0100
  { "O_CREAT", O_CREAT, TARGET_LINUX_O_CREAT },
#endif
#ifdef O_EXCL
# define TARGET_LINUX_O_EXCL 0200
  { "O_EXCL", O_EXCL, TARGET_LINUX_O_EXCL },
#endif
#ifdef O_NOCTTY
# define TARGET_LINUX_O_NOCTTY 0400
  { "O_NOCTTY", O_NOCTTY, TARGET_LINUX_O_NOCTTY },
#endif
#ifdef O_TRUNC
# define TARGET_LINUX_O_TRUNC 01000
  { "O_TRUNC", O_TRUNC, TARGET_LINUX_O_TRUNC },
#endif
#ifdef O_APPEND
# define TARGET_LINUX_O_APPEND 02000
  { "O_APPEND", O_APPEND, TARGET_LINUX_O_APPEND },
#endif
#ifdef O_NONBLOCK
# define TARGET_LINUX_O_NONBLOCK 04000
  { "O_NONBLOCK", O_NONBLOCK, TARGET_LINUX_O_NONBLOCK },
#endif
#ifdef O_SYNC
# define TARGET_LINUX_O_SYNC 010000
  { "O_SYNC", O_SYNC, TARGET_LINUX_O_SYNC },
#endif
#ifdef O_ASYNC
# define TARGET_LINUX_O_ASYNC 020000
  { "O_ASYNC", O_ASYNC, TARGET_LINUX_O_ASYNC },
#endif
  { 0, -1, -1 }
};

static CB_TARGET_DEFS_MAP cb_linux_signal_map[] =
{
#ifdef SIGHUP
# define TARGET_LINUX_SIGHUP 1
  { "SIGHUP", SIGHUP, TARGET_LINUX_SIGHUP },
#endif
#ifdef SIGINT
# define TARGET_LINUX_SIGINT 2
  { "SIGINT", SIGINT, TARGET_LINUX_SIGINT },
#endif
#ifdef SIGQUIT
# define TARGET_LINUX_SIGQUIT 3
  { "SIGQUIT", SIGQUIT, TARGET_LINUX_SIGQUIT },
#endif
#ifdef SIGILL
# define TARGET_LINUX_SIGILL 4
  { "SIGILL", SIGILL, TARGET_LINUX_SIGILL },
#endif
#ifdef SIGTRAP
# define TARGET_LINUX_SIGTRAP 5
  { "SIGTRAP", SIGTRAP, TARGET_LINUX_SIGTRAP },
#endif
#ifdef SIGABRT
# define TARGET_LINUX_SIGABRT 6
  { "SIGABRT", SIGABRT, TARGET_LINUX_SIGABRT },
#endif
#ifdef SIGIOT
# define TARGET_LINUX_SIGIOT 6
  { "SIGIOT", SIGIOT, TARGET_LINUX_SIGIOT },
#endif
#ifdef SIGBUS
# define TARGET_LINUX_SIGBUS 7
  { "SIGBUS", SIGBUS, TARGET_LINUX_SIGBUS },
#endif
#ifdef SIGFPE
# define TARGET_LINUX_SIGFPE 8
  { "SIGFPE", SIGFPE, TARGET_LINUX_SIGFPE },
#endif
#ifdef SIGKILL
# define TARGET_LINUX_SIGKILL 9
  { "SIGKILL", SIGKILL, TARGET_LINUX_SIGKILL },
#endif
#ifdef SIGUSR1
# define TARGET_LINUX_SIGUSR1 10
  { "SIGUSR1", SIGUSR1, TARGET_LINUX_SIGUSR1 },
#endif
#ifdef SIGSEGV
# define TARGET_LINUX_SIGSEGV 11
  { "SIGSEGV", SIGSEGV, TARGET_LINUX_SIGSEGV },
#endif
#ifdef SIGUSR2
# define TARGET_LINUX_SIGUSR2 12
  { "SIGUSR2", SIGUSR2, TARGET_LINUX_SIGUSR2 },
#endif
#ifdef SIGPIPE
# define TARGET_LINUX_SIGPIPE 13
  { "SIGPIPE", SIGPIPE, TARGET_LINUX_SIGPIPE },
#endif
#ifdef SIGALRM
# define TARGET_LINUX_SIGALRM 14
  { "SIGALRM", SIGALRM, TARGET_LINUX_SIGALRM },
#endif
#ifdef SIGTERM
# define TARGET_LINUX_SIGTERM 15
  { "SIGTERM", SIGTERM, TARGET_LINUX_SIGTERM },
#endif
#ifdef SIGSTKFLT
# define TARGET_LINUX_SIGSTKFLT 16
  { "SIGSTKFLT", SIGSTKFLT, TARGET_LINUX_SIGSTKFLT },
#endif
#ifdef SIGCHLD
# define TARGET_LINUX_SIGCHLD 17
  { "SIGCHLD", SIGCHLD, TARGET_LINUX_SIGCHLD },
#endif
#ifdef SIGCONT
# define TARGET_LINUX_SIGCONT 18
  { "SIGCONT", SIGCONT, TARGET_LINUX_SIGCONT },
#endif
#ifdef SIGSTOP
# define TARGET_LINUX_SIGSTOP 19
  { "SIGSTOP", SIGSTOP, TARGET_LINUX_SIGSTOP },
#endif
#ifdef SIGTSTP
# define TARGET_LINUX_SIGTSTP 20
  { "SIGTSTP", SIGTSTP, TARGET_LINUX_SIGTSTP },
#endif
#ifdef SIGTTIN
# define TARGET_LINUX_SIGTTIN 21
  { "SIGTTIN", SIGTTIN, TARGET_LINUX_SIGTTIN },
#endif
#ifdef SIGTTOU
# define TARGET_LINUX_SIGTTOU 22
  { "SIGTTOU", SIGTTOU, TARGET_LINUX_SIGTTOU },
#endif
#ifdef SIGURG
# define TARGET_LINUX_SIGURG 23
  { "SIGURG", SIGURG, TARGET_LINUX_SIGURG },
#endif
#ifdef SIGXCPU
# define TARGET_LINUX_SIGXCPU 24
  { "SIGXCPU", SIGXCPU, TARGET_LINUX_SIGXCPU },
#endif
#ifdef SIGXFSZ
# define TARGET_LINUX_SIGXFSZ 25
  { "SIGXFSZ", SIGXFSZ, TARGET_LINUX_SIGXFSZ },
#endif
#ifdef SIGVTALRM
# define TARGET_LINUX_SIGVTALRM 26
  { "SIGVTALRM", SIGVTALRM, TARGET_LINUX_SIGVTALRM },
#endif
#ifdef SIGPROF
# define TARGET_LINUX_SIGPROF 27
  { "SIGPROF", SIGPROF, TARGET_LINUX_SIGPROF },
#endif
#ifdef SIGWINCH
# define TARGET_LINUX_SIGWINCH 28
  { "SIGWINCH", SIGWINCH, TARGET_LINUX_SIGWINCH },
#endif
#ifdef SIGIO
# define TARGET_LINUX_SIGIO 29
  { "SIGIO", SIGIO, TARGET_LINUX_SIGIO },
#endif
#ifdef SIGPWR
# define TARGET_LINUX_SIGPWR 30
  { "SIGPWR", SIGPWR, TARGET_LINUX_SIGPWR },
#endif
#ifdef SIGSYS
# define TARGET_LINUX_SIGSYS 31
  { "SIGSYS", SIGSYS, TARGET_LINUX_SIGSYS },
#endif
#ifdef SIGUNUSED
# define TARGET_LINUX_SIGUNUSED 31
  { "SIGUNUSED", SIGUNUSED, TARGET_LINUX_SIGUNUSED },
#endif
#ifdef SIG_BLOCK
# define TARGET_LINUX_SIG_BLOCK 0
  { "SIG_BLOCK", SIG_BLOCK, TARGET_LINUX_SIG_BLOCK },
#endif
#ifdef SIG_UNBLOCK
# define TARGET_LINUX_SIG_UNBLOCK 1
  { "SIG_UNBLOCK", SIG_UNBLOCK, TARGET_LINUX_SIG_UNBLOCK },
#endif
#ifdef SIG_SETMASK
# define TARGET_LINUX_SIG_SETMASK 2
  { "SIG_SETMASK", SIG_SETMASK, TARGET_LINUX_SIG_SETMASK },
#endif
  { 0, -1, -1 }
};
