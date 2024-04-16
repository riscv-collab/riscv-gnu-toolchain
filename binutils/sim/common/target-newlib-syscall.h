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

#ifndef TARGET_NEWLIB_SYSCALL_H
#define TARGET_NEWLIB_SYSCALL_H

/* For CB_TARGET_DEFS_MAP.  */
#include "sim/callback.h"

/* This file is kept up-to-date via the gennltvals.py script.  Do not edit
   anything between the START & END comment blocks below.  */

  /* gennltvals: START */
extern CB_TARGET_DEFS_MAP cb_cr16_syscall_map[];
#define TARGET_NEWLIB_CR16_SYS_ARG 24
#define TARGET_NEWLIB_CR16_SYS_chdir 12
#define TARGET_NEWLIB_CR16_SYS_chmod 15
#define TARGET_NEWLIB_CR16_SYS_chown 16
#define TARGET_NEWLIB_CR16_SYS_close 0x402
#define TARGET_NEWLIB_CR16_SYS_create 8
#define TARGET_NEWLIB_CR16_SYS_execv 11
#define TARGET_NEWLIB_CR16_SYS_execve 59
#define TARGET_NEWLIB_CR16_SYS_exit 0x410
#define TARGET_NEWLIB_CR16_SYS_fork 2
#define TARGET_NEWLIB_CR16_SYS_fstat 22
#define TARGET_NEWLIB_CR16_SYS_getpid 20
#define TARGET_NEWLIB_CR16_SYS_isatty 21
#define TARGET_NEWLIB_CR16_SYS_kill 60
#define TARGET_NEWLIB_CR16_SYS_link 9
#define TARGET_NEWLIB_CR16_SYS_lseek 0x405
#define TARGET_NEWLIB_CR16_SYS_mknod 14
#define TARGET_NEWLIB_CR16_SYS_open 0x401
#define TARGET_NEWLIB_CR16_SYS_pipe 42
#define TARGET_NEWLIB_CR16_SYS_read 0x403
#define TARGET_NEWLIB_CR16_SYS_rename 0x406
#define TARGET_NEWLIB_CR16_SYS_stat 38
#define TARGET_NEWLIB_CR16_SYS_time 0x300
#define TARGET_NEWLIB_CR16_SYS_unlink 0x407
#define TARGET_NEWLIB_CR16_SYS_utime 201
#define TARGET_NEWLIB_CR16_SYS_wait 202
#define TARGET_NEWLIB_CR16_SYS_wait4 7
#define TARGET_NEWLIB_CR16_SYS_write 0x404

extern CB_TARGET_DEFS_MAP cb_d10v_syscall_map[];
#define TARGET_NEWLIB_D10V_SYS_ARG 24
#define TARGET_NEWLIB_D10V_SYS_chdir 12
#define TARGET_NEWLIB_D10V_SYS_chmod 15
#define TARGET_NEWLIB_D10V_SYS_chown 16
#define TARGET_NEWLIB_D10V_SYS_close 6
#define TARGET_NEWLIB_D10V_SYS_creat 8
#define TARGET_NEWLIB_D10V_SYS_execv 11
#define TARGET_NEWLIB_D10V_SYS_execve 59
#define TARGET_NEWLIB_D10V_SYS_exit 1
#define TARGET_NEWLIB_D10V_SYS_fork 2
#define TARGET_NEWLIB_D10V_SYS_fstat 22
#define TARGET_NEWLIB_D10V_SYS_getpid 20
#define TARGET_NEWLIB_D10V_SYS_isatty 21
#define TARGET_NEWLIB_D10V_SYS_kill 60
#define TARGET_NEWLIB_D10V_SYS_link 9
#define TARGET_NEWLIB_D10V_SYS_lseek 19
#define TARGET_NEWLIB_D10V_SYS_mknod 14
#define TARGET_NEWLIB_D10V_SYS_open 5
#define TARGET_NEWLIB_D10V_SYS_pipe 42
#define TARGET_NEWLIB_D10V_SYS_read 3
#define TARGET_NEWLIB_D10V_SYS_stat 38
#define TARGET_NEWLIB_D10V_SYS_time 23
#define TARGET_NEWLIB_D10V_SYS_unlink 10
#define TARGET_NEWLIB_D10V_SYS_utime 201
#define TARGET_NEWLIB_D10V_SYS_wait 202
#define TARGET_NEWLIB_D10V_SYS_wait4 7
#define TARGET_NEWLIB_D10V_SYS_write 4

