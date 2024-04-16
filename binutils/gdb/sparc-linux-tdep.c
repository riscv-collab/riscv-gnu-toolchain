/* Target-dependent code for GNU/Linux SPARC.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "dwarf2/frame.h"
#include "frame.h"
#include "frame-unwind.h"
#include "gdbtypes.h"
#include "regset.h"
#include "gdbarch.h"
#include "gdbcore.h"
#include "osabi.h"
#include "regcache.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "xml-syscall.h"
#include "linux-tdep.h"

/* The syscall's XML filename for sparc 32-bit.  */
#define XML_SYSCALL_FILENAME_SPARC32 "syscalls/sparc-linux.xml"

#include "sparc-tdep.h"

/* Signal trampoline support.  */

static void sparc32_linux_sigframe_init (const struct tramp_frame *self,
					 frame_info_ptr this_frame,
					 struct trad_frame_cache *this_cache,
					 CORE_ADDR func);

/* GNU/Linux has two flavors of signals.  Normal signal handlers, and
   "realtime" (RT) signals.  The RT signals can provide additional
   information to the signal handler if the SA_SIGINFO flag is set
   when establishing a signal handler using `sigaction'.  It is not
   unlikely that future versions of GNU/Linux will support SA_SIGINFO
   for normal signals too.  */

/* When the sparc Linux kernel calls a signal handler and the
   SA_RESTORER flag isn't set, the return address points to a bit of
   code on the stack.  This code checks whether the PC appears to be
   within this bit of code.

   The instruction sequence for normal signals is encoded below.
   Checking for the code sequence should be somewhat reliable, because
   the effect is to call the system call sigreturn.  This is unlikely
   to occur anywhere other than a signal trampoline.  */

static const struct tramp_frame sparc32_linux_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0x821020d8, ULONGEST_MAX },		/* mov __NR_sigreturn, %g1 */
    { 0x91d02010, ULONGEST_MAX },		/* ta  0x10 */
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  sparc32_linux_sigframe_init
};

/* The instruction sequence for RT signals is slightly different.  The
   effect is to call the system call rt_sigreturn.  */

static const struct tramp_frame sparc32_linux_rt_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0x82102065, ULONGEST_MAX },		/* mov __NR_rt_sigreturn, %g1 */
    { 0x91d02010, ULONGEST_MAX },		/* ta  0x10 */
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  sparc32_linux_sigframe_init
};

/* This enum represents the signals' numbers on the SPARC
   architecture.  It just contains the signal definitions which are
   different from the generic implementation.

   It is derived from the file <arch/sparc/include/uapi/asm/signal.h>,
   from the Linux kernel tree.  */

enum
  {
    SPARC_LINUX_SIGEMT = 7,
    SPARC_LINUX_SIGBUS = 10,
    SPARC_LINUX_SIGSYS = 12,
    SPARC_LINUX_SIGURG = 16,
    SPARC_LINUX_SIGSTOP = 17,
    SPARC_LINUX_SIGTSTP = 18,
    SPARC_LINUX_SIGCONT = 19,
    SPARC_LINUX_SIGCHLD = 20,
    SPARC_LINUX_SIGIO = 23,
    SPARC_LINUX_SIGPOLL = SPARC_LINUX_SIGIO,
    SPARC_LINUX_SIGLOST = 29,
    SPARC_LINUX_SIGPWR = SPARC_LINUX_SIGLOST,
    SPARC_LINUX_SIGUSR1 = 30,
    SPARC_LINUX_SIGUSR2 = 31,
  };

static void
sparc32_linux_sigframe_init (const struct tramp_frame *self,
			     frame_info_ptr this_frame,
			     struct trad_frame_cache *this_cache,
			     CORE_ADDR func)
{
  CORE_ADDR base, addr, sp_addr;
  int regnum;

  base = get_frame_register_unsigned (this_frame, SPARC_O1_REGNUM);
  if (self == &sparc32_linux_rt_sigframe)
    base += 128;

  /* Offsets from <bits/sigcontext.h>.  */

  trad_frame_set_reg_addr (this_cache, SPARC32_PSR_REGNUM, base + 0);
  trad_frame_set_reg_addr (this_cache, SPARC32_PC_REGNUM, base + 4);
  trad_frame_set_reg_addr (this_cache, SPARC32_NPC_REGNUM, base + 8);
  trad_frame_set_reg_addr (this_cache, SPARC32_Y_REGNUM, base + 12);

  /* Since %g0 is always zero, keep the identity encoding.  */
  addr = base + 20;
  sp_addr = base + 16 + ((SPARC_SP_REGNUM - SPARC_G0_REGNUM) * 4);
  for (regnum = SPARC_G1_REGNUM; regnum <= SPARC_O7_REGNUM; regnum++)
    {
      trad_frame_set_reg_addr (this_cache, regnum, addr);
      addr += 4;
    }

  base = get_frame_register_unsigned (this_frame, SPARC_SP_REGNUM);
  addr = get_frame_memory_unsigned (this_frame, sp_addr, 4);

  for (regnum = SPARC_L0_REGNUM; regnum <= SPARC_I7_REGNUM; regnum++)
    {
      trad_frame_set_reg_addr (this_cache, regnum, addr);
      addr += 4;
    }
  trad_frame_set_id (this_cache, frame_id_build (base, func));
}

