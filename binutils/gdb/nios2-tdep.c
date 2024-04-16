/* Target-machine dependent code for Nios II, for GDB.
   Copyright (C) 2012-2024 Free Software Foundation, Inc.
   Contributed by Peter Brookes (pbrookes@altera.com)
   and Andrew Draper (adraper@altera.com).
   Contributed by Mentor Graphics, Inc.

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
#include "frame-base.h"
#include "trad-frame.h"
#include "dwarf2/frame.h"
#include "symtab.h"
#include "inferior.h"
#include "gdbtypes.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "osabi.h"
#include "target.h"
#include "dis-asm.h"
#include "regcache.h"
#include "value.h"
#include "symfile.h"
#include "arch-utils.h"
#include "infcall.h"
#include "regset.h"
#include "target-descriptions.h"

/* To get entry_point_address.  */
#include "objfiles.h"
#include <algorithm>

/* Nios II specific header.  */
#include "nios2-tdep.h"

#include "features/nios2.c"

/* Control debugging information emitted in this file.  */

static bool nios2_debug = false;

/* The following structures are used in the cache for prologue
   analysis; see the reg_value and reg_saved tables in
   struct nios2_unwind_cache, respectively.  */

/* struct reg_value is used to record that a register has reg's initial
   value at the start of a function plus the given constant offset.
   If reg == 0, then the value is just the offset.
   If reg < 0, then the value is unknown.  */

struct reg_value
{
  int reg;
  int offset;
};

/* struct reg_saved is used to record that a register value has been saved at
   basereg + addr, for basereg >= 0.  If basereg < 0, that indicates
   that the register is not known to have been saved.  Note that when
   basereg == NIOS2_Z_REGNUM (that is, r0, which holds value 0),
   addr is an absolute address.  */

struct reg_saved
{
  int basereg;
  CORE_ADDR addr;
};

struct nios2_unwind_cache
{
  /* The frame's base, optionally used by the high-level debug info.  */
  CORE_ADDR base;

  /* The previous frame's inner most stack address.  Used as this
     frame ID's stack_addr.  */
  CORE_ADDR cfa;

  /* The address of the first instruction in this function.  */
  CORE_ADDR pc;

  /* Which register holds the return address for the frame.  */
  int return_regnum;

  /* Table indicating what changes have been made to each register.  */
  struct reg_value reg_value[NIOS2_NUM_REGS];

  /* Table indicating where each register has been saved.  */
  struct reg_saved reg_saved[NIOS2_NUM_REGS];
};


/* This array is a mapping from Dwarf-2 register numbering to GDB's.  */

static int nios2_dwarf2gdb_regno_map[] =
{
  0, 1, 2, 3,
  4, 5, 6, 7,
  8, 9, 10, 11,
  12, 13, 14, 15,
  16, 17, 18, 19,
  20, 21, 22, 23,
  24, 25,
  NIOS2_GP_REGNUM,        /* 26 */
  NIOS2_SP_REGNUM,        /* 27 */
  NIOS2_FP_REGNUM,        /* 28 */
  NIOS2_EA_REGNUM,        /* 29 */
  NIOS2_BA_REGNUM,        /* 30 */
  NIOS2_RA_REGNUM,        /* 31 */
  NIOS2_PC_REGNUM,        /* 32 */
  NIOS2_STATUS_REGNUM,    /* 33 */
  NIOS2_ESTATUS_REGNUM,   /* 34 */
  NIOS2_BSTATUS_REGNUM,   /* 35 */
  NIOS2_IENABLE_REGNUM,   /* 36 */
  NIOS2_IPENDING_REGNUM,  /* 37 */
  NIOS2_CPUID_REGNUM,     /* 38 */
  39, /* CTL6 */          /* 39 */
  NIOS2_EXCEPTION_REGNUM, /* 40 */
  NIOS2_PTEADDR_REGNUM,   /* 41 */
  NIOS2_TLBACC_REGNUM,    /* 42 */
  NIOS2_TLBMISC_REGNUM,   /* 43 */
  NIOS2_ECCINJ_REGNUM,    /* 44 */
  NIOS2_BADADDR_REGNUM,   /* 45 */
  NIOS2_CONFIG_REGNUM,    /* 46 */
  NIOS2_MPUBASE_REGNUM,   /* 47 */
  NIOS2_MPUACC_REGNUM     /* 48 */
};

static_assert (ARRAY_SIZE (nios2_dwarf2gdb_regno_map) == NIOS2_NUM_REGS);

/* Implement the dwarf2_reg_to_regnum gdbarch method.  */

static int
nios2_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int dw_reg)
{
  if (dw_reg < 0 || dw_reg >= NIOS2_NUM_REGS)
    return -1;

  return nios2_dwarf2gdb_regno_map[dw_reg];
}

/* Canonical names for the 49 registers.  */

static const char *const nios2_reg_names[NIOS2_NUM_REGS] =
{
  "zero", "at", "r2", "r3", "r4", "r5", "r6", "r7",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
  "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
  "et", "bt", "gp", "sp", "fp", "ea", "sstatus", "ra",
  "pc",
  "status", "estatus", "bstatus", "ienable",
  "ipending", "cpuid", "ctl6", "exception",
  "pteaddr", "tlbacc", "tlbmisc", "eccinj",
  "badaddr", "config", "mpubase", "mpuacc"
};

/* Implement the register_name gdbarch method.  */

static const char *
nios2_register_name (struct gdbarch *gdbarch, int regno)
{
  /* Use mnemonic aliases for GPRs.  */
  if (regno < NIOS2_NUM_REGS)
    return nios2_reg_names[regno];
  else
    return tdesc_register_name (gdbarch, regno);
}

/* Implement the register_type gdbarch method.  */

static struct type *
nios2_register_type (struct gdbarch *gdbarch, int regno)
{
  /* If the XML description has register information, use that to
     determine the register type.  */
  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return tdesc_register_type (gdbarch, regno);

  if (regno == NIOS2_PC_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;
  else if (regno == NIOS2_SP_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;
  else
    return builtin_type (gdbarch)->builtin_uint32;
}

/* Given a return value in REGCACHE with a type VALTYPE,
   extract and copy its value into VALBUF.  */

static void
nios2_extract_return_value (struct gdbarch *gdbarch, struct type *valtype,
			    struct regcache *regcache, gdb_byte *valbuf)
{
  int len = valtype->length ();

  /* Return values of up to 8 bytes are returned in $r2 $r3.  */
  if (len <= register_size (gdbarch, NIOS2_R2_REGNUM))
    regcache->cooked_read (NIOS2_R2_REGNUM, valbuf);
  else
    {
      gdb_assert (len <= (register_size (gdbarch, NIOS2_R2_REGNUM)
			  + register_size (gdbarch, NIOS2_R3_REGNUM)));
      regcache->cooked_read (NIOS2_R2_REGNUM, valbuf);
      regcache->cooked_read (NIOS2_R3_REGNUM, valbuf + 4);
    }
}

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  */

static void
nios2_store_return_value (struct gdbarch *gdbarch, struct type *valtype,
			  struct regcache *regcache, const gdb_byte *valbuf)
{
  int len = valtype->length ();

  /* Return values of up to 8 bytes are returned in $r2 $r3.  */
  if (len <= register_size (gdbarch, NIOS2_R2_REGNUM))
    regcache->cooked_write (NIOS2_R2_REGNUM, valbuf);
  else
    {
      gdb_assert (len <= (register_size (gdbarch, NIOS2_R2_REGNUM)
			  + register_size (gdbarch, NIOS2_R3_REGNUM)));
      regcache->cooked_write (NIOS2_R2_REGNUM, valbuf);
      regcache->cooked_write (NIOS2_R3_REGNUM, valbuf + 4);
    }
}


/* Set up the default values of the registers.  */

static void
nios2_setup_default (struct nios2_unwind_cache *cache)
{
  int i;

  for (i = 0; i < NIOS2_NUM_REGS; i++)
    {
      /* All registers start off holding their previous values.  */
      cache->reg_value[i].reg    = i;
      cache->reg_value[i].offset = 0;

      /* All registers start off not saved.  */
      cache->reg_saved[i].basereg = -1;
      cache->reg_saved[i].addr    = 0;
    }
}

/* Initialize the unwind cache.  */

static void
nios2_init_cache (struct nios2_unwind_cache *cache, CORE_ADDR pc)
{
  cache->base = 0;
  cache->cfa = 0;
  cache->pc = pc;
  cache->return_regnum = NIOS2_RA_REGNUM;
  nios2_setup_default (cache);
}

/* Read and identify an instruction at PC.  If INSNP is non-null,
   store the instruction word into that location.  Return the opcode
   pointer or NULL if the memory couldn't be read or disassembled.  */

static const struct nios2_opcode *
nios2_fetch_insn (struct gdbarch *gdbarch, CORE_ADDR pc,
		  unsigned int *insnp)
{
  LONGEST memword;
  unsigned long mach = gdbarch_bfd_arch_info (gdbarch)->mach;
  unsigned int insn;

  if (mach == bfd_mach_nios2r2)
    {
      if (!safe_read_memory_integer (pc, NIOS2_OPCODE_SIZE,
				     BFD_ENDIAN_LITTLE, &memword)
	  && !safe_read_memory_integer (pc, NIOS2_CDX_OPCODE_SIZE,
					BFD_ENDIAN_LITTLE, &memword))
	return NULL;
    }
  else if (!safe_read_memory_integer (pc, NIOS2_OPCODE_SIZE,
				      gdbarch_byte_order (gdbarch), &memword))
    return NULL;

  insn = (unsigned int) memword;
  if (insnp)
    *insnp = insn;
  return nios2_find_opcode_hash (insn, mach);
}


/* Match and disassemble an ADD-type instruction, with 3 register operands.
   Returns true on success, and fills in the operand pointers.  */

static int
nios2_match_add (uint32_t insn, const struct nios2_opcode *op,
		 unsigned long mach, int *ra, int *rb, int *rc)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2 && (op->match == MATCH_R1_ADD || op->match == MATCH_R1_MOV))
    {
      *ra = GET_IW_R_A (insn);
      *rb = GET_IW_R_B (insn);
      *rc = GET_IW_R_C (insn);
      return 1;
    }
  else if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_ADD || op->match == MATCH_R2_MOV)
    {
      *ra = GET_IW_F3X6L5_A (insn);
      *rb = GET_IW_F3X6L5_B (insn);
      *rc = GET_IW_F3X6L5_C (insn);
      return 1;
    }
  else if (op->match == MATCH_R2_ADD_N)
    {
      *ra = nios2_r2_reg3_mappings[GET_IW_T3X1_A3 (insn)];
      *rb = nios2_r2_reg3_mappings[GET_IW_T3X1_B3 (insn)];
      *rc = nios2_r2_reg3_mappings[GET_IW_T3X1_C3 (insn)];
      return 1;
    }
  else if (op->match == MATCH_R2_MOV_N)
    {
      *ra = GET_IW_F2_A (insn);
      *rb = 0;
      *rc = GET_IW_F2_B (insn);
      return 1;
    }
  return 0;
}

