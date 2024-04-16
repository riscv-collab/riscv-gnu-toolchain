/* Target dependent code for GNU/Linux ARC.

   Copyright 2020-2024 Free Software Foundation, Inc.

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

/* GDB header files.  */
#include "defs.h"
#include "linux-tdep.h"
#include "objfiles.h"
#include "opcode/arc.h"
#include "osabi.h"
#include "solib-svr4.h"
#include "disasm.h"

/* ARC header files.  */
#include "opcodes/arc-dis.h"
#include "arc-linux-tdep.h"
#include "arc-tdep.h"
#include "arch/arc.h"

/* Print an "arc-linux" debug statement.  */

#define arc_linux_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (arc_debug, "arc-linux", fmt, ##__VA_ARGS__)

#define REGOFF(offset) (offset * ARC_REGISTER_SIZE)

/* arc_linux_sc_reg_offsets[i] is the offset of register i in the `struct
   sigcontext'.  Array index is an internal GDB register number, as defined in
   arc-tdep.h:arc_regnum.

   From <include/uapi/asm/sigcontext.h> and <include/uapi/asm/ptrace.h>.

   The layout of this struct is tightly bound to "arc_regnum" enum
   in arc-tdep.h.  Any change of order in there, must be reflected
   here as well.  */
static const int arc_linux_sc_reg_offsets[] = {
  /* R0 - R12.  */
  REGOFF (22), REGOFF (21), REGOFF (20), REGOFF (19),
  REGOFF (18), REGOFF (17), REGOFF (16), REGOFF (15),
  REGOFF (14), REGOFF (13), REGOFF (12), REGOFF (11),
  REGOFF (10),

  /* R13 - R25.  */
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER,

  REGOFF (9),			/* R26 (GP) */
  REGOFF (8),			/* FP */
  REGOFF (23),			/* SP */
  ARC_OFFSET_NO_REGISTER,	/* ILINK */
  ARC_OFFSET_NO_REGISTER,	/* R30 */
  REGOFF (7),			/* BLINK */

  /* R32 - R59.  */
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER,

  REGOFF (4),			/* LP_COUNT */
  ARC_OFFSET_NO_REGISTER,	/* RESERVED */
  ARC_OFFSET_NO_REGISTER,	/* LIMM */
  ARC_OFFSET_NO_REGISTER,	/* PCL */

  REGOFF (6),			/* PC  */
  REGOFF (5),			/* STATUS32 */
  REGOFF (2),			/* LP_START */
  REGOFF (3),			/* LP_END */
  REGOFF (1),			/* BTA */
};

/* arc_linux_core_reg_offsets[i] is the offset in the .reg section of GDB
   regnum i.  Array index is an internal GDB register number, as defined in
   arc-tdep.h:arc_regnum.

   From include/uapi/asm/ptrace.h in the ARC Linux sources.  */

/* The layout of this struct is tightly bound to "arc_regnum" enum
   in arc-tdep.h.  Any change of order in there, must be reflected
   here as well.  */
static const int arc_linux_core_reg_offsets[] = {
  /* R0 - R12.  */
  REGOFF (22), REGOFF (21), REGOFF (20), REGOFF (19),
  REGOFF (18), REGOFF (17), REGOFF (16), REGOFF (15),
  REGOFF (14), REGOFF (13), REGOFF (12), REGOFF (11),
  REGOFF (10),

  /* R13 - R25.  */
  REGOFF (37), REGOFF (36), REGOFF (35), REGOFF (34),
  REGOFF (33), REGOFF (32), REGOFF (31), REGOFF (30),
  REGOFF (29), REGOFF (28), REGOFF (27), REGOFF (26),
  REGOFF (25),

  REGOFF (9),			/* R26 (GP) */
  REGOFF (8),			/* FP */
  REGOFF (23),			/* SP */
  ARC_OFFSET_NO_REGISTER,	/* ILINK */
  ARC_OFFSET_NO_REGISTER,	/* R30 */
  REGOFF (7),			/* BLINK */

  /* R32 - R59.  */
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER, ARC_OFFSET_NO_REGISTER,
  ARC_OFFSET_NO_REGISTER,

  REGOFF (4),			/* LP_COUNT */
  ARC_OFFSET_NO_REGISTER,	/* RESERVED */
  ARC_OFFSET_NO_REGISTER,	/* LIMM */
  ARC_OFFSET_NO_REGISTER,	/* PCL */

  REGOFF (39),			/* PC  */
  REGOFF (5),			/* STATUS32 */
  REGOFF (2),			/* LP_START */
  REGOFF (3),			/* LP_END */
  REGOFF (1),			/* BTA */
  REGOFF (6)			/* ERET */
};

