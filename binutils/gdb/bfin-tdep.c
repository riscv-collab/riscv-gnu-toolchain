/* Target-dependent code for Analog Devices Blackfin processor, for GDB.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.

   Contributed by Analog Devices, Inc.

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
#include "inferior.h"
#include "gdbcore.h"
#include "arch-utils.h"
#include "regcache.h"
#include "frame.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"
#include "dis-asm.h"
#include "sim-regno.h"
#include "sim/sim-bfin.h"
#include "dwarf2/frame.h"
#include "symtab.h"
#include "elf-bfd.h"
#include "elf/bfin.h"
#include "osabi.h"
#include "infcall.h"
#include "xml-syscall.h"
#include "bfin-tdep.h"

/* Macros used by prologue functions.  */
#define P_LINKAGE			0xE800
#define P_MINUS_SP1			0x0140
#define P_MINUS_SP2			0x05C0
#define P_MINUS_SP3			0x0540
#define P_MINUS_SP4			0x04C0
#define P_SP_PLUS			0x6C06
#define P_P2_LOW			0xE10A
#define P_P2_HIGH			0XE14A
#define P_SP_EQ_SP_PLUS_P2		0X5BB2
#define P_SP_EQ_P2_PLUS_SP		0x5B96
#define P_MINUS_MINUS_SP_EQ_RETS	0x0167

/* Macros used for program flow control.  */
/* 16 bit instruction, max  */
#define P_16_BIT_INSR_MAX		0xBFFF
/* 32 bit instruction, min  */
#define P_32_BIT_INSR_MIN		0xC000
/* 32 bit instruction, max  */
#define P_32_BIT_INSR_MAX		0xE801
/* jump (preg), 16-bit, min  */
#define P_JUMP_PREG_MIN			0x0050
/* jump (preg), 16-bit, max  */
#define P_JUMP_PREG_MAX			0x0057
/* jump (pc+preg), 16-bit, min  */
#define P_JUMP_PC_PLUS_PREG_MIN		0x0080
/* jump (pc+preg), 16-bit, max  */
#define P_JUMP_PC_PLUS_PREG_MAX		0x0087
/* jump.s pcrel13m2, 16-bit, min  */
#define P_JUMP_S_MIN			0x2000
/* jump.s pcrel13m2, 16-bit, max  */
#define P_JUMP_S_MAX			0x2FFF
/* jump.l pcrel25m2, 32-bit, min  */
#define P_JUMP_L_MIN			0xE200
/* jump.l pcrel25m2, 32-bit, max  */
#define P_JUMP_L_MAX			0xE2FF
/* conditional jump pcrel11m2, 16-bit, min  */
#define P_IF_CC_JUMP_MIN		0x1800
/* conditional jump pcrel11m2, 16-bit, max  */
#define P_IF_CC_JUMP_MAX		0x1BFF
/* conditional jump(bp) pcrel11m2, 16-bit, min  */
#define P_IF_CC_JUMP_BP_MIN		0x1C00
/* conditional jump(bp) pcrel11m2, 16-bit, max  */
#define P_IF_CC_JUMP_BP_MAX		0x1FFF
/* conditional !jump pcrel11m2, 16-bit, min  */
#define P_IF_NOT_CC_JUMP_MIN		0x1000
/* conditional !jump pcrel11m2, 16-bit, max  */
#define P_IF_NOT_CC_JUMP_MAX		0x13FF
/* conditional jump(bp) pcrel11m2, 16-bit, min  */
#define P_IF_NOT_CC_JUMP_BP_MIN		0x1400
/* conditional jump(bp) pcrel11m2, 16-bit, max  */
#define P_IF_NOT_CC_JUMP_BP_MAX		0x17FF
/* call (preg), 16-bit, min  */
#define P_CALL_PREG_MIN			0x0060
/* call (preg), 16-bit, max  */
#define P_CALL_PREG_MAX			0x0067
/* call (pc+preg), 16-bit, min  */
#define P_CALL_PC_PLUS_PREG_MIN		0x0070
/* call (pc+preg), 16-bit, max  */
#define P_CALL_PC_PLUS_PREG_MAX		0x0077
/* call pcrel25m2, 32-bit, min  */
#define P_CALL_MIN			0xE300
/* call pcrel25m2, 32-bit, max  */
#define P_CALL_MAX			0xE3FF
/* RTS  */
#define P_RTS				0x0010
/* MNOP  */
#define P_MNOP				0xC803
/* EXCPT, 16-bit, min  */
#define P_EXCPT_MIN			0x00A0
/* EXCPT, 16-bit, max  */
#define P_EXCPT_MAX			0x00AF
/* multi instruction mask 1, 16-bit  */
#define P_BIT_MULTI_INS_1		0xC000
/* multi instruction mask 2, 16-bit  */
#define P_BIT_MULTI_INS_2		0x0800

