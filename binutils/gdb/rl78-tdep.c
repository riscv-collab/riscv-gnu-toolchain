/* Target-dependent code for the Renesas RL78 for GDB, the GNU debugger.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

   Contributed by Red Hat, Inc.

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
#include "prologue-value.h"
#include "target.h"
#include "regcache.h"
#include "opcode/rl78.h"
#include "dis-asm.h"
#include "gdbtypes.h"
#include "frame.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "value.h"
#include "gdbcore.h"
#include "dwarf2/frame.h"
#include "reggroups.h"
#include "gdbarch.h"
#include "inferior.h"

#include "elf/rl78.h"
#include "elf-bfd.h"

/* Register Banks.  */

enum
{
  RL78_BANK0 = 0,
  RL78_BANK1 = 1,
  RL78_BANK2 = 2,
  RL78_BANK3 = 3,
  RL78_NUMBANKS = 4,
  RL78_REGS_PER_BANK = 8
};

/* Register Numbers.  */

enum
{
  /* All general purpose registers are 8 bits wide.  */
  RL78_RAW_BANK0_R0_REGNUM = 0,
  RL78_RAW_BANK0_R1_REGNUM,
  RL78_RAW_BANK0_R2_REGNUM,
  RL78_RAW_BANK0_R3_REGNUM,
  RL78_RAW_BANK0_R4_REGNUM,
  RL78_RAW_BANK0_R5_REGNUM,
  RL78_RAW_BANK0_R6_REGNUM,
  RL78_RAW_BANK0_R7_REGNUM,

  RL78_RAW_BANK1_R0_REGNUM,
  RL78_RAW_BANK1_R1_REGNUM,
  RL78_RAW_BANK1_R2_REGNUM,
  RL78_RAW_BANK1_R3_REGNUM,
  RL78_RAW_BANK1_R4_REGNUM,
  RL78_RAW_BANK1_R5_REGNUM,
  RL78_RAW_BANK1_R6_REGNUM,
  RL78_RAW_BANK1_R7_REGNUM,

  RL78_RAW_BANK2_R0_REGNUM,
  RL78_RAW_BANK2_R1_REGNUM,
  RL78_RAW_BANK2_R2_REGNUM,
  RL78_RAW_BANK2_R3_REGNUM,
  RL78_RAW_BANK2_R4_REGNUM,
  RL78_RAW_BANK2_R5_REGNUM,
  RL78_RAW_BANK2_R6_REGNUM,
  RL78_RAW_BANK2_R7_REGNUM,

  RL78_RAW_BANK3_R0_REGNUM,
  RL78_RAW_BANK3_R1_REGNUM,
  RL78_RAW_BANK3_R2_REGNUM,
  RL78_RAW_BANK3_R3_REGNUM,
  RL78_RAW_BANK3_R4_REGNUM,
  RL78_RAW_BANK3_R5_REGNUM,
  RL78_RAW_BANK3_R6_REGNUM,
  RL78_RAW_BANK3_R7_REGNUM,

  RL78_PSW_REGNUM,	/* 8 bits */
  RL78_ES_REGNUM,	/* 8 bits */
  RL78_CS_REGNUM,	/* 8 bits */
  RL78_RAW_PC_REGNUM,	/* 20 bits; we'll use 32 bits for it.  */

  /* Fixed address SFRs (some of those above are SFRs too.) */
  RL78_SPL_REGNUM,	/* 8 bits; lower half of SP */
  RL78_SPH_REGNUM,	/* 8 bits; upper half of SP */
  RL78_PMC_REGNUM,	/* 8 bits */
  RL78_MEM_REGNUM,	/* 8 bits ?? */

  RL78_NUM_REGS,

  /* Pseudo registers.  */
  RL78_PC_REGNUM = RL78_NUM_REGS,
  RL78_SP_REGNUM,

  RL78_X_REGNUM,
  RL78_A_REGNUM,
  RL78_C_REGNUM,
  RL78_B_REGNUM,
  RL78_E_REGNUM,
  RL78_D_REGNUM,
  RL78_L_REGNUM,
  RL78_H_REGNUM,

  RL78_AX_REGNUM,
  RL78_BC_REGNUM,
  RL78_DE_REGNUM,
  RL78_HL_REGNUM,

  RL78_BANK0_R0_REGNUM,
  RL78_BANK0_R1_REGNUM,
  RL78_BANK0_R2_REGNUM,
  RL78_BANK0_R3_REGNUM,
  RL78_BANK0_R4_REGNUM,
  RL78_BANK0_R5_REGNUM,
  RL78_BANK0_R6_REGNUM,
  RL78_BANK0_R7_REGNUM,

  RL78_BANK1_R0_REGNUM,
  RL78_BANK1_R1_REGNUM,
  RL78_BANK1_R2_REGNUM,
  RL78_BANK1_R3_REGNUM,
  RL78_BANK1_R4_REGNUM,
  RL78_BANK1_R5_REGNUM,
  RL78_BANK1_R6_REGNUM,
  RL78_BANK1_R7_REGNUM,

  RL78_BANK2_R0_REGNUM,
  RL78_BANK2_R1_REGNUM,
  RL78_BANK2_R2_REGNUM,
  RL78_BANK2_R3_REGNUM,
  RL78_BANK2_R4_REGNUM,
  RL78_BANK2_R5_REGNUM,
  RL78_BANK2_R6_REGNUM,
  RL78_BANK2_R7_REGNUM,

  RL78_BANK3_R0_REGNUM,
  RL78_BANK3_R1_REGNUM,
  RL78_BANK3_R2_REGNUM,
  RL78_BANK3_R3_REGNUM,
  RL78_BANK3_R4_REGNUM,
  RL78_BANK3_R5_REGNUM,
  RL78_BANK3_R6_REGNUM,
  RL78_BANK3_R7_REGNUM,

  RL78_BANK0_RP0_REGNUM,
  RL78_BANK0_RP1_REGNUM,
  RL78_BANK0_RP2_REGNUM,
  RL78_BANK0_RP3_REGNUM,

  RL78_BANK1_RP0_REGNUM,
  RL78_BANK1_RP1_REGNUM,
  RL78_BANK1_RP2_REGNUM,
  RL78_BANK1_RP3_REGNUM,

