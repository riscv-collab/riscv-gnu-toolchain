/* Target-dependent code for GNU/Linux on Alpha.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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
#include "frame.h"
#include "osabi.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "regset.h"
#include "regcache.h"
#include "linux-tdep.h"
#include "alpha-tdep.h"
#include "gdbarch.h"

/* This enum represents the signals' numbers on the Alpha
   architecture.  It just contains the signal definitions which are
   different from the generic implementation.

   It is derived from the file <arch/alpha/include/uapi/asm/signal.h>,
   from the Linux kernel tree.  */

enum
  {
    /* SIGABRT is the same as in the generic implementation, but is
       defined here because SIGIOT depends on it.  */
    ALPHA_LINUX_SIGABRT = 6,
    ALPHA_LINUX_SIGEMT = 7,
    ALPHA_LINUX_SIGBUS = 10,
    ALPHA_LINUX_SIGSYS = 12,
    ALPHA_LINUX_SIGURG = 16,
    ALPHA_LINUX_SIGSTOP = 17,
    ALPHA_LINUX_SIGTSTP = 18,
    ALPHA_LINUX_SIGCONT = 19,
    ALPHA_LINUX_SIGCHLD = 20,
    ALPHA_LINUX_SIGIO = 23,
    ALPHA_LINUX_SIGINFO = 29,
    ALPHA_LINUX_SIGUSR1 = 30,
    ALPHA_LINUX_SIGUSR2 = 31,
    ALPHA_LINUX_SIGPOLL = ALPHA_LINUX_SIGIO,
    ALPHA_LINUX_SIGPWR = ALPHA_LINUX_SIGINFO,
    ALPHA_LINUX_SIGIOT = ALPHA_LINUX_SIGABRT,
  };

/* Under GNU/Linux, signal handler invocations can be identified by
   the designated code sequence that is used to return from a signal
   handler.  In particular, the return address of a signal handler
   points to a sequence that copies $sp to $16, loads $0 with the
   appropriate syscall number, and finally enters the kernel.

   This is somewhat complicated in that:
     (1) the expansion of the "mov" assembler macro has changed over
	 time, from "bis src,src,dst" to "bis zero,src,dst",
     (2) the kernel has changed from using "addq" to "lda" to load the
	 syscall number,
     (3) there is a "normal" sigreturn and an "rt" sigreturn which
	 has a different stack layout.  */

static long
alpha_linux_sigtramp_offset_1 (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  switch (alpha_read_insn (gdbarch, pc))
    {
    case 0x47de0410:		/* bis $30,$30,$16 */
    case 0x47fe0410:		/* bis $31,$30,$16 */
      return 0;

    case 0x43ecf400:		/* addq $31,103,$0 */
    case 0x201f0067:		/* lda $0,103($31) */
    case 0x201f015f:		/* lda $0,351($31) */
      return 4;

    case 0x00000083:		/* call_pal callsys */
      return 8;

    default:
      return -1;
    }
}

static LONGEST
alpha_linux_sigtramp_offset (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  long i, off;

  if (pc & 3)
    return -1;

  /* Guess where we might be in the sequence.  */
  off = alpha_linux_sigtramp_offset_1 (gdbarch, pc);
  if (off < 0)
    return -1;

  /* Verify that the other two insns of the sequence are as we expect.  */
  pc -= off;
  for (i = 0; i < 12; i += 4)
    {
      if (i == off)
	continue;
      if (alpha_linux_sigtramp_offset_1 (gdbarch, pc + i) != i)
	return -1;
    }

  return off;
}

static int
alpha_linux_pc_in_sigtramp (struct gdbarch *gdbarch,
			    CORE_ADDR pc, const char *func_name)
{
  return alpha_linux_sigtramp_offset (gdbarch, pc) >= 0;
}