/* Is THIS_FRAME a sigtramp function - the function that returns from
   signal handler into normal execution flow? This is the case if the PC is
   either at the start of, or in the middle of the two instructions:

     mov r8, __NR_rt_sigreturn ; __NR_rt_sigreturn == 139
     trap_s 0 ; `swi' for ARC700

   On ARC uClibc Linux this function is called __default_rt_sa_restorer.

   Returns TRUE if this is a sigtramp frame.  */

static bool
arc_linux_is_sigtramp (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR pc = get_frame_pc (this_frame);

  arc_linux_debug_printf ("pc=%s", paddress(gdbarch, pc));

  static const gdb_byte insns_be_hs[] = {
    0x20, 0x8a, 0x12, 0xc2,	/* mov  r8,nr_rt_sigreturn */
    0x78, 0x1e			/* trap_s 0 */
  };
  static const gdb_byte insns_be_700[] = {
    0x20, 0x8a, 0x12, 0xc2,	/* mov  r8,nr_rt_sigreturn */
    0x22, 0x6f, 0x00, 0x3f	/* swi */
  };

  gdb_byte arc_sigtramp_insns[sizeof (insns_be_700)];
  size_t insns_sz;
  if (arc_mach_is_arcv2 (gdbarch))
    {
      insns_sz = sizeof (insns_be_hs);
      memcpy (arc_sigtramp_insns, insns_be_hs, insns_sz);
    }
  else
    {
      insns_sz = sizeof (insns_be_700);
      memcpy (arc_sigtramp_insns, insns_be_700, insns_sz);
    }
  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_LITTLE)
    {
      /* On little endian targets, ARC code section is in what is called
	 "middle endian", where half-words are in the big-endian order,
	 only bytes inside the halfwords are in the little endian order.
	 As a result it is very easy to convert big endian instruction to
	 little endian, since it is needed to swap bytes in the halfwords,
	 so there is no need to have information on whether that is a
	 4-byte instruction or 2-byte.  */
      gdb_assert ((insns_sz % 2) == 0);
      for (int i = 0; i < insns_sz; i += 2)
	std::swap (arc_sigtramp_insns[i], arc_sigtramp_insns[i+1]);
    }

  gdb_byte buf[insns_sz];

  /* Read the memory at the PC.  Since we are stopped, any breakpoint must
     have been removed.  */
  if (!safe_frame_unwind_memory (this_frame, pc, {buf, insns_sz}))
    {
      /* Failed to unwind frame.  */
      return FALSE;
    }

  /* Is that code the sigtramp instruction sequence?  */
  if (memcmp (buf, arc_sigtramp_insns, insns_sz) == 0)
    return TRUE;

  /* No - look one instruction earlier in the code...  */
  if (!safe_frame_unwind_memory (this_frame, pc - 4, {buf, insns_sz}))
    {
      /* Failed to unwind frame.  */
      return FALSE;
    }

  return (memcmp (buf, arc_sigtramp_insns, insns_sz) == 0);
}

/* Get sigcontext structure of sigtramp frame - it contains saved
   registers of interrupted frame.

   Stack pointer points to the rt_sigframe structure, and sigcontext can
   be found as in:

   struct rt_sigframe {
     struct siginfo info;
     struct ucontext uc;
     ...
   };

   struct ucontext {
     unsigned long uc_flags;
     struct ucontext *uc_link;
     stack_t uc_stack;
     struct sigcontext uc_mcontext;
     sigset_t uc_sigmask;
   };

   sizeof (struct siginfo) == 0x80
   offsetof (struct ucontext, uc_mcontext) == 0x14

   GDB cannot include linux headers and use offsetof () because those are
   target headers and GDB might be built for a different run host.  There
   doesn't seem to be an established mechanism to figure out those offsets
   via gdbserver, so the only way is to hardcode values in the GDB,
   meaning that GDB will be broken if values will change.  That seems to
   be a very unlikely scenario and other arches (aarch64, alpha, amd64,
   etc) in GDB hardcode values.  */

static CORE_ADDR
arc_linux_sigcontext_addr (frame_info_ptr this_frame)
{
  const int ucontext_offset = 0x80;
  const int sigcontext_offset = 0x14;
  return get_frame_sp (this_frame) + ucontext_offset + sigcontext_offset;
}