  RL78_BANK2_RP0_REGNUM,
  RL78_BANK2_RP1_REGNUM,
  RL78_BANK2_RP2_REGNUM,
  RL78_BANK2_RP3_REGNUM,

  RL78_BANK3_RP0_REGNUM,
  RL78_BANK3_RP1_REGNUM,
  RL78_BANK3_RP2_REGNUM,
  RL78_BANK3_RP3_REGNUM,

  /* These are the same as the above 16 registers, but have
     a pointer type for use as base registers in expression
     evaluation.  These are not user visible registers.  */
  RL78_BANK0_RP0_PTR_REGNUM,
  RL78_BANK0_RP1_PTR_REGNUM,
  RL78_BANK0_RP2_PTR_REGNUM,
  RL78_BANK0_RP3_PTR_REGNUM,

  RL78_BANK1_RP0_PTR_REGNUM,
  RL78_BANK1_RP1_PTR_REGNUM,
  RL78_BANK1_RP2_PTR_REGNUM,
  RL78_BANK1_RP3_PTR_REGNUM,

  RL78_BANK2_RP0_PTR_REGNUM,
  RL78_BANK2_RP1_PTR_REGNUM,
  RL78_BANK2_RP2_PTR_REGNUM,
  RL78_BANK2_RP3_PTR_REGNUM,

  RL78_BANK3_RP0_PTR_REGNUM,
  RL78_BANK3_RP1_PTR_REGNUM,
  RL78_BANK3_RP2_PTR_REGNUM,
  RL78_BANK3_RP3_PTR_REGNUM,

  RL78_NUM_TOTAL_REGS,
  RL78_NUM_PSEUDO_REGS = RL78_NUM_TOTAL_REGS - RL78_NUM_REGS
};

#define RL78_SP_ADDR 0xffff8 

/* Architecture specific data.  */

struct rl78_gdbarch_tdep : gdbarch_tdep_base
{
  /* The ELF header flags specify the multilib used.  */
  int elf_flags = 0;

  struct type *rl78_void = nullptr,
	      *rl78_uint8 = nullptr,
	      *rl78_int8 = nullptr,
	      *rl78_uint16 = nullptr,
	      *rl78_int16 = nullptr,
	      *rl78_uint32 = nullptr,
	      *rl78_int32 = nullptr,
	      *rl78_data_pointer = nullptr,
	      *rl78_code_pointer = nullptr,
	      *rl78_psw_type = nullptr;
};

/* This structure holds the results of a prologue analysis.  */

struct rl78_prologue
{
  /* The offset from the frame base to the stack pointer --- always
     zero or negative.

     Calling this a "size" is a bit misleading, but given that the
     stack grows downwards, using offsets for everything keeps one
     from going completely sign-crazy: you never change anything's
     sign for an ADD instruction; always change the second operand's
     sign for a SUB instruction; and everything takes care of
     itself.  */
  int frame_size;

  /* Non-zero if this function has initialized the frame pointer from
     the stack pointer, zero otherwise.  */
  int has_frame_ptr;

  /* If has_frame_ptr is non-zero, this is the offset from the frame
     base to where the frame pointer points.  This is always zero or
     negative.  */
  int frame_ptr_offset;

  /* The address of the first instruction at which the frame has been
     set up and the arguments are where the debug info says they are
     --- as best as we can tell.  */
  CORE_ADDR prologue_end;

  /* reg_offset[R] is the offset from the CFA at which register R is
     saved, or 1 if register R has not been saved.  (Real values are
     always zero or negative.)  */
  int reg_offset[RL78_NUM_TOTAL_REGS];
};

/* Construct type for PSW register.  */

static struct type *
rl78_psw_type (struct gdbarch *gdbarch)
{
  rl78_gdbarch_tdep *tdep = gdbarch_tdep<rl78_gdbarch_tdep> (gdbarch);

  if (tdep->rl78_psw_type == NULL)
    {
      tdep->rl78_psw_type = arch_flags_type (gdbarch,
					     "builtin_type_rl78_psw", 8);
      append_flags_type_flag (tdep->rl78_psw_type, 0, "CY");
      append_flags_type_flag (tdep->rl78_psw_type, 1, "ISP0");
      append_flags_type_flag (tdep->rl78_psw_type, 2, "ISP1");
      append_flags_type_flag (tdep->rl78_psw_type, 3, "RBS0");
      append_flags_type_flag (tdep->rl78_psw_type, 4, "AC");
      append_flags_type_flag (tdep->rl78_psw_type, 5, "RBS1");
      append_flags_type_flag (tdep->rl78_psw_type, 6, "Z");
      append_flags_type_flag (tdep->rl78_psw_type, 7, "IE");
    }

  return tdep->rl78_psw_type;
}

/* Implement the "register_type" gdbarch method.  */

static struct type *
rl78_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  rl78_gdbarch_tdep *tdep = gdbarch_tdep<rl78_gdbarch_tdep> (gdbarch);

  if (reg_nr == RL78_PC_REGNUM)
    return tdep->rl78_code_pointer;
  else if (reg_nr == RL78_RAW_PC_REGNUM)
    return tdep->rl78_uint32;
  else if (reg_nr == RL78_PSW_REGNUM)
    return rl78_psw_type (gdbarch);
  else if (reg_nr <= RL78_MEM_REGNUM
	   || (RL78_X_REGNUM <= reg_nr && reg_nr <= RL78_H_REGNUM)
	   || (RL78_BANK0_R0_REGNUM <= reg_nr
	       && reg_nr <= RL78_BANK3_R7_REGNUM))
    return tdep->rl78_int8;
  else if (reg_nr == RL78_SP_REGNUM
	   || (RL78_BANK0_RP0_PTR_REGNUM <= reg_nr 
	       && reg_nr <= RL78_BANK3_RP3_PTR_REGNUM))
    return tdep->rl78_data_pointer;
  else
    return tdep->rl78_int16;
}

/* Implement the "register_name" gdbarch method.  */