static CORE_ADDR
alpha_linux_sigcontext_addr (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR pc;
  ULONGEST sp;
  long off;

  pc = get_frame_pc (this_frame);
  sp = get_frame_register_unsigned (this_frame, ALPHA_SP_REGNUM);

  off = alpha_linux_sigtramp_offset (gdbarch, pc);
  gdb_assert (off >= 0);

  /* __NR_rt_sigreturn has a couple of structures on the stack.  This is:

	struct rt_sigframe {
	  struct siginfo info;
	  struct ucontext uc;
	};

	offsetof (struct rt_sigframe, uc.uc_mcontext);  */

  if (alpha_read_insn (gdbarch, pc - off + 4) == 0x201f015f)
    return sp + 176;

  /* __NR_sigreturn has the sigcontext structure at the top of the stack.  */
  return sp;
}

/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
alpha_linux_supply_gregset (const struct regset *regset,
			    struct regcache *regcache,
			    int regnum, const void *gregs, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) gregs;

  gdb_assert (len >= 32 * 8);
  alpha_supply_int_regs (regcache, regnum, regs, regs + 31 * 8,
			 len >= 33 * 8 ? regs + 32 * 8 : NULL);
}

/* Collect register REGNUM from the register cache REGCACHE and store
   it in the buffer specified by GREGS and LEN as described by the
   general-purpose register set REGSET.  If REGNUM is -1, do this for
   all registers in REGSET.  */

static void
alpha_linux_collect_gregset (const struct regset *regset,
			     const struct regcache *regcache,
			     int regnum, void *gregs, size_t len)
{
  gdb_byte *regs = (gdb_byte *) gregs;

  gdb_assert (len >= 32 * 8);
  alpha_fill_int_regs (regcache, regnum, regs, regs + 31 * 8,
		       len >= 33 * 8 ? regs + 32 * 8 : NULL);
}

/* Supply register REGNUM from the buffer specified by FPREGS and LEN
   in the floating-point register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
alpha_linux_supply_fpregset (const struct regset *regset,
			     struct regcache *regcache,
			     int regnum, const void *fpregs, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) fpregs;

  gdb_assert (len >= 32 * 8);
  alpha_supply_fp_regs (regcache, regnum, regs, regs + 31 * 8);
}

/* Collect register REGNUM from the register cache REGCACHE and store
   it in the buffer specified by FPREGS and LEN as described by the
   general-purpose register set REGSET.  If REGNUM is -1, do this for
   all registers in REGSET.  */

static void
alpha_linux_collect_fpregset (const struct regset *regset,
			      const struct regcache *regcache,
			      int regnum, void *fpregs, size_t len)
{
  gdb_byte *regs = (gdb_byte *) fpregs;

  gdb_assert (len >= 32 * 8);
  alpha_fill_fp_regs (regcache, regnum, regs, regs + 31 * 8);
}

static const struct regset alpha_linux_gregset =
{
  NULL,
  alpha_linux_supply_gregset, alpha_linux_collect_gregset
};

static const struct regset alpha_linux_fpregset =
{
  NULL,
  alpha_linux_supply_fpregset, alpha_linux_collect_fpregset
};

/* Iterate over core file register note sections.  */

static void
alpha_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					  iterate_over_regset_sections_cb *cb,
					  void *cb_data,
					  const struct regcache *regcache)
{
  cb (".reg", 32 * 8, 32 * 8, &alpha_linux_gregset, NULL, cb_data);
  cb (".reg2", 32 * 8, 32 * 8, &alpha_linux_fpregset, NULL, cb_data);
}

/* Implementation of `gdbarch_gdb_signal_from_target', as defined in
   gdbarch.h.  */

