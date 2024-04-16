/* Target-dependent code for Moxie.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "value.h"
#include "inferior.h"
#include "symfile.h"
#include "objfiles.h"
#include "osabi.h"
#include "language.h"
#include "arch-utils.h"
#include "regcache.h"
#include "trad-frame.h"
#include "dis-asm.h"
#include "record.h"
#include "record-full.h"

#include "moxie-tdep.h"
#include <algorithm>

/* Use an invalid address value as 'not available' marker.  */
enum { REG_UNAVAIL = (CORE_ADDR) -1 };

struct moxie_frame_cache
{
  /* Base address.  */
  CORE_ADDR base;
  CORE_ADDR pc;
  LONGEST framesize;
  CORE_ADDR saved_regs[MOXIE_NUM_REGS];
  CORE_ADDR saved_sp;
};

/* Implement the "frame_align" gdbarch method.  */

static CORE_ADDR
moxie_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  /* Align to the size of an instruction (so that they can safely be
     pushed onto the stack.  */
  return sp & ~1;
}

constexpr gdb_byte moxie_break_insn[] = { 0x35, 0x00 };

typedef BP_MANIPULATION (moxie_break_insn) moxie_breakpoint;

/* Moxie register names.  */

static const char * const moxie_register_names[] = {
  "$fp",  "$sp",  "$r0",  "$r1",  "$r2",
  "$r3",  "$r4",  "$r5", "$r6", "$r7",
  "$r8", "$r9", "$r10", "$r11", "$r12",
  "$r13", "$pc", "$cc" };

/* Implement the "register_name" gdbarch method.  */

static const char *
moxie_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static_assert (ARRAY_SIZE (moxie_register_names) == MOXIE_NUM_REGS);
  return moxie_register_names[reg_nr];
}

/* Implement the "register_type" gdbarch method.  */

static struct type *
moxie_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  if (reg_nr == MOXIE_PC_REGNUM)
    return  builtin_type (gdbarch)->builtin_func_ptr;
  else if (reg_nr == MOXIE_SP_REGNUM || reg_nr == MOXIE_FP_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;
  else
    return builtin_type (gdbarch)->builtin_int32;
}

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  */

static void
moxie_store_return_value (struct type *type, struct regcache *regcache,
			 const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR regval;
  int len = type->length ();

  /* Things always get returned in RET1_REGNUM, RET2_REGNUM.  */
  regval = extract_unsigned_integer (valbuf, len > 4 ? 4 : len, byte_order);
  regcache_cooked_write_unsigned (regcache, RET1_REGNUM, regval);
  if (len > 4)
    {
      regval = extract_unsigned_integer (valbuf + 4, len - 4, byte_order);
      regcache_cooked_write_unsigned (regcache, RET1_REGNUM + 1, regval);
    }
}

/* Decode the instructions within the given address range.  Decide
   when we must have reached the end of the function prologue.  If a
   frame_info pointer is provided, fill in its saved_regs etc.

   Returns the address of the first instruction after the prologue.  */

static CORE_ADDR
moxie_analyze_prologue (CORE_ADDR start_addr, CORE_ADDR end_addr,
			struct moxie_frame_cache *cache,
			struct gdbarch *gdbarch)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR next_addr;
  ULONGEST inst, inst2;
  LONGEST offset;
  int regnum;

  /* Record where the jsra instruction saves the PC and FP.  */
  cache->saved_regs[MOXIE_PC_REGNUM] = -4;
  cache->saved_regs[MOXIE_FP_REGNUM] = 0;
  cache->framesize = 0;

  if (start_addr >= end_addr)
    return end_addr;

  for (next_addr = start_addr; next_addr < end_addr; )
    {
      inst = read_memory_unsigned_integer (next_addr, 2, byte_order);

      /* Match "push $sp $rN" where N is between 0 and 13 inclusive.  */
      if (inst >= 0x0612 && inst <= 0x061f)
	{
	  regnum = inst & 0x000f;
	  cache->framesize += 4;
	  cache->saved_regs[regnum] = cache->framesize;
	  next_addr += 2;
	}
      else
	break;
    }

  inst = read_memory_unsigned_integer (next_addr, 2, byte_order);

  /* Optional stack allocation for args and local vars <= 4
     byte.  */
  if (inst == 0x01e0)          /* ldi.l $r12, X */
    {
      offset = read_memory_integer (next_addr + 2, 4, byte_order);
      inst2 = read_memory_unsigned_integer (next_addr + 6, 2, byte_order);
      
      if (inst2 == 0x291e)     /* sub.l $sp, $r12 */
	{
	  cache->framesize += offset;
	}
      
      return (next_addr + 8);
    }
  else if ((inst & 0xff00) == 0x9100)   /* dec $sp, X */
    {
      cache->framesize += (inst & 0x00ff);
      next_addr += 2;

      while (next_addr < end_addr)
	{
	  inst = read_memory_unsigned_integer (next_addr, 2, byte_order);
	  if ((inst & 0xff00) != 0x9100) /* no more dec $sp, X */
	    break;
	  cache->framesize += (inst & 0x00ff);
	  next_addr += 2;
	}
    }

  return next_addr;
}

