/* Target-dependent code for the NEC V850 for GDB, the GNU debugger.

   Copyright (C) 1996-2024 Free Software Foundation, Inc.

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
#include "frame-base.h"
#include "trad-frame.h"
#include "frame-unwind.h"
#include "dwarf2/frame.h"
#include "gdbtypes.h"
#include "inferior.h"
#include "gdbcore.h"
#include "arch-utils.h"
#include "regcache.h"
#include "dis-asm.h"
#include "osabi.h"
#include "elf-bfd.h"
#include "elf/v850.h"
#include "gdbarch.h"

enum
  {
    /* General purpose registers.  */
    E_R0_REGNUM,
    E_R1_REGNUM,
    E_R2_REGNUM,
    E_R3_REGNUM, E_SP_REGNUM = E_R3_REGNUM,
    E_R4_REGNUM,
    E_R5_REGNUM,
    E_R6_REGNUM, E_ARG0_REGNUM = E_R6_REGNUM,
    E_R7_REGNUM,
    E_R8_REGNUM,
    E_R9_REGNUM, E_ARGLAST_REGNUM = E_R9_REGNUM,
    E_R10_REGNUM, E_V0_REGNUM = E_R10_REGNUM,
    E_R11_REGNUM, E_V1_REGNUM = E_R11_REGNUM,
    E_R12_REGNUM,
    E_R13_REGNUM,
    E_R14_REGNUM,
    E_R15_REGNUM,
    E_R16_REGNUM,
    E_R17_REGNUM,
    E_R18_REGNUM,
    E_R19_REGNUM,
    E_R20_REGNUM,
    E_R21_REGNUM,
    E_R22_REGNUM,
    E_R23_REGNUM,
    E_R24_REGNUM,
    E_R25_REGNUM,
    E_R26_REGNUM,
    E_R27_REGNUM,
    E_R28_REGNUM,
    E_R29_REGNUM, E_FP_REGNUM = E_R29_REGNUM,
    E_R30_REGNUM, E_EP_REGNUM = E_R30_REGNUM,
    E_R31_REGNUM, E_LP_REGNUM = E_R31_REGNUM,

    /* System registers - main banks.  */
    E_R32_REGNUM, E_SR0_REGNUM = E_R32_REGNUM,
    E_R33_REGNUM,
    E_R34_REGNUM,
    E_R35_REGNUM,
    E_R36_REGNUM,
    E_R37_REGNUM, E_PS_REGNUM = E_R37_REGNUM,
    E_R38_REGNUM,
    E_R39_REGNUM,
    E_R40_REGNUM,
    E_R41_REGNUM,
    E_R42_REGNUM,
    E_R43_REGNUM,
    E_R44_REGNUM,
    E_R45_REGNUM,
    E_R46_REGNUM,
    E_R47_REGNUM,
    E_R48_REGNUM,
    E_R49_REGNUM,
    E_R50_REGNUM,
    E_R51_REGNUM,
    E_R52_REGNUM, E_CTBP_REGNUM = E_R52_REGNUM,
    E_R53_REGNUM,
    E_R54_REGNUM,
    E_R55_REGNUM,
    E_R56_REGNUM,
    E_R57_REGNUM,
    E_R58_REGNUM,
    E_R59_REGNUM,
    E_R60_REGNUM,
    E_R61_REGNUM,
    E_R62_REGNUM,
    E_R63_REGNUM,

    /* PC.  */
    E_R64_REGNUM, E_PC_REGNUM = E_R64_REGNUM,
    E_R65_REGNUM,
    E_NUM_OF_V850_REGS,
    E_NUM_OF_V850E_REGS = E_NUM_OF_V850_REGS,

    /* System registers - MPV (PROT00) bank.  */
    E_R66_REGNUM = E_NUM_OF_V850_REGS,
    E_R67_REGNUM,
    E_R68_REGNUM,
    E_R69_REGNUM,
    E_R70_REGNUM,
    E_R71_REGNUM,
    E_R72_REGNUM,
    E_R73_REGNUM,
    E_R74_REGNUM,
    E_R75_REGNUM,
    E_R76_REGNUM,
    E_R77_REGNUM,
    E_R78_REGNUM,
    E_R79_REGNUM,
    E_R80_REGNUM,
    E_R81_REGNUM,
    E_R82_REGNUM,
    E_R83_REGNUM,
    E_R84_REGNUM,
    E_R85_REGNUM,
    E_R86_REGNUM,
    E_R87_REGNUM,
    E_R88_REGNUM,
    E_R89_REGNUM,
    E_R90_REGNUM,
    E_R91_REGNUM,
    E_R92_REGNUM,
    E_R93_REGNUM,

    /* System registers - MPU (PROT01) bank.  */
    E_R94_REGNUM,
    E_R95_REGNUM,
    E_R96_REGNUM,
    E_R97_REGNUM,
    E_R98_REGNUM,
    E_R99_REGNUM,
    E_R100_REGNUM,
    E_R101_REGNUM,
    E_R102_REGNUM,
    E_R103_REGNUM,
    E_R104_REGNUM,
    E_R105_REGNUM,
    E_R106_REGNUM,
    E_R107_REGNUM,
    E_R108_REGNUM,
    E_R109_REGNUM,
    E_R110_REGNUM,
    E_R111_REGNUM,
    E_R112_REGNUM,
    E_R113_REGNUM,
    E_R114_REGNUM,
    E_R115_REGNUM,
    E_R116_REGNUM,
    E_R117_REGNUM,
    E_R118_REGNUM,
    E_R119_REGNUM,
    E_R120_REGNUM,
    E_R121_REGNUM,

    /* FPU system registers.  */
    E_R122_REGNUM,
    E_R123_REGNUM,
    E_R124_REGNUM,
    E_R125_REGNUM,
    E_R126_REGNUM,
    E_R127_REGNUM,
    E_R128_REGNUM, E_FPSR_REGNUM = E_R128_REGNUM,
    E_R129_REGNUM, E_FPEPC_REGNUM = E_R129_REGNUM,
    E_R130_REGNUM, E_FPST_REGNUM = E_R130_REGNUM,
    E_R131_REGNUM, E_FPCC_REGNUM = E_R131_REGNUM,
    E_R132_REGNUM, E_FPCFG_REGNUM = E_R132_REGNUM,
    E_R133_REGNUM,
    E_R134_REGNUM,
    E_R135_REGNUM,
    E_R136_REGNUM,
    E_R137_REGNUM,
    E_R138_REGNUM,
    E_R139_REGNUM,
    E_R140_REGNUM,
    E_R141_REGNUM,
    E_R142_REGNUM,
    E_R143_REGNUM,
    E_R144_REGNUM,
    E_R145_REGNUM,
    E_R146_REGNUM,
    E_R147_REGNUM,
    E_R148_REGNUM,
    E_R149_REGNUM,
    E_NUM_OF_V850E2_REGS,

    /* v850e3v5 system registers, selID 1 thru 7.  */
    E_SELID_1_R0_REGNUM = E_NUM_OF_V850E2_REGS,
    E_SELID_1_R31_REGNUM = E_SELID_1_R0_REGNUM + 31,

    E_SELID_2_R0_REGNUM,
    E_SELID_2_R31_REGNUM = E_SELID_2_R0_REGNUM + 31,

    E_SELID_3_R0_REGNUM,
    E_SELID_3_R31_REGNUM = E_SELID_3_R0_REGNUM + 31,

    E_SELID_4_R0_REGNUM,
    E_SELID_4_R31_REGNUM = E_SELID_4_R0_REGNUM + 31,

    E_SELID_5_R0_REGNUM,
    E_SELID_5_R31_REGNUM = E_SELID_5_R0_REGNUM + 31,

    E_SELID_6_R0_REGNUM,
    E_SELID_6_R31_REGNUM = E_SELID_6_R0_REGNUM + 31,

    E_SELID_7_R0_REGNUM,
    E_SELID_7_R31_REGNUM = E_SELID_7_R0_REGNUM + 31,

    /* v850e3v5 vector registers.  */
    E_VR0_REGNUM,
    E_VR31_REGNUM = E_VR0_REGNUM + 31,

    E_NUM_OF_V850E3V5_REGS,

    /* Total number of possible registers.  */
    E_NUM_REGS = E_NUM_OF_V850E3V5_REGS
  };

