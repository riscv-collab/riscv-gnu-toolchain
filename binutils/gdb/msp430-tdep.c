/* Target-dependent code for the Texas Instruments MSP430 for GDB, the
   GNU debugger.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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

#include "elf/msp430.h"
#include "opcode/msp430-decode.h"
#include "elf-bfd.h"

/* Register Numbers.  */

enum
{
  MSP430_PC_RAW_REGNUM,
  MSP430_SP_RAW_REGNUM,
  MSP430_SR_RAW_REGNUM,
  MSP430_CG_RAW_REGNUM,
  MSP430_R4_RAW_REGNUM,
  MSP430_R5_RAW_REGNUM,
  MSP430_R6_RAW_REGNUM,
  MSP430_R7_RAW_REGNUM,
  MSP430_R8_RAW_REGNUM,
  MSP430_R9_RAW_REGNUM,
  MSP430_R10_RAW_REGNUM,
  MSP430_R11_RAW_REGNUM,
  MSP430_R12_RAW_REGNUM,
  MSP430_R13_RAW_REGNUM,
  MSP430_R14_RAW_REGNUM,
  MSP430_R15_RAW_REGNUM,

  MSP430_NUM_REGS,

  MSP430_PC_REGNUM = MSP430_NUM_REGS,
  MSP430_SP_REGNUM,
  MSP430_SR_REGNUM,
  MSP430_CG_REGNUM,
  MSP430_R4_REGNUM,
  MSP430_R5_REGNUM,
  MSP430_R6_REGNUM,
  MSP430_R7_REGNUM,
  MSP430_R8_REGNUM,
  MSP430_R9_REGNUM,
  MSP430_R10_REGNUM,
  MSP430_R11_REGNUM,
  MSP430_R12_REGNUM,
  MSP430_R13_REGNUM,
  MSP430_R14_REGNUM,
  MSP430_R15_REGNUM,

  MSP430_NUM_TOTAL_REGS,
  MSP430_NUM_PSEUDO_REGS = MSP430_NUM_TOTAL_REGS - MSP430_NUM_REGS
};

enum
{
  /* TI MSP430 Architecture.  */
  MSP_ISA_MSP430,

  /* TI MSP430X Architecture.  */
  MSP_ISA_MSP430X
};

enum
{
  /* The small code model limits code addresses to 16 bits.  */
  MSP_SMALL_CODE_MODEL,

  /* The large code model uses 20 bit addresses for function
     pointers.  These are stored in memory using four bytes (32 bits).  */
  MSP_LARGE_CODE_MODEL
};

/* Architecture specific data.  */

struct msp430_gdbarch_tdep : gdbarch_tdep_base
{
  /* The ELF header flags specify the multilib used.  */
  int elf_flags = 0;

  /* One of MSP_ISA_MSP430 or MSP_ISA_MSP430X.  */
  int isa = 0;

  /* One of MSP_SMALL_CODE_MODEL or MSP_LARGE_CODE_MODEL.  If, at
     some point, we support different data models too, we'll probably
     structure things so that we can combine values using logical
     "or".  */
  int code_model = 0;
};

/* This structure holds the results of a prologue analysis.  */

struct msp430_prologue
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
  int reg_offset[MSP430_NUM_TOTAL_REGS];
};

/* Implement the "register_type" gdbarch method.  */

static struct type *
msp430_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  if (reg_nr < MSP430_NUM_REGS)
    return builtin_type (gdbarch)->builtin_uint32;
  else if (reg_nr == MSP430_PC_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;
  else
    return builtin_type (gdbarch)->builtin_uint16;
}

/* Implement another version of the "register_type" gdbarch method
   for msp430x.  */

static struct type *
msp430x_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  if (reg_nr < MSP430_NUM_REGS)
    return builtin_type (gdbarch)->builtin_uint32;
  else if (reg_nr == MSP430_PC_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;
  else
    return builtin_type (gdbarch)->builtin_uint32;
}

/* Implement the "register_name" gdbarch method.  */

static const char *
msp430_register_name (struct gdbarch *gdbarch, int regnr)
{
  static const char *const reg_names[] = {
    /* Raw registers.  */
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    /* Pseudo registers.  */
    "pc", "sp", "sr", "cg", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
  };

  static_assert (ARRAY_SIZE (reg_names) == (MSP430_NUM_REGS
						+ MSP430_NUM_PSEUDO_REGS));
  return reg_names[regnr];
}

