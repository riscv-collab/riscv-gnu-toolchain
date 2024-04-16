/* Target dependent code for GDB on TI C6x systems.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.
   Contributed by Andrew Jenner <andrew@codesourcery.com>
   Contributed by Yao Qi <yao@codesourcery.com>

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
#include "target.h"
#include "dis-asm.h"
#include "regcache.h"
#include "value.h"
#include "symfile.h"
#include "arch-utils.h"
#include "glibc-tdep.h"
#include "infcall.h"
#include "regset.h"
#include "tramp-frame.h"
#include "linux-tdep.h"
#include "solib.h"
#include "objfiles.h"
#include "osabi.h"
#include "tic6x-tdep.h"
#include "language.h"
#include "target-descriptions.h"
#include <algorithm>

#define TIC6X_OPCODE_SIZE 4
#define TIC6X_FETCH_PACKET_SIZE 32

#define INST_S_BIT(INST) ((INST >> 1) & 1)
#define INST_X_BIT(INST) ((INST >> 12) & 1)

const gdb_byte tic6x_bkpt_illegal_opcode_be[] = { 0x56, 0x45, 0x43, 0x14 };
const gdb_byte tic6x_bkpt_illegal_opcode_le[] = { 0x14, 0x43, 0x45, 0x56 };

struct tic6x_unwind_cache
{
  /* The frame's base, optionally used by the high-level debug info.  */
  CORE_ADDR base;

  /* The previous frame's inner most stack address.  Used as this
     frame ID's stack_addr.  */
  CORE_ADDR cfa;

  /* The address of the first instruction in this function */
  CORE_ADDR pc;

  /* Which register holds the return address for the frame.  */
  int return_regnum;

  /* The offset of register saved on stack.  If register is not saved, the
     corresponding element is -1.  */
  CORE_ADDR reg_saved[TIC6X_NUM_CORE_REGS];
};


/* Name of TI C6x core registers.  */
static const char *const tic6x_register_names[] =
{
  "A0",  "A1",  "A2",  "A3",  /*  0  1  2  3 */
  "A4",  "A5",  "A6",  "A7",  /*  4  5  6  7 */
  "A8",  "A9",  "A10", "A11", /*  8  9 10 11 */
  "A12", "A13", "A14", "A15", /* 12 13 14 15 */
  "B0",  "B1",  "B2",  "B3",  /* 16 17 18 19 */
  "B4",  "B5",  "B6",  "B7",  /* 20 21 22 23 */
  "B8",  "B9",  "B10", "B11", /* 24 25 26 27 */
  "B12", "B13", "B14", "B15", /* 28 29 30 31 */
  "CSR", "PC",                /* 32 33       */
};

/* This array maps the arguments to the register number which passes argument
   in function call according to C6000 ELF ABI.  */
static const int arg_regs[] = { 4, 20, 6, 22, 8, 24, 10, 26, 12, 28 };

/* This is the implementation of gdbarch method register_name.  */

static const char *
tic6x_register_name (struct gdbarch *gdbarch, int regno)
{
  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return tdesc_register_name (gdbarch, regno);
  else if (regno >= ARRAY_SIZE (tic6x_register_names))
    return "";
  else
    return tic6x_register_names[regno];
}

/* This is the implementation of gdbarch method register_type.  */