/* Match and disassemble a SUB-type instruction, with 3 register operands.
   Returns true on success, and fills in the operand pointers.  */

static int
nios2_match_sub (uint32_t insn, const struct nios2_opcode *op,
		 unsigned long mach, int *ra, int *rb, int *rc)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2 && op->match == MATCH_R1_SUB)
    {
      *ra = GET_IW_R_A (insn);
      *rb = GET_IW_R_B (insn);
      *rc = GET_IW_R_C (insn);
      return 1;
    }
  else if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_SUB)
    {
      *ra = GET_IW_F3X6L5_A (insn);
      *rb = GET_IW_F3X6L5_B (insn);
      *rc = GET_IW_F3X6L5_C (insn);
      return 1;
    }
  else if (op->match == MATCH_R2_SUB_N)
    {
      *ra = nios2_r2_reg3_mappings[GET_IW_T3X1_A3 (insn)];
      *rb = nios2_r2_reg3_mappings[GET_IW_T3X1_B3 (insn)];
      *rc = nios2_r2_reg3_mappings[GET_IW_T3X1_C3 (insn)];
      return 1;
    }
  return 0;
}

/* Match and disassemble an ADDI-type instruction, with 2 register operands
   and one immediate operand.
   Returns true on success, and fills in the operand pointers.  */

static int
nios2_match_addi (uint32_t insn, const struct nios2_opcode *op,
		  unsigned long mach, int *ra, int *rb, int *imm)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2 && op->match == MATCH_R1_ADDI)
    {
      *ra = GET_IW_I_A (insn);
      *rb = GET_IW_I_B (insn);
      *imm = (signed) (GET_IW_I_IMM16 (insn) << 16) >> 16;
      return 1;
    }
  else if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_ADDI)
    {
      *ra = GET_IW_F2I16_A (insn);
      *rb = GET_IW_F2I16_B (insn);
      *imm = (signed) (GET_IW_F2I16_IMM16 (insn) << 16) >> 16;
      return 1;
    }
  else if (op->match == MATCH_R2_ADDI_N || op->match == MATCH_R2_SUBI_N)
    {
      *ra = nios2_r2_reg3_mappings[GET_IW_T2X1I3_A3 (insn)];
      *rb = nios2_r2_reg3_mappings[GET_IW_T2X1I3_B3 (insn)];
      *imm = nios2_r2_asi_n_mappings[GET_IW_T2X1I3_IMM3 (insn)];
      if (op->match == MATCH_R2_SUBI_N)
	*imm = - (*imm);
      return 1;
    }
  else if (op->match == MATCH_R2_SPADDI_N)
    {
      *ra = nios2_r2_reg3_mappings[GET_IW_T1I7_A3 (insn)];
      *rb = NIOS2_SP_REGNUM;
      *imm = GET_IW_T1I7_IMM7 (insn) << 2;
      return 1;
    }
  else if (op->match == MATCH_R2_SPINCI_N || op->match == MATCH_R2_SPDECI_N)
    {
      *ra = NIOS2_SP_REGNUM;
      *rb = NIOS2_SP_REGNUM;
      *imm = GET_IW_X1I7_IMM7 (insn) << 2;
      if (op->match == MATCH_R2_SPDECI_N)
	*imm = - (*imm);
      return 1;
    }
  return 0;
}

/* Match and disassemble an ORHI-type instruction, with 2 register operands
   and one unsigned immediate operand.
   Returns true on success, and fills in the operand pointers.  */

static int
nios2_match_orhi (uint32_t insn, const struct nios2_opcode *op,
		  unsigned long mach, int *ra, int *rb, unsigned int *uimm)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2 && op->match == MATCH_R1_ORHI)
    {
      *ra = GET_IW_I_A (insn);
      *rb = GET_IW_I_B (insn);
      *uimm = GET_IW_I_IMM16 (insn);
      return 1;
    }
  else if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_ORHI)
    {
      *ra = GET_IW_F2I16_A (insn);
      *rb = GET_IW_F2I16_B (insn);
      *uimm = GET_IW_F2I16_IMM16 (insn);
      return 1;
    }
  return 0;
}

/* Match and disassemble a STW-type instruction, with 2 register operands
   and one immediate operand.
   Returns true on success, and fills in the operand pointers.  */

static int
nios2_match_stw (uint32_t insn, const struct nios2_opcode *op,
		 unsigned long mach, int *ra, int *rb, int *imm)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2 && (op->match == MATCH_R1_STW || op->match == MATCH_R1_STWIO))
    {
      *ra = GET_IW_I_A (insn);
      *rb = GET_IW_I_B (insn);
      *imm = (signed) (GET_IW_I_IMM16 (insn) << 16) >> 16;
      return 1;
    }
  else if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_STW)
    {
      *ra = GET_IW_F2I16_A (insn);
      *rb = GET_IW_F2I16_B (insn);
      *imm = (signed) (GET_IW_F2I16_IMM16 (insn) << 16) >> 16;
      return 1;
    }
  else if (op->match == MATCH_R2_STWIO)
    {
      *ra = GET_IW_F2X4I12_A (insn);
      *rb = GET_IW_F2X4I12_B (insn);
      *imm = (signed) (GET_IW_F2X4I12_IMM12 (insn) << 20) >> 20;
      return 1;
    }
  else if (op->match == MATCH_R2_STW_N)
    {
      *ra = nios2_r2_reg3_mappings[GET_IW_T2I4_A3 (insn)];
      *rb = nios2_r2_reg3_mappings[GET_IW_T2I4_B3 (insn)];
      *imm = GET_IW_T2I4_IMM4 (insn) << 2;
      return 1;
    }
  else if (op->match == MATCH_R2_STWSP_N)
    {
      *ra = NIOS2_SP_REGNUM;
      *rb = GET_IW_F1I5_B (insn);
      *imm = GET_IW_F1I5_IMM5 (insn) << 2;
      return 1;
    }
  else if (op->match == MATCH_R2_STWZ_N)
    {
      *ra = nios2_r2_reg3_mappings[GET_IW_T1X1I6_A3 (insn)];
      *rb = 0;
      *imm = GET_IW_T1X1I6_IMM6 (insn) << 2;
      return 1;
    }
  return 0;
}

/* Match and disassemble a LDW-type instruction, with 2 register operands
   and one immediate operand.
   Returns true on success, and fills in the operand pointers.  */

static int
nios2_match_ldw (uint32_t insn, const struct nios2_opcode *op,
		 unsigned long mach, int *ra, int *rb, int *imm)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2 && (op->match == MATCH_R1_LDW || op->match == MATCH_R1_LDWIO))
    {
      *ra = GET_IW_I_A (insn);
      *rb = GET_IW_I_B (insn);
      *imm = (signed) (GET_IW_I_IMM16 (insn) << 16) >> 16;
      return 1;
    }
  else if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_LDW)
    {
      *ra = GET_IW_F2I16_A (insn);
      *rb = GET_IW_F2I16_B (insn);
      *imm = (signed) (GET_IW_F2I16_IMM16 (insn) << 16) >> 16;
      return 1;
    }
  else if (op->match == MATCH_R2_LDWIO)
    {
      *ra = GET_IW_F2X4I12_A (insn);
      *rb = GET_IW_F2X4I12_B (insn);
      *imm = (signed) (GET_IW_F2X4I12_IMM12 (insn) << 20) >> 20;
      return 1;
    }
  else if (op->match == MATCH_R2_LDW_N)
    {
      *ra = nios2_r2_reg3_mappings[GET_IW_T2I4_A3 (insn)];
      *rb = nios2_r2_reg3_mappings[GET_IW_T2I4_B3 (insn)];
      *imm = GET_IW_T2I4_IMM4 (insn) << 2;
      return 1;
    }
  else if (op->match == MATCH_R2_LDWSP_N)
    {
      *ra = NIOS2_SP_REGNUM;
      *rb = GET_IW_F1I5_B (insn);
      *imm = GET_IW_F1I5_IMM5 (insn) << 2;
      return 1;
    }
  return 0;
}

/* Match and disassemble a RDCTL instruction, with 2 register operands.
   Returns true on success, and fills in the operand pointers.  */

