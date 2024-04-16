/* Target-dependent code for the Renesas RX for GDB, the GNU debugger.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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
#include "opcode/rx.h"
#include "dis-asm.h"
#include "gdbtypes.h"
#include "frame.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "value.h"
#include "gdbcore.h"
#include "dwarf2/frame.h"
#include "remote.h"
#include "target-descriptions.h"
#include "gdbarch.h"
#include "inferior.h"

#include "elf/rx.h"
#include "elf-bfd.h"
#include <algorithm>

#include "features/rx.c"

/* Certain important register numbers.  */
enum
{
  RX_SP_REGNUM = 0,
  RX_R1_REGNUM = 1,
  RX_R4_REGNUM = 4,
  RX_FP_REGNUM = 6,
  RX_R15_REGNUM = 15,
  RX_USP_REGNUM = 16,
  RX_PSW_REGNUM = 18,
  RX_PC_REGNUM = 19,
  RX_BPSW_REGNUM = 21,
  RX_BPC_REGNUM = 22,
  RX_FPSW_REGNUM = 24,
  RX_ACC_REGNUM = 25,
  RX_NUM_REGS = 26
};

/* RX frame types.  */
enum rx_frame_type {
  RX_FRAME_TYPE_NORMAL,
  RX_FRAME_TYPE_EXCEPTION,
  RX_FRAME_TYPE_FAST_INTERRUPT
};

/* Architecture specific data.  */
struct rx_gdbarch_tdep : gdbarch_tdep_base
{
  /* The ELF header flags specify the multilib used.  */
  int elf_flags = 0;

  /* Type of PSW and BPSW.  */
  struct type *rx_psw_type = nullptr;

  /* Type of FPSW.  */
  struct type *rx_fpsw_type = nullptr;
};

/* This structure holds the results of a prologue analysis.  */
struct rx_prologue
{
  /* Frame type, either a normal frame or one of two types of exception
     frames.  */
  enum rx_frame_type frame_type;

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
  int reg_offset[RX_NUM_REGS];
};

/* RX register names */
static const char *const rx_register_names[] = {
  "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
  "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
  "usp", "isp", "psw", "pc",  "intb", "bpsw","bpc","fintv",
  "fpsw", "acc",
};


/* Function for finding saved registers in a 'struct pv_area'; this
   function is passed to pv_area::scan.

   If VALUE is a saved register, ADDR says it was saved at a constant
   offset from the frame base, and SIZE indicates that the whole
   register was saved, record its offset.  */
static void
check_for_saved (void *result_untyped, pv_t addr, CORE_ADDR size, pv_t value)
{
  struct rx_prologue *result = (struct rx_prologue *) result_untyped;

  if (value.kind == pvk_register
      && value.k == 0
      && pv_is_register (addr, RX_SP_REGNUM)
      && size == register_size (current_inferior ()->arch (), value.reg))
    result->reg_offset[value.reg] = addr.k;
}

/* Define a "handle" struct for fetching the next opcode.  */
struct rx_get_opcode_byte_handle
{
  CORE_ADDR pc;
};

/* Fetch a byte on behalf of the opcode decoder.  HANDLE contains
   the memory address of the next byte to fetch.  If successful,
   the address in the handle is updated and the byte fetched is
   returned as the value of the function.  If not successful, -1
   is returned.  */
static int
rx_get_opcode_byte (void *handle)
{
  struct rx_get_opcode_byte_handle *opcdata
    = (struct rx_get_opcode_byte_handle *) handle;
  int status;
  gdb_byte byte;

  status = target_read_code (opcdata->pc, &byte, 1);
  if (status == 0)
    {
      opcdata->pc += 1;
      return byte;
    }
  else
    return -1;
}

/* Analyze a prologue starting at START_PC, going no further than
   LIMIT_PC.  Fill in RESULT as appropriate.  */

static void
rx_analyze_prologue (CORE_ADDR start_pc, CORE_ADDR limit_pc,
		     enum rx_frame_type frame_type,
		     struct rx_prologue *result)
{
  CORE_ADDR pc, next_pc;
  int rn;
  pv_t reg[RX_NUM_REGS];
  CORE_ADDR after_last_frame_setup_insn = start_pc;

