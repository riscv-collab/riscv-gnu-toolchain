/* Target syscall mappings for newlib/libgloss environment.
   Copyright 1995-2024 Free Software Foundation, Inc.
   Contributed by Mike Frysinger.

   This file is part of simulators.

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

/* This must come before any other includes.  */
#include "defs.h"

#include "sim/callback.h"

#include "target-newlib-syscall.h"

/* This file is kept up-to-date via the gennltvals.py script.  Do not edit
   anything between the START & END comment blocks below.  */

  /* gennltvals: START */
CB_TARGET_DEFS_MAP cb_cr16_syscall_map[] = {
#ifdef CB_SYS_ARG
  { "ARG", CB_SYS_ARG, TARGET_NEWLIB_CR16_SYS_ARG },
#endif
#ifdef CB_SYS_chdir
  { "chdir", CB_SYS_chdir, TARGET_NEWLIB_CR16_SYS_chdir },
#endif
#ifdef CB_SYS_chmod
  { "chmod", CB_SYS_chmod, TARGET_NEWLIB_CR16_SYS_chmod },
#endif
#ifdef CB_SYS_chown
  { "chown", CB_SYS_chown, TARGET_NEWLIB_CR16_SYS_chown },
#endif
#ifdef CB_SYS_close
  { "close", CB_SYS_close, TARGET_NEWLIB_CR16_SYS_close },
#endif
#ifdef CB_SYS_create
  { "create", CB_SYS_create, TARGET_NEWLIB_CR16_SYS_create },
#endif
#ifdef CB_SYS_execv
  { "execv", CB_SYS_execv, TARGET_NEWLIB_CR16_SYS_execv },
#endif
#ifdef CB_SYS_execve
  { "execve", CB_SYS_execve, TARGET_NEWLIB_CR16_SYS_execve },
#endif
#ifdef CB_SYS_exit
  { "exit", CB_SYS_exit, TARGET_NEWLIB_CR16_SYS_exit },
#endif
#ifdef CB_SYS_fork
  { "fork", CB_SYS_fork, TARGET_NEWLIB_CR16_SYS_fork },
#endif
#ifdef CB_SYS_fstat
  { "fstat", CB_SYS_fstat, TARGET_NEWLIB_CR16_SYS_fstat },
#endif
#ifdef CB_SYS_getpid
  { "getpid", CB_SYS_getpid, TARGET_NEWLIB_CR16_SYS_getpid },
#endif
#ifdef CB_SYS_isatty
  { "isatty", CB_SYS_isatty, TARGET_NEWLIB_CR16_SYS_isatty },
#endif
#ifdef CB_SYS_kill
  { "kill", CB_SYS_kill, TARGET_NEWLIB_CR16_SYS_kill },
#endif
#ifdef CB_SYS_link
  { "link", CB_SYS_link, TARGET_NEWLIB_CR16_SYS_link },
#endif
#ifdef CB_SYS_lseek
  { "lseek", CB_SYS_lseek, TARGET_NEWLIB_CR16_SYS_lseek },
#endif
#ifdef CB_SYS_mknod
  { "mknod", CB_SYS_mknod, TARGET_NEWLIB_CR16_SYS_mknod },
#endif
#ifdef CB_SYS_open
  { "open", CB_SYS_open, TARGET_NEWLIB_CR16_SYS_open },
#endif
#ifdef CB_SYS_pipe
  { "pipe", CB_SYS_pipe, TARGET_NEWLIB_CR16_SYS_pipe },
#endif
#ifdef CB_SYS_read
  { "read", CB_SYS_read, TARGET_NEWLIB_CR16_SYS_read },
#endif
#ifdef CB_SYS_rename
  { "rename", CB_SYS_rename, TARGET_NEWLIB_CR16_SYS_rename },
#endif
#ifdef CB_SYS_stat
  { "stat", CB_SYS_stat, TARGET_NEWLIB_CR16_SYS_stat },
#endif
#ifdef CB_SYS_time
  { "time", CB_SYS_time, TARGET_NEWLIB_CR16_SYS_time },
#endif
#ifdef CB_SYS_unlink
  { "unlink", CB_SYS_unlink, TARGET_NEWLIB_CR16_SYS_unlink },
#endif
#ifdef CB_SYS_utime
  { "utime", CB_SYS_utime, TARGET_NEWLIB_CR16_SYS_utime },
#endif
#ifdef CB_SYS_wait
  { "wait", CB_SYS_wait, TARGET_NEWLIB_CR16_SYS_wait },
#endif
#ifdef CB_SYS_wait4
  { "wait4", CB_SYS_wait4, TARGET_NEWLIB_CR16_SYS_wait4 },
#endif
#ifdef CB_SYS_write
  { "write", CB_SYS_write, TARGET_NEWLIB_CR16_SYS_write },
#endif
  {NULL, -1, -1},
};

