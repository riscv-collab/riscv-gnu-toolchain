/* Common target dependent code for GDB on ARM systems.
   Copyright (C) 1988-2024 Free Software Foundation, Inc.

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

#ifndef ARCH_ARM_H
#define ARCH_ARM_H

#include "gdbsupport/tdesc.h"

/* Prologue helper macros for ARMv8.1-m PACBTI.  */
#define IS_PAC(instruction)	(instruction == 0xf3af801d)
#define IS_PACBTI(instruction)	(instruction == 0xf3af800d)
#define IS_BTI(instruction)	(instruction == 0xf3af800f)
#define IS_PACG(instruction)	((instruction & 0xfff0f0f0) == 0xfb60f000)
#define IS_AUT(instruction)	(instruction == 0xf3af802d)
#define IS_AUTG(instruction)	((instruction & 0xfff00ff0) == 0xfb500f00)

/* DWARF register numbers according to the AADWARF32 document.  */
enum arm_dwarf_regnum {
  ARM_DWARF_RA_AUTH_CODE = 143
};

/* Register numbers of various important registers.  */

enum gdb_regnum {
  ARM_A1_REGNUM = 0,		/* first integer-like argument */
  ARM_A4_REGNUM = 3,		/* last integer-like argument */
  ARM_AP_REGNUM = 11,
  ARM_IP_REGNUM = 12,
  ARM_SP_REGNUM = 13,		/* Contains address of top of stack */
  ARM_LR_REGNUM = 14,		/* address to return to from a function call */
  ARM_PC_REGNUM = 15,		/* Contains program counter */
  /* F0..F7 are the fp registers for the (obsolete) FPA architecture.  */
  ARM_F0_REGNUM = 16,		/* first floating point register */
  ARM_F3_REGNUM = 19,		/* last floating point argument register */
  ARM_F7_REGNUM = 23, 		/* last floating point register */
  ARM_FPS_REGNUM = 24,		/* floating point status register */
  ARM_PS_REGNUM = 25,		/* Contains processor status */
  ARM_WR0_REGNUM,		/* WMMX data registers.  */
  ARM_WR15_REGNUM = ARM_WR0_REGNUM + 15,
  ARM_WC0_REGNUM,		/* WMMX control registers.  */
  ARM_WCSSF_REGNUM = ARM_WC0_REGNUM + 2,
  ARM_WCASF_REGNUM = ARM_WC0_REGNUM + 3,
  ARM_WC7_REGNUM = ARM_WC0_REGNUM + 7,
  ARM_WCGR0_REGNUM,		/* WMMX general purpose registers.  */
  ARM_WCGR3_REGNUM = ARM_WCGR0_REGNUM + 3,
  ARM_WCGR7_REGNUM = ARM_WCGR0_REGNUM + 7,
  ARM_D0_REGNUM,		/* VFP double-precision registers.  */
  ARM_D31_REGNUM = ARM_D0_REGNUM + 31,
  ARM_FPSCR_REGNUM,

  /* Other useful registers.  */
  ARM_FP_REGNUM = 11,		/* Frame register in ARM code, if used.  */
  THUMB_FP_REGNUM = 7,		/* Frame register in Thumb code, if used.  */
  ARM_LAST_ARG_REGNUM = ARM_A4_REGNUM,
  ARM_LAST_FP_ARG_REGNUM = ARM_F3_REGNUM
};

/* Register count constants.  */
enum arm_register_counts {
  /* Number of Q registers for MVE.  */
  ARM_MVE_NUM_Q_REGS = 8,
  /* Number of argument registers.  */
  ARM_NUM_ARG_REGS = 4,
  /* Number of floating point argument registers.  */
  ARM_NUM_FP_ARG_REGS = 4,
  /* Number of registers (old, defined as ARM_FPSCR_REGNUM + 1.  */
  ARM_NUM_REGS = ARM_FPSCR_REGNUM + 1
};

/* Enum describing the different kinds of breakpoints.  */
enum arm_breakpoint_kinds
{
   ARM_BP_KIND_THUMB = 2,
   ARM_BP_KIND_THUMB2 = 3,
   ARM_BP_KIND_ARM = 4,
};

/* Supported Arm FP hardware types.  */
enum arm_fp_type {
   ARM_FP_TYPE_NONE = 0,
   ARM_FP_TYPE_VFPV2,
   ARM_FP_TYPE_VFPV3,
   ARM_FP_TYPE_IWMMXT,
   ARM_FP_TYPE_INVALID
};

/* Supported M-profile Arm types.  */
enum arm_m_profile_type {
   ARM_M_TYPE_M_PROFILE,
   ARM_M_TYPE_VFP_D16,
   ARM_M_TYPE_WITH_FPA,
   ARM_M_TYPE_MVE,
   ARM_M_TYPE_SYSTEM,
   ARM_M_TYPE_INVALID
};

