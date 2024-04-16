/* Target-dependent code for the Tilera TILE-Gx processor.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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
#include "frame-unwind.h"
#include "dwarf2/frame.h"
#include "trad-frame.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "value.h"
#include "dis-asm.h"
#include "inferior.h"
#include "arch-utils.h"
#include "regcache.h"
#include "regset.h"
#include "osabi.h"
#include "linux-tdep.h"
#include "objfiles.h"
#include "solib-svr4.h"
#include "tilegx-tdep.h"
#include "opcode/tilegx.h"
#include <algorithm>
#include "gdbsupport/byte-vector.h"

struct tilegx_frame_cache
{
  /* Base address.  */
  CORE_ADDR base;
  /* Function start.  */
  CORE_ADDR start_pc;

  /* Table of saved registers.  */
  trad_frame_saved_reg *saved_regs;
};

/* Register state values used by analyze_prologue.  */
enum reverse_state
  {
    REVERSE_STATE_REGISTER,
    REVERSE_STATE_VALUE,
    REVERSE_STATE_UNKNOWN
  };

/* Register state used by analyze_prologue().  */
struct tilegx_reverse_regs
{
  LONGEST value;
  enum reverse_state state;
};

static const struct tilegx_reverse_regs
template_reverse_regs[TILEGX_NUM_PHYS_REGS] =
  {
    { TILEGX_R0_REGNUM,  REVERSE_STATE_REGISTER },
    { TILEGX_R1_REGNUM,  REVERSE_STATE_REGISTER },
    { TILEGX_R2_REGNUM,  REVERSE_STATE_REGISTER },
    { TILEGX_R3_REGNUM,  REVERSE_STATE_REGISTER },
    { TILEGX_R4_REGNUM,  REVERSE_STATE_REGISTER },
    { TILEGX_R5_REGNUM,  REVERSE_STATE_REGISTER },
    { TILEGX_R6_REGNUM,  REVERSE_STATE_REGISTER },
    { TILEGX_R7_REGNUM,  REVERSE_STATE_REGISTER },
    { TILEGX_R8_REGNUM,  REVERSE_STATE_REGISTER },
    { TILEGX_R9_REGNUM,  REVERSE_STATE_REGISTER },
    { TILEGX_R10_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R11_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R12_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R13_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R14_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R15_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R16_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R17_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R18_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R19_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R20_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R21_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R22_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R23_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R24_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R25_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R26_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R27_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R28_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R29_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R30_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R31_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R32_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R33_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R34_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R35_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R36_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R37_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R38_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R39_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R40_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R41_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R42_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R43_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R44_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R45_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R46_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R47_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R48_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R49_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R50_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R51_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_R52_REGNUM, REVERSE_STATE_REGISTER },
    { TILEGX_TP_REGNUM,  REVERSE_STATE_REGISTER },
    { TILEGX_SP_REGNUM,  REVERSE_STATE_REGISTER },
    { TILEGX_LR_REGNUM,  REVERSE_STATE_REGISTER },
    { 0, REVERSE_STATE_UNKNOWN },
    { 0, REVERSE_STATE_UNKNOWN },
    { 0, REVERSE_STATE_UNKNOWN },
    { 0, REVERSE_STATE_UNKNOWN },
    { 0, REVERSE_STATE_UNKNOWN },
    { 0, REVERSE_STATE_UNKNOWN },
    { 0, REVERSE_STATE_UNKNOWN },
    { TILEGX_ZERO_REGNUM, REVERSE_STATE_VALUE }
  };

/* Implement the "register_name" gdbarch method.  */

static const char *
tilegx_register_name (struct gdbarch *gdbarch, int regnum)
{
  static const char *const register_names[] =
    {
      "r0",   "r1",   "r2",   "r3",   "r4",   "r5",   "r6",   "r7",
      "r8",   "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",
      "r16",  "r17",  "r18",  "r19",  "r20",  "r21",  "r22",  "r23",
      "r24",  "r25",  "r26",  "r27",  "r28",  "r29",  "r30",  "r31",
      "r32",  "r33",  "r34",  "r35",  "r36",  "r37",  "r38",  "r39",
      "r40",  "r41",  "r42",  "r43",  "r44",  "r45",  "r46",  "r47",
      "r48",  "r49",  "r50",  "r51",  "r52",  "tp",   "sp",   "lr",
      "sn",   "idn0", "idn1", "udn0", "udn1", "udn2", "udn3", "zero",
      "pc",   "faultnum",
    };

  static_assert (TILEGX_NUM_REGS == ARRAY_SIZE (register_names));
  return register_names[regnum];
}