/* Return the address of a system call's alternative return
   address.  */

static CORE_ADDR
sparc32_linux_step_trap (frame_info_ptr frame, unsigned long insn)
{
  if (insn == 0x91d02010)
    {
      ULONGEST sc_num = get_frame_register_unsigned (frame, SPARC_G1_REGNUM);

      /* __NR_rt_sigreturn is 101 and __NR_sigreturn is 216.  */
      if (sc_num == 101 || sc_num == 216)
	{
	  struct gdbarch *gdbarch = get_frame_arch (frame);
	  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

	  ULONGEST sp, pc_offset;

	  sp = get_frame_register_unsigned (frame, SPARC_SP_REGNUM);

	  /* The kernel puts the sigreturn registers on the stack,
	     and this is where the signal unwinding state is take from
	     when returning from a signal.

	     For __NR_sigreturn, this register area sits 96 bytes from
	     the base of the stack.  The saved PC sits 4 bytes into the
	     sigreturn register save area.

	     For __NR_rt_sigreturn a siginfo_t, which is 128 bytes, sits
	     right before the sigreturn register save area.  */

	  pc_offset = 96 + 4;
	  if (sc_num == 101)
	    pc_offset += 128;

	  return read_memory_unsigned_integer (sp + pc_offset, 4, byte_order);
	}
    }

  return 0;
}


const struct sparc_gregmap sparc32_linux_core_gregmap =
{
  32 * 4,			/* %psr */
  33 * 4,			/* %pc */
  34 * 4,			/* %npc */
  35 * 4,			/* %y */
  -1,				/* %wim */
  -1,				/* %tbr */
  1 * 4,			/* %g1 */
  16 * 4,			/* %l0 */
  4,				/* y size */
};


static void
sparc32_linux_supply_core_gregset (const struct regset *regset,
				   struct regcache *regcache,
				   int regnum, const void *gregs, size_t len)
{
  sparc32_supply_gregset (&sparc32_linux_core_gregmap,
			  regcache, regnum, gregs);
}

static void
sparc32_linux_collect_core_gregset (const struct regset *regset,
				    const struct regcache *regcache,
				    int regnum, void *gregs, size_t len)
{
  sparc32_collect_gregset (&sparc32_linux_core_gregmap,
			   regcache, regnum, gregs);
}

static void
sparc32_linux_supply_core_fpregset (const struct regset *regset,
				    struct regcache *regcache,
				    int regnum, const void *fpregs, size_t len)
{
  sparc32_supply_fpregset (&sparc32_bsd_fpregmap, regcache, regnum, fpregs);
}

static void
sparc32_linux_collect_core_fpregset (const struct regset *regset,
				     const struct regcache *regcache,
				     int regnum, void *fpregs, size_t len)
{
  sparc32_collect_fpregset (&sparc32_bsd_fpregmap, regcache, regnum, fpregs);
}

/* Set the program counter for process PTID to PC.  */

#define PSR_SYSCALL	0x00004000

static void
sparc_linux_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  gdbarch *arch = regcache->arch ();
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (arch);
  ULONGEST psr;

  regcache_cooked_write_unsigned (regcache, tdep->pc_regnum, pc);
  regcache_cooked_write_unsigned (regcache, tdep->npc_regnum, pc + 4);

  /* Clear the "in syscall" bit to prevent the kernel from
     messing with the PCs we just installed, if we happen to be
     within an interrupted system call that the kernel wants to
     restart.

     Note that after we return from the dummy call, the PSR et al.
     registers will be automatically restored, and the kernel
     continues to restart the system call at this point.  */
  regcache_cooked_read_unsigned (regcache, SPARC32_PSR_REGNUM, &psr);
  psr &= ~PSR_SYSCALL;
  regcache_cooked_write_unsigned (regcache, SPARC32_PSR_REGNUM, psr);
}

static LONGEST
sparc32_linux_get_syscall_number (struct gdbarch *gdbarch,
				  thread_info *thread)
{
  struct regcache *regcache = get_thread_regcache (thread);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  /* The content of a register.  */
  gdb_byte buf[4];
  /* The result.  */
  LONGEST ret;

  /* Getting the system call number from the register.
     When dealing with the sparc architecture, this information
     is stored at the %g1 register.  */
  regcache->cooked_read (SPARC_G1_REGNUM, buf);

  ret = extract_signed_integer (buf, 4, byte_order);

  return ret;
}

/* Implementation of `gdbarch_gdb_signal_from_target', as defined in
   gdbarch.h.  */

