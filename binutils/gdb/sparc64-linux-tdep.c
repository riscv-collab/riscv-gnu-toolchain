/* Target-dependent code for GNU/Linux UltraSPARC.

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
#include "frame.h"
#include "frame-unwind.h"
#include "dwarf2/frame.h"
#include "regset.h"
#include "regcache.h"
#include "gdbarch.h"
#include "gdbcore.h"
#include "osabi.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "xml-syscall.h"
#include "linux-tdep.h"

/* ADI specific si_code */
#ifndef SEGV_ACCADI
#define SEGV_ACCADI	3
#endif
#ifndef SEGV_ADIDERR
#define SEGV_ADIDERR	4
#endif
#ifndef SEGV_ADIPERR
#define SEGV_ADIPERR	5
#endif

/* The syscall's XML filename for sparc 64-bit.  */
#define XML_SYSCALL_FILENAME_SPARC64 "syscalls/sparc64-linux.xml"

#include "sparc64-tdep.h"

/* Signal trampoline support.  */

static void sparc64_linux_sigframe_init (const struct tramp_frame *self,
					 frame_info_ptr this_frame,
					 struct trad_frame_cache *this_cache,
					 CORE_ADDR func);

/* See sparc-linux-tdep.c for details.  Note that 64-bit binaries only
   use RT signals.  */

static const struct tramp_frame sparc64_linux_rt_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0x82102065, ULONGEST_MAX },		/* mov __NR_rt_sigreturn, %g1 */
    { 0x91d0206d, ULONGEST_MAX },		/* ta  0x6d */
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  sparc64_linux_sigframe_init
};

static void
sparc64_linux_sigframe_init (const struct tramp_frame *self,
			     frame_info_ptr this_frame,
			     struct trad_frame_cache *this_cache,
			     CORE_ADDR func)
{
  CORE_ADDR base, addr, sp_addr;
  int regnum;

  base = get_frame_register_unsigned (this_frame, SPARC_O1_REGNUM);
  base += 128;

  /* Offsets from <bits/sigcontext.h>.  */

  /* Since %g0 is always zero, keep the identity encoding.  */
  addr = base + 8;
  sp_addr = base + ((SPARC_SP_REGNUM - SPARC_G0_REGNUM) * 8);
  for (regnum = SPARC_G1_REGNUM; regnum <= SPARC_O7_REGNUM; regnum++)
    {
      trad_frame_set_reg_addr (this_cache, regnum, addr);
      addr += 8;
    }

  trad_frame_set_reg_addr (this_cache, SPARC64_STATE_REGNUM, addr + 0);
  trad_frame_set_reg_addr (this_cache, SPARC64_PC_REGNUM, addr + 8);
  trad_frame_set_reg_addr (this_cache, SPARC64_NPC_REGNUM, addr + 16);
  trad_frame_set_reg_addr (this_cache, SPARC64_Y_REGNUM, addr + 24);
  trad_frame_set_reg_addr (this_cache, SPARC64_FPRS_REGNUM, addr + 28);

  base = get_frame_register_unsigned (this_frame, SPARC_SP_REGNUM);
  if (base & 1)
    base += BIAS;

  addr = get_frame_memory_unsigned (this_frame, sp_addr, 8);
  if (addr & 1)
    addr += BIAS;

  for (regnum = SPARC_L0_REGNUM; regnum <= SPARC_I7_REGNUM; regnum++)
    {
      trad_frame_set_reg_addr (this_cache, regnum, addr);
      addr += 8;
    }
  trad_frame_set_id (this_cache, frame_id_build (base, func));
}

/* sparc64 GNU/Linux implementation of the report_signal_info
   gdbarch hook.
   Displays information related to ADI memory corruptions.  */

