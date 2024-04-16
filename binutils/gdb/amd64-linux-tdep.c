/* Target-dependent code for GNU/Linux x86-64.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.
   Contributed by Jiri Smid, SuSE Labs.

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

#include "defs.h"
#include "arch-utils.h"
#include "frame.h"
#include "gdbcore.h"
#include "regcache.h"
#include "osabi.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "reggroups.h"
#include "regset.h"
#include "parser-defs.h"
#include "user-regs.h"
#include "amd64-linux-tdep.h"
#include "i386-linux-tdep.h"
#include "linux-tdep.h"
#include "gdbsupport/x86-xstate.h"

#include "amd64-tdep.h"
#include "solib-svr4.h"
#include "xml-syscall.h"
#include "glibc-tdep.h"
#include "arch/amd64.h"
#include "target-descriptions.h"
#include "expop.h"

/* The syscall's XML filename for i386.  */
#define XML_SYSCALL_FILENAME_AMD64 "syscalls/amd64-linux.xml"

#include "record-full.h"
#include "linux-record.h"

/* Mapping between the general-purpose registers in `struct user'
   format and GDB's register cache layout.  */

/* From <sys/reg.h>.  */
int amd64_linux_gregset_reg_offset[] =
{
  10 * 8,			/* %rax */
  5 * 8,			/* %rbx */
  11 * 8,			/* %rcx */
  12 * 8,			/* %rdx */
  13 * 8,			/* %rsi */
  14 * 8,			/* %rdi */
  4 * 8,			/* %rbp */
  19 * 8,			/* %rsp */
  9 * 8,			/* %r8 ...  */
  8 * 8,
  7 * 8,
  6 * 8,
  3 * 8,
  2 * 8,
  1 * 8,
  0 * 8,			/* ... %r15 */
  16 * 8,			/* %rip */
  18 * 8,			/* %eflags */
  17 * 8,			/* %cs */
  20 * 8,			/* %ss */
  23 * 8,			/* %ds */
  24 * 8,			/* %es */
  25 * 8,			/* %fs */
  26 * 8,			/* %gs */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1,		/* MPX registers BND0 ... BND3.  */
  -1, -1,			/* MPX registers BNDCFGU and BNDSTATUS.  */
  -1, -1, -1, -1, -1, -1, -1, -1,     /* xmm16 ... xmm31 (AVX512)  */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,     /* ymm16 ... ymm31 (AVX512)  */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,     /* k0 ... k7 (AVX512)  */
  -1, -1, -1, -1, -1, -1, -1, -1,     /* zmm0 ... zmm31 (AVX512)  */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1,				/* PKEYS register pkru  */

  /* End of hardware registers */
  21 * 8, 22 * 8,		      /* fs_base and gs_base.  */
  15 * 8			      /* "orig_rax" */
};


/* Support for signal handlers.  */

#define LINUX_SIGTRAMP_INSN0	0x48	/* mov $NNNNNNNN, %rax */
#define LINUX_SIGTRAMP_OFFSET0	0
#define LINUX_SIGTRAMP_INSN1	0x0f	/* syscall */
#define LINUX_SIGTRAMP_OFFSET1	7

static const gdb_byte amd64_linux_sigtramp_code[] =
{
  /* mov $__NR_rt_sigreturn, %rax */
  LINUX_SIGTRAMP_INSN0, 0xc7, 0xc0, 0x0f, 0x00, 0x00, 0x00,
  /* syscall */
  LINUX_SIGTRAMP_INSN1, 0x05
};

static const gdb_byte amd64_x32_linux_sigtramp_code[] =
{
  /* mov $__NR_rt_sigreturn, %rax.  */
  LINUX_SIGTRAMP_INSN0, 0xc7, 0xc0, 0x01, 0x02, 0x00, 0x40,
  /* syscall */
  LINUX_SIGTRAMP_INSN1, 0x05
};

#define LINUX_SIGTRAMP_LEN (sizeof amd64_linux_sigtramp_code)

/* If PC is in a sigtramp routine, return the address of the start of
   the routine.  Otherwise, return 0.  */

static CORE_ADDR
amd64_linux_sigtramp_start (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch;
  const gdb_byte *sigtramp_code;
  CORE_ADDR pc = get_frame_pc (this_frame);
  gdb_byte buf[LINUX_SIGTRAMP_LEN];

  /* We only recognize a signal trampoline if PC is at the start of
     one of the two instructions.  We optimize for finding the PC at
     the start, as will be the case when the trampoline is not the
     first frame on the stack.  We assume that in the case where the
     PC is not at the start of the instruction sequence, there will be
     a few trailing readable bytes on the stack.  */

  if (!safe_frame_unwind_memory (this_frame, pc, buf))
    return 0;

  if (buf[0] != LINUX_SIGTRAMP_INSN0)
    {
      if (buf[0] != LINUX_SIGTRAMP_INSN1)
	return 0;

      pc -= LINUX_SIGTRAMP_OFFSET1;
      if (!safe_frame_unwind_memory (this_frame, pc, buf))
	return 0;
    }

  gdbarch = get_frame_arch (this_frame);
  if (gdbarch_ptr_bit (gdbarch) == 32)
    sigtramp_code = amd64_x32_linux_sigtramp_code;
  else
    sigtramp_code = amd64_linux_sigtramp_code;
  if (memcmp (buf, sigtramp_code, LINUX_SIGTRAMP_LEN) != 0)
    return 0;

  return pc;
}

/* Return whether THIS_FRAME corresponds to a GNU/Linux sigtramp
   routine.  */

static int
amd64_linux_sigtramp_p (frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);

  /* If we have NAME, we can optimize the search.  The trampoline is
     named __restore_rt.  However, it isn't dynamically exported from
     the shared C library, so the trampoline may appear to be part of
     the preceding function.  This should always be sigaction,
     __sigaction, or __libc_sigaction (all aliases to the same
     function).  */
  if (name == NULL || strstr (name, "sigaction") != NULL)
    return (amd64_linux_sigtramp_start (this_frame) != 0);

  return (strcmp ("__restore_rt", name) == 0);
}

/* Offset to struct sigcontext in ucontext, from <asm/ucontext.h>.  */
#define AMD64_LINUX_UCONTEXT_SIGCONTEXT_OFFSET 40

/* Assuming THIS_FRAME is a GNU/Linux sigtramp routine, return the
   address of the associated sigcontext structure.  */

static CORE_ADDR
amd64_linux_sigcontext_addr (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp;
  gdb_byte buf[8];

  get_frame_register (this_frame, AMD64_RSP_REGNUM, buf);
  sp = extract_unsigned_integer (buf, 8, byte_order);

  /* The sigcontext structure is part of the user context.  A pointer
     to the user context is passed as the third argument to the signal
     handler, i.e. in %rdx.  Unfortunately %rdx isn't preserved across
     function calls so we can't use it.  Fortunately the user context
     is part of the signal frame and the unwound %rsp directly points
     at it.  */
  return sp + AMD64_LINUX_UCONTEXT_SIGCONTEXT_OFFSET;
}


static LONGEST
amd64_linux_get_syscall_number (struct gdbarch *gdbarch,
				thread_info *thread)
{
  struct regcache *regcache = get_thread_regcache (thread);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  /* The content of a register.  */
  gdb_byte buf[8];
  /* The result.  */
  LONGEST ret;

  /* Getting the system call number from the register.
     When dealing with x86_64 architecture, this information
     is stored at %rax register.  */
  regcache->cooked_read (AMD64_LINUX_ORIG_RAX_REGNUM, buf);

  ret = extract_signed_integer (buf, byte_order);

  return ret;
}


/* From <asm/sigcontext.h>.  */
static int amd64_linux_sc_reg_offset[] =
{
  13 * 8,			/* %rax */
  11 * 8,			/* %rbx */
  14 * 8,			/* %rcx */
  12 * 8,			/* %rdx */
  9 * 8,			/* %rsi */
  8 * 8,			/* %rdi */
  10 * 8,			/* %rbp */
  15 * 8,			/* %rsp */
  0 * 8,			/* %r8 */
  1 * 8,			/* %r9 */
  2 * 8,			/* %r10 */
  3 * 8,			/* %r11 */
  4 * 8,			/* %r12 */
  5 * 8,			/* %r13 */
  6 * 8,			/* %r14 */
  7 * 8,			/* %r15 */
  16 * 8,			/* %rip */
  17 * 8,			/* %eflags */

  /* FIXME: kettenis/2002030531: The registers %cs, %fs and %gs are
     available in `struct sigcontext'.  However, they only occupy two
     bytes instead of four, which makes using them here rather
     difficult.  Leave them out for now.  */
  -1,				/* %cs */
  -1,				/* %ss */
  -1,				/* %ds */
  -1,				/* %es */
  -1,				/* %fs */
  -1				/* %gs */
};

static int
amd64_linux_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
				 const struct reggroup *group)
{ 
  if (regnum == AMD64_LINUX_ORIG_RAX_REGNUM)
    return (group == system_reggroup
	    || group == save_reggroup
	    || group == restore_reggroup);
  return i386_register_reggroup_p (gdbarch, regnum, group);
}

/* Set the program counter for process PTID to PC.  */

static void
amd64_linux_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  regcache_cooked_write_unsigned (regcache, AMD64_RIP_REGNUM, pc);

  /* We must be careful with modifying the program counter.  If we
     just interrupted a system call, the kernel might try to restart
     it when we resume the inferior.  On restarting the system call,
     the kernel will try backing up the program counter even though it
     no longer points at the system call.  This typically results in a
     SIGSEGV or SIGILL.  We can prevent this by writing `-1' in the
     "orig_rax" pseudo-register.

     Note that "orig_rax" is saved when setting up a dummy call frame.
     This means that it is properly restored when that frame is
     popped, and that the interrupted system call will be restarted
     when we resume the inferior on return from a function call from
     within GDB.  In all other cases the system call will not be
     restarted.  */
  regcache_cooked_write_unsigned (regcache, AMD64_LINUX_ORIG_RAX_REGNUM, -1);
}

/* Record all registers but IP register for process-record.  */

static int
amd64_all_but_ip_registers_record (struct regcache *regcache)
{
  if (record_full_arch_list_add_reg (regcache, AMD64_RAX_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_RCX_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_RDX_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_RBX_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_RSP_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_RBP_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_RSI_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_RDI_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_R8_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_R9_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_R10_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_R11_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_R12_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_R13_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_R14_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_R15_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_EFLAGS_REGNUM))
    return -1;

  return 0;
}

/* amd64_canonicalize_syscall maps from the native amd64 Linux set 
   of syscall ids into a canonical set of syscall ids used by 
   process record.  */