/* Implement the "register_reggroup_p" gdbarch method.  */

static int
msp430_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			    const struct reggroup *group)
{
  if (group == all_reggroup)
    return 1;

  /* All other registers are saved and restored.  */
  if (group == save_reggroup || group == restore_reggroup)
    return (MSP430_NUM_REGS <= regnum && regnum < MSP430_NUM_TOTAL_REGS);

  return group == general_reggroup;
}

/* Implement the "pseudo_register_read" gdbarch method.  */

static enum register_status
msp430_pseudo_register_read (struct gdbarch *gdbarch,
			     readable_regcache *regcache,
			     int regnum, gdb_byte *buffer)
{
  if (MSP430_NUM_REGS <= regnum && regnum < MSP430_NUM_TOTAL_REGS)
    {
      enum register_status status;
      ULONGEST val;
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      int regsize = register_size (gdbarch, regnum);
      int raw_regnum = regnum - MSP430_NUM_REGS;

      status = regcache->raw_read (raw_regnum, &val);
      if (status == REG_VALID)
	store_unsigned_integer (buffer, regsize, byte_order, val);

      return status;
    }
  else
    gdb_assert_not_reached ("invalid pseudo register number");
}

/* Implement the "pseudo_register_write" gdbarch method.  */

static void
msp430_pseudo_register_write (struct gdbarch *gdbarch,
			      struct regcache *regcache,
			      int regnum, const gdb_byte *buffer)
{
  if (MSP430_NUM_REGS <= regnum && regnum < MSP430_NUM_TOTAL_REGS)

    {
      ULONGEST val;
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      int regsize = register_size (gdbarch, regnum);
      int raw_regnum = regnum - MSP430_NUM_REGS;

      val = extract_unsigned_integer (buffer, regsize, byte_order);
      regcache_raw_write_unsigned (regcache, raw_regnum, val);

    }
  else
    gdb_assert_not_reached ("invalid pseudo register number");
}

/* Implement the `register_sim_regno' gdbarch method.  */

static int
msp430_register_sim_regno (struct gdbarch *gdbarch, int regnum)
{
  gdb_assert (regnum < MSP430_NUM_REGS);

  /* So long as regnum is in [0, RL78_NUM_REGS), it's valid.  We
     just want to override the default here which disallows register
     numbers which have no names.  */
  return regnum;
}

constexpr gdb_byte msp430_break_insn[] = { 0x43, 0x43 };

typedef BP_MANIPULATION (msp430_break_insn) msp430_breakpoint;

/* Define a "handle" struct for fetching the next opcode.  */

struct msp430_get_opcode_byte_handle
{
  CORE_ADDR pc;
};

/* Fetch a byte on behalf of the opcode decoder.  HANDLE contains
   the memory address of the next byte to fetch.  If successful,
   the address in the handle is updated and the byte fetched is
   returned as the value of the function.  If not successful, -1
   is returned.  */