static int
nios2_match_rdctl (uint32_t insn, const struct nios2_opcode *op,
		   unsigned long mach, int *ra, int *rc)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2 && (op->match == MATCH_R1_RDCTL))
    {
      *ra = GET_IW_R_IMM5 (insn);
      *rc = GET_IW_R_C (insn);
      return 1;
    }
  else if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_RDCTL)
    {
      *ra = GET_IW_F3X6L5_IMM5 (insn);
      *rc = GET_IW_F3X6L5_C (insn);
      return 1;
    }
  return 0;
}

/* Match and disassemble a PUSH.N or STWM instruction.
   Returns true on success, and fills in the operand pointers.  */

static int
nios2_match_stwm (uint32_t insn, const struct nios2_opcode *op,
		  unsigned long mach, unsigned int *reglist,
		  int *ra, int *imm, int *wb, int *id)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_PUSH_N)
    {
      *reglist = 1 << 31;
      if (GET_IW_L5I4X1_FP (insn))
	*reglist |= (1 << 28);
      if (GET_IW_L5I4X1_CS (insn))
	{
	  int val = GET_IW_L5I4X1_REGRANGE (insn);
	  *reglist |= nios2_r2_reg_range_mappings[val];
	}
      *ra = NIOS2_SP_REGNUM;
      *imm = GET_IW_L5I4X1_IMM4 (insn) << 2;
      *wb = 1;
      *id = 0;
      return 1;
    }
  else if (op->match == MATCH_R2_STWM)
    {
      unsigned int rawmask = GET_IW_F1X4L17_REGMASK (insn);
      if (GET_IW_F1X4L17_RS (insn))
	{
	  *reglist = ((rawmask << 14) & 0x00ffc000);
	  if (rawmask & (1 << 10))
	    *reglist |= (1 << 28);
	  if (rawmask & (1 << 11))
	    *reglist |= (1 << 31);
	}
      else
	*reglist = rawmask << 2;
      *ra = GET_IW_F1X4L17_A (insn);
      *imm = 0;
      *wb = GET_IW_F1X4L17_WB (insn);
      *id = GET_IW_F1X4L17_ID (insn);
      return 1;
    }
  return 0;
}

/* Match and disassemble a POP.N or LDWM instruction.
   Returns true on success, and fills in the operand pointers.  */

static int
nios2_match_ldwm (uint32_t insn, const struct nios2_opcode *op,
		  unsigned long mach, unsigned int *reglist,
		  int *ra, int *imm, int *wb, int *id, int *ret)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_POP_N)
    {
      *reglist = 1 << 31;
      if (GET_IW_L5I4X1_FP (insn))
	*reglist |= (1 << 28);
      if (GET_IW_L5I4X1_CS (insn))
	{
	  int val = GET_IW_L5I4X1_REGRANGE (insn);
	  *reglist |= nios2_r2_reg_range_mappings[val];
	}
      *ra = NIOS2_SP_REGNUM;
      *imm = GET_IW_L5I4X1_IMM4 (insn) << 2;
      *wb = 1;
      *id = 1;
      *ret = 1;
      return 1;
    }
  else if (op->match == MATCH_R2_LDWM)
    {
      unsigned int rawmask = GET_IW_F1X4L17_REGMASK (insn);
      if (GET_IW_F1X4L17_RS (insn))
	{
	  *reglist = ((rawmask << 14) & 0x00ffc000);
	  if (rawmask & (1 << 10))
	    *reglist |= (1 << 28);
	  if (rawmask & (1 << 11))
	    *reglist |= (1 << 31);
	}
      else
	*reglist = rawmask << 2;
      *ra = GET_IW_F1X4L17_A (insn);
      *imm = 0;
      *wb = GET_IW_F1X4L17_WB (insn);
      *id = GET_IW_F1X4L17_ID (insn);
      *ret = GET_IW_F1X4L17_PC (insn);
      return 1;
    }
  return 0;
}

/* Match and disassemble a branch instruction, with (potentially)
   2 register operands and one immediate operand.
   Returns true on success, and fills in the operand pointers.  */

enum branch_condition {
  branch_none,
  branch_eq,
  branch_ne,
  branch_ge,
  branch_geu,
  branch_lt,
  branch_ltu
};
  
static int
nios2_match_branch (uint32_t insn, const struct nios2_opcode *op,
		    unsigned long mach, int *ra, int *rb, int *imm,
		    enum branch_condition *cond)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2)
    {
      switch (op->match)
	{
	case MATCH_R1_BR:
	  *cond = branch_none;
	  break;
	case MATCH_R1_BEQ:
	  *cond = branch_eq;
	  break;
	case MATCH_R1_BNE:
	  *cond = branch_ne;
	  break;
	case MATCH_R1_BGE:
	  *cond = branch_ge;
	  break;
	case MATCH_R1_BGEU:
	  *cond = branch_geu;
	  break;
	case MATCH_R1_BLT:
	  *cond = branch_lt;
	  break;
	case MATCH_R1_BLTU:
	  *cond = branch_ltu;
	  break;
	default:
	  return 0;
	}
      *imm = (signed) (GET_IW_I_IMM16 (insn) << 16) >> 16;
      *ra = GET_IW_I_A (insn);
      *rb = GET_IW_I_B (insn);
      return 1;
    }
  else
    {
      switch (op->match)
	{
	case MATCH_R2_BR_N:
	  *cond = branch_none;
	  *ra = NIOS2_Z_REGNUM;
	  *rb = NIOS2_Z_REGNUM;
	  *imm = (signed) ((GET_IW_I10_IMM10 (insn) << 1) << 21) >> 21;
	  return 1;
	case MATCH_R2_BEQZ_N:
	  *cond = branch_eq;
	  *ra = nios2_r2_reg3_mappings[GET_IW_T1I7_A3 (insn)];
	  *rb = NIOS2_Z_REGNUM;
	  *imm = (signed) ((GET_IW_T1I7_IMM7 (insn) << 1) << 24) >> 24;
	  return 1;
	case MATCH_R2_BNEZ_N:
	  *cond = branch_ne;
	  *ra = nios2_r2_reg3_mappings[GET_IW_T1I7_A3 (insn)];
	  *rb = NIOS2_Z_REGNUM;
	  *imm = (signed) ((GET_IW_T1I7_IMM7 (insn) << 1) << 24) >> 24;
	  return 1;
	case MATCH_R2_BR:
	  *cond = branch_none;
	  break;
	case MATCH_R2_BEQ:
	  *cond = branch_eq;
	  break;
	case MATCH_R2_BNE:
	  *cond = branch_ne;
	  break;
	case MATCH_R2_BGE:
	  *cond = branch_ge;
	  break;
	case MATCH_R2_BGEU:
	  *cond = branch_geu;
	  break;
	case MATCH_R2_BLT:
	  *cond = branch_lt;
	  break;
	case MATCH_R2_BLTU:
	  *cond = branch_ltu;
	  break;
	default:
	  return 0;
	}
      *ra = GET_IW_F2I16_A (insn);
      *rb = GET_IW_F2I16_B (insn);
      *imm = (signed) (GET_IW_F2I16_IMM16 (insn) << 16) >> 16;
      return 1;
    }
  return 0;
}

/* Match and disassemble a direct jump instruction, with an
   unsigned operand.  Returns true on success, and fills in the operand
   pointer.  */

static int
nios2_match_jmpi (uint32_t insn, const struct nios2_opcode *op,
		  unsigned long mach, unsigned int *uimm)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2 && op->match == MATCH_R1_JMPI)
    {
      *uimm = GET_IW_J_IMM26 (insn) << 2;
      return 1;
    }
  else if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_JMPI)
    {
      *uimm = GET_IW_L26_IMM26 (insn) << 2;
      return 1;
    }
  return 0;
}

/* Match and disassemble a direct call instruction, with an
   unsigned operand.  Returns true on success, and fills in the operand
   pointer.  */

static int
nios2_match_calli (uint32_t insn, const struct nios2_opcode *op,
		   unsigned long mach, unsigned int *uimm)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2 && op->match == MATCH_R1_CALL)
    {
      *uimm = GET_IW_J_IMM26 (insn) << 2;
      return 1;
    }
  else if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_CALL)
    {
      *uimm = GET_IW_L26_IMM26 (insn) << 2;
      return 1;
    }
  return 0;
}

/* Match and disassemble an indirect jump instruction, with a
   (possibly implicit) register operand.  Returns true on success, and fills
   in the operand pointer.  */

static int
nios2_match_jmpr (uint32_t insn, const struct nios2_opcode *op,
		  unsigned long mach, int *ra)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2)
    switch (op->match)
      {
      case MATCH_R1_JMP:
	*ra = GET_IW_I_A (insn);
	return 1;
      case MATCH_R1_RET:
	*ra = NIOS2_RA_REGNUM;
	return 1;
      case MATCH_R1_ERET:
	*ra = NIOS2_EA_REGNUM;
	return 1;
      case MATCH_R1_BRET:
	*ra = NIOS2_BA_REGNUM;
	return 1;
      default:
	return 0;
      }
  else
    switch (op->match)
      {
      case MATCH_R2_JMP:
	*ra = GET_IW_F2I16_A (insn);
	return 1;
      case MATCH_R2_JMPR_N:
	*ra = GET_IW_F1X1_A (insn);
	return 1;
      case MATCH_R2_RET:
      case MATCH_R2_RET_N:
	*ra = NIOS2_RA_REGNUM;
	return 1;
      case MATCH_R2_ERET:
	*ra = NIOS2_EA_REGNUM;
	return 1;
      case MATCH_R2_BRET:
	*ra = NIOS2_BA_REGNUM;
	return 1;
      default:
	return 0;
      }
  return 0;
}

