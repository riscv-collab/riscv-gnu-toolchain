/* Target-dependent code for the Matsushita MN10300 for GDB, the GNU debugger.

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
#include "arch-utils.h"
#include "dis-asm.h"
#include "gdbtypes.h"
#include "regcache.h"
#include "gdbcore.h"
#include "value.h"
#include "frame.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "symtab.h"
#include "dwarf2/frame.h"
#include "osabi.h"
#include "infcall.h"
#include "prologue-value.h"
#include "target.h"

#include "mn10300-tdep.h"


/* The am33-2 has 64 registers.  */
#define MN10300_MAX_NUM_REGS 64

/* Big enough to hold the size of the largest register in bytes.  */
#define MN10300_MAX_REGISTER_SIZE      64

/* This structure holds the results of a prologue analysis.  */
struct mn10300_prologue
{
  /* The architecture for which we generated this prologue info.  */
  struct gdbarch *gdbarch;

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
  int reg_offset[MN10300_MAX_NUM_REGS];
};


/* Compute the alignment required by a type.  */

static int
mn10300_type_align (struct type *type)
{
  int i, align = 1;

  switch (type->code ())
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_SET:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_FLT:
    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
      return type->length ();

    case TYPE_CODE_COMPLEX:
      return type->length () / 2;

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      for (i = 0; i < type->num_fields (); i++)
	{
	  int falign = mn10300_type_align (type->field (i).type ());
	  while (align < falign)
	    align <<= 1;
	}
      return align;

    case TYPE_CODE_ARRAY:
      /* HACK!  Structures containing arrays, even small ones, are not
	 eligible for returning in registers.  */
      return 256;

    case TYPE_CODE_TYPEDEF:
      return mn10300_type_align (check_typedef (type));

    default:
      internal_error (_("bad switch"));
    }
}

/* Should call_function allocate stack space for a struct return?  */
static int
mn10300_use_struct_convention (struct type *type)
{
  /* Structures bigger than a pair of words can't be returned in
     registers.  */
  if (type->length () > 8)
    return 1;

  switch (type->code ())
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      /* Structures with a single field are handled as the field
	 itself.  */
      if (type->num_fields () == 1)
	return mn10300_use_struct_convention (type->field (0).type ());

      /* Structures with word or double-word size are passed in memory, as
	 long as they require at least word alignment.  */
      if (mn10300_type_align (type) >= 4)
	return 0;

      return 1;

      /* Arrays are addressable, so they're never returned in
	 registers.  This condition can only hold when the array is
	 the only field of a struct or union.  */
    case TYPE_CODE_ARRAY:
      return 1;

    case TYPE_CODE_TYPEDEF:
      return mn10300_use_struct_convention (check_typedef (type));

    default:
      return 0;
    }
}

static void
mn10300_store_return_value (struct gdbarch *gdbarch, struct type *type,
			    struct regcache *regcache, const gdb_byte *valbuf)
{
  int len = type->length ();
  int reg, regsz;
  
  if (type->code () == TYPE_CODE_PTR)
    reg = 4;
  else
    reg = 0;

  regsz = register_size (gdbarch, reg);

  if (len <= regsz)
    regcache->raw_write_part (reg, 0, len, valbuf);
  else if (len <= 2 * regsz)
    {
      regcache->raw_write (reg, valbuf);
      gdb_assert (regsz == register_size (gdbarch, reg + 1));
      regcache->raw_write_part (reg + 1, 0, len - regsz, valbuf + regsz);
    }
  else
    internal_error (_("Cannot store return value %d bytes long."), len);
}

static void
mn10300_extract_return_value (struct gdbarch *gdbarch, struct type *type,
			      struct regcache *regcache, void *valbuf)
{
  gdb_byte buf[MN10300_MAX_REGISTER_SIZE];
  int len = type->length ();
  int reg, regsz;

  if (type->code () == TYPE_CODE_PTR)
    reg = 4;
  else
    reg = 0;

  regsz = register_size (gdbarch, reg);
  gdb_assert (regsz <= MN10300_MAX_REGISTER_SIZE);
  if (len <= regsz)
    {
      regcache->raw_read (reg, buf);
      memcpy (valbuf, buf, len);
    }
  else if (len <= 2 * regsz)
    {
      regcache->raw_read (reg, buf);
      memcpy (valbuf, buf, regsz);
      gdb_assert (regsz == register_size (gdbarch, reg + 1));
      regcache->raw_read (reg + 1, buf);
      memcpy ((char *) valbuf + regsz, buf, len - regsz);
    }
  else
    internal_error (_("Cannot extract return value %d bytes long."), len);
}

/* Determine, for architecture GDBARCH, how a return value of TYPE
   should be returned.  If it is supposed to be returned in registers,
   and READBUF is non-zero, read the appropriate value from REGCACHE,
   and copy it into READBUF.  If WRITEBUF is non-zero, write the value
   from WRITEBUF into REGCACHE.  */

static enum return_value_convention
mn10300_return_value (struct gdbarch *gdbarch, struct value *function,
		      struct type *type, struct regcache *regcache,
		      gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (mn10300_use_struct_convention (type))
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf)
    mn10300_extract_return_value (gdbarch, type, regcache, readbuf);
  if (writebuf)
    mn10300_store_return_value (gdbarch, type, regcache, writebuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}

static const char *
register_name (int reg, const char **regs, long num_regs)
{
  gdb_assert (reg < num_regs);
  return regs[reg];
}

static const char *
mn10300_generic_register_name (struct gdbarch *gdbarch, int reg)
{
  static const char *regs[] =
  { "d0", "d1", "d2", "d3", "a0", "a1", "a2", "a3",
    "sp", "pc", "mdr", "psw", "lir", "lar", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "fp"
  };
  return register_name (reg, regs, ARRAY_SIZE (regs));
}