CB_TARGET_DEFS_MAP cb_d10v_syscall_map[] = {
#ifdef CB_SYS_ARG
  { "ARG", CB_SYS_ARG, TARGET_NEWLIB_D10V_SYS_ARG },
#endif
#ifdef CB_SYS_chdir
  { "chdir", CB_SYS_chdir, TARGET_NEWLIB_D10V_SYS_chdir },
#endif
#ifdef CB_SYS_chmod
  { "chmod", CB_SYS_chmod, TARGET_NEWLIB_D10V_SYS_chmod },
#endif
#ifdef CB_SYS_chown
  { "chown", CB_SYS_chown, TARGET_NEWLIB_D10V_SYS_chown },
#endif
#ifdef CB_SYS_close
  { "close", CB_SYS_close, TARGET_NEWLIB_D10V_SYS_close },
#endif
#ifdef CB_SYS_creat
  { "creat", CB_SYS_creat, TARGET_NEWLIB_D10V_SYS_creat },
#endif
#ifdef CB_SYS_execv
  { "execv", CB_SYS_execv, TARGET_NEWLIB_D10V_SYS_execv },
#endif
#ifdef CB_SYS_execve
  { "execve", CB_SYS_execve, TARGET_NEWLIB_D10V_SYS_execve },
#endif
#ifdef CB_SYS_exit
  { "exit", CB_SYS_exit, TARGET_NEWLIB_D10V_SYS_exit },
#endif
#ifdef CB_SYS_fork
  { "fork", CB_SYS_fork, TARGET_NEWLIB_D10V_SYS_fork },
#endif
#ifdef CB_SYS_fstat
  { "fstat", CB_SYS_fstat, TARGET_NEWLIB_D10V_SYS_fstat },
#endif
#ifdef CB_SYS_getpid
  { "getpid", CB_SYS_getpid, TARGET_NEWLIB_D10V_SYS_getpid },
#endif
#ifdef CB_SYS_isatty
  { "isatty", CB_SYS_isatty, TARGET_NEWLIB_D10V_SYS_isatty },
#endif
#ifdef CB_SYS_kill
  { "kill", CB_SYS_kill, TARGET_NEWLIB_D10V_SYS_kill },
#endif
#ifdef CB_SYS_link
  { "link", CB_SYS_link, TARGET_NEWLIB_D10V_SYS_link },
#endif
#ifdef CB_SYS_lseek
  { "lseek", CB_SYS_lseek, TARGET_NEWLIB_D10V_SYS_lseek },
#endif
#ifdef CB_SYS_mknod
  { "mknod", CB_SYS_mknod, TARGET_NEWLIB_D10V_SYS_mknod },
#endif
#ifdef CB_SYS_open
  { "open", CB_SYS_open, TARGET_NEWLIB_D10V_SYS_open },
#endif
#ifdef CB_SYS_pipe
  { "pipe", CB_SYS_pipe, TARGET_NEWLIB_D10V_SYS_pipe },
#endif
#ifdef CB_SYS_read
  { "read", CB_SYS_read, TARGET_NEWLIB_D10V_SYS_read },
#endif
#ifdef CB_SYS_stat
  { "stat", CB_SYS_stat, TARGET_NEWLIB_D10V_SYS_stat },
#endif
#ifdef CB_SYS_time
  { "time", CB_SYS_time, TARGET_NEWLIB_D10V_SYS_time },
#endif
#ifdef CB_SYS_unlink
  { "unlink", CB_SYS_unlink, TARGET_NEWLIB_D10V_SYS_unlink },
#endif
#ifdef CB_SYS_utime
  { "utime", CB_SYS_utime, TARGET_NEWLIB_D10V_SYS_utime },
#endif
#ifdef CB_SYS_wait
  { "wait", CB_SYS_wait, TARGET_NEWLIB_D10V_SYS_wait },
#endif
#ifdef CB_SYS_wait4
  { "wait4", CB_SYS_wait4, TARGET_NEWLIB_D10V_SYS_wait4 },
#endif
#ifdef CB_SYS_write
  { "write", CB_SYS_write, TARGET_NEWLIB_D10V_SYS_write },
#endif
  {NULL, -1, -1},
};