static const char *
rl78_register_name (struct gdbarch *gdbarch, int regnr)
{
  static const char *const reg_names[] =
  {
    "",		/* bank0_r0 */
    "",		/* bank0_r1 */
    "",		/* bank0_r2 */
    "",		/* bank0_r3 */
    "",		/* bank0_r4 */
    "",		/* bank0_r5 */
    "",		/* bank0_r6 */
    "",		/* bank0_r7 */

    "",		/* bank1_r0 */
    "",		/* bank1_r1 */
    "",		/* bank1_r2 */
    "",		/* bank1_r3 */
    "",		/* bank1_r4 */
    "",		/* bank1_r5 */
    "",		/* bank1_r6 */
    "",		/* bank1_r7 */

    "",		/* bank2_r0 */
    "",		/* bank2_r1 */
    "",		/* bank2_r2 */
    "",		/* bank2_r3 */
    "",		/* bank2_r4 */
    "",		/* bank2_r5 */
    "",		/* bank2_r6 */
    "",		/* bank2_r7 */

    "",		/* bank3_r0 */
    "",		/* bank3_r1 */
    "",		/* bank3_r2 */
    "",		/* bank3_r3 */
    "",		/* bank3_r4 */
    "",		/* bank3_r5 */
    "",		/* bank3_r6 */
    "",		/* bank3_r7 */

    "psw",
    "es",
    "cs",
    "",

    "",		/* spl */
    "",		/* sph */
    "pmc",
    "mem",

    "pc",
    "sp",

    "x",
    "a",
    "c",
    "b",
    "e",
    "d",
    "l",
    "h",

    "ax",
    "bc",
    "de",
    "hl",

    "bank0_r0",
    "bank0_r1",
    "bank0_r2",
    "bank0_r3",
    "bank0_r4",
    "bank0_r5",
    "bank0_r6",
    "bank0_r7",

    "bank1_r0",
    "bank1_r1",
    "bank1_r2",
    "bank1_r3",
    "bank1_r4",
    "bank1_r5",
    "bank1_r6",
    "bank1_r7",

    "bank2_r0",
    "bank2_r1",
    "bank2_r2",
    "bank2_r3",
    "bank2_r4",
    "bank2_r5",
    "bank2_r6",
    "bank2_r7",

    "bank3_r0",
    "bank3_r1",
    "bank3_r2",
    "bank3_r3",
    "bank3_r4",
    "bank3_r5",
    "bank3_r6",
    "bank3_r7",

    "bank0_rp0",
    "bank0_rp1",
    "bank0_rp2",
    "bank0_rp3",

    "bank1_rp0",
    "bank1_rp1",
    "bank1_rp2",
    "bank1_rp3",

    "bank2_rp0",
    "bank2_rp1",
    "bank2_rp2",
    "bank2_rp3",

    "bank3_rp0",
    "bank3_rp1",
    "bank3_rp2",
    "bank3_rp3",

    /* The 16 register slots would be named
       bank0_rp0_ptr_regnum ... bank3_rp3_ptr_regnum, but we don't
       want these to be user visible registers.  */
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
  };

  return reg_names[regnr];
}

/* Implement the "register_name" gdbarch method for the g10 variant.  */

static const char *
rl78_g10_register_name (struct gdbarch *gdbarch, int regnr)
{
  static const char *const reg_names[] =
  {
    "",		/* bank0_r0 */
    "",		/* bank0_r1 */
    "",		/* bank0_r2 */
    "",		/* bank0_r3 */
    "",		/* bank0_r4 */
    "",		/* bank0_r5 */
    "",		/* bank0_r6 */
    "",		/* bank0_r7 */

    "",		/* bank1_r0 */
    "",		/* bank1_r1 */
    "",		/* bank1_r2 */
    "",		/* bank1_r3 */
    "",		/* bank1_r4 */
    "",		/* bank1_r5 */
    "",		/* bank1_r6 */
    "",		/* bank1_r7 */

    "",		/* bank2_r0 */
    "",		/* bank2_r1 */
    "",		/* bank2_r2 */
    "",		/* bank2_r3 */
    "",		/* bank2_r4 */
    "",		/* bank2_r5 */
    "",		/* bank2_r6 */
    "",		/* bank2_r7 */

    "",		/* bank3_r0 */
    "",		/* bank3_r1 */
    "",		/* bank3_r2 */
    "",		/* bank3_r3 */
    "",		/* bank3_r4 */
    "",		/* bank3_r5 */
    "",		/* bank3_r6 */
    "",		/* bank3_r7 */

    "psw",
    "es",
    "cs",
    "",

    "",		/* spl */
    "",		/* sph */
    "pmc",
    "mem",

    "pc",
    "sp",

    "x",
    "a",
    "c",
    "b",
    "e",
    "d",
    "l",
    "h",

    "ax",
    "bc",
    "de",
    "hl",

    "bank0_r0",
    "bank0_r1",
    "bank0_r2",
    "bank0_r3",
    "bank0_r4",
    "bank0_r5",
    "bank0_r6",
    "bank0_r7",

    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",

    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",

    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",

    "bank0_rp0",
    "bank0_rp1",
    "bank0_rp2",
    "bank0_rp3",

    "",
    "",
    "",
    "",

    "",
    "",
    "",
    "",

    "",
    "",
    "",
    "",

    /* The 16 register slots would be named
       bank0_rp0_ptr_regnum ... bank3_rp3_ptr_regnum, but we don't
       want these to be user visible registers.  */
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
  };

  return reg_names[regnr];
}

/* Implement the "register_reggroup_p" gdbarch method.  */

static int
rl78_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			  const struct reggroup *group)
{
  if (group == all_reggroup)
    return 1;

  /* All other registers are saved and restored.  */
  if (group == save_reggroup || group == restore_reggroup)
    {
      if ((regnum < RL78_NUM_REGS
	   && regnum != RL78_SPL_REGNUM
	   && regnum != RL78_SPH_REGNUM
	   && regnum != RL78_RAW_PC_REGNUM)
	  || regnum == RL78_SP_REGNUM
	  || regnum == RL78_PC_REGNUM)
	return 1;
      else
	return 0;
    }

  if ((RL78_BANK0_R0_REGNUM <= regnum && regnum <= RL78_BANK3_R7_REGNUM)
      || regnum == RL78_ES_REGNUM
      || regnum == RL78_CS_REGNUM
      || regnum == RL78_SPL_REGNUM
      || regnum == RL78_SPH_REGNUM
      || regnum == RL78_PMC_REGNUM
      || regnum == RL78_MEM_REGNUM
      || regnum == RL78_RAW_PC_REGNUM
      || (RL78_BANK0_RP0_REGNUM <= regnum && regnum <= RL78_BANK3_RP3_REGNUM))
    return group == system_reggroup;

  return group == general_reggroup;
}