static const char *
am33_register_name (struct gdbarch *gdbarch, int reg)
{
  static const char *regs[] =
  { "d0", "d1", "d2", "d3", "a0", "a1", "a2", "a3",
    "sp", "pc", "mdr", "psw", "lir", "lar", "",
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "ssp", "msp", "usp", "mcrh", "mcrl", "mcvf", "", "", ""
  };
  return register_name (reg, regs, ARRAY_SIZE (regs));
}

static const char *
am33_2_register_name (struct gdbarch *gdbarch, int reg)
{
  static const char *regs[] =
  {
    "d0", "d1", "d2", "d3", "a0", "a1", "a2", "a3",
    "sp", "pc", "mdr", "psw", "lir", "lar", "mdrq", "r0",
    "r1", "r2", "r3", "r4", "r5", "r6", "r7", "ssp",
    "msp", "usp", "mcrh", "mcrl", "mcvf", "fpcr", "", "",
    "fs0", "fs1", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7",
    "fs8", "fs9", "fs10", "fs11", "fs12", "fs13", "fs14", "fs15",
    "fs16", "fs17", "fs18", "fs19", "fs20", "fs21", "fs22", "fs23",
    "fs24", "fs25", "fs26", "fs27", "fs28", "fs29", "fs30", "fs31"
  };
  return register_name (reg, regs, ARRAY_SIZE (regs));
}

static struct type *
mn10300_register_type (struct gdbarch *gdbarch, int reg)
{
  return builtin_type (gdbarch)->builtin_int;
}

/* The breakpoint instruction must be the same size as the smallest
   instruction in the instruction set.

   The Matsushita mn10x00 processors have single byte instructions
   so we need a single byte breakpoint.  Matsushita hasn't defined
   one, so we defined it ourselves.  */
constexpr gdb_byte mn10300_break_insn[] = {0xff};

typedef BP_MANIPULATION (mn10300_break_insn) mn10300_breakpoint;

/* Model the semantics of pushing a register onto the stack.  This
   is a helper function for mn10300_analyze_prologue, below.  */
static void
push_reg (pv_t *regs, struct pv_area *stack, int regnum)
{
  regs[E_SP_REGNUM] = pv_add_constant (regs[E_SP_REGNUM], -4);
  stack->store (regs[E_SP_REGNUM], 4, regs[regnum]);
}

/* Translate an "r" register number extracted from an instruction encoding
   into a GDB register number.  Adapted from a simulator function
   of the same name; see am33.igen.  */
static int
translate_rreg (int rreg)
{
 /* The higher register numbers actually correspond to the
     basic machine's address and data registers.  */
  if (rreg > 7 && rreg < 12)
    return E_A0_REGNUM + rreg - 8;
  else if (rreg > 11 && rreg < 16)
    return E_D0_REGNUM + rreg - 12;
  else
    return E_E0_REGNUM + rreg;
}

/* Find saved registers in a 'struct pv_area'; we pass this to pv_area::scan.

   If VALUE is a saved register, ADDR says it was saved at a constant
   offset from the frame base, and SIZE indicates that the whole
   register was saved, record its offset in RESULT_UNTYPED.  */
static void
check_for_saved (void *result_untyped, pv_t addr, CORE_ADDR size, pv_t value)
{
  struct mn10300_prologue *result = (struct mn10300_prologue *) result_untyped;

  if (value.kind == pvk_register
      && value.k == 0
      && pv_is_register (addr, E_SP_REGNUM)
      && size == register_size (result->gdbarch, value.reg))
    result->reg_offset[value.reg] = addr.k;
}

/* Analyze the prologue to determine where registers are saved,
   the end of the prologue, etc.  The result of this analysis is
   returned in RESULT.  See struct mn10300_prologue above for more
   information.  */