static int
msp430_get_opcode_byte (void *handle)
{
  struct msp430_get_opcode_byte_handle *opcdata
    = (struct msp430_get_opcode_byte_handle *) handle;
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
check_for_saved (void *result_untyped, pv_t addr, CORE_ADDR size, pv_t value)
{
  struct msp430_prologue *result = (struct msp430_prologue *) result_untyped;

  if (value.kind == pvk_register
      && value.k == 0
      && pv_is_register (addr, MSP430_SP_REGNUM)
      && size == register_size (current_inferior ()->arch  (), value.reg))
    result->reg_offset[value.reg] = addr.k;
}

/* Analyze a prologue starting at START_PC, going no further than
   LIMIT_PC.  Fill in RESULT as appropriate.  */

static void
msp430_analyze_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc,
			 CORE_ADDR limit_pc, struct msp430_prologue *result)
{
  CORE_ADDR pc, next_pc;
  int rn;
  pv_t reg[MSP430_NUM_TOTAL_REGS];
  CORE_ADDR after_last_frame_setup_insn = start_pc;
  msp430_gdbarch_tdep *tdep = gdbarch_tdep<msp430_gdbarch_tdep> (gdbarch);
  int code_model = tdep->code_model;
  int sz;

  memset (result, 0, sizeof (*result));

  for (rn = 0; rn < MSP430_NUM_TOTAL_REGS; rn++)
    {
      reg[rn] = pv_register (rn, 0);
      result->reg_offset[rn] = 1;
    }

  pv_area stack (MSP430_SP_REGNUM, gdbarch_addr_bit (gdbarch));

  /* The call instruction has saved the return address on the stack.  */
  sz = code_model == MSP_LARGE_CODE_MODEL ? 4 : 2;
  reg[MSP430_SP_REGNUM] = pv_add_constant (reg[MSP430_SP_REGNUM], -sz);
  stack.store (reg[MSP430_SP_REGNUM], sz, reg[MSP430_PC_REGNUM]);

  pc = start_pc;
  while (pc < limit_pc)
    {
      int bytes_read;
      struct msp430_get_opcode_byte_handle opcode_handle;
      MSP430_Opcode_Decoded opc;

      opcode_handle.pc = pc;
      bytes_read = msp430_decode_opcode (pc, &opc, msp430_get_opcode_byte,
					 &opcode_handle);
      next_pc = pc + bytes_read;

      if (opc.id == MSO_push && opc.op[0].type == MSP430_Operand_Register)
	{
	  int rsrc = opc.op[0].reg;

	  reg[MSP430_SP_REGNUM] = pv_add_constant (reg[MSP430_SP_REGNUM], -2);
	  stack.store (reg[MSP430_SP_REGNUM], 2, reg[rsrc]);
	  after_last_frame_setup_insn = next_pc;
	}
      else if (opc.id == MSO_push	/* PUSHM  */
	       && opc.op[0].type == MSP430_Operand_None
	       && opc.op[1].type == MSP430_Operand_Register)
	{
	  int rsrc = opc.op[1].reg;
	  int count = opc.repeats + 1;
	  int size = opc.size == 16 ? 2 : 4;

	  while (count > 0)
	    {
	      reg[MSP430_SP_REGNUM]
		= pv_add_constant (reg[MSP430_SP_REGNUM], -size);
	      stack.store (reg[MSP430_SP_REGNUM], size, reg[rsrc]);
	      rsrc--;
	      count--;
	    }
	  after_last_frame_setup_insn = next_pc;
	}
      else if (opc.id == MSO_sub
	       && opc.op[0].type == MSP430_Operand_Register
	       && opc.op[0].reg == MSR_SP
	       && opc.op[1].type == MSP430_Operand_Immediate)
	{
	  int addend = opc.op[1].addend;

	  reg[MSP430_SP_REGNUM] = pv_add_constant (reg[MSP430_SP_REGNUM],
						   -addend);
	  after_last_frame_setup_insn = next_pc;
	}
      else if (opc.id == MSO_mov
	       && opc.op[0].type == MSP430_Operand_Immediate
	       && 12 <= opc.op[0].reg && opc.op[0].reg <= 15)
	after_last_frame_setup_insn = next_pc;
      else
	{
	  /* Terminate the prologue scan.  */
	  break;
	}

      pc = next_pc;
    }

  /* Is the frame size (offset, really) a known constant?  */
  if (pv_is_register (reg[MSP430_SP_REGNUM], MSP430_SP_REGNUM))
    result->frame_size = reg[MSP430_SP_REGNUM].k;

  /* Record where all the registers were saved.  */
  stack.scan (check_for_saved, result);

  result->prologue_end = after_last_frame_setup_insn;
}

/* Implement the "skip_prologue" gdbarch method.  */

static CORE_ADDR
msp430_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  const char *name;
  CORE_ADDR func_addr, func_end;
  struct msp430_prologue p;

  /* Try to find the extent of the function that contains PC.  */
  if (!find_pc_partial_function (pc, &name, &func_addr, &func_end))
    return pc;

  msp430_analyze_prologue (gdbarch, pc, func_end, &p);
  return p.prologue_end;
}

/* Given a frame described by THIS_FRAME, decode the prologue of its
   associated function if there is not cache entry as specified by
   THIS_PROLOGUE_CACHE.  Save the decoded prologue in the cache and
   return that struct as the value of this function.  */