/* Strip bits to form an instruction address.  (When fetching a
   32-bit address from the stack, the high eight bits are garbage.
   This function strips off those unused bits.)  */

static CORE_ADDR
rl78_make_instruction_address (CORE_ADDR addr)
{
  return addr & 0xffffff;
}

/* Set / clear bits necessary to make a data address.  */

static CORE_ADDR
rl78_make_data_address (CORE_ADDR addr)
{
  return (addr & 0xffff) | 0xf0000;
}

/* Implement the "pseudo_register_read" gdbarch method.  */

static enum register_status
rl78_pseudo_register_read (struct gdbarch *gdbarch,
			   readable_regcache *regcache,
			   int reg, gdb_byte *buffer)
{
  enum register_status status;

  if (RL78_BANK0_R0_REGNUM <= reg && reg <= RL78_BANK3_R7_REGNUM)
    {
      int raw_regnum = RL78_RAW_BANK0_R0_REGNUM
		       + (reg - RL78_BANK0_R0_REGNUM);

      status = regcache->raw_read (raw_regnum, buffer);
    }
  else if (RL78_BANK0_RP0_REGNUM <= reg && reg <= RL78_BANK3_RP3_REGNUM)
    {
      int raw_regnum = 2 * (reg - RL78_BANK0_RP0_REGNUM)
		       + RL78_RAW_BANK0_R0_REGNUM;

      status = regcache->raw_read (raw_regnum, buffer);
      if (status == REG_VALID)
	status = regcache->raw_read (raw_regnum + 1, buffer + 1);
    }
  else if (RL78_BANK0_RP0_PTR_REGNUM <= reg && reg <= RL78_BANK3_RP3_PTR_REGNUM)
    {
      int raw_regnum = 2 * (reg - RL78_BANK0_RP0_PTR_REGNUM)
		       + RL78_RAW_BANK0_R0_REGNUM;

      status = regcache->raw_read (raw_regnum, buffer);
      if (status == REG_VALID)
	status = regcache->raw_read (raw_regnum + 1, buffer + 1);
    }
  else if (reg == RL78_SP_REGNUM)
    {
      status = regcache->raw_read (RL78_SPL_REGNUM, buffer);
      if (status == REG_VALID)
	status = regcache->raw_read (RL78_SPH_REGNUM, buffer + 1);
    }
  else if (reg == RL78_PC_REGNUM)
    {
      gdb_byte rawbuf[4];

      status = regcache->raw_read (RL78_RAW_PC_REGNUM, rawbuf);
      memcpy (buffer, rawbuf, 3);
    }
  else if (RL78_X_REGNUM <= reg && reg <= RL78_H_REGNUM)
    {
      ULONGEST psw;

      status = regcache->raw_read (RL78_PSW_REGNUM, &psw);
      if (status == REG_VALID)
	{
	  /* RSB0 is at bit 3; RSBS1 is at bit 5.  */
	  int bank = ((psw >> 3) & 1) | ((psw >> 4) & 1);
	  int raw_regnum = RL78_RAW_BANK0_R0_REGNUM + bank * RL78_REGS_PER_BANK
			   + (reg - RL78_X_REGNUM);
	  status = regcache->raw_read (raw_regnum, buffer);
	}
    }
  else if (RL78_AX_REGNUM <= reg && reg <= RL78_HL_REGNUM)
    {
      ULONGEST psw;

      status = regcache->raw_read (RL78_PSW_REGNUM, &psw);
      if (status == REG_VALID)
	{
	  /* RSB0 is at bit 3; RSBS1 is at bit 5.  */
	  int bank = ((psw >> 3) & 1) | ((psw >> 4) & 1);
	  int raw_regnum = RL78_RAW_BANK0_R0_REGNUM + bank * RL78_REGS_PER_BANK
			   + 2 * (reg - RL78_AX_REGNUM);
	  status = regcache->raw_read (raw_regnum, buffer);
	  if (status == REG_VALID)
	    status = regcache->raw_read (raw_regnum + 1, buffer + 1);
	}
    }
  else
    gdb_assert_not_reached ("invalid pseudo register number");
  return status;
}

/* Implement the "pseudo_register_write" gdbarch method.  */