/* Match and disassemble an indirect call instruction, with a register
   operand.  Returns true on success, and fills in the operand pointer.  */

static int
nios2_match_callr (uint32_t insn, const struct nios2_opcode *op,
		   unsigned long mach, int *ra)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2 && op->match == MATCH_R1_CALLR)
    {
      *ra = GET_IW_I_A (insn);
      return 1;
    }
  else if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_CALLR)
    {
      *ra = GET_IW_F2I16_A (insn);
      return 1;
    }
  else if (op->match == MATCH_R2_CALLR_N)
    {
      *ra = GET_IW_F1X1_A (insn);
      return 1;
    }
  return 0;
}

/* Match and disassemble a break instruction, with an unsigned operand.
   Returns true on success, and fills in the operand pointer.  */

static int
nios2_match_break (uint32_t insn, const struct nios2_opcode *op,
		  unsigned long mach, unsigned int *uimm)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2 && op->match == MATCH_R1_BREAK)
    {
      *uimm = GET_IW_R_IMM5 (insn);
      return 1;
    }
  else if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_BREAK)
    {
      *uimm = GET_IW_F3X6L5_IMM5 (insn);
      return 1;
    }
  else if (op->match == MATCH_R2_BREAK_N)
    {
      *uimm = GET_IW_X2L5_IMM5 (insn);
      return 1;
    }
  return 0;
}

/* Match and disassemble a trap instruction, with an unsigned operand.
   Returns true on success, and fills in the operand pointer.  */

static int
nios2_match_trap (uint32_t insn, const struct nios2_opcode *op,
		  unsigned long mach, unsigned int *uimm)
{
  int is_r2 = (mach == bfd_mach_nios2r2);

  if (!is_r2 && op->match == MATCH_R1_TRAP)
    {
      *uimm = GET_IW_R_IMM5 (insn);
      return 1;
    }
  else if (!is_r2)
    return 0;
  else if (op->match == MATCH_R2_TRAP)
    {
      *uimm = GET_IW_F3X6L5_IMM5 (insn);
      return 1;
    }
  else if (op->match == MATCH_R2_TRAP_N)
    {
      *uimm = GET_IW_X2L5_IMM5 (insn);
      return 1;
    }
  return 0;
}

/* Helper function to identify when we're in a function epilogue;
   that is, the part of the function from the point at which the
   stack adjustments are made, to the return or sibcall.
   Note that we may have several stack adjustment instructions, and
   this function needs to test whether the stack teardown has already
   started before current_pc, not whether it has completed.  */

static int
nios2_in_epilogue_p (struct gdbarch *gdbarch,
		     CORE_ADDR current_pc,
		     CORE_ADDR start_pc)
{
  unsigned long mach = gdbarch_bfd_arch_info (gdbarch)->mach;
  int is_r2 = (mach == bfd_mach_nios2r2);
  /* Maximum number of possibly-epilogue instructions to check.
     Note that this number should not be too large, else we can
     potentially end up iterating through unmapped memory.  */
  int ninsns, max_insns = 5;
  unsigned int insn;
  const struct nios2_opcode *op = NULL;
  unsigned int uimm;
  int imm;
  int wb, id, ret;
  int ra, rb, rc;
  enum branch_condition cond;
  CORE_ADDR pc;

  /* There has to be a previous instruction in the function.  */
  if (current_pc <= start_pc)
    return 0;

  /* Find the previous instruction before current_pc.  For R2, it might
     be either a 16-bit or 32-bit instruction; the only way to know for
     sure is to scan through from the beginning of the function,
     disassembling as we go.  */
  if (is_r2)
    for (pc = start_pc; ; )
      {
	op = nios2_fetch_insn (gdbarch, pc, &insn);
	if (op == NULL)
	  return 0;
	if (pc + op->size < current_pc)
	  pc += op->size;
	else
	  break;
	/* We can skip over insns to a forward branch target.  Since
	   the branch offset is relative to the next instruction,
	   it's correct to do this after incrementing the pc above.  */
	if (nios2_match_branch (insn, op, mach, &ra, &rb, &imm, &cond)
	    && imm > 0
	    && pc + imm < current_pc)
	  pc += imm;
      }
  /* Otherwise just go back to the previous 32-bit insn.  */
  else
    pc = current_pc - NIOS2_OPCODE_SIZE;

  /* Beginning with the previous instruction we just located, check whether
     we are in a sequence of at least one stack adjustment instruction.
     Possible instructions here include:
	 ADDI sp, sp, n
	 ADD sp, sp, rn
	 LDW sp, n(sp)
	 SPINCI.N n
	 LDWSP.N sp, n(sp)
	 LDWM {reglist}, (sp)++, wb */
  for (ninsns = 0; ninsns < max_insns; ninsns++)
    {
      int ok = 0;

      /* Fetch the insn at pc.  */
      op = nios2_fetch_insn (gdbarch, pc, &insn);
      if (op == NULL)
	return 0;
      pc += op->size;

      /* Was it a stack adjustment?  */
      if (nios2_match_addi (insn, op, mach, &ra, &rb, &imm))
	ok = (rb == NIOS2_SP_REGNUM);
      else if (nios2_match_add (insn, op, mach, &ra, &rb, &rc))
	ok = (rc == NIOS2_SP_REGNUM);
      else if (nios2_match_ldw (insn, op, mach, &ra, &rb, &imm))
	ok = (rb == NIOS2_SP_REGNUM);
      else if (nios2_match_ldwm (insn, op, mach, &uimm, &ra,
				 &imm, &wb, &ret, &id))
	ok = (ra == NIOS2_SP_REGNUM && wb && id);
      if (!ok)
	break;
    }

  /* No stack adjustments found.  */
  if (ninsns == 0)
    return 0;

  /* We found more stack adjustments than we expect GCC to be generating.
     Since it looks like a stack unwind might be in progress tell GDB to
     treat it as such.  */
  if (ninsns == max_insns)
    return 1;

  /* The next instruction following the stack adjustments must be a
     return, jump, or unconditional branch, or a CDX pop.n or ldwm
     that does an implicit return.  */
  if (nios2_match_jmpr (insn, op, mach, &ra)
      || nios2_match_jmpi (insn, op, mach, &uimm)
      || (nios2_match_ldwm (insn, op, mach, &uimm, &ra, &imm, &wb, &id, &ret)
	  && ret)
      || (nios2_match_branch (insn, op, mach, &ra, &rb, &imm, &cond)
	  && cond == branch_none))
    return 1;

  return 0;
}

/* Implement the stack_frame_destroyed_p gdbarch method.  */

static int
nios2_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr;

  if (find_pc_partial_function (pc, NULL, &func_addr, NULL))
    return nios2_in_epilogue_p (gdbarch, pc, func_addr);

  return 0;
}

/* Do prologue analysis, returning the PC of the first instruction
   after the function prologue.  Assumes CACHE has already been
   initialized.  THIS_FRAME can be null, in which case we are only
   interested in skipping the prologue.  Otherwise CACHE is filled in
   from the frame information.

   The prologue may consist of the following parts:
     1) Profiling instrumentation.  For non-PIC code it looks like:
	  mov	 r8, ra
	  call	 mcount
	  mov	 ra, r8

     2) A stack adjustment and save of R4-R7 for varargs functions.
	For R2 CDX this is typically handled with a STWM, otherwise
	this is typically merged with item 3.

     3) A stack adjustment and save of the callee-saved registers.
	For R2 CDX these are typically handled with a PUSH.N or STWM,
	otherwise as an explicit SP decrement and individual register
	saves.

	There may also be a stack switch here in an exception handler
	in place of a stack adjustment.  It looks like:
	  movhi  rx, %hiadj(newstack)
	  addhi  rx, rx, %lo(newstack)
	  stw    sp, constant(rx)
	  mov    sp, rx

     4) A frame pointer save, which can be either a MOV or ADDI.

     5) A further stack pointer adjustment.  This is normally included
	adjustment in step 3 unless the total adjustment is too large
	to be done in one step.

     7) A stack overflow check, which can take either of these forms:
	  bgeu   sp, rx, +8
	  trap  3
	or
	  bltu   sp, rx, .Lstack_overflow
	  ...
	.Lstack_overflow:
	  trap  3
	  
	Older versions of GCC emitted "break 3" instead of "trap 3" here,
	so we check for both cases.

	Older GCC versions emitted stack overflow checks after the SP
	adjustments in both steps 3 and 4.  Starting with GCC 6, there is
	at most one overflow check, which is placed before the first
	stack adjustment for R2 CDX and after the first stack adjustment
	otherwise.

    The prologue instructions may be combined or interleaved with other
    instructions.

    To cope with all this variability we decode all the instructions
    from the start of the prologue until we hit an instruction that
    cannot possibly be a prologue instruction, such as a branch, call,
    return, or epilogue instruction.  The prologue is considered to end
    at the last instruction that can definitely be considered a
    prologue instruction.  */

