/* Target-dependent code for Renesas Super-H, for GDB.

   Copyright (C) 1993-2024 Free Software Foundation, Inc.

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

/* Contributed by Steve Chamberlain
   sac@cygnus.com.  */

#include "defs.h"
#include "frame.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "dwarf2/frame.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "value.h"
#include "dis-asm.h"
#include "inferior.h"
#include "arch-utils.h"
#include "regcache.h"
#include "target-float.h"
#include "osabi.h"
#include "reggroups.h"
#include "regset.h"
#include "objfiles.h"

#include "sh-tdep.h"

#include "elf-bfd.h"
#include "solib-svr4.h"

/* sh flags */
#include "elf/sh.h"
#include "dwarf2.h"
/* registers numbers shared with the simulator.  */
#include "sim/sim-sh.h"
#include <algorithm>

/* List of "set sh ..." and "show sh ..." commands.  */
static struct cmd_list_element *setshcmdlist = NULL;
static struct cmd_list_element *showshcmdlist = NULL;

static const char sh_cc_gcc[] = "gcc";
static const char sh_cc_renesas[] = "renesas";
static const char *const sh_cc_enum[] = {
  sh_cc_gcc,
  sh_cc_renesas, 
  NULL
};

static const char *sh_active_calling_convention = sh_cc_gcc;

#define SH_NUM_REGS 67

struct sh_frame_cache
{
  /* Base address.  */
  CORE_ADDR base;
  LONGEST sp_offset;
  CORE_ADDR pc;

  /* Flag showing that a frame has been created in the prologue code.  */
  int uses_fp;

  /* Saved registers.  */
  CORE_ADDR saved_regs[SH_NUM_REGS];
  CORE_ADDR saved_sp;
};

static int
sh_is_renesas_calling_convention (struct type *func_type)
{
  int val = 0;

  if (func_type)
    {
      func_type = check_typedef (func_type);

      if (func_type->code () == TYPE_CODE_PTR)
	func_type = check_typedef (func_type->target_type ());

      if (func_type->code () == TYPE_CODE_FUNC
	  && TYPE_CALLING_CONVENTION (func_type) == DW_CC_GNU_renesas_sh)
	val = 1;
    }

  if (sh_active_calling_convention == sh_cc_renesas)
    val = 1;

  return val;
}

static const char *
sh_sh_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static const char *register_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "pc", "pr", "gbr", "vbr", "mach", "macl", "sr"
  };

  if (reg_nr >= ARRAY_SIZE (register_names))
    return "";
  return register_names[reg_nr];
}

static const char *
sh_sh3_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static const char *register_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "pc", "pr", "gbr", "vbr", "mach", "macl", "sr",
    "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "ssr", "spc",
    "r0b0", "r1b0", "r2b0", "r3b0", "r4b0", "r5b0", "r6b0", "r7b0",
    "r0b1", "r1b1", "r2b1", "r3b1", "r4b1", "r5b1", "r6b1", "r7b1",
  };

  if (reg_nr >= ARRAY_SIZE (register_names))
    return "";
  return register_names[reg_nr];
}

static const char *
sh_sh3e_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static const char *register_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "pc", "pr", "gbr", "vbr", "mach", "macl", "sr",
    "fpul", "fpscr",
    "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7",
    "fr8", "fr9", "fr10", "fr11", "fr12", "fr13", "fr14", "fr15",
    "ssr", "spc",
    "r0b0", "r1b0", "r2b0", "r3b0", "r4b0", "r5b0", "r6b0", "r7b0",
    "r0b1", "r1b1", "r2b1", "r3b1", "r4b1", "r5b1", "r6b1", "r7b1",
  };
  if (reg_nr >= ARRAY_SIZE (register_names))
    return "";
  return register_names[reg_nr];
}

static const char *
sh_sh2e_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static const char *register_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "pc", "pr", "gbr", "vbr", "mach", "macl", "sr",
    "fpul", "fpscr",
    "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7",
    "fr8", "fr9", "fr10", "fr11", "fr12", "fr13", "fr14", "fr15",
  };
  if (reg_nr >= ARRAY_SIZE (register_names))
    return "";
  return register_names[reg_nr];
}

static const char *
sh_sh2a_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static const char *register_names[] = {
    /* general registers 0-15 */
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    /* 16 - 22 */
    "pc", "pr", "gbr", "vbr", "mach", "macl", "sr",
    /* 23, 24 */
    "fpul", "fpscr",
    /* floating point registers 25 - 40 */
    "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7",
    "fr8", "fr9", "fr10", "fr11", "fr12", "fr13", "fr14", "fr15",
    /* 41, 42 */
    "", "",
    /* 43 - 62.  Banked registers.  The bank number used is determined by
       the bank register (63).  */
    "r0b", "r1b", "r2b", "r3b", "r4b", "r5b", "r6b", "r7b",
    "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b",
    "machb", "ivnb", "prb", "gbrb", "maclb",
    /* 63: register bank number, not a real register but used to
       communicate the register bank currently get/set.  This register
       is hidden to the user, who manipulates it using the pseudo
       register called "bank" (67).  See below.  */
    "",
    /* 64 - 66 */
    "ibcr", "ibnr", "tbr",
    /* 67: register bank number, the user visible pseudo register.  */
    "bank",
    /* double precision (pseudo) 68 - 75 */
    "dr0", "dr2", "dr4", "dr6", "dr8", "dr10", "dr12", "dr14",
  };
  if (reg_nr >= ARRAY_SIZE (register_names))
    return "";
  return register_names[reg_nr];
}

static const char *
sh_sh2a_nofpu_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static const char *register_names[] = {
    /* general registers 0-15 */
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    /* 16 - 22 */
    "pc", "pr", "gbr", "vbr", "mach", "macl", "sr",
    /* 23, 24 */
    "", "",
    /* floating point registers 25 - 40 */
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    /* 41, 42 */
    "", "",
    /* 43 - 62.  Banked registers.  The bank number used is determined by
       the bank register (63).  */
    "r0b", "r1b", "r2b", "r3b", "r4b", "r5b", "r6b", "r7b",
    "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b",
    "machb", "ivnb", "prb", "gbrb", "maclb",
    /* 63: register bank number, not a real register but used to
       communicate the register bank currently get/set.  This register
       is hidden to the user, who manipulates it using the pseudo
       register called "bank" (67).  See below.  */
    "",
    /* 64 - 66 */
    "ibcr", "ibnr", "tbr",
    /* 67: register bank number, the user visible pseudo register.  */
    "bank",
    /* double precision (pseudo) 68 - 75: report blank, see below.  */
  };
  if (reg_nr >= ARRAY_SIZE (register_names))
    return "";
  return register_names[reg_nr];
}

static const char *
sh_sh_dsp_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static const char *register_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "pc", "pr", "gbr", "vbr", "mach", "macl", "sr",
    "", "dsr",
    "a0g", "a0", "a1g", "a1", "m0", "m1", "x0", "x1",
    "y0", "y1", "", "", "", "", "", "mod",
    "", "",
    "rs", "re",
  };
  if (reg_nr >= ARRAY_SIZE (register_names))
    return "";
  return register_names[reg_nr];
}

static const char *
sh_sh3_dsp_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static const char *register_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "pc", "pr", "gbr", "vbr", "mach", "macl", "sr",
    "", "dsr",
    "a0g", "a0", "a1g", "a1", "m0", "m1", "x0", "x1",
    "y0", "y1", "", "", "", "", "", "mod",
    "ssr", "spc",
    "rs", "re", "", "", "", "", "", "",
    "r0b", "r1b", "r2b", "r3b", "r4b", "r5b", "r6b", "r7b",
  };
  if (reg_nr >= ARRAY_SIZE (register_names))
    return "";
  return register_names[reg_nr];
}

static const char *
sh_sh4_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static const char *register_names[] = {
    /* general registers 0-15 */
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    /* 16 - 22 */
    "pc", "pr", "gbr", "vbr", "mach", "macl", "sr",
    /* 23, 24 */
    "fpul", "fpscr",
    /* floating point registers 25 - 40 */
    "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7",
    "fr8", "fr9", "fr10", "fr11", "fr12", "fr13", "fr14", "fr15",
    /* 41, 42 */
    "ssr", "spc",
    /* bank 0 43 - 50 */
    "r0b0", "r1b0", "r2b0", "r3b0", "r4b0", "r5b0", "r6b0", "r7b0",
    /* bank 1 51 - 58 */
    "r0b1", "r1b1", "r2b1", "r3b1", "r4b1", "r5b1", "r6b1", "r7b1",
    /* 59 - 66 */
    "", "", "", "", "", "", "", "",
    /* pseudo bank register.  */
    "",
    /* double precision (pseudo) 68 - 75 */
    "dr0", "dr2", "dr4", "dr6", "dr8", "dr10", "dr12", "dr14",
    /* vectors (pseudo) 76 - 79 */
    "fv0", "fv4", "fv8", "fv12",
    /* FIXME: missing XF */
    /* FIXME: missing XD */
  };
  if (reg_nr >= ARRAY_SIZE (register_names))
    return "";
  return register_names[reg_nr];
}

static const char *
sh_sh4_nofpu_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static const char *register_names[] = {
    /* general registers 0-15 */
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    /* 16 - 22 */
    "pc", "pr", "gbr", "vbr", "mach", "macl", "sr",
    /* 23, 24 */
    "", "",
    /* floating point registers 25 - 40 -- not for nofpu target */
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    /* 41, 42 */
    "ssr", "spc",
    /* bank 0 43 - 50 */
    "r0b0", "r1b0", "r2b0", "r3b0", "r4b0", "r5b0", "r6b0", "r7b0",
    /* bank 1 51 - 58 */
    "r0b1", "r1b1", "r2b1", "r3b1", "r4b1", "r5b1", "r6b1", "r7b1",
    /* 59 - 66 */
    "", "", "", "", "", "", "", "",
    /* pseudo bank register.  */
    "",
    /* double precision (pseudo) 68 - 75 -- not for nofpu target */
    "", "", "", "", "", "", "", "",
    /* vectors (pseudo) 76 - 79 -- not for nofpu target: report blank
       below.  */
  };
  if (reg_nr >= ARRAY_SIZE (register_names))
    return "";
  return register_names[reg_nr];
}