static enum gdb_signal
sparc32_linux_gdb_signal_from_target (struct gdbarch *gdbarch,
				      int signal)
{
  switch (signal)
    {
    case SPARC_LINUX_SIGEMT:
      return GDB_SIGNAL_EMT;

    case SPARC_LINUX_SIGBUS:
      return GDB_SIGNAL_BUS;

    case SPARC_LINUX_SIGSYS:
      return GDB_SIGNAL_SYS;

    case SPARC_LINUX_SIGURG:
      return GDB_SIGNAL_URG;

    case SPARC_LINUX_SIGSTOP:
      return GDB_SIGNAL_STOP;

    case SPARC_LINUX_SIGTSTP:
      return GDB_SIGNAL_TSTP;

    case SPARC_LINUX_SIGCONT:
      return GDB_SIGNAL_CONT;

    case SPARC_LINUX_SIGCHLD:
      return GDB_SIGNAL_CHLD;

    /* No way to differentiate between SIGIO and SIGPOLL.
       Therefore, we just handle the first one.  */
    case SPARC_LINUX_SIGIO:
      return GDB_SIGNAL_IO;

    /* No way to differentiate between SIGLOST and SIGPWR.
       Therefore, we just handle the first one.  */
    case SPARC_LINUX_SIGLOST:
      return GDB_SIGNAL_LOST;

    case SPARC_LINUX_SIGUSR1:
      return GDB_SIGNAL_USR1;

    case SPARC_LINUX_SIGUSR2:
      return GDB_SIGNAL_USR2;
    }

  return linux_gdb_signal_from_target (gdbarch, signal);
}

/* Implementation of `gdbarch_gdb_signal_to_target', as defined in
   gdbarch.h.  */

static int
sparc32_linux_gdb_signal_to_target (struct gdbarch *gdbarch,
				    enum gdb_signal signal)
{
  switch (signal)
    {
    case GDB_SIGNAL_EMT:
      return SPARC_LINUX_SIGEMT;

    case GDB_SIGNAL_BUS:
      return SPARC_LINUX_SIGBUS;

    case GDB_SIGNAL_SYS:
      return SPARC_LINUX_SIGSYS;

    case GDB_SIGNAL_URG:
      return SPARC_LINUX_SIGURG;

    case GDB_SIGNAL_STOP:
      return SPARC_LINUX_SIGSTOP;

    case GDB_SIGNAL_TSTP:
      return SPARC_LINUX_SIGTSTP;

    case GDB_SIGNAL_CONT:
      return SPARC_LINUX_SIGCONT;

    case GDB_SIGNAL_CHLD:
      return SPARC_LINUX_SIGCHLD;

    case GDB_SIGNAL_IO:
      return SPARC_LINUX_SIGIO;

    case GDB_SIGNAL_POLL:
      return SPARC_LINUX_SIGPOLL;

    case GDB_SIGNAL_LOST:
      return SPARC_LINUX_SIGLOST;

    case GDB_SIGNAL_PWR:
      return SPARC_LINUX_SIGPWR;

    case GDB_SIGNAL_USR1:
      return SPARC_LINUX_SIGUSR1;

    case GDB_SIGNAL_USR2:
      return SPARC_LINUX_SIGUSR2;
    }

  return linux_gdb_signal_to_target (gdbarch, signal);
}



static const struct regset sparc32_linux_gregset =
  {
    NULL,
    sparc32_linux_supply_core_gregset,
    sparc32_linux_collect_core_gregset
  };

static const struct regset sparc32_linux_fpregset =
  {
    NULL,
    sparc32_linux_supply_core_fpregset,
    sparc32_linux_collect_core_fpregset
  };

static void
sparc32_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  linux_init_abi (info, gdbarch, 0);

  tdep->gregset = &sparc32_linux_gregset;
  tdep->sizeof_gregset = 152;

  tdep->fpregset = &sparc32_linux_fpregset;
  tdep->sizeof_fpregset = 396;

  tramp_frame_prepend_unwinder (gdbarch, &sparc32_linux_sigframe);
  tramp_frame_prepend_unwinder (gdbarch, &sparc32_linux_rt_sigframe);

  /* GNU/Linux has SVR4-style shared libraries...  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, linux_ilp32_fetch_link_map_offsets);

  /* ...which means that we need some special handling when doing
     prologue analysis.  */
  tdep->plt_entry_size = 12;

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  /* Make sure we can single-step over signal return system calls.  */
  tdep->step_trap = sparc32_linux_step_trap;

  /* Hook in the DWARF CFI frame unwinder.  */
  dwarf2_append_unwinders (gdbarch);

  set_gdbarch_write_pc (gdbarch, sparc_linux_write_pc);

  /* Functions for 'catch syscall'.  */
  set_xml_syscall_file_name (gdbarch, XML_SYSCALL_FILENAME_SPARC32);
  set_gdbarch_get_syscall_number (gdbarch,
				  sparc32_linux_get_syscall_number);

  set_gdbarch_gdb_signal_from_target (gdbarch,
				      sparc32_linux_gdb_signal_from_target);
  set_gdbarch_gdb_signal_to_target (gdbarch,
				    sparc32_linux_gdb_signal_to_target);
}

void _initialize_sparc_linux_tdep ();
void
_initialize_sparc_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_sparc, 0, GDB_OSABI_LINUX,
			  sparc32_linux_init_abi);
}