static struct msp430_prologue *
msp430_analyze_frame_prologue (frame_info_ptr this_frame,
			       void **this_prologue_cache)
{
  if (!*this_prologue_cache)
    {
      CORE_ADDR func_start, stop_addr;

      *this_prologue_cache = FRAME_OBSTACK_ZALLOC (struct msp430_prologue);

      func_start = get_frame_func (this_frame);
      stop_addr = get_frame_pc (this_frame);

      /* If we couldn't find any function containing the PC, then
	 just initialize the prologue cache, but don't do anything.  */
      if (!func_start)
	stop_addr = func_start;

      msp430_analyze_prologue (get_frame_arch (this_frame), func_start,
			       stop_addr,
			       (struct msp430_prologue *) *this_prologue_cache);
    }

  return (struct msp430_prologue *) *this_prologue_cache;
}

/* Given a frame and a prologue cache, return this frame's base.  */

static CORE_ADDR
msp430_frame_base (frame_info_ptr this_frame, void **this_prologue_cache)
{
  struct msp430_prologue *p
    = msp430_analyze_frame_prologue (this_frame, this_prologue_cache);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, MSP430_SP_REGNUM);

  return sp - p->frame_size;
}

/* Implement the "frame_this_id" method for unwinding frames.  */

static void
msp430_this_id (frame_info_ptr this_frame,
		void **this_prologue_cache, struct frame_id *this_id)
{
  *this_id = frame_id_build (msp430_frame_base (this_frame,
						this_prologue_cache),
			     get_frame_func (this_frame));
}

/* Implement the "frame_prev_register" method for unwinding frames.  */

static struct value *
msp430_prev_register (frame_info_ptr this_frame,
		      void **this_prologue_cache, int regnum)
{
  struct msp430_prologue *p
    = msp430_analyze_frame_prologue (this_frame, this_prologue_cache);
  CORE_ADDR frame_base = msp430_frame_base (this_frame, this_prologue_cache);

  if (regnum == MSP430_SP_REGNUM)
    return frame_unwind_got_constant (this_frame, regnum, frame_base);

  /* If prologue analysis says we saved this register somewhere,
     return a description of the stack slot holding it.  */
  else if (p->reg_offset[regnum] != 1)
    {
      struct value *rv = frame_unwind_got_memory (this_frame, regnum,
						  frame_base +
						  p->reg_offset[regnum]);

      if (regnum == MSP430_PC_REGNUM)
	{
	  ULONGEST pc = value_as_long (rv);

	  return frame_unwind_got_constant (this_frame, regnum, pc);
	}
      return rv;
    }

  /* Otherwise, presume we haven't changed the value of this
     register, and get it from the next frame.  */
  else
    return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind msp430_unwind = {
  "msp430 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  msp430_this_id,
  msp430_prev_register,
  NULL,
  default_frame_sniffer
};

/* Implement the "dwarf2_reg_to_regnum" gdbarch method.  */

static int
msp430_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  if (reg >= 0 && reg < MSP430_NUM_REGS)
    return reg + MSP430_NUM_REGS;
  return -1;
}

/* Implement the "return_value" gdbarch method.  */

static enum return_value_convention
msp430_return_value (struct gdbarch *gdbarch,
		     struct value *function,
		     struct type *valtype,
		     struct regcache *regcache,
		     gdb_byte *readbuf, const gdb_byte *writebuf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  LONGEST valtype_len = valtype->length ();
  msp430_gdbarch_tdep *tdep = gdbarch_tdep<msp430_gdbarch_tdep> (gdbarch);
  int code_model = tdep->code_model;

  if (valtype->length () > 8
      || valtype->code () == TYPE_CODE_STRUCT
      || valtype->code () == TYPE_CODE_UNION)
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf)
    {
      ULONGEST u;
      int argreg = MSP430_R12_REGNUM;
      int offset = 0;

      while (valtype_len > 0)
	{
	  int size = 2;

	  if (code_model == MSP_LARGE_CODE_MODEL
	      && valtype->code () == TYPE_CODE_PTR)
	    {
	      size = 4;
	    }

	  regcache_cooked_read_unsigned (regcache, argreg, &u);
	  store_unsigned_integer (readbuf + offset, size, byte_order, u);
	  valtype_len -= size;
	  offset += size;
	  argreg++;
	}
    }

  if (writebuf)
    {
      ULONGEST u;
      int argreg = MSP430_R12_REGNUM;
      int offset = 0;

      while (valtype_len > 0)
	{
	  int size = 2;

	  if (code_model == MSP_LARGE_CODE_MODEL
	      && valtype->code () == TYPE_CODE_PTR)
	    {
	      size = 4;
	    }

	  u = extract_unsigned_integer (writebuf + offset, size, byte_order);
	  regcache_cooked_write_unsigned (regcache, argreg, u);
	  valtype_len -= size;
	  offset += size;
	  argreg++;
	}
    }

  return RETURN_VALUE_REGISTER_CONVENTION;
}