static void
mn10300_analyze_prologue (struct gdbarch *gdbarch,
			  CORE_ADDR start_pc, CORE_ADDR limit_pc,
			  struct mn10300_prologue *result)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR pc;
  int rn;
  pv_t regs[MN10300_MAX_NUM_REGS];
  CORE_ADDR after_last_frame_setup_insn = start_pc;
  int am33_mode = get_am33_mode (gdbarch);

  memset (result, 0, sizeof (*result));
  result->gdbarch = gdbarch;

  for (rn = 0; rn < MN10300_MAX_NUM_REGS; rn++)
    {
      regs[rn] = pv_register (rn, 0);
      result->reg_offset[rn] = 1;
    }
  pv_area stack (E_SP_REGNUM, gdbarch_addr_bit (gdbarch));

  /* The typical call instruction will have saved the return address on the
     stack.  Space for the return address has already been preallocated in
     the caller's frame.  It's possible, such as when using -mrelax with gcc
     that other registers were saved as well.  If this happens, we really
     have no chance of deciphering the frame.  DWARF info can save the day
     when this happens.  */
  stack.store (regs[E_SP_REGNUM], 4, regs[E_PC_REGNUM]);

  pc = start_pc;
  while (pc < limit_pc)
    {
      int status;
      gdb_byte instr[2];

      /* Instructions can be as small as one byte; however, we usually
	 need at least two bytes to do the decoding, so fetch that many
	 to begin with.  */
      status = target_read_memory (pc, instr, 2);
      if (status != 0)
	break;

      /* movm [regs], sp  */
      if (instr[0] == 0xcf)
	{
	  gdb_byte save_mask;

	  save_mask = instr[1];

	  if ((save_mask & movm_exreg0_bit) && am33_mode)
	    {
	      push_reg (regs, &stack, E_E2_REGNUM);
	      push_reg (regs, &stack, E_E3_REGNUM);
	    }
	  if ((save_mask & movm_exreg1_bit) && am33_mode)
	    {
	      push_reg (regs, &stack, E_E4_REGNUM);
	      push_reg (regs, &stack, E_E5_REGNUM);
	      push_reg (regs, &stack, E_E6_REGNUM);
	      push_reg (regs, &stack, E_E7_REGNUM);
	    }
	  if ((save_mask & movm_exother_bit) && am33_mode)
	    {
	      push_reg (regs, &stack, E_E0_REGNUM);
	      push_reg (regs, &stack, E_E1_REGNUM);
	      push_reg (regs, &stack, E_MDRQ_REGNUM);
	      push_reg (regs, &stack, E_MCRH_REGNUM);
	      push_reg (regs, &stack, E_MCRL_REGNUM);
	      push_reg (regs, &stack, E_MCVF_REGNUM);
	    }
	  if (save_mask & movm_d2_bit)
	    push_reg (regs, &stack, E_D2_REGNUM);
	  if (save_mask & movm_d3_bit)
	    push_reg (regs, &stack, E_D3_REGNUM);
	  if (save_mask & movm_a2_bit)
	    push_reg (regs, &stack, E_A2_REGNUM);
	  if (save_mask & movm_a3_bit)
	    push_reg (regs, &stack, E_A3_REGNUM);
	  if (save_mask & movm_other_bit)
	    {
	      push_reg (regs, &stack, E_D0_REGNUM);
	      push_reg (regs, &stack, E_D1_REGNUM);
	      push_reg (regs, &stack, E_A0_REGNUM);
	      push_reg (regs, &stack, E_A1_REGNUM);
	      push_reg (regs, &stack, E_MDR_REGNUM);
	      push_reg (regs, &stack, E_LIR_REGNUM);
	      push_reg (regs, &stack, E_LAR_REGNUM);
	      /* The `other' bit leaves a blank area of four bytes at
		 the beginning of its block of saved registers, making
		 it 32 bytes long in total.  */
	      regs[E_SP_REGNUM] = pv_add_constant (regs[E_SP_REGNUM], -4);
	    }

	  pc += 2;
	  after_last_frame_setup_insn = pc;
	}
      /* mov sp, aN */
      else if ((instr[0] & 0xfc) == 0x3c)
	{
	  int aN = instr[0] & 0x03;

	  regs[E_A0_REGNUM + aN] = regs[E_SP_REGNUM];

	  pc += 1;
	  if (aN == 3)
	    after_last_frame_setup_insn = pc;
	}
      /* mov aM, aN */
      else if ((instr[0] & 0xf0) == 0x90
	       && (instr[0] & 0x03) != ((instr[0] & 0x0c) >> 2))
	{
	  int aN = instr[0] & 0x03;
	  int aM = (instr[0] & 0x0c) >> 2;

	  regs[E_A0_REGNUM + aN] = regs[E_A0_REGNUM + aM];

	  pc += 1;
	}
      /* mov dM, dN */
      else if ((instr[0] & 0xf0) == 0x80
	       && (instr[0] & 0x03) != ((instr[0] & 0x0c) >> 2))
	{
	  int dN = instr[0] & 0x03;
	  int dM = (instr[0] & 0x0c) >> 2;

	  regs[E_D0_REGNUM + dN] = regs[E_D0_REGNUM + dM];

	  pc += 1;
	}
      /* mov aM, dN */
      else if (instr[0] == 0xf1 && (instr[1] & 0xf0) == 0xd0)
	{
	  int dN = instr[1] & 0x03;
	  int aM = (instr[1] & 0x0c) >> 2;

	  regs[E_D0_REGNUM + dN] = regs[E_A0_REGNUM + aM];

	  pc += 2;
	}
      /* mov dM, aN */
      else if (instr[0] == 0xf1 && (instr[1] & 0xf0) == 0xe0)
	{
	  int aN = instr[1] & 0x03;
	  int dM = (instr[1] & 0x0c) >> 2;

	  regs[E_A0_REGNUM + aN] = regs[E_D0_REGNUM + dM];

	  pc += 2;
	}
      /* add imm8, SP */
      else if (instr[0] == 0xf8 && instr[1] == 0xfe)
	{
	  gdb_byte buf[1];
	  LONGEST imm8;


	  status = target_read_memory (pc + 2, buf, 1);
	  if (status != 0)
	    break;

	  imm8 = extract_signed_integer (buf, 1, byte_order);
	  regs[E_SP_REGNUM] = pv_add_constant (regs[E_SP_REGNUM], imm8);

	  pc += 3;
	  /* Stack pointer adjustments are frame related.  */
	  after_last_frame_setup_insn = pc;
	}
      /* add imm16, SP */
      else if (instr[0] == 0xfa && instr[1] == 0xfe)
	{
	  gdb_byte buf[2];
	  LONGEST imm16;

	  status = target_read_memory (pc + 2, buf, 2);
	  if (status != 0)
	    break;

	  imm16 = extract_signed_integer (buf, 2, byte_order);
	  regs[E_SP_REGNUM] = pv_add_constant (regs[E_SP_REGNUM], imm16);

	  pc += 4;
	  /* Stack pointer adjustments are frame related.  */
	  after_last_frame_setup_insn = pc;
	}
      /* add imm32, SP */
      else if (instr[0] == 0xfc && instr[1] == 0xfe)
	{
	  gdb_byte buf[4];
	  LONGEST imm32;

	  status = target_read_memory (pc + 2, buf, 4);
	  if (status != 0)
	    break;


	  imm32 = extract_signed_integer (buf, 4, byte_order);
	  regs[E_SP_REGNUM] = pv_add_constant (regs[E_SP_REGNUM], imm32);

	  pc += 6;
	  /* Stack pointer adjustments are frame related.  */
	  after_last_frame_setup_insn = pc;
	}
      /* add imm8, aN  */
      else if ((instr[0] & 0xfc) == 0x20)
	{
	  int aN;
	  LONGEST imm8;

	  aN = instr[0] & 0x03;
	  imm8 = extract_signed_integer (&instr[1], 1, byte_order);

	  regs[E_A0_REGNUM + aN] = pv_add_constant (regs[E_A0_REGNUM + aN],
						    imm8);

	  pc += 2;
	}
      /* add imm16, aN  */
      else if (instr[0] == 0xfa && (instr[1] & 0xfc) == 0xd0)
	{
	  int aN;
	  LONGEST imm16;
	  gdb_byte buf[2];

	  aN = instr[1] & 0x03;

	  status = target_read_memory (pc + 2, buf, 2);
	  if (status != 0)
	    break;


	  imm16 = extract_signed_integer (buf, 2, byte_order);

	  regs[E_A0_REGNUM + aN] = pv_add_constant (regs[E_A0_REGNUM + aN],
						    imm16);

	  pc += 4;
	}
      /* add imm32, aN  */
      else if (instr[0] == 0xfc && (instr[1] & 0xfc) == 0xd0)
	{
	  int aN;
	  LONGEST imm32;
	  gdb_byte buf[4];

	  aN = instr[1] & 0x03;

	  status = target_read_memory (pc + 2, buf, 4);
	  if (status != 0)
	    break;

	  imm32 = extract_signed_integer (buf, 2, byte_order);

	  regs[E_A0_REGNUM + aN] = pv_add_constant (regs[E_A0_REGNUM + aN],
						    imm32);
	  pc += 6;
	}
      /* fmov fsM, (rN) */
      else if (instr[0] == 0xf9 && (instr[1] & 0xfd) == 0x30)
	{
	  int fsM, sM, Y, rN;
	  gdb_byte buf[1];

	  Y = (instr[1] & 0x02) >> 1;

	  status = target_read_memory (pc + 2, buf, 1);
	  if (status != 0)
	    break;

	  sM = (buf[0] & 0xf0) >> 4;
	  rN = buf[0] & 0x0f;
	  fsM = (Y << 4) | sM;

	  stack.store (regs[translate_rreg (rN)], 4,
		       regs[E_FS0_REGNUM + fsM]);

	  pc += 3;
	}
      /* fmov fsM, (sp) */
      else if (instr[0] == 0xf9 && (instr[1] & 0xfd) == 0x34)
	{
	  int fsM, sM, Y;
	  gdb_byte buf[1];

	  Y = (instr[1] & 0x02) >> 1;

	  status = target_read_memory (pc + 2, buf, 1);
	  if (status != 0)
	    break;

	  sM = (buf[0] & 0xf0) >> 4;
	  fsM = (Y << 4) | sM;

	  stack.store (regs[E_SP_REGNUM], 4,
		       regs[E_FS0_REGNUM + fsM]);

	  pc += 3;
	}
      /* fmov fsM, (rN, rI) */
      else if (instr[0] == 0xfb && instr[1] == 0x37)
	{
	  int fsM, sM, Z, rN, rI;
	  gdb_byte buf[2];


	  status = target_read_memory (pc + 2, buf, 2);
	  if (status != 0)
	    break;

	  rI = (buf[0] & 0xf0) >> 4;
	  rN = buf[0] & 0x0f;
	  sM = (buf[1] & 0xf0) >> 4;
	  Z = (buf[1] & 0x02) >> 1;
	  fsM = (Z << 4) | sM;

	  stack.store (pv_add (regs[translate_rreg (rN)],
			       regs[translate_rreg (rI)]),
		       4, regs[E_FS0_REGNUM + fsM]);

	  pc += 4;
	}
      /* fmov fsM, (d8, rN) */
      else if (instr[0] == 0xfb && (instr[1] & 0xfd) == 0x30)
	{
	  int fsM, sM, Y, rN;
	  LONGEST d8;
	  gdb_byte buf[2];

	  Y = (instr[1] & 0x02) >> 1;

	  status = target_read_memory (pc + 2, buf, 2);
	  if (status != 0)
	    break;

	  sM = (buf[0] & 0xf0) >> 4;
	  rN = buf[0] & 0x0f;
	  fsM = (Y << 4) | sM;
	  d8 = extract_signed_integer (&buf[1], 1, byte_order);

	  stack.store (pv_add_constant (regs[translate_rreg (rN)], d8),
		       4, regs[E_FS0_REGNUM + fsM]);

	  pc += 4;
	}
      /* fmov fsM, (d24, rN) */
      else if (instr[0] == 0xfd && (instr[1] & 0xfd) == 0x30)
	{
	  int fsM, sM, Y, rN;
	  LONGEST d24;
	  gdb_byte buf[4];

	  Y = (instr[1] & 0x02) >> 1;

	  status = target_read_memory (pc + 2, buf, 4);
	  if (status != 0)
	    break;

	  sM = (buf[0] & 0xf0) >> 4;
	  rN = buf[0] & 0x0f;
	  fsM = (Y << 4) | sM;
	  d24 = extract_signed_integer (&buf[1], 3, byte_order);

	  stack.store (pv_add_constant (regs[translate_rreg (rN)], d24),
		       4, regs[E_FS0_REGNUM + fsM]);

	  pc += 6;
	}
      /* fmov fsM, (d32, rN) */
      else if (instr[0] == 0xfe && (instr[1] & 0xfd) == 0x30)
	{
	  int fsM, sM, Y, rN;
	  LONGEST d32;
	  gdb_byte buf[5];

	  Y = (instr[1] & 0x02) >> 1;

	  status = target_read_memory (pc + 2, buf, 5);
	  if (status != 0)
	    break;

	  sM = (buf[0] & 0xf0) >> 4;
	  rN = buf[0] & 0x0f;
	  fsM = (Y << 4) | sM;
	  d32 = extract_signed_integer (&buf[1], 4, byte_order);

	  stack.store (pv_add_constant (regs[translate_rreg (rN)], d32),
		       4, regs[E_FS0_REGNUM + fsM]);

	  pc += 7;
	}
      /* fmov fsM, (d8, SP) */
      else if (instr[0] == 0xfb && (instr[1] & 0xfd) == 0x34)
	{
	  int fsM, sM, Y;
	  LONGEST d8;
	  gdb_byte buf[2];

	  Y = (instr[1] & 0x02) >> 1;

	  status = target_read_memory (pc + 2, buf, 2);
	  if (status != 0)
	    break;

	  sM = (buf[0] & 0xf0) >> 4;
	  fsM = (Y << 4) | sM;
	  d8 = extract_signed_integer (&buf[1], 1, byte_order);

	  stack.store (pv_add_constant (regs[E_SP_REGNUM], d8),
		       4, regs[E_FS0_REGNUM + fsM]);

	  pc += 4;
	}
      /* fmov fsM, (d24, SP) */
      else if (instr[0] == 0xfd && (instr[1] & 0xfd) == 0x34)
	{
	  int fsM, sM, Y;
	  LONGEST d24;
	  gdb_byte buf[4];

	  Y = (instr[1] & 0x02) >> 1;

	  status = target_read_memory (pc + 2, buf, 4);
	  if (status != 0)
	    break;

	  sM = (buf[0] & 0xf0) >> 4;
	  fsM = (Y << 4) | sM;
	  d24 = extract_signed_integer (&buf[1], 3, byte_order);

	  stack.store (pv_add_constant (regs[E_SP_REGNUM], d24),
		       4, regs[E_FS0_REGNUM + fsM]);

	  pc += 6;
	}
      /* fmov fsM, (d32, SP) */
      else if (instr[0] == 0xfe && (instr[1] & 0xfd) == 0x34)
	{
	  int fsM, sM, Y;
	  LONGEST d32;
	  gdb_byte buf[5];

	  Y = (instr[1] & 0x02) >> 1;

	  status = target_read_memory (pc + 2, buf, 5);
	  if (status != 0)
	    break;

	  sM = (buf[0] & 0xf0) >> 4;
	  fsM = (Y << 4) | sM;
	  d32 = extract_signed_integer (&buf[1], 4, byte_order);

	  stack.store (pv_add_constant (regs[E_SP_REGNUM], d32),
		       4, regs[E_FS0_REGNUM + fsM]);

	  pc += 7;
	}
      /* fmov fsM, (rN+) */
      else if (instr[0] == 0xf9 && (instr[1] & 0xfd) == 0x31)
	{
	  int fsM, sM, Y, rN, rN_regnum;
	  gdb_byte buf[1];

	  Y = (instr[1] & 0x02) >> 1;

	  status = target_read_memory (pc + 2, buf, 1);
	  if (status != 0)
	    break;

	  sM = (buf[0] & 0xf0) >> 4;
	  rN = buf[0] & 0x0f;
	  fsM = (Y << 4) | sM;

	  rN_regnum = translate_rreg (rN);

	  stack.store (regs[rN_regnum], 4,
		       regs[E_FS0_REGNUM + fsM]);
	  regs[rN_regnum] = pv_add_constant (regs[rN_regnum], 4);

	  pc += 3;
	}
      /* fmov fsM, (rN+, imm8) */
      else if (instr[0] == 0xfb && (instr[1] & 0xfd) == 0x31)
	{
	  int fsM, sM, Y, rN, rN_regnum;
	  LONGEST imm8;
	  gdb_byte buf[2];

	  Y = (instr[1] & 0x02) >> 1;

	  status = target_read_memory (pc + 2, buf, 2);
	  if (status != 0)
	    break;

	  sM = (buf[0] & 0xf0) >> 4;
	  rN = buf[0] & 0x0f;
	  fsM = (Y << 4) | sM;
	  imm8 = extract_signed_integer (&buf[1], 1, byte_order);

	  rN_regnum = translate_rreg (rN);

	  stack.store (regs[rN_regnum], 4, regs[E_FS0_REGNUM + fsM]);
	  regs[rN_regnum] = pv_add_constant (regs[rN_regnum], imm8);

	  pc += 4;
	}
      /* fmov fsM, (rN+, imm24) */
      else if (instr[0] == 0xfd && (instr[1] & 0xfd) == 0x31)
	{
	  int fsM, sM, Y, rN, rN_regnum;
	  LONGEST imm24;
	  gdb_byte buf[4];

	  Y = (instr[1] & 0x02) >> 1;

	  status = target_read_memory (pc + 2, buf, 4);
	  if (status != 0)
	    break;

	  sM = (buf[0] & 0xf0) >> 4;
	  rN = buf[0] & 0x0f;
	  fsM = (Y << 4) | sM;
	  imm24 = extract_signed_integer (&buf[1], 3, byte_order);

	  rN_regnum = translate_rreg (rN);

	  stack.store (regs[rN_regnum], 4, regs[E_FS0_REGNUM + fsM]);
	  regs[rN_regnum] = pv_add_constant (regs[rN_regnum], imm24);

	  pc += 6;
	}
      /* fmov fsM, (rN+, imm32) */
      else if (instr[0] == 0xfe && (instr[1] & 0xfd) == 0x31)
	{
	  int fsM, sM, Y, rN, rN_regnum;
	  LONGEST imm32;
	  gdb_byte buf[5];

	  Y = (instr[1] & 0x02) >> 1;

	  status = target_read_memory (pc + 2, buf, 5);
	  if (status != 0)
	    break;

	  sM = (buf[0] & 0xf0) >> 4;
	  rN = buf[0] & 0x0f;
	  fsM = (Y << 4) | sM;
	  imm32 = extract_signed_integer (&buf[1], 4, byte_order);

	  rN_regnum = translate_rreg (rN);

	  stack.store (regs[rN_regnum], 4, regs[E_FS0_REGNUM + fsM]);
	  regs[rN_regnum] = pv_add_constant (regs[rN_regnum], imm32);

	  pc += 7;
	}
      /* mov imm8, aN */
      else if ((instr[0] & 0xf0) == 0x90)
	{
	  int aN = instr[0] & 0x03;
	  LONGEST imm8;

	  imm8 = extract_signed_integer (&instr[1], 1, byte_order);

	  regs[E_A0_REGNUM + aN] = pv_constant (imm8);
	  pc += 2;
	}
      /* mov imm16, aN */
      else if ((instr[0] & 0xfc) == 0x24)
	{
	  int aN = instr[0] & 0x03;
	  gdb_byte buf[2];
	  LONGEST imm16;

	  status = target_read_memory (pc + 1, buf, 2);
	  if (status != 0)
	    break;

	  imm16 = extract_signed_integer (buf, 2, byte_order);
	  regs[E_A0_REGNUM + aN] = pv_constant (imm16);
	  pc += 3;
	}
      /* mov imm32, aN */
      else if (instr[0] == 0xfc && ((instr[1] & 0xfc) == 0xdc))
	{
	  int aN = instr[1] & 0x03;
	  gdb_byte buf[4];
	  LONGEST imm32;

	  status = target_read_memory (pc + 2, buf, 4);
	  if (status != 0)
	    break;

	  imm32 = extract_signed_integer (buf, 4, byte_order);
	  regs[E_A0_REGNUM + aN] = pv_constant (imm32);
	  pc += 6;
	}
      /* mov imm8, dN */
      else if ((instr[0] & 0xf0) == 0x80)
	{
	  int dN = instr[0] & 0x03;
	  LONGEST imm8;

	  imm8 = extract_signed_integer (&instr[1], 1, byte_order);

	  regs[E_D0_REGNUM + dN] = pv_constant (imm8);
	  pc += 2;
	}
      /* mov imm16, dN */
      else if ((instr[0] & 0xfc) == 0x2c)
	{
	  int dN = instr[0] & 0x03;
	  gdb_byte buf[2];
	  LONGEST imm16;

	  status = target_read_memory (pc + 1, buf, 2);
	  if (status != 0)
	    break;

	  imm16 = extract_signed_integer (buf, 2, byte_order);
	  regs[E_D0_REGNUM + dN] = pv_constant (imm16);
	  pc += 3;
	}
      /* mov imm32, dN */
      else if (instr[0] == 0xfc && ((instr[1] & 0xfc) == 0xcc))
	{
	  int dN = instr[1] & 0x03;
	  gdb_byte buf[4];
	  LONGEST imm32;

	  status = target_read_memory (pc + 2, buf, 4);
	  if (status != 0)
	    break;

	  imm32 = extract_signed_integer (buf, 4, byte_order);
	  regs[E_D0_REGNUM + dN] = pv_constant (imm32);
	  pc += 6;
	}
      else
	{
	  /* We've hit some instruction that we don't recognize.  Hopefully,
	     we have enough to do prologue analysis.  */
	  break;
	}
    }

  /* Is the frame size (offset, really) a known constant?  */
  if (pv_is_register (regs[E_SP_REGNUM], E_SP_REGNUM))
    result->frame_size = regs[E_SP_REGNUM].k;

  /* Was the frame pointer initialized?  */
  if (pv_is_register (regs[E_A3_REGNUM], E_SP_REGNUM))
    {
      result->has_frame_ptr = 1;
      result->frame_ptr_offset = regs[E_A3_REGNUM].k;
    }

  /* Record where all the registers were saved.  */
  stack.scan (check_for_saved, (void *) result);

  result->prologue_end = after_last_frame_setup_insn;
}