/* This is the implementation of gdbarch method register_type.  */

static struct type *
tilegx_register_type (struct gdbarch *gdbarch, int regnum)
{
  if (regnum == TILEGX_PC_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;
  else
    return builtin_type (gdbarch)->builtin_uint64;
}

/* This is the implementation of gdbarch method dwarf2_reg_to_regnum.  */

static int
tilegx_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, int num)
{
  return num;
}

/* Makes the decision of whether a given type is a scalar type.
   Scalar types are returned in the registers r2-r11 as they fit.  */

static int
tilegx_type_is_scalar (struct type *t)
{
  return (t->code () != TYPE_CODE_STRUCT
	  && t->code () != TYPE_CODE_UNION
	  && t->code () != TYPE_CODE_ARRAY);
}

/* Returns non-zero if the given struct type will be returned using
   a special convention, rather than the normal function return method.
   Used in the context of the "return" command, and target function
   calls from the debugger.  */

static int
tilegx_use_struct_convention (struct type *type)
{
  /* Only scalars which fit in R0 - R9 can be returned in registers.
     Otherwise, they are returned via a pointer passed in R0.  */
  return (!tilegx_type_is_scalar (type)
	  && (type->length () > (1 + TILEGX_R9_REGNUM - TILEGX_R0_REGNUM)
	      * tilegx_reg_size));
}

/* Find a function's return value in the appropriate registers (in
   REGCACHE), and copy it into VALBUF.  */

static void
tilegx_extract_return_value (struct type *type, struct regcache *regcache,
			     gdb_byte *valbuf)
{
  int len = type->length ();
  int i, regnum = TILEGX_R0_REGNUM;

  for (i = 0; i < len; i += tilegx_reg_size)
    regcache->raw_read (regnum++, valbuf + i);
}

/* Copy the function return value from VALBUF into the proper
   location for a function return.
   Called only in the context of the "return" command.  */

static void
tilegx_store_return_value (struct type *type, struct regcache *regcache,
			   const void *valbuf)
{
  if (type->length () < tilegx_reg_size)
    {
      /* Add leading zeros to the (little-endian) value.  */
      gdb_byte buf[tilegx_reg_size] = { 0 };

      memcpy (buf, valbuf, type->length ());
      regcache->raw_write (TILEGX_R0_REGNUM, buf);
    }
  else
    {
      int len = type->length ();
      int i, regnum = TILEGX_R0_REGNUM;

      for (i = 0; i < len; i += tilegx_reg_size)
	regcache->raw_write (regnum++, (gdb_byte *) valbuf + i);
    }
}

/* This is the implementation of gdbarch method return_value.  */

static enum return_value_convention
tilegx_return_value (struct gdbarch *gdbarch, struct value *function,
		     struct type *type, struct regcache *regcache,
		     gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (tilegx_use_struct_convention (type))
    return RETURN_VALUE_STRUCT_CONVENTION;
  if (writebuf)
    tilegx_store_return_value (type, regcache, writebuf);
  else if (readbuf)
    tilegx_extract_return_value (type, regcache, readbuf);
  return RETURN_VALUE_REGISTER_CONVENTION;
}

/* This is the implementation of gdbarch method frame_align.  */

static CORE_ADDR
tilegx_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return addr & -8;
}


/* Implement the "push_dummy_call" gdbarch method.  */