/* Implement the "frame_align" gdbarch method.  */

static CORE_ADDR
msp430_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  return align_down (sp, 2);
}

/* Implement the "push_dummy_call" gdbarch method.  */

static CORE_ADDR
msp430_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			struct regcache *regcache, CORE_ADDR bp_addr,
			int nargs, struct value **args, CORE_ADDR sp,
			function_call_return_method return_method,
			CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int write_pass;
  int sp_off = 0;
  CORE_ADDR cfa;
  msp430_gdbarch_tdep *tdep = gdbarch_tdep<msp430_gdbarch_tdep> (gdbarch);
  int code_model = tdep->code_model;

  struct type *func_type = function->type ();

  /* Dereference function pointer types.  */
  while (func_type->code () == TYPE_CODE_PTR)
    func_type = func_type->target_type ();

  /* The end result had better be a function or a method.  */
  gdb_assert (func_type->code () == TYPE_CODE_FUNC
	      || func_type->code () == TYPE_CODE_METHOD);

  /* We make two passes; the first does the stack allocation,
     the second actually stores the arguments.  */
  for (write_pass = 0; write_pass <= 1; write_pass++)
    {
      int i;
      int arg_reg = MSP430_R12_REGNUM;
      int args_on_stack = 0;

      if (write_pass)
	sp = align_down (sp - sp_off, 4);
      sp_off = 0;

      if (return_method == return_method_struct)
	{
	  if (write_pass)
	    regcache_cooked_write_unsigned (regcache, arg_reg, struct_addr);
	  arg_reg++;
	}

      /* Push the arguments.  */
      for (i = 0; i < nargs; i++)
	{
	  struct value *arg = args[i];
	  const gdb_byte *arg_bits = arg->contents_all ().data ();
	  struct type *arg_type = check_typedef (arg->type ());
	  ULONGEST arg_size = arg_type->length ();
	  int offset;
	  int current_arg_on_stack;
	  gdb_byte struct_addr_buf[4];

	  current_arg_on_stack = 0;

	  if (arg_type->code () == TYPE_CODE_STRUCT
	      || arg_type->code () == TYPE_CODE_UNION)
	    {
	      /* Aggregates of any size are passed by reference.  */
	      store_unsigned_integer (struct_addr_buf, 4, byte_order,
				      arg->address ());
	      arg_bits = struct_addr_buf;
	      arg_size = (code_model == MSP_LARGE_CODE_MODEL) ? 4 : 2;
	    }
	  else
	    {
	      /* Scalars bigger than 8 bytes such as complex doubles are passed
		 on the stack.  */
	      if (arg_size > 8)
		current_arg_on_stack = 1;
	    }


	  for (offset = 0; offset < arg_size; offset += 2)
	    {
	      /* The condition below prevents 8 byte scalars from being split
		 between registers and memory (stack).  It also prevents other
		 splits once the stack has been written to.  */
	      if (!current_arg_on_stack
		  && (arg_reg
		      + ((arg_size == 8 || args_on_stack)
			 ? ((arg_size - offset) / 2 - 1)
			 : 0) <= MSP430_R15_REGNUM))
		{
		  int size = 2;

		  if (code_model == MSP_LARGE_CODE_MODEL
		      && (arg_type->code () == TYPE_CODE_PTR
			  || TYPE_IS_REFERENCE (arg_type)
			  || arg_type->code () == TYPE_CODE_STRUCT
			  || arg_type->code () == TYPE_CODE_UNION))
		    {
		      /* When using the large memory model, pointer,
			 reference, struct, and union arguments are
			 passed using the entire register.  (As noted
			 earlier, aggregates are always passed by
			 reference.) */
		      if (offset != 0)
			continue;
		      size = 4;
		    }

		  if (write_pass)
		    regcache_cooked_write_unsigned (regcache, arg_reg,
						    extract_unsigned_integer
						    (arg_bits + offset, size,
						     byte_order));

		  arg_reg++;
		}
	      else
		{
		  if (write_pass)
		    write_memory (sp + sp_off, arg_bits + offset, 2);

		  sp_off += 2;
		  args_on_stack = 1;
		  current_arg_on_stack = 1;
		}
	    }
	}
    }

  /* Keep track of the stack address prior to pushing the return address.
     This is the value that we'll return.  */
  cfa = sp;

  /* Push the return address.  */
  {
    int sz = tdep->code_model == MSP_SMALL_CODE_MODEL ? 2 : 4;
    sp = sp - sz;
    write_memory_unsigned_integer (sp, sz, byte_order, bp_addr);
  }

  /* Update the stack pointer.  */
  regcache_cooked_write_unsigned (regcache, MSP430_SP_REGNUM, sp);

  return cfa;
}