CB_TARGET_DEFS_MAP cb_mcore_syscall_map[] = {
#ifdef CB_SYS_access
  { "access", CB_SYS_access, TARGET_NEWLIB_MCORE_SYS_access },
#endif
#ifdef CB_SYS_close
  { "close", CB_SYS_close, TARGET_NEWLIB_MCORE_SYS_close },
#endif
#ifdef CB_SYS_creat
  { "creat", CB_SYS_creat, TARGET_NEWLIB_MCORE_SYS_creat },
#endif
#ifdef CB_SYS_link
  { "link", CB_SYS_link, TARGET_NEWLIB_MCORE_SYS_link },
#endif
#ifdef CB_SYS_lseek
  { "lseek", CB_SYS_lseek, TARGET_NEWLIB_MCORE_SYS_lseek },
#endif
#ifdef CB_SYS_open
  { "open", CB_SYS_open, TARGET_NEWLIB_MCORE_SYS_open },
#endif
#ifdef CB_SYS_read
  { "read", CB_SYS_read, TARGET_NEWLIB_MCORE_SYS_read },
#endif
#ifdef CB_SYS_time
  { "time", CB_SYS_time, TARGET_NEWLIB_MCORE_SYS_time },
#endif
#ifdef CB_SYS_times
  { "times", CB_SYS_times, TARGET_NEWLIB_MCORE_SYS_times },
#endif
#ifdef CB_SYS_unlink
  { "unlink", CB_SYS_unlink, TARGET_NEWLIB_MCORE_SYS_unlink },
#endif
#ifdef CB_SYS_write
  { "write", CB_SYS_write, TARGET_NEWLIB_MCORE_SYS_write },
#endif
  {NULL, -1, -1},
};