  memset (result, 0, sizeof (*result));

  result->frame_type = frame_type;

  for (rn = 0; rn < RX_NUM_REGS; rn++)
    {
      reg[rn] = pv_register (rn, 0);
      result->reg_offset[rn] = 1;
    }

  pv_area stack (RX_SP_REGNUM, gdbarch_addr_bit (current_inferior ()->arch ()));

  if (frame_type == RX_FRAME_TYPE_FAST_INTERRUPT)
    {
      /* This code won't do anything useful at present, but this is
	 what happens for fast interrupts.  */
      reg[RX_BPSW_REGNUM] = reg[RX_PSW_REGNUM];
      reg[RX_BPC_REGNUM] = reg[RX_PC_REGNUM];
    }
  else
    {
      /* When an exception occurs, the PSW is saved to the interrupt stack
	 first.  */
      if (frame_type == RX_FRAME_TYPE_EXCEPTION)
	{
	  reg[RX_SP_REGNUM] = pv_add_constant (reg[RX_SP_REGNUM], -4);
	  stack.store (reg[RX_SP_REGNUM], 4, reg[RX_PSW_REGNUM]);
	}

      /* The call instruction (or an exception/interrupt) has saved the return
	  address on the stack.  */
      reg[RX_SP_REGNUM] = pv_add_constant (reg[RX_SP_REGNUM], -4);
      stack.store (reg[RX_SP_REGNUM], 4, reg[RX_PC_REGNUM]);

    }


  pc = start_pc;
  while (pc < limit_pc)
    {
      int bytes_read;
      struct rx_get_opcode_byte_handle opcode_handle;
      RX_Opcode_Decoded opc;

      opcode_handle.pc = pc;
      bytes_read = rx_decode_opcode (pc, &opc, rx_get_opcode_byte,
				     &opcode_handle);
      next_pc = pc + bytes_read;

      if (opc.id == RXO_pushm	/* pushm r1, r2 */
	  && opc.op[1].type == RX_Operand_Register
	  && opc.op[2].type == RX_Operand_Register)
	{
	  int r1, r2;
	  int r;

	  r1 = opc.op[1].reg;
	  r2 = opc.op[2].reg;
	  for (r = r2; r >= r1; r--)
	    {
	      reg[RX_SP_REGNUM] = pv_add_constant (reg[RX_SP_REGNUM], -4);
	      stack.store (reg[RX_SP_REGNUM], 4, reg[r]);
	    }
	  after_last_frame_setup_insn = next_pc;
	}
      else if (opc.id == RXO_mov	/* mov.l rdst, rsrc */
	       && opc.op[0].type == RX_Operand_Register
	       && opc.op[1].type == RX_Operand_Register
	       && opc.size == RX_Long)
	{
	  int rdst, rsrc;

	  rdst = opc.op[0].reg;
	  rsrc = opc.op[1].reg;
	  reg[rdst] = reg[rsrc];
	  if (rdst == RX_FP_REGNUM && rsrc == RX_SP_REGNUM)
	    after_last_frame_setup_insn = next_pc;
	}
      else if (opc.id == RXO_mov	/* mov.l rsrc, [-SP] */
	       && opc.op[0].type == RX_Operand_Predec
	       && opc.op[0].reg == RX_SP_REGNUM
	       && opc.op[1].type == RX_Operand_Register
	       && opc.size == RX_Long)
	{
	  int rsrc;

	  rsrc = opc.op[1].reg;
	  reg[RX_SP_REGNUM] = pv_add_constant (reg[RX_SP_REGNUM], -4);
	  stack.store (reg[RX_SP_REGNUM], 4, reg[rsrc]);
	  after_last_frame_setup_insn = next_pc;
	}
      else if (opc.id == RXO_add	/* add #const, rsrc, rdst */
	       && opc.op[0].type == RX_Operand_Register
	       && opc.op[1].type == RX_Operand_Immediate
	       && opc.op[2].type == RX_Operand_Register)
	{
	  int rdst = opc.op[0].reg;
	  int addend = opc.op[1].addend;
	  int rsrc = opc.op[2].reg;
	  reg[rdst] = pv_add_constant (reg[rsrc], addend);
	  /* Negative adjustments to the stack pointer or frame pointer
	     are (most likely) part of the prologue.  */
	  if ((rdst == RX_SP_REGNUM || rdst == RX_FP_REGNUM) && addend < 0)
	    after_last_frame_setup_insn = next_pc;
	}
      else if (opc.id == RXO_mov
	       && opc.op[0].type == RX_Operand_Indirect
	       && opc.op[1].type == RX_Operand_Register
	       && opc.size == RX_Long
	       && (opc.op[0].reg == RX_SP_REGNUM
		   || opc.op[0].reg == RX_FP_REGNUM)
	       && (RX_R1_REGNUM <= opc.op[1].reg
		   && opc.op[1].reg <= RX_R4_REGNUM))
	{
	  /* This moves an argument register to the stack.  Don't
	     record it, but allow it to be a part of the prologue.  */
	}
      else if (opc.id == RXO_branch
	       && opc.op[0].type == RX_Operand_Immediate
	       && next_pc < opc.op[0].addend)
	{
	  /* When a loop appears as the first statement of a function
	     body, gcc 4.x will use a BRA instruction to branch to the
	     loop condition checking code.  This BRA instruction is
	     marked as part of the prologue.  We therefore set next_pc
	     to this branch target and also stop the prologue scan.
	     The instructions at and beyond the branch target should
	     no longer be associated with the prologue.

	     Note that we only consider forward branches here.  We
	     presume that a forward branch is being used to skip over
	     a loop body.

	     A backwards branch is covered by the default case below.
	     If we were to encounter a backwards branch, that would
	     most likely mean that we've scanned through a loop body.
	     We definitely want to stop the prologue scan when this
	     happens and that is precisely what is done by the default
	     case below.  */

	  after_last_frame_setup_insn = opc.op[0].addend;
	  break;		/* Scan no further if we hit this case.  */
	}
      else
	{
	  /* Terminate the prologue scan.  */
	  break;
	}

      pc = next_pc;
    }