/* The maximum bytes we search to skip the prologue.  */
#define UPPER_LIMIT			40

/* ASTAT bits  */
#define ASTAT_CC_POS			5
#define ASTAT_CC			(1 << ASTAT_CC_POS)

/* Initial value: Register names used in BFIN's ISA documentation.  */

static const char * const bfin_register_name_strings[] =
{
  "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
  "p0", "p1", "p2", "p3", "p4", "p5", "sp", "fp",
  "i0", "i1", "i2", "i3", "m0", "m1", "m2", "m3",
  "b0", "b1", "b2", "b3", "l0", "l1", "l2", "l3",
  "a0x", "a0w", "a1x", "a1w", "astat", "rets",
  "lc0", "lt0", "lb0", "lc1", "lt1", "lb1", "cycles", "cycles2",
  "usp", "seqstat", "syscfg", "reti", "retx", "retn", "rete",
  "pc", "cc",
};

#define NUM_BFIN_REGNAMES ARRAY_SIZE (bfin_register_name_strings)


/* In this diagram successive memory locations increase downwards or the
   stack grows upwards with negative indices.  (PUSH analogy for stack.)

   The top frame is the "frame" of the current function being executed.

     +--------------+ SP    -
     |  local vars  |       ^
     +--------------+       |
     |  save regs   |       |
     +--------------+ FP    |
     |   old FP    -|--    top
     +--------------+  |  frame
     |    RETS      |  |    |
     +--------------+  |    |
     |   param 1    |  |    |
     |   param 2    |  |    |
     |    ...       |  |    V
     +--------------+  |    -
     |  local vars  |  |    ^
     +--------------+  |    |
     |  save regs   |  |    |
     +--------------+<-     |
     |   old FP    -|--   next
     +--------------+  |  frame
     |    RETS      |  |    |
     +--------------+  |    |
     |   param 1    |  |    |
     |   param 2    |  |    |
     |    ...       |  |    V
     +--------------+  |    -
     |  local vars  |  |    ^
     +--------------+  |    |
     |  save regs   |  |    |
     +--------------+<-  next frame
     |   old FP     |       |
     +--------------+       |
     |    RETS      |       V
     +--------------+       -

   The frame chain is formed as following:

     FP has the topmost frame.
     FP + 4 has the previous FP and so on.  */


/* Map from DWARF2 register number to GDB register number.  */