/* Implement the "cannot_fetch_register" gdbarch method.  */

static int
arc_linux_cannot_fetch_register (struct gdbarch *gdbarch, int regnum)
{
  /* Assume that register is readable if it is unknown.  */
  switch (regnum)
    {
    case ARC_ILINK_REGNUM:
    case ARC_RESERVED_REGNUM:
    case ARC_LIMM_REGNUM:
      return true;
    case ARC_R30_REGNUM:
    case ARC_R58_REGNUM:
    case ARC_R59_REGNUM:
      return !arc_mach_is_arcv2 (gdbarch);
    }
  return (regnum > ARC_BLINK_REGNUM) && (regnum < ARC_LP_COUNT_REGNUM);
}

/* Implement the "cannot_store_register" gdbarch method.  */

static int
arc_linux_cannot_store_register (struct gdbarch *gdbarch, int regnum)
{
  /* Assume that register is writable if it is unknown.  */
  switch (regnum)
    {
    case ARC_ILINK_REGNUM:
    case ARC_RESERVED_REGNUM:
    case ARC_LIMM_REGNUM:
    case ARC_PCL_REGNUM:
      return true;
    case ARC_R30_REGNUM:
    case ARC_R58_REGNUM:
    case ARC_R59_REGNUM:
      return !arc_mach_is_arcv2 (gdbarch);
    }
  return (regnum > ARC_BLINK_REGNUM) && (regnum < ARC_LP_COUNT_REGNUM);
}

/* For ARC Linux, breakpoints use the 16-bit TRAP_S 1 instruction, which
   is 0x3e78 (little endian) or 0x783e (big endian).  */

static const gdb_byte arc_linux_trap_s_be[] = { 0x78, 0x3e };
static const gdb_byte arc_linux_trap_s_le[] = { 0x3e, 0x78 };
static const int trap_size = 2;   /* Number of bytes to insert "trap".  */

/* Implement the "breakpoint_kind_from_pc" gdbarch method.  */

static int
arc_linux_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  return trap_size;
}

/* Implement the "sw_breakpoint_from_kind" gdbarch method.  */