  /* Is the frame size (offset, really) a known constant?  */
  if (pv_is_register (reg[RX_SP_REGNUM], RX_SP_REGNUM))
    result->frame_size = reg[RX_SP_REGNUM].k;

  /* Was the frame pointer initialized?  */
  if (pv_is_register (reg[RX_FP_REGNUM], RX_SP_REGNUM))
    {
      result->has_frame_ptr = 1;
      result->frame_ptr_offset = reg[RX_FP_REGNUM].k;
    }

  /* Record where all the registers were saved.  */
  stack.scan (check_for_saved, (void *) result);

  result->prologue_end = after_last_frame_setup_insn;
}


/* Implement the "skip_prologue" gdbarch method.  */
static CORE_ADDR
rx_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  const char *name;
  CORE_ADDR func_addr, func_end;
  struct rx_prologue p;

  /* Try to find the extent of the function that contains PC.  */
  if (!find_pc_partial_function (pc, &name, &func_addr, &func_end))
    return pc;

  /* The frame type doesn't matter here, since we only care about
     where the prologue ends.  We'll use RX_FRAME_TYPE_NORMAL.  */
  rx_analyze_prologue (pc, func_end, RX_FRAME_TYPE_NORMAL, &p);
  return p.prologue_end;
}

/* Given a frame described by THIS_FRAME, decode the prologue of its
   associated function if there is not cache entry as specified by
   THIS_PROLOGUE_CACHE.  Save the decoded prologue in the cache and
   return that struct as the value of this function.  */