/* Find the end of function prologue.  */

static CORE_ADDR
moxie_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr = 0, func_end = 0;
  const char *func_name;

  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever
     is greater.  */
  if (find_pc_partial_function (pc, &func_name, &func_addr, &func_end))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);
      if (post_prologue_pc != 0)
	return std::max (pc, post_prologue_pc);
      else
	{
	  /* Can't determine prologue from the symbol table, need to examine
	     instructions.  */
	  struct symtab_and_line sal;
	  struct symbol *sym;
	  struct moxie_frame_cache cache;
	  CORE_ADDR plg_end;
	  
	  memset (&cache, 0, sizeof cache);
	  
	  plg_end = moxie_analyze_prologue (func_addr, 
					    func_end, &cache, gdbarch);
	  /* Found a function.  */
	  sym = lookup_symbol (func_name, NULL, VAR_DOMAIN, NULL).symbol;
	  /* Don't use line number debug info for assembly source
	     files.  */
	  if (sym && sym->language () != language_asm)
	    {
	      sal = find_pc_line (func_addr, 0);
	      if (sal.end && sal.end < func_end)
		{
		  /* Found a line number, use it as end of
		     prologue.  */
		  return sal.end;
		}
	    }
	  /* No useable line symbol.  Use result of prologue parsing
	     method.  */
	  return plg_end;
	}
    }

  /* No function symbol -- just return the PC.  */
  return (CORE_ADDR) pc;
}

struct moxie_unwind_cache
{
  /* The previous frame's inner most stack address.  Used as this
     frame ID's stack_addr.  */
  CORE_ADDR prev_sp;
  /* The frame's base, optionally used by the high-level debug info.  */
  CORE_ADDR base;
  int size;
  /* How far the SP and r13 (FP) have been offset from the start of
     the stack frame (as defined by the previous frame's stack
     pointer).  */
  LONGEST sp_offset;
  LONGEST r13_offset;
  int uses_frame;
  /* Table indicating the location of each and every register.  */
  trad_frame_saved_reg *saved_regs;
};

/* Read an unsigned integer from the inferior, and adjust
   endianness.  */
static ULONGEST
moxie_process_readu (CORE_ADDR addr, gdb_byte *buf,
		     int length, enum bfd_endian byte_order)
{
  if (target_read_memory (addr, buf, length))
    {
      if (record_debug)
	gdb_printf (gdb_stderr,
		    _("Process record: error reading memory at "
		      "addr 0x%s len = %d.\n"),
		    paddress (current_inferior ()->arch  (), addr), length);
      return -1;
    }

  return extract_unsigned_integer (buf, length, byte_order);
}


/* Helper macro to extract the signed 10-bit offset from a 16-bit
   branch instruction.	*/
#define INST2OFFSET(o) ((((signed short)((o & ((1<<10)-1))<<6))>>6)<<1)

/* Insert a single step breakpoint.  */