static void
rl78_pseudo_register_write (struct gdbarch *gdbarch,
			    struct regcache *regcache,
			    int reg, const gdb_byte *buffer)
{
  if (RL78_BANK0_R0_REGNUM <= reg && reg <= RL78_BANK3_R7_REGNUM)
    {
      int raw_regnum = RL78_RAW_BANK0_R0_REGNUM
		       + (reg - RL78_BANK0_R0_REGNUM);

      regcache->raw_write (raw_regnum, buffer);
    }
  else if (RL78_BANK0_RP0_REGNUM <= reg && reg <= RL78_BANK3_RP3_REGNUM)
    {
      int raw_regnum = 2 * (reg - RL78_BANK0_RP0_REGNUM)
		       + RL78_RAW_BANK0_R0_REGNUM;

      regcache->raw_write (raw_regnum, buffer);
      regcache->raw_write (raw_regnum + 1, buffer + 1);
    }
  else if (RL78_BANK0_RP0_PTR_REGNUM <= reg && reg <= RL78_BANK3_RP3_PTR_REGNUM)
    {
      int raw_regnum = 2 * (reg - RL78_BANK0_RP0_PTR_REGNUM)
		       + RL78_RAW_BANK0_R0_REGNUM;

      regcache->raw_write (raw_regnum, buffer);
      regcache->raw_write (raw_regnum + 1, buffer + 1);
    }
  else if (reg == RL78_SP_REGNUM)
    {
      regcache->raw_write (RL78_SPL_REGNUM, buffer);
      regcache->raw_write (RL78_SPH_REGNUM, buffer + 1);
    }
  else if (reg == RL78_PC_REGNUM)
    {
      gdb_byte rawbuf[4];

      memcpy (rawbuf, buffer, 3);
      rawbuf[3] = 0;
      regcache->raw_write (RL78_RAW_PC_REGNUM, rawbuf);
    }
  else if (RL78_X_REGNUM <= reg && reg <= RL78_H_REGNUM)
    {
      ULONGEST psw;
      int bank;
      int raw_regnum;

      regcache_raw_read_unsigned (regcache, RL78_PSW_REGNUM, &psw);
      bank = ((psw >> 3) & 1) | ((psw >> 4) & 1);
      /* RSB0 is at bit 3; RSBS1 is at bit 5.  */
      raw_regnum = RL78_RAW_BANK0_R0_REGNUM + bank * RL78_REGS_PER_BANK
		   + (reg - RL78_X_REGNUM);
      regcache->raw_write (raw_regnum, buffer);
    }
  else if (RL78_AX_REGNUM <= reg && reg <= RL78_HL_REGNUM)
    {
      ULONGEST psw;
      int bank, raw_regnum;

      regcache_raw_read_unsigned (regcache, RL78_PSW_REGNUM, &psw);
      bank = ((psw >> 3) & 1) | ((psw >> 4) & 1);
      /* RSB0 is at bit 3; RSBS1 is at bit 5.  */
      raw_regnum = RL78_RAW_BANK0_R0_REGNUM + bank * RL78_REGS_PER_BANK
		   + 2 * (reg - RL78_AX_REGNUM);
      regcache->raw_write (raw_regnum, buffer);
      regcache->raw_write (raw_regnum + 1, buffer + 1);
    }
  else
    gdb_assert_not_reached ("invalid pseudo register number");
}

/* The documented BRK instruction is actually a two byte sequence,
   {0x61, 0xcc}, but instructions may be as short as one byte.
   Correspondence with Renesas revealed that the one byte sequence
   0xff is used when a one byte breakpoint instruction is required.  */
constexpr gdb_byte rl78_break_insn[] = { 0xff };

typedef BP_MANIPULATION (rl78_break_insn) rl78_breakpoint;

/* Define a "handle" struct for fetching the next opcode.  */

struct rl78_get_opcode_byte_handle
{
  CORE_ADDR pc;
};

static int
opc_reg_to_gdb_regnum (int opcreg)
{
  switch (opcreg)
    {
      case RL78_Reg_X:
	return RL78_X_REGNUM;
      case RL78_Reg_A:
	return RL78_A_REGNUM;
      case RL78_Reg_C:
	return RL78_C_REGNUM;
      case RL78_Reg_B:
	return RL78_B_REGNUM;
      case RL78_Reg_E:
	return RL78_E_REGNUM;
      case RL78_Reg_D:
	return RL78_D_REGNUM;
      case RL78_Reg_L:
	return RL78_L_REGNUM;
      case RL78_Reg_H:
	return RL78_H_REGNUM;
      case RL78_Reg_AX:
	return RL78_AX_REGNUM;
      case RL78_Reg_BC:
	return RL78_BC_REGNUM;
      case RL78_Reg_DE:
	return RL78_DE_REGNUM;
      case RL78_Reg_HL:
	return RL78_HL_REGNUM;
      case RL78_Reg_SP:
	return RL78_SP_REGNUM;
      case RL78_Reg_PSW:
	return RL78_PSW_REGNUM;
      case RL78_Reg_CS:
	return RL78_CS_REGNUM;
      case RL78_Reg_ES:
	return RL78_ES_REGNUM;
      case RL78_Reg_PMC:
	return RL78_PMC_REGNUM;
      case RL78_Reg_MEM:
	return RL78_MEM_REGNUM;
      default:
	internal_error (_("Undefined mapping for opc reg %d"),
			opcreg);
    }

  /* Not reached.  */
  return 0;
}

/* Fetch a byte on behalf of the opcode decoder.  HANDLE contains
   the memory address of the next byte to fetch.  If successful,
   the address in the handle is updated and the byte fetched is
   returned as the value of the function.  If not successful, -1
   is returned.  */

static int
rl78_get_opcode_byte (void *handle)
{
  struct rl78_get_opcode_byte_handle *opcdata
    = (struct rl78_get_opcode_byte_handle *) handle;
  int status;
  gdb_byte byte;

  status = target_read_memory (opcdata->pc, &byte, 1);
  if (status == 0)
    {
      opcdata->pc += 1;
      return byte;
    }
  else
    return -1;
}

/* Function for finding saved registers in a 'struct pv_area'; this
   function is passed to pv_area::scan.

   If VALUE is a saved register, ADDR says it was saved at a constant
   offset from the frame base, and SIZE indicates that the whole
   register was saved, record its offset.  */

static void
check_for_saved (void *result_untyped, pv_t addr, CORE_ADDR size,
		 pv_t value)
{
  struct rl78_prologue *result = (struct rl78_prologue *) result_untyped;

  if (value.kind == pvk_register
      && value.k == 0
      && pv_is_register (addr, RL78_SP_REGNUM)
      && size == register_size (current_inferior ()->arch (), value.reg))
    result->reg_offset[value.reg] = addr.k;
}

/* Analyze a prologue starting at START_PC, going no further than
   LIMIT_PC.  Fill in RESULT as appropriate.  */