static struct type *
tic6x_register_type (struct gdbarch *gdbarch, int regno)
{

  if (regno == TIC6X_PC_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;
  else
    return builtin_type (gdbarch)->builtin_uint32;
}

static void
tic6x_setup_default (struct tic6x_unwind_cache *cache)
{
  int i;

  for (i = 0; i < TIC6X_NUM_CORE_REGS; i++)
    cache->reg_saved[i] = -1;
}

static unsigned long tic6x_fetch_instruction (struct gdbarch *, CORE_ADDR);
static int tic6x_register_number (int reg, int side, int crosspath);

/* Do a full analysis of the prologue at START_PC and update CACHE accordingly.
   Bail out early if CURRENT_PC is reached.  Returns the address of the first
   instruction after the prologue.  */

static CORE_ADDR
tic6x_analyze_prologue (struct gdbarch *gdbarch, const CORE_ADDR start_pc,
			const CORE_ADDR current_pc,
			struct tic6x_unwind_cache *cache,
			frame_info_ptr this_frame)
{
  unsigned int src_reg, base_reg, dst_reg;
  int i;
  CORE_ADDR pc = start_pc;
  CORE_ADDR return_pc = start_pc;
  int frame_base_offset_to_sp = 0;
  /* Counter of non-stw instructions after first insn ` sub sp, xxx, sp'.  */
  int non_stw_insn_counter = 0;

  if (start_pc >= current_pc)
    return_pc = current_pc;

  cache->base = 0;

  /* The landmarks in prologue is one or two SUB instructions to SP.
     Instructions on setting up dsbt are in the last part of prologue, if
     needed.  In maxim, prologue can be divided to three parts by two
     `sub sp, xx, sp' insns.  */

  /* Step 1: Look for the 1st and 2nd insn `sub sp, xx, sp',  in which, the
     2nd one is optional.  */
  while (pc < current_pc)
    {
      unsigned long inst = tic6x_fetch_instruction (gdbarch, pc);

      if ((inst & 0x1ffc) == 0x1dc0 || (inst & 0x1ffc) == 0x1bc0
	  || (inst & 0x0ffc) == 0x9c0)
	{
	  /* SUBAW/SUBAH/SUB, and src1 is ucst 5.  */
	  unsigned int src2 = tic6x_register_number ((inst >> 18) & 0x1f,
						     INST_S_BIT (inst), 0);
	  unsigned int dst = tic6x_register_number ((inst >> 23) & 0x1f,
						    INST_S_BIT (inst), 0);

	  if (src2 == TIC6X_SP_REGNUM && dst == TIC6X_SP_REGNUM)
	    {
	      /* Extract const from insn SUBAW/SUBAH/SUB, and translate it to
		 offset.  The constant offset is decoded in bit 13-17 in all
		 these three kinds of instructions.  */
	      unsigned int ucst5 = (inst >> 13) & 0x1f;

	      if ((inst & 0x1ffc) == 0x1dc0)	/* SUBAW */
		frame_base_offset_to_sp += ucst5 << 2;
	      else if ((inst & 0x1ffc) == 0x1bc0)	/* SUBAH */
		frame_base_offset_to_sp += ucst5 << 1;
	      else if ((inst & 0x0ffc) == 0x9c0)	/* SUB */
		frame_base_offset_to_sp += ucst5;
	      else
		gdb_assert_not_reached ("unexpected instruction");

	      return_pc = pc + 4;
	    }
	}
      else if ((inst & 0x174) == 0x74)	/* stw SRC, *+b15(uconst) */
	{
	  /* The y bit determines which file base is read from.  */
	  base_reg = tic6x_register_number ((inst >> 18) & 0x1f,
					    (inst >> 7) & 1, 0);

	  if (base_reg == TIC6X_SP_REGNUM)
	    {
	      src_reg = tic6x_register_number ((inst >> 23) & 0x1f,
					       INST_S_BIT (inst), 0);

	      cache->reg_saved[src_reg] = ((inst >> 13) & 0x1f) << 2;

	      return_pc = pc + 4;
	    }
	  non_stw_insn_counter = 0;
	}
      else
	{
	  non_stw_insn_counter++;
	  /* Following instruction sequence may be emitted in prologue:

	     <+0>: subah .D2 b15,28,b15
	     <+4>: or .L2X 0,a4,b0
	     <+8>: || stw .D2T2 b14,*+b15(56)
	     <+12>:[!b0] b .S1 0xe50e4c1c <sleep+220>
	     <+16>:|| stw .D2T1 a10,*+b15(48)
	     <+20>:stw .D2T2 b3,*+b15(52)
	     <+24>:stw .D2T1 a4,*+b15(40)

	     we should look forward for next instruction instead of breaking loop
	     here.  So far, we allow almost two sequential non-stw instructions
	     in prologue.  */
	  if (non_stw_insn_counter >= 2)
	    break;
	}


      pc += 4;
    }
  /* Step 2: Skip insn on setting up dsbt if it is.  Usually, it looks like,
     ldw .D2T2 *+b14(0),b14 */
  unsigned long inst = tic6x_fetch_instruction (gdbarch, pc);
  /* The s bit determines which file dst will be loaded into, same effect as
     other places.  */
  dst_reg = tic6x_register_number ((inst >> 23) & 0x1f, (inst >> 1) & 1, 0);
  /* The y bit (bit 7), instead of s bit, determines which file base be
     used.  */
  base_reg = tic6x_register_number ((inst >> 18) & 0x1f, (inst >> 7) & 1, 0);

  if ((inst & 0x164) == 0x64	/* ldw */
      && dst_reg == TIC6X_DP_REGNUM	/* dst is B14 */
      && base_reg == TIC6X_DP_REGNUM)	/* baseR is B14 */
    {
      return_pc = pc + 4;
    }

  if (this_frame)
    {
      cache->base = get_frame_register_unsigned (this_frame, TIC6X_SP_REGNUM);

      if (cache->reg_saved[TIC6X_FP_REGNUM] != -1)
	{
	  /* If the FP now holds an offset from the CFA then this is a frame
	     which uses the frame pointer.  */

	  cache->cfa = get_frame_register_unsigned (this_frame,
						    TIC6X_FP_REGNUM);
	}
      else
	{
	  /* FP doesn't hold an offset from the CFA.  If SP still holds an
	     offset from the CFA then we might be in a function which omits
	     the frame pointer.  */

	  cache->cfa = cache->base + frame_base_offset_to_sp;
	}
    }

  /* Adjust all the saved registers such that they contain addresses
     instead of offsets.  */
  for (i = 0; i < TIC6X_NUM_CORE_REGS; i++)
    if (cache->reg_saved[i] != -1)
      cache->reg_saved[i] = cache->base + cache->reg_saved[i];

  return return_pc;
}

/* This is the implementation of gdbarch method skip_prologue.  */

static CORE_ADDR
tic6x_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  CORE_ADDR func_addr;
  struct tic6x_unwind_cache cache;

  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever is
     greater.  */
  if (find_pc_partial_function (start_pc, NULL, &func_addr, NULL))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);
      if (post_prologue_pc != 0)
	return std::max (start_pc, post_prologue_pc);
    }

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  */
  return tic6x_analyze_prologue (gdbarch, start_pc, (CORE_ADDR) -1, &cache,
				 NULL);
}

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
tic6x_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  return 4;
}

