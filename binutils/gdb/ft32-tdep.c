/* Target-dependent code for FT32.

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

#include "opcode/ft32.h"

#include "ft32-tdep.h"
#include "sim/sim-ft32.h"
#include <algorithm>

#define RAM_BIAS  0x800000  /* Bias added to RAM addresses.  */

/* Use an invalid address -1 as 'not available' marker.  */
enum { REG_UNAVAIL = (CORE_ADDR) (-1) };

struct ft32_frame_cache
{
  /* Base address of the frame */
  CORE_ADDR base;
  /* Function this frame belongs to */
  CORE_ADDR pc;
  /* Total size of this frame */
  LONGEST framesize;
  /* Saved registers in this frame */
  CORE_ADDR saved_regs[FT32_NUM_REGS];
  /* Saved SP in this frame */
  CORE_ADDR saved_sp;
  /* Has the new frame been LINKed.  */
  bfd_boolean established;
};

/* Implement the "frame_align" gdbarch method.  */

static CORE_ADDR
ft32_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  /* Align to the size of an instruction (so that they can safely be
     pushed onto the stack.  */
  return sp & ~1;
}


constexpr gdb_byte ft32_break_insn[] = { 0x02, 0x00, 0x34, 0x00 };

typedef BP_MANIPULATION (ft32_break_insn) ft32_breakpoint;

/* FT32 register names.  */

static const char *const ft32_register_names[] =
{
    "fp", "sp",
    "r0", "r1", "r2", "r3",  "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19",  "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28", "cc",
    "pc"
};

/* Implement the "register_name" gdbarch method.  */

static const char *
ft32_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  static_assert (ARRAY_SIZE (ft32_register_names) == FT32_NUM_REGS);
  return ft32_register_names[reg_nr];
}

/* Implement the "register_type" gdbarch method.  */

static struct type *
ft32_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  if (reg_nr == FT32_PC_REGNUM)
    {
      ft32_gdbarch_tdep *tdep = gdbarch_tdep<ft32_gdbarch_tdep> (gdbarch);
      return tdep->pc_type;
    }
  else if (reg_nr == FT32_SP_REGNUM || reg_nr == FT32_FP_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;
  else
    return builtin_type (gdbarch)->builtin_int32;
}

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  */

static void
ft32_store_return_value (struct type *type, struct regcache *regcache,
			 const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR regval;
  int len = type->length ();

  /* Things always get returned in RET1_REGNUM, RET2_REGNUM.  */
  regval = extract_unsigned_integer (valbuf, len > 4 ? 4 : len, byte_order);
  regcache_cooked_write_unsigned (regcache, FT32_R0_REGNUM, regval);
  if (len > 4)
    {
      regval = extract_unsigned_integer (valbuf + 4,
					 len - 4, byte_order);
      regcache_cooked_write_unsigned (regcache, FT32_R1_REGNUM, regval);
    }
}

/* Fetch a single 32-bit instruction from address a. If memory contains
   a compressed instruction pair, return the expanded instruction.  */

static ULONGEST
ft32_fetch_instruction (CORE_ADDR a, int *isize,
			enum bfd_endian byte_order)
{
  unsigned int sc[2];
  ULONGEST inst;

  CORE_ADDR a4 = a & ~3;
  inst = read_code_unsigned_integer (a4, 4, byte_order);
  *isize = ft32_decode_shortcode (a4, inst, sc) ? 2 : 4;
  if (*isize == 2)
    return sc[1 & (a >> 1)];
  else
    return inst;
}

/* Decode the instructions within the given address range.  Decide
   when we must have reached the end of the function prologue.  If a
   frame_info pointer is provided, fill in its saved_regs etc.

   Returns the address of the first instruction after the prologue.  */