static void
rl78_analyze_prologue (CORE_ADDR start_pc,
		       CORE_ADDR limit_pc, struct rl78_prologue *result)
{
  CORE_ADDR pc, next_pc;
  int rn;
  pv_t reg[RL78_NUM_TOTAL_REGS];
  CORE_ADDR after_last_frame_setup_insn = start_pc;
  int bank = 0;

  memset (result, 0, sizeof (*result));

  for (rn = 0; rn < RL78_NUM_TOTAL_REGS; rn++)
    {
      reg[rn] = pv_register (rn, 0);
      result->reg_offset[rn] = 1;
    }

  pv_area stack (RL78_SP_REGNUM,
		 gdbarch_addr_bit (current_inferior ()->arch ()));

  /* The call instruction has saved the return address on the stack.  */
  reg[RL78_SP_REGNUM] = pv_add_constant (reg[RL78_SP_REGNUM], -4);
  stack.store (reg[RL78_SP_REGNUM], 4, reg[RL78_PC_REGNUM]);

  pc = start_pc;
  while (pc < limit_pc)
    {
      int bytes_read;
      struct rl78_get_opcode_byte_handle opcode_handle;
      RL78_Opcode_Decoded opc;

      opcode_handle.pc = pc;
      bytes_read = rl78_decode_opcode (pc, &opc, rl78_get_opcode_byte,
				       &opcode_handle, RL78_ISA_DEFAULT);
      next_pc = pc + bytes_read;

      if (opc.id == RLO_sel)
	{
	  bank = opc.op[1].addend;
	}
      else if (opc.id == RLO_mov
	       && opc.op[0].type == RL78_Operand_PreDec
	       && opc.op[0].reg == RL78_Reg_SP
	       && opc.op[1].type == RL78_Operand_Register)
	{
	  int rsrc = (bank * RL78_REGS_PER_BANK) 
	    + 2 * (opc.op[1].reg - RL78_Reg_AX);

	  reg[RL78_SP_REGNUM] = pv_add_constant (reg[RL78_SP_REGNUM], -1);
	  stack.store (reg[RL78_SP_REGNUM], 1, reg[rsrc]);
	  reg[RL78_SP_REGNUM] = pv_add_constant (reg[RL78_SP_REGNUM], -1);
	  stack.store (reg[RL78_SP_REGNUM], 1, reg[rsrc + 1]);
	  after_last_frame_setup_insn = next_pc;
	}
      else if (opc.id == RLO_sub
	       && opc.op[0].type == RL78_Operand_Register
	       && opc.op[0].reg == RL78_Reg_SP
	       && opc.op[1].type == RL78_Operand_Immediate)
	{
	  int addend = opc.op[1].addend;

	  reg[RL78_SP_REGNUM] = pv_add_constant (reg[RL78_SP_REGNUM],
						 -addend);
	  after_last_frame_setup_insn = next_pc;
	}
      else if (opc.id == RLO_mov
	       && opc.size == RL78_Word
	       && opc.op[0].type == RL78_Operand_Register
	       && opc.op[1].type == RL78_Operand_Indirect
	       && opc.op[1].addend == RL78_SP_ADDR)
	{
	  reg[opc_reg_to_gdb_regnum (opc.op[0].reg)]
	    = reg[RL78_SP_REGNUM];
	}
      else if (opc.id == RLO_sub
	       && opc.size == RL78_Word
	       && opc.op[0].type == RL78_Operand_Register
	       && opc.op[1].type == RL78_Operand_Immediate)
	{
	  int addend = opc.op[1].addend;
	  int regnum = opc_reg_to_gdb_regnum (opc.op[0].reg);

	  reg[regnum] = pv_add_constant (reg[regnum], -addend);
	}
      else if (opc.id == RLO_mov
	       && opc.size == RL78_Word
	       && opc.op[0].type == RL78_Operand_Indirect
	       && opc.op[0].addend == RL78_SP_ADDR
	       && opc.op[1].type == RL78_Operand_Register)
	{
	  reg[RL78_SP_REGNUM]
	    = reg[opc_reg_to_gdb_regnum (opc.op[1].reg)];
	  after_last_frame_setup_insn = next_pc;
	}
      else
	{
	  /* Terminate the prologue scan.  */
	  break;
	}

      pc = next_pc;
    }

  /* Is the frame size (offset, really) a known constant?  */
  if (pv_is_register (reg[RL78_SP_REGNUM], RL78_SP_REGNUM))
    result->frame_size = reg[RL78_SP_REGNUM].k;

  /* Record where all the registers were saved.  */
  stack.scan (check_for_saved, (void *) result);

  result->prologue_end = after_last_frame_setup_insn;
}

/* Implement the "addr_bits_remove" gdbarch method.  */

static CORE_ADDR
rl78_addr_bits_remove (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return addr & 0xffffff;
}

/* Implement the "address_to_pointer" gdbarch method.  */

static void
rl78_address_to_pointer (struct gdbarch *gdbarch,
			 struct type *type, gdb_byte *buf, CORE_ADDR addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  store_unsigned_integer (buf, type->length (), byte_order,
			  addr & 0xffffff);
}

/* Implement the "pointer_to_address" gdbarch method.  */

static CORE_ADDR
rl78_pointer_to_address (struct gdbarch *gdbarch,
			 struct type *type, const gdb_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR addr
    = extract_unsigned_integer (buf, type->length (), byte_order);

  /* Is it a code address?  */
  if (type->target_type ()->code () == TYPE_CODE_FUNC
      || type->target_type ()->code () == TYPE_CODE_METHOD
      || TYPE_CODE_SPACE (type->target_type ())
      || type->length () == 4)
    return rl78_make_instruction_address (addr);
  else
    return rl78_make_data_address (addr);
}

/* Implement the "skip_prologue" gdbarch method.  */

static CORE_ADDR
rl78_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  const char *name;
  CORE_ADDR func_addr, func_end;
  struct rl78_prologue p;

  /* Try to find the extent of the function that contains PC.  */
  if (!find_pc_partial_function (pc, &name, &func_addr, &func_end))
    return pc;

  rl78_analyze_prologue (pc, func_end, &p);
  return p.prologue_end;
}

/* Implement the "unwind_pc" gdbarch method.  */

static CORE_ADDR
rl78_unwind_pc (struct gdbarch *arch, frame_info_ptr next_frame)
{
  return rl78_addr_bits_remove
	   (arch, frame_unwind_register_unsigned (next_frame,
						  RL78_PC_REGNUM));
}

/* Given a frame described by THIS_FRAME, decode the prologue of its
   associated function if there is not cache entry as specified by
   THIS_PROLOGUE_CACHE.  Save the decoded prologue in the cache and
   return that struct as the value of this function.  */