static enum gdb_syscall
amd64_canonicalize_syscall (enum amd64_syscall syscall_number)
{
  DIAGNOSTIC_PUSH
  DIAGNOSTIC_IGNORE_SWITCH_DIFFERENT_ENUM_TYPES

  switch (syscall_number) {
  case amd64_sys_read:
  case amd64_x32_sys_read:
    return gdb_sys_read;

  case amd64_sys_write:
  case amd64_x32_sys_write:
    return gdb_sys_write;

  case amd64_sys_open:
  case amd64_x32_sys_open:
    return gdb_sys_open;

  case amd64_sys_close:
  case amd64_x32_sys_close:
    return gdb_sys_close;

  case amd64_sys_newstat:
  case amd64_x32_sys_newstat:
    return gdb_sys_newstat;

  case amd64_sys_newfstat:
  case amd64_x32_sys_newfstat:
    return gdb_sys_newfstat;

  case amd64_sys_newlstat:
  case amd64_x32_sys_newlstat:
    return gdb_sys_newlstat;

  case amd64_sys_poll:
  case amd64_x32_sys_poll:
    return gdb_sys_poll;

  case amd64_sys_lseek:
  case amd64_x32_sys_lseek:
    return gdb_sys_lseek;

  case amd64_sys_mmap:
  case amd64_x32_sys_mmap:
    return gdb_sys_mmap2;

  case amd64_sys_mprotect:
  case amd64_x32_sys_mprotect:
    return gdb_sys_mprotect;

  case amd64_sys_munmap:
  case amd64_x32_sys_munmap:
    return gdb_sys_munmap;

  case amd64_sys_brk:
  case amd64_x32_sys_brk:
    return gdb_sys_brk;

  case amd64_sys_rt_sigaction:
  case amd64_x32_sys_rt_sigaction:
    return gdb_sys_rt_sigaction;

  case amd64_sys_rt_sigprocmask:
  case amd64_x32_sys_rt_sigprocmask:
    return gdb_sys_rt_sigprocmask;

  case amd64_sys_rt_sigreturn:
  case amd64_x32_sys_rt_sigreturn:
    return gdb_sys_rt_sigreturn;

  case amd64_sys_ioctl:
  case amd64_x32_sys_ioctl:
    return gdb_sys_ioctl;

  case amd64_sys_pread64:
  case amd64_x32_sys_pread64:
    return gdb_sys_pread64;

  case amd64_sys_pwrite64:
  case amd64_x32_sys_pwrite64:
    return gdb_sys_pwrite64;

  case amd64_sys_readv:
  case amd64_x32_sys_readv:
    return gdb_sys_readv;

  case amd64_sys_writev:
  case amd64_x32_sys_writev:
    return gdb_sys_writev;

  case amd64_sys_access:
  case amd64_x32_sys_access:
    return gdb_sys_access;

  case amd64_sys_pipe:
  case amd64_x32_sys_pipe:
    return gdb_sys_pipe;

  case amd64_sys_pipe2:
    return gdb_sys_pipe2;

  case amd64_sys_getrandom:
    return gdb_sys_getrandom;

  case amd64_sys_select:
  case amd64_x32_sys_select:
    return gdb_sys_select;

  case amd64_sys_sched_yield:
  case amd64_x32_sys_sched_yield:
    return gdb_sys_sched_yield;

  case amd64_sys_mremap:
  case amd64_x32_sys_mremap:
    return gdb_sys_mremap;

  case amd64_sys_msync:
  case amd64_x32_sys_msync:
    return gdb_sys_msync;

  case amd64_sys_mincore:
  case amd64_x32_sys_mincore:
    return gdb_sys_mincore;

  case amd64_sys_madvise:
  case amd64_x32_sys_madvise:
    return gdb_sys_madvise;

  case amd64_sys_shmget:
  case amd64_x32_sys_shmget:
    return gdb_sys_shmget;

  case amd64_sys_shmat:
  case amd64_x32_sys_shmat:
    return gdb_sys_shmat;

  case amd64_sys_shmctl:
  case amd64_x32_sys_shmctl:
    return gdb_sys_shmctl;

  case amd64_sys_dup:
  case amd64_x32_sys_dup:
    return gdb_sys_dup;

  case amd64_sys_dup2:
  case amd64_x32_sys_dup2:
    return gdb_sys_dup2;

  case amd64_sys_pause:
  case amd64_x32_sys_pause:
    return gdb_sys_pause;

  case amd64_sys_nanosleep:
  case amd64_x32_sys_nanosleep:
    return gdb_sys_nanosleep;

  case amd64_sys_getitimer:
  case amd64_x32_sys_getitimer:
    return gdb_sys_getitimer;

  case amd64_sys_alarm:
  case amd64_x32_sys_alarm:
    return gdb_sys_alarm;

  case amd64_sys_setitimer:
  case amd64_x32_sys_setitimer:
    return gdb_sys_setitimer;

  case amd64_sys_getpid:
  case amd64_x32_sys_getpid:
    return gdb_sys_getpid;

  case amd64_sys_sendfile64:
  case amd64_x32_sys_sendfile64:
    return gdb_sys_sendfile64;

  case amd64_sys_socket:
  case amd64_x32_sys_socket:
    return gdb_sys_socket;

  case amd64_sys_connect:
  case amd64_x32_sys_connect:
    return gdb_sys_connect;

  case amd64_sys_accept:
  case amd64_x32_sys_accept:
    return gdb_sys_accept;

  case amd64_sys_sendto:
  case amd64_x32_sys_sendto:
    return gdb_sys_sendto;

  case amd64_sys_recvfrom:
  case amd64_x32_sys_recvfrom:
    return gdb_sys_recvfrom;

  case amd64_sys_sendmsg:
  case amd64_x32_sys_sendmsg:
    return gdb_sys_sendmsg;

  case amd64_sys_recvmsg:
  case amd64_x32_sys_recvmsg:
    return gdb_sys_recvmsg;

  case amd64_sys_shutdown:
  case amd64_x32_sys_shutdown:
    return gdb_sys_shutdown;

  case amd64_sys_bind:
  case amd64_x32_sys_bind:
    return gdb_sys_bind;

  case amd64_sys_listen:
  case amd64_x32_sys_listen:
    return gdb_sys_listen;

  case amd64_sys_getsockname:
  case amd64_x32_sys_getsockname:
    return gdb_sys_getsockname;

  case amd64_sys_getpeername:
  case amd64_x32_sys_getpeername:
    return gdb_sys_getpeername;

  case amd64_sys_socketpair:
  case amd64_x32_sys_socketpair:
    return gdb_sys_socketpair;

  case amd64_sys_setsockopt:
  case amd64_x32_sys_setsockopt:
    return gdb_sys_setsockopt;

  case amd64_sys_getsockopt:
  case amd64_x32_sys_getsockopt:
    return gdb_sys_getsockopt;

  case amd64_sys_clone:
  case amd64_x32_sys_clone:
    return gdb_sys_clone;

  case amd64_sys_fork:
  case amd64_x32_sys_fork:
    return gdb_sys_fork;

  case amd64_sys_vfork:
  case amd64_x32_sys_vfork:
    return gdb_sys_vfork;

  case amd64_sys_execve:
  case amd64_x32_sys_execve:
    return gdb_sys_execve;

  case amd64_sys_exit:
  case amd64_x32_sys_exit:
    return gdb_sys_exit;

  case amd64_sys_wait4:
  case amd64_x32_sys_wait4:
    return gdb_sys_wait4;

  case amd64_sys_kill:
  case amd64_x32_sys_kill:
    return gdb_sys_kill;

  case amd64_sys_uname:
  case amd64_x32_sys_uname:
    return gdb_sys_uname;

  case amd64_sys_semget:
  case amd64_x32_sys_semget:
    return gdb_sys_semget;

  case amd64_sys_semop:
  case amd64_x32_sys_semop:
    return gdb_sys_semop;

  case amd64_sys_semctl:
  case amd64_x32_sys_semctl:
    return gdb_sys_semctl;

  case amd64_sys_shmdt:
  case amd64_x32_sys_shmdt:
    return gdb_sys_shmdt;

  case amd64_sys_msgget:
  case amd64_x32_sys_msgget:
    return gdb_sys_msgget;

  case amd64_sys_msgsnd:
  case amd64_x32_sys_msgsnd:
    return gdb_sys_msgsnd;

  case amd64_sys_msgrcv:
  case amd64_x32_sys_msgrcv:
    return gdb_sys_msgrcv;

  case amd64_sys_msgctl:
  case amd64_x32_sys_msgctl:
    return gdb_sys_msgctl;

  case amd64_sys_fcntl:
  case amd64_x32_sys_fcntl:
    return gdb_sys_fcntl;

  case amd64_sys_flock:
  case amd64_x32_sys_flock:
    return gdb_sys_flock;

  case amd64_sys_fsync:
  case amd64_x32_sys_fsync:
    return gdb_sys_fsync;

  case amd64_sys_fdatasync:
  case amd64_x32_sys_fdatasync:
    return gdb_sys_fdatasync;

  case amd64_sys_truncate:
  case amd64_x32_sys_truncate:
    return gdb_sys_truncate;

  case amd64_sys_ftruncate:
  case amd64_x32_sys_ftruncate:
    return gdb_sys_ftruncate;

  case amd64_sys_getdents:
  case amd64_x32_sys_getdents:
    return gdb_sys_getdents;

  case amd64_sys_getcwd:
  case amd64_x32_sys_getcwd:
    return gdb_sys_getcwd;

  case amd64_sys_chdir:
  case amd64_x32_sys_chdir:
    return gdb_sys_chdir;

  case amd64_sys_fchdir:
  case amd64_x32_sys_fchdir:
    return gdb_sys_fchdir;

  case amd64_sys_rename:
  case amd64_x32_sys_rename:
    return gdb_sys_rename;

  case amd64_sys_mkdir:
  case amd64_x32_sys_mkdir:
    return gdb_sys_mkdir;

  case amd64_sys_rmdir:
  case amd64_x32_sys_rmdir:
    return gdb_sys_rmdir;

  case amd64_sys_creat:
  case amd64_x32_sys_creat:
    return gdb_sys_creat;

  case amd64_sys_link:
  case amd64_x32_sys_link:
    return gdb_sys_link;

  case amd64_sys_unlink:
  case amd64_x32_sys_unlink:
    return gdb_sys_unlink;

  case amd64_sys_symlink:
  case amd64_x32_sys_symlink:
    return gdb_sys_symlink;

  case amd64_sys_readlink:
  case amd64_x32_sys_readlink:
    return gdb_sys_readlink;

  case amd64_sys_chmod:
  case amd64_x32_sys_chmod:
    return gdb_sys_chmod;

  case amd64_sys_fchmod:
  case amd64_x32_sys_fchmod:
    return gdb_sys_fchmod;

  case amd64_sys_chown:
  case amd64_x32_sys_chown:
    return gdb_sys_chown;

  case amd64_sys_fchown:
  case amd64_x32_sys_fchown:
    return gdb_sys_fchown;

  case amd64_sys_lchown:
  case amd64_x32_sys_lchown:
    return gdb_sys_lchown;

  case amd64_sys_umask:
  case amd64_x32_sys_umask:
    return gdb_sys_umask;

  case amd64_sys_gettimeofday:
  case amd64_x32_sys_gettimeofday:
    return gdb_sys_gettimeofday;

  case amd64_sys_getrlimit:
  case amd64_x32_sys_getrlimit:
    return gdb_sys_getrlimit;

  case amd64_sys_getrusage:
  case amd64_x32_sys_getrusage:
    return gdb_sys_getrusage;

  case amd64_sys_sysinfo:
  case amd64_x32_sys_sysinfo:
    return gdb_sys_sysinfo;

  case amd64_sys_times:
  case amd64_x32_sys_times:
    return gdb_sys_times;

  case amd64_sys_ptrace:
  case amd64_x32_sys_ptrace:
    return gdb_sys_ptrace;

  case amd64_sys_getuid:
  case amd64_x32_sys_getuid:
    return gdb_sys_getuid;

  case amd64_sys_syslog:
  case amd64_x32_sys_syslog:
    return gdb_sys_syslog;

  case amd64_sys_getgid:
  case amd64_x32_sys_getgid:
    return gdb_sys_getgid;

  case amd64_sys_setuid:
  case amd64_x32_sys_setuid:
    return gdb_sys_setuid;

  case amd64_sys_setgid:
  case amd64_x32_sys_setgid:
    return gdb_sys_setgid;

  case amd64_sys_geteuid:
  case amd64_x32_sys_geteuid:
    return gdb_sys_geteuid;

  case amd64_sys_getegid:
  case amd64_x32_sys_getegid:
    return gdb_sys_getegid;

  case amd64_sys_setpgid:
  case amd64_x32_sys_setpgid:
    return gdb_sys_setpgid;

  case amd64_sys_getppid:
  case amd64_x32_sys_getppid:
    return gdb_sys_getppid;

  case amd64_sys_getpgrp:
  case amd64_x32_sys_getpgrp:
    return gdb_sys_getpgrp;

  case amd64_sys_setsid:
  case amd64_x32_sys_setsid:
    return gdb_sys_setsid;

  case amd64_sys_setreuid:
  case amd64_x32_sys_setreuid:
    return gdb_sys_setreuid;

  case amd64_sys_setregid:
  case amd64_x32_sys_setregid:
    return gdb_sys_setregid;

  case amd64_sys_getgroups:
  case amd64_x32_sys_getgroups:
    return gdb_sys_getgroups;

  case amd64_sys_setgroups:
  case amd64_x32_sys_setgroups:
    return gdb_sys_setgroups;

  case amd64_sys_setresuid:
  case amd64_x32_sys_setresuid:
    return gdb_sys_setresuid;

  case amd64_sys_getresuid:
  case amd64_x32_sys_getresuid:
    return gdb_sys_getresuid;

  case amd64_sys_setresgid:
  case amd64_x32_sys_setresgid:
    return gdb_sys_setresgid;

  case amd64_sys_getresgid:
  case amd64_x32_sys_getresgid:
    return gdb_sys_getresgid;

  case amd64_sys_getpgid:
  case amd64_x32_sys_getpgid:
    return gdb_sys_getpgid;

  case amd64_sys_setfsuid:
  case amd64_x32_sys_setfsuid:
    return gdb_sys_setfsuid;

  case amd64_sys_setfsgid:
  case amd64_x32_sys_setfsgid:
    return gdb_sys_setfsgid;

  case amd64_sys_getsid:
  case amd64_x32_sys_getsid:
    return gdb_sys_getsid;

  case amd64_sys_capget:
  case amd64_x32_sys_capget:
    return gdb_sys_capget;

  case amd64_sys_capset:
  case amd64_x32_sys_capset:
    return gdb_sys_capset;

  case amd64_sys_rt_sigpending:
  case amd64_x32_sys_rt_sigpending:
    return gdb_sys_rt_sigpending;

  case amd64_sys_rt_sigtimedwait:
  case amd64_x32_sys_rt_sigtimedwait:
    return gdb_sys_rt_sigtimedwait;

  case amd64_sys_rt_sigqueueinfo:
  case amd64_x32_sys_rt_sigqueueinfo:
    return gdb_sys_rt_sigqueueinfo;

  case amd64_sys_rt_sigsuspend:
  case amd64_x32_sys_rt_sigsuspend:
    return gdb_sys_rt_sigsuspend;

  case amd64_sys_sigaltstack:
  case amd64_x32_sys_sigaltstack:
    return gdb_sys_sigaltstack;

  case amd64_sys_utime:
  case amd64_x32_sys_utime:
    return gdb_sys_utime;

  case amd64_sys_mknod:
  case amd64_x32_sys_mknod:
    return gdb_sys_mknod;

  case amd64_sys_personality:
  case amd64_x32_sys_personality:
    return gdb_sys_personality;

  case amd64_sys_ustat:
  case amd64_x32_sys_ustat:
    return gdb_sys_ustat;

  case amd64_sys_statfs:
  case amd64_x32_sys_statfs:
    return gdb_sys_statfs;

  case amd64_sys_fstatfs:
  case amd64_x32_sys_fstatfs:
    return gdb_sys_fstatfs;

  case amd64_sys_sysfs:
  case amd64_x32_sys_sysfs:
    return gdb_sys_sysfs;

  case amd64_sys_getpriority:
  case amd64_x32_sys_getpriority:
    return gdb_sys_getpriority;

  case amd64_sys_setpriority:
  case amd64_x32_sys_setpriority:
    return gdb_sys_setpriority;

  case amd64_sys_sched_setparam:
  case amd64_x32_sys_sched_setparam:
    return gdb_sys_sched_setparam;

  case amd64_sys_sched_getparam:
  case amd64_x32_sys_sched_getparam:
    return gdb_sys_sched_getparam;

  case amd64_sys_sched_setscheduler:
  case amd64_x32_sys_sched_setscheduler:
    return gdb_sys_sched_setscheduler;

  case amd64_sys_sched_getscheduler:
  case amd64_x32_sys_sched_getscheduler:
    return gdb_sys_sched_getscheduler;

  case amd64_sys_sched_get_priority_max:
  case amd64_x32_sys_sched_get_priority_max:
    return gdb_sys_sched_get_priority_max;

  case amd64_sys_sched_get_priority_min:
  case amd64_x32_sys_sched_get_priority_min:
    return gdb_sys_sched_get_priority_min;

  case amd64_sys_sched_rr_get_interval:
  case amd64_x32_sys_sched_rr_get_interval:
    return gdb_sys_sched_rr_get_interval;

  case amd64_sys_mlock:
  case amd64_x32_sys_mlock:
    return gdb_sys_mlock;

  case amd64_sys_munlock:
  case amd64_x32_sys_munlock:
    return gdb_sys_munlock;

  case amd64_sys_mlockall:
  case amd64_x32_sys_mlockall:
    return gdb_sys_mlockall;

  case amd64_sys_munlockall:
  case amd64_x32_sys_munlockall:
    return gdb_sys_munlockall;

  case amd64_sys_vhangup:
  case amd64_x32_sys_vhangup:
    return gdb_sys_vhangup;

  case amd64_sys_modify_ldt:
  case amd64_x32_sys_modify_ldt:
    return gdb_sys_modify_ldt;

  case amd64_sys_pivot_root:
  case amd64_x32_sys_pivot_root:
    return gdb_sys_pivot_root;

  case amd64_sys_sysctl:
  case amd64_x32_sys_sysctl:
    return gdb_sys_sysctl;

  case amd64_sys_prctl:
  case amd64_x32_sys_prctl:
    return gdb_sys_prctl;

  case amd64_sys_arch_prctl:
  case amd64_x32_sys_arch_prctl:
    return gdb_sys_no_syscall;	/* Note */

  case amd64_sys_adjtimex:
  case amd64_x32_sys_adjtimex:
    return gdb_sys_adjtimex;

  case amd64_sys_setrlimit:
  case amd64_x32_sys_setrlimit:
    return gdb_sys_setrlimit;

  case amd64_sys_chroot:
  case amd64_x32_sys_chroot:
    return gdb_sys_chroot;

  case amd64_sys_sync:
  case amd64_x32_sys_sync:
    return gdb_sys_sync;

  case amd64_sys_acct:
  case amd64_x32_sys_acct:
    return gdb_sys_acct;

  case amd64_sys_settimeofday:
  case amd64_x32_sys_settimeofday:
    return gdb_sys_settimeofday;

  case amd64_sys_mount:
  case amd64_x32_sys_mount:
    return gdb_sys_mount;

  case amd64_sys_umount:
  case amd64_x32_sys_umount:
    return gdb_sys_umount;

  case amd64_sys_swapon:
  case amd64_x32_sys_swapon:
    return gdb_sys_swapon;

  case amd64_sys_swapoff:
  case amd64_x32_sys_swapoff:
    return gdb_sys_swapoff;

  case amd64_sys_reboot:
  case amd64_x32_sys_reboot:
    return gdb_sys_reboot;

  case amd64_sys_sethostname:
  case amd64_x32_sys_sethostname:
    return gdb_sys_sethostname;

  case amd64_sys_setdomainname:
  case amd64_x32_sys_setdomainname:
    return gdb_sys_setdomainname;

  case amd64_sys_iopl:
  case amd64_x32_sys_iopl:
    return gdb_sys_iopl;

  case amd64_sys_ioperm:
  case amd64_x32_sys_ioperm:
    return gdb_sys_ioperm;

  case amd64_sys_init_module:
  case amd64_x32_sys_init_module:
    return gdb_sys_init_module;

  case amd64_sys_delete_module:
  case amd64_x32_sys_delete_module:
    return gdb_sys_delete_module;

  case amd64_sys_quotactl:
  case amd64_x32_sys_quotactl:
    return gdb_sys_quotactl;

  case amd64_sys_nfsservctl:
    return gdb_sys_nfsservctl;

  case amd64_sys_gettid:
  case amd64_x32_sys_gettid:
    return gdb_sys_gettid;

  case amd64_sys_readahead:
  case amd64_x32_sys_readahead:
    return gdb_sys_readahead;

  case amd64_sys_setxattr:
  case amd64_x32_sys_setxattr:
    return gdb_sys_setxattr;

  case amd64_sys_lsetxattr:
  case amd64_x32_sys_lsetxattr:
    return gdb_sys_lsetxattr;

  case amd64_sys_fsetxattr:
  case amd64_x32_sys_fsetxattr:
    return gdb_sys_fsetxattr;

  case amd64_sys_getxattr:
  case amd64_x32_sys_getxattr:
    return gdb_sys_getxattr;

  case amd64_sys_lgetxattr:
  case amd64_x32_sys_lgetxattr:
    return gdb_sys_lgetxattr;

  case amd64_sys_fgetxattr:
  case amd64_x32_sys_fgetxattr:
    return gdb_sys_fgetxattr;

  case amd64_sys_listxattr:
  case amd64_x32_sys_listxattr:
    return gdb_sys_listxattr;

  case amd64_sys_llistxattr:
  case amd64_x32_sys_llistxattr:
    return gdb_sys_llistxattr;

  case amd64_sys_flistxattr:
  case amd64_x32_sys_flistxattr:
    return gdb_sys_flistxattr;

  case amd64_sys_removexattr:
  case amd64_x32_sys_removexattr:
    return gdb_sys_removexattr;

  case amd64_sys_lremovexattr:
  case amd64_x32_sys_lremovexattr:
    return gdb_sys_lremovexattr;

  case amd64_sys_fremovexattr:
  case amd64_x32_sys_fremovexattr:
    return gdb_sys_fremovexattr;

  case amd64_sys_tkill:
  case amd64_x32_sys_tkill:
    return gdb_sys_tkill;

  case amd64_sys_time:
  case amd64_x32_sys_time:
    return gdb_sys_time;

  case amd64_sys_futex:
  case amd64_x32_sys_futex:
    return gdb_sys_futex;

  case amd64_sys_sched_setaffinity:
  case amd64_x32_sys_sched_setaffinity:
    return gdb_sys_sched_setaffinity;

  case amd64_sys_sched_getaffinity:
  case amd64_x32_sys_sched_getaffinity:
    return gdb_sys_sched_getaffinity;

  case amd64_sys_io_setup:
  case amd64_x32_sys_io_setup:
    return gdb_sys_io_setup;

  case amd64_sys_io_destroy:
  case amd64_x32_sys_io_destroy:
    return gdb_sys_io_destroy;

  case amd64_sys_io_getevents:
  case amd64_x32_sys_io_getevents:
    return gdb_sys_io_getevents;

  case amd64_sys_io_submit:
  case amd64_x32_sys_io_submit:
    return gdb_sys_io_submit;

  case amd64_sys_io_cancel:
  case amd64_x32_sys_io_cancel:
    return gdb_sys_io_cancel;

  case amd64_sys_lookup_dcookie:
  case amd64_x32_sys_lookup_dcookie:
    return gdb_sys_lookup_dcookie;

  case amd64_sys_epoll_create:
  case amd64_x32_sys_epoll_create:
    return gdb_sys_epoll_create;

  case amd64_sys_remap_file_pages:
  case amd64_x32_sys_remap_file_pages:
    return gdb_sys_remap_file_pages;

  case amd64_sys_getdents64:
  case amd64_x32_sys_getdents64:
    return gdb_sys_getdents64;

  case amd64_sys_set_tid_address:
  case amd64_x32_sys_set_tid_address:
    return gdb_sys_set_tid_address;

  case amd64_sys_restart_syscall:
  case amd64_x32_sys_restart_syscall:
    return gdb_sys_restart_syscall;

  case amd64_sys_semtimedop:
  case amd64_x32_sys_semtimedop:
    return gdb_sys_semtimedop;

  case amd64_sys_fadvise64:
  case amd64_x32_sys_fadvise64:
    return gdb_sys_fadvise64;

  case amd64_sys_timer_create:
  case amd64_x32_sys_timer_create:
    return gdb_sys_timer_create;

  case amd64_sys_timer_settime:
  case amd64_x32_sys_timer_settime:
    return gdb_sys_timer_settime;

  case amd64_sys_timer_gettime:
  case amd64_x32_sys_timer_gettime:
    return gdb_sys_timer_gettime;

  case amd64_sys_timer_getoverrun:
  case amd64_x32_sys_timer_getoverrun:
    return gdb_sys_timer_getoverrun;

  case amd64_sys_timer_delete:
  case amd64_x32_sys_timer_delete:
    return gdb_sys_timer_delete;

  case amd64_sys_clock_settime:
  case amd64_x32_sys_clock_settime:
    return gdb_sys_clock_settime;

  case amd64_sys_clock_gettime:
  case amd64_x32_sys_clock_gettime:
    return gdb_sys_clock_gettime;

  case amd64_sys_clock_getres:
  case amd64_x32_sys_clock_getres:
    return gdb_sys_clock_getres;

  case amd64_sys_clock_nanosleep:
  case amd64_x32_sys_clock_nanosleep:
    return gdb_sys_clock_nanosleep;

  case amd64_sys_exit_group:
  case amd64_x32_sys_exit_group:
    return gdb_sys_exit_group;

  case amd64_sys_epoll_wait:
  case amd64_x32_sys_epoll_wait:
    return gdb_sys_epoll_wait;

  case amd64_sys_epoll_ctl:
  case amd64_x32_sys_epoll_ctl:
    return gdb_sys_epoll_ctl;

  case amd64_sys_tgkill:
  case amd64_x32_sys_tgkill:
    return gdb_sys_tgkill;

  case amd64_sys_utimes:
  case amd64_x32_sys_utimes:
    return gdb_sys_utimes;

  case amd64_sys_mbind:
  case amd64_x32_sys_mbind:
    return gdb_sys_mbind;

  case amd64_sys_set_mempolicy:
  case amd64_x32_sys_set_mempolicy:
    return gdb_sys_set_mempolicy;

  case amd64_sys_get_mempolicy:
  case amd64_x32_sys_get_mempolicy:
    return gdb_sys_get_mempolicy;

  case amd64_sys_mq_open:
  case amd64_x32_sys_mq_open:
    return gdb_sys_mq_open;

  case amd64_sys_mq_unlink:
  case amd64_x32_sys_mq_unlink:
    return gdb_sys_mq_unlink;

  case amd64_sys_mq_timedsend:
  case amd64_x32_sys_mq_timedsend:
    return gdb_sys_mq_timedsend;

  case amd64_sys_mq_timedreceive:
  case amd64_x32_sys_mq_timedreceive:
    return gdb_sys_mq_timedreceive;

  case amd64_sys_mq_notify:
  case amd64_x32_sys_mq_notify:
    return gdb_sys_mq_notify;

  case amd64_sys_mq_getsetattr:
  case amd64_x32_sys_mq_getsetattr:
    return gdb_sys_mq_getsetattr;

  case amd64_sys_kexec_load:
  case amd64_x32_sys_kexec_load:
    return gdb_sys_kexec_load;

  case amd64_sys_waitid:
  case amd64_x32_sys_waitid:
    return gdb_sys_waitid;

  case amd64_sys_add_key:
  case amd64_x32_sys_add_key:
    return gdb_sys_add_key;

  case amd64_sys_request_key:
  case amd64_x32_sys_request_key:
    return gdb_sys_request_key;

  case amd64_sys_keyctl:
  case amd64_x32_sys_keyctl:
    return gdb_sys_keyctl;

  case amd64_sys_ioprio_set:
  case amd64_x32_sys_ioprio_set:
    return gdb_sys_ioprio_set;

  case amd64_sys_ioprio_get:
  case amd64_x32_sys_ioprio_get:
    return gdb_sys_ioprio_get;

  case amd64_sys_inotify_init:
  case amd64_x32_sys_inotify_init:
    return gdb_sys_inotify_init;

  case amd64_sys_inotify_add_watch:
  case amd64_x32_sys_inotify_add_watch:
    return gdb_sys_inotify_add_watch;

  case amd64_sys_inotify_rm_watch:
  case amd64_x32_sys_inotify_rm_watch:
    return gdb_sys_inotify_rm_watch;

  case amd64_sys_migrate_pages:
  case amd64_x32_sys_migrate_pages:
    return gdb_sys_migrate_pages;

  case amd64_sys_openat:
  case amd64_x32_sys_openat:
    return gdb_sys_openat;

  case amd64_sys_mkdirat:
  case amd64_x32_sys_mkdirat:
    return gdb_sys_mkdirat;

  case amd64_sys_mknodat:
  case amd64_x32_sys_mknodat:
    return gdb_sys_mknodat;

  case amd64_sys_fchownat:
  case amd64_x32_sys_fchownat:
    return gdb_sys_fchownat;

  case amd64_sys_futimesat:
  case amd64_x32_sys_futimesat:
    return gdb_sys_futimesat;

  case amd64_sys_newfstatat:
  case amd64_x32_sys_newfstatat:
    return gdb_sys_newfstatat;

  case amd64_sys_unlinkat:
  case amd64_x32_sys_unlinkat:
    return gdb_sys_unlinkat;

  case amd64_sys_renameat:
  case amd64_x32_sys_renameat:
    return gdb_sys_renameat;

  case amd64_sys_linkat:
  case amd64_x32_sys_linkat:
    return gdb_sys_linkat;

  case amd64_sys_symlinkat:
  case amd64_x32_sys_symlinkat:
    return gdb_sys_symlinkat;

  case amd64_sys_readlinkat:
  case amd64_x32_sys_readlinkat:
    return gdb_sys_readlinkat;

  case amd64_sys_fchmodat:
  case amd64_x32_sys_fchmodat:
    return gdb_sys_fchmodat;

  case amd64_sys_faccessat:
  case amd64_x32_sys_faccessat:
    return gdb_sys_faccessat;

  case amd64_sys_pselect6:
  case amd64_x32_sys_pselect6:
    return gdb_sys_pselect6;

  case amd64_sys_ppoll:
  case amd64_x32_sys_ppoll:
    return gdb_sys_ppoll;

  case amd64_sys_unshare:
  case amd64_x32_sys_unshare:
    return gdb_sys_unshare;

  case amd64_sys_set_robust_list:
  case amd64_x32_sys_set_robust_list:
    return gdb_sys_set_robust_list;

  case amd64_sys_get_robust_list:
  case amd64_x32_sys_get_robust_list:
    return gdb_sys_get_robust_list;

  case amd64_sys_splice:
  case amd64_x32_sys_splice:
    return gdb_sys_splice;

  case amd64_sys_tee:
  case amd64_x32_sys_tee:
    return gdb_sys_tee;

  case amd64_sys_sync_file_range:
  case amd64_x32_sys_sync_file_range:
    return gdb_sys_sync_file_range;

  case amd64_sys_vmsplice:
  case amd64_x32_sys_vmsplice:
    return gdb_sys_vmsplice;

  case amd64_sys_move_pages:
  case amd64_x32_sys_move_pages:
    return gdb_sys_move_pages;

  default:
    return gdb_sys_no_syscall;
  }

  DIAGNOSTIC_POP
}