static CORE_ADDR
ft32_analyze_prologue (CORE_ADDR start_addr, CORE_ADDR end_addr,
		       struct ft32_frame_cache *cache,
		       struct gdbarch *gdbarch)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR next_addr;
  ULONGEST inst;
  int isize = 0;
  int regnum, pushreg;
  struct bound_minimal_symbol msymbol;
  const int first_saved_reg = 13;	/* The first saved register.  */
  /* PROLOGS are addresses of the subroutine prologs, PROLOGS[n]
     is the address of __prolog_$rN.
     __prolog_$rN pushes registers from 13 through n inclusive.
     So for example CALL __prolog_$r15 is equivalent to:
       PUSH $r13 
       PUSH $r14 
       PUSH $r15 
     Note that PROLOGS[0] through PROLOGS[12] are unused.  */
  CORE_ADDR prologs[32];

  cache->saved_regs[FT32_PC_REGNUM] = 0;
  cache->framesize = 0;

  for (regnum = first_saved_reg; regnum < 32; regnum++)
    {
      char prolog_symbol[32];

      snprintf (prolog_symbol, sizeof (prolog_symbol), "__prolog_$r%02d",
		regnum);
      msymbol = lookup_minimal_symbol (prolog_symbol, NULL, NULL);
      if (msymbol.minsym)
	prologs[regnum] = msymbol.value_address ();
      else
	prologs[regnum] = 0;
    }

  if (start_addr >= end_addr)
    return end_addr;

  cache->established = 0;
  for (next_addr = start_addr; next_addr < end_addr; next_addr += isize)
    {
      inst = ft32_fetch_instruction (next_addr, &isize, byte_order);

      if (FT32_IS_PUSH (inst))
	{
	  pushreg = FT32_PUSH_REG (inst);
	  cache->framesize += 4;
	  cache->saved_regs[FT32_R0_REGNUM + pushreg] = cache->framesize;
	}
      else if (FT32_IS_CALL (inst))
	{
	  for (regnum = first_saved_reg; regnum < 32; regnum++)
	    {
	      if ((4 * (inst & 0x3ffff)) == prologs[regnum])
		{
		  for (pushreg = first_saved_reg; pushreg <= regnum;
		       pushreg++)
		    {
		      cache->framesize += 4;
		      cache->saved_regs[FT32_R0_REGNUM + pushreg] =
			cache->framesize;
		    }
		}
	    }
	  break;
	}
      else
	break;
    }
  for (regnum = FT32_R0_REGNUM; regnum < FT32_PC_REGNUM; regnum++)
    {
      if (cache->saved_regs[regnum] != REG_UNAVAIL)
	cache->saved_regs[regnum] =
	  cache->framesize - cache->saved_regs[regnum];
    }
  cache->saved_regs[FT32_PC_REGNUM] = cache->framesize;

  /* It is a LINK?  */
  if (next_addr < end_addr)
    {
      inst = ft32_fetch_instruction (next_addr, &isize, byte_order);
      if (FT32_IS_LINK (inst))
	{
	  cache->established = 1;
	  for (regnum = FT32_R0_REGNUM; regnum < FT32_PC_REGNUM; regnum++)
	    {
	      if (cache->saved_regs[regnum] != REG_UNAVAIL)
		cache->saved_regs[regnum] += 4;
	    }
	  cache->saved_regs[FT32_PC_REGNUM] = cache->framesize + 4;
	  cache->saved_regs[FT32_FP_REGNUM] = 0;
	  cache->framesize += FT32_LINK_SIZE (inst);
	  next_addr += isize;
	}
    }

  return next_addr;
}

/* Find the end of function prologue.  */

static CORE_ADDR
ft32_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
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
	  struct ft32_frame_cache cache;
	  CORE_ADDR plg_end;

	  memset (&cache, 0, sizeof cache);

	  plg_end = ft32_analyze_prologue (func_addr,
					   func_end, &cache, gdbarch);
	  /* Found a function.  */
	  sym = lookup_symbol (func_name, NULL, VAR_DOMAIN, NULL).symbol;
	  /* Don't use line number debug info for assembly source files.  */
	  if ((sym != NULL) && sym->language () != language_asm)
	    {
	      sal = find_pc_line (func_addr, 0);
	      if (sal.end && sal.end < func_end)
		{
		  /* Found a line number, use it as end of prologue.  */
		  return sal.end;
		}
	    }
	  /* No useable line symbol.  Use result of prologue parsing method.  */
	  return plg_end;
	}
    }

  /* No function symbol -- just return the PC.  */
  return pc;
}

/* Implementation of `pointer_to_address' gdbarch method.

   On FT32 address space zero is RAM, address space 1 is flash.
   RAM appears at address RAM_BIAS, flash at address 0.  */

static CORE_ADDR
ft32_pointer_to_address (struct gdbarch *gdbarch,
			 struct type *type, const gdb_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR addr
    = extract_unsigned_integer (buf, type->length (), byte_order);

  if (TYPE_ADDRESS_CLASS_1 (type))
    return addr;
  else
    return addr | RAM_BIAS;
}