static struct rx_prologue *
rx_analyze_frame_prologue (frame_info_ptr this_frame,
			   enum rx_frame_type frame_type,
			   void **this_prologue_cache)
{
  if (!*this_prologue_cache)
    {
      CORE_ADDR func_start, stop_addr;

      *this_prologue_cache = FRAME_OBSTACK_ZALLOC (struct rx_prologue);

      func_start = get_frame_func (this_frame);
      stop_addr = get_frame_pc (this_frame);

      /* If we couldn't find any function containing the PC, then
	 just initialize the prologue cache, but don't do anything.  */
      if (!func_start)
	stop_addr = func_start;

      rx_analyze_prologue (func_start, stop_addr, frame_type,
			   (struct rx_prologue *) *this_prologue_cache);
    }

  return (struct rx_prologue *) *this_prologue_cache;
}

/* Determine type of frame by scanning the function for a return
   instruction.  */

static enum rx_frame_type
rx_frame_type (frame_info_ptr this_frame, void **this_cache)
{
  const char *name;
  CORE_ADDR pc, start_pc, lim_pc;
  int bytes_read;
  struct rx_get_opcode_byte_handle opcode_handle;
  RX_Opcode_Decoded opc;

  gdb_assert (this_cache != NULL);

  /* If we have a cached value, return it.  */

  if (*this_cache != NULL)
    {
      struct rx_prologue *p = (struct rx_prologue *) *this_cache;

      return p->frame_type;
    }

  /* No cached value; scan the function.  The frame type is cached in
     rx_analyze_prologue / rx_analyze_frame_prologue.  */
  
  pc = get_frame_pc (this_frame);
  
  /* Attempt to find the last address in the function.  If it cannot
     be determined, set the limit to be a short ways past the frame's
     pc.  */
  if (!find_pc_partial_function (pc, &name, &start_pc, &lim_pc))
    lim_pc = pc + 20;

  while (pc < lim_pc)
    {
      opcode_handle.pc = pc;
      bytes_read = rx_decode_opcode (pc, &opc, rx_get_opcode_byte,
				     &opcode_handle);

      if (bytes_read <= 0 || opc.id == RXO_rts)
	return RX_FRAME_TYPE_NORMAL;
      else if (opc.id == RXO_rtfi)
	return RX_FRAME_TYPE_FAST_INTERRUPT;
      else if (opc.id == RXO_rte)
	return RX_FRAME_TYPE_EXCEPTION;

      pc += bytes_read;
    }

  return RX_FRAME_TYPE_NORMAL;
}


/* Given the next frame and a prologue cache, return this frame's
   base.  */

static CORE_ADDR
rx_frame_base (frame_info_ptr this_frame, void **this_cache)
{
  enum rx_frame_type frame_type = rx_frame_type (this_frame, this_cache);
  struct rx_prologue *p
    = rx_analyze_frame_prologue (this_frame, frame_type, this_cache);

  /* In functions that use alloca, the distance between the stack
     pointer and the frame base varies dynamically, so we can't use
     the SP plus static information like prologue analysis to find the
     frame base.  However, such functions must have a frame pointer,
     to be able to restore the SP on exit.  So whenever we do have a
     frame pointer, use that to find the base.  */
  if (p->has_frame_ptr)
    {
      CORE_ADDR fp = get_frame_register_unsigned (this_frame, RX_FP_REGNUM);
      return fp - p->frame_ptr_offset;
    }
  else
    {
      CORE_ADDR sp = get_frame_register_unsigned (this_frame, RX_SP_REGNUM);
      return sp - p->frame_size;
    }
}

/* Implement the "frame_this_id" method for unwinding frames.  */

static void
rx_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		  struct frame_id *this_id)
{
  *this_id = frame_id_build (rx_frame_base (this_frame, this_cache),
			     get_frame_func (this_frame));
}

/* Implement the "frame_prev_register" method for unwinding frames.  */