static const char *
sh_sh4al_dsp_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static const char *register_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "pc", "pr", "gbr", "vbr", "mach", "macl", "sr",
    "", "dsr",
    "a0g", "a0", "a1g", "a1", "m0", "m1", "x0", "x1",
    "y0", "y1", "", "", "", "", "", "mod",
    "ssr", "spc",
    "rs", "re", "", "", "", "", "", "",
    "r0b", "r1b", "r2b", "r3b", "r4b", "r5b", "r6b", "r7b",
  };
  if (reg_nr >= ARRAY_SIZE (register_names))
    return "";
  return register_names[reg_nr];
}

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
sh_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  return 2;
}

/* Implement the sw_breakpoint_from_kind gdbarch method.  */

static const gdb_byte *
sh_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  *size = kind;

  /* For remote stub targets, trapa #20 is used.  */
  if (strcmp (target_shortname (), "remote") == 0)
    {
      static unsigned char big_remote_breakpoint[] = { 0xc3, 0x20 };
      static unsigned char little_remote_breakpoint[] = { 0x20, 0xc3 };

      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	return big_remote_breakpoint;
      else
	return little_remote_breakpoint;
    }
  else
    {
      /* 0xc3c3 is trapa #c3, and it works in big and little endian
	 modes.  */
      static unsigned char breakpoint[] = { 0xc3, 0xc3 };

      return breakpoint;
    }
}

/* Prologue looks like
   mov.l	r14,@-r15
   sts.l	pr,@-r15
   mov.l	<regs>,@-r15
   sub		<room_for_loca_vars>,r15
   mov		r15,r14

   Actually it can be more complicated than this but that's it, basically.  */

#define GET_SOURCE_REG(x)  	(((x) >> 4) & 0xf)
#define GET_TARGET_REG(x)  	(((x) >> 8) & 0xf)

/* JSR @Rm         0100mmmm00001011 */
#define IS_JSR(x)		(((x) & 0xf0ff) == 0x400b)

/* STS.L PR,@-r15  0100111100100010
   r15-4-->r15, PR-->(r15) */
#define IS_STS(x)  		((x) == 0x4f22)

/* STS.L MACL,@-r15  0100111100010010
   r15-4-->r15, MACL-->(r15) */
#define IS_MACL_STS(x)  	((x) == 0x4f12)

/* MOV.L Rm,@-r15  00101111mmmm0110
   r15-4-->r15, Rm-->(R15) */
#define IS_PUSH(x) 		(((x) & 0xff0f) == 0x2f06)

/* MOV r15,r14     0110111011110011
   r15-->r14  */
#define IS_MOV_SP_FP(x)  	((x) == 0x6ef3)

/* ADD #imm,r15    01111111iiiiiiii
   r15+imm-->r15 */
#define IS_ADD_IMM_SP(x) 	(((x) & 0xff00) == 0x7f00)

#define IS_MOV_R3(x) 		(((x) & 0xff00) == 0x1a00)
#define IS_SHLL_R3(x)		((x) == 0x4300)

/* ADD r3,r15      0011111100111100
   r15+r3-->r15 */
#define IS_ADD_R3SP(x)		((x) == 0x3f3c)

/* FMOV.S FRm,@-Rn  Rn-4-->Rn, FRm-->(Rn)     1111nnnnmmmm1011
   FMOV DRm,@-Rn    Rn-8-->Rn, DRm-->(Rn)     1111nnnnmmm01011
   FMOV XDm,@-Rn    Rn-8-->Rn, XDm-->(Rn)     1111nnnnmmm11011 */
/* CV, 2003-08-28: Only suitable with Rn == SP, therefore name changed to
		   make this entirely clear.  */
/* #define IS_FMOV(x)		(((x) & 0xf00f) == 0xf00b) */
#define IS_FPUSH(x)		(((x) & 0xff0f) == 0xff0b)

/* MOV Rm,Rn          Rm-->Rn        0110nnnnmmmm0011  4 <= m <= 7 */
#define IS_MOV_ARG_TO_REG(x) \
	(((x) & 0xf00f) == 0x6003 && \
	 ((x) & 0x00f0) >= 0x0040 && \
	 ((x) & 0x00f0) <= 0x0070)
/* MOV.L Rm,@Rn               0010nnnnmmmm0010  n = 14, 4 <= m <= 7 */
#define IS_MOV_ARG_TO_IND_R14(x) \
	(((x) & 0xff0f) == 0x2e02 && \
	 ((x) & 0x00f0) >= 0x0040 && \
	 ((x) & 0x00f0) <= 0x0070)
/* MOV.L Rm,@(disp*4,Rn)      00011110mmmmdddd  n = 14, 4 <= m <= 7 */
#define IS_MOV_ARG_TO_IND_R14_WITH_DISP(x) \
	(((x) & 0xff00) == 0x1e00 && \
	 ((x) & 0x00f0) >= 0x0040 && \
	 ((x) & 0x00f0) <= 0x0070)

/* MOV.W @(disp*2,PC),Rn      1001nnnndddddddd */
#define IS_MOVW_PCREL_TO_REG(x)	(((x) & 0xf000) == 0x9000)
/* MOV.L @(disp*4,PC),Rn      1101nnnndddddddd */
#define IS_MOVL_PCREL_TO_REG(x)	(((x) & 0xf000) == 0xd000)
/* MOVI20 #imm20,Rn           0000nnnniiii0000 */
#define IS_MOVI20(x)		(((x) & 0xf00f) == 0x0000)
/* SUB Rn,R15                 00111111nnnn1000 */
#define IS_SUB_REG_FROM_SP(x)	(((x) & 0xff0f) == 0x3f08)

#define FPSCR_SZ		(1 << 20)

/* The following instructions are used for epilogue testing.  */
#define IS_RESTORE_FP(x)	((x) == 0x6ef6)
#define IS_RTS(x)		((x) == 0x000b)
#define IS_LDS(x)  		((x) == 0x4f26)
#define IS_MACL_LDS(x)  	((x) == 0x4f16)
#define IS_MOV_FP_SP(x)  	((x) == 0x6fe3)
#define IS_ADD_REG_TO_FP(x)	(((x) & 0xff0f) == 0x3e0c)
#define IS_ADD_IMM_FP(x) 	(((x) & 0xff00) == 0x7e00)

static CORE_ADDR
sh_analyze_prologue (struct gdbarch *gdbarch,
		     CORE_ADDR pc, CORE_ADDR limit_pc,
		     struct sh_frame_cache *cache, ULONGEST fpscr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST inst;
  int offset;
  int sav_offset = 0;
  int r3_val = 0;
  int reg, sav_reg = -1;

  cache->uses_fp = 0;
  for (; pc < limit_pc; pc += 2)
    {
      inst = read_memory_unsigned_integer (pc, 2, byte_order);
      /* See where the registers will be saved to.  */
      if (IS_PUSH (inst))
	{
	  cache->saved_regs[GET_SOURCE_REG (inst)] = cache->sp_offset;
	  cache->sp_offset += 4;
	}
      else if (IS_STS (inst))
	{
	  cache->saved_regs[PR_REGNUM] = cache->sp_offset;
	  cache->sp_offset += 4;
	}
      else if (IS_MACL_STS (inst))
	{
	  cache->saved_regs[MACL_REGNUM] = cache->sp_offset;
	  cache->sp_offset += 4;
	}
      else if (IS_MOV_R3 (inst))
	{
	  r3_val = ((inst & 0xff) ^ 0x80) - 0x80;
	}
      else if (IS_SHLL_R3 (inst))
	{
	  r3_val <<= 1;
	}
      else if (IS_ADD_R3SP (inst))
	{
	  cache->sp_offset += -r3_val;
	}
      else if (IS_ADD_IMM_SP (inst))
	{
	  offset = ((inst & 0xff) ^ 0x80) - 0x80;
	  cache->sp_offset -= offset;
	}
      else if (IS_MOVW_PCREL_TO_REG (inst))
	{
	  if (sav_reg < 0)
	    {
	      reg = GET_TARGET_REG (inst);
	      if (reg < 14)
		{
		  sav_reg = reg;
		  offset = (inst & 0xff) << 1;
		  sav_offset =
		    read_memory_integer ((pc + 4) + offset, 2, byte_order);
		}
	    }
	}
      else if (IS_MOVL_PCREL_TO_REG (inst))
	{
	  if (sav_reg < 0)
	    {
	      reg = GET_TARGET_REG (inst);
	      if (reg < 14)
		{
		  sav_reg = reg;
		  offset = (inst & 0xff) << 2;
		  sav_offset =
		    read_memory_integer (((pc & 0xfffffffc) + 4) + offset,
					 4, byte_order);
		}
	    }
	}
      else if (IS_MOVI20 (inst)
	       && (pc + 2 < limit_pc))
	{
	  if (sav_reg < 0)
	    {
	      reg = GET_TARGET_REG (inst);
	      if (reg < 14)
		{
		  sav_reg = reg;
		  sav_offset = GET_SOURCE_REG (inst) << 16;
		  /* MOVI20 is a 32 bit instruction!  */
		  pc += 2;
		  sav_offset
		    |= read_memory_unsigned_integer (pc, 2, byte_order);
		  /* Now sav_offset contains an unsigned 20 bit value.
		     It must still get sign extended.  */
		  if (sav_offset & 0x00080000)
		    sav_offset |= 0xfff00000;
		}
	    }
	}
      else if (IS_SUB_REG_FROM_SP (inst))
	{
	  reg = GET_SOURCE_REG (inst);
	  if (sav_reg > 0 && reg == sav_reg)
	    {
	      sav_reg = -1;
	    }
	  cache->sp_offset += sav_offset;
	}
      else if (IS_FPUSH (inst))
	{
	  if (fpscr & FPSCR_SZ)
	    {
	      cache->sp_offset += 8;
	    }
	  else
	    {
	      cache->sp_offset += 4;
	    }
	}
      else if (IS_MOV_SP_FP (inst))
	{
	  pc += 2;
	  /* Don't go any further than six more instructions.  */
	  limit_pc = std::min (limit_pc, pc + (2 * 6));

	  cache->uses_fp = 1;
	  /* At this point, only allow argument register moves to other
	     registers or argument register moves to @(X,fp) which are
	     moving the register arguments onto the stack area allocated
	     by a former add somenumber to SP call.  Don't allow moving
	     to an fp indirect address above fp + cache->sp_offset.  */
	  for (; pc < limit_pc; pc += 2)
	    {
	      inst = read_memory_integer (pc, 2, byte_order);
	      if (IS_MOV_ARG_TO_IND_R14 (inst))
		{
		  reg = GET_SOURCE_REG (inst);
		  if (cache->sp_offset > 0)
		    cache->saved_regs[reg] = cache->sp_offset;
		}
	      else if (IS_MOV_ARG_TO_IND_R14_WITH_DISP (inst))
		{
		  reg = GET_SOURCE_REG (inst);
		  offset = (inst & 0xf) * 4;
		  if (cache->sp_offset > offset)
		    cache->saved_regs[reg] = cache->sp_offset - offset;
		}
	      else if (IS_MOV_ARG_TO_REG (inst))
		continue;
	      else
		break;
	    }
	  break;
	}
      else if (IS_JSR (inst))
	{
	  /* We have found a jsr that has been scheduled into the prologue.
	     If we continue the scan and return a pc someplace after this,
	     then setting a breakpoint on this function will cause it to
	     appear to be called after the function it is calling via the
	     jsr, which will be very confusing.  Most likely the next
	     instruction is going to be IS_MOV_SP_FP in the delay slot.  If
	     so, note that before returning the current pc.  */
	  if (pc + 2 < limit_pc)
	    {
	      inst = read_memory_integer (pc + 2, 2, byte_order);
	      if (IS_MOV_SP_FP (inst))
		cache->uses_fp = 1;
	    }
	  break;
	}
#if 0		/* This used to just stop when it found an instruction
		   that was not considered part of the prologue.  Now,
		   we just keep going looking for likely
		   instructions.  */
      else
	break;
#endif
    }

  return pc;
}