/* In order to keep code size small, the compiler may create epilogue
   code through which more than one function epilogue is routed.  I.e.
   the epilogue and return may just be a branch to some common piece of
   code which is responsible for tearing down the frame and performing
   the return.  These epilog (label) names will have the common prefix
   defined here.  */

static const char msp430_epilog_name_prefix[] = "__mspabi_func_epilog_";

/* Implement the "in_return_stub" gdbarch method.  */

static int
msp430_in_return_stub (struct gdbarch *gdbarch, CORE_ADDR pc,
		       const char *name)
{
  return (name != NULL
	  && startswith (name, msp430_epilog_name_prefix));
}

/* Implement the "skip_trampoline_code" gdbarch method.  */
static CORE_ADDR
msp430_skip_trampoline_code (frame_info_ptr frame, CORE_ADDR pc)
{
  struct bound_minimal_symbol bms;
  const char *stub_name;
  struct gdbarch *gdbarch = get_frame_arch (frame);

  bms = lookup_minimal_symbol_by_pc (pc);
  if (!bms.minsym)
    return pc;

  stub_name = bms.minsym->linkage_name ();

  msp430_gdbarch_tdep *tdep = gdbarch_tdep<msp430_gdbarch_tdep> (gdbarch);
  if (tdep->code_model == MSP_SMALL_CODE_MODEL
      && msp430_in_return_stub (gdbarch, pc, stub_name))
    {
      CORE_ADDR sp = get_frame_register_unsigned (frame, MSP430_SP_REGNUM);

      return read_memory_integer
	(sp + 2 * (stub_name[strlen (msp430_epilog_name_prefix)] - '0'),
	 2, gdbarch_byte_order (gdbarch));
    }

  return pc;
}

/* Allocate and initialize a gdbarch object.  */

