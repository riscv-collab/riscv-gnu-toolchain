/* Target dependent code for ARC architecture, for GDB.

   Copyright 2005-2024 Free Software Foundation, Inc.
   Contributed by Synopsys Inc.

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

#ifndef ARC_TDEP_H
#define ARC_TDEP_H

/* Need disassemble_info.  */
#include "dis-asm.h"
#include "gdbarch.h"
#include "arch/arc.h"

/* To simplify GDB code this enum assumes that internal regnums should be same
   as architectural register numbers, i.e. PCL regnum is 63.  This allows to
   use internal GDB regnums as architectural numbers when dealing with
   instruction encodings, for example when analyzing what are the registers
   saved in function prologue.  */

enum arc_regnum
  {
    /* Core registers.  */
    ARC_R0_REGNUM = 0,
    ARC_R1_REGNUM = 1,
    ARC_R4_REGNUM = 4,
    ARC_R7_REGNUM = 7,
    ARC_R9_REGNUM = 9,
    ARC_R13_REGNUM = 13,
    ARC_R16_REGNUM = 16,
    ARC_R25_REGNUM = 25,
    /* Global data pointer.  */
    ARC_GP_REGNUM,
    /* Frame pointer.  */
    ARC_FP_REGNUM,
    /* Stack pointer.  */
    ARC_SP_REGNUM,
    /* Return address from interrupt.  */
    ARC_ILINK_REGNUM,
    ARC_R30_REGNUM,
    /* Return address from function.  */
    ARC_BLINK_REGNUM,
    /* Accumulator registers.  */
    ARC_R58_REGNUM = 58,
    ARC_R59_REGNUM,
    /* Zero-delay loop counter.  */
    ARC_LP_COUNT_REGNUM = 60,
    /* Reserved register number.  There should never be a register with such
       number, this name is needed only for a sanity check in
      arc_cannot_(fetch|store)_register.  */
    ARC_RESERVED_REGNUM,
    /* Long-immediate value.  This is not a physical register - if instruction
       has register 62 as an operand, then this operand is a literal value
       stored in the instruction memory right after the instruction itself.
       This value is required in this enumeration as an architectural number
       for instruction analysis.  */
    ARC_LIMM_REGNUM,
    /* Program counter, aligned to 4-bytes, read-only.  */
    ARC_PCL_REGNUM,
    ARC_LAST_CORE_REGNUM = ARC_PCL_REGNUM,

    /* AUX registers.  */
    /* Actual program counter.  */
    ARC_PC_REGNUM,
    ARC_FIRST_AUX_REGNUM = ARC_PC_REGNUM,
    /* Status register.  */
    ARC_STATUS32_REGNUM,
    /* Zero-delay loop start instruction.  */
    ARC_LP_START_REGNUM,
    /* Zero-delay loop next-after-last instruction.  */
    ARC_LP_END_REGNUM,
    /* Branch target address.  */
    ARC_BTA_REGNUM,
    /* Exception return address.  */
    ARC_ERET_REGNUM,
    ARC_LAST_AUX_REGNUM = ARC_ERET_REGNUM,
    ARC_LAST_REGNUM = ARC_LAST_AUX_REGNUM,

    /* Additional ABI constants.  */
    ARC_FIRST_ARG_REGNUM = ARC_R0_REGNUM,
    ARC_LAST_ARG_REGNUM = ARC_R7_REGNUM,
    ARC_FIRST_CALLEE_SAVED_REGNUM = ARC_R13_REGNUM,
    ARC_LAST_CALLEE_SAVED_REGNUM = ARC_R25_REGNUM,
  };

/* Number of bytes in ARC register.  All ARC registers are considered 32-bit.
   Those registers, which are actually shorter has zero-on-read for extra bits.
   Longer registers are represented as pairs of 32-bit registers.  */
#define ARC_REGISTER_SIZE  4

/* STATUS32 register: hardware loops disabled bit.  */
#define ARC_STATUS32_L_MASK (1 << 12)
/* STATUS32 register: current instruction is a delay slot.  */
#define ARC_STATUS32_DE_MASK (1 << 6)

/* Special value for register offset arrays.  */
#define ARC_OFFSET_NO_REGISTER (-1)

#define arc_print(fmt, args...) gdb_printf (gdb_stdlog, fmt, ##args)

extern bool arc_debug;

/* Print an "arc" debug statement.  */

#define arc_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (arc_debug, "arc", fmt, ##__VA_ARGS__)

/* Target-dependent information.  */

struct arc_gdbarch_tdep : gdbarch_tdep_base
{
  /* Offset to PC value in jump buffer.  If this is negative, longjmp
     support will be disabled.  */
  int jb_pc = 0;

  /* Whether target has hardware (aka zero-delay) loops.  */
  bool has_hw_loops = false;

  /* Detect sigtramp.  */
  bool (*is_sigtramp) (frame_info_ptr) = nullptr;

  /* Get address of sigcontext for sigtramp.  */
  CORE_ADDR (*sigcontext_addr) (frame_info_ptr) = nullptr;

  /* Offset of registers in `struct sigcontext'.  */
  const int *sc_reg_offset = nullptr;

  /* Number of registers in sc_reg_offsets.  Most likely a ARC_LAST_REGNUM,
     but in theory it could be less, so it is kept separate.  */
  int sc_num_regs = 0;
};

/* Utility functions used by other ARC-specific modules.  */

static inline int
arc_mach_is_arc600 (struct gdbarch *gdbarch)
{
  return (gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_arc_arc600
	  || gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_arc_arc601);
}

static inline int
arc_mach_is_arc700 (struct gdbarch *gdbarch)
{
  return gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_arc_arc700;
}

static inline int
arc_mach_is_arcv2 (struct gdbarch *gdbarch)
{
  return gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_arc_arcv2;
}

/* ARC EM and ARC HS are unique BFD arches, however they share the same machine
   number as "ARCv2".  */

static inline bool
arc_arch_is_hs (const struct bfd_arch_info* arch)
{
  return startswith (arch->printable_name, "HS");
}

static inline bool
arc_arch_is_em (const struct bfd_arch_info* arch)
{
  return startswith (arch->printable_name, "EM");
}

/* Function to access ARC disassembler.  Underlying opcodes disassembler will
   print an instruction into stream specified in the INFO, so if it is
   undesired, then this stream should be set to some invisible stream, but it
   can't be set to an actual NULL value - that would cause a crash.  */
int arc_delayed_print_insn (bfd_vma addr, struct disassemble_info *info);

/* Get branch/jump target address for the INSN.  Note that this function
   returns branch target and doesn't evaluate if this branch is taken or not.
   For the indirect jumps value depends in register state, hence can change.
   It is an error to call this function for a non-branch instruction.  */

CORE_ADDR arc_insn_get_branch_target (const struct arc_instruction &insn);

/* Get address of next instruction after INSN, assuming linear execution (no
   taken branches).  If instruction has a delay slot, then returned value will
   point at the instruction in delay slot.  That is - "address of instruction +
   instruction length with LIMM".  */

CORE_ADDR arc_insn_get_linear_next_pc (const struct arc_instruction &insn);

/* Create an arc_arch_features instance from the provided data.  */

arc_arch_features arc_arch_features_create (const bfd *abfd,
					    const unsigned long mach);

#endif /* ARC_TDEP_H */