static void
sparc64_linux_report_signal_info (struct gdbarch *gdbarch, struct ui_out *uiout,
				  enum gdb_signal siggnal)
{
  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word != 64
      || siggnal != GDB_SIGNAL_SEGV)
    return;

  CORE_ADDR addr = 0;
  long si_code = 0;

  try
    {
      /* Evaluate si_code to see if the segfault is ADI related.  */
      si_code = parse_and_eval_long ("$_siginfo.si_code\n");

      if (si_code >= SEGV_ACCADI && si_code <= SEGV_ADIPERR)
	addr = parse_and_eval_long ("$_siginfo._sifields._sigfault.si_addr");
    }
  catch (const gdb_exception_error &exception)
    {
      return;
    }

  /* Print out ADI event based on sig_code value */
  switch (si_code)
    {
    case SEGV_ACCADI:	/* adi not enabled */
      uiout->text ("\n");
      uiout->field_string ("sigcode-meaning", _("ADI disabled"));
      uiout->text (_(" while accessing address "));
      uiout->field_core_addr ("bound-access", gdbarch, addr);
      break;
    case SEGV_ADIDERR:	/* disrupting mismatch */
      uiout->text ("\n");
      uiout->field_string ("sigcode-meaning", _("ADI deferred mismatch"));
      uiout->text (_(" while accessing address "));
      uiout->field_core_addr ("bound-access", gdbarch, addr);
      break;
    case SEGV_ADIPERR:	/* precise mismatch */
      uiout->text ("\n");
      uiout->field_string ("sigcode-meaning", _("ADI precise mismatch"));
      uiout->text (_(" while accessing address "));
      uiout->field_core_addr ("bound-access", gdbarch, addr);
      break;
    default:
      break;
    }

}


/* Return the address of a system call's alternative return
   address.  */

static CORE_ADDR
sparc64_linux_step_trap (frame_info_ptr frame, unsigned long insn)
{
  /* __NR_rt_sigreturn is 101  */
  if ((insn == 0x91d0206d)
      && (get_frame_register_unsigned (frame, SPARC_G1_REGNUM) == 101))
    {
      struct gdbarch *gdbarch = get_frame_arch (frame);
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

      ULONGEST sp = get_frame_register_unsigned (frame, SPARC_SP_REGNUM);
      if (sp & 1)
	sp += BIAS;

      /* The kernel puts the sigreturn registers on the stack,
	 and this is where the signal unwinding state is take from
	 when returning from a signal.

	 A siginfo_t sits 192 bytes from the base of the stack.  This
	 siginfo_t is 128 bytes, and is followed by the sigreturn
	 register save area.  The saved PC sits at a 136 byte offset
	 into there.  */

      return read_memory_unsigned_integer (sp + 192 + 128 + 136,
					   8, byte_order);
    }

  return 0;
}


const struct sparc_gregmap sparc64_linux_core_gregmap =
{
  32 * 8,			/* %tstate */
  33 * 8,			/* %tpc */
  34 * 8,			/* %tnpc */
  35 * 8,			/* %y */
  -1,				/* %wim */
  -1,				/* %tbr */
  1 * 8,			/* %g1 */
  16 * 8,			/* %l0 */
  8,				/* y size */
};


static void
sparc64_linux_supply_core_gregset (const struct regset *regset,
				   struct regcache *regcache,
				   int regnum, const void *gregs, size_t len)
{
  sparc64_supply_gregset (&sparc64_linux_core_gregmap,
			  regcache, regnum, gregs);
}

static void
sparc64_linux_collect_core_gregset (const struct regset *regset,
				    const struct regcache *regcache,
				    int regnum, void *gregs, size_t len)
{
  sparc64_collect_gregset (&sparc64_linux_core_gregmap,
			   regcache, regnum, gregs);
}

static void
sparc64_linux_supply_core_fpregset (const struct regset *regset,
				    struct regcache *regcache,
				    int regnum, const void *fpregs, size_t len)
{
  sparc64_supply_fpregset (&sparc64_bsd_fpregmap, regcache, regnum, fpregs);
}

static void
sparc64_linux_collect_core_fpregset (const struct regset *regset,
				     const struct regcache *regcache,
				     int regnum, void *fpregs, size_t len)
{
  sparc64_collect_fpregset (&sparc64_bsd_fpregmap, regcache, regnum, fpregs);
}

/* Set the program counter for process PTID to PC.  */

#define TSTATE_SYSCALL	0x0000000000000020ULL

static void
sparc64_linux_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  gdbarch *arch = regcache->arch ();
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (arch);
  ULONGEST state;

  regcache_cooked_write_unsigned (regcache, tdep->pc_regnum, pc);
  regcache_cooked_write_unsigned (regcache, tdep->npc_regnum, pc + 4);

  /* Clear the "in syscall" bit to prevent the kernel from
     messing with the PCs we just installed, if we happen to be
     within an interrupted system call that the kernel wants to
     restart.

     Note that after we return from the dummy call, the TSTATE et al.
     registers will be automatically restored, and the kernel
     continues to restart the system call at this point.  */
  regcache_cooked_read_unsigned (regcache, SPARC64_STATE_REGNUM, &state);
  state &= ~TSTATE_SYSCALL;
  regcache_cooked_write_unsigned (regcache, SPARC64_STATE_REGNUM, state);
}