static enum gdb_signal
alpha_linux_gdb_signal_from_target (struct gdbarch *gdbarch,
				    int signal)
{
  switch (signal)
    {
    case ALPHA_LINUX_SIGEMT:
      return GDB_SIGNAL_EMT;

    case ALPHA_LINUX_SIGBUS:
      return GDB_SIGNAL_BUS;

    case ALPHA_LINUX_SIGSYS:
      return GDB_SIGNAL_SYS;

    case ALPHA_LINUX_SIGURG:
      return GDB_SIGNAL_URG;

    case ALPHA_LINUX_SIGSTOP:
      return GDB_SIGNAL_STOP;

    case ALPHA_LINUX_SIGTSTP:
      return GDB_SIGNAL_TSTP;

    case ALPHA_LINUX_SIGCONT:
      return GDB_SIGNAL_CONT;

    case ALPHA_LINUX_SIGCHLD:
      return GDB_SIGNAL_CHLD;

    /* No way to differentiate between SIGIO and SIGPOLL.
       Therefore, we just handle the first one.  */
    case ALPHA_LINUX_SIGIO:
      return GDB_SIGNAL_IO;

    /* No way to differentiate between SIGINFO and SIGPWR.
       Therefore, we just handle the first one.  */
    case ALPHA_LINUX_SIGINFO:
      return GDB_SIGNAL_INFO;

    case ALPHA_LINUX_SIGUSR1:
      return GDB_SIGNAL_USR1;

    case ALPHA_LINUX_SIGUSR2:
      return GDB_SIGNAL_USR2;
    }

  return linux_gdb_signal_from_target (gdbarch, signal);
}

/* Implementation of `gdbarch_gdb_signal_to_target', as defined in
   gdbarch.h.  */

static int
alpha_linux_gdb_signal_to_target (struct gdbarch *gdbarch,
				  enum gdb_signal signal)
{
  switch (signal)
    {
    case GDB_SIGNAL_EMT:
      return ALPHA_LINUX_SIGEMT;

    case GDB_SIGNAL_BUS:
      return ALPHA_LINUX_SIGBUS;

    case GDB_SIGNAL_SYS:
      return ALPHA_LINUX_SIGSYS;

    case GDB_SIGNAL_URG:
      return ALPHA_LINUX_SIGURG;

    case GDB_SIGNAL_STOP:
      return ALPHA_LINUX_SIGSTOP;

    case GDB_SIGNAL_TSTP:
      return ALPHA_LINUX_SIGTSTP;

    case GDB_SIGNAL_CONT:
      return ALPHA_LINUX_SIGCONT;

    case GDB_SIGNAL_CHLD:
      return ALPHA_LINUX_SIGCHLD;

    case GDB_SIGNAL_IO:
      return ALPHA_LINUX_SIGIO;

    case GDB_SIGNAL_INFO:
      return ALPHA_LINUX_SIGINFO;

    case GDB_SIGNAL_USR1:
      return ALPHA_LINUX_SIGUSR1;

    case GDB_SIGNAL_USR2:
      return ALPHA_LINUX_SIGUSR2;

    case GDB_SIGNAL_POLL:
      return ALPHA_LINUX_SIGPOLL;

    case GDB_SIGNAL_PWR:
      return ALPHA_LINUX_SIGPWR;
    }

  return linux_gdb_signal_to_target (gdbarch, signal);
}

static void
alpha_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  linux_init_abi (info, gdbarch, 0);

  /* Hook into the DWARF CFI frame unwinder.  */
  alpha_dwarf2_init_abi (info, gdbarch);

  /* Hook into the MDEBUG frame unwinder.  */
  alpha_mdebug_init_abi (info, gdbarch);

  alpha_gdbarch_tdep *tdep = gdbarch_tdep<alpha_gdbarch_tdep> (gdbarch);
  tdep->dynamic_sigtramp_offset = alpha_linux_sigtramp_offset;
  tdep->sigcontext_addr = alpha_linux_sigcontext_addr;
  tdep->pc_in_sigtramp = alpha_linux_pc_in_sigtramp;
  tdep->jb_pc = 2;
  tdep->jb_elt_size = 8;

  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, linux_lp64_fetch_link_map_offsets);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, alpha_linux_iterate_over_regset_sections);

  set_gdbarch_gdb_signal_from_target (gdbarch,
				      alpha_linux_gdb_signal_from_target);
  set_gdbarch_gdb_signal_to_target (gdbarch,
				    alpha_linux_gdb_signal_to_target);
}

void _initialize_alpha_linux_tdep ();
void
_initialize_alpha_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_alpha, 0, GDB_OSABI_LINUX,
			  alpha_linux_init_abi);
}