static const int map_gcc_gdb[] =
{
  BFIN_R0_REGNUM,
  BFIN_R1_REGNUM,
  BFIN_R2_REGNUM,
  BFIN_R3_REGNUM,
  BFIN_R4_REGNUM,
  BFIN_R5_REGNUM,
  BFIN_R6_REGNUM,
  BFIN_R7_REGNUM,
  BFIN_P0_REGNUM,
  BFIN_P1_REGNUM,
  BFIN_P2_REGNUM,
  BFIN_P3_REGNUM,
  BFIN_P4_REGNUM,
  BFIN_P5_REGNUM,
  BFIN_SP_REGNUM,
  BFIN_FP_REGNUM,
  BFIN_I0_REGNUM,
  BFIN_I1_REGNUM,
  BFIN_I2_REGNUM,
  BFIN_I3_REGNUM,
  BFIN_B0_REGNUM,
  BFIN_B1_REGNUM,
  BFIN_B2_REGNUM,
  BFIN_B3_REGNUM,
  BFIN_L0_REGNUM,
  BFIN_L1_REGNUM,
  BFIN_L2_REGNUM,
  BFIN_L3_REGNUM,
  BFIN_M0_REGNUM,
  BFIN_M1_REGNUM,
  BFIN_M2_REGNUM,
  BFIN_M3_REGNUM,
  BFIN_A0_DOT_X_REGNUM,
  BFIN_A1_DOT_X_REGNUM,
  BFIN_CC_REGNUM,
  BFIN_RETS_REGNUM,
  BFIN_RETI_REGNUM,
  BFIN_RETX_REGNUM,
  BFIN_RETN_REGNUM,
  BFIN_RETE_REGNUM,
  BFIN_ASTAT_REGNUM,
  BFIN_SEQSTAT_REGNUM,
  BFIN_USP_REGNUM,
  BFIN_LT0_REGNUM,
  BFIN_LT1_REGNUM,
  BFIN_LC0_REGNUM,
  BFIN_LC1_REGNUM,
  BFIN_LB0_REGNUM,
  BFIN_LB1_REGNUM
};

/* Big enough to hold the size of the largest register in bytes.  */
#define BFIN_MAX_REGISTER_SIZE	4

struct bfin_frame_cache
{
  /* Base address.  */
  CORE_ADDR base;
  CORE_ADDR sp_offset;
  CORE_ADDR pc;
  int frameless_pc_value;

  /* Saved registers.  */
  CORE_ADDR saved_regs[BFIN_NUM_REGS];
  CORE_ADDR saved_sp;

  /* Stack space reserved for local variables.  */
  long locals;
};

/* Allocate and initialize a frame cache.  */

static struct bfin_frame_cache *
bfin_alloc_frame_cache (void)
{
  struct bfin_frame_cache *cache;
  int i;

  cache = FRAME_OBSTACK_ZALLOC (struct bfin_frame_cache);

  /* Base address.  */
  cache->base = 0;
  cache->sp_offset = -4;
  cache->pc = 0;
  cache->frameless_pc_value = 0;

  /* Saved registers.  We initialize these to -1 since zero is a valid
     offset (that's where fp is supposed to be stored).  */
  for (i = 0; i < BFIN_NUM_REGS; i++)
    cache->saved_regs[i] = -1;

  /* Frameless until proven otherwise.  */
  cache->locals = -1;

  return cache;
}

static struct bfin_frame_cache *
bfin_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct bfin_frame_cache *cache;
  int i;

  if (*this_cache)
    return (struct bfin_frame_cache *) *this_cache;

  cache = bfin_alloc_frame_cache ();
  *this_cache = cache;

  cache->base = get_frame_register_unsigned (this_frame, BFIN_FP_REGNUM);
  if (cache->base == 0)
    return cache;

  /* For normal frames, PC is stored at [FP + 4].  */
  cache->saved_regs[BFIN_PC_REGNUM] = 4;
  cache->saved_regs[BFIN_FP_REGNUM] = 0;

  /* Adjust all the saved registers such that they contain addresses
     instead of offsets.  */
  for (i = 0; i < BFIN_NUM_REGS; i++)
    if (cache->saved_regs[i] != -1)
      cache->saved_regs[i] += cache->base;

  cache->pc = get_frame_func (this_frame) ;
  if (cache->pc == 0 || cache->pc == get_frame_pc (this_frame))
    {
      /* Either there is no prologue (frameless function) or we are at
	 the start of a function.  In short we do not have a frame.
	 PC is stored in rets register.  FP points to previous frame.  */

      cache->saved_regs[BFIN_PC_REGNUM] =
	get_frame_register_unsigned (this_frame, BFIN_RETS_REGNUM);
      cache->frameless_pc_value = 1;
      cache->base = get_frame_register_unsigned (this_frame, BFIN_FP_REGNUM);
      cache->saved_regs[BFIN_FP_REGNUM] = cache->base;
      cache->saved_sp = cache->base;
    }
  else
    {
      cache->frameless_pc_value = 0;

      /* Now that we have the base address for the stack frame we can
	 calculate the value of SP in the calling frame.  */
      cache->saved_sp = cache->base + 8;
    }

  return cache;
}