extern CB_TARGET_DEFS_MAP cb_mcore_syscall_map[];
#define TARGET_NEWLIB_MCORE_SYS_access 33
#define TARGET_NEWLIB_MCORE_SYS_close 6
#define TARGET_NEWLIB_MCORE_SYS_creat 8
#define TARGET_NEWLIB_MCORE_SYS_link 9
#define TARGET_NEWLIB_MCORE_SYS_lseek 19
#define TARGET_NEWLIB_MCORE_SYS_open 5
#define TARGET_NEWLIB_MCORE_SYS_read 3
#define TARGET_NEWLIB_MCORE_SYS_time 13
#define TARGET_NEWLIB_MCORE_SYS_times 43
#define TARGET_NEWLIB_MCORE_SYS_unlink 10
#define TARGET_NEWLIB_MCORE_SYS_write 4

extern CB_TARGET_DEFS_MAP cb_riscv_syscall_map[];
#define TARGET_NEWLIB_RISCV_SYS_access 1033
#define TARGET_NEWLIB_RISCV_SYS_brk 214
#define TARGET_NEWLIB_RISCV_SYS_chdir 49
#define TARGET_NEWLIB_RISCV_SYS_clock_gettime64 403
#define TARGET_NEWLIB_RISCV_SYS_close 57
#define TARGET_NEWLIB_RISCV_SYS_dup 23
#define TARGET_NEWLIB_RISCV_SYS_exit 93
#define TARGET_NEWLIB_RISCV_SYS_exit_group 94
#define TARGET_NEWLIB_RISCV_SYS_faccessat 48
#define TARGET_NEWLIB_RISCV_SYS_fcntl 25
#define TARGET_NEWLIB_RISCV_SYS_fstat 80
#define TARGET_NEWLIB_RISCV_SYS_fstatat 79
#define TARGET_NEWLIB_RISCV_SYS_getcwd 17
#define TARGET_NEWLIB_RISCV_SYS_getdents 61
#define TARGET_NEWLIB_RISCV_SYS_getegid 177
#define TARGET_NEWLIB_RISCV_SYS_geteuid 175
#define TARGET_NEWLIB_RISCV_SYS_getgid 176
#define TARGET_NEWLIB_RISCV_SYS_getmainvars 2011
#define TARGET_NEWLIB_RISCV_SYS_getpid 172
#define TARGET_NEWLIB_RISCV_SYS_gettimeofday 169
#define TARGET_NEWLIB_RISCV_SYS_getuid 174
#define TARGET_NEWLIB_RISCV_SYS_kill 129
#define TARGET_NEWLIB_RISCV_SYS_link 1025
#define TARGET_NEWLIB_RISCV_SYS_lseek 62
#define TARGET_NEWLIB_RISCV_SYS_lstat 1039
#define TARGET_NEWLIB_RISCV_SYS_mkdir 1030
#define TARGET_NEWLIB_RISCV_SYS_mmap 222
#define TARGET_NEWLIB_RISCV_SYS_mremap 216
#define TARGET_NEWLIB_RISCV_SYS_munmap 215
#define TARGET_NEWLIB_RISCV_SYS_open 1024
#define TARGET_NEWLIB_RISCV_SYS_openat 56
#define TARGET_NEWLIB_RISCV_SYS_pread 67
#define TARGET_NEWLIB_RISCV_SYS_pwrite 68
#define TARGET_NEWLIB_RISCV_SYS_read 63
#define TARGET_NEWLIB_RISCV_SYS_rt_sigaction 134
#define TARGET_NEWLIB_RISCV_SYS_stat 1038
#define TARGET_NEWLIB_RISCV_SYS_time 1062
#define TARGET_NEWLIB_RISCV_SYS_times 153
#define TARGET_NEWLIB_RISCV_SYS_uname 160
#define TARGET_NEWLIB_RISCV_SYS_unlink 1026
#define TARGET_NEWLIB_RISCV_SYS_write 64
#define TARGET_NEWLIB_RISCV_SYS_writev 66