static struct value *
rx_frame_prev_register (frame_info_ptr this_frame, void **this_cache,
			int regnum)
{
  enum rx_frame_type frame_type = rx_frame_type (this_frame, this_cache);
  struct rx_prologue *p
    = rx_analyze_frame_prologue (this_frame, frame_type, this_cache);
  CORE_ADDR frame_base = rx_frame_base (this_frame, this_cache);

  if (regnum == RX_SP_REGNUM)
    {
      if (frame_type == RX_FRAME_TYPE_EXCEPTION)
	{
	  struct value *psw_val;
	  CORE_ADDR psw;

	  psw_val = rx_frame_prev_register (this_frame, this_cache,
					    RX_PSW_REGNUM);
	  psw = extract_unsigned_integer
	    (psw_val->contents_all ().data (), 4,
	     gdbarch_byte_order (get_frame_arch (this_frame)));

	  if ((psw & 0x20000 /* U bit */) != 0)
	    return rx_frame_prev_register (this_frame, this_cache,
					   RX_USP_REGNUM);

	  /* Fall through for the case where U bit is zero.  */
	}

      return frame_unwind_got_constant (this_frame, regnum, frame_base);
    }

  if (frame_type == RX_FRAME_TYPE_FAST_INTERRUPT)
    {
      if (regnum == RX_PC_REGNUM)
	return rx_frame_prev_register (this_frame, this_cache,
				       RX_BPC_REGNUM);
      if (regnum == RX_PSW_REGNUM)
	return rx_frame_prev_register (this_frame, this_cache,
				       RX_BPSW_REGNUM);
    }

  /* If prologue analysis says we saved this register somewhere,
     return a description of the stack slot holding it.  */
  if (p->reg_offset[regnum] != 1)
    return frame_unwind_got_memory (this_frame, regnum,
				    frame_base + p->reg_offset[regnum]);

  /* Otherwise, presume we haven't changed the value of this
     register, and get it from the next frame.  */
  return frame_unwind_got_register (this_frame, regnum, regnum);
}

/* Return TRUE if the frame indicated by FRAME_TYPE is a normal frame.  */

static int
normal_frame_p (enum rx_frame_type frame_type)
{
  return (frame_type == RX_FRAME_TYPE_NORMAL);
}

/* Return TRUE if the frame indicated by FRAME_TYPE is an exception
   frame.  */

static int
exception_frame_p (enum rx_frame_type frame_type)
{
  return (frame_type == RX_FRAME_TYPE_EXCEPTION
	  || frame_type == RX_FRAME_TYPE_FAST_INTERRUPT);
}

/* Common code used by both normal and exception frame sniffers.  */

static int
rx_frame_sniffer_common (const struct frame_unwind *self,
			 frame_info_ptr this_frame,
			 void **this_cache,
			 int (*sniff_p)(enum rx_frame_type) )
{
  gdb_assert (this_cache != NULL);

  if (*this_cache == NULL)
    {
      enum rx_frame_type frame_type = rx_frame_type (this_frame, this_cache);

      if (sniff_p (frame_type))
	{
	  /* The call below will fill in the cache, including the frame
	     type.  */
	  (void) rx_analyze_frame_prologue (this_frame, frame_type, this_cache);

	  return 1;
	}
      else
	return 0;
    }
  else
    {
      struct rx_prologue *p = (struct rx_prologue *) *this_cache;

      return sniff_p (p->frame_type);
    }
}

/* Frame sniffer for normal (non-exception) frames.  */

static int
rx_frame_sniffer (const struct frame_unwind *self,
		  frame_info_ptr this_frame,
		  void **this_cache)
{
  return rx_frame_sniffer_common (self, this_frame, this_cache,
				  normal_frame_p);
}

/* Frame sniffer for exception frames.  */

static int
rx_exception_sniffer (const struct frame_unwind *self,
			     frame_info_ptr this_frame,
			     void **this_cache)
{
  return rx_frame_sniffer_common (self, this_frame, this_cache,
				  exception_frame_p);
}

/* Data structure for normal code using instruction-based prologue
   analyzer.  */

static const struct frame_unwind rx_frame_unwind = {
  "rx prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  rx_frame_this_id,
  rx_frame_prev_register,
  NULL,
  rx_frame_sniffer
};

/* Data structure for exception code using instruction-based prologue
   analyzer.  */

static const struct frame_unwind rx_exception_unwind = {
  "rx exception",
  /* SIGTRAMP_FRAME could be used here, but backtraces are less informative.  */
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  rx_frame_this_id,
  rx_frame_prev_register,
  NULL,
  rx_exception_sniffer
};