static CORE_ADDR
tilegx_push_dummy_call (struct gdbarch *gdbarch,
			struct value *function,
			struct regcache *regcache,
			CORE_ADDR bp_addr, int nargs,
			struct value **args,
			CORE_ADDR sp, function_call_return_method return_method,
			CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR stack_dest = sp;
  int argreg = TILEGX_R0_REGNUM;
  int i, j;
  int typelen, slacklen;
  static const gdb_byte four_zero_words[16] = { 0 };

  /* If struct_return is 1, then the struct return address will
     consume one argument-passing register.  */
  if (return_method == return_method_struct)
    regcache_cooked_write_unsigned (regcache, argreg++, struct_addr);

  /* Arguments are passed in R0 - R9, and as soon as an argument
     will not fit completely in the remaining registers, then it,
     and all remaining arguments, are put on the stack.  */
  for (i = 0; i < nargs && argreg <= TILEGX_R9_REGNUM; i++)
    {
      const gdb_byte *val;
      typelen = args[i]->enclosing_type ()->length ();

      if (typelen > (TILEGX_R9_REGNUM - argreg + 1) * tilegx_reg_size)
	break;

      /* Put argument into registers wordwise.	*/
      val = args[i]->contents ().data ();
      for (j = 0; j < typelen; j += tilegx_reg_size)
	{
	  /* ISSUE: Why special handling for "typelen = 4x + 1"?
	     I don't ever see "typelen" values except 4 and 8.	*/
	  int n = (typelen - j == 1) ? 1 : tilegx_reg_size;
	  ULONGEST w = extract_unsigned_integer (val + j, n, byte_order);

	  regcache_cooked_write_unsigned (regcache, argreg++, w);
	}
    }

  /* Align SP.  */
  stack_dest = tilegx_frame_align (gdbarch, stack_dest);

  /* Loop backwards through remaining arguments and push them on
     the stack, word aligned.  */
  for (j = nargs - 1; j >= i; j--)
    {
      const gdb_byte *contents = args[j]->contents ().data ();

      typelen = args[j]->enclosing_type ()->length ();
      slacklen = align_up (typelen, 8) - typelen;
      gdb::byte_vector val (typelen + slacklen);
      memcpy (val.data (), contents, typelen);
      memset (val.data () + typelen, 0, slacklen);

      /* Now write data to the stack.  The stack grows downwards.  */
      stack_dest -= typelen + slacklen;
      write_memory (stack_dest, val.data (), typelen + slacklen);
    }

  /* Add 16 bytes for linkage space to the stack.  */
  stack_dest = stack_dest - 16;
  write_memory (stack_dest, four_zero_words, 16);

  /* Update stack pointer.  */
  regcache_cooked_write_unsigned (regcache, TILEGX_SP_REGNUM, stack_dest);

  /* Set the return address register to point to the entry point of
     the program, where a breakpoint lies in wait.  */
  regcache_cooked_write_unsigned (regcache, TILEGX_LR_REGNUM, bp_addr);

  return stack_dest;
}


/* Decode the instructions within the given address range.
   Decide when we must have reached the end of the function prologue.
   If a frame_info pointer is provided, fill in its saved_regs etc.
   Returns the address of the first instruction after the prologue.
   NOTE: This is often called with start_addr being the start of some
   function, and end_addr being the current PC.  */