/* Function: skip_prologue
   Return the address of the first inst past the prologue of the function.  */

static CORE_ADDR
mn10300_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  const char *name;
  CORE_ADDR func_addr, func_end;
  struct mn10300_prologue p;

  /* Try to find the extent of the function that contains PC.  */
  if (!find_pc_partial_function (pc, &name, &func_addr, &func_end))
    return pc;

  mn10300_analyze_prologue (gdbarch, pc, func_end, &p);
  return p.prologue_end;
}

/* Wrapper for mn10300_analyze_prologue: find the function start;
   use the current frame PC as the limit, then
   invoke mn10300_analyze_prologue and return its result.  */
static struct mn10300_prologue *
mn10300_analyze_frame_prologue (frame_info_ptr this_frame,
			   void **this_prologue_cache)
{
  if (!*this_prologue_cache)
    {
      CORE_ADDR func_start, stop_addr;

      *this_prologue_cache = FRAME_OBSTACK_ZALLOC (struct mn10300_prologue);

      func_start = get_frame_func (this_frame);
      stop_addr = get_frame_pc (this_frame);

      /* If we couldn't find any function containing the PC, then
	 just initialize the prologue cache, but don't do anything.  */
      if (!func_start)
	stop_addr = func_start;

      mn10300_analyze_prologue (get_frame_arch (this_frame),
				func_start, stop_addr,
				((struct mn10300_prologue *)
				 *this_prologue_cache));
    }

  return (struct mn10300_prologue *) *this_prologue_cache;
}