static std::vector<CORE_ADDR>
moxie_software_single_step (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  CORE_ADDR addr;
  gdb_byte buf[4];
  uint16_t inst;
  uint32_t tmpu32;
  ULONGEST fp;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  std::vector<CORE_ADDR> next_pcs;

  addr = regcache_read_pc (regcache);

  inst = (uint16_t) moxie_process_readu (addr, buf, 2, byte_order);

  /* Decode instruction.  */
  if (inst & (1 << 15))
    {
      if (inst & (1 << 14))
	{
	  /* This is a Form 3 instruction.  */
	  int opcode = (inst >> 10 & 0xf);

	  switch (opcode)
	    {
	    case 0x00: /* beq */
	    case 0x01: /* bne */
	    case 0x02: /* blt */
	    case 0x03: /* bgt */
	    case 0x04: /* bltu */
	    case 0x05: /* bgtu */
	    case 0x06: /* bge */
	    case 0x07: /* ble */
	    case 0x08: /* bgeu */
	    case 0x09: /* bleu */
	      /* Insert breaks on both branches, because we can't currently tell
		 which way things will go.  */
	      next_pcs.push_back (addr + 2);
	      next_pcs.push_back (addr + 2 + INST2OFFSET(inst));
	      break;
	    default:
	      {
		/* Do nothing.	*/
		break;
	      }
	    }
	}
      else
	{
	  /* This is a Form 2 instruction.  They are all 16 bits.  */
	  next_pcs.push_back (addr + 2);
	}
    }
  else
    {
      /* This is a Form 1 instruction.	*/
      int opcode = inst >> 8;

      switch (opcode)
	{
	  /* 16-bit instructions.  */
	case 0x00: /* bad */
	case 0x02: /* mov (register-to-register) */
	case 0x05: /* add.l */
	case 0x06: /* push */
	case 0x07: /* pop */
	case 0x0a: /* ld.l (register indirect) */
	case 0x0b: /* st.l */
	case 0x0e: /* cmp */
	case 0x0f: /* nop */
	case 0x10: /* sex.b */
	case 0x11: /* sex.s */
	case 0x12: /* zex.b */
	case 0x13: /* zex.s */
	case 0x14: /* umul.x */
	case 0x15: /* mul.x */
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x1c: /* ld.b (register indirect) */
	case 0x1e: /* st.b */
	case 0x21: /* ld.s (register indirect) */
	case 0x23: /* st.s */
	case 0x26: /* and */
	case 0x27: /* lshr */
	case 0x28: /* ashl */
	case 0x29: /* sub.l */
	case 0x2a: /* neg */
	case 0x2b: /* or */
	case 0x2c: /* not */
	case 0x2d: /* ashr */
	case 0x2e: /* xor */
	case 0x2f: /* mul.l */
	case 0x31: /* div.l */
	case 0x32: /* udiv.l */
	case 0x33: /* mod.l */
	case 0x34: /* umod.l */
	  next_pcs.push_back (addr + 2);
	  break;

	  /* 32-bit instructions.  */
	case 0x0c: /* ldo.l */
	case 0x0d: /* sto.l */
	case 0x36: /* ldo.b */
	case 0x37: /* sto.b */
	case 0x38: /* ldo.s */
	case 0x39: /* sto.s */
	  next_pcs.push_back (addr + 4);
	  break;

	  /* 48-bit instructions.  */
	case 0x01: /* ldi.l (immediate) */
	case 0x08: /* lda.l */
	case 0x09: /* sta.l */
	case 0x1b: /* ldi.b (immediate) */
	case 0x1d: /* lda.b */
	case 0x1f: /* sta.b */
	case 0x20: /* ldi.s (immediate) */
	case 0x22: /* lda.s */
	case 0x24: /* sta.s */
	  next_pcs.push_back (addr + 6);
	  break;

	  /* Control flow instructions.	 */
	case 0x03: /* jsra */
	case 0x1a: /* jmpa */
	  next_pcs.push_back (moxie_process_readu (addr + 2, buf, 4,
						   byte_order));
	  break;

	case 0x04: /* ret */
	  regcache_cooked_read_unsigned (regcache, MOXIE_FP_REGNUM, &fp);
	  next_pcs.push_back (moxie_process_readu (fp + 4, buf, 4, byte_order));
	  break;

	case 0x19: /* jsr */
	case 0x25: /* jmp */
	  regcache->raw_read ((inst >> 4) & 0xf, (gdb_byte *) & tmpu32);
	  next_pcs.push_back (tmpu32);
	  break;

	case 0x30: /* swi */
	case 0x35: /* brk */
	  /* Unsupported, for now.  */
	  break;
	}
    }

  return next_pcs;
}