static CORE_ADDR
tilegx_analyze_prologue (struct gdbarch* gdbarch,
			 CORE_ADDR start_addr, CORE_ADDR end_addr,
			 struct tilegx_frame_cache *cache,
			 frame_info_ptr next_frame)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR next_addr;
  CORE_ADDR prolog_end = end_addr;
  gdb_byte instbuf[32 * TILEGX_BUNDLE_SIZE_IN_BYTES];
  CORE_ADDR instbuf_start;
  unsigned int instbuf_size;
  int status;
  uint64_t bundle;
  struct tilegx_decoded_instruction
    decoded[TILEGX_MAX_INSTRUCTIONS_PER_BUNDLE];
  int num_insns;
  struct tilegx_reverse_regs reverse_frame[TILEGX_NUM_PHYS_REGS];
  struct tilegx_reverse_regs
    new_reverse_frame[TILEGX_MAX_INSTRUCTIONS_PER_BUNDLE];
  int dest_regs[TILEGX_MAX_INSTRUCTIONS_PER_BUNDLE];
  int reverse_frame_valid, prolog_done, branch_seen, lr_saved_on_stack_p;
  LONGEST prev_sp_value;
  int i, j;

  if (start_addr >= end_addr
      || (start_addr % TILEGX_BUNDLE_ALIGNMENT_IN_BYTES) != 0)
    return end_addr;

  /* Initialize the reverse frame.  This maps the CURRENT frame's
     registers to the outer frame's registers (the frame on the
     stack goes the other way).  */
  memcpy (&reverse_frame, &template_reverse_regs, sizeof (reverse_frame));

  prolog_done = 0;
  branch_seen = 0;
  prev_sp_value = 0;
  lr_saved_on_stack_p = 0;

  /* To cut down on round-trip overhead, we fetch multiple bundles
     at once.  These variables describe the range of memory we have
     prefetched.  */
  instbuf_start = 0;
  instbuf_size = 0;

  for (next_addr = start_addr;
       next_addr < end_addr;
       next_addr += TILEGX_BUNDLE_SIZE_IN_BYTES)
    {
      /* Retrieve the next instruction.  */
      if (next_addr - instbuf_start >= instbuf_size)
	{
	  /* Figure out how many bytes to fetch.  Don't span a page
	     boundary since that might cause an unnecessary memory
	     error.  */
	  unsigned int size_on_same_page = 4096 - (next_addr & 4095);

	  instbuf_size = sizeof instbuf;

	  if (instbuf_size > size_on_same_page)
	    instbuf_size = size_on_same_page;

	  instbuf_size = std::min ((CORE_ADDR) instbuf_size,
				   (end_addr - next_addr));
	  instbuf_start = next_addr;

	  status = safe_frame_unwind_memory (next_frame, instbuf_start,
					     {instbuf, instbuf_size});
	  if (status == 0)
	    memory_error (TARGET_XFER_E_IO, next_addr);
	}

      reverse_frame_valid = 0;

      bundle = extract_unsigned_integer (&instbuf[next_addr - instbuf_start],
					 8, byte_order);

      num_insns = parse_insn_tilegx (bundle, next_addr, decoded);

      for (i = 0; i < num_insns; i++)
	{
	  struct tilegx_decoded_instruction *this_insn = &decoded[i];
	  long long *operands = this_insn->operand_values;
	  const struct tilegx_opcode *opcode = this_insn->opcode;

	  switch (opcode->mnemonic)
	    {
	    case TILEGX_OPC_ST:
	      if (cache
		  && reverse_frame[operands[0]].state == REVERSE_STATE_VALUE
		  && reverse_frame[operands[1]].state
		  == REVERSE_STATE_REGISTER)
		{
		  LONGEST saved_address = reverse_frame[operands[0]].value;
		  unsigned saved_register
		    = (unsigned) reverse_frame[operands[1]].value;

		  cache->saved_regs[saved_register].set_addr (saved_address);
		} 
	      else if (cache
		       && (operands[0] == TILEGX_SP_REGNUM) 
		       && (operands[1] == TILEGX_LR_REGNUM))
		lr_saved_on_stack_p = 1;
	      break;
	    case TILEGX_OPC_ADDI:
	    case TILEGX_OPC_ADDLI:
	      if (cache
		  && operands[0] == TILEGX_SP_REGNUM
		  && operands[1] == TILEGX_SP_REGNUM
		  && reverse_frame[operands[1]].state == REVERSE_STATE_REGISTER)
		{
		  /* Special case.  We're fixing up the stack frame.  */
		  uint64_t hopefully_sp
		    = (unsigned) reverse_frame[operands[1]].value;
		  short op2_as_short = (short) operands[2];
		  signed char op2_as_char = (signed char) operands[2];

		  /* Fix up the sign-extension.  */
		  if (opcode->mnemonic == TILEGX_OPC_ADDI)
		    op2_as_short = op2_as_char;
		  prev_sp_value = (cache->saved_regs[hopefully_sp].addr ()
				   - op2_as_short);

		  new_reverse_frame[i].state = REVERSE_STATE_VALUE;
		  new_reverse_frame[i].value
		    = cache->saved_regs[hopefully_sp].addr ();
		  cache->saved_regs[hopefully_sp].set_value (prev_sp_value);
		}
	      else
		{
		  short op2_as_short = (short) operands[2];
		  signed char op2_as_char = (signed char) operands[2];

		  /* Fix up the sign-extension.  */
		  if (opcode->mnemonic == TILEGX_OPC_ADDI)
		    op2_as_short = op2_as_char;

		  new_reverse_frame[i] = reverse_frame[operands[1]];
		  if (new_reverse_frame[i].state == REVERSE_STATE_VALUE)
		    new_reverse_frame[i].value += op2_as_short;
		  else
		    new_reverse_frame[i].state = REVERSE_STATE_UNKNOWN;
		}
	      reverse_frame_valid |= 1 << i;
	      dest_regs[i] = operands[0];
	      break;
	    case TILEGX_OPC_ADD:
	      if (reverse_frame[operands[1]].state == REVERSE_STATE_VALUE
		  && reverse_frame[operands[2]].state == REVERSE_STATE_VALUE)
		{
		  /* We have values -- we can do this.  */
		  new_reverse_frame[i] = reverse_frame[operands[2]];
		  new_reverse_frame[i].value
		    += reverse_frame[operands[i]].value;
		}
	      else
		{
		  /* We don't know anything about the values.  Punt.  */
		  new_reverse_frame[i].state = REVERSE_STATE_UNKNOWN;
		}
	      reverse_frame_valid |= 1 << i;
	      dest_regs[i] = operands[0];
	      break;
	    case TILEGX_OPC_MOVE:
	      new_reverse_frame[i] = reverse_frame[operands[1]];
	      reverse_frame_valid |= 1 << i;
	      dest_regs[i] = operands[0];
	      break;
	    case TILEGX_OPC_MOVEI:
	    case TILEGX_OPC_MOVELI:
	      new_reverse_frame[i].state = REVERSE_STATE_VALUE;
	      new_reverse_frame[i].value = operands[1];
	      reverse_frame_valid |= 1 << i;
	      dest_regs[i] = operands[0];
	      break;
	    case TILEGX_OPC_ORI:
	      if (reverse_frame[operands[1]].state == REVERSE_STATE_VALUE)
		{
		  /* We have a value in A -- we can do this.  */
		  new_reverse_frame[i] = reverse_frame[operands[1]];
		  new_reverse_frame[i].value
		    = reverse_frame[operands[1]].value | operands[2];
		}
	      else if (operands[2] == 0)
		{
		  /* This is a move.  */
		  new_reverse_frame[i] = reverse_frame[operands[1]];
		}
	      else
		{
		  /* We don't know anything about the values.  Punt.  */
		  new_reverse_frame[i].state = REVERSE_STATE_UNKNOWN;
		}
	      reverse_frame_valid |= 1 << i;
	      dest_regs[i] = operands[0];
	      break;
	    case TILEGX_OPC_OR:
	      if (reverse_frame[operands[1]].state == REVERSE_STATE_VALUE
		  && reverse_frame[operands[1]].value == 0)
		{
		  /* This is a move.  */
		  new_reverse_frame[i] = reverse_frame[operands[2]];
		}
	      else if (reverse_frame[operands[2]].state == REVERSE_STATE_VALUE
		       && reverse_frame[operands[2]].value == 0)
		{
		  /* This is a move.  */
		  new_reverse_frame[i] = reverse_frame[operands[1]];
		}
	      else
		{
		  /* We don't know anything about the values.  Punt.  */
		  new_reverse_frame[i].state = REVERSE_STATE_UNKNOWN;
		}
	      reverse_frame_valid |= 1 << i;
	      dest_regs[i] = operands[0];
	      break;
	    case TILEGX_OPC_SUB:
	      if (reverse_frame[operands[1]].state == REVERSE_STATE_VALUE
		  && reverse_frame[operands[2]].state == REVERSE_STATE_VALUE)
		{
		  /* We have values -- we can do this.  */
		  new_reverse_frame[i] = reverse_frame[operands[1]];
		  new_reverse_frame[i].value
		    -= reverse_frame[operands[2]].value;
		}
	      else
		{
		  /* We don't know anything about the values.  Punt.  */
		  new_reverse_frame[i].state = REVERSE_STATE_UNKNOWN;
		}
	      reverse_frame_valid |= 1 << i;
	      dest_regs[i] = operands[0];
	      break;

	    case TILEGX_OPC_FNOP:
	    case TILEGX_OPC_INFO:
	    case TILEGX_OPC_INFOL:
	      /* Nothing to see here, move on.
		 Note that real NOP is treated as a 'real' instruction
		 because someone must have intended that it be there.
		 It therefore terminates the prolog.  */
	      break;

	    case TILEGX_OPC_J:
	    case TILEGX_OPC_JAL:

	    case TILEGX_OPC_BEQZ:
	    case TILEGX_OPC_BEQZT:
	    case TILEGX_OPC_BGEZ:
	    case TILEGX_OPC_BGEZT:
	    case TILEGX_OPC_BGTZ:
	    case TILEGX_OPC_BGTZT:
	    case TILEGX_OPC_BLBC:
	    case TILEGX_OPC_BLBCT:
	    case TILEGX_OPC_BLBS:
	    case TILEGX_OPC_BLBST:
	    case TILEGX_OPC_BLEZ:
	    case TILEGX_OPC_BLEZT:
	    case TILEGX_OPC_BLTZ:
	    case TILEGX_OPC_BLTZT:
	    case TILEGX_OPC_BNEZ:
	    case TILEGX_OPC_BNEZT:

	    case TILEGX_OPC_IRET:
	    case TILEGX_OPC_JALR:
	    case TILEGX_OPC_JALRP:
	    case TILEGX_OPC_JR:
	    case TILEGX_OPC_JRP:
	    case TILEGX_OPC_SWINT0:
	    case TILEGX_OPC_SWINT1:
	    case TILEGX_OPC_SWINT2:
	    case TILEGX_OPC_SWINT3:
	      /* We're really done -- this is a branch.  */
	      branch_seen = 1;
	      prolog_done = 1;
	      break;
	    default:
	      /* We don't know or care what this instruction is.
		 All we know is that it isn't part of a prolog, and if
		 there's a destination register, we're trashing it.  */
	      prolog_done = 1;
	      for (j = 0; j < opcode->num_operands; j++)
		{
		  if (this_insn->operands[j]->is_dest_reg)
		    {
		      dest_regs[i] = operands[j];
		      new_reverse_frame[i].state = REVERSE_STATE_UNKNOWN;
		      reverse_frame_valid |= 1 << i;
		      break;
		    }
		}
	      break;
	    }
	}

      /* Now update the reverse frames.  */
      for (i = 0; i < num_insns; i++)
	{
	  /* ISSUE: Does this properly handle "network" registers?  */
	  if ((reverse_frame_valid & (1 << i))
	      && dest_regs[i] != TILEGX_ZERO_REGNUM)
	    reverse_frame[dest_regs[i]] = new_reverse_frame[i];
	}

      if (prev_sp_value != 0)
	{
	  /* GCC uses R52 as a frame pointer.  Have we seen "move r52, sp"?  */
	  if (reverse_frame[TILEGX_R52_REGNUM].state == REVERSE_STATE_REGISTER
	      && reverse_frame[TILEGX_R52_REGNUM].value == TILEGX_SP_REGNUM)
	  {
	    reverse_frame[TILEGX_R52_REGNUM].state = REVERSE_STATE_VALUE;
	    reverse_frame[TILEGX_R52_REGNUM].value = prev_sp_value;
	  }

	  prev_sp_value = 0;
	}

      if (prolog_done && prolog_end == end_addr)
	{
	  /* We found non-prolog code.	As such, _this_ instruction
	     is the one after the prolog.  We keep processing, because
	     there may be more prolog code in there, but this is what
	     we'll return.  */
	  /* ISSUE: There may not have actually been a prologue, and
	     we may have simply skipped some random instructions.  */
	  prolog_end = next_addr;
	}
      if (branch_seen)
	{
	  /* We saw a branch.  The prolog absolutely must be over.  */
	  break;
	}
    }

  if (prolog_end == end_addr && cache)
    {
      /* We may have terminated the prolog early, and we're certainly
	 at THIS point right now.  It's possible that the values of
	 registers we need are currently actually in other registers
	 (and haven't been written to memory yet).  Go find them.  */
      for (i = 0; i < TILEGX_NUM_PHYS_REGS; i++)
	{
	  if (reverse_frame[i].state == REVERSE_STATE_REGISTER
	      && reverse_frame[i].value != i)
	    {
	      unsigned saved_register = (unsigned) reverse_frame[i].value;

	      cache->saved_regs[saved_register].set_realreg (i);
	    }
	}
    }

  if (lr_saved_on_stack_p)
    {
      CORE_ADDR addr = cache->saved_regs[TILEGX_SP_REGNUM].addr ();
      cache->saved_regs[TILEGX_LR_REGNUM].set_addr (addr);
    }

  return prolog_end;
}