static struct rl78_prologue *
rl78_analyze_frame_prologue (frame_info_ptr this_frame,
			   void **this_prologue_cache)
{
  if (!*this_prologue_cache)
    {
      CORE_ADDR func_start, stop_addr;

      *this_prologue_cache = FRAME_OBSTACK_ZALLOC (struct rl78_prologue);

      func_start = get_frame_func (this_frame);
      stop_addr = get_frame_pc (this_frame);

      /* If we couldn't find any function containing the PC, then
	 just initialize the prologue cache, but don't do anything.  */
      if (!func_start)
	stop_addr = func_start;

      rl78_analyze_prologue (func_start, stop_addr,
			     (struct rl78_prologue *) *this_prologue_cache);
    }

  return (struct rl78_prologue *) *this_prologue_cache;
}

/* Given a frame and a prologue cache, return this frame's base.  */

static CORE_ADDR
rl78_frame_base (frame_info_ptr this_frame, void **this_prologue_cache)
{
  struct rl78_prologue *p
    = rl78_analyze_frame_prologue (this_frame, this_prologue_cache);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, RL78_SP_REGNUM);

  return rl78_make_data_address (sp - p->frame_size);
}

/* Implement the "frame_this_id" method for unwinding frames.  */

static void
rl78_this_id (frame_info_ptr this_frame,
	      void **this_prologue_cache, struct frame_id *this_id)
{
  *this_id = frame_id_build (rl78_frame_base (this_frame,
					      this_prologue_cache),
			     get_frame_func (this_frame));
}

/* Implement the "frame_prev_register" method for unwinding frames.  */

static struct value *
rl78_prev_register (frame_info_ptr this_frame,
		    void **this_prologue_cache, int regnum)
{
  struct rl78_prologue *p
    = rl78_analyze_frame_prologue (this_frame, this_prologue_cache);
  CORE_ADDR frame_base = rl78_frame_base (this_frame, this_prologue_cache);

  if (regnum == RL78_SP_REGNUM)
    return frame_unwind_got_constant (this_frame, regnum, frame_base);

  else if (regnum == RL78_SPL_REGNUM)
    return frame_unwind_got_constant (this_frame, regnum,
				      (frame_base & 0xff));

  else if (regnum == RL78_SPH_REGNUM)
    return frame_unwind_got_constant (this_frame, regnum,
				      ((frame_base >> 8) & 0xff));

  /* If prologue analysis says we saved this register somewhere,
     return a description of the stack slot holding it.  */
  else if (p->reg_offset[regnum] != 1)
    {
      struct value *rv =
	frame_unwind_got_memory (this_frame, regnum,
				 frame_base + p->reg_offset[regnum]);

      if (regnum == RL78_PC_REGNUM)
	{
	  ULONGEST pc = rl78_make_instruction_address (value_as_long (rv));

	  return frame_unwind_got_constant (this_frame, regnum, pc);
	}
      return rv;
    }

  /* Otherwise, presume we haven't changed the value of this
     register, and get it from the next frame.  */
  else
    return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind rl78_unwind =
{
  "rl78 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  rl78_this_id,
  rl78_prev_register,
  NULL,
  default_frame_sniffer
};

/* Implement the "dwarf_reg_to_regnum" gdbarch method.  */

static int
rl78_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  if (0 <= reg && reg <= 31)
    {
      if ((reg & 1) == 0)
	/* Map even registers to their 16-bit counterparts which have a
	   pointer type.  This is usually what is required from the DWARF
	   info.  */
	return (reg >> 1) + RL78_BANK0_RP0_PTR_REGNUM;
      else
	return reg;
    }
  else if (reg == 32)
    return RL78_SP_REGNUM;
  else if (reg == 33)
    return -1;			/* ap */
  else if (reg == 34)
    return RL78_PSW_REGNUM;
  else if (reg == 35)
    return RL78_ES_REGNUM;
  else if (reg == 36)
    return RL78_CS_REGNUM;
  else if (reg == 37)
    return RL78_PC_REGNUM;
  else
    return -1;
}

/* Implement the `register_sim_regno' gdbarch method.  */

static int
rl78_register_sim_regno (struct gdbarch *gdbarch, int regnum)
{
  gdb_assert (regnum < RL78_NUM_REGS);

  /* So long as regnum is in [0, RL78_NUM_REGS), it's valid.  We
     just want to override the default here which disallows register
     numbers which have no names.  */
  return regnum;
}

/* Implement the "return_value" gdbarch method.  */

static enum return_value_convention
rl78_return_value (struct gdbarch *gdbarch,
		   struct value *function,
		   struct type *valtype,
		   struct regcache *regcache,
		   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST valtype_len = valtype->length ();
  rl78_gdbarch_tdep *tdep = gdbarch_tdep<rl78_gdbarch_tdep> (gdbarch);
  int is_g10 = tdep->elf_flags & E_FLAG_RL78_G10;

  if (valtype_len > 8)
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf)
    {
      ULONGEST u;
      int argreg = RL78_RAW_BANK1_R0_REGNUM;
      CORE_ADDR g10_raddr = 0xffec8;
      int offset = 0;

      while (valtype_len > 0)
	{
	  if (is_g10)
	    u = read_memory_integer (g10_raddr, 1,
				     gdbarch_byte_order (gdbarch));
	  else
	    regcache_cooked_read_unsigned (regcache, argreg, &u);
	  store_unsigned_integer (readbuf + offset, 1, byte_order, u);
	  valtype_len -= 1;
	  offset += 1;
	  argreg++;
	  g10_raddr++;
	}
    }

  if (writebuf)
    {
      ULONGEST u;
      int argreg = RL78_RAW_BANK1_R0_REGNUM;
      CORE_ADDR g10_raddr = 0xffec8;
      int offset = 0;

      while (valtype_len > 0)
	{
	  u = extract_unsigned_integer (writebuf + offset, 1, byte_order);
	  if (is_g10) {
	    gdb_byte b = u & 0xff;
	    write_memory (g10_raddr, &b, 1);
	  }
	  else
	    regcache_cooked_write_unsigned (regcache, argreg, u);
	  valtype_len -= 1;
	  offset += 1;
	  argreg++;
	  g10_raddr++;
	}
    }

  return RETURN_VALUE_REGISTER_CONVENTION;
}


/* Implement the "frame_align" gdbarch method.  */

static CORE_ADDR
rl78_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  return rl78_make_data_address (align_down (sp, 2));
}


/* Implement the "dummy_id" gdbarch method.  */