enum
{
  v850_reg_size = 4
};

/* Size of return datatype which fits into all return registers.  */
enum
{
  E_MAX_RETTYPE_SIZE_IN_REGS = 2 * v850_reg_size
};

/* When v850 support was added to GCC in the late nineties, the intention
   was to follow the Green Hills ABI for v850.  In fact, the authors of
   that support at the time thought that they were doing so.  As far as
   I can tell, the calling conventions are correct, but the return value
   conventions were not quite right.  Over time, the return value code
   in this file was modified to mostly reflect what GCC was actually
   doing instead of to actually follow the Green Hills ABI as it did
   when the code was first written.

   Renesas defined the RH850 ABI which they use in their compiler.  It
   is similar to the original Green Hills ABI with some minor
   differences.  */

enum v850_abi
{
  V850_ABI_GCC,
  V850_ABI_RH850
};

/* Architecture specific data.  */

struct v850_gdbarch_tdep : gdbarch_tdep_base
{
  /* Fields from the ELF header.  */
  int e_flags = 0;
  int e_machine = 0;

  /* Which ABI are we using?  */
  enum v850_abi abi {};
  int eight_byte_align = 0;
};

struct v850_frame_cache
{ 
  /* Base address.  */
  CORE_ADDR base;
  LONGEST sp_offset;
  CORE_ADDR pc;
  
  /* Flag showing that a frame has been created in the prologue code.  */
  int uses_fp;
  
  /* Saved registers.  */
  trad_frame_saved_reg *saved_regs;
};

/* Info gleaned from scanning a function's prologue.  */
struct pifsr		/* Info about one saved register.  */
{
  int offset;		/* Offset from sp or fp.  */
  int cur_frameoffset;	/* Current frameoffset.  */
  int reg;		/* Saved register number.  */
};

static const char *
v850_register_name (struct gdbarch *gdbarch, int regnum)
{
  static const char *v850_reg_names[] =
  { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", 
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", 
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", 
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
    "eipc", "eipsw", "fepc", "fepsw", "ecr", "psw", "sr6", "sr7",
    "sr8", "sr9", "sr10", "sr11", "sr12", "sr13", "sr14", "sr15",
    "sr16", "sr17", "sr18", "sr19", "sr20", "sr21", "sr22", "sr23",
    "sr24", "sr25", "sr26", "sr27", "sr28", "sr29", "sr30", "sr31",
    "pc", "fp"
  };
  static_assert (E_NUM_OF_V850_REGS == ARRAY_SIZE (v850_reg_names));
  return v850_reg_names[regnum];
}

static const char *
v850e_register_name (struct gdbarch *gdbarch, int regnum)
{
  static const char *v850e_reg_names[] =
  {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
    "eipc", "eipsw", "fepc", "fepsw", "ecr", "psw", "sr6", "sr7",
    "sr8", "sr9", "sr10", "sr11", "sr12", "sr13", "sr14", "sr15",
    "ctpc", "ctpsw", "dbpc", "dbpsw", "ctbp", "sr21", "sr22", "sr23",
    "sr24", "sr25", "sr26", "sr27", "sr28", "sr29", "sr30", "sr31",
    "pc", "fp"
  };
  static_assert (E_NUM_OF_V850E_REGS == ARRAY_SIZE (v850e_reg_names));
  return v850e_reg_names[regnum];
}