CB_TARGET_DEFS_MAP cb_riscv_syscall_map[] = {
#ifdef CB_SYS_access
  { "access", CB_SYS_access, TARGET_NEWLIB_RISCV_SYS_access },
#endif
#ifdef CB_SYS_brk
  { "brk", CB_SYS_brk, TARGET_NEWLIB_RISCV_SYS_brk },
#endif
#ifdef CB_SYS_chdir
  { "chdir", CB_SYS_chdir, TARGET_NEWLIB_RISCV_SYS_chdir },
#endif
#ifdef CB_SYS_clock_gettime64
  { "clock_gettime64", CB_SYS_clock_gettime64, TARGET_NEWLIB_RISCV_SYS_clock_gettime64 },
#endif
#ifdef CB_SYS_close
  { "close", CB_SYS_close, TARGET_NEWLIB_RISCV_SYS_close },
#endif
#ifdef CB_SYS_dup
  { "dup", CB_SYS_dup, TARGET_NEWLIB_RISCV_SYS_dup },
#endif
#ifdef CB_SYS_exit
  { "exit", CB_SYS_exit, TARGET_NEWLIB_RISCV_SYS_exit },
#endif
#ifdef CB_SYS_exit_group
  { "exit_group", CB_SYS_exit_group, TARGET_NEWLIB_RISCV_SYS_exit_group },
#endif
#ifdef CB_SYS_faccessat
  { "faccessat", CB_SYS_faccessat, TARGET_NEWLIB_RISCV_SYS_faccessat },
#endif
#ifdef CB_SYS_fcntl
  { "fcntl", CB_SYS_fcntl, TARGET_NEWLIB_RISCV_SYS_fcntl },
#endif
#ifdef CB_SYS_fstat
  { "fstat", CB_SYS_fstat, TARGET_NEWLIB_RISCV_SYS_fstat },
#endif
#ifdef CB_SYS_fstatat
  { "fstatat", CB_SYS_fstatat, TARGET_NEWLIB_RISCV_SYS_fstatat },
#endif
#ifdef CB_SYS_getcwd
  { "getcwd", CB_SYS_getcwd, TARGET_NEWLIB_RISCV_SYS_getcwd },
#endif
#ifdef CB_SYS_getdents
  { "getdents", CB_SYS_getdents, TARGET_NEWLIB_RISCV_SYS_getdents },
#endif
#ifdef CB_SYS_getegid
  { "getegid", CB_SYS_getegid, TARGET_NEWLIB_RISCV_SYS_getegid },
#endif
#ifdef CB_SYS_geteuid
  { "geteuid", CB_SYS_geteuid, TARGET_NEWLIB_RISCV_SYS_geteuid },
#endif
#ifdef CB_SYS_getgid
  { "getgid", CB_SYS_getgid, TARGET_NEWLIB_RISCV_SYS_getgid },
#endif
#ifdef CB_SYS_getmainvars
  { "getmainvars", CB_SYS_getmainvars, TARGET_NEWLIB_RISCV_SYS_getmainvars },
#endif
#ifdef CB_SYS_getpid
  { "getpid", CB_SYS_getpid, TARGET_NEWLIB_RISCV_SYS_getpid },
#endif
#ifdef CB_SYS_gettimeofday
  { "gettimeofday", CB_SYS_gettimeofday, TARGET_NEWLIB_RISCV_SYS_gettimeofday },
#endif
#ifdef CB_SYS_getuid
  { "getuid", CB_SYS_getuid, TARGET_NEWLIB_RISCV_SYS_getuid },
#endif
#ifdef CB_SYS_kill
  { "kill", CB_SYS_kill, TARGET_NEWLIB_RISCV_SYS_kill },
#endif
#ifdef CB_SYS_link
  { "link", CB_SYS_link, TARGET_NEWLIB_RISCV_SYS_link },
#endif
#ifdef CB_SYS_lseek
  { "lseek", CB_SYS_lseek, TARGET_NEWLIB_RISCV_SYS_lseek },
#endif
#ifdef CB_SYS_lstat
  { "lstat", CB_SYS_lstat, TARGET_NEWLIB_RISCV_SYS_lstat },
#endif
#ifdef CB_SYS_mkdir
  { "mkdir", CB_SYS_mkdir, TARGET_NEWLIB_RISCV_SYS_mkdir },
#endif
#ifdef CB_SYS_mmap
  { "mmap", CB_SYS_mmap, TARGET_NEWLIB_RISCV_SYS_mmap },
#endif
#ifdef CB_SYS_mremap
  { "mremap", CB_SYS_mremap, TARGET_NEWLIB_RISCV_SYS_mremap },
#endif
#ifdef CB_SYS_munmap
  { "munmap", CB_SYS_munmap, TARGET_NEWLIB_RISCV_SYS_munmap },
#endif
#ifdef CB_SYS_open
  { "open", CB_SYS_open, TARGET_NEWLIB_RISCV_SYS_open },
#endif
#ifdef CB_SYS_openat
  { "openat", CB_SYS_openat, TARGET_NEWLIB_RISCV_SYS_openat },
#endif
#ifdef CB_SYS_pread
  { "pread", CB_SYS_pread, TARGET_NEWLIB_RISCV_SYS_pread },
#endif
#ifdef CB_SYS_pwrite
  { "pwrite", CB_SYS_pwrite, TARGET_NEWLIB_RISCV_SYS_pwrite },
#endif
#ifdef CB_SYS_read
  { "read", CB_SYS_read, TARGET_NEWLIB_RISCV_SYS_read },
#endif
#ifdef CB_SYS_rt_sigaction
  { "rt_sigaction", CB_SYS_rt_sigaction, TARGET_NEWLIB_RISCV_SYS_rt_sigaction },
#endif
#ifdef CB_SYS_stat
  { "stat", CB_SYS_stat, TARGET_NEWLIB_RISCV_SYS_stat },
#endif
#ifdef CB_SYS_time
  { "time", CB_SYS_time, TARGET_NEWLIB_RISCV_SYS_time },
#endif
#ifdef CB_SYS_times
  { "times", CB_SYS_times, TARGET_NEWLIB_RISCV_SYS_times },
#endif
#ifdef CB_SYS_uname
  { "uname", CB_SYS_uname, TARGET_NEWLIB_RISCV_SYS_uname },
#endif
#ifdef CB_SYS_unlink
  { "unlink", CB_SYS_unlink, TARGET_NEWLIB_RISCV_SYS_unlink },
#endif
#ifdef CB_SYS_write
  { "write", CB_SYS_write, TARGET_NEWLIB_RISCV_SYS_write },
#endif
#ifdef CB_SYS_writev
  { "writev", CB_SYS_writev, TARGET_NEWLIB_RISCV_SYS_writev },
#endif
  {NULL, -1, -1},
};