/* Skip any prologue before the guts of a function.  */
static CORE_ADDR
sh_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR post_prologue_pc, func_addr, func_end_addr, limit_pc;
  struct sh_frame_cache cache;

  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever
     is greater.  */
  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end_addr))
    {
      post_prologue_pc = skip_prologue_using_sal (gdbarch, func_addr);
      if (post_prologue_pc != 0)
	return std::max (pc, post_prologue_pc);
    }

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  */

  /* Find an upper limit on the function prologue using the debug
     information.  If the debug information could not be used to provide
     that bound, then use an arbitrary large number as the upper bound.  */
  limit_pc = skip_prologue_using_sal (gdbarch, pc);
  if (limit_pc == 0)
    /* Don't go any further than 28 instructions.  */
    limit_pc = pc + (2 * 28);

  /* Do not allow limit_pc to be past the function end, if we know
     where that end is...  */
  if (func_end_addr != 0)
    limit_pc = std::min (limit_pc, func_end_addr);

  cache.sp_offset = -4;
  post_prologue_pc = sh_analyze_prologue (gdbarch, pc, limit_pc, &cache, 0);
  if (cache.uses_fp)
    pc = post_prologue_pc;

  return pc;
}

/* The ABI says:

   Aggregate types not bigger than 8 bytes that have the same size and
   alignment as one of the integer scalar types are returned in the
   same registers as the integer type they match.

   For example, a 2-byte aligned structure with size 2 bytes has the
   same size and alignment as a short int, and will be returned in R0.
   A 4-byte aligned structure with size 8 bytes has the same size and
   alignment as a long long int, and will be returned in R0 and R1.

   When an aggregate type is returned in R0 and R1, R0 contains the
   first four bytes of the aggregate, and R1 contains the
   remainder.  If the size of the aggregate type is not a multiple of 4
   bytes, the aggregate is tail-padded up to a multiple of 4
   bytes.  The value of the padding is undefined.  For little-endian
   targets the padding will appear at the most significant end of the
   last element, for big-endian targets the padding appears at the
   least significant end of the last element.

   All other aggregate types are returned by address.  The caller
   function passes the address of an area large enough to hold the
   aggregate value in R2.  The called function stores the result in
   this location.

   To reiterate, structs smaller than 8 bytes could also be returned
   in memory, if they don't pass the "same size and alignment as an
   integer type" rule.

   For example, in

   struct s { char c[3]; } wibble;
   struct s foo(void) {  return wibble; }

   the return value from foo() will be in memory, not
   in R0, because there is no 3-byte integer type.

   Similarly, in 

   struct s { char c[2]; } wibble;
   struct s foo(void) {  return wibble; }

   because a struct containing two chars has alignment 1, that matches
   type char, but size 2, that matches type short.  There's no integer
   type that has alignment 1 and size 2, so the struct is returned in
   memory.  */

static int
sh_use_struct_convention (int renesas_abi, struct type *type)
{
  int len = type->length ();
  int nelem = type->num_fields ();

  /* The Renesas ABI returns aggregate types always on stack.  */
  if (renesas_abi && (type->code () == TYPE_CODE_STRUCT
		      || type->code () == TYPE_CODE_UNION))
    return 1;

  /* Non-power of 2 length types and types bigger than 8 bytes (which don't
     fit in two registers anyway) use struct convention.  */
  if (len != 1 && len != 2 && len != 4 && len != 8)
    return 1;

  /* Scalar types and aggregate types with exactly one field are aligned
     by definition.  They are returned in registers.  */
  if (nelem <= 1)
    return 0;

  /* If the first field in the aggregate has the same length as the entire
     aggregate type, the type is returned in registers.  */
  if (type->field (0).type ()->length () == len)
    return 0;

  /* If the size of the aggregate is 8 bytes and the first field is
     of size 4 bytes its alignment is equal to long long's alignment,
     so it's returned in registers.  */
  if (len == 8 && type->field (0).type ()->length () == 4)
    return 0;

  /* Otherwise use struct convention.  */
  return 1;
}

static int
sh_use_struct_convention_nofpu (int renesas_abi, struct type *type)
{
  /* The Renesas ABI returns long longs/doubles etc. always on stack.  */
  if (renesas_abi && type->num_fields () == 0 && type->length () >= 8)
    return 1;
  return sh_use_struct_convention (renesas_abi, type);
}

static CORE_ADDR
sh_frame_align (struct gdbarch *ignore, CORE_ADDR sp)
{
  return sp & ~3;
}

/* Function: push_dummy_call (formerly push_arguments)
   Setup the function arguments for calling a function in the inferior.

   On the Renesas SH architecture, there are four registers (R4 to R7)
   which are dedicated for passing function arguments.  Up to the first
   four arguments (depending on size) may go into these registers.
   The rest go on the stack.

   MVS: Except on SH variants that have floating point registers.
   In that case, float and double arguments are passed in the same
   manner, but using FP registers instead of GP registers.

   Arguments that are smaller than 4 bytes will still take up a whole
   register or a whole 32-bit word on the stack, and will be 
   right-justified in the register or the stack word.  This includes
   chars, shorts, and small aggregate types.

   Arguments that are larger than 4 bytes may be split between two or 
   more registers.  If there are not enough registers free, an argument
   may be passed partly in a register (or registers), and partly on the
   stack.  This includes doubles, long longs, and larger aggregates.
   As far as I know, there is no upper limit to the size of aggregates 
   that will be passed in this way; in other words, the convention of 
   passing a pointer to a large aggregate instead of a copy is not used.

   MVS: The above appears to be true for the SH variants that do not
   have an FPU, however those that have an FPU appear to copy the
   aggregate argument onto the stack (and not place it in registers)
   if it is larger than 16 bytes (four GP registers).

   An exceptional case exists for struct arguments (and possibly other
   aggregates such as arrays) if the size is larger than 4 bytes but 
   not a multiple of 4 bytes.  In this case the argument is never split 
   between the registers and the stack, but instead is copied in its
   entirety onto the stack, AND also copied into as many registers as 
   there is room for.  In other words, space in registers permitting, 
   two copies of the same argument are passed in.  As far as I can tell,
   only the one on the stack is used, although that may be a function 
   of the level of compiler optimization.  I suspect this is a compiler
   bug.  Arguments of these odd sizes are left-justified within the 
   word (as opposed to arguments smaller than 4 bytes, which are 
   right-justified).

   If the function is to return an aggregate type such as a struct, it 
   is either returned in the normal return value register R0 (if its 
   size is no greater than one byte), or else the caller must allocate
   space into which the callee will copy the return value (if the size
   is greater than one byte).  In this case, a pointer to the return 
   value location is passed into the callee in register R2, which does 
   not displace any of the other arguments passed in via registers R4
   to R7.  */