static const char *
v850e2_register_name (struct gdbarch *gdbarch, int regnum)
{
  static const char *v850e2_reg_names[] =
  {
    /* General purpose registers.  */
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",

    /* System registers - main banks.  */
    "eipc", "eipsw", "fepc", "fepsw", "ecr", "psw", "pid", "cfg",
    "", "", "", "sccfg", "scbp", "eiic", "feic", "dbic",
    "ctpc", "ctpsw", "dbpc", "dbpsw", "ctbp", "dir", "", "",
    "", "", "", "", "eiwr", "fewr", "dbwr", "bsel",


    /* PC.  */
    "pc", "",

    /* System registers - MPV (PROT00) bank.  */
    "vsecr", "vstid", "vsadr", "", "vmecr", "vmtid", "vmadr", "",
    "vpecr", "vptid", "vpadr", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "mca", "mcs", "mcc", "mcr",

    /* System registers - MPU (PROT01) bank.  */
    "mpm", "mpc", "tid", "", "", "", "ipa0l", "ipa0u",
    "ipa1l", "ipa1u", "ipa2l", "ipa2u", "ipa3l", "ipa3u", "ipa4l", "ipa4u",
    "dpa0l", "dpa0u", "dpa1l", "dpa1u", "dpa2l", "dpa2u", "dpa3l", "dpa3u",
    "dpa4l", "dpa4u", "dpa5l", "dpa5u",

    /* FPU system registers.  */
    "", "", "", "", "", "", "fpsr", "fpepc",
    "fpst", "fpcc", "fpcfg", "fpec", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "fpspc"
  };
  if (regnum >= E_NUM_OF_V850E2_REGS)
    return "";
  return v850e2_reg_names[regnum];
}

/* Implement the "register_name" gdbarch method for v850e3v5.  */

static const char *
v850e3v5_register_name (struct gdbarch *gdbarch, int regnum)
{
  static const char *v850e3v5_reg_names[] =
  {
    /* General purpose registers.  */
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",

    /* selID 0, not including FPU registers.  The FPU registers are
       listed later on.  */
    "eipc", "eipsw", "fepc", "fepsw",
    "", "psw", "" /* fpsr */, "" /* fpepc */,
    "" /* fpst */, "" /* fpcc */, "" /* fpcfg */, "" /* fpec */,
    "sesr", "eiic", "feic", "",
    "ctpc", "ctpsw", "", "", "ctbp", "", "", "",
    "", "", "", "", "eiwr", "fewr", "", "bsel",


    /* PC.  */
    "pc", "",

    /* v850e2 MPV bank.  */
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "",

    /* Skip v850e2 MPU bank.  It's tempting to reuse these, but we need
       32 entries for this bank.  */
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "",

    /* FPU system registers.  These are actually in selID 0, but
       are placed here to preserve register numbering compatibility
       with previous architectures.  */
    "", "", "", "", "", "", "fpsr", "fpepc",
    "fpst", "fpcc", "fpcfg", "fpec", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "",

    /* selID 1.  */
    "mcfg0", "mcfg1", "rbase", "ebase", "intbp", "mctl", "pid", "fpipr",
    "", "", "tcsel", "sccfg", "scbp", "hvccfg", "hvcbp", "vsel",
    "vmprt0", "vmprt1", "vmprt2", "", "", "", "", "vmscctl",
    "vmsctbl0", "vmsctbl1", "vmsctbl2", "vmsctbl3", "", "", "", "",

    /* selID 2.  */
    "htcfg0", "", "", "", "", "htctl", "mea", "asid",
    "mei", "ispr", "pmr", "icsr", "intcfg", "", "", "",
    "tlbsch", "", "", "", "", "", "", "htscctl",
    "htsctbl0", "htsctbl1", "htsctbl2", "htsctbl3",
    "htsctbl4", "htsctbl5", "htsctbl6", "htsctbl7",

    /* selID 3.  */
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",

    /* selID 4.  */
    "tlbidx", "", "", "", "telo0", "telo1", "tehi0", "tehi1",
    "", "", "tlbcfg", "", "bwerrl", "bwerrh", "brerrl", "brerrh",
    "ictagl", "ictagh", "icdatl", "icdath",
    "dctagl", "dctagh", "dcdatl", "dcdath",
    "icctrl", "dcctrl", "iccfg", "dccfg", "icerr", "dcerr", "", "",

    /* selID 5.  */
    "mpm", "mprc", "", "", "mpbrgn", "mptrgn", "", "",
    "mca", "mcs", "mcc", "mcr", "", "", "", "",
    "", "", "", "", "mpprt0", "mpprt1", "mpprt2", "",
    "", "", "", "", "", "", "", "",

    /* selID 6.  */
    "mpla0", "mpua0", "mpat0", "", "mpla1", "mpua1", "mpat1", "",
    "mpla2", "mpua2", "mpat2", "", "mpla3", "mpua3", "mpat3", "",
    "mpla4", "mpua4", "mpat4", "", "mpla5", "mpua5", "mpat5", "",
    "mpla6", "mpua6", "mpat6", "", "mpla7", "mpua7", "mpat7", "",

    /* selID 7.  */
    "mpla8", "mpua8", "mpat8", "", "mpla9", "mpua9", "mpat9", "",
    "mpla10", "mpua10", "mpat10", "", "mpla11", "mpua11", "mpat11", "",
    "mpla12", "mpua12", "mpat12", "", "mpla13", "mpua13", "mpat13", "",
    "mpla14", "mpua14", "mpat14", "", "mpla15", "mpua15", "mpat15", "",

    /* Vector Registers */
    "vr0", "vr1", "vr2", "vr3", "vr4", "vr5", "vr6", "vr7",
    "vr8", "vr9", "vr10", "vr11", "vr12", "vr13", "vr14", "vr15",
    "vr16", "vr17", "vr18", "vr19", "vr20", "vr21", "vr22", "vr23",
    "vr24", "vr25", "vr26", "vr27", "vr28", "vr29", "vr30", "vr31",
  };

  static_assert (E_NUM_OF_V850E3V5_REGS
		     == ARRAY_SIZE (v850e3v5_reg_names));
  return v850e3v5_reg_names[regnum];
}

/* Returns the default type for register N.  */