static CORE_ADDR
nios2_analyze_prologue (struct gdbarch *gdbarch, const CORE_ADDR start_pc,
			const CORE_ADDR current_pc,
			struct nios2_unwind_cache *cache,
			frame_info_ptr this_frame)
{
  /* Maximum number of possibly-prologue instructions to check.
     Note that this number should not be too large, else we can
     potentially end up iterating through unmapped memory.  */
  int ninsns, max_insns = 50;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned long mach = gdbarch_bfd_arch_info (gdbarch)->mach;

  /* Does the frame set up the FP register?  */
  int base_reg = 0;

  struct reg_value *value = cache->reg_value;
  struct reg_value temp_value[NIOS2_NUM_REGS];

  /* Save the starting PC so we can correct the pc after running
     through the prolog, using symbol info.  */
  CORE_ADDR pc = start_pc;

  /* Is this an exception handler?  */
  int exception_handler = 0;

  /* What was the original value of SP (or fake original value for
     functions which switch stacks?  */
  CORE_ADDR frame_high;

  /* The last definitely-prologue instruction seen.  */
  CORE_ADDR prologue_end;

  /* Is this the innermost function?  */
  int innermost = (this_frame ? (frame_relative_level (this_frame) == 0) : 1);

  if (nios2_debug)
    gdb_printf (gdb_stdlog,
		"{ nios2_analyze_prologue start=%s, current=%s ",
		paddress (gdbarch, start_pc),
		paddress (gdbarch, current_pc));

  /* Set up the default values of the registers.  */
  nios2_setup_default (cache);

  /* Find the prologue instructions.  */
  prologue_end = start_pc;
  for (ninsns = 0; ninsns < max_insns; ninsns++)
    {
      /* Present instruction.  */
      uint32_t insn;
      const struct nios2_opcode *op;
      int ra, rb, rc, imm;
      unsigned int uimm;
      unsigned int reglist;
      int wb, id, ret;
      enum branch_condition cond;

      if (pc == current_pc)
	{
	  /* When we reach the current PC we must save the current
	     register state (for the backtrace) but keep analysing
	     because there might be more to find out (eg. is this an
	     exception handler).  */
	  memcpy (temp_value, value, sizeof (temp_value));
	  value = temp_value;
	  if (nios2_debug)
	    gdb_printf (gdb_stdlog, "*");
	}

      op = nios2_fetch_insn (gdbarch, pc, &insn);

      /* Unknown opcode?  Stop scanning.  */
      if (op == NULL)
	break;
      pc += op->size;

      if (nios2_debug)
	{
	  if (op->size == 2)
	    gdb_printf (gdb_stdlog, "[%04X]", insn & 0xffff);
	  else
	    gdb_printf (gdb_stdlog, "[%08X]", insn);
	}

      /* The following instructions can appear in the prologue.  */

      if (nios2_match_add (insn, op, mach, &ra, &rb, &rc))
	{
	  /* ADD   rc, ra, rb  (also used for MOV) */
	  if (rc == NIOS2_SP_REGNUM
	      && rb == 0
	      && value[ra].reg == cache->reg_saved[NIOS2_SP_REGNUM].basereg)
	    {
	      /* If the previous value of SP is available somewhere
		 near the new stack pointer value then this is a
		 stack switch.  */

	      /* If any registers were saved on the stack before then
		 we can't backtrace into them now.  */
	      for (int i = 0 ; i < NIOS2_NUM_REGS ; i++)
		{
		  if (cache->reg_saved[i].basereg == NIOS2_SP_REGNUM)
		    cache->reg_saved[i].basereg = -1;
		  if (value[i].reg == NIOS2_SP_REGNUM)
		    value[i].reg = -1;
		}

	      /* Create a fake "high water mark" 4 bytes above where SP
		 was stored and fake up the registers to be consistent
		 with that.  */
	      value[NIOS2_SP_REGNUM].reg = NIOS2_SP_REGNUM;
	      value[NIOS2_SP_REGNUM].offset
		= (value[ra].offset
		   - cache->reg_saved[NIOS2_SP_REGNUM].addr
		   - 4);
	      cache->reg_saved[NIOS2_SP_REGNUM].basereg = NIOS2_SP_REGNUM;
	      cache->reg_saved[NIOS2_SP_REGNUM].addr = -4;
	    }

	  else if (rc == NIOS2_SP_REGNUM && ra == NIOS2_FP_REGNUM)
	    /* This is setting SP from FP.  This only happens in the
	       function epilogue.  */
	    break;

	  else if (rc != 0)
	    {
	      if (value[rb].reg == 0)
		value[rc].reg = value[ra].reg;
	      else if (value[ra].reg == 0)
		value[rc].reg = value[rb].reg;
	      else
		value[rc].reg = -1;
	      value[rc].offset = value[ra].offset + value[rb].offset;
	    }

	  /* The add/move is only considered a prologue instruction
	     if the destination is SP or FP.  */
	  if (rc == NIOS2_SP_REGNUM || rc == NIOS2_FP_REGNUM)
	    prologue_end = pc;
	}
      
      else if (nios2_match_sub (insn, op, mach, &ra, &rb, &rc))
	{
	  /* SUB   rc, ra, rb */
	  if (rc == NIOS2_SP_REGNUM && rb == NIOS2_SP_REGNUM
	      && value[rc].reg != 0)
	    /* If we are decrementing the SP by a non-constant amount,
	       this is alloca, not part of the prologue.  */
	    break;
	  else if (rc != 0)
	    {
	      if (value[rb].reg == 0)
		value[rc].reg = value[ra].reg;
	      else
		value[rc].reg = -1;
	      value[rc].offset = value[ra].offset - value[rb].offset;
	    }
	}

      else if (nios2_match_addi (insn, op, mach, &ra, &rb, &imm))
	{
	  /* ADDI    rb, ra, imm */

	  /* A positive stack adjustment has to be part of the epilogue.  */
	  if (rb == NIOS2_SP_REGNUM
	      && (imm > 0 || value[ra].reg != NIOS2_SP_REGNUM))
	    break;

	  /* Likewise restoring SP from FP.  */
	  else if (rb == NIOS2_SP_REGNUM && ra == NIOS2_FP_REGNUM)
	    break;

	  if (rb != 0)
	    {
	      value[rb].reg    = value[ra].reg;
	      value[rb].offset = value[ra].offset + imm;
	    }

	  /* The add is only considered a prologue instruction
	     if the destination is SP or FP.  */
	  if (rb == NIOS2_SP_REGNUM || rb == NIOS2_FP_REGNUM)
	    prologue_end = pc;
	}

      else if (nios2_match_orhi (insn, op, mach, &ra, &rb, &uimm))
	{
	  /* ORHI  rb, ra, uimm   (also used for MOVHI) */
	  if (rb != 0)
	    {
	      value[rb].reg    = (value[ra].reg == 0) ? 0 : -1;
	      value[rb].offset = value[ra].offset | (uimm << 16);
	    }
	}

      else if (nios2_match_stw (insn, op, mach, &ra, &rb, &imm))
	{
	  /* STW rb, imm(ra) */

	  /* Are we storing the original value of a register to the stack?
	     For exception handlers the value of EA-4 (return
	     address from interrupts etc) is sometimes stored.  */
	  int orig = value[rb].reg;
	  if (orig > 0
	      && (value[rb].offset == 0
		  || (orig == NIOS2_EA_REGNUM && value[rb].offset == -4))
	      && value[ra].reg == NIOS2_SP_REGNUM)
	    {
	      if (pc < current_pc)
		{
		  /* Save off callee saved registers.  */
		  cache->reg_saved[orig].basereg = value[ra].reg;
		  cache->reg_saved[orig].addr = value[ra].offset + imm;
		}
	      
	      prologue_end = pc;
	      
	      if (orig == NIOS2_EA_REGNUM || orig == NIOS2_ESTATUS_REGNUM)
		exception_handler = 1;
	    }
	  else
	    /* Non-stack memory writes cannot appear in the prologue.  */
	    break;
	}

      else if (nios2_match_stwm (insn, op, mach,
				 &reglist, &ra, &imm, &wb, &id))
	{
	  /* PUSH.N {reglist}, adjust
	     or
	     STWM {reglist}, --(SP)[, writeback] */
	  int off = 0;

	  if (ra != NIOS2_SP_REGNUM || id != 0)
	    /* This is a non-stack-push memory write and cannot be
	       part of the prologue.  */
	    break;

	  for (int i = 31; i >= 0; i--)
	    if (reglist & (1 << i))
	      {
		int orig = value[i].reg;
		
		off += 4;
		if (orig > 0 && value[i].offset == 0 && pc < current_pc)
		  {
		    cache->reg_saved[orig].basereg
		      = value[NIOS2_SP_REGNUM].reg;
		    cache->reg_saved[orig].addr
		      = value[NIOS2_SP_REGNUM].offset - off;
		  }
	      }

	  if (wb)
	    value[NIOS2_SP_REGNUM].offset -= off;
	  value[NIOS2_SP_REGNUM].offset -= imm;

	  prologue_end = pc;
	}

      else if (nios2_match_rdctl (insn, op, mach, &ra, &rc))
	{
	  /* RDCTL rC, ctlN
	     This can appear in exception handlers in combination with
	     a subsequent save to the stack frame.  */
	  if (rc != 0)
	    {
	      value[rc].reg    = NIOS2_STATUS_REGNUM + ra;
	      value[rc].offset = 0;
	    }
	}

      else if (nios2_match_calli (insn, op, mach, &uimm))
	{
	  if (value[8].reg == NIOS2_RA_REGNUM
	      && value[8].offset == 0
	      && value[NIOS2_SP_REGNUM].reg == NIOS2_SP_REGNUM
	      && value[NIOS2_SP_REGNUM].offset == 0)
	    {
	      /* A CALL instruction.  This is treated as a call to mcount
		 if ra has been stored into r8 beforehand and if it's
		 before the stack adjust.
		 Note mcount corrupts r2-r3, r9-r15 & ra.  */
	      for (int i = 2 ; i <= 3 ; i++)
		value[i].reg = -1;
	      for (int i = 9 ; i <= 15 ; i++)
		value[i].reg = -1;
	      value[NIOS2_RA_REGNUM].reg = -1;

	      prologue_end = pc;
	    }

	  /* Other calls are not part of the prologue.  */
	  else
	    break;
	}

      else if (nios2_match_branch (insn, op, mach, &ra, &rb, &imm, &cond))
	{
	  /* Branches not involving a stack overflow check aren't part of
	     the prologue.  */
	  if (ra != NIOS2_SP_REGNUM)
	    break;
	  else if (cond == branch_geu)
	    {
	      /* BGEU sp, rx, +8
		 TRAP 3  (or BREAK 3)
		 This instruction sequence is used in stack checking;
		 we can ignore it.  */
	      unsigned int next_insn;
	      const struct nios2_opcode *next_op
		= nios2_fetch_insn (gdbarch, pc, &next_insn);
	      if (next_op != NULL
		  && (nios2_match_trap (next_insn, op, mach, &uimm)
		      || nios2_match_break (next_insn, op, mach, &uimm)))
		pc += next_op->size;
	      else
		break;
	    }
	  else if (cond == branch_ltu)
	    {
	      /* BLTU sp, rx, .Lstackoverflow
		 If the location branched to holds a TRAP or BREAK
		 instruction then this is also stack overflow detection.  */
	      unsigned int next_insn;
	      const struct nios2_opcode *next_op
		= nios2_fetch_insn (gdbarch, pc + imm, &next_insn);
	      if (next_op != NULL
		  && (nios2_match_trap (next_insn, op, mach, &uimm)
		      || nios2_match_break (next_insn, op, mach, &uimm)))
		;
	      else
		break;
	    }
	  else
	    break;
	}

      /* All other calls, jumps, returns, TRAPs, or BREAKs terminate
	 the prologue.  */
      else if (nios2_match_callr (insn, op, mach, &ra)
	       || nios2_match_jmpr (insn, op, mach, &ra)
	       || nios2_match_jmpi (insn, op, mach, &uimm)
	       || (nios2_match_ldwm (insn, op, mach, &reglist, &ra,
				     &imm, &wb, &id, &ret)
		   && ret)
	       || nios2_match_trap (insn, op, mach, &uimm)
	       || nios2_match_break (insn, op, mach, &uimm))
	break;
    }

  /* If THIS_FRAME is NULL, we are being called from skip_prologue
     and are only interested in the PROLOGUE_END value, so just
     return that now and skip over the cache updates, which depend
     on having frame information.  */
  if (this_frame == NULL)
    return prologue_end;

  /* If we are in the function epilogue and have already popped
     registers off the stack in preparation for returning, then we
     want to go back to the original register values.  */
  if (innermost && nios2_in_epilogue_p (gdbarch, current_pc, start_pc))
    nios2_setup_default (cache);

  /* Exception handlers use a different return address register.  */
  if (exception_handler)
    cache->return_regnum = NIOS2_EA_REGNUM;

  if (nios2_debug)
    gdb_printf (gdb_stdlog, "\n-> retreg=%d, ", cache->return_regnum);

  if (cache->reg_value[NIOS2_FP_REGNUM].reg == NIOS2_SP_REGNUM)
    /* If the FP now holds an offset from the CFA then this is a
       normal frame which uses the frame pointer.  */
    base_reg = NIOS2_FP_REGNUM;
  else if (cache->reg_value[NIOS2_SP_REGNUM].reg == NIOS2_SP_REGNUM)
    /* FP doesn't hold an offset from the CFA.  If SP still holds an
       offset from the CFA then we might be in a function which omits
       the frame pointer, or we might be partway through the prologue.
       In both cases we can find the CFA using SP.  */
    base_reg = NIOS2_SP_REGNUM;
  else
    {
      /* Somehow the stack pointer has been corrupted.
	 We can't return.  */
      if (nios2_debug)
	gdb_printf (gdb_stdlog, "<can't reach cfa> }\n");
      return 0;
    }

  if (cache->reg_value[base_reg].offset == 0
      || cache->reg_saved[NIOS2_RA_REGNUM].basereg != NIOS2_SP_REGNUM
      || cache->reg_saved[cache->return_regnum].basereg != NIOS2_SP_REGNUM)
    {
      /* If the frame didn't adjust the stack, didn't save RA or
	 didn't save EA in an exception handler then it must either
	 be a leaf function (doesn't call any other functions) or it
	 can't return.  If it has called another function then it
	 can't be a leaf, so set base == 0 to indicate that we can't
	 backtrace past it.  */

      if (!innermost)
	{
	  /* If it isn't the innermost function then it can't be a
	     leaf, unless it was interrupted.  Check whether RA for
	     this frame is the same as PC.  If so then it probably
	     wasn't interrupted.  */
	  CORE_ADDR ra
	    = get_frame_register_unsigned (this_frame, NIOS2_RA_REGNUM);

	  if (ra == current_pc)
	    {
	      if (nios2_debug)
		gdb_printf
		  (gdb_stdlog,
		   "<noreturn ADJUST %s, r31@r%d+?>, r%d@r%d+?> }\n",
		   paddress (gdbarch, cache->reg_value[base_reg].offset),
		   cache->reg_saved[NIOS2_RA_REGNUM].basereg,
		   cache->return_regnum,
		   cache->reg_saved[cache->return_regnum].basereg);
	      return 0;
	    }
	}
    }

  /* Get the value of whichever register we are using for the
     base.  */
  cache->base = get_frame_register_unsigned (this_frame, base_reg);

  /* What was the value of SP at the start of this function (or just
     after the stack switch).  */
  frame_high = cache->base - cache->reg_value[base_reg].offset;

  /* Adjust all the saved registers such that they contain addresses
     instead of offsets.  */
  for (int i = 0; i < NIOS2_NUM_REGS; i++)
    if (cache->reg_saved[i].basereg == NIOS2_SP_REGNUM)
      {
	cache->reg_saved[i].basereg = NIOS2_Z_REGNUM;
	cache->reg_saved[i].addr += frame_high;
      }

  for (int i = 0; i < NIOS2_NUM_REGS; i++)
    if (cache->reg_saved[i].basereg == NIOS2_GP_REGNUM)
      {
	CORE_ADDR gp = get_frame_register_unsigned (this_frame,
						    NIOS2_GP_REGNUM);

	for ( ; i < NIOS2_NUM_REGS; i++)
	  if (cache->reg_saved[i].basereg == NIOS2_GP_REGNUM)
	    {
	      cache->reg_saved[i].basereg = NIOS2_Z_REGNUM;
	      cache->reg_saved[i].addr += gp;
	    }
      }

  /* Work out what the value of SP was on the first instruction of
     this function.  If we didn't switch stacks then this can be
     trivially computed from the base address.  */
  if (cache->reg_saved[NIOS2_SP_REGNUM].basereg == NIOS2_Z_REGNUM)
    cache->cfa
      = read_memory_unsigned_integer (cache->reg_saved[NIOS2_SP_REGNUM].addr,
				      4, byte_order);
  else
    cache->cfa = frame_high;

  /* Exception handlers restore ESTATUS into STATUS.  */
  if (exception_handler)
    {
      cache->reg_saved[NIOS2_STATUS_REGNUM]
	= cache->reg_saved[NIOS2_ESTATUS_REGNUM];
      cache->reg_saved[NIOS2_ESTATUS_REGNUM].basereg = -1;
    }

  if (nios2_debug)
    gdb_printf (gdb_stdlog, "cfa=%s }\n",
		paddress (gdbarch, cache->cfa));

  return prologue_end;
}