/* System control registers accessible through an addresses.  */
enum system_register_address : CORE_ADDR
{
  /* M-profile Floating-Point Context Control Register address, defined in
     ARMv7-M (Section B3.2.2) and ARMv8-M (Section D1.2.99) reference
     manuals.  */
  FPCCR = 0xe000ef34,

  /* M-profile Floating-Point Context Address Register address, defined in
     ARMv7-M (Section B3.2.2) and ARMv8-M (Section D1.2.98) reference
     manuals.  */
  FPCAR = 0xe000ef38
};

/* Instruction condition field values.  */
#define INST_EQ		0x0
#define INST_NE		0x1
#define INST_CS		0x2
#define INST_CC		0x3
#define INST_MI		0x4
#define INST_PL		0x5
#define INST_VS		0x6
#define INST_VC		0x7
#define INST_HI		0x8
#define INST_LS		0x9
#define INST_GE		0xa
#define INST_LT		0xb
#define INST_GT		0xc
#define INST_LE		0xd
#define INST_AL		0xe
#define INST_NV		0xf

#define FLAG_N		0x80000000
#define FLAG_Z		0x40000000
#define FLAG_C		0x20000000
#define FLAG_V		0x10000000

#define CPSR_T		0x20

#define XPSR_T		0x01000000

/* Size of registers.  */

#define ARM_INT_REGISTER_SIZE		4
/* IEEE extended doubles are 80 bits.  DWORD aligned they use 96 bits.  */
#define ARM_FP_REGISTER_SIZE		12
#define ARM_VFP_REGISTER_SIZE		8
#define IWMMXT_VEC_REGISTER_SIZE	8

/* Size of register sets.  */

/* r0-r12,sp,lr,pc,cpsr.  */
#define ARM_CORE_REGS_SIZE (17 * ARM_INT_REGISTER_SIZE)
/* f0-f8,fps.  */
#define ARM_FP_REGS_SIZE (8 * ARM_FP_REGISTER_SIZE + ARM_INT_REGISTER_SIZE)
/* d0-d15,fpscr.  */
#define ARM_VFP2_REGS_SIZE (16 * ARM_VFP_REGISTER_SIZE + ARM_INT_REGISTER_SIZE)
/* d0-d31,fpscr.  */
#define ARM_VFP3_REGS_SIZE (32 * ARM_VFP_REGISTER_SIZE + ARM_INT_REGISTER_SIZE)
/* wR0-wR15,fpscr.  */
#define IWMMXT_REGS_SIZE (16 * IWMMXT_VEC_REGISTER_SIZE \
			  + 6 * ARM_INT_REGISTER_SIZE)

/* Addresses for calling Thumb functions have the bit 0 set.
   Here are some macros to test, set, or clear bit 0 of addresses.  */
#define IS_THUMB_ADDR(addr)    ((addr) & 1)
#define MAKE_THUMB_ADDR(addr)  ((addr) | 1)
#define UNMAKE_THUMB_ADDR(addr) ((addr) & ~1)

/* Support routines for instruction parsing.  */
#define submask(x) ((1L << ((x) + 1)) - 1)
#define bits(obj,st,fn) (((obj) >> (st)) & submask ((fn) - (st)))
#define bit(obj,st) (((obj) >> (st)) & 1)
#define sbits(obj,st,fn) \
  ((long) (bits(obj,st,fn) | ((long) bit(obj,fn) * ~ submask (fn - st))))
#define BranchDest(addr,instr) \
  ((CORE_ADDR) (((unsigned long) (addr)) + 8 + (sbits (instr, 0, 23) << 2)))

/* Forward declaration.  */
struct reg_buffer_common;

/* Return the size in bytes of the complete Thumb instruction whose
   first halfword is INST1.  */
int thumb_insn_size (unsigned short inst1);

/* Returns true if the condition evaluates to true.  */
int condition_true (unsigned long cond, unsigned long status_reg);

/* Return 1 if THIS_INSTR might change control flow, 0 otherwise.  */
int arm_instruction_changes_pc (uint32_t this_instr);

/* Return 1 if the 16-bit Thumb instruction INST might change
   control flow, 0 otherwise.  */
int thumb_instruction_changes_pc (unsigned short inst);

/* Return 1 if the 32-bit Thumb instruction in INST1 and INST2
   might change control flow, 0 otherwise.  */
int thumb2_instruction_changes_pc (unsigned short inst1, unsigned short inst2);

/* Advance the state of the IT block and return that state.  */
int thumb_advance_itstate (unsigned int itstate);

/* Decode shifted register value.  */

unsigned long shifted_reg_val (reg_buffer_common *regcache,
			       unsigned long inst,
			       int carry,
			       unsigned long pc_val,
			       unsigned long status_reg);

/* Create an Arm target description with the given FP hardware type.  */

target_desc *arm_create_target_description (arm_fp_type fp_type, bool tls);

/* Create an Arm M-profile target description with the given hardware type.  */

target_desc *arm_create_mprofile_target_description (arm_m_profile_type m_type);

#endif /* ARCH_ARM_H */