static struct type *
v850_register_type (struct gdbarch *gdbarch, int regnum)
{
  if (regnum == E_PC_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;
  else if (E_VR0_REGNUM <= regnum && regnum <= E_VR31_REGNUM)
    return builtin_type (gdbarch)->builtin_uint64;
  return builtin_type (gdbarch)->builtin_int32;
}

static int
v850_type_is_scalar (struct type *t)
{
  return (t->code () != TYPE_CODE_STRUCT
	  && t->code () != TYPE_CODE_UNION
	  && t->code () != TYPE_CODE_ARRAY);
}

/* Should call_function allocate stack space for a struct return?  */

static int
v850_use_struct_convention (struct gdbarch *gdbarch, struct type *type)
{
  int i;
  struct type *fld_type, *tgt_type;
  v850_gdbarch_tdep *tdep = gdbarch_tdep<v850_gdbarch_tdep> (gdbarch);

  if (tdep->abi == V850_ABI_RH850)
    {
      if (v850_type_is_scalar (type) && type->length () <= 8)
	return 0;

      /* Structs are never returned in registers for this ABI.  */
      return 1;
    }
  /* 1. The value is greater than 8 bytes -> returned by copying.  */
  if (type->length () > 8)
    return 1;

  /* 2. The value is a single basic type -> returned in register.  */
  if (v850_type_is_scalar (type))
    return 0;

  /* The value is a structure or union with a single element and that
     element is either a single basic type or an array of a single basic
     type whose size is greater than or equal to 4 -> returned in register.  */
  if ((type->code () == TYPE_CODE_STRUCT
       || type->code () == TYPE_CODE_UNION)
       && type->num_fields () == 1)
    {
      fld_type = type->field (0).type ();
      if (v850_type_is_scalar (fld_type) && fld_type->length () >= 4)
	return 0;

      if (fld_type->code () == TYPE_CODE_ARRAY)
	{
	  tgt_type = fld_type->target_type ();
	  if (v850_type_is_scalar (tgt_type) && tgt_type->length () >= 4)
	    return 0;
	}
    }

  /* The value is a structure whose first element is an integer or a float,
     and which contains no arrays of more than two elements -> returned in
     register.  */
  if (type->code () == TYPE_CODE_STRUCT
      && v850_type_is_scalar (type->field (0).type ())
      && type->field (0).type ()->length () == 4)
    {
      for (i = 1; i < type->num_fields (); ++i)
	{
	  fld_type = type->field (0).type ();
	  if (fld_type->code () == TYPE_CODE_ARRAY)
	    {
	      tgt_type = fld_type->target_type ();
	      if (tgt_type->length () > 0
		  && fld_type->length () / tgt_type->length () > 2)
		return 1;
	    }
	}
      return 0;
    }
    
  /* The value is a union which contains at least one field which
     would be returned in registers according to these rules ->
     returned in register.  */
  if (type->code () == TYPE_CODE_UNION)
    {
      for (i = 0; i < type->num_fields (); ++i)
	{
	  fld_type = type->field (0).type ();
	  if (!v850_use_struct_convention (gdbarch, fld_type))
	    return 0;
	}
    }

  return 1;
}

/* Structure for mapping bits in register lists to register numbers.  */

struct reg_list
{
  long mask;
  int regno;
};

/* Helper function for v850_scan_prologue to handle prepare instruction.  */

static void
v850_handle_prepare (int insn, int insn2, CORE_ADDR * current_pc_ptr,
		     struct v850_frame_cache *pi, struct pifsr **pifsr_ptr)
{
  CORE_ADDR current_pc = *current_pc_ptr;
  struct pifsr *pifsr = *pifsr_ptr;
  long next = insn2 & 0xffff;
  long list12 = ((insn & 1) << 16) + (next & 0xffe0);
  long offset = (insn & 0x3e) << 1;
  static struct reg_list reg_table[] =
  {
    {0x00800, 20},		/* r20 */
    {0x00400, 21},		/* r21 */
    {0x00200, 22},		/* r22 */
    {0x00100, 23},		/* r23 */
    {0x08000, 24},		/* r24 */
    {0x04000, 25},		/* r25 */
    {0x02000, 26},		/* r26 */
    {0x01000, 27},		/* r27 */
    {0x00080, 28},		/* r28 */
    {0x00040, 29},		/* r29 */
    {0x10000, 30},		/* ep */
    {0x00020, 31},		/* lp */
    {0, 0}			/* end of table */
  };
  int i;

  if ((next & 0x1f) == 0x0b)		/* skip imm16 argument */
    current_pc += 2;
  else if ((next & 0x1f) == 0x13)	/* skip imm16 argument */
    current_pc += 2;
  else if ((next & 0x1f) == 0x1b)	/* skip imm32 argument */
    current_pc += 4;

  /* Calculate the total size of the saved registers, and add it to the
     immediate value used to adjust SP.  */
  for (i = 0; reg_table[i].mask != 0; i++)
    if (list12 & reg_table[i].mask)
      offset += v850_reg_size;
  pi->sp_offset -= offset;

  /* Calculate the offsets of the registers relative to the value the SP
     will have after the registers have been pushed and the imm5 value has
     been subtracted from it.  */
  if (pifsr)
    {
      for (i = 0; reg_table[i].mask != 0; i++)
	{
	  if (list12 & reg_table[i].mask)
	    {
	      int reg = reg_table[i].regno;
	      offset -= v850_reg_size;
	      pifsr->reg = reg;
	      pifsr->offset = offset;
	      pifsr->cur_frameoffset = pi->sp_offset;
	      pifsr++;
	    }
	}
    }

  /* Set result parameters.  */
  *current_pc_ptr = current_pc;
  *pifsr_ptr = pifsr;
}


/* Helper function for v850_scan_prologue to handle pushm/pushl instructions.
   The SR bit of the register list is not supported.  gcc does not generate
   this bit.  */

static void
v850_handle_pushm (int insn, int insn2, struct v850_frame_cache *pi,
		   struct pifsr **pifsr_ptr)
{
  struct pifsr *pifsr = *pifsr_ptr;
  long list12 = ((insn & 0x0f) << 16) + (insn2 & 0xfff0);
  long offset = 0;
  static struct reg_list pushml_reg_table[] =
  {
    {0x80000, E_PS_REGNUM},	/* PSW */
    {0x40000, 1},		/* r1 */
    {0x20000, 2},		/* r2 */
    {0x10000, 3},		/* r3 */
    {0x00800, 4},		/* r4 */
    {0x00400, 5},		/* r5 */
    {0x00200, 6},		/* r6 */
    {0x00100, 7},		/* r7 */
    {0x08000, 8},		/* r8 */
    {0x04000, 9},		/* r9 */
    {0x02000, 10},		/* r10 */
    {0x01000, 11},		/* r11 */
    {0x00080, 12},		/* r12 */
    {0x00040, 13},		/* r13 */
    {0x00020, 14},		/* r14 */
    {0x00010, 15},		/* r15 */
    {0, 0}			/* end of table */
  };
  static struct reg_list pushmh_reg_table[] =
  {
    {0x80000, 16},		/* r16 */
    {0x40000, 17},		/* r17 */
    {0x20000, 18},		/* r18 */
    {0x10000, 19},		/* r19 */
    {0x00800, 20},		/* r20 */
    {0x00400, 21},		/* r21 */
    {0x00200, 22},		/* r22 */
    {0x00100, 23},		/* r23 */
    {0x08000, 24},		/* r24 */
    {0x04000, 25},		/* r25 */
    {0x02000, 26},		/* r26 */
    {0x01000, 27},		/* r27 */
    {0x00080, 28},		/* r28 */
    {0x00040, 29},		/* r29 */
    {0x00010, 30},		/* r30 */
    {0x00020, 31},		/* r31 */
    {0, 0}			/* end of table */
  };
  struct reg_list *reg_table;
  int i;

  /* Is this a pushml or a pushmh?  */
  if ((insn2 & 7) == 1)
    reg_table = pushml_reg_table;
  else
    reg_table = pushmh_reg_table;

  /* Calculate the total size of the saved registers, and add it to the
     immediate value used to adjust SP.  */
  for (i = 0; reg_table[i].mask != 0; i++)
    if (list12 & reg_table[i].mask)
      offset += v850_reg_size;
  pi->sp_offset -= offset;

  /* Calculate the offsets of the registers relative to the value the SP
     will have after the registers have been pushed and the imm5 value is
     subtracted from it.  */
  if (pifsr)
    {
      for (i = 0; reg_table[i].mask != 0; i++)
	{
	  if (list12 & reg_table[i].mask)
	    {
	      int reg = reg_table[i].regno;
	      offset -= v850_reg_size;
	      pifsr->reg = reg;
	      pifsr->offset = offset;
	      pifsr->cur_frameoffset = pi->sp_offset;
	      pifsr++;
	    }
	}
    }

  /* Set result parameters.  */
  *pifsr_ptr = pifsr;
}

/* Helper function to evaluate if register is one of the "save" registers.
   This allows to simplify conditionals in v850_analyze_prologue a lot.  */

static int
v850_is_save_register (int reg)
{
 /* The caller-save registers are R2, R20 - R29 and R31.  All other
    registers are either special purpose (PC, SP), argument registers,
    or just considered free for use in the caller.  */
 return reg == E_R2_REGNUM
	|| (reg >= E_R20_REGNUM && reg <= E_R29_REGNUM)
	|| reg == E_R31_REGNUM;
}

/* Scan the prologue of the function that contains PC, and record what
   we find in PI.  Returns the pc after the prologue.  Note that the
   addresses saved in frame->saved_regs are just frame relative (negative
   offsets from the frame pointer).  This is because we don't know the
   actual value of the frame pointer yet.  In some circumstances, the
   frame pointer can't be determined till after we have scanned the
   prologue.  */

static CORE_ADDR
v850_analyze_prologue (struct gdbarch *gdbarch,
		       CORE_ADDR func_addr, CORE_ADDR pc,
		       struct v850_frame_cache *pi, ULONGEST ctbp)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR prologue_end, current_pc;
  struct pifsr pifsrs[E_NUM_REGS + 1];
  struct pifsr *pifsr, *pifsr_tmp;
  int ep_used;
  int reg;
  CORE_ADDR save_pc, save_end;
  int regsave_func_p;
  int r12_tmp;

  memset (&pifsrs, 0, sizeof pifsrs);
  pifsr = &pifsrs[0];

  prologue_end = pc;

  /* Now, search the prologue looking for instructions that setup fp, save
     rp, adjust sp and such.  We also record the frame offset of any saved
     registers.  */

  pi->sp_offset = 0;
  pi->uses_fp = 0;
  ep_used = 0;
  regsave_func_p = 0;
  save_pc = 0;
  save_end = 0;
  r12_tmp = 0;

  for (current_pc = func_addr; current_pc < prologue_end;)
    {
      int insn;
      int insn2 = -1; /* dummy value */

      insn = read_memory_integer (current_pc, 2, byte_order);
      current_pc += 2;
      if ((insn & 0x0780) >= 0x0600)	/* Four byte instruction?  */
	{
	  insn2 = read_memory_integer (current_pc, 2, byte_order);
	  current_pc += 2;
	}

      if ((insn & 0xffc0) == ((10 << 11) | 0x0780) && !regsave_func_p)
	{			/* jarl <func>,10 */
	  long low_disp = insn2 & ~(long) 1;
	  long disp = (((((insn & 0x3f) << 16) + low_disp)
			& ~(long) 1) ^ 0x00200000) - 0x00200000;

	  save_pc = current_pc;
	  save_end = prologue_end;
	  regsave_func_p = 1;
	  current_pc += disp - 4;
	  prologue_end = (current_pc
			  + (2 * 3)	/* moves to/from ep */
			  + 4		/* addi <const>,sp,sp */
			  + 2		/* jmp [r10] */
			  + (2 * 12)	/* sst.w to save r2, r20-r29, r31 */
			  + 20);	/* slop area */
	}
      else if ((insn & 0xffc0) == 0x0200 && !regsave_func_p)
	{			/* callt <imm6> */
	  long adr = ctbp + ((insn & 0x3f) << 1);

	  save_pc = current_pc;
	  save_end = prologue_end;
	  regsave_func_p = 1;
	  current_pc = ctbp + (read_memory_unsigned_integer (adr, 2, byte_order)
			       & 0xffff);
	  prologue_end = (current_pc
			  + (2 * 3)	/* prepare list2,imm5,sp/imm */
			  + 4		/* ctret */
			  + 20);	/* slop area */
	  continue;
	}
      else if ((insn & 0xffc0) == 0x0780)	/* prepare list2,imm5 */
	{
	  v850_handle_prepare (insn, insn2, &current_pc, pi, &pifsr);
	  continue;
	}
      else if (insn == 0x07e0 && regsave_func_p && insn2 == 0x0144)
	{			/* ctret after processing register save.  */
	  current_pc = save_pc;
	  prologue_end = save_end;
	  regsave_func_p = 0;
	  continue;
	}
      else if ((insn & 0xfff0) == 0x07e0 && (insn2 & 5) == 1)
	{			/* pushml, pushmh */
	  v850_handle_pushm (insn, insn2, pi, &pifsr);
	  continue;
	}
      else if ((insn & 0xffe0) == 0x0060 && regsave_func_p)
	{			/* jmp after processing register save.  */
	  current_pc = save_pc;
	  prologue_end = save_end;
	  regsave_func_p = 0;
	  continue;
	}
      else if ((insn & 0x07c0) == 0x0780	/* jarl or jr */
	       || (insn & 0xffe0) == 0x0060	/* jmp */
	       || (insn & 0x0780) == 0x0580)	/* branch */
	{
	  break;		/* Ran into end of prologue.  */
	}

      else if ((insn & 0xffe0) == ((E_SP_REGNUM << 11) | 0x0240))
	/* add <imm>,sp */
	pi->sp_offset += ((insn & 0x1f) ^ 0x10) - 0x10;
      else if (insn == ((E_SP_REGNUM << 11) | 0x0600 | E_SP_REGNUM))
	/* addi <imm>,sp,sp */
	pi->sp_offset += insn2;
      else if (insn == ((E_FP_REGNUM << 11) | 0x0000 | E_SP_REGNUM))
	/* mov sp,fp */
	pi->uses_fp = 1;
      else if (insn == ((E_R12_REGNUM << 11) | 0x0640 | E_R0_REGNUM))
	/* movhi hi(const),r0,r12 */
	r12_tmp = insn2 << 16;
      else if (insn == ((E_R12_REGNUM << 11) | 0x0620 | E_R12_REGNUM))
	/* movea lo(const),r12,r12 */
	r12_tmp += insn2;
      else if (insn == ((E_SP_REGNUM << 11) | 0x01c0 | E_R12_REGNUM) && r12_tmp)
	/* add r12,sp */
	pi->sp_offset += r12_tmp;
      else if (insn == ((E_EP_REGNUM << 11) | 0x0000 | E_SP_REGNUM))
	/* mov sp,ep */
	ep_used = 1;
      else if (insn == ((E_EP_REGNUM << 11) | 0x0000 | E_R1_REGNUM))
	/* mov r1,ep */
	ep_used = 0;
      else if (((insn & 0x07ff) == (0x0760 | E_SP_REGNUM)	
		|| (pi->uses_fp
		    && (insn & 0x07ff) == (0x0760 | E_FP_REGNUM)))
	       && pifsr
	       && v850_is_save_register (reg = (insn >> 11) & 0x1f))
	{
	  /* st.w <reg>,<offset>[sp] or st.w <reg>,<offset>[fp] */
	  pifsr->reg = reg;
	  pifsr->offset = insn2 & ~1;
	  pifsr->cur_frameoffset = pi->sp_offset;
	  pifsr++;
	}
      else if (ep_used
	       && ((insn & 0x0781) == 0x0501)
	       && pifsr
	       && v850_is_save_register (reg = (insn >> 11) & 0x1f))
	{
	  /* sst.w <reg>,<offset>[ep] */
	  pifsr->reg = reg;
	  pifsr->offset = (insn & 0x007e) << 1;
	  pifsr->cur_frameoffset = pi->sp_offset;
	  pifsr++;
	}
    }

  /* Fix up any offsets to the final offset.  If a frame pointer was created,
     use it instead of the stack pointer.  */
  for (pifsr_tmp = pifsrs; pifsr_tmp != pifsr; pifsr_tmp++)
    {
      pifsr_tmp->offset -= pi->sp_offset - pifsr_tmp->cur_frameoffset;
      pi->saved_regs[pifsr_tmp->reg].set_addr (pifsr_tmp->offset);
    }

  return current_pc;
}