/* Implement the skip_prologue gdbarch hook.  */

static CORE_ADDR
nios2_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  CORE_ADDR func_addr;

  struct nios2_unwind_cache cache;

  /* See if we can determine the end of the prologue via the symbol
     table.  If so, then return either PC, or the PC after the
     prologue, whichever is greater.  */
  if (find_pc_partial_function (start_pc, NULL, &func_addr, NULL))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);

      if (post_prologue_pc != 0)
	return std::max (start_pc, post_prologue_pc);
    }

  /* Prologue analysis does the rest....  */
  nios2_init_cache (&cache, start_pc);
  return nios2_analyze_prologue (gdbarch, start_pc, start_pc, &cache, NULL);
}

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
nios2_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  unsigned long mach = gdbarch_bfd_arch_info (gdbarch)->mach;

  if (mach == bfd_mach_nios2r2)
    {
      unsigned int insn;
      const struct nios2_opcode *op
	= nios2_fetch_insn (gdbarch, *pcptr, &insn);

      if (op && op->size == NIOS2_CDX_OPCODE_SIZE)
	return NIOS2_CDX_OPCODE_SIZE;
      else
	return NIOS2_OPCODE_SIZE;
    }
  else
    return NIOS2_OPCODE_SIZE;
}

/* Implement the sw_breakpoint_from_kind gdbarch method.  */