/* Parse the arguments of current system call instruction and record
   the values of the registers and memory that will be changed into
   "record_full_arch_list".  This instruction is "syscall".

   Return -1 if something wrong.  */

static struct linux_record_tdep amd64_linux_record_tdep;
static struct linux_record_tdep amd64_x32_linux_record_tdep;

#define RECORD_ARCH_GET_FS	0x1003
#define RECORD_ARCH_GET_GS	0x1004

static int
amd64_linux_syscall_record_common (struct regcache *regcache,
				   struct linux_record_tdep *linux_record_tdep_p)
{
  int ret;
  ULONGEST syscall_native;
  enum gdb_syscall syscall_gdb = gdb_sys_no_syscall;

  regcache_raw_read_unsigned (regcache, AMD64_RAX_REGNUM, &syscall_native);

  switch (syscall_native)
    {
    case amd64_sys_rt_sigreturn:
    case amd64_x32_sys_rt_sigreturn:
      if (amd64_all_but_ip_registers_record (regcache))
	return -1;
      return 0;
      break;

    case amd64_sys_arch_prctl:
    case amd64_x32_sys_arch_prctl:
      {
	ULONGEST arg3;
	regcache_raw_read_unsigned (regcache, linux_record_tdep_p->arg3,
				    &arg3);
	if (arg3 == RECORD_ARCH_GET_FS || arg3 == RECORD_ARCH_GET_GS)
	  {
	    CORE_ADDR addr;

	    regcache_raw_read_unsigned (regcache,
					linux_record_tdep_p->arg2,
					&addr);
	    if (record_full_arch_list_add_mem
		(addr, linux_record_tdep_p->size_ulong))
	      return -1;
	  }
	goto record_regs;
      }
      break;
    }