/* Helper function to justify value in register according to endianness.  */
static const gdb_byte *
sh_justify_value_in_reg (struct gdbarch *gdbarch, struct value *val, int len)
{
  static gdb_byte valbuf[4];

  memset (valbuf, 0, sizeof (valbuf));
  if (len < 4)
    {
      /* value gets right-justified in the register or stack word.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	memcpy (valbuf + (4 - len), val->contents ().data (), len);
      else
	memcpy (valbuf, val->contents ().data (), len);
      return valbuf;
    }
  return val->contents ().data ();
}

/* Helper function to eval number of bytes to allocate on stack.  */
static CORE_ADDR
sh_stack_allocsize (int nargs, struct value **args)
{
  int stack_alloc = 0;
  while (nargs-- > 0)
    stack_alloc += ((args[nargs]->type ()->length () + 3) & ~3);
  return stack_alloc;
}

/* Helper functions for getting the float arguments right.  Registers usage
   depends on the ABI and the endianness.  The comments should enlighten how
   it's intended to work.  */

/* This array stores which of the float arg registers are already in use.  */
static int flt_argreg_array[FLOAT_ARGLAST_REGNUM - FLOAT_ARG0_REGNUM + 1];

/* This function just resets the above array to "no reg used so far".  */
static void
sh_init_flt_argreg (void)
{
  memset (flt_argreg_array, 0, sizeof flt_argreg_array);
}

/* This function returns the next register to use for float arg passing.
   It returns either a valid value between FLOAT_ARG0_REGNUM and
   FLOAT_ARGLAST_REGNUM if a register is available, otherwise it returns 
   FLOAT_ARGLAST_REGNUM + 1 to indicate that no register is available.

   Note that register number 0 in flt_argreg_array corresponds with the
   real float register fr4.  In contrast to FLOAT_ARG0_REGNUM (value is
   29) the parity of the register number is preserved, which is important
   for the double register passing test (see the "argreg & 1" test below).  */
static int
sh_next_flt_argreg (struct gdbarch *gdbarch, int len, struct type *func_type)
{
  int argreg;

  /* First search for the next free register.  */
  for (argreg = 0; argreg <= FLOAT_ARGLAST_REGNUM - FLOAT_ARG0_REGNUM;
       ++argreg)
    if (!flt_argreg_array[argreg])
      break;

  /* No register left?  */
  if (argreg > FLOAT_ARGLAST_REGNUM - FLOAT_ARG0_REGNUM)
    return FLOAT_ARGLAST_REGNUM + 1;

  if (len == 8)
    {
      /* Doubles are always starting in a even register number.  */
      if (argreg & 1)
	{
	  /* In gcc ABI, the skipped register is lost for further argument
	     passing now.  Not so in Renesas ABI.  */
	  if (!sh_is_renesas_calling_convention (func_type))
	    flt_argreg_array[argreg] = 1;

	  ++argreg;

	  /* No register left?  */
	  if (argreg > FLOAT_ARGLAST_REGNUM - FLOAT_ARG0_REGNUM)
	    return FLOAT_ARGLAST_REGNUM + 1;
	}
      /* Also mark the next register as used.  */
      flt_argreg_array[argreg + 1] = 1;
    }
  else if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_LITTLE
	   && !sh_is_renesas_calling_convention (func_type))
    {
      /* In little endian, gcc passes floats like this: f5, f4, f7, f6, ...  */
      if (!flt_argreg_array[argreg + 1])
	++argreg;
    }
  flt_argreg_array[argreg] = 1;
  return FLOAT_ARG0_REGNUM + argreg;
}

/* Helper function which figures out, if a type is treated like a float type.

   The FPU ABIs have a special way how to treat types as float types.
   Structures with exactly one member, which is of type float or double, are
   treated exactly as the base types float or double:

     struct sf {
       float f;
     };

     struct sd {
       double d;
     };

   are handled the same way as just

     float f;

     double d;

   As a result, arguments of these struct types are pushed into floating point
   registers exactly as floats or doubles, using the same decision algorithm.

   The same is valid if these types are used as function return types.  The
   above structs are returned in fr0 resp. fr0,fr1 instead of in r0, r0,r1
   or even using struct convention as it is for other structs.  */

static int
sh_treat_as_flt_p (struct type *type)
{
  /* Ordinary float types are obviously treated as float.  */
  if (type->code () == TYPE_CODE_FLT)
    return 1;
  /* Otherwise non-struct types are not treated as float.  */
  if (type->code () != TYPE_CODE_STRUCT)
    return 0;
  /* Otherwise structs with more than one member are not treated as float.  */
  if (type->num_fields () != 1)
    return 0;
  /* Otherwise if the type of that member is float, the whole type is
     treated as float.  */
  if (type->field (0).type ()->code () == TYPE_CODE_FLT)
    return 1;
  /* Otherwise it's not treated as float.  */
  return 0;
}

static CORE_ADDR
sh_push_dummy_call_fpu (struct gdbarch *gdbarch,
			struct value *function,
			struct regcache *regcache,
			CORE_ADDR bp_addr, int nargs,
			struct value **args,
			CORE_ADDR sp, function_call_return_method return_method,
			CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int stack_offset = 0;
  int argreg = ARG0_REGNUM;
  int flt_argreg = 0;
  int argnum;
  struct type *func_type = function->type ();
  struct type *type;
  CORE_ADDR regval;
  const gdb_byte *val;
  int len, reg_size = 0;
  int pass_on_stack = 0;
  int treat_as_flt;
  int last_reg_arg = INT_MAX;

  /* The Renesas ABI expects all varargs arguments, plus the last
     non-vararg argument to be on the stack, no matter how many
     registers have been used so far.  */
  if (sh_is_renesas_calling_convention (func_type)
      && func_type->has_varargs ())
    last_reg_arg = func_type->num_fields () - 2;

  /* First force sp to a 4-byte alignment.  */
  sp = sh_frame_align (gdbarch, sp);

  /* Make room on stack for args.  */
  sp -= sh_stack_allocsize (nargs, args);

  /* Initialize float argument mechanism.  */
  sh_init_flt_argreg ();

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  There are 16 bytes
     in four registers available.  Loop thru args from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      type = args[argnum]->type ();
      len = type->length ();
      val = sh_justify_value_in_reg (gdbarch, args[argnum], len);

      /* Some decisions have to be made how various types are handled.
	 This also differs in different ABIs.  */
      pass_on_stack = 0;

      /* Find out the next register to use for a floating point value.  */
      treat_as_flt = sh_treat_as_flt_p (type);
      if (treat_as_flt)
	flt_argreg = sh_next_flt_argreg (gdbarch, len, func_type);
      /* In Renesas ABI, long longs and aggregate types are always passed
	 on stack.  */
      else if (sh_is_renesas_calling_convention (func_type)
	       && ((type->code () == TYPE_CODE_INT && len == 8)
		   || type->code () == TYPE_CODE_STRUCT
		   || type->code () == TYPE_CODE_UNION))
	pass_on_stack = 1;
      /* In contrast to non-FPU CPUs, arguments are never split between
	 registers and stack.  If an argument doesn't fit in the remaining
	 registers it's always pushed entirely on the stack.  */
      else if (len > ((ARGLAST_REGNUM - argreg + 1) * 4))
	pass_on_stack = 1;

      while (len > 0)
	{
	  if ((treat_as_flt && flt_argreg > FLOAT_ARGLAST_REGNUM)
	      || (!treat_as_flt && (argreg > ARGLAST_REGNUM
				    || pass_on_stack))
	      || argnum > last_reg_arg)
	    {
	      /* The data goes entirely on the stack, 4-byte aligned.  */
	      reg_size = (len + 3) & ~3;
	      write_memory (sp + stack_offset, val, reg_size);
	      stack_offset += reg_size;
	    }
	  else if (treat_as_flt && flt_argreg <= FLOAT_ARGLAST_REGNUM)
	    {
	      /* Argument goes in a float argument register.  */
	      reg_size = register_size (gdbarch, flt_argreg);
	      regval = extract_unsigned_integer (val, reg_size, byte_order);
	      /* In little endian mode, float types taking two registers
		 (doubles on sh4, long doubles on sh2e, sh3e and sh4) must
		 be stored swapped in the argument registers.  The below
		 code first writes the first 32 bits in the next but one
		 register, increments the val and len values accordingly
		 and then proceeds as normal by writing the second 32 bits
		 into the next register.  */
	      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_LITTLE
		  && type->length () == 2 * reg_size)
		{
		  regcache_cooked_write_unsigned (regcache, flt_argreg + 1,
						  regval);
		  val += reg_size;
		  len -= reg_size;
		  regval = extract_unsigned_integer (val, reg_size,
						     byte_order);
		}
	      regcache_cooked_write_unsigned (regcache, flt_argreg++, regval);
	    }
	  else if (!treat_as_flt && argreg <= ARGLAST_REGNUM)
	    {
	      /* there's room in a register */
	      reg_size = register_size (gdbarch, argreg);
	      regval = extract_unsigned_integer (val, reg_size, byte_order);
	      regcache_cooked_write_unsigned (regcache, argreg++, regval);
	    }
	  /* Store the value one register at a time or in one step on
	     stack.  */
	  len -= reg_size;
	  val += reg_size;
	}
    }

  if (return_method == return_method_struct)
    {
      if (sh_is_renesas_calling_convention (func_type))
	/* If the function uses the Renesas ABI, subtract another 4 bytes from
	   the stack and store the struct return address there.  */
	write_memory_unsigned_integer (sp -= 4, 4, byte_order, struct_addr);
      else
	/* Using the gcc ABI, the "struct return pointer" pseudo-argument has
	   its own dedicated register.  */
	regcache_cooked_write_unsigned (regcache,
					STRUCT_RETURN_REGNUM, struct_addr);
    }

  /* Store return address.  */
  regcache_cooked_write_unsigned (regcache, PR_REGNUM, bp_addr);

  /* Update stack pointer.  */
  regcache_cooked_write_unsigned (regcache,
				  gdbarch_sp_regnum (gdbarch), sp);

  return sp;
}

static CORE_ADDR
sh_push_dummy_call_nofpu (struct gdbarch *gdbarch,
			  struct value *function,
			  struct regcache *regcache,
			  CORE_ADDR bp_addr,
			  int nargs, struct value **args,
			  CORE_ADDR sp,
			  function_call_return_method return_method,
			  CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int stack_offset = 0;
  int argreg = ARG0_REGNUM;
  int argnum;
  struct type *func_type = function->type ();
  struct type *type;
  CORE_ADDR regval;
  const gdb_byte *val;
  int len, reg_size = 0;
  int pass_on_stack = 0;
  int last_reg_arg = INT_MAX;

  /* The Renesas ABI expects all varargs arguments, plus the last
     non-vararg argument to be on the stack, no matter how many
     registers have been used so far.  */
  if (sh_is_renesas_calling_convention (func_type)
      && func_type->has_varargs ())
    last_reg_arg = func_type->num_fields () - 2;

  /* First force sp to a 4-byte alignment.  */
  sp = sh_frame_align (gdbarch, sp);

  /* Make room on stack for args.  */
  sp -= sh_stack_allocsize (nargs, args);

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  There are 16 bytes
     in four registers available.  Loop thru args from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      type = args[argnum]->type ();
      len = type->length ();
      val = sh_justify_value_in_reg (gdbarch, args[argnum], len);

      /* Some decisions have to be made how various types are handled.
	 This also differs in different ABIs.  */
      pass_on_stack = 0;
      /* Renesas ABI pushes doubles and long longs entirely on stack.
	 Same goes for aggregate types.  */
      if (sh_is_renesas_calling_convention (func_type)
	  && ((type->code () == TYPE_CODE_INT && len >= 8)
	      || (type->code () == TYPE_CODE_FLT && len >= 8)
	      || type->code () == TYPE_CODE_STRUCT
	      || type->code () == TYPE_CODE_UNION))
	pass_on_stack = 1;
      while (len > 0)
	{
	  if (argreg > ARGLAST_REGNUM || pass_on_stack
	      || argnum > last_reg_arg)
	    {
	      /* The remainder of the data goes entirely on the stack,
		 4-byte aligned.  */
	      reg_size = (len + 3) & ~3;
	      write_memory (sp + stack_offset, val, reg_size);
	      stack_offset += reg_size;
	    }
	  else if (argreg <= ARGLAST_REGNUM)
	    {
	      /* There's room in a register.  */
	      reg_size = register_size (gdbarch, argreg);
	      regval = extract_unsigned_integer (val, reg_size, byte_order);
	      regcache_cooked_write_unsigned (regcache, argreg++, regval);
	    }
	  /* Store the value reg_size bytes at a time.  This means that things
	     larger than reg_size bytes may go partly in registers and partly
	     on the stack.  */
	  len -= reg_size;
	  val += reg_size;
	}
    }

  if (return_method == return_method_struct)
    {
      if (sh_is_renesas_calling_convention (func_type))
	/* If the function uses the Renesas ABI, subtract another 4 bytes from
	   the stack and store the struct return address there.  */
	write_memory_unsigned_integer (sp -= 4, 4, byte_order, struct_addr);
      else
	/* Using the gcc ABI, the "struct return pointer" pseudo-argument has
	   its own dedicated register.  */
	regcache_cooked_write_unsigned (regcache,
					STRUCT_RETURN_REGNUM, struct_addr);
    }

  /* Store return address.  */
  regcache_cooked_write_unsigned (regcache, PR_REGNUM, bp_addr);

  /* Update stack pointer.  */
  regcache_cooked_write_unsigned (regcache,
				  gdbarch_sp_regnum (gdbarch), sp);

  return sp;
}