/* Given a return value in `regbuf' with a type `valtype', 
   extract and copy its value into `valbuf'.  */

static void
moxie_extract_return_value (struct type *type, struct regcache *regcache,
			    gdb_byte *dst)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int len = type->length ();
  ULONGEST tmp;

  /* By using store_unsigned_integer we avoid having to do
     anything special for small big-endian values.  */
  regcache_cooked_read_unsigned (regcache, RET1_REGNUM, &tmp);
  store_unsigned_integer (dst, (len > 4 ? len - 4 : len), byte_order, tmp);

  /* Ignore return values more than 8 bytes in size because the moxie
     returns anything more than 8 bytes in the stack.  */
  if (len > 4)
    {
      regcache_cooked_read_unsigned (regcache, RET1_REGNUM + 1, &tmp);
      store_unsigned_integer (dst + len - 4, 4, byte_order, tmp);
    }
}

/* Implement the "return_value" gdbarch method.  */

static enum return_value_convention
moxie_return_value (struct gdbarch *gdbarch, struct value *function,
		   struct type *valtype, struct regcache *regcache,
		   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (valtype->length () > 8)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else
    {
      if (readbuf != NULL)
	moxie_extract_return_value (valtype, regcache, readbuf);
      if (writebuf != NULL)
	moxie_store_return_value (valtype, regcache, writebuf);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

/* Allocate and initialize a moxie_frame_cache object.  */

static struct moxie_frame_cache *
moxie_alloc_frame_cache (void)
{
  struct moxie_frame_cache *cache;
  int i;

  cache = FRAME_OBSTACK_ZALLOC (struct moxie_frame_cache);

  cache->base = 0;
  cache->saved_sp = 0;
  cache->pc = 0;
  cache->framesize = 0;
  for (i = 0; i < MOXIE_NUM_REGS; ++i)
    cache->saved_regs[i] = REG_UNAVAIL;

  return cache;
}

/* Populate a moxie_frame_cache object for this_frame.  */

static struct moxie_frame_cache *
moxie_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct moxie_frame_cache *cache;
  CORE_ADDR current_pc;
  int i;

  if (*this_cache)
    return (struct moxie_frame_cache *) *this_cache;

  cache = moxie_alloc_frame_cache ();
  *this_cache = cache;

  cache->base = get_frame_register_unsigned (this_frame, MOXIE_FP_REGNUM);
  if (cache->base == 0)
    return cache;

  cache->pc = get_frame_func (this_frame);
  current_pc = get_frame_pc (this_frame);
  if (cache->pc)
    {
      struct gdbarch *gdbarch = get_frame_arch (this_frame);
      moxie_analyze_prologue (cache->pc, current_pc, cache, gdbarch);
    }

  cache->saved_sp = cache->base - cache->framesize;

  for (i = 0; i < MOXIE_NUM_REGS; ++i)
    if (cache->saved_regs[i] != REG_UNAVAIL)
      cache->saved_regs[i] = cache->base - cache->saved_regs[i];

  return cache;
}

/* Given a GDB frame, determine the address of the calling function's
   frame.  This will be used to create a new GDB frame struct.  */

static void
moxie_frame_this_id (frame_info_ptr this_frame,
		    void **this_prologue_cache, struct frame_id *this_id)
{
  struct moxie_frame_cache *cache = moxie_frame_cache (this_frame,
						   this_prologue_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  *this_id = frame_id_build (cache->saved_sp, cache->pc);
}

/* Get the value of register regnum in the previous stack frame.  */

static struct value *
moxie_frame_prev_register (frame_info_ptr this_frame,
			  void **this_prologue_cache, int regnum)
{
  struct moxie_frame_cache *cache = moxie_frame_cache (this_frame,
						   this_prologue_cache);

  gdb_assert (regnum >= 0);

  if (regnum == MOXIE_SP_REGNUM && cache->saved_sp)
    return frame_unwind_got_constant (this_frame, regnum, cache->saved_sp);

  if (regnum < MOXIE_NUM_REGS && cache->saved_regs[regnum] != REG_UNAVAIL)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->saved_regs[regnum]);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind moxie_frame_unwind = {
  "moxie prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  moxie_frame_this_id,
  moxie_frame_prev_register,
  NULL,
  default_frame_sniffer
};

/* Return the base address of this_frame.  */

static CORE_ADDR
moxie_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct moxie_frame_cache *cache = moxie_frame_cache (this_frame,
						       this_cache);

  return cache->base;
}

static const struct frame_base moxie_frame_base = {
  &moxie_frame_unwind,
  moxie_frame_base_address,
  moxie_frame_base_address,
  moxie_frame_base_address
};

/* Parse the current instruction and record the values of the registers and
   memory that will be changed in current instruction to "record_arch_list".
   Return -1 if something wrong.  */

static int
moxie_process_record (struct gdbarch *gdbarch, struct regcache *regcache,
		      CORE_ADDR addr)
{
  gdb_byte buf[4];
  uint16_t inst;
  uint32_t tmpu32;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  if (record_debug > 1)
    gdb_printf (gdb_stdlog, "Process record: moxie_process_record "
		"addr = 0x%s\n",
		paddress (current_inferior ()->arch  (), addr));

  inst = (uint16_t) moxie_process_readu (addr, buf, 2, byte_order);

  /* Decode instruction.  */
  if (inst & (1 << 15))
    {
      if (inst & (1 << 14))
	{
	  /* This is a Form 3 instruction.  */
	  int opcode = (inst >> 10 & 0xf);
	  
	  switch (opcode)
	    {
	    case 0x00: /* beq */
	    case 0x01: /* bne */
	    case 0x02: /* blt */
	    case 0x03: /* bgt */
	    case 0x04: /* bltu */
	    case 0x05: /* bgtu */
	    case 0x06: /* bge */
	    case 0x07: /* ble */
	    case 0x08: /* bgeu */
	    case 0x09: /* bleu */
	      /* Do nothing.  */
	      break;
	    default:
	      {
		/* Do nothing.  */
		break;
	      }
	    }
	}
      else
	{
	  /* This is a Form 2 instruction.  */
	  int opcode = (inst >> 12 & 0x3);
	  switch (opcode)
	    {
	    case 0x00: /* inc */
	    case 0x01: /* dec */
	    case 0x02: /* gsr */
	      {
		int reg = (inst >> 8) & 0xf;
		if (record_full_arch_list_add_reg (regcache, reg))
		  return -1;
	      }
	      break;
	    case 0x03: /* ssr */
	      {
		/* Do nothing until GDB learns about moxie's special
		   registers.  */
	      }
	      break;
	    default:
	      /* Do nothing.  */
	      break;
	    }
	}
    }
  else
    {
      /* This is a Form 1 instruction.  */
      int opcode = inst >> 8;

      switch (opcode)
	{
	case 0x00: /* nop */
	  /* Do nothing.  */
	  break;
	case 0x01: /* ldi.l (immediate) */
	case 0x02: /* mov (register-to-register) */
	  {
	    int reg = (inst >> 4) & 0xf;
	    if (record_full_arch_list_add_reg (regcache, reg))
	      return -1;
	  }
	  break;
	case 0x03: /* jsra */
	  {
	    regcache->raw_read (
			       MOXIE_SP_REGNUM, (gdb_byte *) & tmpu32);
	    tmpu32 = extract_unsigned_integer ((gdb_byte *) & tmpu32, 
					       4, byte_order);
	    if (record_full_arch_list_add_reg (regcache, MOXIE_FP_REGNUM)
		|| (record_full_arch_list_add_reg (regcache,
						   MOXIE_SP_REGNUM))
		|| record_full_arch_list_add_mem (tmpu32 - 12, 12))
	      return -1;
	  }
	  break;
	case 0x04: /* ret */
	  {
	    if (record_full_arch_list_add_reg (regcache, MOXIE_FP_REGNUM)
		|| (record_full_arch_list_add_reg (regcache,
						   MOXIE_SP_REGNUM)))
	      return -1;
	  }
	  break;
	case 0x05: /* add.l */
	  {
	    int reg = (inst >> 4) & 0xf;
	    if (record_full_arch_list_add_reg (regcache, reg))
	      return -1;
	  }
	  break;
	case 0x06: /* push */
	  {
	    int reg = (inst >> 4) & 0xf;
	    regcache->raw_read (reg, (gdb_byte *) & tmpu32);
	    tmpu32 = extract_unsigned_integer ((gdb_byte *) & tmpu32, 
					       4, byte_order);
	    if (record_full_arch_list_add_reg (regcache, reg)
		|| record_full_arch_list_add_mem (tmpu32 - 4, 4))
	      return -1;
	  }
	  break;
	case 0x07: /* pop */
	  {
	    int a = (inst >> 4) & 0xf;
	    int b = inst & 0xf;
	    if (record_full_arch_list_add_reg (regcache, a)
		|| record_full_arch_list_add_reg (regcache, b))
	      return -1;
	  }
	  break;
	case 0x08: /* lda.l */
	  {
	    int reg = (inst >> 4) & 0xf;
	    if (record_full_arch_list_add_reg (regcache, reg))
	      return -1;
	  }
	  break;
	case 0x09: /* sta.l */
	  {
	    tmpu32 = (uint32_t) moxie_process_readu (addr+2, buf, 
						     4, byte_order);
	    if (record_full_arch_list_add_mem (tmpu32, 4))
	      return -1;
	  }
	  break;
	case 0x0a: /* ld.l (register indirect) */
	  {
	    int reg = (inst >> 4) & 0xf;
	    if (record_full_arch_list_add_reg (regcache, reg))
	      return -1;
	  }
	  break;
	case 0x0b: /* st.l */
	  {
	    int reg = (inst >> 4) & 0xf;
	    regcache->raw_read (reg, (gdb_byte *) & tmpu32);
	    tmpu32 = extract_unsigned_integer ((gdb_byte *) & tmpu32, 
					       4, byte_order);
	    if (record_full_arch_list_add_mem (tmpu32, 4))
	      return -1;
	  }
	  break;
	case 0x0c: /* ldo.l */
	  {
	    int reg = (inst >> 4) & 0xf;
	    if (record_full_arch_list_add_reg (regcache, reg))
	      return -1;
	  }
	  break;
	case 0x0d: /* sto.l */
	  {
	    int reg = (inst >> 4) & 0xf;
	    uint32_t offset = (((int16_t) moxie_process_readu (addr+2, buf, 2,
							       byte_order)) << 16 ) >> 16;
	    regcache->raw_read (reg, (gdb_byte *) & tmpu32);
	    tmpu32 = extract_unsigned_integer ((gdb_byte *) & tmpu32, 
					       4, byte_order);
	    tmpu32 += offset;
	    if (record_full_arch_list_add_mem (tmpu32, 4))
	      return -1;
	  }
	  break;
	case 0x0e: /* cmp */
	  {
	    if (record_full_arch_list_add_reg (regcache, MOXIE_CC_REGNUM))
	      return -1;
	  }
	  break;
	case 0x0f: /* nop */
	  {
	    /* Do nothing.  */
	    break;
	  }
	case 0x10: /* sex.b */
	case 0x11: /* sex.s */
	case 0x12: /* zex.b */
	case 0x13: /* zex.s */
	case 0x14: /* umul.x */
	case 0x15: /* mul.x */
	  {
	    int reg = (inst >> 4) & 0xf;
	    if (record_full_arch_list_add_reg (regcache, reg))
	      return -1;
	  }
	  break;
	case 0x16:
	case 0x17:
	case 0x18:
	  {
	    /* Do nothing.  */
	    break;
	  }
	case 0x19: /* jsr */
	  {
	    regcache->raw_read (
			       MOXIE_SP_REGNUM, (gdb_byte *) & tmpu32);
	    tmpu32 = extract_unsigned_integer ((gdb_byte *) & tmpu32, 
					       4, byte_order);
	    if (record_full_arch_list_add_reg (regcache, MOXIE_FP_REGNUM)
		|| (record_full_arch_list_add_reg (regcache,
						   MOXIE_SP_REGNUM))
		|| record_full_arch_list_add_mem (tmpu32 - 12, 12))
	      return -1;
	  }
	  break;
	case 0x1a: /* jmpa */
	  {
	    /* Do nothing.  */
	  }
	  break;
	case 0x1b: /* ldi.b (immediate) */
	case 0x1c: /* ld.b (register indirect) */
	case 0x1d: /* lda.b */
	  {
	    int reg = (inst >> 4) & 0xf;
	    if (record_full_arch_list_add_reg (regcache, reg))
	      return -1;
	  }
	  break;
	case 0x1e: /* st.b */
	  {
	    int reg = (inst >> 4) & 0xf;
	    regcache->raw_read (reg, (gdb_byte *) & tmpu32);
	    tmpu32 = extract_unsigned_integer ((gdb_byte *) & tmpu32, 
					       4, byte_order);
	    if (record_full_arch_list_add_mem (tmpu32, 1))
	      return -1;
	  }
	  break;
	case 0x1f: /* sta.b */
	  {
	    tmpu32 = moxie_process_readu (addr+2, buf, 4, byte_order);
	    if (record_full_arch_list_add_mem (tmpu32, 1))
	      return -1;
	  }
	  break;
	case 0x20: /* ldi.s (immediate) */
	case 0x21: /* ld.s (register indirect) */
	case 0x22: /* lda.s */
	  {
	    int reg = (inst >> 4) & 0xf;
	    if (record_full_arch_list_add_reg (regcache, reg))
	      return -1;
	  }
	  break;
	case 0x23: /* st.s */
	  {
	    int reg = (inst >> 4) & 0xf;
	    regcache->raw_read (reg, (gdb_byte *) & tmpu32);
	    tmpu32 = extract_unsigned_integer ((gdb_byte *) & tmpu32, 
					       4, byte_order);
	    if (record_full_arch_list_add_mem (tmpu32, 2))
	      return -1;
	  }
	  break;
	case 0x24: /* sta.s */
	  {
	    tmpu32 = moxie_process_readu (addr+2, buf, 4, byte_order);
	    if (record_full_arch_list_add_mem (tmpu32, 2))
	      return -1;
	  }
	  break;
	case 0x25: /* jmp */
	  {
	    /* Do nothing.  */
	  }
	  break;
	case 0x26: /* and */
	case 0x27: /* lshr */
	case 0x28: /* ashl */
	case 0x29: /* sub */
	case 0x2a: /* neg */
	case 0x2b: /* or */
	case 0x2c: /* not */
	case 0x2d: /* ashr */
	case 0x2e: /* xor */
	case 0x2f: /* mul */
	  {
	    int reg = (inst >> 4) & 0xf;
	    if (record_full_arch_list_add_reg (regcache, reg))
	      return -1;
	  }
	  break;
	case 0x30: /* swi */
	  {
	    /* We currently implement support for libgloss' 
	       system calls.  */

	    int inum = moxie_process_readu (addr+2, buf, 4, byte_order);

	    switch (inum)
	      {
	      case 0x1: /* SYS_exit */
		{
		  /* Do nothing.  */
		}
		break;
	      case 0x2: /* SYS_open */
		{
		  if (record_full_arch_list_add_reg (regcache, RET1_REGNUM))
		    return -1;
		}
		break;
	      case 0x4: /* SYS_read */
		{
		  uint32_t length, ptr;

		  /* Read buffer pointer is in $r1.  */
		  regcache->raw_read (3, (gdb_byte *) & ptr);
		  ptr = extract_unsigned_integer ((gdb_byte *) & ptr, 
						  4, byte_order);

		  /* String length is at 0x12($fp).  */
		  regcache->raw_read (
				     MOXIE_FP_REGNUM, (gdb_byte *) & tmpu32);
		  tmpu32 = extract_unsigned_integer ((gdb_byte *) & tmpu32, 
						     4, byte_order);
		  length = moxie_process_readu (tmpu32+20, buf, 4, byte_order);

		  if (record_full_arch_list_add_mem (ptr, length))
		    return -1;
		}
		break;
	      case 0x5: /* SYS_write */
		{
		  if (record_full_arch_list_add_reg (regcache, RET1_REGNUM))
		    return -1;
		}
		break;
	      default:
		break;
	      }
	  }
	  break;
	case 0x31: /* div.l */
	case 0x32: /* udiv.l */
	case 0x33: /* mod.l */
	case 0x34: /* umod.l */
	  {
	    int reg = (inst >> 4) & 0xf;
	    if (record_full_arch_list_add_reg (regcache, reg))
	      return -1;
	  }
	  break;
	case 0x35: /* brk */
	  /* Do nothing.  */
	  break;
	case 0x36: /* ldo.b */
	  {
	    int reg = (inst >> 4) & 0xf;
	    if (record_full_arch_list_add_reg (regcache, reg))
	      return -1;
	  }
	  break;
	case 0x37: /* sto.b */
	  {
	    int reg = (inst >> 4) & 0xf;
	    uint32_t offset = (((int16_t) moxie_process_readu (addr+2, buf, 2,
							       byte_order)) << 16 ) >> 16;
	    regcache->raw_read (reg, (gdb_byte *) & tmpu32);
	    tmpu32 = extract_unsigned_integer ((gdb_byte *) & tmpu32, 
					       4, byte_order);
	    tmpu32 += offset;
	    if (record_full_arch_list_add_mem (tmpu32, 1))
	      return -1;
	  }
	  break;
	case 0x38: /* ldo.s */
	  {
	    int reg = (inst >> 4) & 0xf;
	    if (record_full_arch_list_add_reg (regcache, reg))
	      return -1;
	  }
	  break;
	case 0x39: /* sto.s */
	  {
	    int reg = (inst >> 4) & 0xf;
	    uint32_t offset = (((int16_t) moxie_process_readu (addr+2, buf, 2,
							       byte_order)) << 16 ) >> 16;
	    regcache->raw_read (reg, (gdb_byte *) & tmpu32);
	    tmpu32 = extract_unsigned_integer ((gdb_byte *) & tmpu32, 
					       4, byte_order);
	    tmpu32 += offset;
	    if (record_full_arch_list_add_mem (tmpu32, 2))
	      return -1;
	  }
	  break;
	default:
	  /* Do nothing.  */
	  break;
	}
    }

  if (record_full_arch_list_add_reg (regcache, MOXIE_PC_REGNUM))
    return -1;
  if (record_full_arch_list_add_end ())
    return -1;
  return 0;
}

/* Allocate and initialize the moxie gdbarch object.  */

static struct gdbarch *
moxie_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  /* If there is already a candidate, use it.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* Allocate space for the new architecture.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new moxie_gdbarch_tdep));

  set_gdbarch_wchar_bit (gdbarch, 32);
  set_gdbarch_wchar_signed (gdbarch, 0);

  set_gdbarch_num_regs (gdbarch, MOXIE_NUM_REGS);
  set_gdbarch_sp_regnum (gdbarch, MOXIE_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, MOXIE_PC_REGNUM);
  set_gdbarch_register_name (gdbarch, moxie_register_name);
  set_gdbarch_register_type (gdbarch, moxie_register_type);

  set_gdbarch_return_value (gdbarch, moxie_return_value);

  set_gdbarch_skip_prologue (gdbarch, moxie_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       moxie_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       moxie_breakpoint::bp_from_kind);
  set_gdbarch_frame_align (gdbarch, moxie_frame_align);

  frame_base_set_default (gdbarch, &moxie_frame_base);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  /* Hook in the default unwinders.  */
  frame_unwind_append_unwinder (gdbarch, &moxie_frame_unwind);

  /* Single stepping.  */
  set_gdbarch_software_single_step (gdbarch, moxie_software_single_step);

  /* Support simple overlay manager.  */
  set_gdbarch_overlay_update (gdbarch, simple_overlay_update);

  /* Support reverse debugging.  */
  set_gdbarch_process_record (gdbarch, moxie_process_record);

  return gdbarch;
}

/* Register this machine's init routine.  */

void _initialize_moxie_tdep ();
void
_initialize_moxie_tdep ()
{
  gdbarch_register (bfd_arch_moxie, moxie_gdbarch_init);
}