/* Given the next frame and a prologue cache, return this frame's
   base.  */
static CORE_ADDR
mn10300_frame_base (frame_info_ptr this_frame, void **this_prologue_cache)
{
  struct mn10300_prologue *p
    = mn10300_analyze_frame_prologue (this_frame, this_prologue_cache);

  /* In functions that use alloca, the distance between the stack
     pointer and the frame base varies dynamically, so we can't use
     the SP plus static information like prologue analysis to find the
     frame base.  However, such functions must have a frame pointer,
     to be able to restore the SP on exit.  So whenever we do have a
     frame pointer, use that to find the base.  */
  if (p->has_frame_ptr)
    {
      CORE_ADDR fp = get_frame_register_unsigned (this_frame, E_A3_REGNUM);
      return fp - p->frame_ptr_offset;
    }
  else
    {
      CORE_ADDR sp = get_frame_register_unsigned (this_frame, E_SP_REGNUM);
      return sp - p->frame_size;
    }
}

static void
mn10300_frame_this_id (frame_info_ptr this_frame,
		       void **this_prologue_cache,
		       struct frame_id *this_id)
{
  *this_id = frame_id_build (mn10300_frame_base (this_frame,
						 this_prologue_cache),
			     get_frame_func (this_frame));

}

static struct value *
mn10300_frame_prev_register (frame_info_ptr this_frame,
			     void **this_prologue_cache, int regnum)
{
  struct mn10300_prologue *p
    = mn10300_analyze_frame_prologue (this_frame, this_prologue_cache);
  CORE_ADDR frame_base = mn10300_frame_base (this_frame, this_prologue_cache);

  if (regnum == E_SP_REGNUM)
    return frame_unwind_got_constant (this_frame, regnum, frame_base);

  /* If prologue analysis says we saved this register somewhere,
     return a description of the stack slot holding it.  */
  if (p->reg_offset[regnum] != 1)
    return frame_unwind_got_memory (this_frame, regnum,
				    frame_base + p->reg_offset[regnum]);

  /* Otherwise, presume we haven't changed the value of this
     register, and get it from the next frame.  */
  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind mn10300_frame_unwind = {
  "mn10300 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  mn10300_frame_this_id, 
  mn10300_frame_prev_register,
  NULL,
  default_frame_sniffer
};

static void
mn10300_frame_unwind_init (struct gdbarch *gdbarch)
{
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &mn10300_frame_unwind);
}