/* Find a function's return value in the appropriate registers (in
   regbuf), and copy it into valbuf.  Extract from an array REGBUF
   containing the (raw) register state a function return value of type
   TYPE, and copy that, in virtual format, into VALBUF.  */
static void
sh_extract_return_value_nofpu (struct type *type, struct regcache *regcache,
			       gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int len = type->length ();

  if (len <= 4)
    {
      ULONGEST c;

      regcache_cooked_read_unsigned (regcache, R0_REGNUM, &c);
      store_unsigned_integer (valbuf, len, byte_order, c);
    }
  else if (len == 8)
    {
      int i, regnum = R0_REGNUM;
      for (i = 0; i < len; i += 4)
	regcache->raw_read (regnum++, valbuf + i);
    }
  else
    error (_("bad size for return value"));
}

static void
sh_extract_return_value_fpu (struct type *type, struct regcache *regcache,
			     gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  if (sh_treat_as_flt_p (type))
    {
      int len = type->length ();
      int i, regnum = gdbarch_fp0_regnum (gdbarch);
      for (i = 0; i < len; i += 4)
	if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_LITTLE)
	  regcache->raw_read (regnum++,
			     valbuf + len - 4 - i);
	else
	  regcache->raw_read (regnum++, valbuf + i);
    }
  else
    sh_extract_return_value_nofpu (type, regcache, valbuf);
}

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.
   If the architecture is sh4 or sh3e, store a function's return value
   in the R0 general register or in the FP0 floating point register,
   depending on the type of the return value.  In all the other cases
   the result is stored in r0, left-justified.  */
static void
sh_store_return_value_nofpu (struct type *type, struct regcache *regcache,
			     const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST val;
  int len = type->length ();

  if (len <= 4)
    {
      val = extract_unsigned_integer (valbuf, len, byte_order);
      regcache_cooked_write_unsigned (regcache, R0_REGNUM, val);
    }
  else
    {
      int i, regnum = R0_REGNUM;
      for (i = 0; i < len; i += 4)
	regcache->raw_write (regnum++, valbuf + i);
    }
}

static void
sh_store_return_value_fpu (struct type *type, struct regcache *regcache,
			   const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  if (sh_treat_as_flt_p (type))
    {
      int len = type->length ();
      int i, regnum = gdbarch_fp0_regnum (gdbarch);
      for (i = 0; i < len; i += 4)
	if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_LITTLE)
	  regcache->raw_write (regnum++,
			      valbuf + len - 4 - i);
	else
	  regcache->raw_write (regnum++, valbuf + i);
    }
  else
    sh_store_return_value_nofpu (type, regcache, valbuf);
}

static enum return_value_convention
sh_return_value_nofpu (struct gdbarch *gdbarch, struct value *function,
		       struct type *type, struct regcache *regcache,
		       gdb_byte *readbuf, const gdb_byte *writebuf)
{
  struct type *func_type = function ? function->type () : NULL;

  if (sh_use_struct_convention_nofpu
	(sh_is_renesas_calling_convention (func_type), type))
    return RETURN_VALUE_STRUCT_CONVENTION;
  if (writebuf)
    sh_store_return_value_nofpu (type, regcache, writebuf);
  else if (readbuf)
    sh_extract_return_value_nofpu (type, regcache, readbuf);
  return RETURN_VALUE_REGISTER_CONVENTION;
}

static enum return_value_convention
sh_return_value_fpu (struct gdbarch *gdbarch, struct value *function,
		     struct type *type, struct regcache *regcache,
		     gdb_byte *readbuf, const gdb_byte *writebuf)
{
  struct type *func_type = function ? function->type () : NULL;

  if (sh_use_struct_convention (
	sh_is_renesas_calling_convention (func_type), type))
    return RETURN_VALUE_STRUCT_CONVENTION;
  if (writebuf)
    sh_store_return_value_fpu (type, regcache, writebuf);
  else if (readbuf)
    sh_extract_return_value_fpu (type, regcache, readbuf);
  return RETURN_VALUE_REGISTER_CONVENTION;
}

static struct type *
sh_sh2a_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  if ((reg_nr >= gdbarch_fp0_regnum (gdbarch)
       && (reg_nr <= FP_LAST_REGNUM)) || (reg_nr == FPUL_REGNUM))
    return builtin_type (gdbarch)->builtin_float;
  else if (reg_nr >= DR0_REGNUM && reg_nr <= DR_LAST_REGNUM)
    return builtin_type (gdbarch)->builtin_double;
  else
    return builtin_type (gdbarch)->builtin_int;
}

/* Return the GDB type object for the "standard" data type
   of data in register N.  */
static struct type *
sh_sh3e_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  if ((reg_nr >= gdbarch_fp0_regnum (gdbarch)
       && (reg_nr <= FP_LAST_REGNUM)) || (reg_nr == FPUL_REGNUM))
    return builtin_type (gdbarch)->builtin_float;
  else
    return builtin_type (gdbarch)->builtin_int;
}

static struct type *
sh_sh4_build_float_register_type (struct gdbarch *gdbarch, int high)
{
  return lookup_array_range_type (builtin_type (gdbarch)->builtin_float,
				  0, high);
}

static struct type *
sh_sh4_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  if ((reg_nr >= gdbarch_fp0_regnum (gdbarch)
       && (reg_nr <= FP_LAST_REGNUM)) || (reg_nr == FPUL_REGNUM))
    return builtin_type (gdbarch)->builtin_float;
  else if (reg_nr >= DR0_REGNUM && reg_nr <= DR_LAST_REGNUM)
    return builtin_type (gdbarch)->builtin_double;
  else if (reg_nr >= FV0_REGNUM && reg_nr <= FV_LAST_REGNUM)
    return sh_sh4_build_float_register_type (gdbarch, 3);
  else
    return builtin_type (gdbarch)->builtin_int;
}

static struct type *
sh_default_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  return builtin_type (gdbarch)->builtin_int;
}

/* Is a register in a reggroup?
   The default code in reggroup.c doesn't identify system registers, some
   float registers or any of the vector registers.
   TODO: sh2a and dsp registers.  */
static int
sh_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			const struct reggroup *reggroup)
{
  if (*gdbarch_register_name (gdbarch, regnum) == '\0')
    return 0;

  if (reggroup == float_reggroup
      && (regnum == FPUL_REGNUM
	  || regnum == FPSCR_REGNUM))
    return 1;

  if (regnum >= FV0_REGNUM && regnum <= FV_LAST_REGNUM)
    {
      if (reggroup == vector_reggroup || reggroup == float_reggroup)
	return 1;
      if (reggroup == general_reggroup)
	return 0;
    }

  if (regnum == VBR_REGNUM
      || regnum == SR_REGNUM
      || regnum == FPSCR_REGNUM
      || regnum == SSR_REGNUM
      || regnum == SPC_REGNUM)
    {
      if (reggroup == system_reggroup)
	return 1;
      if (reggroup == general_reggroup)
	return 0;
    }

  /* The default code can cope with any other registers.  */
  return default_register_reggroup_p (gdbarch, regnum, reggroup);
}

/* On the sh4, the DRi pseudo registers are problematic if the target
   is little endian.  When the user writes one of those registers, for
   instance with 'set var $dr0=1', we want the double to be stored
   like this: 
   fr0 = 0x00 0x00 0xf0 0x3f 
   fr1 = 0x00 0x00 0x00 0x00 

   This corresponds to little endian byte order & big endian word
   order.  However if we let gdb write the register w/o conversion, it
   will write fr0 and fr1 this way:
   fr0 = 0x00 0x00 0x00 0x00
   fr1 = 0x00 0x00 0xf0 0x3f
   because it will consider fr0 and fr1 as a single LE stretch of memory.
   
   To achieve what we want we must force gdb to store things in
   floatformat_ieee_double_littlebyte_bigword (which is defined in
   include/floatformat.h and libiberty/floatformat.c.

   In case the target is big endian, there is no problem, the
   raw bytes will look like:
   fr0 = 0x3f 0xf0 0x00 0x00
   fr1 = 0x00 0x00 0x00 0x00

   The other pseudo registers (the FVs) also don't pose a problem
   because they are stored as 4 individual FP elements.  */