/* Implement the sw_breakpoint_from_kind gdbarch method.  */

static const gdb_byte *
tic6x_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  tic6x_gdbarch_tdep *tdep = gdbarch_tdep<tic6x_gdbarch_tdep> (gdbarch);

  *size = kind;

  if (tdep == NULL || tdep->breakpoint == NULL)
    {
      if (BFD_ENDIAN_BIG == gdbarch_byte_order_for_code (gdbarch))
	return tic6x_bkpt_illegal_opcode_be;
      else
	return tic6x_bkpt_illegal_opcode_le;
    }
  else
    return tdep->breakpoint;
}

static void
tic6x_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
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
     while the below is c6x specific.  */

  /* Callee save registers.  The ABI designates A10-A15 and B10-B15 as
     callee-save.  */
  else if ((regnum >= 10 && regnum <= 15) || (regnum >= 26 && regnum <= 31))
    reg->how = DWARF2_FRAME_REG_SAME_VALUE;
  else
    /* All other registers are caller-save.  */
    reg->how = DWARF2_FRAME_REG_UNDEFINED;
}

/* This is the implementation of gdbarch method unwind_pc.  */

static CORE_ADDR
tic6x_unwind_pc (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  gdb_byte buf[8];

  frame_unwind_register (next_frame,  TIC6X_PC_REGNUM, buf);
  return extract_typed_address (buf, builtin_type (gdbarch)->builtin_func_ptr);
}

/* Frame base handling.  */

static struct tic6x_unwind_cache*
tic6x_frame_unwind_cache (frame_info_ptr this_frame,
			  void **this_prologue_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR current_pc;
  struct tic6x_unwind_cache *cache;

  if (*this_prologue_cache)
    return (struct tic6x_unwind_cache *) *this_prologue_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct tic6x_unwind_cache);
  (*this_prologue_cache) = cache;

  cache->return_regnum = TIC6X_RA_REGNUM;

  tic6x_setup_default (cache);

  cache->pc = get_frame_func (this_frame);
  current_pc = get_frame_pc (this_frame);

  /* Prologue analysis does the rest...  */
  if (cache->pc != 0)
    tic6x_analyze_prologue (gdbarch, cache->pc, current_pc, cache, this_frame);

  return cache;
}

static void
tic6x_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		     struct frame_id *this_id)
{
  struct tic6x_unwind_cache *cache =
    tic6x_frame_unwind_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  (*this_id) = frame_id_build (cache->cfa, cache->pc);
}

static struct value *
tic6x_frame_prev_register (frame_info_ptr this_frame, void **this_cache,
			   int regnum)
{
  struct tic6x_unwind_cache *cache =
    tic6x_frame_unwind_cache (this_frame, this_cache);

  gdb_assert (regnum >= 0);

  /* The PC of the previous frame is stored in the RA register of
     the current frame.  Frob regnum so that we pull the value from
     the correct place.  */
  if (regnum == TIC6X_PC_REGNUM)
    regnum = cache->return_regnum;

  if (regnum == TIC6X_SP_REGNUM && cache->cfa)
    return frame_unwind_got_constant (this_frame, regnum, cache->cfa);

  /* If we've worked out where a register is stored then load it from
     there.  */
  if (regnum < TIC6X_NUM_CORE_REGS && cache->reg_saved[regnum] != -1)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->reg_saved[regnum]);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static CORE_ADDR
tic6x_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct tic6x_unwind_cache *info
    = tic6x_frame_unwind_cache (this_frame, this_cache);
  return info->base;
}

static const struct frame_unwind tic6x_frame_unwind =
{
  "tic6x prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  tic6x_frame_this_id,
  tic6x_frame_prev_register,
  NULL,
  default_frame_sniffer
};

static const struct frame_base tic6x_frame_base =
{
  &tic6x_frame_unwind,
  tic6x_frame_base_address,
  tic6x_frame_base_address,
  tic6x_frame_base_address
};


static struct tic6x_unwind_cache *
tic6x_make_stub_cache (frame_info_ptr this_frame)
{
  struct tic6x_unwind_cache *cache;

  cache = FRAME_OBSTACK_ZALLOC (struct tic6x_unwind_cache);

  cache->return_regnum = TIC6X_RA_REGNUM;

  tic6x_setup_default (cache);

  cache->cfa = get_frame_register_unsigned (this_frame, TIC6X_SP_REGNUM);

  return cache;
}