static void
bfin_frame_this_id (frame_info_ptr this_frame,
		    void **this_cache,
		    struct frame_id *this_id)
{
  struct bfin_frame_cache *cache = bfin_frame_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  /* See the end of bfin_push_dummy_call.  */
  *this_id = frame_id_build (cache->base + 8, cache->pc);
}

static struct value *
bfin_frame_prev_register (frame_info_ptr this_frame,
			  void **this_cache,
			  int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct bfin_frame_cache *cache = bfin_frame_cache (this_frame, this_cache);

  if (regnum == gdbarch_sp_regnum (gdbarch) && cache->saved_sp)
    return frame_unwind_got_constant (this_frame, regnum, cache->saved_sp);

  if (regnum < BFIN_NUM_REGS && cache->saved_regs[regnum] != -1)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->saved_regs[regnum]);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind bfin_frame_unwind =
{
  "bfin prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  bfin_frame_this_id,
  bfin_frame_prev_register,
  NULL,
  default_frame_sniffer
};

/* Check for "[--SP] = <reg>;" insns.  These are appear in function
   prologues to save misc registers onto the stack.  */

static int
is_minus_minus_sp (int op)
{
  op &= 0xFFC0;

  if ((op == P_MINUS_SP1) || (op == P_MINUS_SP2)
      || (op == P_MINUS_SP3) || (op == P_MINUS_SP4))
    return 1;

  return 0;
}

/* Skip all the insns that appear in generated function prologues.  */

static CORE_ADDR
bfin_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int op = read_memory_unsigned_integer (pc, 2, byte_order);
  CORE_ADDR orig_pc = pc;
  int done = 0;

  /* The new gcc prologue generates the register saves BEFORE the link
     or RETS saving instruction.
     So, our job is to stop either at those instructions or some upper
     limit saying there is no frame!  */

  while (!done)
    {
      if (is_minus_minus_sp (op))
	{
	  while (is_minus_minus_sp (op))
	    {
	      pc += 2;
	      op = read_memory_unsigned_integer (pc, 2, byte_order);
	    }

	  if (op == P_LINKAGE)
	    pc += 4;

	  done = 1;
	}
      else if (op == P_LINKAGE)
	{
	  pc += 4;
	  done = 1;
	}
      else if (op == P_MINUS_MINUS_SP_EQ_RETS)
	{
	  pc += 2;
	  done = 1;
	}
      else if (op == P_RTS)
	{
	  done = 1;
	}
      else if ((op >= P_JUMP_PREG_MIN && op <= P_JUMP_PREG_MAX)
	       || (op >= P_JUMP_PC_PLUS_PREG_MIN
		   && op <= P_JUMP_PC_PLUS_PREG_MAX)
	       || (op == P_JUMP_S_MIN && op <= P_JUMP_S_MAX))
	{
	  done = 1;
	}
      else if (pc - orig_pc >= UPPER_LIMIT)
	{
	  warning (_("Function Prologue not recognised; "
		     "pc will point to ENTRY_POINT of the function"));
	  pc = orig_pc + 2;
	  done = 1;
	}
      else
	{
	  pc += 2; /* Not a terminating instruction go on.  */
	  op = read_memory_unsigned_integer (pc, 2, byte_order);
	}
    }

   /* TODO:
      Dwarf2 uses entry point value AFTER some register initializations.
      We should perhaps skip such asssignments as well (R6 = R1, ...).  */

  return pc;
}