  syscall_gdb
    = amd64_canonicalize_syscall ((enum amd64_syscall) syscall_native);

  if (syscall_gdb == gdb_sys_no_syscall)
    {
      gdb_printf (gdb_stderr,
		  _("Process record and replay target doesn't "
		    "support syscall number %s\n"), 
		  pulongest (syscall_native));
      return -1;
    }
  else
    {
      ret = record_linux_system_call (syscall_gdb, regcache,
				      linux_record_tdep_p);
      if (ret)
	return ret;
    }

 record_regs:
  /* Record the return value of the system call.  */
  if (record_full_arch_list_add_reg (regcache, AMD64_RCX_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, AMD64_R11_REGNUM))
    return -1;

  return 0;
}

static int
amd64_linux_syscall_record (struct regcache *regcache)
{
  return amd64_linux_syscall_record_common (regcache,
					    &amd64_linux_record_tdep);
}

static int
amd64_x32_linux_syscall_record (struct regcache *regcache)
{
  return amd64_linux_syscall_record_common (regcache,
					    &amd64_x32_linux_record_tdep);
}

#define AMD64_LINUX_redzone    128
#define AMD64_LINUX_xstate     512
#define AMD64_LINUX_frame_size 560

static int
amd64_linux_record_signal (struct gdbarch *gdbarch,
			   struct regcache *regcache,
			   enum gdb_signal signal)
{
  ULONGEST rsp;