/* Return the address of the first code past the prologue of the function.  */

static CORE_ADDR
v850_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr, func_end;

  /* See what the symbol table says.  */

  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      struct symtab_and_line sal;

      sal = find_pc_line (func_addr, 0);
      if (sal.line != 0 && sal.end < func_end)
	return sal.end;

      /* Either there's no line info, or the line after the prologue is after
	 the end of the function.  In this case, there probably isn't a
	 prologue.  */
      return pc;
    }

  /* We can't find the start of this function, so there's nothing we
     can do.  */
  return pc;
}

/* Return 1 if the data structure has any 8-byte fields that'll require
   the entire data structure to be aligned.  Otherwise, return 0.  */

static int
v850_eight_byte_align_p (struct type *type)
{
  type = check_typedef (type);

  if (v850_type_is_scalar (type))
    return (type->length () == 8);
  else
    {
      int i;

      for (i = 0; i < type->num_fields (); i++)
	{
	  if (v850_eight_byte_align_p (type->field (i).type ()))
	    return 1;
	}
    }
  return 0;
}

static CORE_ADDR
v850_frame_align (struct gdbarch *ignore, CORE_ADDR sp)
{
  return sp & ~3;
}

/* Setup arguments and LP for a call to the target.  First four args
   go in R6->R9, subsequent args go into sp + 16 -> sp + ...  Structs
   are passed by reference.  64 bit quantities (doubles and long longs)
   may be split between the regs and the stack.  When calling a function
   that returns a struct, a pointer to the struct is passed in as a secret
   first argument (always in R6).

   Stack space for the args has NOT been allocated: that job is up to us.  */