/* This is the implementation of gdbarch method skip_prologue.  */

static CORE_ADDR
tilegx_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  CORE_ADDR func_start, end_pc;
  struct obj_section *s;

  /* This is the preferred method, find the end of the prologue by
     using the debugging information.  */
  if (find_pc_partial_function (start_pc, NULL, &func_start, NULL))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_start);

      if (post_prologue_pc != 0)
	return std::max (start_pc, post_prologue_pc);
    }

  /* Don't straddle a section boundary.  */
  s = find_pc_section (start_pc);
  end_pc = start_pc + 8 * TILEGX_BUNDLE_SIZE_IN_BYTES;
  if (s != NULL)
    end_pc = std::min (end_pc, s->endaddr ());

  /* Otherwise, try to skip prologue the hard way.  */
  return tilegx_analyze_prologue (gdbarch,
				  start_pc,
				  end_pc,
				  NULL, NULL);
}

/* This is the implementation of gdbarch method stack_frame_destroyed_p.  */

static int
tilegx_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr = 0, func_end = 0;

  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      CORE_ADDR addr = func_end - TILEGX_BUNDLE_SIZE_IN_BYTES;

      /* FIXME: Find the actual epilogue.  */
      /* HACK: Just assume the final bundle is the "ret" instruction".  */
      if (pc > addr)
	return 1;
    }
  return 0;
}