CB_TARGET_DEFS_MAP cb_sh_syscall_map[] = {
#ifdef CB_SYS_ARG
  { "ARG", CB_SYS_ARG, TARGET_NEWLIB_SH_SYS_ARG },
#endif
#ifdef CB_SYS_argc
  { "argc", CB_SYS_argc, TARGET_NEWLIB_SH_SYS_argc },
#endif
#ifdef CB_SYS_argn
  { "argn", CB_SYS_argn, TARGET_NEWLIB_SH_SYS_argn },
#endif
#ifdef CB_SYS_argnlen
  { "argnlen", CB_SYS_argnlen, TARGET_NEWLIB_SH_SYS_argnlen },
#endif
#ifdef CB_SYS_chdir
  { "chdir", CB_SYS_chdir, TARGET_NEWLIB_SH_SYS_chdir },
#endif
#ifdef CB_SYS_chmod
  { "chmod", CB_SYS_chmod, TARGET_NEWLIB_SH_SYS_chmod },
#endif
#ifdef CB_SYS_chown
  { "chown", CB_SYS_chown, TARGET_NEWLIB_SH_SYS_chown },
#endif
#ifdef CB_SYS_close
  { "close", CB_SYS_close, TARGET_NEWLIB_SH_SYS_close },
#endif
#ifdef CB_SYS_creat
  { "creat", CB_SYS_creat, TARGET_NEWLIB_SH_SYS_creat },
#endif
#ifdef CB_SYS_execv
  { "execv", CB_SYS_execv, TARGET_NEWLIB_SH_SYS_execv },
#endif
#ifdef CB_SYS_execve
  { "execve", CB_SYS_execve, TARGET_NEWLIB_SH_SYS_execve },
#endif
#ifdef CB_SYS_exit
  { "exit", CB_SYS_exit, TARGET_NEWLIB_SH_SYS_exit },
#endif
#ifdef CB_SYS_fork
  { "fork", CB_SYS_fork, TARGET_NEWLIB_SH_SYS_fork },
#endif
#ifdef CB_SYS_fstat
  { "fstat", CB_SYS_fstat, TARGET_NEWLIB_SH_SYS_fstat },
#endif
#ifdef CB_SYS_ftruncate
  { "ftruncate", CB_SYS_ftruncate, TARGET_NEWLIB_SH_SYS_ftruncate },
#endif
#ifdef CB_SYS_getpid
  { "getpid", CB_SYS_getpid, TARGET_NEWLIB_SH_SYS_getpid },
#endif
#ifdef CB_SYS_isatty
  { "isatty", CB_SYS_isatty, TARGET_NEWLIB_SH_SYS_isatty },
#endif
#ifdef CB_SYS_link
  { "link", CB_SYS_link, TARGET_NEWLIB_SH_SYS_link },
#endif
#ifdef CB_SYS_lseek
  { "lseek", CB_SYS_lseek, TARGET_NEWLIB_SH_SYS_lseek },
#endif
#ifdef CB_SYS_mknod
  { "mknod", CB_SYS_mknod, TARGET_NEWLIB_SH_SYS_mknod },
#endif
#ifdef CB_SYS_open
  { "open", CB_SYS_open, TARGET_NEWLIB_SH_SYS_open },
#endif
#ifdef CB_SYS_pipe
  { "pipe", CB_SYS_pipe, TARGET_NEWLIB_SH_SYS_pipe },
#endif
#ifdef CB_SYS_read
  { "read", CB_SYS_read, TARGET_NEWLIB_SH_SYS_read },
#endif
#ifdef CB_SYS_stat
  { "stat", CB_SYS_stat, TARGET_NEWLIB_SH_SYS_stat },
#endif
#ifdef CB_SYS_time
  { "time", CB_SYS_time, TARGET_NEWLIB_SH_SYS_time },
#endif
#ifdef CB_SYS_truncate
  { "truncate", CB_SYS_truncate, TARGET_NEWLIB_SH_SYS_truncate },
#endif
#ifdef CB_SYS_unlink
  { "unlink", CB_SYS_unlink, TARGET_NEWLIB_SH_SYS_unlink },
#endif
#ifdef CB_SYS_utime
  { "utime", CB_SYS_utime, TARGET_NEWLIB_SH_SYS_utime },
#endif
#ifdef CB_SYS_wait
  { "wait", CB_SYS_wait, TARGET_NEWLIB_SH_SYS_wait },
#endif
#ifdef CB_SYS_wait4
  { "wait4", CB_SYS_wait4, TARGET_NEWLIB_SH_SYS_wait4 },
#endif
#ifdef CB_SYS_write
  { "write", CB_SYS_write, TARGET_NEWLIB_SH_SYS_write },
#endif
  {NULL, -1, -1},
};