static CORE_ADDR
v850_push_dummy_call (struct gdbarch *gdbarch,
		      struct value *function,
		      struct regcache *regcache,
		      CORE_ADDR bp_addr,
		      int nargs,
		      struct value **args,
		      CORE_ADDR sp,
		      function_call_return_method return_method,
		      CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int argreg;
  int argnum;
  int arg_space = 0;
  int stack_offset;
  v850_gdbarch_tdep *tdep = gdbarch_tdep<v850_gdbarch_tdep> (gdbarch);

  if (tdep->abi == V850_ABI_RH850)
    stack_offset = 0;
  else
    {
      /* The offset onto the stack at which we will start copying parameters
	 (after the registers are used up) begins at 16 rather than at zero.
	 That's how the ABI is defined, though there's no indication that these
	 16 bytes are used for anything, not even for saving incoming
	 argument registers.  */
      stack_offset = 16;
    }

  /* Now make space on the stack for the args.  */
  for (argnum = 0; argnum < nargs; argnum++)
    arg_space += ((args[argnum]->type ()->length () + 3) & ~3);
  sp -= arg_space + stack_offset;

  argreg = E_ARG0_REGNUM;
  /* The struct_return pointer occupies the first parameter register.  */
  if (return_method == return_method_struct)
    regcache_cooked_write_unsigned (regcache, argreg++, struct_addr);

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  There are 16 bytes
     in four registers available.  Loop thru args from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      int len;
      gdb_byte *val;
      gdb_byte valbuf[v850_reg_size];

      if (!v850_type_is_scalar ((*args)->type ())
	  && tdep->abi == V850_ABI_GCC
	  && (*args)->type ()->length () > E_MAX_RETTYPE_SIZE_IN_REGS)
	{
	  store_unsigned_integer (valbuf, 4, byte_order,
				  (*args)->address ());
	  len = 4;
	  val = valbuf;
	}
      else
	{
	  len = (*args)->type ()->length ();
	  val = (gdb_byte *) (*args)->contents ().data ();
	}

      if (tdep->eight_byte_align
	  && v850_eight_byte_align_p ((*args)->type ()))
	{
	  if (argreg <= E_ARGLAST_REGNUM && (argreg & 1))
	    argreg++;
	  else if (stack_offset & 0x4)
	    stack_offset += 4;
	}

      while (len > 0)
	if (argreg <= E_ARGLAST_REGNUM)
	  {
	    CORE_ADDR regval;

	    regval = extract_unsigned_integer (val, v850_reg_size, byte_order);
	    regcache_cooked_write_unsigned (regcache, argreg, regval);

	    len -= v850_reg_size;
	    val += v850_reg_size;
	    argreg++;
	  }
	else
	  {
	    write_memory (sp + stack_offset, val, 4);

	    len -= 4;
	    val += 4;
	    stack_offset += 4;
	  }
      args++;
    }

  /* Store return address.  */
  regcache_cooked_write_unsigned (regcache, E_LP_REGNUM, bp_addr);

  /* Update stack pointer.  */
  regcache_cooked_write_unsigned (regcache, E_SP_REGNUM, sp);

  return sp;
}