static const gdb_byte *
arc_linux_sw_breakpoint_from_kind (struct gdbarch *gdbarch,
				   int kind, int *size)
{
  gdb_assert (kind == trap_size);
  *size = kind;
  return ((gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	  ? arc_linux_trap_s_be
	  : arc_linux_trap_s_le);
}

/* Check for an atomic sequence of instructions beginning with an
   LLOCK instruction and ending with a SCOND instruction.

   These patterns are hand coded in libc's (glibc and uclibc). Take
   a look at [1] for instance:

   main+14: llock   r2,[r0]
   main+18: brne.nt r2,0,main+30
   main+22: scond   r3,[r0]
   main+26: bne     main+14
   main+30: mov_s   r0,0

   If such a sequence is found, attempt to step over it.
   A breakpoint is placed at the end of the sequence.

   This function expects the INSN to be a "llock(d)" instruction.

   [1]
   https://cgit.uclibc-ng.org/cgi/cgit/uclibc-ng.git/tree/libc/ \
     sysdeps/linux/arc/bits/atomic.h#n46
   */

static std::vector<CORE_ADDR>
handle_atomic_sequence (arc_instruction insn, disassemble_info *di)
{
  const int atomic_seq_len = 24;    /* Instruction sequence length.  */
  std::vector<CORE_ADDR> next_pcs;

  /* Sanity check.  */
  gdb_assert (insn.insn_class == LLOCK);

  /* Data size we are dealing with: LLOCK vs. LLOCKD  */
  arc_ldst_data_size llock_data_size_mode = insn.data_size_mode;
  /* Indicator if any conditional branch is found in the sequence.  */
  bool found_bc = false;
  /* Becomes true if "LLOCK(D) .. SCOND(D)" sequence is found.  */
  bool is_pattern_valid = false;

  for (int insn_count = 0; insn_count < atomic_seq_len; ++insn_count)
    {
      arc_insn_decode (arc_insn_get_linear_next_pc (insn),
		       di, arc_delayed_print_insn, &insn);

      if (insn.insn_class == BRCC)
	{
	  /* If more than one conditional branch is found, this is not
	     the pattern we are interested in.  */
	  if (found_bc)
	    break;
	  found_bc = true;
	  continue;
	}

      /* This is almost a happy ending.  */
      if (insn.insn_class == SCOND)
	{
	  /* SCOND should match the LLOCK's data size.  */
	  if (insn.data_size_mode == llock_data_size_mode)
	    is_pattern_valid = true;
	  break;
	}
    }

  if (is_pattern_valid)
    {
      /* Get next instruction after scond(d).  There is no limm.  */
      next_pcs.push_back (insn.address + insn.length);
    }

  return next_pcs;
}

/* Implement the "software_single_step" gdbarch method.  */

static std::vector<CORE_ADDR>
arc_linux_software_single_step (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  arc_gdbarch_tdep *tdep = gdbarch_tdep<arc_gdbarch_tdep> (gdbarch);
  struct gdb_non_printing_memory_disassembler dis (gdbarch);

  /* Read current instruction.  */
  struct arc_instruction curr_insn;
  arc_insn_decode (regcache_read_pc (regcache), dis.disasm_info (),
		   arc_delayed_print_insn, &curr_insn);

  if (curr_insn.insn_class == LLOCK)
    return handle_atomic_sequence (curr_insn, dis.disasm_info ());

  CORE_ADDR next_pc = arc_insn_get_linear_next_pc (curr_insn);
  std::vector<CORE_ADDR> next_pcs;

  /* For instructions with delay slots, the fall thru is not the
     instruction immediately after the current instruction, but the one
     after that.  */
  if (curr_insn.has_delay_slot)
    {
      struct arc_instruction next_insn;
      arc_insn_decode (next_pc, dis.disasm_info (), arc_delayed_print_insn,
		       &next_insn);
      next_pcs.push_back (arc_insn_get_linear_next_pc (next_insn));
    }
  else
    next_pcs.push_back (next_pc);

  ULONGEST status32;
  regcache_cooked_read_unsigned (regcache, gdbarch_ps_regnum (gdbarch),
				 &status32);

  if (curr_insn.is_control_flow)
    {
      CORE_ADDR branch_pc = arc_insn_get_branch_target (curr_insn);
      if (branch_pc != next_pc)
	next_pcs.push_back (branch_pc);
    }
  /* Is current instruction the last in a loop body?  */
  else if (tdep->has_hw_loops)
    {
      /* If STATUS32.L is 1, then ZD-loops are disabled.  */
      if ((status32 & ARC_STATUS32_L_MASK) == 0)
	{
	  ULONGEST lp_end, lp_start, lp_count;
	  regcache_cooked_read_unsigned (regcache, ARC_LP_START_REGNUM,
					 &lp_start);
	  regcache_cooked_read_unsigned (regcache, ARC_LP_END_REGNUM, &lp_end);
	  regcache_cooked_read_unsigned (regcache, ARC_LP_COUNT_REGNUM,
					 &lp_count);

	  arc_linux_debug_printf ("lp_start = %s, lp_end = %s, "
				  "lp_count = %s, next_pc = %s",
				  paddress (gdbarch, lp_start),
				  paddress (gdbarch, lp_end),
				  pulongest (lp_count),
				  paddress (gdbarch, next_pc));

	  if (next_pc == lp_end && lp_count > 1)
	    {
	      /* The instruction is in effect a jump back to the start of
		 the loop.  */
	      next_pcs.push_back (lp_start);
	    }
	}
    }

  /* Is this a delay slot?  Then next PC is in BTA register.  */
  if ((status32 & ARC_STATUS32_DE_MASK) != 0)
    {
      ULONGEST bta;
      regcache_cooked_read_unsigned (regcache, ARC_BTA_REGNUM, &bta);
      next_pcs.push_back (bta);
    }

  return next_pcs;
}

/* Implement the "skip_solib_resolver" gdbarch method.

   See glibc_skip_solib_resolver for details.  */

static CORE_ADDR
arc_linux_skip_solib_resolver (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  /* For uClibc 0.9.26+.

     An unresolved PLT entry points to "__dl_linux_resolve", which calls
     "_dl_linux_resolver" to do the resolving and then eventually jumps to
     the function.

     So we look for the symbol `_dl_linux_resolver', and if we are there,
     gdb sets a breakpoint at the return address, and continues.  */
  struct bound_minimal_symbol resolver
    = lookup_minimal_symbol ("_dl_linux_resolver", NULL, NULL);

  if (arc_debug)
    {
      if (resolver.minsym != nullptr)
	{
	  CORE_ADDR res_addr = resolver.value_address ();
	  arc_linux_debug_printf ("pc = %s, resolver at %s",
				  print_core_address (gdbarch, pc),
				  print_core_address (gdbarch, res_addr));
	}
      else
	arc_linux_debug_printf ("pc = %s, no resolver found",
				print_core_address (gdbarch, pc));
    }

  if (resolver.minsym != nullptr && resolver.value_address () == pc)
    {
      /* Find the return address.  */
      return frame_unwind_caller_pc (get_current_frame ());
    }
  else
    {
      /* No breakpoint required.  */
      return 0;
    }
}

/* Populate REGCACHE with register REGNUM from BUF.  */

static void
supply_register (struct regcache *regcache, int regnum, const gdb_byte *buf)
{
  /* Skip non-existing registers.  */
  if ((arc_linux_core_reg_offsets[regnum] == ARC_OFFSET_NO_REGISTER))
    return;

  regcache->raw_supply (regnum, buf + arc_linux_core_reg_offsets[regnum]);
}

void
arc_linux_supply_gregset (const struct regset *regset,
			  struct regcache *regcache,
			  int regnum, const void *gregs, size_t size)
{
  static_assert (ARC_LAST_REGNUM
		     < ARRAY_SIZE (arc_linux_core_reg_offsets));

  const bfd_byte *buf = (const bfd_byte *) gregs;

  /* REGNUM == -1 means writing all the registers.  */
  if (regnum == -1)
    for (int reg = 0; reg <= ARC_LAST_REGNUM; reg++)
      supply_register (regcache, reg, buf);
  else if (regnum <= ARC_LAST_REGNUM)
    supply_register (regcache, regnum, buf);
  else
    gdb_assert_not_reached ("Invalid regnum in arc_linux_supply_gregset.");
}

void
arc_linux_supply_v2_regset (const struct regset *regset,
			    struct regcache *regcache, int regnum,
			    const void *v2_regs, size_t size)
{
  const bfd_byte *buf = (const bfd_byte *) v2_regs;

  /* user_regs_arcv2 is defined in linux arch/arc/include/uapi/asm/ptrace.h.  */
  if (regnum == -1 || regnum == ARC_R30_REGNUM)
    regcache->raw_supply (ARC_R30_REGNUM, buf);
  if (regnum == -1 || regnum == ARC_R58_REGNUM)
    regcache->raw_supply (ARC_R58_REGNUM, buf + REGOFF (1));
  if (regnum == -1 || regnum == ARC_R59_REGNUM)
    regcache->raw_supply (ARC_R59_REGNUM, buf + REGOFF (2));
}

/* Populate BUF with register REGNUM from the REGCACHE.  */

static void
collect_register (const struct regcache *regcache, struct gdbarch *gdbarch,
		  int regnum, gdb_byte *buf)
{
  int offset;

  /* Skip non-existing registers.  */
  if (arc_linux_core_reg_offsets[regnum] == ARC_OFFSET_NO_REGISTER)
    return;

  /* The address where the execution has stopped is in pseudo-register
     STOP_PC.  However, when kernel code is returning from the exception,
     it uses the value from ERET register.  Since, TRAP_S (the breakpoint
     instruction) commits, the ERET points to the next instruction.  In
     other words: ERET != STOP_PC.  To jump back from the kernel code to
     the correct address, ERET must be overwritten by GDB's STOP_PC.  Else,
     the program will continue at the address after the current instruction.
     */
  if (regnum == gdbarch_pc_regnum (gdbarch))
    offset = arc_linux_core_reg_offsets[ARC_ERET_REGNUM];
  else
    offset = arc_linux_core_reg_offsets[regnum];
  regcache->raw_collect (regnum, buf + offset);
}

void
arc_linux_collect_gregset (const struct regset *regset,
			   const struct regcache *regcache,
			   int regnum, void *gregs, size_t size)
{
  static_assert (ARC_LAST_REGNUM
		     < ARRAY_SIZE (arc_linux_core_reg_offsets));

  gdb_byte *buf = (gdb_byte *) gregs;
  struct gdbarch *gdbarch = regcache->arch ();

  /* REGNUM == -1 means writing all the registers.  */
  if (regnum == -1)
    for (int reg = 0; reg <= ARC_LAST_REGNUM; reg++)
      collect_register (regcache, gdbarch, reg, buf);
  else if (regnum <= ARC_LAST_REGNUM)
    collect_register (regcache, gdbarch, regnum, buf);
  else
    gdb_assert_not_reached ("Invalid regnum in arc_linux_collect_gregset.");
}

void
arc_linux_collect_v2_regset (const struct regset *regset,
			     const struct regcache *regcache, int regnum,
			     void *v2_regs, size_t size)
{
  bfd_byte *buf = (bfd_byte *) v2_regs;

  if (regnum == -1 || regnum == ARC_R30_REGNUM)
    regcache->raw_collect (ARC_R30_REGNUM, buf);
  if (regnum == -1 || regnum == ARC_R58_REGNUM)
    regcache->raw_collect (ARC_R58_REGNUM, buf + REGOFF (1));
  if (regnum == -1 || regnum == ARC_R59_REGNUM)
    regcache->raw_collect (ARC_R59_REGNUM, buf + REGOFF (2));
}

/* Linux regset definitions.  */

static const struct regset arc_linux_gregset = {
  arc_linux_core_reg_offsets,
  arc_linux_supply_gregset,
  arc_linux_collect_gregset,
};

static const struct regset arc_linux_v2_regset = {
  NULL,
  arc_linux_supply_v2_regset,
  arc_linux_collect_v2_regset,
};

/* Implement the `iterate_over_regset_sections` gdbarch method.  */

static void
arc_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					iterate_over_regset_sections_cb *cb,
					void *cb_data,
					const struct regcache *regcache)
{
  /* There are 40 registers in Linux user_regs_struct, although some of
     them are now just a mere paddings, kept to maintain binary
     compatibility with older tools.  */
  const int sizeof_gregset = 40 * ARC_REGISTER_SIZE;

  cb (".reg", sizeof_gregset, sizeof_gregset, &arc_linux_gregset, NULL,
      cb_data);
  cb (".reg-arc-v2", ARC_LINUX_SIZEOF_V2_REGSET, ARC_LINUX_SIZEOF_V2_REGSET,
      &arc_linux_v2_regset, NULL, cb_data);
}