static void
tic6x_stub_this_id (frame_info_ptr this_frame, void **this_cache,
		    struct frame_id *this_id)
{
  struct tic6x_unwind_cache *cache;

  if (*this_cache == NULL)
    *this_cache = tic6x_make_stub_cache (this_frame);
  cache = (struct tic6x_unwind_cache *) *this_cache;

  *this_id = frame_id_build (cache->cfa, get_frame_pc (this_frame));
}

static int
tic6x_stub_unwind_sniffer (const struct frame_unwind *self,
			   frame_info_ptr this_frame,
			   void **this_prologue_cache)
{
  CORE_ADDR addr_in_block;

  addr_in_block = get_frame_address_in_block (this_frame);
  if (in_plt_section (addr_in_block))
    return 1;

  return 0;
}

static const struct frame_unwind tic6x_stub_unwind =
{
  "tic6x stub",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  tic6x_stub_this_id,
  tic6x_frame_prev_register,
  NULL,
  tic6x_stub_unwind_sniffer
};

/* Return the instruction on address PC.  */

static unsigned long
tic6x_fetch_instruction (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  return read_memory_unsigned_integer (pc, TIC6X_OPCODE_SIZE, byte_order);
}

/* Compute the condition of INST if it is a conditional instruction.  Always
   return 1 if INST is not a conditional instruction.  */

static int
tic6x_condition_true (struct regcache *regcache, unsigned long inst)
{
  int register_number;
  int register_value;
  static const int register_numbers[8] = { -1, 16, 17, 18, 1, 2, 0, -1 };

  register_number = register_numbers[(inst >> 29) & 7];
  if (register_number == -1)
    return 1;

  register_value = regcache_raw_get_signed (regcache, register_number);
  if ((inst & 0x10000000) != 0)
    return register_value == 0;
  return register_value != 0;
}

/* Get the register number by decoding raw bits REG, SIDE, and CROSSPATH in
   instruction.  */

static int
tic6x_register_number (int reg, int side, int crosspath)
{
  int r = (reg & 15) | ((crosspath ^ side) << 4);
  if ((reg & 16) != 0) /* A16 - A31, B16 - B31 */
    r += 37;
  return r;
}

static int
tic6x_extract_signed_field (int value, int low_bit, int bits)
{
  int mask = (1 << bits) - 1;
  int r = (value >> low_bit) & mask;
  if ((r & (1 << (bits - 1))) != 0)
    r -= mask + 1;
  return r;
}

/* Determine where to set a single step breakpoint.  */

static CORE_ADDR
tic6x_get_next_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();
  unsigned long inst;
  int register_number;
  int last = 0;

  do
    {
      inst = tic6x_fetch_instruction (gdbarch, pc);

      last = !(inst & 1);

      if (inst == TIC6X_INST_SWE)
	{
	  tic6x_gdbarch_tdep *tdep
	    = gdbarch_tdep<tic6x_gdbarch_tdep> (gdbarch);

	  if (tdep->syscall_next_pc != NULL)
	    return tdep->syscall_next_pc (get_current_frame ());
	}

      if (tic6x_condition_true (regcache, inst))
	{
	  if ((inst & 0x0000007c) == 0x00000010)
	    {
	      /* B with displacement */
	      pc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
	      pc += tic6x_extract_signed_field (inst, 7, 21) << 2;
	      break;
	    }
	  if ((inst & 0x0f83effc) == 0x00000360)
	    {
	      /* B with register */

	      register_number = tic6x_register_number ((inst >> 18) & 0x1f,
						       INST_S_BIT (inst),
						       INST_X_BIT (inst));
	      pc = regcache_raw_get_unsigned (regcache, register_number);
	      break;
	    }
	  if ((inst & 0x00001ffc) == 0x00001020)
	    {
	      /* BDEC */
	      register_number = tic6x_register_number ((inst >> 23) & 0x1f,
						       INST_S_BIT (inst), 0);
	      if (regcache_raw_get_signed (regcache, register_number) >= 0)
		{
		  pc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
		  pc += tic6x_extract_signed_field (inst, 7, 10) << 2;
		}
	      break;
	    }
	  if ((inst & 0x00001ffc) == 0x00000120)
	    {
	      /* BNOP with displacement */
	      pc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
	      pc += tic6x_extract_signed_field (inst, 16, 12) << 2;
	      break;
	    }
	  if ((inst & 0x0f830ffe) == 0x00800362)
	    {
	      /* BNOP with register */
	      register_number = tic6x_register_number ((inst >> 18) & 0x1f,
						       1, INST_X_BIT (inst));
	      pc = regcache_raw_get_unsigned (regcache, register_number);
	      break;
	    }
	  if ((inst & 0x00001ffc) == 0x00000020)
	    {
	      /* BPOS */
	      register_number = tic6x_register_number ((inst >> 23) & 0x1f,
						       INST_S_BIT (inst), 0);
	      if (regcache_raw_get_signed (regcache, register_number) >= 0)
		{
		  pc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
		  pc += tic6x_extract_signed_field (inst, 13, 10) << 2;
		}
	      break;
	    }
	  if ((inst & 0xf000007c) == 0x10000010)
	    {
	      /* CALLP */
	      pc &= ~(TIC6X_FETCH_PACKET_SIZE - 1);
	      pc += tic6x_extract_signed_field (inst, 7, 21) << 2;
	      break;
	    }
	}
      pc += TIC6X_OPCODE_SIZE;
    }
  while (!last);
  return pc;
}