static struct gdbarch *
msp430_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  int elf_flags, isa, code_model;

  /* Extract the elf_flags if available.  */
  if (info.abfd != NULL
      && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    elf_flags = elf_elfheader (info.abfd)->e_flags;
  else
    elf_flags = 0;

  if (info.abfd != NULL)
    switch (bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_PROC,
				      OFBA_MSPABI_Tag_ISA))
      {
      case 1:
	isa = MSP_ISA_MSP430;
	code_model = MSP_SMALL_CODE_MODEL;
	break;
      case 2:
	isa = MSP_ISA_MSP430X;
	switch (bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_PROC,
					  OFBA_MSPABI_Tag_Code_Model))
	  {
	  case 1:
	    code_model = MSP_SMALL_CODE_MODEL;
	    break;
	  case 2:
	    code_model = MSP_LARGE_CODE_MODEL;
	    break;
	  default:
	    internal_error (_("Unknown msp430x code memory model"));
	    break;
	  }
	break;
      case 0:
	/* This can happen when loading a previously dumped data structure.
	   Use the ISA and code model from the current architecture, provided
	   it's compatible.  */
	{
	  struct gdbarch *ca = get_current_arch ();
	  if (ca && gdbarch_bfd_arch_info (ca)->arch == bfd_arch_msp430)
	    {
	      msp430_gdbarch_tdep *ca_tdep
		= gdbarch_tdep<msp430_gdbarch_tdep> (ca);

	      elf_flags = ca_tdep->elf_flags;
	      isa = ca_tdep->isa;
	      code_model = ca_tdep->code_model;
	      break;
	    }
	}
	[[fallthrough]];
      default:
	error (_("Unknown msp430 isa"));
	break;
      }
  else
    {
      isa = MSP_ISA_MSP430;
      code_model = MSP_SMALL_CODE_MODEL;
    }


  /* Try to find the architecture in the list of already defined
     architectures.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      msp430_gdbarch_tdep *candidate_tdep
	= gdbarch_tdep<msp430_gdbarch_tdep> (arches->gdbarch);

      if (candidate_tdep->elf_flags != elf_flags
	  || candidate_tdep->isa != isa
	  || candidate_tdep->code_model != code_model)
	continue;

      return arches->gdbarch;
    }

  /* None found, create a new architecture from the information
     provided.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new msp430_gdbarch_tdep));
  msp430_gdbarch_tdep *tdep = gdbarch_tdep<msp430_gdbarch_tdep> (gdbarch);

  tdep->elf_flags = elf_flags;
  tdep->isa = isa;
  tdep->code_model = code_model;

  /* Registers.  */
  set_gdbarch_num_regs (gdbarch, MSP430_NUM_REGS);
  set_gdbarch_num_pseudo_regs (gdbarch, MSP430_NUM_PSEUDO_REGS);
  set_gdbarch_register_name (gdbarch, msp430_register_name);
  if (isa == MSP_ISA_MSP430)
    set_gdbarch_register_type (gdbarch, msp430_register_type);
  else
    set_gdbarch_register_type (gdbarch, msp430x_register_type);
  set_gdbarch_pc_regnum (gdbarch, MSP430_PC_REGNUM);
  set_gdbarch_sp_regnum (gdbarch, MSP430_SP_REGNUM);
  set_gdbarch_register_reggroup_p (gdbarch, msp430_register_reggroup_p);
  set_gdbarch_pseudo_register_read (gdbarch, msp430_pseudo_register_read);
  set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						msp430_pseudo_register_write);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, msp430_dwarf2_reg_to_regnum);
  set_gdbarch_register_sim_regno (gdbarch, msp430_register_sim_regno);

  /* Data types.  */
  set_gdbarch_char_signed (gdbarch, 0);
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 16);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  if (code_model == MSP_SMALL_CODE_MODEL)
    {
      set_gdbarch_ptr_bit (gdbarch, 16);
      set_gdbarch_addr_bit (gdbarch, 16);
    }
  else				/* MSP_LARGE_CODE_MODEL */
    {
      set_gdbarch_ptr_bit (gdbarch, 32);
      set_gdbarch_addr_bit (gdbarch, 32);
    }
  set_gdbarch_dwarf2_addr_size (gdbarch, 4);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_double);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_double);

  /* Breakpoints.  */
  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       msp430_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       msp430_breakpoint::bp_from_kind);
  set_gdbarch_decr_pc_after_break (gdbarch, 1);

  /* Frames, prologues, etc.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_skip_prologue (gdbarch, msp430_skip_prologue);
  set_gdbarch_frame_align (gdbarch, msp430_frame_align);
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &msp430_unwind);

  /* Dummy frames, return values.  */
  set_gdbarch_push_dummy_call (gdbarch, msp430_push_dummy_call);
  set_gdbarch_return_value (gdbarch, msp430_return_value);

  /* Trampolines.  */
  set_gdbarch_in_solib_return_trampoline (gdbarch, msp430_in_return_stub);
  set_gdbarch_skip_trampoline_code (gdbarch, msp430_skip_trampoline_code);

  /* Virtual tables.  */
  set_gdbarch_vbit_in_delta (gdbarch, 0);

  return gdbarch;
}

/* Register the initialization routine.  */

void _initialize_msp430_tdep ();
void
_initialize_msp430_tdep ()
{
  gdbarch_register (bfd_arch_msp430, msp430_gdbarch_init);
}