/* This is the implementation of gdbarch method get_longjmp_target.  */

static int
tilegx_get_longjmp_target (frame_info_ptr frame, CORE_ADDR *pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR jb_addr;
  gdb_byte buf[8];

  jb_addr = get_frame_register_unsigned (frame, TILEGX_R0_REGNUM);

  /* TileGX jmp_buf contains 32 elements of type __uint_reg_t which
     has a size of 8 bytes.  The return address is stored in the 25th
     slot.  */
  if (target_read_memory (jb_addr + 25 * 8, buf, 8))
    return 0;

  *pc = extract_unsigned_integer (buf, 8, byte_order);

  return 1;
}

/* by assigning the 'faultnum' reg in kernel pt_regs with this value,
   kernel do_signal will not check r0. see tilegx kernel/signal.c
   for details.  */
#define INT_SWINT_1_SIGRETURN (~0)

/* Implement the "write_pc" gdbarch method.  */

static void
tilegx_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  regcache_cooked_write_unsigned (regcache, TILEGX_PC_REGNUM, pc);

  /* We must be careful with modifying the program counter.  If we
     just interrupted a system call, the kernel might try to restart
     it when we resume the inferior.  On restarting the system call,
     the kernel will try backing up the program counter even though it
     no longer points at the system call.  This typically results in a
     SIGSEGV or SIGILL.  We can prevent this by writing INT_SWINT_1_SIGRETURN
     in the "faultnum" pseudo-register.

     Note that "faultnum" is saved when setting up a dummy call frame.
     This means that it is properly restored when that frame is
     popped, and that the interrupted system call will be restarted
     when we resume the inferior on return from a function call from
     within GDB.  In all other cases the system call will not be
     restarted.  */
  regcache_cooked_write_unsigned (regcache, TILEGX_FAULTNUM_REGNUM,
				  INT_SWINT_1_SIGRETURN);
}