/* Return the GDB type object for the "standard" data type of data in
   register N.  This should be void pointer for P0-P5, SP, FP;
   void pointer to function for PC; int otherwise.  */

static struct type *
bfin_register_type (struct gdbarch *gdbarch, int regnum)
{
  if ((regnum >= BFIN_P0_REGNUM && regnum <= BFIN_FP_REGNUM)
      || regnum == BFIN_USP_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;

  if (regnum == BFIN_PC_REGNUM || regnum == BFIN_RETS_REGNUM
      || regnum == BFIN_RETI_REGNUM || regnum == BFIN_RETX_REGNUM
      || regnum == BFIN_RETN_REGNUM || regnum == BFIN_RETE_REGNUM
      || regnum == BFIN_LT0_REGNUM || regnum == BFIN_LB0_REGNUM
      || regnum == BFIN_LT1_REGNUM || regnum == BFIN_LB1_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;

  return builtin_type (gdbarch)->builtin_int32;
}

static CORE_ADDR
bfin_push_dummy_call (struct gdbarch *gdbarch,
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
  int i;
  long reg_r0, reg_r1, reg_r2;
  int total_len = 0;

  for (i = nargs - 1; i >= 0; i--)
    {
      struct type *value_type = args[i]->enclosing_type ();

      total_len += align_up (value_type->length (), 4);
    }

  /* At least twelve bytes of stack space must be allocated for the function's
     arguments, even for functions that have less than 12 bytes of argument
     data.  */

  if (total_len < 12)
    sp -= 12 - total_len;

  /* Push arguments in reverse order.  */

  for (i = nargs - 1; i >= 0; i--)
    {
      struct type *value_type = args[i]->enclosing_type ();
      struct type *arg_type = check_typedef (value_type);
      int container_len = align_up (arg_type->length (), 4);

      sp -= container_len;
      write_memory (sp, args[i]->contents ().data (), container_len);
    }

  /* Initialize R0, R1, and R2 to the first 3 words of parameters.  */

  reg_r0 = read_memory_integer (sp, 4, byte_order);
  regcache_cooked_write_unsigned (regcache, BFIN_R0_REGNUM, reg_r0);
  reg_r1 = read_memory_integer (sp + 4, 4, byte_order);
  regcache_cooked_write_unsigned (regcache, BFIN_R1_REGNUM, reg_r1);
  reg_r2 = read_memory_integer (sp + 8, 4, byte_order);
  regcache_cooked_write_unsigned (regcache, BFIN_R2_REGNUM, reg_r2);

  /* Store struct value address.  */

  if (return_method == return_method_struct)
    regcache_cooked_write_unsigned (regcache, BFIN_P0_REGNUM, struct_addr);

  /* Set the dummy return value to bp_addr.
     A dummy breakpoint will be setup to execute the call.  */

  regcache_cooked_write_unsigned (regcache, BFIN_RETS_REGNUM, bp_addr);

  /* Finally, update the stack pointer.  */

  regcache_cooked_write_unsigned (regcache, BFIN_SP_REGNUM, sp);

  return sp;
}

/* Convert DWARF2 register number REG to the appropriate register number
   used by GDB.  */

static int
bfin_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  if (reg < 0 || reg >= ARRAY_SIZE (map_gcc_gdb))
    return -1;

  return map_gcc_gdb[reg];
}

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
bfin_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned short iw;

  iw = read_memory_unsigned_integer (*pcptr, 2, byte_order);

  if ((iw & 0xf000) >= 0xc000)
    /* 32-bit instruction.  */
    return 4;
  else
    return 2;
}