static void
v850_extract_return_value (struct type *type, struct regcache *regcache,
			   gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int len = type->length ();

  if (len <= v850_reg_size)
    {
      ULONGEST val;

      regcache_cooked_read_unsigned (regcache, E_V0_REGNUM, &val);
      store_unsigned_integer (valbuf, len, byte_order, val);
    }
  else if (len <= 2 * v850_reg_size)
    {
      int i, regnum = E_V0_REGNUM;
      gdb_byte buf[v850_reg_size];
      for (i = 0; len > 0; i += 4, len -= 4)
	{
	  regcache->raw_read (regnum++, buf);
	  memcpy (valbuf + i, buf, len > 4 ? 4 : len);
	}
    }
}

static void
v850_store_return_value (struct type *type, struct regcache *regcache,
			 const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int len = type->length ();

  if (len <= v850_reg_size)
      regcache_cooked_write_unsigned
	(regcache, E_V0_REGNUM,
	 extract_unsigned_integer (valbuf, len, byte_order));
  else if (len <= 2 * v850_reg_size)
    {
      int i, regnum = E_V0_REGNUM;
      for (i = 0; i < len; i += 4)
	regcache->raw_write (regnum++, valbuf + i);
    }
}

static enum return_value_convention
v850_return_value (struct gdbarch *gdbarch, struct value *function,
		   struct type *type, struct regcache *regcache,
		   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (v850_use_struct_convention (gdbarch, type))
    return RETURN_VALUE_STRUCT_CONVENTION;
  if (writebuf)
    v850_store_return_value (type, regcache, writebuf);
  else if (readbuf)
    v850_extract_return_value (type, regcache, readbuf);
  return RETURN_VALUE_REGISTER_CONVENTION;
}

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
v850_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  return 2;
}

/* Implement the sw_breakpoint_from_kind gdbarch method.  */

static const gdb_byte *
v850_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  *size = kind;

    switch (gdbarch_bfd_arch_info (gdbarch)->mach)
    {
    case bfd_mach_v850e2:
    case bfd_mach_v850e2v3:
    case bfd_mach_v850e3v5:
      {
	/* Implement software breakpoints by using the dbtrap instruction.
	   Older architectures had no such instruction.  For those, an
	   unconditional branch to self instruction is used.  */

	static unsigned char dbtrap_breakpoint[] = { 0x40, 0xf8 };

	return dbtrap_breakpoint;
      }
      break;
    default:
      {
	static unsigned char breakpoint[] = { 0x85, 0x05 };

	return breakpoint;
      }
      break;
    }
}