/* Implement the "push_dummy_call" gdbarch method.  */
static CORE_ADDR
rx_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		    struct regcache *regcache, CORE_ADDR bp_addr, int nargs,
		    struct value **args, CORE_ADDR sp,
		    function_call_return_method return_method,
		    CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int write_pass;
  int sp_off = 0;
  CORE_ADDR cfa;
  int num_register_candidate_args;

  struct type *func_type = function->type ();

  /* Dereference function pointer types.  */
  while (func_type->code () == TYPE_CODE_PTR)
    func_type = func_type->target_type ();

  /* The end result had better be a function or a method.  */
  gdb_assert (func_type->code () == TYPE_CODE_FUNC
	      || func_type->code () == TYPE_CODE_METHOD);

  /* Functions with a variable number of arguments have all of their
     variable arguments and the last non-variable argument passed
     on the stack.

     Otherwise, we can pass up to four arguments on the stack.

     Once computed, we leave this value alone.  I.e. we don't update
     it in case of a struct return going in a register or an argument
     requiring multiple registers, etc.  We rely instead on the value
     of the ``arg_reg'' variable to get these other details correct.  */

  if (func_type->has_varargs ())
    num_register_candidate_args = func_type->num_fields () - 1;
  else
    num_register_candidate_args = 4;

  /* We make two passes; the first does the stack allocation,
     the second actually stores the arguments.  */
  for (write_pass = 0; write_pass <= 1; write_pass++)
    {
      int i;
      int arg_reg = RX_R1_REGNUM;

      if (write_pass)
	sp = align_down (sp - sp_off, 4);
      sp_off = 0;

      if (return_method == return_method_struct)
	{
	  struct type *return_type = func_type->target_type ();

	  gdb_assert (return_type->code () == TYPE_CODE_STRUCT
		      || func_type->code () == TYPE_CODE_UNION);

	  if (return_type->length () > 16
	      || return_type->length () % 4 != 0)
	    {
	      if (write_pass)
		regcache_cooked_write_unsigned (regcache, RX_R15_REGNUM,
						struct_addr);
	    }
	}

      /* Push the arguments.  */
      for (i = 0; i < nargs; i++)
	{
	  struct value *arg = args[i];
	  const gdb_byte *arg_bits = arg->contents_all ().data ();
	  struct type *arg_type = check_typedef (arg->type ());
	  ULONGEST arg_size = arg_type->length ();

	  if (i == 0 && struct_addr != 0
	      && return_method != return_method_struct
	      && arg_type->code () == TYPE_CODE_PTR
	      && extract_unsigned_integer (arg_bits, 4,
					   byte_order) == struct_addr)
	    {
	      /* This argument represents the address at which C++ (and
		 possibly other languages) store their return value.
		 Put this value in R15.  */
	      if (write_pass)
		regcache_cooked_write_unsigned (regcache, RX_R15_REGNUM,
						struct_addr);
	    }
	  else if (arg_type->code () != TYPE_CODE_STRUCT
		   && arg_type->code () != TYPE_CODE_UNION
		   && arg_size <= 8)
	    {
	      /* Argument is a scalar.  */
	      if (arg_size == 8)
		{
		  if (i < num_register_candidate_args
		      && arg_reg <= RX_R4_REGNUM - 1)
		    {
		      /* If argument registers are going to be used to pass
			 an 8 byte scalar, the ABI specifies that two registers
			 must be available.  */
		      if (write_pass)
			{
			  regcache_cooked_write_unsigned (regcache, arg_reg,
							  extract_unsigned_integer
							  (arg_bits, 4,
							   byte_order));
			  regcache_cooked_write_unsigned (regcache,
							  arg_reg + 1,
							  extract_unsigned_integer
							  (arg_bits + 4, 4,
							   byte_order));
			}
		      arg_reg += 2;
		    }
		  else
		    {
		      sp_off = align_up (sp_off, 4);
		      /* Otherwise, pass the 8 byte scalar on the stack.  */
		      if (write_pass)
			write_memory (sp + sp_off, arg_bits, 8);
		      sp_off += 8;
		    }
		}
	      else
		{
		  ULONGEST u;

		  gdb_assert (arg_size <= 4);

		  u =
		    extract_unsigned_integer (arg_bits, arg_size, byte_order);

		  if (i < num_register_candidate_args
		      && arg_reg <= RX_R4_REGNUM)
		    {
		      if (write_pass)
			regcache_cooked_write_unsigned (regcache, arg_reg, u);
		      arg_reg += 1;
		    }
		  else
		    {
		      int p_arg_size = 4;

		      if (func_type->is_prototyped ()
			  && i < func_type->num_fields ())
			{
			  struct type *p_arg_type =
			    func_type->field (i).type ();
			  p_arg_size = p_arg_type->length ();
			}

		      sp_off = align_up (sp_off, p_arg_size);

		      if (write_pass)
			write_memory_unsigned_integer (sp + sp_off,
						       p_arg_size, byte_order,
						       u);
		      sp_off += p_arg_size;
		    }
		}
	    }
	  else
	    {
	      /* Argument is a struct or union.  Pass as much of the struct
		 in registers, if possible.  Pass the rest on the stack.  */
	      while (arg_size > 0)
		{
		  if (i < num_register_candidate_args
		      && arg_reg <= RX_R4_REGNUM
		      && arg_size <= 4 * (RX_R4_REGNUM - arg_reg + 1)
		      && arg_size % 4 == 0)
		    {
		      int len = std::min (arg_size, (ULONGEST) 4);

		      if (write_pass)
			regcache_cooked_write_unsigned (regcache, arg_reg,
							extract_unsigned_integer
							(arg_bits, len,
							 byte_order));
		      arg_bits += len;
		      arg_size -= len;
		      arg_reg++;
		    }
		  else
		    {
		      sp_off = align_up (sp_off, 4);
		      if (write_pass)
			write_memory (sp + sp_off, arg_bits, arg_size);
		      sp_off += align_up (arg_size, 4);
		      arg_size = 0;
		    }
		}
	    }
	}
    }

  /* Keep track of the stack address prior to pushing the return address.
     This is the value that we'll return.  */
  cfa = sp;

  /* Push the return address.  */
  sp = sp - 4;
  write_memory_unsigned_integer (sp, 4, byte_order, bp_addr);

  /* Update the stack pointer.  */
  regcache_cooked_write_unsigned (regcache, RX_SP_REGNUM, sp);

  return cfa;
}