static const gdb_byte *
nios2_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
/* The Nios II ABI for Linux says: "Userspace programs should not use
   the break instruction and userspace debuggers should not insert
   one." and "Userspace breakpoints are accomplished using the trap
   instruction with immediate operand 31 (all ones)."

   So, we use "trap 31" consistently as the breakpoint on bare-metal
   as well as Linux targets.  */

  /* R2 trap encoding:
     ((0x2d << 26) | (0x1f << 21) | (0x1d << 16) | (0x20 << 0))
     0xb7fd0020
     CDX trap.n encoding:
     ((0xd << 12) | (0x1f << 6) | (0x9 << 0))
     0xd7c9
     Note that code is always little-endian on R2.  */
  *size = kind;

  if (kind == NIOS2_CDX_OPCODE_SIZE)
    {
      static const gdb_byte cdx_breakpoint_le[] = {0xc9, 0xd7};

      return cdx_breakpoint_le;
    }
  else
    {
      unsigned long mach = gdbarch_bfd_arch_info (gdbarch)->mach;

      if (mach == bfd_mach_nios2r2)
	{
	  static const gdb_byte r2_breakpoint_le[] = {0x20, 0x00, 0xfd, 0xb7};

	  return r2_breakpoint_le;
	}
      else
	{
	  enum bfd_endian byte_order_for_code
	    = gdbarch_byte_order_for_code (gdbarch);
	  /* R1 trap encoding:
	     ((0x1d << 17) | (0x2d << 11) | (0x1f << 6) | (0x3a << 0))
	     0x003b6ffa */
	  static const gdb_byte r1_breakpoint_le[] = {0xfa, 0x6f, 0x3b, 0x0};
	  static const gdb_byte r1_breakpoint_be[] = {0x0, 0x3b, 0x6f, 0xfa};

	  if (byte_order_for_code == BFD_ENDIAN_BIG)
	    return r1_breakpoint_be;
	  else
	    return r1_breakpoint_le;
	}
    }
}

/* Implement the frame_align gdbarch method.  */

static CORE_ADDR
nios2_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return align_down (addr, 4);
}


/* Implement the return_value gdbarch method.  */

static enum return_value_convention
nios2_return_value (struct gdbarch *gdbarch, struct value *function,
		    struct type *type, struct regcache *regcache,
		    gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (type->length () > 8)
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf)
    nios2_extract_return_value (gdbarch, type, regcache, readbuf);
  if (writebuf)
    nios2_store_return_value (gdbarch, type, regcache, writebuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}

/* Implement the push_dummy_call gdbarch method.  */

static CORE_ADDR
nios2_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		       struct regcache *regcache, CORE_ADDR bp_addr,
		       int nargs, struct value **args, CORE_ADDR sp,
		       function_call_return_method return_method,
		       CORE_ADDR struct_addr)
{
  int argreg;
  int argnum;
  int arg_space = 0;
  int stack_offset = 0;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* Set the return address register to point to the entry point of
     the program, where a breakpoint lies in wait.  */
  regcache_cooked_write_signed (regcache, NIOS2_RA_REGNUM, bp_addr);

  /* Now make space on the stack for the args.  */
  for (argnum = 0; argnum < nargs; argnum++)
    arg_space += align_up (args[argnum]->type ()->length (), 4);
  sp -= arg_space;

  /* Initialize the register pointer.  */
  argreg = NIOS2_FIRST_ARGREG;

  /* The struct_return pointer occupies the first parameter-passing
     register.  */
  if (return_method == return_method_struct)
    regcache_cooked_write_unsigned (regcache, argreg++, struct_addr);

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop through args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      const gdb_byte *val;
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (arg->type ());
      int len = arg_type->length ();

      val = arg->contents ().data ();

      /* Copy the argument to general registers or the stack in
	 register-sized pieces.  Large arguments are split between
	 registers and stack.  */
      while (len > 0)
	{
	  int partial_len = (len < 4 ? len : 4);

	  if (argreg <= NIOS2_LAST_ARGREG)
	    {
	      /* The argument is being passed in a register.  */
	      CORE_ADDR regval = extract_unsigned_integer (val, partial_len,
							   byte_order);

	      regcache_cooked_write_unsigned (regcache, argreg, regval);
	      argreg++;
	    }
	  else
	    {
	      /* The argument is being passed on the stack.  */
	      CORE_ADDR addr = sp + stack_offset;

	      write_memory (addr, val, partial_len);
	      stack_offset += align_up (partial_len, 4);
	    }

	  len -= partial_len;
	  val += partial_len;
	}
    }

  regcache_cooked_write_signed (regcache, NIOS2_SP_REGNUM, sp);

  /* Return adjusted stack pointer.  */
  return sp;
}

/* Implement the unwind_pc gdbarch method.  */

static CORE_ADDR
nios2_unwind_pc (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  gdb_byte buf[4];

  frame_unwind_register (next_frame, NIOS2_PC_REGNUM, buf);
  return extract_typed_address (buf, builtin_type (gdbarch)->builtin_func_ptr);
}

/* Use prologue analysis to fill in the register cache
   *THIS_PROLOGUE_CACHE for THIS_FRAME.  This function initializes
   *THIS_PROLOGUE_CACHE first.  */

static struct nios2_unwind_cache *
nios2_frame_unwind_cache (frame_info_ptr this_frame,
			  void **this_prologue_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR current_pc;
  struct nios2_unwind_cache *cache;

  if (*this_prologue_cache)
    return (struct nios2_unwind_cache *) *this_prologue_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct nios2_unwind_cache);
  *this_prologue_cache = cache;

  /* Zero all fields.  */
  nios2_init_cache (cache, get_frame_func (this_frame));

  /* Prologue analysis does the rest...  */
  current_pc = get_frame_pc (this_frame);
  if (cache->pc != 0)
    nios2_analyze_prologue (gdbarch, cache->pc, current_pc, cache, this_frame);

  return cache;
}

/* Implement the this_id function for the normal unwinder.  */

static void
nios2_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		     struct frame_id *this_id)
{
  struct nios2_unwind_cache *cache =
    nios2_frame_unwind_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  *this_id = frame_id_build (cache->cfa, cache->pc);
}

/* Implement the prev_register function for the normal unwinder.  */

static struct value *
nios2_frame_prev_register (frame_info_ptr this_frame, void **this_cache,
			   int regnum)
{
  struct nios2_unwind_cache *cache =
    nios2_frame_unwind_cache (this_frame, this_cache);

  gdb_assert (regnum >= 0 && regnum < NIOS2_NUM_REGS);

  /* The PC of the previous frame is stored in the RA register of
     the current frame.  Frob regnum so that we pull the value from
     the correct place.  */
  if (regnum == NIOS2_PC_REGNUM)
    regnum = cache->return_regnum;

  if (regnum == NIOS2_SP_REGNUM && cache->cfa)
    return frame_unwind_got_constant (this_frame, regnum, cache->cfa);

  /* If we've worked out where a register is stored then load it from
     there.  */
  if (cache->reg_saved[regnum].basereg == NIOS2_Z_REGNUM)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->reg_saved[regnum].addr);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

/* Implement the this_base, this_locals, and this_args hooks
   for the normal unwinder.  */

static CORE_ADDR
nios2_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct nios2_unwind_cache *info
    = nios2_frame_unwind_cache (this_frame, this_cache);

  return info->base;
}

/* Data structures for the normal prologue-analysis-based
   unwinder.  */

static const struct frame_unwind nios2_frame_unwind =
{
  "nios2 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  nios2_frame_this_id,
  nios2_frame_prev_register,
  NULL,
  default_frame_sniffer
};

static const struct frame_base nios2_frame_base =
{
  &nios2_frame_unwind,
  nios2_frame_base_address,
  nios2_frame_base_address,
  nios2_frame_base_address
};

/* Fill in the register cache *THIS_CACHE for THIS_FRAME for use
   in the stub unwinder.  */

static struct trad_frame_cache *
nios2_stub_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  CORE_ADDR pc;
  CORE_ADDR start_addr;
  CORE_ADDR stack_addr;
  struct trad_frame_cache *this_trad_cache;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);

  if (*this_cache != NULL)
    return (struct trad_frame_cache *) *this_cache;
  this_trad_cache = trad_frame_cache_zalloc (this_frame);
  *this_cache = this_trad_cache;

  /* The return address is in the link register.  */
  trad_frame_set_reg_realreg (this_trad_cache,
			      gdbarch_pc_regnum (gdbarch),
			      NIOS2_RA_REGNUM);

  /* Frame ID, since it's a frameless / stackless function, no stack
     space is allocated and SP on entry is the current SP.  */
  pc = get_frame_pc (this_frame);
  find_pc_partial_function (pc, NULL, &start_addr, NULL);
  stack_addr = get_frame_register_unsigned (this_frame, NIOS2_SP_REGNUM);
  trad_frame_set_id (this_trad_cache, frame_id_build (start_addr, stack_addr));
  /* Assume that the frame's base is the same as the stack pointer.  */
  trad_frame_set_this_base (this_trad_cache, stack_addr);

  return this_trad_cache;
}

/* Implement the this_id function for the stub unwinder.  */

static void
nios2_stub_frame_this_id (frame_info_ptr this_frame, void **this_cache,
			  struct frame_id *this_id)
{
  struct trad_frame_cache *this_trad_cache
    = nios2_stub_frame_cache (this_frame, this_cache);

  trad_frame_get_id (this_trad_cache, this_id);
}

/* Implement the prev_register function for the stub unwinder.  */

static struct value *
nios2_stub_frame_prev_register (frame_info_ptr this_frame,
				void **this_cache, int regnum)
{
  struct trad_frame_cache *this_trad_cache
    = nios2_stub_frame_cache (this_frame, this_cache);

  return trad_frame_get_register (this_trad_cache, this_frame, regnum);
}

/* Implement the sniffer function for the stub unwinder.
   This unwinder is used for cases where the normal
   prologue-analysis-based unwinder can't work,
   such as PLT stubs.  */

static int
nios2_stub_frame_sniffer (const struct frame_unwind *self,
			  frame_info_ptr this_frame, void **cache)
{
  gdb_byte dummy[4];
  CORE_ADDR pc = get_frame_address_in_block (this_frame);

  /* Use the stub unwinder for unreadable code.  */
  if (target_read_memory (get_frame_pc (this_frame), dummy, 4) != 0)
    return 1;

  if (in_plt_section (pc))
    return 1;

  return 0;
}