/* Implement the sw_breakpoint_from_kind gdbarch method.  */

static const gdb_byte *
bfin_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  static unsigned char bfin_breakpoint[] = {0xa1, 0x00, 0x00, 0x00};
  static unsigned char bfin_sim_breakpoint[] = {0x25, 0x00, 0x00, 0x00};

  *size = kind;

  if (strcmp (target_shortname (), "sim") == 0)
    return bfin_sim_breakpoint;
  else
    return bfin_breakpoint;
}

static void
bfin_extract_return_value (struct type *type,
			   struct regcache *regs,
			   gdb_byte *dst)
{
  struct gdbarch *gdbarch = regs->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  bfd_byte *valbuf = dst;
  int len = type->length ();
  ULONGEST tmp;
  int regno = BFIN_R0_REGNUM;

  gdb_assert (len <= 8);

  while (len > 0)
    {
      regcache_cooked_read_unsigned (regs, regno++, &tmp);
      store_unsigned_integer (valbuf, (len > 4 ? 4 : len), byte_order, tmp);
      len -= 4;
      valbuf += 4;
    }
}

/* Write into appropriate registers a function return value of type
   TYPE, given in virtual format.  */

static void
bfin_store_return_value (struct type *type,
			 struct regcache *regs,
			 const gdb_byte *src)
{
  const bfd_byte *valbuf = src;

  /* Integral values greater than one word are stored in consecutive
     registers starting with R0.  This will always be a multiple of
     the register size.  */

  int len = type->length ();
  int regno = BFIN_R0_REGNUM;

  gdb_assert (len <= 8);

  while (len > 0)
    {
      regs->cooked_write (regno++, valbuf);
      len -= 4;
      valbuf += 4;
    }
}

/* Determine, for architecture GDBARCH, how a return value of TYPE
   should be returned.  If it is supposed to be returned in registers,
   and READBUF is nonzero, read the appropriate value from REGCACHE,
   and copy it into READBUF.  If WRITEBUF is nonzero, write the value
   from WRITEBUF into REGCACHE.  */

static enum return_value_convention
bfin_return_value (struct gdbarch *gdbarch,
		   struct value *function,
		   struct type *type,
		   struct regcache *regcache,
		   gdb_byte *readbuf,
		   const gdb_byte *writebuf)
{
  if (type->length () > 8)
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf)
    bfin_extract_return_value (type, regcache, readbuf);

  if (writebuf)
    bfin_store_return_value (type, regcache, writebuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}

/* Return the BFIN register name corresponding to register I.  */

static const char *
bfin_register_name (struct gdbarch *gdbarch, int i)
{
  return bfin_register_name_strings[i];
}

static enum register_status
bfin_pseudo_register_read (struct gdbarch *gdbarch, readable_regcache *regcache,
			   int regnum, gdb_byte *buffer)
{
  gdb_byte buf[BFIN_MAX_REGISTER_SIZE];
  enum register_status status;

  if (regnum != BFIN_CC_REGNUM)
    internal_error (_("invalid register number %d"), regnum);

  /* Extract the CC bit from the ASTAT register.  */
  status = regcache->raw_read (BFIN_ASTAT_REGNUM, buf);
  if (status == REG_VALID)
    {
      buffer[1] = buffer[2] = buffer[3] = 0;
      buffer[0] = !!(buf[0] & ASTAT_CC);
    }
  return status;
}

static void
bfin_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			    int regnum, const gdb_byte *buffer)
{
  gdb_byte buf[BFIN_MAX_REGISTER_SIZE];

  if (regnum != BFIN_CC_REGNUM)
    internal_error (_("invalid register number %d"), regnum);

  /* Overlay the CC bit in the ASTAT register.  */
  regcache->raw_read (BFIN_ASTAT_REGNUM, buf);
  buf[0] = (buf[0] & ~ASTAT_CC) | ((buffer[0] & 1) << ASTAT_CC_POS);
  regcache->raw_write (BFIN_ASTAT_REGNUM, buf);
}