  if (amd64_all_but_ip_registers_record (regcache))
    return -1;

  if (record_full_arch_list_add_reg (regcache, AMD64_RIP_REGNUM))
    return -1;

  /* Record the change in the stack.  */
  regcache_raw_read_unsigned (regcache, AMD64_RSP_REGNUM, &rsp);
  /* redzone
     sp -= 128; */
  rsp -= AMD64_LINUX_redzone;
  /* This is for xstate.
     sp -= sizeof (struct _fpstate);  */
  rsp -= AMD64_LINUX_xstate;
  /* This is for frame_size.
     sp -= sizeof (struct rt_sigframe);  */
  rsp -= AMD64_LINUX_frame_size;
  if (record_full_arch_list_add_mem (rsp, AMD64_LINUX_redzone
				     + AMD64_LINUX_xstate
				     + AMD64_LINUX_frame_size))
    return -1;

  if (record_full_arch_list_add_end ())
    return -1;

  return 0;
}

const target_desc *
amd64_linux_read_description (uint64_t xcr0_features_bit, bool is_x32)
{
  static target_desc *amd64_linux_tdescs \
    [2/*AVX*/][2/*MPX*/][2/*AVX512*/][2/*PKRU*/] = {};
  static target_desc *x32_linux_tdescs \
    [2/*AVX*/][2/*AVX512*/][2/*PKRU*/] = {};

  target_desc **tdesc;

  if (is_x32)
    {
      tdesc = &x32_linux_tdescs[(xcr0_features_bit & X86_XSTATE_AVX) ? 1 : 0 ]
	[(xcr0_features_bit & X86_XSTATE_AVX512) ? 1 : 0]
	[(xcr0_features_bit & X86_XSTATE_PKRU) ? 1 : 0];
    }
  else
    {
      tdesc = &amd64_linux_tdescs[(xcr0_features_bit & X86_XSTATE_AVX) ? 1 : 0]
	[(xcr0_features_bit & X86_XSTATE_MPX) ? 1 : 0]
	[(xcr0_features_bit & X86_XSTATE_AVX512) ? 1 : 0]
	[(xcr0_features_bit & X86_XSTATE_PKRU) ? 1 : 0];
    }

  if (*tdesc == NULL)
    *tdesc = amd64_create_target_description (xcr0_features_bit, is_x32,
					      true, true);

  return *tdesc;
}

/* Get Linux/x86 target description from core dump.  */

static const struct target_desc *
amd64_linux_core_read_description (struct gdbarch *gdbarch,
				  struct target_ops *target,
				  bfd *abfd)
{
  /* Linux/x86-64.  */
  x86_xsave_layout layout;
  uint64_t xcr0 = i386_linux_core_read_xsave_info (abfd, layout);
  if (xcr0 == 0)
    xcr0 = X86_XSTATE_SSE_MASK;

  return amd64_linux_read_description (xcr0 & X86_XSTATE_ALL_MASK,
				       gdbarch_ptr_bit (gdbarch) == 32);
}

/* Similar to amd64_supply_fpregset, but use XSAVE extended state.  */

static void
amd64_linux_supply_xstateregset (const struct regset *regset,
				 struct regcache *regcache, int regnum,
				 const void *xstateregs, size_t len)
{
  amd64_supply_xsave (regcache, regnum, xstateregs);
}

/* Similar to amd64_collect_fpregset, but use XSAVE extended state.  */

static void
amd64_linux_collect_xstateregset (const struct regset *regset,
				  const struct regcache *regcache,
				  int regnum, void *xstateregs, size_t len)
{
  amd64_collect_xsave (regcache, regnum, xstateregs, 1);
}

static const struct regset amd64_linux_xstateregset =
  {
    NULL,
    amd64_linux_supply_xstateregset,
    amd64_linux_collect_xstateregset
  };

/* Iterate over core file register note sections.  */

static void
amd64_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					  iterate_over_regset_sections_cb *cb,
					  void *cb_data,
					  const struct regcache *regcache)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  cb (".reg", 27 * 8, 27 * 8, &i386_gregset, NULL, cb_data);
  cb (".reg2", 512, 512, &amd64_fpregset, NULL, cb_data);
  if (tdep->xsave_layout.sizeof_xsave != 0)
    cb (".reg-xstate", tdep->xsave_layout.sizeof_xsave,
	tdep->xsave_layout.sizeof_xsave, &amd64_linux_xstateregset,
	"XSAVE extended state", cb_data);
}

/* The instruction sequences used in x86_64 machines for a
   disabled is-enabled probe.  */

const gdb_byte amd64_dtrace_disabled_probe_sequence_1[] = {
  /* xor %rax, %rax */  0x48, 0x33, 0xc0,
  /* nop            */  0x90,
  /* nop            */  0x90
};

const gdb_byte amd64_dtrace_disabled_probe_sequence_2[] = {
  /* xor %rax, %rax */  0x48, 0x33, 0xc0,
  /* ret            */  0xc3,
  /* nop            */  0x90
};

/* The instruction sequence used in x86_64 machines for enabling a
   DTrace is-enabled probe.  */

const gdb_byte amd64_dtrace_enable_probe_sequence[] = {
  /* mov $0x1, %eax */ 0xb8, 0x01, 0x00, 0x00, 0x00
};

/* The instruction sequence used in x86_64 machines for disabling a
   DTrace is-enabled probe.  */

const gdb_byte amd64_dtrace_disable_probe_sequence[] = {
  /* xor %rax, %rax; nop; nop */ 0x48, 0x33, 0xC0, 0x90, 0x90
};

/* Implementation of `gdbarch_dtrace_probe_is_enabled', as defined in
   gdbarch.h.  */

static int
amd64_dtrace_probe_is_enabled (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  gdb_byte buf[5];

  /* This function returns 1 if the instructions at ADDR do _not_
     follow any of the amd64_dtrace_disabled_probe_sequence_*
     patterns.

     Note that ADDR is offset 3 bytes from the beginning of these
     sequences.  */
  
  read_code (addr - 3, buf, 5);
  return (memcmp (buf, amd64_dtrace_disabled_probe_sequence_1, 5) != 0
	  && memcmp (buf, amd64_dtrace_disabled_probe_sequence_2, 5) != 0);
}

/* Implementation of `gdbarch_dtrace_enable_probe', as defined in
   gdbarch.h.  */

static void
amd64_dtrace_enable_probe (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  /* Note also that ADDR is offset 3 bytes from the beginning of
     amd64_dtrace_enable_probe_sequence.  */

  write_memory (addr - 3, amd64_dtrace_enable_probe_sequence, 5);
}

/* Implementation of `gdbarch_dtrace_disable_probe', as defined in
   gdbarch.h.  */

static void
amd64_dtrace_disable_probe (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  /* Note also that ADDR is offset 3 bytes from the beginning of
     amd64_dtrace_disable_probe_sequence.  */

  write_memory (addr - 3, amd64_dtrace_disable_probe_sequence, 5);
}