/* 64-bit pattern for a { bpt ; nop } bundle.  */
constexpr gdb_byte tilegx_break_insn[] =
  { 0x00, 0x50, 0x48, 0x51, 0xae, 0x44, 0x6a, 0x28 };

typedef BP_MANIPULATION (tilegx_break_insn) tilegx_breakpoint;

/* Normal frames.  */

static struct tilegx_frame_cache *
tilegx_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct tilegx_frame_cache *cache;
  CORE_ADDR current_pc;

  if (*this_cache)
    return (struct tilegx_frame_cache *) *this_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct tilegx_frame_cache);
  *this_cache = cache;
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);
  cache->base = 0;
  cache->start_pc = get_frame_func (this_frame);
  current_pc = get_frame_pc (this_frame);

  cache->base = get_frame_register_unsigned (this_frame, TILEGX_SP_REGNUM);
  cache->saved_regs[TILEGX_SP_REGNUM].set_value (cache->base);

  if (cache->start_pc)
    tilegx_analyze_prologue (gdbarch, cache->start_pc, current_pc,
			     cache, this_frame);

  cache->saved_regs[TILEGX_PC_REGNUM] = cache->saved_regs[TILEGX_LR_REGNUM];

  return cache;
}

/* Retrieve the value of REGNUM in FRAME.  */

static struct value*
tilegx_frame_prev_register (frame_info_ptr this_frame,
			    void **this_cache,
			    int regnum)
{
  struct tilegx_frame_cache *info =
    tilegx_frame_cache (this_frame, this_cache);

  return trad_frame_get_prev_register (this_frame, info->saved_regs,
				       regnum);
}

/* Build frame id.  */

static void
tilegx_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		      struct frame_id *this_id)
{
  struct tilegx_frame_cache *info =
    tilegx_frame_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (info->base == 0)
    return;

  (*this_id) = frame_id_build (info->base, info->start_pc);
}

static CORE_ADDR
tilegx_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct tilegx_frame_cache *cache =
    tilegx_frame_cache (this_frame, this_cache);

  return cache->base;
}