/* This is the implementation of gdbarch method software_single_step.  */

static std::vector<CORE_ADDR>
tic6x_software_single_step (struct regcache *regcache)
{
  CORE_ADDR next_pc = tic6x_get_next_pc (regcache, regcache_read_pc (regcache));

  return {next_pc};
}

/* This is the implementation of gdbarch method frame_align.  */

static CORE_ADDR
tic6x_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return align_down (addr, 8);
}

/* Given a return value in REGCACHE with a type VALTYPE, extract and copy its
   value into VALBUF.  */

static void
tic6x_extract_return_value (struct type *valtype, struct regcache *regcache,
			    enum bfd_endian byte_order, gdb_byte *valbuf)
{
  int len = valtype->length ();

  /* pointer types are returned in register A4,
     up to 32-bit types in A4
     up to 64-bit types in A5:A4  */
  if (len <= 4)
    {
      /* In big-endian,
	 - one-byte structure or union occupies the LSB of single even register.
	 - for two-byte structure or union, the first byte occupies byte 1 of
	 register and the second byte occupies byte 0.
	 so, we read the contents in VAL from the LSBs of register.  */
      if (len < 3 && byte_order == BFD_ENDIAN_BIG)
	regcache->cooked_read_part (TIC6X_A4_REGNUM, 4 - len, len, valbuf);
      else
	regcache->cooked_read (TIC6X_A4_REGNUM, valbuf);
    }
  else if (len <= 8)
    {
      /* For a 5-8 byte structure or union in big-endian, the first byte
	 occupies byte 3 (the MSB) of the upper (odd) register and the
	 remaining bytes fill the decreasingly significant bytes.  5-7
	 byte structures or unions have padding in the LSBs of the
	 lower (even) register.  */
      if (byte_order == BFD_ENDIAN_BIG)
	{
	  regcache->cooked_read (TIC6X_A4_REGNUM, valbuf + 4);
	  regcache->cooked_read (TIC6X_A5_REGNUM, valbuf);
	}
      else
	{
	  regcache->cooked_read (TIC6X_A4_REGNUM, valbuf);
	  regcache->cooked_read (TIC6X_A5_REGNUM, valbuf + 4);
	}
    }
}

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  */

static void
tic6x_store_return_value (struct type *valtype, struct regcache *regcache,
			  enum bfd_endian byte_order, const gdb_byte *valbuf)
{
  int len = valtype->length ();

  /* return values of up to 8 bytes are returned in A5:A4 */

  if (len <= 4)
    {
      if (len < 3 && byte_order == BFD_ENDIAN_BIG)
	regcache->cooked_write_part (TIC6X_A4_REGNUM, 4 - len, len, valbuf);
      else
	regcache->cooked_write (TIC6X_A4_REGNUM, valbuf);
    }
  else if (len <= 8)
    {
      if (byte_order == BFD_ENDIAN_BIG)
	{
	  regcache->cooked_write (TIC6X_A4_REGNUM, valbuf + 4);
	  regcache->cooked_write (TIC6X_A5_REGNUM, valbuf);
	}
      else
	{
	  regcache->cooked_write (TIC6X_A4_REGNUM, valbuf);
	  regcache->cooked_write (TIC6X_A5_REGNUM, valbuf + 4);
	}
    }
}

/* This is the implementation of gdbarch method return_value.  */

static enum return_value_convention
tic6x_return_value (struct gdbarch *gdbarch, struct value *function,
		    struct type *type, struct regcache *regcache,
		    gdb_byte *readbuf, const gdb_byte *writebuf)
{
  /* In C++, when function returns an object, even its size is small
     enough, it stii has to be passed via reference, pointed by register
     A3.  */
  if (current_language->la_language == language_cplus)
    {
      if (type != NULL)
	{
	  type = check_typedef (type);
	  if (!(language_pass_by_reference (type).trivially_copyable))
	    return RETURN_VALUE_STRUCT_CONVENTION;
	}
    }

  if (type->length () > 8)
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf)
    tic6x_extract_return_value (type, regcache,
				gdbarch_byte_order (gdbarch), readbuf);
  if (writebuf)
    tic6x_store_return_value (type, regcache,
			      gdbarch_byte_order (gdbarch), writebuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}

/* Get the alignment requirement of TYPE.  */