static struct type *
sh_littlebyte_bigword_type (struct gdbarch *gdbarch)
{
  sh_gdbarch_tdep *tdep = gdbarch_tdep<sh_gdbarch_tdep> (gdbarch);

  if (tdep->sh_littlebyte_bigword_type == NULL)
    {
      type_allocator alloc (gdbarch);
      tdep->sh_littlebyte_bigword_type
	= init_float_type (alloc, -1, "builtin_type_sh_littlebyte_bigword",
			   floatformats_ieee_double_littlebyte_bigword);
    }

  return tdep->sh_littlebyte_bigword_type;
}

static void
sh_register_convert_to_virtual (struct gdbarch *gdbarch, int regnum,
				struct type *type, gdb_byte *from, gdb_byte *to)
{
  if (gdbarch_byte_order (gdbarch) != BFD_ENDIAN_LITTLE)
    {
      /* It is a no-op.  */
      memcpy (to, from, register_size (gdbarch, regnum));
      return;
    }

  if (regnum >= DR0_REGNUM && regnum <= DR_LAST_REGNUM)
    target_float_convert (from, sh_littlebyte_bigword_type (gdbarch),
			  to, type);
  else
    error
      ("sh_register_convert_to_virtual called with non DR register number");
}

static void
sh_register_convert_to_raw (struct gdbarch *gdbarch, struct type *type,
			    int regnum, const gdb_byte *from, gdb_byte *to)
{
  if (gdbarch_byte_order (gdbarch) != BFD_ENDIAN_LITTLE)
    {
      /* It is a no-op.  */
      memcpy (to, from, register_size (gdbarch, regnum));
      return;
    }

  if (regnum >= DR0_REGNUM && regnum <= DR_LAST_REGNUM)
    target_float_convert (from, type,
			  to, sh_littlebyte_bigword_type (gdbarch));
  else
    error (_("sh_register_convert_to_raw called with non DR register number"));
}

/* For vectors of 4 floating point registers.  */
static int
fv_reg_base_num (struct gdbarch *gdbarch, int fv_regnum)
{
  int fp_regnum;

  fp_regnum = gdbarch_fp0_regnum (gdbarch)
	      + (fv_regnum - FV0_REGNUM) * 4;
  return fp_regnum;
}

/* For double precision floating point registers, i.e 2 fp regs.  */
static int
dr_reg_base_num (struct gdbarch *gdbarch, int dr_regnum)
{
  int fp_regnum;

  fp_regnum = gdbarch_fp0_regnum (gdbarch)
	      + (dr_regnum - DR0_REGNUM) * 2;
  return fp_regnum;
}

/* Concatenate PORTIONS contiguous raw registers starting at
   BASE_REGNUM into BUFFER.  */

static enum register_status
pseudo_register_read_portions (struct gdbarch *gdbarch,
			       readable_regcache *regcache,
			       int portions,
			       int base_regnum, gdb_byte *buffer)
{
  int portion;

  for (portion = 0; portion < portions; portion++)
    {
      enum register_status status;
      gdb_byte *b;

      b = buffer + register_size (gdbarch, base_regnum) * portion;
      status = regcache->raw_read (base_regnum + portion, b);
      if (status != REG_VALID)
	return status;
    }

  return REG_VALID;
}

static enum register_status
sh_pseudo_register_read (struct gdbarch *gdbarch, readable_regcache *regcache,
			 int reg_nr, gdb_byte *buffer)
{
  int base_regnum;
  enum register_status status;

  if (reg_nr == PSEUDO_BANK_REGNUM)
    return regcache->raw_read (BANK_REGNUM, buffer);
  else if (reg_nr >= DR0_REGNUM && reg_nr <= DR_LAST_REGNUM)
    {
      /* Enough space for two float registers.  */
      gdb_byte temp_buffer[4 * 2];
      base_regnum = dr_reg_base_num (gdbarch, reg_nr);

      /* Build the value in the provided buffer.  */
      /* Read the real regs for which this one is an alias.  */
      status = pseudo_register_read_portions (gdbarch, regcache,
					      2, base_regnum, temp_buffer);
      if (status == REG_VALID)
	{
	  /* We must pay attention to the endianness. */
	  sh_register_convert_to_virtual (gdbarch, reg_nr,
					  register_type (gdbarch, reg_nr),
					  temp_buffer, buffer);
	}
      return status;
    }
  else if (reg_nr >= FV0_REGNUM && reg_nr <= FV_LAST_REGNUM)
    {
      base_regnum = fv_reg_base_num (gdbarch, reg_nr);

      /* Read the real regs for which this one is an alias.  */
      return pseudo_register_read_portions (gdbarch, regcache,
					    4, base_regnum, buffer);
    }
  else
    gdb_assert_not_reached ("invalid pseudo register number");
}

static void
sh_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			  int reg_nr, const gdb_byte *buffer)
{
  int base_regnum, portion;

  if (reg_nr == PSEUDO_BANK_REGNUM)
    {
      /* When the bank register is written to, the whole register bank
	 is switched and all values in the bank registers must be read
	 from the target/sim again.  We're just invalidating the regcache
	 so that a re-read happens next time it's necessary.  */
      int bregnum;

      regcache->raw_write (BANK_REGNUM, buffer);
      for (bregnum = R0_BANK0_REGNUM; bregnum < MACLB_REGNUM; ++bregnum)
	regcache->invalidate (bregnum);
    }
  else if (reg_nr >= DR0_REGNUM && reg_nr <= DR_LAST_REGNUM)
    {
      /* Enough space for two float registers.  */
      gdb_byte temp_buffer[4 * 2];
      base_regnum = dr_reg_base_num (gdbarch, reg_nr);

      /* We must pay attention to the endianness.  */
      sh_register_convert_to_raw (gdbarch, register_type (gdbarch, reg_nr),
				  reg_nr, buffer, temp_buffer);

      /* Write the real regs for which this one is an alias.  */
      for (portion = 0; portion < 2; portion++)
	regcache->raw_write (base_regnum + portion,
			    (temp_buffer
			     + register_size (gdbarch,
					      base_regnum) * portion));
    }
  else if (reg_nr >= FV0_REGNUM && reg_nr <= FV_LAST_REGNUM)
    {
      base_regnum = fv_reg_base_num (gdbarch, reg_nr);

      /* Write the real regs for which this one is an alias.  */
      for (portion = 0; portion < 4; portion++)
	regcache->raw_write (base_regnum + portion,
			    (buffer
			     + register_size (gdbarch,
					      base_regnum) * portion));
    }
}

static int
sh_dsp_register_sim_regno (struct gdbarch *gdbarch, int nr)
{
  if (legacy_register_sim_regno (gdbarch, nr) < 0)
    return legacy_register_sim_regno (gdbarch, nr);
  if (nr >= DSR_REGNUM && nr <= Y1_REGNUM)
    return nr - DSR_REGNUM + SIM_SH_DSR_REGNUM;
  if (nr == MOD_REGNUM)
    return SIM_SH_MOD_REGNUM;
  if (nr == RS_REGNUM)
    return SIM_SH_RS_REGNUM;
  if (nr == RE_REGNUM)
    return SIM_SH_RE_REGNUM;
  if (nr >= DSP_R0_BANK_REGNUM && nr <= DSP_R7_BANK_REGNUM)
    return nr - DSP_R0_BANK_REGNUM + SIM_SH_R0_BANK_REGNUM;
  return nr;
}

static int
sh_sh2a_register_sim_regno (struct gdbarch *gdbarch, int nr)
{
  switch (nr)
    {
      case TBR_REGNUM:
	return SIM_SH_TBR_REGNUM;
      case IBNR_REGNUM:
	return SIM_SH_IBNR_REGNUM;
      case IBCR_REGNUM:
	return SIM_SH_IBCR_REGNUM;
      case BANK_REGNUM:
	return SIM_SH_BANK_REGNUM;
      case MACLB_REGNUM:
	return SIM_SH_BANK_MACL_REGNUM;
      case GBRB_REGNUM:
	return SIM_SH_BANK_GBR_REGNUM;
      case PRB_REGNUM:
	return SIM_SH_BANK_PR_REGNUM;
      case IVNB_REGNUM:
	return SIM_SH_BANK_IVN_REGNUM;
      case MACHB_REGNUM:
	return SIM_SH_BANK_MACH_REGNUM;
      default:
	break;
    }
  return legacy_register_sim_regno (gdbarch, nr);
}

/* Set up the register unwinding such that call-clobbered registers are
   not displayed in frames >0 because the true value is not certain.
   The 'undefined' registers will show up as 'not available' unless the
   CFI says otherwise.

   This function is currently set up for SH4 and compatible only.  */

static void
sh_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
			  struct dwarf2_frame_state_reg *reg,
			  frame_info_ptr this_frame)
{
  /* Mark the PC as the destination for the return address.  */
  if (regnum == gdbarch_pc_regnum (gdbarch))
    reg->how = DWARF2_FRAME_REG_RA;

  /* Mark the stack pointer as the call frame address.  */
  else if (regnum == gdbarch_sp_regnum (gdbarch))
    reg->how = DWARF2_FRAME_REG_CFA;

  /* The above was taken from the default init_reg in dwarf2-frame.c
     while the below is SH specific.  */

  /* Caller save registers.  */
  else if ((regnum >= R0_REGNUM && regnum <= R0_REGNUM+7)
	   || (regnum >= FR0_REGNUM && regnum <= FR0_REGNUM+11)
	   || (regnum >= DR0_REGNUM && regnum <= DR0_REGNUM+5)
	   || (regnum >= FV0_REGNUM && regnum <= FV0_REGNUM+2)
	   || (regnum == MACH_REGNUM)
	   || (regnum == MACL_REGNUM)
	   || (regnum == FPUL_REGNUM)
	   || (regnum == SR_REGNUM))
    reg->how = DWARF2_FRAME_REG_UNDEFINED;

  /* Callee save registers.  */
  else if ((regnum >= R0_REGNUM+8 && regnum <= R0_REGNUM+15)
	   || (regnum >= FR0_REGNUM+12 && regnum <= FR0_REGNUM+15)
	   || (regnum >= DR0_REGNUM+6 && regnum <= DR0_REGNUM+8)
	   || (regnum == FV0_REGNUM+3))
    reg->how = DWARF2_FRAME_REG_SAME_VALUE;

  /* Other registers.  These are not in the ABI and may or may not
     mean anything in frames >0 so don't show them.  */
  else if ((regnum >= R0_BANK0_REGNUM && regnum <= R0_BANK0_REGNUM+15)
	   || (regnum == GBR_REGNUM)
	   || (regnum == VBR_REGNUM)
	   || (regnum == FPSCR_REGNUM)
	   || (regnum == SSR_REGNUM)
	   || (regnum == SPC_REGNUM))
    reg->how = DWARF2_FRAME_REG_UNDEFINED;
}