/* Implementation of `gdbarch_dtrace_parse_probe_argument', as defined
   in gdbarch.h.  */

static expr::operation_up
amd64_dtrace_parse_probe_argument (struct gdbarch *gdbarch,
				   int narg)
{
  /* DTrace probe arguments can be found on the ABI-defined places for
     regular arguments at the current PC.  The probe abstraction
     currently supports up to 12 arguments for probes.  */

  using namespace expr;

  if (narg < 6)
    {
      static const int arg_reg_map[6] =
	{
	  AMD64_RDI_REGNUM,  /* Arg 1.  */
	  AMD64_RSI_REGNUM,  /* Arg 2.  */
	  AMD64_RDX_REGNUM,  /* Arg 3.  */
	  AMD64_RCX_REGNUM,  /* Arg 4.  */
	  AMD64_R8_REGNUM,   /* Arg 5.  */
	  AMD64_R9_REGNUM    /* Arg 6.  */
	};
      int regno = arg_reg_map[narg];
      const char *regname = user_reg_map_regnum_to_name (gdbarch, regno);
      return make_operation<register_operation> (regname);
    }
  else
    {
      /* Additional arguments are passed on the stack.  */
      const char *regname = user_reg_map_regnum_to_name (gdbarch, AMD64_RSP_REGNUM);

      /* Displacement.  */
      struct type *long_type = builtin_type (gdbarch)->builtin_long;
      operation_up disp	= make_operation<long_const_operation> (long_type,
								narg - 6);

      /* Register: SP.  */
      operation_up reg = make_operation<register_operation> (regname);

      operation_up add = make_operation<add_operation> (std::move (disp),
							std::move (reg));

      /* Cast to long. */
      operation_up cast = make_operation<unop_cast_operation> (std::move (add),
							       long_type);

      return make_operation<unop_ind_operation> (std::move (cast));
    }
}

static void
amd64_linux_init_abi_common(struct gdbarch_info info, struct gdbarch *gdbarch,
			    int num_disp_step_buffers)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  linux_init_abi (info, gdbarch, num_disp_step_buffers);

  tdep->sigtramp_p = amd64_linux_sigtramp_p;
  tdep->sigcontext_addr = amd64_linux_sigcontext_addr;
  tdep->sc_reg_offset = amd64_linux_sc_reg_offset;
  tdep->sc_num_regs = ARRAY_SIZE (amd64_linux_sc_reg_offset);

  tdep->xsave_xcr0_offset = I386_LINUX_XSAVE_XCR0_OFFSET;
  set_gdbarch_core_read_x86_xsave_layout
    (gdbarch, i386_linux_core_read_x86_xsave_layout);

  /* Add the %orig_rax register used for syscall restarting.  */
  set_gdbarch_write_pc (gdbarch, amd64_linux_write_pc);

  tdep->register_reggroup_p = amd64_linux_register_reggroup_p;

  /* Functions for 'catch syscall'.  */
  set_xml_syscall_file_name (gdbarch, XML_SYSCALL_FILENAME_AMD64);
  set_gdbarch_get_syscall_number (gdbarch,
				  amd64_linux_get_syscall_number);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  /* GNU/Linux uses the dynamic linker included in the GNU C Library.  */
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  /* Iterate over core file register note sections.  */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, amd64_linux_iterate_over_regset_sections);

  set_gdbarch_core_read_description (gdbarch,
				     amd64_linux_core_read_description);

  /* Displaced stepping.  */
  set_gdbarch_displaced_step_copy_insn (gdbarch,
					amd64_displaced_step_copy_insn);
  set_gdbarch_displaced_step_fixup (gdbarch, amd64_displaced_step_fixup);

  set_gdbarch_process_record (gdbarch, i386_process_record);
  set_gdbarch_process_record_signal (gdbarch, amd64_linux_record_signal);

  set_gdbarch_get_siginfo_type (gdbarch, x86_linux_get_siginfo_type);
  set_gdbarch_report_signal_info (gdbarch, i386_linux_report_signal_info);
}