/* Implementation of `address_class_type_flags' gdbarch method.

   This method maps DW_AT_address_class attributes to a
   type_instance_flag_value.  */

static type_instance_flags
ft32_address_class_type_flags (int byte_size, int dwarf2_addr_class)
{
  /* The value 1 of the DW_AT_address_class attribute corresponds to the
     __flash__ qualifier, meaning pointer to data in FT32 program memory.
   */
  if (dwarf2_addr_class == 1)
    return TYPE_INSTANCE_FLAG_ADDRESS_CLASS_1;
  return 0;
}

/* Implementation of `address_class_type_flags_to_name' gdbarch method.

   Convert a type_instance_flag_value to an address space qualifier.  */

static const char*
ft32_address_class_type_flags_to_name (struct gdbarch *gdbarch,
				       type_instance_flags type_flags)
{
  if (type_flags & TYPE_INSTANCE_FLAG_ADDRESS_CLASS_1)
    return "flash";
  else
    return NULL;
}

/* Implementation of `address_class_name_to_type_flags' gdbarch method.

   Convert an address space qualifier to a type_instance_flag_value.  */

static bool
ft32_address_class_name_to_type_flags (struct gdbarch *gdbarch,
				       const char* name,
				       type_instance_flags *type_flags_ptr)
{
  if (strcmp (name, "flash") == 0)
    {
      *type_flags_ptr = TYPE_INSTANCE_FLAG_ADDRESS_CLASS_1;
      return true;
    }
  else
    return false;
}

/* Given a return value in `regbuf' with a type `valtype',
   extract and copy its value into `valbuf'.  */

static void
ft32_extract_return_value (struct type *type, struct regcache *regcache,
			   gdb_byte *dst)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  bfd_byte *valbuf = dst;
  int len = type->length ();
  ULONGEST tmp;

  /* By using store_unsigned_integer we avoid having to do
     anything special for small big-endian values.  */
  regcache_cooked_read_unsigned (regcache, FT32_R0_REGNUM, &tmp);
  store_unsigned_integer (valbuf, (len > 4 ? len - 4 : len), byte_order, tmp);

  /* Ignore return values more than 8 bytes in size because the ft32
     returns anything more than 8 bytes in the stack.  */
  if (len > 4)
    {
      regcache_cooked_read_unsigned (regcache, FT32_R1_REGNUM, &tmp);
      store_unsigned_integer (valbuf + len - 4, 4, byte_order, tmp);
    }
}

/* Implement the "return_value" gdbarch method.  */