/* Function: push_dummy_call
 *
 * Set up machine state for a target call, including
 * function arguments, stack, return address, etc.
 *
 */

static CORE_ADDR
mn10300_push_dummy_call (struct gdbarch *gdbarch, 
			 struct value *target_func,
			 struct regcache *regcache,
			 CORE_ADDR bp_addr, 
			 int nargs, struct value **args,
			 CORE_ADDR sp, 
			 function_call_return_method return_method,
			 CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  const int push_size = register_size (gdbarch, E_PC_REGNUM);
  int regs_used;
  int len, arg_len; 
  int stack_offset = 0;
  int argnum;
  const gdb_byte *val;
  gdb_byte valbuf[MN10300_MAX_REGISTER_SIZE];

  /* This should be a nop, but align the stack just in case something
     went wrong.  Stacks are four byte aligned on the mn10300.  */
  sp &= ~3;

  /* Now make space on the stack for the args.

     XXX This doesn't appear to handle pass-by-invisible reference
     arguments.  */
  regs_used = (return_method == return_method_struct) ? 1 : 0;
  for (len = 0, argnum = 0; argnum < nargs; argnum++)
    {
      arg_len = (args[argnum]->type ()->length () + 3) & ~3;
      while (regs_used < 2 && arg_len > 0)
	{
	  regs_used++;
	  arg_len -= push_size;
	}
      len += arg_len;
    }

  /* Allocate stack space.  */
  sp -= len;

  if (return_method == return_method_struct)
    {
      regs_used = 1;
      regcache_cooked_write_unsigned (regcache, E_D0_REGNUM, struct_addr);
    }
  else
    regs_used = 0;

  /* Push all arguments onto the stack.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      /* FIXME what about structs?  Unions?  */
      if ((*args)->type ()->code () == TYPE_CODE_STRUCT
	  && (*args)->type ()->length () > 8)
	{
	  /* Change to pointer-to-type.  */
	  arg_len = push_size;
	  gdb_assert (push_size <= MN10300_MAX_REGISTER_SIZE);
	  store_unsigned_integer (valbuf, push_size, byte_order,
				  (*args)->address ());
	  val = &valbuf[0];
	}
      else
	{
	  arg_len = (*args)->type ()->length ();
	  val = (*args)->contents ().data ();
	}

      while (regs_used < 2 && arg_len > 0)
	{
	  regcache_cooked_write_unsigned (regcache, regs_used, 
		  extract_unsigned_integer (val, push_size, byte_order));
	  val += push_size;
	  arg_len -= push_size;
	  regs_used++;
	}

      while (arg_len > 0)
	{
	  write_memory (sp + stack_offset, val, push_size);
	  arg_len -= push_size;
	  val += push_size;
	  stack_offset += push_size;
	}

      args++;
    }

  /* Make space for the flushback area.  */
  sp -= 8;

  /* Push the return address that contains the magic breakpoint.  */
  sp -= 4;
  write_memory_unsigned_integer (sp, push_size, byte_order, bp_addr);

  /* The CPU also writes the return address always into the
     MDR register on "call".  */
  regcache_cooked_write_unsigned (regcache, E_MDR_REGNUM, bp_addr);

  /* Update $sp.  */
  regcache_cooked_write_unsigned (regcache, E_SP_REGNUM, sp);

  /* On the mn10300, it's possible to move some of the stack adjustment
     and saving of the caller-save registers out of the prologue and
     into the call sites.  (When using gcc, this optimization can
     occur when using the -mrelax switch.) If this occurs, the dwarf2
     info will reflect this fact.  We can test to see if this is the
     case by creating a new frame using the current stack pointer and
     the address of the function that we're about to call.  We then
     unwind SP and see if it's different than the SP of our newly
     created frame.  If the SP values are the same, the caller is not
     expected to allocate any additional stack.  On the other hand, if
     the SP values are different, the difference determines the
     additional stack that must be allocated.
     
     Note that we don't update the return value though because that's
     the value of the stack just after pushing the arguments, but prior
     to performing the call.  This value is needed in order to
     construct the frame ID of the dummy call.  */
  {
    CORE_ADDR func_addr = find_function_addr (target_func, NULL);
    CORE_ADDR unwound_sp 
      = gdbarch_unwind_sp (gdbarch, create_new_frame (sp, func_addr));
    if (sp != unwound_sp)
      regcache_cooked_write_unsigned (regcache, E_SP_REGNUM,
				      sp - (unwound_sp - sp));
  }

  return sp;
}