static void
amd64_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  struct tdesc_arch_data *tdesc_data = info.tdesc_data;
  const struct tdesc_feature *feature;
  int valid_p;

  gdb_assert (tdesc_data);

  tdep->gregset_reg_offset = amd64_linux_gregset_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (amd64_linux_gregset_reg_offset);
  tdep->sizeof_gregset = 27 * 8;

  amd64_init_abi (info, gdbarch,
		  amd64_linux_read_description (X86_XSTATE_SSE_MASK, false));

  const target_desc *tdesc = tdep->tdesc;

  /* Reserve a number for orig_rax.  */
  set_gdbarch_num_regs (gdbarch, AMD64_LINUX_NUM_REGS);

  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.i386.linux");
  if (feature == NULL)
    return;

  valid_p = tdesc_numbered_register (feature, tdesc_data,
				     AMD64_LINUX_ORIG_RAX_REGNUM,
				     "orig_rax");
  if (!valid_p)
    return;

  amd64_linux_init_abi_common (info, gdbarch, 2);

  /* Initialize the amd64_linux_record_tdep.  */
  /* These values are the size of the type that will be used in a system
     call.  They are obtained from Linux Kernel source.  */
  amd64_linux_record_tdep.size_pointer
    = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
  amd64_linux_record_tdep.size__old_kernel_stat = 32;
  amd64_linux_record_tdep.size_tms = 32;
  amd64_linux_record_tdep.size_loff_t = 8;
  amd64_linux_record_tdep.size_flock = 32;
  amd64_linux_record_tdep.size_oldold_utsname = 45;
  amd64_linux_record_tdep.size_ustat = 32;
  /* ADM64 doesn't need this size because it doesn't have sys_sigaction
     but sys_rt_sigaction.  */
  amd64_linux_record_tdep.size_old_sigaction = 32;
  /* ADM64 doesn't need this size because it doesn't have sys_sigpending
     but sys_rt_sigpending.  */
  amd64_linux_record_tdep.size_old_sigset_t = 8;
  amd64_linux_record_tdep.size_rlimit = 16;
  amd64_linux_record_tdep.size_rusage = 144;
  amd64_linux_record_tdep.size_timeval = 16;
  amd64_linux_record_tdep.size_timezone = 8;
  /* ADM64 doesn't need this size because it doesn't have sys_getgroups16
     but sys_getgroups.  */
  amd64_linux_record_tdep.size_old_gid_t = 2;
  /* ADM64 doesn't need this size because it doesn't have sys_getresuid16
     but sys_getresuid.  */
  amd64_linux_record_tdep.size_old_uid_t = 2;
  amd64_linux_record_tdep.size_fd_set = 128;
  /* ADM64 doesn't need this size because it doesn't have sys_readdir. */
  amd64_linux_record_tdep.size_old_dirent = 280;
  amd64_linux_record_tdep.size_statfs = 120;
  amd64_linux_record_tdep.size_statfs64 = 120;
  amd64_linux_record_tdep.size_sockaddr = 16;
  amd64_linux_record_tdep.size_int
    = gdbarch_int_bit (gdbarch) / TARGET_CHAR_BIT;
  amd64_linux_record_tdep.size_long
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  amd64_linux_record_tdep.size_ulong
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  amd64_linux_record_tdep.size_msghdr = 56;
  amd64_linux_record_tdep.size_itimerval = 32;
  amd64_linux_record_tdep.size_stat = 144;
  amd64_linux_record_tdep.size_old_utsname = 325;
  amd64_linux_record_tdep.size_sysinfo = 112;
  amd64_linux_record_tdep.size_msqid_ds = 120;
  amd64_linux_record_tdep.size_shmid_ds = 112;
  amd64_linux_record_tdep.size_new_utsname = 390;
  amd64_linux_record_tdep.size_timex = 208;
  amd64_linux_record_tdep.size_mem_dqinfo = 24;
  amd64_linux_record_tdep.size_if_dqblk = 72;
  amd64_linux_record_tdep.size_fs_quota_stat = 80;
  amd64_linux_record_tdep.size_timespec = 16;
  amd64_linux_record_tdep.size_pollfd = 8;
  amd64_linux_record_tdep.size_NFS_FHSIZE = 32;
  amd64_linux_record_tdep.size_knfsd_fh = 132;
  amd64_linux_record_tdep.size_TASK_COMM_LEN = 16;
  amd64_linux_record_tdep.size_sigaction = 32;
  amd64_linux_record_tdep.size_sigset_t = 8;
  amd64_linux_record_tdep.size_siginfo_t = 128;
  amd64_linux_record_tdep.size_cap_user_data_t = 8;
  amd64_linux_record_tdep.size_stack_t = 24;
  amd64_linux_record_tdep.size_off_t = 8;
  amd64_linux_record_tdep.size_stat64 = 144;
  amd64_linux_record_tdep.size_gid_t = 4;
  amd64_linux_record_tdep.size_uid_t = 4;
  amd64_linux_record_tdep.size_PAGE_SIZE = 4096;
  amd64_linux_record_tdep.size_flock64 = 32;
  amd64_linux_record_tdep.size_user_desc = 16;
  amd64_linux_record_tdep.size_io_event = 32;
  amd64_linux_record_tdep.size_iocb = 64;
  amd64_linux_record_tdep.size_epoll_event = 12;
  amd64_linux_record_tdep.size_itimerspec = 32;
  amd64_linux_record_tdep.size_mq_attr = 64;
  amd64_linux_record_tdep.size_termios = 36;
  amd64_linux_record_tdep.size_termios2 = 44;
  amd64_linux_record_tdep.size_pid_t = 4;
  amd64_linux_record_tdep.size_winsize = 8;
  amd64_linux_record_tdep.size_serial_struct = 72;
  amd64_linux_record_tdep.size_serial_icounter_struct = 80;
  amd64_linux_record_tdep.size_hayes_esp_config = 12;
  amd64_linux_record_tdep.size_size_t = 8;
  amd64_linux_record_tdep.size_iovec = 16;
  amd64_linux_record_tdep.size_time_t = 8;

  /* These values are the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  They are obtained from Linux Kernel source.  */
  amd64_linux_record_tdep.fcntl_F_GETLK = 5;
  amd64_linux_record_tdep.fcntl_F_GETLK64 = 12;
  amd64_linux_record_tdep.fcntl_F_SETLK64 = 13;
  amd64_linux_record_tdep.fcntl_F_SETLKW64 = 14;

  amd64_linux_record_tdep.arg1 = AMD64_RDI_REGNUM;
  amd64_linux_record_tdep.arg2 = AMD64_RSI_REGNUM;
  amd64_linux_record_tdep.arg3 = AMD64_RDX_REGNUM;
  amd64_linux_record_tdep.arg4 = AMD64_R10_REGNUM;
  amd64_linux_record_tdep.arg5 = AMD64_R8_REGNUM;
  amd64_linux_record_tdep.arg6 = AMD64_R9_REGNUM;

  /* These values are the second argument of system call "sys_ioctl".
     They are obtained from Linux Kernel source.  */
  amd64_linux_record_tdep.ioctl_TCGETS = 0x5401;
  amd64_linux_record_tdep.ioctl_TCSETS = 0x5402;
  amd64_linux_record_tdep.ioctl_TCSETSW = 0x5403;
  amd64_linux_record_tdep.ioctl_TCSETSF = 0x5404;
  amd64_linux_record_tdep.ioctl_TCGETA = 0x5405;
  amd64_linux_record_tdep.ioctl_TCSETA = 0x5406;
  amd64_linux_record_tdep.ioctl_TCSETAW = 0x5407;
  amd64_linux_record_tdep.ioctl_TCSETAF = 0x5408;
  amd64_linux_record_tdep.ioctl_TCSBRK = 0x5409;
  amd64_linux_record_tdep.ioctl_TCXONC = 0x540A;
  amd64_linux_record_tdep.ioctl_TCFLSH = 0x540B;
  amd64_linux_record_tdep.ioctl_TIOCEXCL = 0x540C;
  amd64_linux_record_tdep.ioctl_TIOCNXCL = 0x540D;
  amd64_linux_record_tdep.ioctl_TIOCSCTTY = 0x540E;
  amd64_linux_record_tdep.ioctl_TIOCGPGRP = 0x540F;
  amd64_linux_record_tdep.ioctl_TIOCSPGRP = 0x5410;
  amd64_linux_record_tdep.ioctl_TIOCOUTQ = 0x5411;
  amd64_linux_record_tdep.ioctl_TIOCSTI = 0x5412;
  amd64_linux_record_tdep.ioctl_TIOCGWINSZ = 0x5413;
  amd64_linux_record_tdep.ioctl_TIOCSWINSZ = 0x5414;
  amd64_linux_record_tdep.ioctl_TIOCMGET = 0x5415;
  amd64_linux_record_tdep.ioctl_TIOCMBIS = 0x5416;
  amd64_linux_record_tdep.ioctl_TIOCMBIC = 0x5417;
  amd64_linux_record_tdep.ioctl_TIOCMSET = 0x5418;
  amd64_linux_record_tdep.ioctl_TIOCGSOFTCAR = 0x5419;
  amd64_linux_record_tdep.ioctl_TIOCSSOFTCAR = 0x541A;
  amd64_linux_record_tdep.ioctl_FIONREAD = 0x541B;
  amd64_linux_record_tdep.ioctl_TIOCINQ
    = amd64_linux_record_tdep.ioctl_FIONREAD;
  amd64_linux_record_tdep.ioctl_TIOCLINUX = 0x541C;
  amd64_linux_record_tdep.ioctl_TIOCCONS = 0x541D;
  amd64_linux_record_tdep.ioctl_TIOCGSERIAL = 0x541E;
  amd64_linux_record_tdep.ioctl_TIOCSSERIAL = 0x541F;
  amd64_linux_record_tdep.ioctl_TIOCPKT = 0x5420;
  amd64_linux_record_tdep.ioctl_FIONBIO = 0x5421;
  amd64_linux_record_tdep.ioctl_TIOCNOTTY = 0x5422;
  amd64_linux_record_tdep.ioctl_TIOCSETD = 0x5423;
  amd64_linux_record_tdep.ioctl_TIOCGETD = 0x5424;
  amd64_linux_record_tdep.ioctl_TCSBRKP = 0x5425;
  amd64_linux_record_tdep.ioctl_TIOCTTYGSTRUCT = 0x5426;
  amd64_linux_record_tdep.ioctl_TIOCSBRK = 0x5427;
  amd64_linux_record_tdep.ioctl_TIOCCBRK = 0x5428;
  amd64_linux_record_tdep.ioctl_TIOCGSID = 0x5429;
  amd64_linux_record_tdep.ioctl_TCGETS2 = 0x802c542a;
  amd64_linux_record_tdep.ioctl_TCSETS2 = 0x402c542b;
  amd64_linux_record_tdep.ioctl_TCSETSW2 = 0x402c542c;
  amd64_linux_record_tdep.ioctl_TCSETSF2 = 0x402c542d;
  amd64_linux_record_tdep.ioctl_TIOCGPTN = 0x80045430;
  amd64_linux_record_tdep.ioctl_TIOCSPTLCK = 0x40045431;
  amd64_linux_record_tdep.ioctl_FIONCLEX = 0x5450;
  amd64_linux_record_tdep.ioctl_FIOCLEX = 0x5451;
  amd64_linux_record_tdep.ioctl_FIOASYNC = 0x5452;
  amd64_linux_record_tdep.ioctl_TIOCSERCONFIG = 0x5453;
  amd64_linux_record_tdep.ioctl_TIOCSERGWILD = 0x5454;
  amd64_linux_record_tdep.ioctl_TIOCSERSWILD = 0x5455;
  amd64_linux_record_tdep.ioctl_TIOCGLCKTRMIOS = 0x5456;
  amd64_linux_record_tdep.ioctl_TIOCSLCKTRMIOS = 0x5457;
  amd64_linux_record_tdep.ioctl_TIOCSERGSTRUCT = 0x5458;
  amd64_linux_record_tdep.ioctl_TIOCSERGETLSR = 0x5459;
  amd64_linux_record_tdep.ioctl_TIOCSERGETMULTI = 0x545A;
  amd64_linux_record_tdep.ioctl_TIOCSERSETMULTI = 0x545B;
  amd64_linux_record_tdep.ioctl_TIOCMIWAIT = 0x545C;
  amd64_linux_record_tdep.ioctl_TIOCGICOUNT = 0x545D;
  amd64_linux_record_tdep.ioctl_TIOCGHAYESESP = 0x545E;
  amd64_linux_record_tdep.ioctl_TIOCSHAYESESP = 0x545F;
  amd64_linux_record_tdep.ioctl_FIOQSIZE = 0x5460;

  tdep->i386_syscall_record = amd64_linux_syscall_record;

  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, linux_lp64_fetch_link_map_offsets);

  /* Register DTrace handlers.  */
  set_gdbarch_dtrace_parse_probe_argument (gdbarch, amd64_dtrace_parse_probe_argument);
  set_gdbarch_dtrace_probe_is_enabled (gdbarch, amd64_dtrace_probe_is_enabled);
  set_gdbarch_dtrace_enable_probe (gdbarch, amd64_dtrace_enable_probe);
  set_gdbarch_dtrace_disable_probe (gdbarch, amd64_dtrace_disable_probe);
}