static struct sh_frame_cache *
sh_alloc_frame_cache (void)
{
  struct sh_frame_cache *cache;
  int i;

  cache = FRAME_OBSTACK_ZALLOC (struct sh_frame_cache);

  /* Base address.  */
  cache->base = 0;
  cache->saved_sp = 0;
  cache->sp_offset = 0;
  cache->pc = 0;

  /* Frameless until proven otherwise.  */
  cache->uses_fp = 0;

  /* Saved registers.  We initialize these to -1 since zero is a valid
     offset (that's where fp is supposed to be stored).  */
  for (i = 0; i < SH_NUM_REGS; i++)
    {
      cache->saved_regs[i] = -1;
    }

  return cache;
}

static struct sh_frame_cache *
sh_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct sh_frame_cache *cache;
  CORE_ADDR current_pc;
  int i;

  if (*this_cache)
    return (struct sh_frame_cache *) *this_cache;

  cache = sh_alloc_frame_cache ();
  *this_cache = cache;

  /* In principle, for normal frames, fp holds the frame pointer,
     which holds the base address for the current stack frame.
     However, for functions that don't need it, the frame pointer is
     optional.  For these "frameless" functions the frame pointer is
     actually the frame pointer of the calling frame.  */
  cache->base = get_frame_register_unsigned (this_frame, FP_REGNUM);
  if (cache->base == 0)
    return cache;

  cache->pc = get_frame_func (this_frame);
  current_pc = get_frame_pc (this_frame);
  if (cache->pc != 0)
    {
      ULONGEST fpscr;

      /* Check for the existence of the FPSCR register.	 If it exists,
	 fetch its value for use in prologue analysis.	Passing a zero
	 value is the best choice for architecture variants upon which
	 there's no FPSCR register.  */
      if (gdbarch_register_reggroup_p (gdbarch, FPSCR_REGNUM, all_reggroup))
	fpscr = get_frame_register_unsigned (this_frame, FPSCR_REGNUM);
      else
	fpscr = 0;

      sh_analyze_prologue (gdbarch, cache->pc, current_pc, cache, fpscr);
    }

  if (!cache->uses_fp)
    {
      /* We didn't find a valid frame, which means that CACHE->base
	 currently holds the frame pointer for our calling frame.  If
	 we're at the start of a function, or somewhere half-way its
	 prologue, the function's frame probably hasn't been fully
	 setup yet.  Try to reconstruct the base address for the stack
	 frame by looking at the stack pointer.  For truly "frameless"
	 functions this might work too.  */
      cache->base = get_frame_register_unsigned
		     (this_frame, gdbarch_sp_regnum (gdbarch));
    }

  /* Now that we have the base address for the stack frame we can
     calculate the value of sp in the calling frame.  */
  cache->saved_sp = cache->base + cache->sp_offset;

  /* Adjust all the saved registers such that they contain addresses
     instead of offsets.  */
  for (i = 0; i < SH_NUM_REGS; i++)
    if (cache->saved_regs[i] != -1)
      cache->saved_regs[i] = cache->saved_sp - cache->saved_regs[i] - 4;

  return cache;
}

static struct value *
sh_frame_prev_register (frame_info_ptr this_frame,
			void **this_cache, int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct sh_frame_cache *cache = sh_frame_cache (this_frame, this_cache);

  gdb_assert (regnum >= 0);

  if (regnum == gdbarch_sp_regnum (gdbarch) && cache->saved_sp)
    return frame_unwind_got_constant (this_frame, regnum, cache->saved_sp);

  /* The PC of the previous frame is stored in the PR register of
     the current frame.  Frob regnum so that we pull the value from
     the correct place.  */
  if (regnum == gdbarch_pc_regnum (gdbarch))
    regnum = PR_REGNUM;

  if (regnum < SH_NUM_REGS && cache->saved_regs[regnum] != -1)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->saved_regs[regnum]);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static void
sh_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		  struct frame_id *this_id)
{
  struct sh_frame_cache *cache = sh_frame_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  *this_id = frame_id_build (cache->saved_sp, cache->pc);
}

static const struct frame_unwind sh_frame_unwind = {
  "sh prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  sh_frame_this_id,
  sh_frame_prev_register,
  NULL,
  default_frame_sniffer
};

static CORE_ADDR
sh_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct sh_frame_cache *cache = sh_frame_cache (this_frame, this_cache);

  return cache->base;
}

static const struct frame_base sh_frame_base = {
  &sh_frame_unwind,
  sh_frame_base_address,
  sh_frame_base_address,
  sh_frame_base_address
};

static struct sh_frame_cache *
sh_make_stub_cache (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct sh_frame_cache *cache;

  cache = sh_alloc_frame_cache ();

  cache->saved_sp
    = get_frame_register_unsigned (this_frame, gdbarch_sp_regnum (gdbarch));

  return cache;
}

static void
sh_stub_this_id (frame_info_ptr this_frame, void **this_cache,
		 struct frame_id *this_id)
{
  struct sh_frame_cache *cache;

  if (*this_cache == NULL)
    *this_cache = sh_make_stub_cache (this_frame);
  cache = (struct sh_frame_cache *) *this_cache;

  *this_id = frame_id_build (cache->saved_sp, get_frame_pc (this_frame));
}

static int
sh_stub_unwind_sniffer (const struct frame_unwind *self,
			frame_info_ptr this_frame,
			void **this_prologue_cache)
{
  CORE_ADDR addr_in_block;

  addr_in_block = get_frame_address_in_block (this_frame);
  if (in_plt_section (addr_in_block))
    return 1;

  return 0;
}

static const struct frame_unwind sh_stub_unwind =
{
  "sh stub",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  sh_stub_this_id,
  sh_frame_prev_register,
  NULL,
  sh_stub_unwind_sniffer
};

/* Implement the stack_frame_destroyed_p gdbarch method.

   The epilogue is defined here as the area at the end of a function,
   either on the `ret' instruction itself or after an instruction which
   destroys the function's stack frame.  */

static int
sh_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR func_addr = 0, func_end = 0;

  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      ULONGEST inst;
      /* The sh epilogue is max. 14 bytes long.  Give another 14 bytes
	 for a nop and some fixed data (e.g. big offsets) which are
	 unfortunately also treated as part of the function (which
	 means, they are below func_end.  */
      CORE_ADDR addr = func_end - 28;
      if (addr < func_addr + 4)
	addr = func_addr + 4;
      if (pc < addr)
	return 0;

      /* First search forward until hitting an rts.  */
      while (addr < func_end
	     && !IS_RTS (read_memory_unsigned_integer (addr, 2, byte_order)))
	addr += 2;
      if (addr >= func_end)
	return 0;

      /* At this point we should find a mov.l @r15+,r14 instruction,
	 either before or after the rts.  If not, then the function has
	 probably no "normal" epilogue and we bail out here.  */
      inst = read_memory_unsigned_integer (addr - 2, 2, byte_order);
      if (IS_RESTORE_FP (read_memory_unsigned_integer (addr - 2, 2,
						       byte_order)))
	addr -= 2;
      else if (!IS_RESTORE_FP (read_memory_unsigned_integer (addr + 2, 2,
							     byte_order)))
	return 0;

      inst = read_memory_unsigned_integer (addr - 2, 2, byte_order);

      /* Step over possible lds.l @r15+,macl.  */
      if (IS_MACL_LDS (inst))
	{
	  addr -= 2;
	  inst = read_memory_unsigned_integer (addr - 2, 2, byte_order);
	}

      /* Step over possible lds.l @r15+,pr.  */
      if (IS_LDS (inst))
	{
	  addr -= 2;
	  inst = read_memory_unsigned_integer (addr - 2, 2, byte_order);
	}

      /* Step over possible mov r14,r15.  */
      if (IS_MOV_FP_SP (inst))
	{
	  addr -= 2;
	  inst = read_memory_unsigned_integer (addr - 2, 2, byte_order);
	}

      /* Now check for FP adjustments, using add #imm,r14 or add rX, r14
	 instructions.  */
      while (addr > func_addr + 4
	     && (IS_ADD_REG_TO_FP (inst) || IS_ADD_IMM_FP (inst)))
	{
	  addr -= 2;
	  inst = read_memory_unsigned_integer (addr - 2, 2, byte_order);
	}

      /* On SH2a check if the previous instruction was perhaps a MOVI20.
	 That's allowed for the epilogue.  */
      if ((gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_sh2a
	   || gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_sh2a_nofpu)
	  && addr > func_addr + 6
	  && IS_MOVI20 (read_memory_unsigned_integer (addr - 4, 2,
						      byte_order)))
	addr -= 4;

      if (pc >= addr)
	return 1;
    }
  return 0;
}


/* Supply register REGNUM from the buffer specified by REGS and LEN
   in the register set REGSET to register cache REGCACHE.
   REGTABLE specifies where each register can be found in REGS.
   If REGNUM is -1, do this for all registers in REGSET.  */

void
sh_corefile_supply_regset (const struct regset *regset,
			   struct regcache *regcache,
			   int regnum, const void *regs, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
  sh_gdbarch_tdep *tdep = gdbarch_tdep<sh_gdbarch_tdep> (gdbarch);
  const struct sh_corefile_regmap *regmap = (regset == &sh_corefile_gregset
					     ? tdep->core_gregmap
					     : tdep->core_fpregmap);
  int i;

  for (i = 0; regmap[i].regnum != -1; i++)
    {
      if ((regnum == -1 || regnum == regmap[i].regnum)
	  && regmap[i].offset + 4 <= len)
	regcache->raw_supply
	  (regmap[i].regnum, (char *) regs + regmap[i].offset);
    }
}

/* Collect register REGNUM in the register set REGSET from register cache
   REGCACHE into the buffer specified by REGS and LEN.
   REGTABLE specifies where each register can be found in REGS.
   If REGNUM is -1, do this for all registers in REGSET.  */