extern CB_TARGET_DEFS_MAP cb_sh_syscall_map[];
#define TARGET_NEWLIB_SH_SYS_ARG 24
#define TARGET_NEWLIB_SH_SYS_argc 172
#define TARGET_NEWLIB_SH_SYS_argn 174
#define TARGET_NEWLIB_SH_SYS_argnlen 173
#define TARGET_NEWLIB_SH_SYS_chdir 12
#define TARGET_NEWLIB_SH_SYS_chmod 15
#define TARGET_NEWLIB_SH_SYS_chown 16
#define TARGET_NEWLIB_SH_SYS_close 6
#define TARGET_NEWLIB_SH_SYS_creat 8
#define TARGET_NEWLIB_SH_SYS_execv 11
#define TARGET_NEWLIB_SH_SYS_execve 59
#define TARGET_NEWLIB_SH_SYS_exit 1
#define TARGET_NEWLIB_SH_SYS_fork 2
#define TARGET_NEWLIB_SH_SYS_fstat 22
#define TARGET_NEWLIB_SH_SYS_ftruncate 130
#define TARGET_NEWLIB_SH_SYS_getpid 20
#define TARGET_NEWLIB_SH_SYS_isatty 21
#define TARGET_NEWLIB_SH_SYS_link 9
#define TARGET_NEWLIB_SH_SYS_lseek 19
#define TARGET_NEWLIB_SH_SYS_mknod 14
#define TARGET_NEWLIB_SH_SYS_open 5
#define TARGET_NEWLIB_SH_SYS_pipe 42
#define TARGET_NEWLIB_SH_SYS_read 3
#define TARGET_NEWLIB_SH_SYS_stat 38
#define TARGET_NEWLIB_SH_SYS_time 23
#define TARGET_NEWLIB_SH_SYS_truncate 129
#define TARGET_NEWLIB_SH_SYS_unlink 10
#define TARGET_NEWLIB_SH_SYS_utime 201
#define TARGET_NEWLIB_SH_SYS_wait 202
#define TARGET_NEWLIB_SH_SYS_wait4 7
#define TARGET_NEWLIB_SH_SYS_write 4

extern CB_TARGET_DEFS_MAP cb_v850_syscall_map[];
#define TARGET_NEWLIB_V850_SYS_ARG 24
#define TARGET_NEWLIB_V850_SYS_chdir 12
#define TARGET_NEWLIB_V850_SYS_chmod 15
#define TARGET_NEWLIB_V850_SYS_chown 16
#define TARGET_NEWLIB_V850_SYS_close 6
#define TARGET_NEWLIB_V850_SYS_creat 8
#define TARGET_NEWLIB_V850_SYS_execv 11
#define TARGET_NEWLIB_V850_SYS_execve 59
#define TARGET_NEWLIB_V850_SYS_exit 1
#define TARGET_NEWLIB_V850_SYS_fork 2
#define TARGET_NEWLIB_V850_SYS_fstat 22
#define TARGET_NEWLIB_V850_SYS_getpid 20
#define TARGET_NEWLIB_V850_SYS_gettimeofday 116
#define TARGET_NEWLIB_V850_SYS_isatty 21
#define TARGET_NEWLIB_V850_SYS_link 9
#define TARGET_NEWLIB_V850_SYS_lseek 19
#define TARGET_NEWLIB_V850_SYS_mknod 14
#define TARGET_NEWLIB_V850_SYS_open 5
#define TARGET_NEWLIB_V850_SYS_pipe 42
#define TARGET_NEWLIB_V850_SYS_read 3
#define TARGET_NEWLIB_V850_SYS_rename 134
#define TARGET_NEWLIB_V850_SYS_stat 38
#define TARGET_NEWLIB_V850_SYS_time 23
#define TARGET_NEWLIB_V850_SYS_times 43
#define TARGET_NEWLIB_V850_SYS_unlink 10
#define TARGET_NEWLIB_V850_SYS_utime 201
#define TARGET_NEWLIB_V850_SYS_wait 202
#define TARGET_NEWLIB_V850_SYS_wait4 7
#define TARGET_NEWLIB_V850_SYS_write 4

extern CB_TARGET_DEFS_MAP cb_init_syscall_map[];
#define TARGET_NEWLIB_SYS_argc 22
#define TARGET_NEWLIB_SYS_argn 24
#define TARGET_NEWLIB_SYS_argnlen 23
#define TARGET_NEWLIB_SYS_argv 13
#define TARGET_NEWLIB_SYS_argvlen 12
#define TARGET_NEWLIB_SYS_chdir 14
#define TARGET_NEWLIB_SYS_chmod 16
#define TARGET_NEWLIB_SYS_close 3
#define TARGET_NEWLIB_SYS_exit 1
#define TARGET_NEWLIB_SYS_fstat 10
#define TARGET_NEWLIB_SYS_getpid 8
#define TARGET_NEWLIB_SYS_gettimeofday 19
#define TARGET_NEWLIB_SYS_kill 9
#define TARGET_NEWLIB_SYS_link 21
#define TARGET_NEWLIB_SYS_lseek 6
#define TARGET_NEWLIB_SYS_open 2
#define TARGET_NEWLIB_SYS_read 4
#define TARGET_NEWLIB_SYS_reconfig 25
#define TARGET_NEWLIB_SYS_stat 15
#define TARGET_NEWLIB_SYS_time 18
#define TARGET_NEWLIB_SYS_times 20
#define TARGET_NEWLIB_SYS_unlink 7
#define TARGET_NEWLIB_SYS_utime 17
#define TARGET_NEWLIB_SYS_write 5
  /* gennltvals: END */

#endif