static struct frame_id
rl78_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame)
{
  return
    frame_id_build (rl78_make_data_address
		      (get_frame_register_unsigned
			(this_frame, RL78_SP_REGNUM)),
		    get_frame_pc (this_frame));
}


/* Implement the "push_dummy_call" gdbarch method.  */

static CORE_ADDR
rl78_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		      struct regcache *regcache, CORE_ADDR bp_addr,
		      int nargs, struct value **args, CORE_ADDR sp,
		      function_call_return_method return_method,
		      CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[4];
  int i;

  /* Push arguments in reverse order.  */
  for (i = nargs - 1; i >= 0; i--)
    {
      struct type *value_type = args[i]->enclosing_type ();
      int len = value_type->length ();
      int container_len = (len + 1) & ~1;

      sp -= container_len;
      write_memory (rl78_make_data_address (sp),
		    args[i]->contents_all ().data (), len);
    }

  /* Store struct value address.  */
  if (return_method == return_method_struct)
    {
      store_unsigned_integer (buf, 2, byte_order, struct_addr);
      sp -= 2;
      write_memory (rl78_make_data_address (sp), buf, 2);
    }

  /* Store return address.  */
  sp -= 4;
  store_unsigned_integer (buf, 4, byte_order, bp_addr);
  write_memory (rl78_make_data_address (sp), buf, 4);

  /* Finally, update the stack pointer...  */
  regcache_cooked_write_unsigned (regcache, RL78_SP_REGNUM, sp);

  /* DWARF2/GCC uses the stack address *before* the function call as a
     frame's CFA.  */
  return rl78_make_data_address (sp + 4);
}

/* Allocate and initialize a gdbarch object.  */

static struct gdbarch *
rl78_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  int elf_flags;

  /* Extract the elf_flags if available.  */
  if (info.abfd != NULL
      && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    elf_flags = elf_elfheader (info.abfd)->e_flags;
  else
    elf_flags = 0;


  /* Try to find the architecture in the list of already defined
     architectures.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      rl78_gdbarch_tdep *tdep
	= gdbarch_tdep<rl78_gdbarch_tdep> (arches->gdbarch);

      if (tdep->elf_flags != elf_flags)
	continue;

      return arches->gdbarch;
    }

  /* None found, create a new architecture from the information
     provided.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new rl78_gdbarch_tdep));
  rl78_gdbarch_tdep *tdep = gdbarch_tdep<rl78_gdbarch_tdep> (gdbarch);

  tdep->elf_flags = elf_flags;

  /* Initialize types.  */
  type_allocator alloc (gdbarch);
  tdep->rl78_void = alloc.new_type (TYPE_CODE_VOID, TARGET_CHAR_BIT, "void");
  tdep->rl78_uint8 = init_integer_type (alloc, 8, 1, "uint8_t");
  tdep->rl78_int8 = init_integer_type (alloc, 8, 0, "int8_t");
  tdep->rl78_uint16 = init_integer_type (alloc, 16, 1, "uint16_t");
  tdep->rl78_int16 = init_integer_type (alloc, 16, 0, "int16_t");
  tdep->rl78_uint32 = init_integer_type (alloc, 32, 1, "uint32_t");
  tdep->rl78_int32 = init_integer_type (alloc, 32, 0, "int32_t");

  tdep->rl78_data_pointer
    = init_pointer_type (alloc, 16, "rl78_data_addr_t", tdep->rl78_void);
  tdep->rl78_code_pointer
    = init_pointer_type (alloc, 32, "rl78_code_addr_t", tdep->rl78_void);

  /* Registers.  */
  set_gdbarch_num_regs (gdbarch, RL78_NUM_REGS);
  set_gdbarch_num_pseudo_regs (gdbarch, RL78_NUM_PSEUDO_REGS);
  if (tdep->elf_flags & E_FLAG_RL78_G10)
    set_gdbarch_register_name (gdbarch, rl78_g10_register_name);
  else
    set_gdbarch_register_name (gdbarch, rl78_register_name);
  set_gdbarch_register_type (gdbarch, rl78_register_type);
  set_gdbarch_pc_regnum (gdbarch, RL78_PC_REGNUM);
  set_gdbarch_sp_regnum (gdbarch, RL78_SP_REGNUM);
  set_gdbarch_pseudo_register_read (gdbarch, rl78_pseudo_register_read);
  set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						rl78_pseudo_register_write);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, rl78_dwarf_reg_to_regnum);
  set_gdbarch_register_reggroup_p (gdbarch, rl78_register_reggroup_p);
  set_gdbarch_register_sim_regno (gdbarch, rl78_register_sim_regno);

  /* Data types.  */
  set_gdbarch_char_signed (gdbarch, 0);
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 16);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_ptr_bit (gdbarch, 16);
  set_gdbarch_addr_bit (gdbarch, 32);
  set_gdbarch_dwarf2_addr_size (gdbarch, 4);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_double_bit (gdbarch, 32);
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_double);
  set_gdbarch_pointer_to_address (gdbarch, rl78_pointer_to_address);
  set_gdbarch_address_to_pointer (gdbarch, rl78_address_to_pointer);
  set_gdbarch_addr_bits_remove (gdbarch, rl78_addr_bits_remove);

  /* Breakpoints.  */
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, rl78_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, rl78_breakpoint::bp_from_kind);
  set_gdbarch_decr_pc_after_break (gdbarch, 1);

  /* Frames, prologues, etc.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_skip_prologue (gdbarch, rl78_skip_prologue);
  set_gdbarch_unwind_pc (gdbarch, rl78_unwind_pc);
  set_gdbarch_frame_align (gdbarch, rl78_frame_align);

  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &rl78_unwind);

  /* Dummy frames, return values.  */
  set_gdbarch_dummy_id (gdbarch, rl78_dummy_id);
  set_gdbarch_push_dummy_call (gdbarch, rl78_push_dummy_call);
  set_gdbarch_return_value (gdbarch, rl78_return_value);

  /* Virtual tables.  */
  set_gdbarch_vbit_in_delta (gdbarch, 1);

  return gdbarch;
}

/* Register the above initialization routine.  */

void _initialize_rl78_tdep ();
void
_initialize_rl78_tdep ()
{
  gdbarch_register (bfd_arch_rl78, rl78_gdbarch_init);
}