void
sh_corefile_collect_regset (const struct regset *regset,
			    const struct regcache *regcache,
			    int regnum, void *regs, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
  sh_gdbarch_tdep *tdep = gdbarch_tdep<sh_gdbarch_tdep> (gdbarch);
  const struct sh_corefile_regmap *regmap = (regset == &sh_corefile_gregset
					     ? tdep->core_gregmap
					     : tdep->core_fpregmap);
  int i;

  for (i = 0; regmap[i].regnum != -1; i++)
    {
      if ((regnum == -1 || regnum == regmap[i].regnum)
	  && regmap[i].offset + 4 <= len)
	regcache->raw_collect (regmap[i].regnum,
			      (char *)regs + regmap[i].offset);
    }
}

/* The following two regsets have the same contents, so it is tempting to
   unify them, but they are distiguished by their address, so don't.  */

const struct regset sh_corefile_gregset =
{
  NULL,
  sh_corefile_supply_regset,
  sh_corefile_collect_regset
};

static const struct regset sh_corefile_fpregset =
{
  NULL,
  sh_corefile_supply_regset,
  sh_corefile_collect_regset
};

static void
sh_iterate_over_regset_sections (struct gdbarch *gdbarch,
				 iterate_over_regset_sections_cb *cb,
				 void *cb_data,
				 const struct regcache *regcache)
{
  sh_gdbarch_tdep *tdep = gdbarch_tdep<sh_gdbarch_tdep> (gdbarch);

  if (tdep->core_gregmap != NULL)
    cb (".reg", tdep->sizeof_gregset, tdep->sizeof_gregset,
	&sh_corefile_gregset, NULL, cb_data);

  if (tdep->core_fpregmap != NULL)
    cb (".reg2", tdep->sizeof_fpregset, tdep->sizeof_fpregset,
	&sh_corefile_fpregset, NULL, cb_data);
}

/* This is the implementation of gdbarch method
   return_in_first_hidden_param_p.  */

static int
sh_return_in_first_hidden_param_p (struct gdbarch *gdbarch,
				     struct type *type)
{
  return 0;
}



static struct gdbarch *
sh_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  /* If there is already a candidate, use it.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* None found, create a new architecture from the information
     provided.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new sh_gdbarch_tdep));

  set_gdbarch_short_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_int_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_long_bit (gdbarch, 8 * TARGET_CHAR_BIT);

  set_gdbarch_wchar_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_wchar_signed (gdbarch, 0);

  set_gdbarch_float_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_double_bit (gdbarch, 8 * TARGET_CHAR_BIT);
  set_gdbarch_long_double_bit (gdbarch, 8 * TARGET_CHAR_BIT);
  set_gdbarch_ptr_bit (gdbarch, 4 * TARGET_CHAR_BIT);

  set_gdbarch_num_regs (gdbarch, SH_NUM_REGS);
  set_gdbarch_sp_regnum (gdbarch, 15);
  set_gdbarch_pc_regnum (gdbarch, 16);
  set_gdbarch_fp0_regnum (gdbarch, -1);
  set_gdbarch_num_pseudo_regs (gdbarch, 0);

  set_gdbarch_register_type (gdbarch, sh_default_register_type);
  set_gdbarch_register_reggroup_p (gdbarch, sh_register_reggroup_p);

  set_gdbarch_breakpoint_kind_from_pc (gdbarch, sh_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, sh_sw_breakpoint_from_kind);

  set_gdbarch_register_sim_regno (gdbarch, legacy_register_sim_regno);

  set_gdbarch_return_value (gdbarch, sh_return_value_nofpu);

  set_gdbarch_skip_prologue (gdbarch, sh_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_push_dummy_call (gdbarch, sh_push_dummy_call_nofpu);
  set_gdbarch_return_in_first_hidden_param_p (gdbarch,
					      sh_return_in_first_hidden_param_p);

  set_gdbarch_believe_pcc_promotion (gdbarch, 1);

  set_gdbarch_frame_align (gdbarch, sh_frame_align);
  frame_base_set_default (gdbarch, &sh_frame_base);

  set_gdbarch_stack_frame_destroyed_p (gdbarch, sh_stack_frame_destroyed_p);

  dwarf2_frame_set_init_reg (gdbarch, sh_dwarf2_frame_init_reg);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, sh_iterate_over_regset_sections);

  switch (info.bfd_arch_info->mach)
    {
    case bfd_mach_sh:
      set_gdbarch_register_name (gdbarch, sh_sh_register_name);
      break;

    case bfd_mach_sh2:
      set_gdbarch_register_name (gdbarch, sh_sh_register_name);
      break;

    case bfd_mach_sh2e:
      /* doubles on sh2e and sh3e are actually 4 byte.  */
      set_gdbarch_double_bit (gdbarch, 4 * TARGET_CHAR_BIT);
      set_gdbarch_double_format (gdbarch, floatformats_ieee_single);

      set_gdbarch_register_name (gdbarch, sh_sh2e_register_name);
      set_gdbarch_register_type (gdbarch, sh_sh3e_register_type);
      set_gdbarch_fp0_regnum (gdbarch, 25);
      set_gdbarch_return_value (gdbarch, sh_return_value_fpu);
      set_gdbarch_push_dummy_call (gdbarch, sh_push_dummy_call_fpu);
      break;

    case bfd_mach_sh2a:
      set_gdbarch_register_name (gdbarch, sh_sh2a_register_name);
      set_gdbarch_register_type (gdbarch, sh_sh2a_register_type);
      set_gdbarch_register_sim_regno (gdbarch, sh_sh2a_register_sim_regno);

      set_gdbarch_fp0_regnum (gdbarch, 25);
      set_gdbarch_num_pseudo_regs (gdbarch, 9);
      set_gdbarch_pseudo_register_read (gdbarch, sh_pseudo_register_read);
      set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						    sh_pseudo_register_write);
      set_gdbarch_return_value (gdbarch, sh_return_value_fpu);
      set_gdbarch_push_dummy_call (gdbarch, sh_push_dummy_call_fpu);
      break;

    case bfd_mach_sh2a_nofpu:
      set_gdbarch_register_name (gdbarch, sh_sh2a_nofpu_register_name);
      set_gdbarch_register_sim_regno (gdbarch, sh_sh2a_register_sim_regno);

      set_gdbarch_num_pseudo_regs (gdbarch, 1);
      set_gdbarch_pseudo_register_read (gdbarch, sh_pseudo_register_read);
      set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						    sh_pseudo_register_write);
      break;

    case bfd_mach_sh_dsp:
      set_gdbarch_register_name (gdbarch, sh_sh_dsp_register_name);
      set_gdbarch_register_sim_regno (gdbarch, sh_dsp_register_sim_regno);
      break;

    case bfd_mach_sh3:
    case bfd_mach_sh3_nommu:
    case bfd_mach_sh2a_nofpu_or_sh3_nommu:
      set_gdbarch_register_name (gdbarch, sh_sh3_register_name);
      break;

    case bfd_mach_sh3e:
    case bfd_mach_sh2a_or_sh3e:
      /* doubles on sh2e and sh3e are actually 4 byte.  */
      set_gdbarch_double_bit (gdbarch, 4 * TARGET_CHAR_BIT);
      set_gdbarch_double_format (gdbarch, floatformats_ieee_single);

      set_gdbarch_register_name (gdbarch, sh_sh3e_register_name);
      set_gdbarch_register_type (gdbarch, sh_sh3e_register_type);
      set_gdbarch_fp0_regnum (gdbarch, 25);
      set_gdbarch_return_value (gdbarch, sh_return_value_fpu);
      set_gdbarch_push_dummy_call (gdbarch, sh_push_dummy_call_fpu);
      break;

    case bfd_mach_sh3_dsp:
      set_gdbarch_register_name (gdbarch, sh_sh3_dsp_register_name);
      set_gdbarch_register_sim_regno (gdbarch, sh_dsp_register_sim_regno);
      break;

    case bfd_mach_sh4:
    case bfd_mach_sh4a:
    case bfd_mach_sh2a_or_sh4:
      set_gdbarch_register_name (gdbarch, sh_sh4_register_name);
      set_gdbarch_register_type (gdbarch, sh_sh4_register_type);
      set_gdbarch_fp0_regnum (gdbarch, 25);
      set_gdbarch_num_pseudo_regs (gdbarch, 13);
      set_gdbarch_pseudo_register_read (gdbarch, sh_pseudo_register_read);
      set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						    sh_pseudo_register_write);
      set_gdbarch_return_value (gdbarch, sh_return_value_fpu);
      set_gdbarch_push_dummy_call (gdbarch, sh_push_dummy_call_fpu);
      break;

    case bfd_mach_sh4_nofpu:
    case bfd_mach_sh4a_nofpu:
    case bfd_mach_sh4_nommu_nofpu:
    case bfd_mach_sh2a_nofpu_or_sh4_nommu_nofpu:
      set_gdbarch_register_name (gdbarch, sh_sh4_nofpu_register_name);
      break;

    case bfd_mach_sh4al_dsp:
      set_gdbarch_register_name (gdbarch, sh_sh4al_dsp_register_name);
      set_gdbarch_register_sim_regno (gdbarch, sh_dsp_register_sim_regno);
      break;

    default:
      set_gdbarch_register_name (gdbarch, sh_sh_register_name);
      break;
    }

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &sh_stub_unwind);
  frame_unwind_append_unwinder (gdbarch, &sh_frame_unwind);

  return gdbarch;
}

void _initialize_sh_tdep ();
void
_initialize_sh_tdep ()
{
  gdbarch_register (bfd_arch_sh, sh_gdbarch_init, NULL);

  add_setshow_prefix_cmd ("sh", no_class,
			  _("SH specific commands."),
			  _("SH specific commands."),
			  &setshcmdlist, &showshcmdlist,
			  &setlist, &showlist);
  
  add_setshow_enum_cmd ("calling-convention", class_vars, sh_cc_enum,
			&sh_active_calling_convention,
			_("Set calling convention used when calling target "
			  "functions from GDB."),
			_("Show calling convention used when calling target "
			  "functions from GDB."),
			_("gcc       - Use GCC calling convention (default).\n"
			  "renesas   - Enforce Renesas calling convention."),
			NULL, NULL,
			&setshcmdlist, &showshcmdlist);
}