static LONGEST
sparc64_linux_get_syscall_number (struct gdbarch *gdbarch,
				  thread_info *thread)
{
  struct regcache *regcache = get_thread_regcache (thread);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  /* The content of a register.  */
  gdb_byte buf[8];
  /* The result.  */
  LONGEST ret;

  /* Getting the system call number from the register.
     When dealing with the sparc architecture, this information
     is stored at the %g1 register.  */
  regcache->cooked_read (SPARC_G1_REGNUM, buf);

  ret = extract_signed_integer (buf, 8, byte_order);

  return ret;
}


/* Implement the "get_longjmp_target" gdbarch method.  */

static int
sparc64_linux_get_longjmp_target (frame_info_ptr frame, CORE_ADDR *pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  CORE_ADDR jb_addr;
  gdb_byte buf[8];

  jb_addr = get_frame_register_unsigned (frame, SPARC_O0_REGNUM);

  /* setjmp and longjmp in SPARC64 are implemented in glibc using the
     setcontext and getcontext system calls respectively.  These
     system calls operate on ucontext_t structures, which happen to
     partially have the same structure than jmp_buf.  However the
     ucontext returned by getcontext, and thus the jmp_buf structure
     returned by setjmp, contains the context of the trap instruction
     in the glibc __[sig]setjmp wrapper, not the context of the user
     code calling setjmp.

     %o7 in the jmp_buf structure is stored at offset 18*8 in the
     mc_gregs array, which is itself located at offset 32 into
     jmp_buf.  See bits/setjmp.h.  This register contains the address
     of the 'call setjmp' instruction in user code.

     In order to determine the longjmp target address in the
     initiating frame we need to examine the call instruction itself,
     in particular whether the annul bit is set.  If it is not set
     then we need to jump over the instruction at the delay slot.  */

  if (target_read_memory (jb_addr + 32 + (18 * 8), buf, 8))
    return 0;

  *pc = extract_unsigned_integer (buf, 8, gdbarch_byte_order (gdbarch));

  if (!sparc_is_annulled_branch_insn (*pc))
      *pc += 4; /* delay slot insn  */
  *pc += 4; /* call insn  */

  return 1;
}



static const struct regset sparc64_linux_gregset =
  {
    NULL,
    sparc64_linux_supply_core_gregset,
    sparc64_linux_collect_core_gregset
  };

static const struct regset sparc64_linux_fpregset =
  {
    NULL,
    sparc64_linux_supply_core_fpregset,
    sparc64_linux_collect_core_fpregset
  };

static void
sparc64_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  linux_init_abi (info, gdbarch, 0);

  tdep->gregset = &sparc64_linux_gregset;
  tdep->sizeof_gregset = 288;

  tdep->fpregset = &sparc64_linux_fpregset;
  tdep->sizeof_fpregset = 280;

  tramp_frame_prepend_unwinder (gdbarch, &sparc64_linux_rt_sigframe);

  /* Hook in the DWARF CFI frame unwinder.  */
  dwarf2_append_unwinders (gdbarch);

  sparc64_init_abi (info, gdbarch);

  /* GNU/Linux has SVR4-style shared libraries...  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, linux_lp64_fetch_link_map_offsets);

  /* ...which means that we need some special handling when doing
     prologue analysis.  */
  tdep->plt_entry_size = 16;

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  /* Make sure we can single-step over signal return system calls.  */
  tdep->step_trap = sparc64_linux_step_trap;

  /* Make sure we can single-step over longjmp calls.  */
  set_gdbarch_get_longjmp_target (gdbarch, sparc64_linux_get_longjmp_target);

  set_gdbarch_write_pc (gdbarch, sparc64_linux_write_pc);

  /* Functions for 'catch syscall'.  */
  set_xml_syscall_file_name (gdbarch, XML_SYSCALL_FILENAME_SPARC64);
  set_gdbarch_get_syscall_number (gdbarch,
				  sparc64_linux_get_syscall_number);
  set_gdbarch_report_signal_info (gdbarch, sparc64_linux_report_signal_info);
}

void _initialize_sparc64_linux_tdep ();
void
_initialize_sparc64_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_sparc, bfd_mach_sparc_v9,
			  GDB_OSABI_LINUX, sparc64_linux_init_abi);
}