/* Implement the "return_value" gdbarch method.  */
static enum return_value_convention
rx_return_value (struct gdbarch *gdbarch,
		 struct value *function,
		 struct type *valtype,
		 struct regcache *regcache,
		 gdb_byte *readbuf, const gdb_byte *writebuf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST valtype_len = valtype->length ();

  if (valtype->length () > 16
      || ((valtype->code () == TYPE_CODE_STRUCT
	   || valtype->code () == TYPE_CODE_UNION)
	  && valtype->length () % 4 != 0))
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf)
    {
      ULONGEST u;
      int argreg = RX_R1_REGNUM;
      int offset = 0;

      while (valtype_len > 0)
	{
	  int len = std::min (valtype_len, (ULONGEST) 4);

	  regcache_cooked_read_unsigned (regcache, argreg, &u);
	  store_unsigned_integer (readbuf + offset, len, byte_order, u);
	  valtype_len -= len;
	  offset += len;
	  argreg++;
	}
    }

  if (writebuf)
    {
      ULONGEST u;
      int argreg = RX_R1_REGNUM;
      int offset = 0;

      while (valtype_len > 0)
	{
	  int len = std::min (valtype_len, (ULONGEST) 4);

	  u = extract_unsigned_integer (writebuf + offset, len, byte_order);
	  regcache_cooked_write_unsigned (regcache, argreg, u);
	  valtype_len -= len;
	  offset += len;
	  argreg++;
	}
    }

  return RETURN_VALUE_REGISTER_CONVENTION;
}