/* Define the data structures for the stub unwinder.  */

static const struct frame_unwind nios2_stub_frame_unwind =
{
  "nios2 stub",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  nios2_stub_frame_this_id,
  nios2_stub_frame_prev_register,
  NULL,
  nios2_stub_frame_sniffer
};



/* Determine where to set a single step breakpoint while considering
   branch prediction.  */

static CORE_ADDR
nios2_get_next_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();
  nios2_gdbarch_tdep *tdep = gdbarch_tdep<nios2_gdbarch_tdep> (gdbarch);
  unsigned long mach = gdbarch_bfd_arch_info (gdbarch)->mach;
  unsigned int insn;
  const struct nios2_opcode *op = nios2_fetch_insn (gdbarch, pc, &insn);
  int ra;
  int rb;
  int imm;
  unsigned int uimm;
  int wb, id, ret;
  enum branch_condition cond;

  /* Do something stupid if we can't disassemble the insn at pc.  */
  if (op == NULL)
    return pc + NIOS2_OPCODE_SIZE;
    
  if (nios2_match_branch (insn, op, mach, &ra, &rb, &imm, &cond))
    {
      int ras = regcache_raw_get_signed (regcache, ra);
      int rbs = regcache_raw_get_signed (regcache, rb);
      unsigned int rau = regcache_raw_get_unsigned (regcache, ra);
      unsigned int rbu = regcache_raw_get_unsigned (regcache, rb);

      pc += op->size;
      switch (cond)
	{
	case branch_none:
	  pc += imm;
	  break;
	case branch_eq:
	  if (ras == rbs)
	    pc += imm;
	  break;
	case branch_ne:
	  if (ras != rbs)
	    pc += imm;
	  break;
	case branch_ge:
	  if (ras >= rbs)
	    pc += imm;
	  break;
	case branch_geu:
	  if (rau >= rbu)
	    pc += imm;
	  break;
	case branch_lt:
	  if (ras < rbs)
	    pc += imm;
	  break;
	case branch_ltu:
	  if (rau < rbu)
	    pc += imm;
	  break;
	default:
	  break;
	}
    }

  else if (nios2_match_jmpi (insn, op, mach, &uimm))
    pc = (pc & 0xf0000000) | uimm;
  else if (nios2_match_calli (insn, op, mach, &uimm))
    {
      CORE_ADDR callto = (pc & 0xf0000000) | uimm;
      if (tdep->is_kernel_helper != NULL
	  && tdep->is_kernel_helper (callto))
	/* Step over call to kernel helper, which we cannot debug
	   from user space.  */
	pc += op->size;
      else
	pc = callto;
    }

  else if (nios2_match_jmpr (insn, op, mach, &ra))
    pc = regcache_raw_get_unsigned (regcache, ra);
  else if (nios2_match_callr (insn, op, mach, &ra))
    {
      CORE_ADDR callto = regcache_raw_get_unsigned (regcache, ra);
      if (tdep->is_kernel_helper != NULL
	  && tdep->is_kernel_helper (callto))
	/* Step over call to kernel helper.  */
	pc += op->size;
      else
	pc = callto;
    }

  else if (nios2_match_ldwm (insn, op, mach, &uimm, &ra, &imm, &wb, &id, &ret)
	   && ret)
    {
      /* If ra is in the reglist, we have to use the value saved in the
	 stack frame rather than the current value.  */
      if (uimm & (1 << NIOS2_RA_REGNUM))
	pc = nios2_unwind_pc (gdbarch, get_current_frame ());
      else
	pc = regcache_raw_get_unsigned (regcache, NIOS2_RA_REGNUM);
    }

  else if (nios2_match_trap (insn, op, mach, &uimm) && uimm == 0)
    {
      if (tdep->syscall_next_pc != NULL)
	return tdep->syscall_next_pc (get_current_frame (), op);
    }

  else
    pc += op->size;

  return pc;
}

/* Implement the software_single_step gdbarch method.  */

static std::vector<CORE_ADDR>
nios2_software_single_step (struct regcache *regcache)
{
  CORE_ADDR next_pc = nios2_get_next_pc (regcache, regcache_read_pc (regcache));

  return {next_pc};
}

/* Implement the get_longjump_target gdbarch method.  */

static int
nios2_get_longjmp_target (frame_info_ptr frame, CORE_ADDR *pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  nios2_gdbarch_tdep *tdep = gdbarch_tdep<nios2_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR jb_addr = get_frame_register_unsigned (frame, NIOS2_R4_REGNUM);
  gdb_byte buf[4];

  if (target_read_memory (jb_addr + (tdep->jb_pc * 4), buf, 4))
    return 0;

  *pc = extract_unsigned_integer (buf, 4, byte_order);
  return 1;
}

/* Implement the type_align gdbarch function.  */

static ULONGEST
nios2_type_align (struct gdbarch *gdbarch, struct type *type)
{
  switch (type->code ())
    {
    case TYPE_CODE_PTR:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_INT:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_FLT:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_DECFLOAT:
    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_MEMBERPTR:
      type = check_typedef (type);
      return std::min<ULONGEST> (4, type->length ());
    default:
      return 0;
    }
}

/* Implement the gcc_target_options gdbarch method.  */
static std::string
nios2_gcc_target_options (struct gdbarch *gdbarch)
{
  /* GCC doesn't know "-m32".  */
  return {};
}

/* Initialize the Nios II gdbarch.  */

static struct gdbarch *
nios2_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  int i;
  tdesc_arch_data_up tdesc_data;
  const struct target_desc *tdesc = info.target_desc;

  if (!tdesc_has_registers (tdesc))
    /* Pick a default target description.  */
    tdesc = tdesc_nios2;

  /* Check any target description for validity.  */
  if (tdesc_has_registers (tdesc))
    {
      const struct tdesc_feature *feature;
      int valid_p;

      feature = tdesc_find_feature (tdesc, "org.gnu.gdb.nios2.cpu");
      if (feature == NULL)
	return NULL;

      tdesc_data = tdesc_data_alloc ();

      valid_p = 1;
      
      for (i = 0; i < NIOS2_NUM_REGS; i++)
	valid_p &= tdesc_numbered_register (feature, tdesc_data.get (), i,
					    nios2_reg_names[i]);

      if (!valid_p)
	return NULL;
    }

  /* Find a candidate among the list of pre-declared architectures.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* None found, create a new architecture from the information
     provided.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new nios2_gdbarch_tdep));
  nios2_gdbarch_tdep *tdep = gdbarch_tdep<nios2_gdbarch_tdep> (gdbarch);

  /* longjmp support not enabled by default.  */
  tdep->jb_pc = -1;

  /* Data type sizes.  */
  set_gdbarch_ptr_bit (gdbarch, 32);
  set_gdbarch_addr_bit (gdbarch, 32);
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);

  set_gdbarch_type_align (gdbarch, nios2_type_align);

  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_double);

  /* The register set.  */
  set_gdbarch_num_regs (gdbarch, NIOS2_NUM_REGS);
  set_gdbarch_sp_regnum (gdbarch, NIOS2_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, NIOS2_PC_REGNUM);	/* Pseudo register PC */

  set_gdbarch_register_name (gdbarch, nios2_register_name);
  set_gdbarch_register_type (gdbarch, nios2_register_type);

  /* Provide register mappings for stabs and dwarf2.  */
  set_gdbarch_stab_reg_to_regnum (gdbarch, nios2_dwarf_reg_to_regnum);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, nios2_dwarf_reg_to_regnum);

  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  /* Call dummy code.  */
  set_gdbarch_frame_align (gdbarch, nios2_frame_align);

  set_gdbarch_return_value (gdbarch, nios2_return_value);

  set_gdbarch_skip_prologue (gdbarch, nios2_skip_prologue);
  set_gdbarch_stack_frame_destroyed_p (gdbarch, nios2_stack_frame_destroyed_p);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, nios2_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, nios2_sw_breakpoint_from_kind);

  set_gdbarch_unwind_pc (gdbarch, nios2_unwind_pc);

  /* The dwarf2 unwinder will normally produce the best results if
     the debug information is available, so register it first.  */
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &nios2_stub_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &nios2_frame_unwind);

  /* Single stepping.  */
  set_gdbarch_software_single_step (gdbarch, nios2_software_single_step);

  /* Target options for compile.  */
  set_gdbarch_gcc_target_options (gdbarch, nios2_gcc_target_options);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  if (tdep->jb_pc >= 0)
    set_gdbarch_get_longjmp_target (gdbarch, nios2_get_longjmp_target);

  frame_base_set_default (gdbarch, &nios2_frame_base);

  /* Enable inferior call support.  */
  set_gdbarch_push_dummy_call (gdbarch, nios2_push_dummy_call);

  if (tdesc_data != nullptr)
    tdesc_use_registers (gdbarch, tdesc, std::move (tdesc_data));

  return gdbarch;
}

void _initialize_nios2_tdep ();
void
_initialize_nios2_tdep ()
{
  gdbarch_register (bfd_arch_nios2, nios2_gdbarch_init, NULL);
  initialize_tdesc_nios2 ();

  /* Allow debugging this file's internals.  */
  add_setshow_boolean_cmd ("nios2", class_maintenance, &nios2_debug,
			   _("Set Nios II debugging."),
			   _("Show Nios II debugging."),
			   _("When on, Nios II specific debugging is enabled."),
			   NULL,
			   NULL,
			   &setdebuglist, &showdebuglist);
}