/* Implement the `core_read_description` gdbarch method.  */

static const struct target_desc *
arc_linux_core_read_description (struct gdbarch *gdbarch,
				 struct target_ops *target,
				 bfd *abfd)
{
  arc_arch_features features
    = arc_arch_features_create (abfd,
				gdbarch_bfd_arch_info (gdbarch)->mach);
  return arc_lookup_target_description (features);
}

/* Initialization specific to Linux environment.  */

static void
arc_linux_init_osabi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  arc_gdbarch_tdep *tdep = gdbarch_tdep<arc_gdbarch_tdep> (gdbarch);

  arc_linux_debug_printf ("GNU/Linux OS/ABI initialization.");

  /* Fill in target-dependent info in ARC-private structure.  */
  tdep->is_sigtramp = arc_linux_is_sigtramp;
  tdep->sigcontext_addr = arc_linux_sigcontext_addr;
  tdep->sc_reg_offset = arc_linux_sc_reg_offsets;
  tdep->sc_num_regs = ARRAY_SIZE (arc_linux_sc_reg_offsets);

  /* If we are using Linux, we have in uClibc
     (libc/sysdeps/linux/arc/bits/setjmp.h):

     typedef int __jmp_buf[13+1+1+1];    //r13-r25, fp, sp, blink

     Where "blink" is a stored PC of a caller function.
   */
  tdep->jb_pc = 15;

  linux_init_abi (info, gdbarch, 0);

  /* Set up target dependent GDB architecture entries.  */
  set_gdbarch_cannot_fetch_register (gdbarch, arc_linux_cannot_fetch_register);
  set_gdbarch_cannot_store_register (gdbarch, arc_linux_cannot_store_register);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       arc_linux_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       arc_linux_sw_breakpoint_from_kind);
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);
  set_gdbarch_software_single_step (gdbarch, arc_linux_software_single_step);
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_gdbarch_skip_solib_resolver (gdbarch, arc_linux_skip_solib_resolver);
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, arc_linux_iterate_over_regset_sections);
  set_gdbarch_core_read_description (gdbarch, arc_linux_core_read_description);

  /* GNU/Linux uses SVR4-style shared libraries, with 32-bit ints, longs
     and pointers (ILP32).  */
  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 linux_ilp32_fetch_link_map_offsets);
}

/* Suppress warning from -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_arc_linux_tdep;

void
_initialize_arc_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_arc, 0, GDB_OSABI_LINUX,
			  arc_linux_init_osabi);
}