static enum return_value_convention
ft32_return_value (struct gdbarch *gdbarch, struct value *function,
		   struct type *valtype, struct regcache *regcache,
		   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (valtype->length () > 8)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else
    {
      if (readbuf != NULL)
	ft32_extract_return_value (valtype, regcache, readbuf);
      if (writebuf != NULL)
	ft32_store_return_value (valtype, regcache, writebuf);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

/* Allocate and initialize a ft32_frame_cache object.  */

static struct ft32_frame_cache *
ft32_alloc_frame_cache (void)
{
  struct ft32_frame_cache *cache;
  int i;

  cache = FRAME_OBSTACK_ZALLOC (struct ft32_frame_cache);

  for (i = 0; i < FT32_NUM_REGS; ++i)
    cache->saved_regs[i] = REG_UNAVAIL;

  return cache;
}

/* Populate a ft32_frame_cache object for this_frame.  */

static struct ft32_frame_cache *
ft32_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct ft32_frame_cache *cache;
  CORE_ADDR current_pc;
  int i;

  if (*this_cache)
    return (struct ft32_frame_cache *) *this_cache;

  cache = ft32_alloc_frame_cache ();
  *this_cache = cache;

  cache->base = get_frame_register_unsigned (this_frame, FT32_FP_REGNUM);
  if (cache->base == 0)
    return cache;

  cache->pc = get_frame_func (this_frame);
  current_pc = get_frame_pc (this_frame);
  if (cache->pc)
    {
      struct gdbarch *gdbarch = get_frame_arch (this_frame);

      ft32_analyze_prologue (cache->pc, current_pc, cache, gdbarch);
      if (!cache->established)
	cache->base = get_frame_register_unsigned (this_frame, FT32_SP_REGNUM);
    }

  cache->saved_sp = cache->base - 4;

  for (i = 0; i < FT32_NUM_REGS; ++i)
    if (cache->saved_regs[i] != REG_UNAVAIL)
      cache->saved_regs[i] = cache->base + cache->saved_regs[i];

  return cache;
}

/* Given a GDB frame, determine the address of the calling function's
   frame.  This will be used to create a new GDB frame struct.  */

static void
ft32_frame_this_id (frame_info_ptr this_frame,
		    void **this_prologue_cache, struct frame_id *this_id)
{
  struct ft32_frame_cache *cache = ft32_frame_cache (this_frame,
						     this_prologue_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  *this_id = frame_id_build (cache->saved_sp, cache->pc);
}

/* Get the value of register regnum in the previous stack frame.  */

static struct value *
ft32_frame_prev_register (frame_info_ptr this_frame,
			  void **this_prologue_cache, int regnum)
{
  struct ft32_frame_cache *cache = ft32_frame_cache (this_frame,
						     this_prologue_cache);

  gdb_assert (regnum >= 0);

  if (regnum == FT32_SP_REGNUM && cache->saved_sp)
    return frame_unwind_got_constant (this_frame, regnum, cache->saved_sp);

  if (regnum < FT32_NUM_REGS && cache->saved_regs[regnum] != REG_UNAVAIL)
      return frame_unwind_got_memory (this_frame, regnum,
				      RAM_BIAS | cache->saved_regs[regnum]);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind ft32_frame_unwind =
{
  "ft32 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  ft32_frame_this_id,
  ft32_frame_prev_register,
  NULL,
  default_frame_sniffer
};

/* Return the base address of this_frame.  */

static CORE_ADDR
ft32_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct ft32_frame_cache *cache = ft32_frame_cache (this_frame,
						     this_cache);

  return cache->base;
}

static const struct frame_base ft32_frame_base =
{
  &ft32_frame_unwind,
  ft32_frame_base_address,
  ft32_frame_base_address,
  ft32_frame_base_address
};

/* Allocate and initialize the ft32 gdbarch object.  */

static struct gdbarch *
ft32_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct type *void_type;
  struct type *func_void_type;

  /* If there is already a candidate, use it.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* Allocate space for the new architecture.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new ft32_gdbarch_tdep));
  ft32_gdbarch_tdep *tdep = gdbarch_tdep<ft32_gdbarch_tdep> (gdbarch);

  /* Create a type for PC.  We can't use builtin types here, as they may not
     be defined.  */
  type_allocator alloc (gdbarch);
  void_type = alloc.new_type (TYPE_CODE_VOID, TARGET_CHAR_BIT, "void");
  func_void_type = make_function_type (void_type, NULL);
  tdep->pc_type = init_pointer_type (alloc, 4 * TARGET_CHAR_BIT, NULL,
				     func_void_type);
  tdep->pc_type->set_instance_flags (tdep->pc_type->instance_flags ()
				     | TYPE_INSTANCE_FLAG_ADDRESS_CLASS_1);

  set_gdbarch_num_regs (gdbarch, FT32_NUM_REGS);
  set_gdbarch_sp_regnum (gdbarch, FT32_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, FT32_PC_REGNUM);
  set_gdbarch_register_name (gdbarch, ft32_register_name);
  set_gdbarch_register_type (gdbarch, ft32_register_type);

  set_gdbarch_return_value (gdbarch, ft32_return_value);

  set_gdbarch_pointer_to_address (gdbarch, ft32_pointer_to_address);

  set_gdbarch_skip_prologue (gdbarch, ft32_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, ft32_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, ft32_breakpoint::bp_from_kind);
  set_gdbarch_frame_align (gdbarch, ft32_frame_align);

  frame_base_set_default (gdbarch, &ft32_frame_base);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  /* Hook in the default unwinders.  */
  frame_unwind_append_unwinder (gdbarch, &ft32_frame_unwind);

  /* Support simple overlay manager.  */
  set_gdbarch_overlay_update (gdbarch, simple_overlay_update);

  set_gdbarch_address_class_type_flags (gdbarch, ft32_address_class_type_flags);
  set_gdbarch_address_class_name_to_type_flags
    (gdbarch, ft32_address_class_name_to_type_flags);
  set_gdbarch_address_class_type_flags_to_name
    (gdbarch, ft32_address_class_type_flags_to_name);

  return gdbarch;
}

/* Register this machine's init routine.  */

void _initialize_ft32_tdep ();
void
_initialize_ft32_tdep ()
{
  gdbarch_register (bfd_arch_ft32, ft32_gdbarch_init);
}