CB_TARGET_DEFS_MAP cb_v850_syscall_map[] = {
#ifdef CB_SYS_ARG
  { "ARG", CB_SYS_ARG, TARGET_NEWLIB_V850_SYS_ARG },
#endif
#ifdef CB_SYS_chdir
  { "chdir", CB_SYS_chdir, TARGET_NEWLIB_V850_SYS_chdir },
#endif
#ifdef CB_SYS_chmod
  { "chmod", CB_SYS_chmod, TARGET_NEWLIB_V850_SYS_chmod },
#endif
#ifdef CB_SYS_chown
  { "chown", CB_SYS_chown, TARGET_NEWLIB_V850_SYS_chown },
#endif
#ifdef CB_SYS_close
  { "close", CB_SYS_close, TARGET_NEWLIB_V850_SYS_close },
#endif
#ifdef CB_SYS_creat
  { "creat", CB_SYS_creat, TARGET_NEWLIB_V850_SYS_creat },
#endif
#ifdef CB_SYS_execv
  { "execv", CB_SYS_execv, TARGET_NEWLIB_V850_SYS_execv },
#endif
#ifdef CB_SYS_execve
  { "execve", CB_SYS_execve, TARGET_NEWLIB_V850_SYS_execve },
#endif
#ifdef CB_SYS_exit
  { "exit", CB_SYS_exit, TARGET_NEWLIB_V850_SYS_exit },
#endif
#ifdef CB_SYS_fork
  { "fork", CB_SYS_fork, TARGET_NEWLIB_V850_SYS_fork },
#endif
#ifdef CB_SYS_fstat
  { "fstat", CB_SYS_fstat, TARGET_NEWLIB_V850_SYS_fstat },
#endif
#ifdef CB_SYS_getpid
  { "getpid", CB_SYS_getpid, TARGET_NEWLIB_V850_SYS_getpid },
#endif
#ifdef CB_SYS_gettimeofday
  { "gettimeofday", CB_SYS_gettimeofday, TARGET_NEWLIB_V850_SYS_gettimeofday },
#endif
#ifdef CB_SYS_isatty
  { "isatty", CB_SYS_isatty, TARGET_NEWLIB_V850_SYS_isatty },
#endif
#ifdef CB_SYS_link
  { "link", CB_SYS_link, TARGET_NEWLIB_V850_SYS_link },
#endif
#ifdef CB_SYS_lseek
  { "lseek", CB_SYS_lseek, TARGET_NEWLIB_V850_SYS_lseek },
#endif
#ifdef CB_SYS_mknod
  { "mknod", CB_SYS_mknod, TARGET_NEWLIB_V850_SYS_mknod },
#endif
#ifdef CB_SYS_open
  { "open", CB_SYS_open, TARGET_NEWLIB_V850_SYS_open },
#endif
#ifdef CB_SYS_pipe
  { "pipe", CB_SYS_pipe, TARGET_NEWLIB_V850_SYS_pipe },
#endif
#ifdef CB_SYS_read
  { "read", CB_SYS_read, TARGET_NEWLIB_V850_SYS_read },
#endif
#ifdef CB_SYS_rename
  { "rename", CB_SYS_rename, TARGET_NEWLIB_V850_SYS_rename },
#endif
#ifdef CB_SYS_stat
  { "stat", CB_SYS_stat, TARGET_NEWLIB_V850_SYS_stat },
#endif
#ifdef CB_SYS_time
  { "time", CB_SYS_time, TARGET_NEWLIB_V850_SYS_time },
#endif
#ifdef CB_SYS_times
  { "times", CB_SYS_times, TARGET_NEWLIB_V850_SYS_times },
#endif
#ifdef CB_SYS_unlink
  { "unlink", CB_SYS_unlink, TARGET_NEWLIB_V850_SYS_unlink },
#endif
#ifdef CB_SYS_utime
  { "utime", CB_SYS_utime, TARGET_NEWLIB_V850_SYS_utime },
#endif
#ifdef CB_SYS_wait
  { "wait", CB_SYS_wait, TARGET_NEWLIB_V850_SYS_wait },
#endif
#ifdef CB_SYS_wait4
  { "wait4", CB_SYS_wait4, TARGET_NEWLIB_V850_SYS_wait4 },
#endif
#ifdef CB_SYS_write
  { "write", CB_SYS_write, TARGET_NEWLIB_V850_SYS_write },
#endif
  {NULL, -1, -1},
};