static CORE_ADDR
bfin_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct bfin_frame_cache *cache = bfin_frame_cache (this_frame, this_cache);

  return cache->base;
}

static CORE_ADDR
bfin_frame_local_address (frame_info_ptr this_frame, void **this_cache)
{
  struct bfin_frame_cache *cache = bfin_frame_cache (this_frame, this_cache);

  return cache->base - 4;
}

static CORE_ADDR
bfin_frame_args_address (frame_info_ptr this_frame, void **this_cache)
{
  struct bfin_frame_cache *cache = bfin_frame_cache (this_frame, this_cache);

  return cache->base + 8;
}

static const struct frame_base bfin_frame_base =
{
  &bfin_frame_unwind,
  bfin_frame_base_address,
  bfin_frame_local_address,
  bfin_frame_args_address
};

static CORE_ADDR
bfin_frame_align (struct gdbarch *gdbarch, CORE_ADDR address)
{
  return align_down (address, 4);
}

enum bfin_abi
bfin_abi (struct gdbarch *gdbarch)
{
  bfin_gdbarch_tdep *tdep = gdbarch_tdep<bfin_gdbarch_tdep> (gdbarch);
  return tdep->bfin_abi;
}

/* Initialize the current architecture based on INFO.  If possible,
   re-use an architecture from ARCHES, which is a list of
   architectures already created during this debugging session.

   Called e.g. at program startup, when reading a core file, and when
   reading a binary file.  */

static struct gdbarch *
bfin_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  enum bfin_abi abi;

  abi = BFIN_ABI_FLAT;

  /* If there is already a candidate, use it.  */

  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      bfin_gdbarch_tdep *tdep
	= gdbarch_tdep<bfin_gdbarch_tdep> (arches->gdbarch);

      if (tdep->bfin_abi != abi)
	continue;

      return arches->gdbarch;
    }

  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new bfin_gdbarch_tdep));
  bfin_gdbarch_tdep *tdep = gdbarch_tdep<bfin_gdbarch_tdep> (gdbarch);

  tdep->bfin_abi = abi;

  set_gdbarch_num_regs (gdbarch, BFIN_NUM_REGS);
  set_gdbarch_pseudo_register_read (gdbarch, bfin_pseudo_register_read);
  set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						bfin_pseudo_register_write);
  set_gdbarch_num_pseudo_regs (gdbarch, BFIN_NUM_PSEUDO_REGS);
  set_gdbarch_sp_regnum (gdbarch, BFIN_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, BFIN_PC_REGNUM);
  set_gdbarch_ps_regnum (gdbarch, BFIN_ASTAT_REGNUM);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, bfin_reg_to_regnum);
  set_gdbarch_register_name (gdbarch, bfin_register_name);
  set_gdbarch_register_type (gdbarch, bfin_register_type);
  set_gdbarch_push_dummy_call (gdbarch, bfin_push_dummy_call);
  set_gdbarch_believe_pcc_promotion (gdbarch, 1);
  set_gdbarch_return_value (gdbarch, bfin_return_value);
  set_gdbarch_skip_prologue (gdbarch, bfin_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, bfin_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, bfin_sw_breakpoint_from_kind);
  set_gdbarch_decr_pc_after_break (gdbarch, 2);
  set_gdbarch_frame_args_skip (gdbarch, 8);
  set_gdbarch_frame_align (gdbarch, bfin_frame_align);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  dwarf2_append_unwinders (gdbarch);

  frame_base_set_default (gdbarch, &bfin_frame_base);

  frame_unwind_append_unwinder (gdbarch, &bfin_frame_unwind);

  return gdbarch;
}

void _initialize_bfin_tdep ();
void
_initialize_bfin_tdep ()
{
  gdbarch_register (bfd_arch_bfin, bfin_gdbarch_init);
}