/* If DWARF2 is a register number appearing in Dwarf2 debug info, then
   mn10300_dwarf2_reg_to_regnum (DWARF2) is the corresponding GDB
   register number.  Why don't Dwarf2 and GDB use the same numbering?
   Who knows?  But since people have object files lying around with
   the existing Dwarf2 numbering, and other people have written stubs
   to work with the existing GDB, neither of them can change.  So we
   just have to cope.  */
static int
mn10300_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, int dwarf2)
{
  /* This table is supposed to be shaped like the gdbarch_register_name
     initializer in gcc/config/mn10300/mn10300.h.  Registers which
     appear in GCC's numbering, but have no counterpart in GDB's
     world, are marked with a -1.  */
  static int dwarf2_to_gdb[] = {
    E_D0_REGNUM, E_D1_REGNUM, E_D2_REGNUM, E_D3_REGNUM,
    E_A0_REGNUM, E_A1_REGNUM, E_A2_REGNUM, E_A3_REGNUM,
    -1, E_SP_REGNUM,

    E_E0_REGNUM, E_E1_REGNUM, E_E2_REGNUM, E_E3_REGNUM,
    E_E4_REGNUM, E_E5_REGNUM, E_E6_REGNUM, E_E7_REGNUM,

    E_FS0_REGNUM + 0, E_FS0_REGNUM + 1, E_FS0_REGNUM + 2, E_FS0_REGNUM + 3,
    E_FS0_REGNUM + 4, E_FS0_REGNUM + 5, E_FS0_REGNUM + 6, E_FS0_REGNUM + 7,

    E_FS0_REGNUM + 8, E_FS0_REGNUM + 9, E_FS0_REGNUM + 10, E_FS0_REGNUM + 11,
    E_FS0_REGNUM + 12, E_FS0_REGNUM + 13, E_FS0_REGNUM + 14, E_FS0_REGNUM + 15,

    E_FS0_REGNUM + 16, E_FS0_REGNUM + 17, E_FS0_REGNUM + 18, E_FS0_REGNUM + 19,
    E_FS0_REGNUM + 20, E_FS0_REGNUM + 21, E_FS0_REGNUM + 22, E_FS0_REGNUM + 23,

    E_FS0_REGNUM + 24, E_FS0_REGNUM + 25, E_FS0_REGNUM + 26, E_FS0_REGNUM + 27,
    E_FS0_REGNUM + 28, E_FS0_REGNUM + 29, E_FS0_REGNUM + 30, E_FS0_REGNUM + 31,

    E_MDR_REGNUM, E_PSW_REGNUM, E_PC_REGNUM
  };

  if (dwarf2 < 0
      || dwarf2 >= ARRAY_SIZE (dwarf2_to_gdb))
    return -1;

  return dwarf2_to_gdb[dwarf2];
}