static void
amd64_x32_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  struct tdesc_arch_data *tdesc_data = info.tdesc_data;
  const struct tdesc_feature *feature;
  int valid_p;

  gdb_assert (tdesc_data);

  tdep->gregset_reg_offset = amd64_linux_gregset_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (amd64_linux_gregset_reg_offset);
  tdep->sizeof_gregset = 27 * 8;

  amd64_x32_init_abi (info, gdbarch,
		      amd64_linux_read_description (X86_XSTATE_SSE_MASK,
						    true));

  /* Reserve a number for orig_rax.  */
  set_gdbarch_num_regs (gdbarch, AMD64_LINUX_NUM_REGS);

  const target_desc *tdesc = tdep->tdesc;

  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.i386.linux");
  if (feature == NULL)
    return;

  valid_p = tdesc_numbered_register (feature, tdesc_data,
				     AMD64_LINUX_ORIG_RAX_REGNUM,
				     "orig_rax");
  if (!valid_p)
    return;

  amd64_linux_init_abi_common (info, gdbarch, 0);

  /* Initialize the amd64_x32_linux_record_tdep.  */
  /* These values are the size of the type that will be used in a system
     call.  They are obtained from Linux Kernel source.  */
  amd64_x32_linux_record_tdep.size_pointer
    = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
  amd64_x32_linux_record_tdep.size__old_kernel_stat = 32;
  amd64_x32_linux_record_tdep.size_tms = 32;
  amd64_x32_linux_record_tdep.size_loff_t = 8;
  amd64_x32_linux_record_tdep.size_flock = 32;
  amd64_x32_linux_record_tdep.size_oldold_utsname = 45;
  amd64_x32_linux_record_tdep.size_ustat = 32;
  /* ADM64 doesn't need this size because it doesn't have sys_sigaction
     but sys_rt_sigaction.  */
  amd64_x32_linux_record_tdep.size_old_sigaction = 16;
  /* ADM64 doesn't need this size because it doesn't have sys_sigpending
     but sys_rt_sigpending.  */
  amd64_x32_linux_record_tdep.size_old_sigset_t = 4;
  amd64_x32_linux_record_tdep.size_rlimit = 16;
  amd64_x32_linux_record_tdep.size_rusage = 144;
  amd64_x32_linux_record_tdep.size_timeval = 16;
  amd64_x32_linux_record_tdep.size_timezone = 8;
  /* ADM64 doesn't need this size because it doesn't have sys_getgroups16
     but sys_getgroups.  */
  amd64_x32_linux_record_tdep.size_old_gid_t = 2;
  /* ADM64 doesn't need this size because it doesn't have sys_getresuid16
     but sys_getresuid.  */
  amd64_x32_linux_record_tdep.size_old_uid_t = 2;
  amd64_x32_linux_record_tdep.size_fd_set = 128;
  /* ADM64 doesn't need this size because it doesn't have sys_readdir. */
  amd64_x32_linux_record_tdep.size_old_dirent = 268;
  amd64_x32_linux_record_tdep.size_statfs = 120;
  amd64_x32_linux_record_tdep.size_statfs64 = 120;
  amd64_x32_linux_record_tdep.size_sockaddr = 16;
  amd64_x32_linux_record_tdep.size_int
    = gdbarch_int_bit (gdbarch) / TARGET_CHAR_BIT;
  amd64_x32_linux_record_tdep.size_long
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  amd64_x32_linux_record_tdep.size_ulong
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  amd64_x32_linux_record_tdep.size_msghdr = 28;
  amd64_x32_linux_record_tdep.size_itimerval = 32;
  amd64_x32_linux_record_tdep.size_stat = 144;
  amd64_x32_linux_record_tdep.size_old_utsname = 325;
  amd64_x32_linux_record_tdep.size_sysinfo = 112;
  amd64_x32_linux_record_tdep.size_msqid_ds = 120;
  amd64_x32_linux_record_tdep.size_shmid_ds = 112;
  amd64_x32_linux_record_tdep.size_new_utsname = 390;
  amd64_x32_linux_record_tdep.size_timex = 208;
  amd64_x32_linux_record_tdep.size_mem_dqinfo = 24;
  amd64_x32_linux_record_tdep.size_if_dqblk = 72;
  amd64_x32_linux_record_tdep.size_fs_quota_stat = 80;
  amd64_x32_linux_record_tdep.size_timespec = 16;
  amd64_x32_linux_record_tdep.size_pollfd = 8;
  amd64_x32_linux_record_tdep.size_NFS_FHSIZE = 32;
  amd64_x32_linux_record_tdep.size_knfsd_fh = 132;
  amd64_x32_linux_record_tdep.size_TASK_COMM_LEN = 16;
  amd64_x32_linux_record_tdep.size_sigaction = 20;
  amd64_x32_linux_record_tdep.size_sigset_t = 8;
  amd64_x32_linux_record_tdep.size_siginfo_t = 128;
  amd64_x32_linux_record_tdep.size_cap_user_data_t = 8;
  amd64_x32_linux_record_tdep.size_stack_t = 12;
  amd64_x32_linux_record_tdep.size_off_t = 8;
  amd64_x32_linux_record_tdep.size_stat64 = 144;
  amd64_x32_linux_record_tdep.size_gid_t = 4;
  amd64_x32_linux_record_tdep.size_uid_t = 4;
  amd64_x32_linux_record_tdep.size_PAGE_SIZE = 4096;
  amd64_x32_linux_record_tdep.size_flock64 = 32;
  amd64_x32_linux_record_tdep.size_user_desc = 16;
  amd64_x32_linux_record_tdep.size_io_event = 32;
  amd64_x32_linux_record_tdep.size_iocb = 64;
  amd64_x32_linux_record_tdep.size_epoll_event = 12;
  amd64_x32_linux_record_tdep.size_itimerspec = 32;
  amd64_x32_linux_record_tdep.size_mq_attr = 64;
  amd64_x32_linux_record_tdep.size_termios = 36;
  amd64_x32_linux_record_tdep.size_termios2 = 44;
  amd64_x32_linux_record_tdep.size_pid_t = 4;
  amd64_x32_linux_record_tdep.size_winsize = 8;
  amd64_x32_linux_record_tdep.size_serial_struct = 72;
  amd64_x32_linux_record_tdep.size_serial_icounter_struct = 80;
  amd64_x32_linux_record_tdep.size_hayes_esp_config = 12;
  amd64_x32_linux_record_tdep.size_size_t = 4;
  amd64_x32_linux_record_tdep.size_iovec = 8;
  amd64_x32_linux_record_tdep.size_time_t = 8;

  /* These values are the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  They are obtained from Linux Kernel source.  */
  amd64_x32_linux_record_tdep.fcntl_F_GETLK = 5;
  amd64_x32_linux_record_tdep.fcntl_F_GETLK64 = 12;
  amd64_x32_linux_record_tdep.fcntl_F_SETLK64 = 13;
  amd64_x32_linux_record_tdep.fcntl_F_SETLKW64 = 14;

  amd64_x32_linux_record_tdep.arg1 = AMD64_RDI_REGNUM;
  amd64_x32_linux_record_tdep.arg2 = AMD64_RSI_REGNUM;
  amd64_x32_linux_record_tdep.arg3 = AMD64_RDX_REGNUM;
  amd64_x32_linux_record_tdep.arg4 = AMD64_R10_REGNUM;
  amd64_x32_linux_record_tdep.arg5 = AMD64_R8_REGNUM;
  amd64_x32_linux_record_tdep.arg6 = AMD64_R9_REGNUM;

  /* These values are the second argument of system call "sys_ioctl".
     They are obtained from Linux Kernel source.  */
  amd64_x32_linux_record_tdep.ioctl_TCGETS = 0x5401;
  amd64_x32_linux_record_tdep.ioctl_TCSETS = 0x5402;
  amd64_x32_linux_record_tdep.ioctl_TCSETSW = 0x5403;
  amd64_x32_linux_record_tdep.ioctl_TCSETSF = 0x5404;
  amd64_x32_linux_record_tdep.ioctl_TCGETA = 0x5405;
  amd64_x32_linux_record_tdep.ioctl_TCSETA = 0x5406;
  amd64_x32_linux_record_tdep.ioctl_TCSETAW = 0x5407;
  amd64_x32_linux_record_tdep.ioctl_TCSETAF = 0x5408;
  amd64_x32_linux_record_tdep.ioctl_TCSBRK = 0x5409;
  amd64_x32_linux_record_tdep.ioctl_TCXONC = 0x540A;
  amd64_x32_linux_record_tdep.ioctl_TCFLSH = 0x540B;
  amd64_x32_linux_record_tdep.ioctl_TIOCEXCL = 0x540C;
  amd64_x32_linux_record_tdep.ioctl_TIOCNXCL = 0x540D;
  amd64_x32_linux_record_tdep.ioctl_TIOCSCTTY = 0x540E;
  amd64_x32_linux_record_tdep.ioctl_TIOCGPGRP = 0x540F;
  amd64_x32_linux_record_tdep.ioctl_TIOCSPGRP = 0x5410;
  amd64_x32_linux_record_tdep.ioctl_TIOCOUTQ = 0x5411;
  amd64_x32_linux_record_tdep.ioctl_TIOCSTI = 0x5412;
  amd64_x32_linux_record_tdep.ioctl_TIOCGWINSZ = 0x5413;
  amd64_x32_linux_record_tdep.ioctl_TIOCSWINSZ = 0x5414;
  amd64_x32_linux_record_tdep.ioctl_TIOCMGET = 0x5415;
  amd64_x32_linux_record_tdep.ioctl_TIOCMBIS = 0x5416;
  amd64_x32_linux_record_tdep.ioctl_TIOCMBIC = 0x5417;
  amd64_x32_linux_record_tdep.ioctl_TIOCMSET = 0x5418;
  amd64_x32_linux_record_tdep.ioctl_TIOCGSOFTCAR = 0x5419;
  amd64_x32_linux_record_tdep.ioctl_TIOCSSOFTCAR = 0x541A;
  amd64_x32_linux_record_tdep.ioctl_FIONREAD = 0x541B;
  amd64_x32_linux_record_tdep.ioctl_TIOCINQ = amd64_x32_linux_record_tdep.ioctl_FIONREAD;
  amd64_x32_linux_record_tdep.ioctl_TIOCLINUX = 0x541C;
  amd64_x32_linux_record_tdep.ioctl_TIOCCONS = 0x541D;
  amd64_x32_linux_record_tdep.ioctl_TIOCGSERIAL = 0x541E;
  amd64_x32_linux_record_tdep.ioctl_TIOCSSERIAL = 0x541F;
  amd64_x32_linux_record_tdep.ioctl_TIOCPKT = 0x5420;
  amd64_x32_linux_record_tdep.ioctl_FIONBIO = 0x5421;
  amd64_x32_linux_record_tdep.ioctl_TIOCNOTTY = 0x5422;
  amd64_x32_linux_record_tdep.ioctl_TIOCSETD = 0x5423;
  amd64_x32_linux_record_tdep.ioctl_TIOCGETD = 0x5424;
  amd64_x32_linux_record_tdep.ioctl_TCSBRKP = 0x5425;
  amd64_x32_linux_record_tdep.ioctl_TIOCTTYGSTRUCT = 0x5426;
  amd64_x32_linux_record_tdep.ioctl_TIOCSBRK = 0x5427;
  amd64_x32_linux_record_tdep.ioctl_TIOCCBRK = 0x5428;
  amd64_x32_linux_record_tdep.ioctl_TIOCGSID = 0x5429;
  amd64_x32_linux_record_tdep.ioctl_TCGETS2 = 0x802c542a;
  amd64_x32_linux_record_tdep.ioctl_TCSETS2 = 0x402c542b;
  amd64_x32_linux_record_tdep.ioctl_TCSETSW2 = 0x402c542c;
  amd64_x32_linux_record_tdep.ioctl_TCSETSF2 = 0x402c542d;
  amd64_x32_linux_record_tdep.ioctl_TIOCGPTN = 0x80045430;
  amd64_x32_linux_record_tdep.ioctl_TIOCSPTLCK = 0x40045431;
  amd64_x32_linux_record_tdep.ioctl_FIONCLEX = 0x5450;
  amd64_x32_linux_record_tdep.ioctl_FIOCLEX = 0x5451;
  amd64_x32_linux_record_tdep.ioctl_FIOASYNC = 0x5452;
  amd64_x32_linux_record_tdep.ioctl_TIOCSERCONFIG = 0x5453;
  amd64_x32_linux_record_tdep.ioctl_TIOCSERGWILD = 0x5454;
  amd64_x32_linux_record_tdep.ioctl_TIOCSERSWILD = 0x5455;
  amd64_x32_linux_record_tdep.ioctl_TIOCGLCKTRMIOS = 0x5456;
  amd64_x32_linux_record_tdep.ioctl_TIOCSLCKTRMIOS = 0x5457;
  amd64_x32_linux_record_tdep.ioctl_TIOCSERGSTRUCT = 0x5458;
  amd64_x32_linux_record_tdep.ioctl_TIOCSERGETLSR = 0x5459;
  amd64_x32_linux_record_tdep.ioctl_TIOCSERGETMULTI = 0x545A;
  amd64_x32_linux_record_tdep.ioctl_TIOCSERSETMULTI = 0x545B;
  amd64_x32_linux_record_tdep.ioctl_TIOCMIWAIT = 0x545C;
  amd64_x32_linux_record_tdep.ioctl_TIOCGICOUNT = 0x545D;
  amd64_x32_linux_record_tdep.ioctl_TIOCGHAYESESP = 0x545E;
  amd64_x32_linux_record_tdep.ioctl_TIOCSHAYESESP = 0x545F;
  amd64_x32_linux_record_tdep.ioctl_FIOQSIZE = 0x5460;

  tdep->i386_syscall_record = amd64_x32_linux_syscall_record;

  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, linux_ilp32_fetch_link_map_offsets);
}

void _initialize_amd64_linux_tdep ();
void
_initialize_amd64_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_i386, bfd_mach_x86_64,
			  GDB_OSABI_LINUX, amd64_linux_init_abi);
  gdbarch_register_osabi (bfd_arch_i386, bfd_mach_x64_32,
			  GDB_OSABI_LINUX, amd64_x32_linux_init_abi);
}