CB_TARGET_DEFS_MAP cb_init_syscall_map[] = {
#ifdef CB_SYS_argc
  { "argc", CB_SYS_argc, TARGET_NEWLIB_SYS_argc },
#endif
#ifdef CB_SYS_argn
  { "argn", CB_SYS_argn, TARGET_NEWLIB_SYS_argn },
#endif
#ifdef CB_SYS_argnlen
  { "argnlen", CB_SYS_argnlen, TARGET_NEWLIB_SYS_argnlen },
#endif
#ifdef CB_SYS_argv
  { "argv", CB_SYS_argv, TARGET_NEWLIB_SYS_argv },
#endif
#ifdef CB_SYS_argvlen
  { "argvlen", CB_SYS_argvlen, TARGET_NEWLIB_SYS_argvlen },
#endif
#ifdef CB_SYS_chdir
  { "chdir", CB_SYS_chdir, TARGET_NEWLIB_SYS_chdir },
#endif
#ifdef CB_SYS_chmod
  { "chmod", CB_SYS_chmod, TARGET_NEWLIB_SYS_chmod },
#endif
#ifdef CB_SYS_close
  { "close", CB_SYS_close, TARGET_NEWLIB_SYS_close },
#endif
#ifdef CB_SYS_exit
  { "exit", CB_SYS_exit, TARGET_NEWLIB_SYS_exit },
#endif
#ifdef CB_SYS_fstat
  { "fstat", CB_SYS_fstat, TARGET_NEWLIB_SYS_fstat },
#endif
#ifdef CB_SYS_getpid
  { "getpid", CB_SYS_getpid, TARGET_NEWLIB_SYS_getpid },
#endif
#ifdef CB_SYS_gettimeofday
  { "gettimeofday", CB_SYS_gettimeofday, TARGET_NEWLIB_SYS_gettimeofday },
#endif
#ifdef CB_SYS_kill
  { "kill", CB_SYS_kill, TARGET_NEWLIB_SYS_kill },
#endif
#ifdef CB_SYS_link
  { "link", CB_SYS_link, TARGET_NEWLIB_SYS_link },
#endif
#ifdef CB_SYS_lseek
  { "lseek", CB_SYS_lseek, TARGET_NEWLIB_SYS_lseek },
#endif
#ifdef CB_SYS_open
  { "open", CB_SYS_open, TARGET_NEWLIB_SYS_open },
#endif
#ifdef CB_SYS_read
  { "read", CB_SYS_read, TARGET_NEWLIB_SYS_read },
#endif
#ifdef CB_SYS_reconfig
  { "reconfig", CB_SYS_reconfig, TARGET_NEWLIB_SYS_reconfig },
#endif
#ifdef CB_SYS_stat
  { "stat", CB_SYS_stat, TARGET_NEWLIB_SYS_stat },
#endif
#ifdef CB_SYS_time
  { "time", CB_SYS_time, TARGET_NEWLIB_SYS_time },
#endif
#ifdef CB_SYS_times
  { "times", CB_SYS_times, TARGET_NEWLIB_SYS_times },
#endif
#ifdef CB_SYS_unlink
  { "unlink", CB_SYS_unlink, TARGET_NEWLIB_SYS_unlink },
#endif
#ifdef CB_SYS_utime
  { "utime", CB_SYS_utime, TARGET_NEWLIB_SYS_utime },
#endif
#ifdef CB_SYS_write
  { "write", CB_SYS_write, TARGET_NEWLIB_SYS_write },
#endif
  {NULL, -1, -1},
};
  /* gennltvals: END */