static struct gdbarch *
mn10300_gdbarch_init (struct gdbarch_info info,
		      struct gdbarch_list *arches)
{
  int num_regs;

  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new mn10300_gdbarch_tdep));
  mn10300_gdbarch_tdep *tdep = gdbarch_tdep<mn10300_gdbarch_tdep> (gdbarch);

  switch (info.bfd_arch_info->mach)
    {
    case 0:
    case bfd_mach_mn10300:
      set_gdbarch_register_name (gdbarch, mn10300_generic_register_name);
      tdep->am33_mode = 0;
      num_regs = 32;
      break;
    case bfd_mach_am33:
      set_gdbarch_register_name (gdbarch, am33_register_name);
      tdep->am33_mode = 1;
      num_regs = 32;
      break;
    case bfd_mach_am33_2:
      set_gdbarch_register_name (gdbarch, am33_2_register_name);
      tdep->am33_mode = 2;
      num_regs = 64;
      set_gdbarch_fp0_regnum (gdbarch, 32);
      break;
    default:
      internal_error (_("mn10300_gdbarch_init: Unknown mn10300 variant"));
      break;
    }

  /* By default, chars are unsigned.  */
  set_gdbarch_char_signed (gdbarch, 0);

  /* Registers.  */
  set_gdbarch_num_regs (gdbarch, num_regs);
  set_gdbarch_register_type (gdbarch, mn10300_register_type);
  set_gdbarch_skip_prologue (gdbarch, mn10300_skip_prologue);
  set_gdbarch_pc_regnum (gdbarch, E_PC_REGNUM);
  set_gdbarch_sp_regnum (gdbarch, E_SP_REGNUM);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, mn10300_dwarf2_reg_to_regnum);

  /* Stack unwinding.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  /* Breakpoints.  */
  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       mn10300_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       mn10300_breakpoint::bp_from_kind);
  /* decr_pc_after_break?  */

  /* Stage 2 */
  set_gdbarch_return_value (gdbarch, mn10300_return_value);
  
  /* Stage 3 -- get target calls working.  */
  set_gdbarch_push_dummy_call (gdbarch, mn10300_push_dummy_call);
  /* set_gdbarch_return_value (store, extract) */


  mn10300_frame_unwind_init (gdbarch);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  return gdbarch;
}
 
/* Dump out the mn10300 specific architecture information.  */

static void
mn10300_dump_tdep (struct gdbarch *gdbarch, struct ui_file *file)
{
  mn10300_gdbarch_tdep *tdep = gdbarch_tdep<mn10300_gdbarch_tdep> (gdbarch);
  gdb_printf (file, "mn10300_dump_tdep: am33_mode = %d\n",
	      tdep->am33_mode);
}

void _initialize_mn10300_tdep ();
void
_initialize_mn10300_tdep ()
{
  gdbarch_register (bfd_arch_mn10300, mn10300_gdbarch_init, mn10300_dump_tdep);
}