constexpr gdb_byte rx_break_insn[] = { 0x00 };

typedef BP_MANIPULATION (rx_break_insn) rx_breakpoint;

/* Implement the dwarf_reg_to_regnum" gdbarch method.  */

static int
rx_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  if (0 <= reg && reg <= 15)
    return reg;
  else if (reg == 16)
    return RX_PSW_REGNUM;
  else if (reg == 17)
    return RX_PC_REGNUM;
  else
    return -1;
}

/* Allocate and initialize a gdbarch object.  */
static struct gdbarch *
rx_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  int elf_flags;
  tdesc_arch_data_up tdesc_data;
  const struct target_desc *tdesc = info.target_desc;

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
      rx_gdbarch_tdep *tdep
	= gdbarch_tdep<rx_gdbarch_tdep> (arches->gdbarch);

      if (tdep->elf_flags != elf_flags)
	continue;

      return arches->gdbarch;
    }

  if (tdesc == NULL)
      tdesc = tdesc_rx;

  /* Check any target description for validity.  */
  if (tdesc_has_registers (tdesc))
    {
      const struct tdesc_feature *feature;
      bool valid_p = true;

      feature = tdesc_find_feature (tdesc, "org.gnu.gdb.rx.core");

      if (feature != NULL)
	{
	  tdesc_data = tdesc_data_alloc ();
	  for (int i = 0; i < RX_NUM_REGS; i++)
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (), i,
						rx_register_names[i]);
	}

      if (!valid_p)
	return NULL;
    }

  gdb_assert(tdesc_data != NULL);

  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new rx_gdbarch_tdep));
  rx_gdbarch_tdep *tdep = gdbarch_tdep<rx_gdbarch_tdep> (gdbarch);

  tdep->elf_flags = elf_flags;

  set_gdbarch_num_regs (gdbarch, RX_NUM_REGS);
  tdesc_use_registers (gdbarch, tdesc, std::move (tdesc_data));

  set_gdbarch_num_pseudo_regs (gdbarch, 0);
  set_gdbarch_pc_regnum (gdbarch, RX_PC_REGNUM);
  set_gdbarch_sp_regnum (gdbarch, RX_SP_REGNUM);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_decr_pc_after_break (gdbarch, 1);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, rx_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, rx_breakpoint::bp_from_kind);
  set_gdbarch_skip_prologue (gdbarch, rx_skip_prologue);

  /* Target builtin data types.  */
  set_gdbarch_char_signed (gdbarch, 0);
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_ptr_bit (gdbarch, 32);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);

  if (elf_flags & E_FLAG_RX_64BIT_DOUBLES)
    {
      set_gdbarch_double_bit (gdbarch, 64);
      set_gdbarch_long_double_bit (gdbarch, 64);
      set_gdbarch_double_format (gdbarch, floatformats_ieee_double);
      set_gdbarch_long_double_format (gdbarch, floatformats_ieee_double);
    }
  else
    {
      set_gdbarch_double_bit (gdbarch, 32);
      set_gdbarch_long_double_bit (gdbarch, 32);
      set_gdbarch_double_format (gdbarch, floatformats_ieee_single);
      set_gdbarch_long_double_format (gdbarch, floatformats_ieee_single);
    }

  /* DWARF register mapping.  */
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, rx_dwarf_reg_to_regnum);

  /* Frame unwinding.  */
  frame_unwind_append_unwinder (gdbarch, &rx_exception_unwind);
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &rx_frame_unwind);

  /* Methods setting up a dummy call, and extracting the return value from
     a call.  */
  set_gdbarch_push_dummy_call (gdbarch, rx_push_dummy_call);
  set_gdbarch_return_value (gdbarch, rx_return_value);

  /* Virtual tables.  */
  set_gdbarch_vbit_in_delta (gdbarch, 1);

  return gdbarch;
}

/* Register the above initialization routine.  */

void _initialize_rx_tdep ();
void
_initialize_rx_tdep ()
{
  gdbarch_register (bfd_arch_rx, rx_gdbarch_init);
  initialize_tdesc_rx ();
}