static struct v850_frame_cache *
v850_alloc_frame_cache (frame_info_ptr this_frame)
{
  struct v850_frame_cache *cache;

  cache = FRAME_OBSTACK_ZALLOC (struct v850_frame_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* Base address.  */
  cache->base = 0;
  cache->sp_offset = 0;
  cache->pc = 0;

  /* Frameless until proven otherwise.  */
  cache->uses_fp = 0;

  return cache;
}

static struct v850_frame_cache *
v850_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct v850_frame_cache *cache;
  CORE_ADDR current_pc;
  int i;

  if (*this_cache)
    return (struct v850_frame_cache *) *this_cache;

  cache = v850_alloc_frame_cache (this_frame);
  *this_cache = cache;

  /* In principle, for normal frames, fp holds the frame pointer,
     which holds the base address for the current stack frame.
     However, for functions that don't need it, the frame pointer is
     optional.  For these "frameless" functions the frame pointer is
     actually the frame pointer of the calling frame.  */
  cache->base = get_frame_register_unsigned (this_frame, E_FP_REGNUM);
  if (cache->base == 0)
    return cache;

  cache->pc = get_frame_func (this_frame);
  current_pc = get_frame_pc (this_frame);
  if (cache->pc != 0)
    {
      ULONGEST ctbp;
      ctbp = get_frame_register_unsigned (this_frame, E_CTBP_REGNUM);
      v850_analyze_prologue (gdbarch, cache->pc, current_pc, cache, ctbp);
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
      cache->base = get_frame_register_unsigned (this_frame, E_SP_REGNUM);
    }

  /* Now that we have the base address for the stack frame we can
     calculate the value of sp in the calling frame.  */
  cache->saved_regs[E_SP_REGNUM].set_value (cache->base - cache->sp_offset);

  /* Adjust all the saved registers such that they contain addresses
     instead of offsets.  */
  for (i = 0; i < gdbarch_num_regs (gdbarch); i++)
    if (cache->saved_regs[i].is_addr ())
      cache->saved_regs[i].set_addr (cache->saved_regs[i].addr ()
				     + cache->base);

  /* The call instruction moves the caller's PC in the callee's LP.
     Since this is an unwind, do the reverse.  Copy the location of LP
     into PC (the address / regnum) so that a request for PC will be
     converted into a request for the LP.  */

  cache->saved_regs[E_PC_REGNUM] = cache->saved_regs[E_LP_REGNUM];

  return cache;
}


static struct value *
v850_frame_prev_register (frame_info_ptr this_frame,
			  void **this_cache, int regnum)
{
  struct v850_frame_cache *cache = v850_frame_cache (this_frame, this_cache);

  gdb_assert (regnum >= 0);

  return trad_frame_get_prev_register (this_frame, cache->saved_regs, regnum);
}

static void
v850_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		    struct frame_id *this_id)
{
  struct v850_frame_cache *cache = v850_frame_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  *this_id = frame_id_build (cache->saved_regs[E_SP_REGNUM].addr (), cache->pc);
}

static const struct frame_unwind v850_frame_unwind = {
  "v850 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  v850_frame_this_id,
  v850_frame_prev_register,
  NULL,
  default_frame_sniffer
};

static CORE_ADDR
v850_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct v850_frame_cache *cache = v850_frame_cache (this_frame, this_cache);

  return cache->base;
}

static const struct frame_base v850_frame_base = {
  &v850_frame_unwind,
  v850_frame_base_address,
  v850_frame_base_address,
  v850_frame_base_address
};

static struct gdbarch *
v850_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  int e_flags, e_machine;

  /* Extract the elf_flags if available.  */
  if (info.abfd != NULL
      && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    {
      e_flags = elf_elfheader (info.abfd)->e_flags;
      e_machine = elf_elfheader (info.abfd)->e_machine;
    }
  else
    {
      e_flags = 0;
      e_machine = 0;
    }


  /* Try to find the architecture in the list of already defined
     architectures.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      v850_gdbarch_tdep *tdep
	= gdbarch_tdep<v850_gdbarch_tdep> (arches->gdbarch);

      if (tdep->e_flags != e_flags || tdep->e_machine != e_machine)
	continue;

      return arches->gdbarch;
    }

  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new v850_gdbarch_tdep));
  v850_gdbarch_tdep *tdep = gdbarch_tdep<v850_gdbarch_tdep> (gdbarch);

  tdep->e_flags = e_flags;
  tdep->e_machine = e_machine;

  switch (tdep->e_machine)
    {
    case EM_V800:
      tdep->abi = V850_ABI_RH850;
      break;
    default:
      tdep->abi = V850_ABI_GCC;
      break;
    }

  tdep->eight_byte_align = (tdep->e_flags & EF_RH850_DATA_ALIGN8) ? 1 : 0;

  switch (info.bfd_arch_info->mach)
    {
    case bfd_mach_v850:
      set_gdbarch_register_name (gdbarch, v850_register_name);
      set_gdbarch_num_regs (gdbarch, E_NUM_OF_V850_REGS);
      break;
    case bfd_mach_v850e:
    case bfd_mach_v850e1:
      set_gdbarch_register_name (gdbarch, v850e_register_name);
      set_gdbarch_num_regs (gdbarch, E_NUM_OF_V850E_REGS);
      break;
    case bfd_mach_v850e2:
    case bfd_mach_v850e2v3:
      set_gdbarch_register_name (gdbarch, v850e2_register_name);
      set_gdbarch_num_regs (gdbarch, E_NUM_REGS);
      break;
    case bfd_mach_v850e3v5:
      set_gdbarch_register_name (gdbarch, v850e3v5_register_name);
      set_gdbarch_num_regs (gdbarch, E_NUM_OF_V850E3V5_REGS);
      break;
    }

  set_gdbarch_num_pseudo_regs (gdbarch, 0);
  set_gdbarch_sp_regnum (gdbarch, E_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, E_PC_REGNUM);
  set_gdbarch_fp0_regnum (gdbarch, -1);

  set_gdbarch_register_type (gdbarch, v850_register_type);

  set_gdbarch_char_signed (gdbarch, 1);
  set_gdbarch_short_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_int_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_long_bit (gdbarch, 8 * TARGET_CHAR_BIT);

  set_gdbarch_float_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_double_bit (gdbarch, 8 * TARGET_CHAR_BIT);
  set_gdbarch_long_double_bit (gdbarch, 8 * TARGET_CHAR_BIT);

  set_gdbarch_ptr_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_addr_bit (gdbarch, 4 * TARGET_CHAR_BIT);

  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_breakpoint_kind_from_pc (gdbarch, v850_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, v850_sw_breakpoint_from_kind);
  set_gdbarch_return_value (gdbarch, v850_return_value);
  set_gdbarch_push_dummy_call (gdbarch, v850_push_dummy_call);
  set_gdbarch_skip_prologue (gdbarch, v850_skip_prologue);

  set_gdbarch_frame_align (gdbarch, v850_frame_align);
  frame_base_set_default (gdbarch, &v850_frame_base);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &v850_frame_unwind);

  return gdbarch;
}

void _initialize_v850_tdep ();
void
_initialize_v850_tdep ()
{
  gdbarch_register (bfd_arch_v850, v850_gdbarch_init);
  gdbarch_register (bfd_arch_v850_rh850, v850_gdbarch_init);
}