static const struct frame_unwind tilegx_frame_unwind = {
  "tilegx prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  tilegx_frame_this_id,
  tilegx_frame_prev_register,
  NULL,                        /* const struct frame_data *unwind_data  */
  default_frame_sniffer,       /* frame_sniffer_ftype *sniffer  */
  NULL                         /* frame_prev_pc_ftype *prev_pc  */
};

static const struct frame_base tilegx_frame_base = {
  &tilegx_frame_unwind,
  tilegx_frame_base_address,
  tilegx_frame_base_address,
  tilegx_frame_base_address
};

/* We cannot read/write the "special" registers.  */

static int
tilegx_cannot_reference_register (struct gdbarch *gdbarch, int regno)
{
  if (regno >= 0 && regno < TILEGX_NUM_EASY_REGS)
    return 0;
  else if (regno == TILEGX_PC_REGNUM
	   || regno == TILEGX_FAULTNUM_REGNUM)
    return 0;
  else
    return 1;
}

static struct gdbarch *
tilegx_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;
  int arch_size = 64;

  /* Handle arch_size == 32 or 64.  Default to 64.  */
  if (info.abfd)
    arch_size = bfd_get_arch_size (info.abfd);

  /* Try to find a pre-existing architecture.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      /* We only have two flavors -- just make sure arch_size matches.  */
      if (gdbarch_ptr_bit (arches->gdbarch) == arch_size)
	return (arches->gdbarch);
    }

  gdbarch = gdbarch_alloc (&info, NULL);

  /* Basic register fields and methods, datatype sizes and stuff.  */

  /* There are 64 physical registers which can be referenced by
     instructions (although only 56 of them can actually be
     debugged) and 1 magic register (the PC).  The other three
     magic registers (ex1, syscall, orig_r0) which are known to
     "ptrace" are ignored by "gdb".  Note that we simply pretend
     that there are 65 registers, and no "pseudo registers".  */
  set_gdbarch_num_regs (gdbarch, TILEGX_NUM_REGS);
  set_gdbarch_num_pseudo_regs (gdbarch, 0);

  set_gdbarch_sp_regnum (gdbarch, TILEGX_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, TILEGX_PC_REGNUM);

  set_gdbarch_register_name (gdbarch, tilegx_register_name);
  set_gdbarch_register_type (gdbarch, tilegx_register_type);

  set_gdbarch_short_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_int_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_bit (gdbarch, arch_size);
  set_gdbarch_long_long_bit (gdbarch, 8 * TARGET_CHAR_BIT);

  set_gdbarch_float_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_double_bit (gdbarch, 8 * TARGET_CHAR_BIT);
  set_gdbarch_long_double_bit (gdbarch, 8 * TARGET_CHAR_BIT);

  set_gdbarch_ptr_bit (gdbarch, arch_size);
  set_gdbarch_addr_bit (gdbarch, arch_size);

  set_gdbarch_cannot_fetch_register (gdbarch,
				     tilegx_cannot_reference_register);
  set_gdbarch_cannot_store_register (gdbarch,
				     tilegx_cannot_reference_register);

  /* Stack grows down.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  /* Frame Info.  */
  set_gdbarch_frame_align (gdbarch, tilegx_frame_align);
  frame_base_set_default (gdbarch, &tilegx_frame_base);

  set_gdbarch_skip_prologue (gdbarch, tilegx_skip_prologue);

  set_gdbarch_stack_frame_destroyed_p (gdbarch, tilegx_stack_frame_destroyed_p);

  /* Map debug registers into internal register numbers.  */
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, tilegx_dwarf2_reg_to_regnum);

  /* These values and methods are used when gdb calls a target function.  */
  set_gdbarch_push_dummy_call (gdbarch, tilegx_push_dummy_call);
  set_gdbarch_get_longjmp_target (gdbarch, tilegx_get_longjmp_target);
  set_gdbarch_write_pc (gdbarch, tilegx_write_pc);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       tilegx_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       tilegx_breakpoint::bp_from_kind);
  set_gdbarch_return_value (gdbarch, tilegx_return_value);

  gdbarch_init_osabi (info, gdbarch);

  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &tilegx_frame_unwind);

  return gdbarch;
}

void _initialize_tilegx_tdep ();
void
_initialize_tilegx_tdep ()
{
  gdbarch_register (bfd_arch_tilegx, tilegx_gdbarch_init);
}