static int
tic6x_arg_type_alignment (struct type *type)
{
  int len = check_typedef (type)->length ();
  enum type_code typecode = check_typedef (type)->code ();

  if (typecode == TYPE_CODE_STRUCT || typecode == TYPE_CODE_UNION)
    {
      /* The stack alignment of a structure (and union) passed by value is the
	 smallest power of two greater than or equal to its size.
	 This cannot exceed 8 bytes, which is the largest allowable size for
	 a structure passed by value.  */

      if (len <= 2)
	return len;
      else if (len <= 4)
	return 4;
      else if (len <= 8)
	return 8;
      else
	gdb_assert_not_reached ("unexpected length of data");
    }
  else
    {
      if (len <= 4)
	return 4;
      else if (len == 8)
	{
	  if (typecode == TYPE_CODE_COMPLEX)
	    return 4;
	  else
	    return 8;
	}
      else if (len == 16)
	{
	  if (typecode == TYPE_CODE_COMPLEX)
	    return 8;
	  else
	    return 16;
	}
      else
	internal_error (_("unexpected length %d of type"),
			len);
    }
}

/* This is the implementation of gdbarch method push_dummy_call.  */

static CORE_ADDR
tic6x_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		       struct regcache *regcache, CORE_ADDR bp_addr,
		       int nargs, struct value **args, CORE_ADDR sp,
		       function_call_return_method return_method,
		       CORE_ADDR struct_addr)
{
  int argreg = 0;
  int argnum;
  int stack_offset = 4;
  int references_offset = 4;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct type *func_type = function->type ();
  /* The first arg passed on stack.  Mostly the first 10 args are passed by
     registers.  */
  int first_arg_on_stack = 10;

  /* Set the return address register to point to the entry point of
     the program, where a breakpoint lies in wait.  */
  regcache_cooked_write_unsigned (regcache, TIC6X_RA_REGNUM, bp_addr);

  /* The caller must pass an argument in A3 containing a destination address
     for the returned value.  The callee returns the object by copying it to
     the address in A3.  */
  if (return_method == return_method_struct)
    regcache_cooked_write_unsigned (regcache, 3, struct_addr);

  /* Determine the type of this function.  */
  func_type = check_typedef (func_type);
  if (func_type->code () == TYPE_CODE_PTR)
    func_type = check_typedef (func_type->target_type ());

  gdb_assert (func_type->code () == TYPE_CODE_FUNC
	      || func_type->code () == TYPE_CODE_METHOD);

  /* For a variadic C function, the last explicitly declared argument and all
     remaining arguments are passed on the stack.  */
  if (func_type->has_varargs ())
    first_arg_on_stack = func_type->num_fields () - 1;

  /* Now make space on the stack for the args.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      int len = align_up (args[argnum]->type ()->length (), 4);
      if (argnum >= 10 - argreg)
	references_offset += len;
      stack_offset += len;
    }
  sp -= stack_offset;
  /* SP should be 8-byte aligned, see C6000 ABI section 4.4.1
     Stack Alignment.  */
  sp = align_down (sp, 8);
  stack_offset = 4;

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop through args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      const gdb_byte *val;
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (arg->type ());
      int len = arg_type->length ();
      enum type_code typecode = arg_type->code ();

      val = arg->contents ().data ();

      /* Copy the argument to general registers or the stack in
	 register-sized pieces.  */
      if (argreg < first_arg_on_stack)
	{
	  if (len <= 4)
	    {
	      if (typecode == TYPE_CODE_STRUCT || typecode == TYPE_CODE_UNION)
		{
		  /* In big-endian,
		     - one-byte structure or union occupies the LSB of single
		     even register.
		     - for two-byte structure or union, the first byte
		     occupies byte 1 of register and the second byte occupies
		     byte 0.
		     so, we write the contents in VAL to the lsp of
		     register.  */
		  if (len < 3 && byte_order == BFD_ENDIAN_BIG)
		    regcache->cooked_write_part (arg_regs[argreg], 4 - len, len,
						 val);
		  else
		    regcache->cooked_write (arg_regs[argreg], val);
		}
	      else
		{
		  /* The argument is being passed by value in a single
		     register.  */
		  CORE_ADDR regval = extract_unsigned_integer (val, len,
							       byte_order);

		  regcache_cooked_write_unsigned (regcache, arg_regs[argreg],
						  regval);
		}
	    }
	  else
	    {
	      if (len <= 8)
		{
		  if (typecode == TYPE_CODE_STRUCT
		      || typecode == TYPE_CODE_UNION)
		    {
		      /* For a 5-8 byte structure or union in big-endian, the
			 first byte occupies byte 3 (the MSB) of the upper (odd)
			 register and the remaining bytes fill the decreasingly
			 significant bytes.  5-7 byte structures or unions have
			 padding in the LSBs of the lower (even) register.  */
		      if (byte_order == BFD_ENDIAN_BIG)
			{
			  regcache->cooked_write (arg_regs[argreg] + 1, val);
			  regcache->cooked_write_part (arg_regs[argreg], 0,
						       len - 4, val + 4);
			}
		      else
			{
			  regcache->cooked_write (arg_regs[argreg], val);
			  regcache->cooked_write_part (arg_regs[argreg] + 1, 0,
						       len - 4, val + 4);
			}
		    }
		  else
		    {
		      /* The argument is being passed by value in a pair of
			 registers.  */
		      ULONGEST regval = extract_unsigned_integer (val, len,
								  byte_order);

		      regcache_cooked_write_unsigned (regcache,
						      arg_regs[argreg],
						      regval);
		      regcache_cooked_write_unsigned (regcache,
						      arg_regs[argreg] + 1,
						      regval >> 32);
		    }
		}
	      else
		{
		  /* The argument is being passed by reference in a single
		     register.  */
		  CORE_ADDR addr;

		  /* It is not necessary to adjust REFERENCES_OFFSET to
		     8-byte aligned in some cases, in which 4-byte alignment
		     is sufficient.  For simplicity, we adjust
		     REFERENCES_OFFSET to 8-byte aligned.  */
		  references_offset = align_up (references_offset, 8);

		  addr = sp + references_offset;
		  write_memory (addr, val, len);
		  references_offset += align_up (len, 4);
		  regcache_cooked_write_unsigned (regcache, arg_regs[argreg],
						  addr);
		}
	    }
	  argreg++;
	}
      else
	{
	  /* The argument is being passed on the stack.  */
	  CORE_ADDR addr;

	  /* There are six different cases of alignment, and these rules can
	     be found in tic6x_arg_type_alignment:

	     1) 4-byte aligned if size is less than or equal to 4 byte, such
	     as short, int, struct, union etc.
	     2) 8-byte aligned if size is less than or equal to 8-byte, such
	     as double, long long,
	     3) 4-byte aligned if it is of type _Complex float, even its size
	     is 8-byte.
	     4) 8-byte aligned if it is of type _Complex double or _Complex
	     long double, even its size is 16-byte.  Because, the address of
	     variable is passed as reference.
	     5) struct and union larger than 8-byte are passed by reference, so
	     it is 4-byte aligned.
	     6) struct and union of size between 4 byte and 8 byte varies.
	     alignment of struct variable is the alignment of its first field,
	     while alignment of union variable is the max of all its fields'
	     alignment.  */

	  if (len <= 4)
	    ; /* Default is 4-byte aligned.  Nothing to be done.  */
	  else if (len <= 8)
	    stack_offset = align_up (stack_offset,
				     tic6x_arg_type_alignment (arg_type));
	  else if (len == 16)
	    {
	      /* _Complex double or _Complex long double */
	      if (typecode == TYPE_CODE_COMPLEX)
		{
		  /* The argument is being passed by reference on stack.  */
		  references_offset = align_up (references_offset, 8);

		  addr = sp + references_offset;
		  /* Store variable on stack.  */
		  write_memory (addr, val, len);

		  references_offset += align_up (len, 4);

		  /* Pass the address of variable on stack as reference.  */
		  store_unsigned_integer ((gdb_byte *) val, 4, byte_order,
					  addr);
		  len = 4;

		}
	      else
		internal_error (_("unexpected type %d of arg %d"),
				typecode, argnum);
	    }
	  else
	    internal_error (_("unexpected length %d of arg %d"), len, argnum);

	  addr = sp + stack_offset;
	  write_memory (addr, val, len);
	  stack_offset += align_up (len, 4);
	}
    }

  regcache_cooked_write_signed (regcache, TIC6X_SP_REGNUM, sp);

  /* Return adjusted stack pointer.  */
  return sp;
}

/* This is the implementation of gdbarch method stack_frame_destroyed_p.  */

static int
tic6x_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  unsigned long inst = tic6x_fetch_instruction (gdbarch, pc);
  /* Normally, the epilogue is composed by instruction `b .S2 b3'.  */
  if ((inst & 0x0f83effc) == 0x360)
    {
      unsigned int src2 = tic6x_register_number ((inst >> 18) & 0x1f,
						 INST_S_BIT (inst),
						 INST_X_BIT (inst));
      if (src2 == TIC6X_RA_REGNUM)
	return 1;
    }

  return 0;
}

/* This is the implementation of gdbarch method get_longjmp_target.  */

static int
tic6x_get_longjmp_target (frame_info_ptr frame, CORE_ADDR *pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR jb_addr;
  gdb_byte buf[4];

  /* JMP_BUF is passed by reference in A4.  */
  jb_addr = get_frame_register_unsigned (frame, 4);

  /* JMP_BUF contains 13 elements of type int, and return address is stored
     in the last slot.  */
  if (target_read_memory (jb_addr + 12 * 4, buf, 4))
    return 0;

  *pc = extract_unsigned_integer (buf, 4, byte_order);

  return 1;
}

/* This is the implementation of gdbarch method
   return_in_first_hidden_param_p.  */

static int
tic6x_return_in_first_hidden_param_p (struct gdbarch *gdbarch,
				      struct type *type)
{
  return 0;
}

static struct gdbarch *
tic6x_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  tdesc_arch_data_up tdesc_data;
  const struct target_desc *tdesc = info.target_desc;
  int has_gp = 0;

  /* Check any target description for validity.  */
  if (tdesc_has_registers (tdesc))
    {
      const struct tdesc_feature *feature;
      int valid_p, i;

      feature = tdesc_find_feature (tdesc, "org.gnu.gdb.tic6x.core");

      if (feature == NULL)
	return NULL;

      tdesc_data = tdesc_data_alloc ();

      valid_p = 1;
      for (i = 0; i < 32; i++)	/* A0 - A15, B0 - B15 */
	valid_p &= tdesc_numbered_register (feature, tdesc_data.get (), i,
					    tic6x_register_names[i]);

      /* CSR */
      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (), i++,
					  tic6x_register_names[TIC6X_CSR_REGNUM]);
      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (), i++,
					  tic6x_register_names[TIC6X_PC_REGNUM]);

      if (!valid_p)
	return NULL;

      feature = tdesc_find_feature (tdesc, "org.gnu.gdb.tic6x.gp");
      if (feature)
	{
	  int j = 0;
	  static const char *const gp[] =
	    {
	      "A16", "A17", "A18", "A19", "A20", "A21", "A22", "A23",
	      "A24", "A25", "A26", "A27", "A28", "A29", "A30", "A31",
	      "B16", "B17", "B18", "B19", "B20", "B21", "B22", "B23",
	      "B24", "B25", "B26", "B27", "B28", "B29", "B30", "B31",
	    };

	  has_gp = 1;
	  valid_p = 1;
	  for (j = 0; j < 32; j++)	/* A16 - A31, B16 - B31 */
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						i++, gp[j]);

	  if (!valid_p)
	    return NULL;
	}

      feature = tdesc_find_feature (tdesc, "org.gnu.gdb.tic6x.c6xp");
      if (feature)
	{
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      i++, "TSR");
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      i++, "ILC");
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      i++, "RILC");

	  if (!valid_p)
	    return NULL;
	}

    }

  /* Find a candidate among extant architectures.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      tic6x_gdbarch_tdep *tdep
	= gdbarch_tdep<tic6x_gdbarch_tdep> (arches->gdbarch);

      if (has_gp != tdep->has_gp)
	continue;

      if (tdep && tdep->breakpoint)
	return arches->gdbarch;
    }

  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new tic6x_gdbarch_tdep));
  tic6x_gdbarch_tdep *tdep = gdbarch_tdep<tic6x_gdbarch_tdep> (gdbarch);

  tdep->has_gp = has_gp;

  /* Data type sizes.  */
  set_gdbarch_ptr_bit (gdbarch, 32);
  set_gdbarch_addr_bit (gdbarch, 32);
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);

  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_double);

  /* The register set.  */
  set_gdbarch_num_regs (gdbarch, TIC6X_NUM_REGS);
  set_gdbarch_sp_regnum (gdbarch, TIC6X_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, TIC6X_PC_REGNUM);

  set_gdbarch_register_name (gdbarch, tic6x_register_name);
  set_gdbarch_register_type (gdbarch, tic6x_register_type);

  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_skip_prologue (gdbarch, tic6x_skip_prologue);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       tic6x_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       tic6x_sw_breakpoint_from_kind);

  set_gdbarch_unwind_pc (gdbarch, tic6x_unwind_pc);

  /* Unwinding.  */
  dwarf2_append_unwinders (gdbarch);

  frame_unwind_append_unwinder (gdbarch, &tic6x_stub_unwind);
  frame_unwind_append_unwinder (gdbarch, &tic6x_frame_unwind);
  frame_base_set_default (gdbarch, &tic6x_frame_base);

  dwarf2_frame_set_init_reg (gdbarch, tic6x_dwarf2_frame_init_reg);

  /* Single stepping.  */
  set_gdbarch_software_single_step (gdbarch, tic6x_software_single_step);

  /* Call dummy code.  */
  set_gdbarch_frame_align (gdbarch, tic6x_frame_align);

  set_gdbarch_return_value (gdbarch, tic6x_return_value);

  /* Enable inferior call support.  */
  set_gdbarch_push_dummy_call (gdbarch, tic6x_push_dummy_call);

  set_gdbarch_get_longjmp_target (gdbarch, tic6x_get_longjmp_target);

  set_gdbarch_stack_frame_destroyed_p (gdbarch, tic6x_stack_frame_destroyed_p);

  set_gdbarch_return_in_first_hidden_param_p (gdbarch,
					      tic6x_return_in_first_hidden_param_p);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  if (tdesc_data != nullptr)
    tdesc_use_registers (gdbarch, tdesc, std::move (tdesc_data));

  return gdbarch;
}

void _initialize_tic6x_tdep ();
void
_initialize_tic6x_tdep ()
{
  gdbarch_register (bfd_arch_tic6x, tic6x_gdbarch_init);
}
