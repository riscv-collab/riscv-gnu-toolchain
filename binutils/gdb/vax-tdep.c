/* Target-dependent code for the VAX.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include "frame.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "gdbcore.h"
#include "gdbtypes.h"
#include "osabi.h"
#include "regcache.h"
#include "regset.h"
#include "trad-frame.h"
#include "value.h"

#include "vax-tdep.h"

/* Return the name of register REGNUM.  */

static const char *
vax_register_name (struct gdbarch *gdbarch, int regnum)
{
  static const char *register_names[] =
  {
    "r0", "r1", "r2",  "r3",  "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "ap", "fp", "sp", "pc",
    "ps",
  };

  static_assert (VAX_NUM_REGS == ARRAY_SIZE (register_names));
  return register_names[regnum];
}

/* Return the GDB type object for the "standard" data type of data in
   register REGNUM.  */

static struct type *
vax_register_type (struct gdbarch *gdbarch, int regnum)
{
  return builtin_type (gdbarch)->builtin_int;
}

/* Core file support.  */

/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
vax_supply_gregset (const struct regset *regset, struct regcache *regcache,
		    int regnum, const void *gregs, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) gregs;
  int i;

  for (i = 0; i < VAX_NUM_REGS; i++)
    {
      if (regnum == i || regnum == -1)
	regcache->raw_supply (i, regs + i * 4);
    }
}

/* VAX register set.  */

static const struct regset vax_gregset =
{
  NULL,
  vax_supply_gregset
};

/* Iterate over core file register note sections.  */

static void
vax_iterate_over_regset_sections (struct gdbarch *gdbarch,
				  iterate_over_regset_sections_cb *cb,
				  void *cb_data,
				  const struct regcache *regcache)
{
  cb (".reg", VAX_NUM_REGS * 4, VAX_NUM_REGS * 4, &vax_gregset, NULL, cb_data);
}

/* The VAX UNIX calling convention uses R1 to pass a structure return
   value address instead of passing it as a first (hidden) argument as
   the VMS calling convention suggests.  */

static CORE_ADDR
vax_store_arguments (struct regcache *regcache, int nargs,
		     struct value **args, CORE_ADDR sp)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[4];
  int count = 0;
  int i;

  /* We create an argument list on the stack, and make the argument
     pointer to it.  */

  /* Push arguments in reverse order.  */
  for (i = nargs - 1; i >= 0; i--)
    {
      int len = args[i]->enclosing_type ()->length ();

      sp -= (len + 3) & ~3;
      count += (len + 3) / 4;
      write_memory (sp, args[i]->contents_all ().data (), len);
    }

  /* Push argument count.  */
  sp -= 4;
  store_unsigned_integer (buf, 4, byte_order, count);
  write_memory (sp, buf, 4);

  /* Update the argument pointer.  */
  store_unsigned_integer (buf, 4, byte_order, sp);
  regcache->cooked_write (VAX_AP_REGNUM, buf);

  return sp;
}

static CORE_ADDR
vax_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		     struct regcache *regcache, CORE_ADDR bp_addr, int nargs,
		     struct value **args, CORE_ADDR sp,
		     function_call_return_method return_method,
		     CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR fp = sp;
  gdb_byte buf[4];

  /* Set up the function arguments.  */
  sp = vax_store_arguments (regcache, nargs, args, sp);

  /* Store return value address.  */
  if (return_method == return_method_struct)
    regcache_cooked_write_unsigned (regcache, VAX_R1_REGNUM, struct_addr);

  /* Store return address in the PC slot.  */
  sp -= 4;
  store_unsigned_integer (buf, 4, byte_order, bp_addr);
  write_memory (sp, buf, 4);

  /* Store the (fake) frame pointer in the FP slot.  */
  sp -= 4;
  store_unsigned_integer (buf, 4, byte_order, fp);
  write_memory (sp, buf, 4);

  /* Skip the AP slot.  */
  sp -= 4;

  /* Store register save mask and control bits.  */
  sp -= 4;
  store_unsigned_integer (buf, 4, byte_order, 0);
  write_memory (sp, buf, 4);

  /* Store condition handler.  */
  sp -= 4;
  store_unsigned_integer (buf, 4, byte_order, 0);
  write_memory (sp, buf, 4);

  /* Update the stack pointer and frame pointer.  */
  store_unsigned_integer (buf, 4, byte_order, sp);
  regcache->cooked_write (VAX_SP_REGNUM, buf);
  regcache->cooked_write (VAX_FP_REGNUM, buf);

  /* Return the saved (fake) frame pointer.  */
  return fp;
}

static struct frame_id
vax_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame)
{
  CORE_ADDR fp;

  fp = get_frame_register_unsigned (this_frame, VAX_FP_REGNUM);
  return frame_id_build (fp, get_frame_pc (this_frame));
}


static enum return_value_convention
vax_return_value (struct gdbarch *gdbarch, struct value *function,
		  struct type *type, struct regcache *regcache,
		  gdb_byte *readbuf, const gdb_byte *writebuf)
{
  int len = type->length ();
  gdb_byte buf[8];

  if (type->code () == TYPE_CODE_STRUCT
      || type->code () == TYPE_CODE_UNION
      || type->code () == TYPE_CODE_ARRAY)
    {
      /* The default on VAX is to return structures in static memory.
	 Consequently a function must return the address where we can
	 find the return value.  */

      if (readbuf)
	{
	  ULONGEST addr;

	  regcache_raw_read_unsigned (regcache, VAX_R0_REGNUM, &addr);
	  read_memory (addr, readbuf, len);
	}

      return RETURN_VALUE_ABI_RETURNS_ADDRESS;
    }

  if (readbuf)
    {
      /* Read the contents of R0 and (if necessary) R1.  */
      regcache->cooked_read (VAX_R0_REGNUM, buf);
      if (len > 4)
	regcache->cooked_read (VAX_R1_REGNUM, buf + 4);
      memcpy (readbuf, buf, len);
    }
  if (writebuf)
    {
      /* Read the contents to R0 and (if necessary) R1.  */
      memcpy (buf, writebuf, len);
      regcache->cooked_write (VAX_R0_REGNUM, buf);
      if (len > 4)
	regcache->cooked_write (VAX_R1_REGNUM, buf + 4);
    }

  return RETURN_VALUE_REGISTER_CONVENTION;
}


/* Use the program counter to determine the contents and size of a
   breakpoint instruction.  Return a pointer to a string of bytes that
   encode a breakpoint instruction, store the length of the string in
   *LEN and optionally adjust *PC to point to the correct memory
   location for inserting the breakpoint.  */

constexpr gdb_byte vax_break_insn[] = { 3 };

typedef BP_MANIPULATION (vax_break_insn) vax_breakpoint;

/* Advance PC across any function entry prologue instructions
   to reach some "real" code.  */

static CORE_ADDR
vax_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte op = read_memory_unsigned_integer (pc, 1, byte_order);

  if (op == 0x11)
    pc += 2;			/* skip brb */
  if (op == 0x31)
    pc += 3;			/* skip brw */
  if (op == 0xC2
      && read_memory_unsigned_integer (pc + 2, 1, byte_order) == 0x5E)
    pc += 3;			/* skip subl2 */
  if (op == 0x9E
      && read_memory_unsigned_integer (pc + 1, 1, byte_order) == 0xAE
      && read_memory_unsigned_integer (pc + 3, 1, byte_order) == 0x5E)
    pc += 4;			/* skip movab */
  if (op == 0x9E
      && read_memory_unsigned_integer (pc + 1, 1, byte_order) == 0xCE
      && read_memory_unsigned_integer (pc + 4, 1, byte_order) == 0x5E)
    pc += 5;			/* skip movab */
  if (op == 0x9E
      && read_memory_unsigned_integer (pc + 1, 1, byte_order) == 0xEE
      && read_memory_unsigned_integer (pc + 6, 1, byte_order) == 0x5E)
    pc += 7;			/* skip movab */

  return pc;
}


/* Unwinding the stack is relatively easy since the VAX has a
   dedicated frame pointer, and frames are set up automatically as the
   result of a function call.  Most of the relevant information can be
   inferred from the documentation of the Procedure Call Instructions
   in the VAX MACRO and Instruction Set Reference Manual.  */

struct vax_frame_cache
{
  /* Base address.  */
  CORE_ADDR base;

  /* Table of saved registers.  */
  trad_frame_saved_reg *saved_regs;
};

static struct vax_frame_cache *
vax_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct vax_frame_cache *cache;
  CORE_ADDR addr;
  ULONGEST mask;
  int regnum;

  if (*this_cache)
    return (struct vax_frame_cache *) *this_cache;

  /* Allocate a new cache.  */
  cache = FRAME_OBSTACK_ZALLOC (struct vax_frame_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* The frame pointer is used as the base for the frame.  */
  cache->base = get_frame_register_unsigned (this_frame, VAX_FP_REGNUM);
  if (cache->base == 0)
    return cache;

  /* The register save mask and control bits determine the layout of
     the stack frame.  */
  mask = get_frame_memory_unsigned (this_frame, cache->base + 4, 4) >> 16;

  /* These are always saved.  */
  cache->saved_regs[VAX_PC_REGNUM].set_addr (cache->base + 16);
  cache->saved_regs[VAX_FP_REGNUM].set_addr (cache->base + 12);
  cache->saved_regs[VAX_AP_REGNUM].set_addr (cache->base + 8);
  cache->saved_regs[VAX_PS_REGNUM].set_addr (cache->base + 4);

  /* Scan the register save mask and record the location of the saved
     registers.  */
  addr = cache->base + 20;
  for (regnum = 0; regnum < VAX_AP_REGNUM; regnum++)
    {
      if (mask & (1 << regnum))
	{
	  cache->saved_regs[regnum].set_addr (addr);
	  addr += 4;
	}
    }

  /* The CALLS/CALLG flag determines whether this frame has a General
     Argument List or a Stack Argument List.  */
  if (mask & (1 << 13))
    {
      ULONGEST numarg;

      /* This is a procedure with Stack Argument List.  Adjust the
	 stack address for the arguments that were pushed onto the
	 stack.  The return instruction will automatically pop the
	 arguments from the stack.  */
      numarg = get_frame_memory_unsigned (this_frame, addr, 1);
      addr += 4 + numarg * 4;
    }

  /* Bits 1:0 of the stack pointer were saved in the control bits.  */
  cache->saved_regs[VAX_SP_REGNUM].set_value (addr + (mask >> 14));

  return cache;
}

static void
vax_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		   struct frame_id *this_id)
{
  struct vax_frame_cache *cache = vax_frame_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  (*this_id) = frame_id_build (cache->base, get_frame_func (this_frame));
}

static struct value *
vax_frame_prev_register (frame_info_ptr this_frame,
			 void **this_cache, int regnum)
{
  struct vax_frame_cache *cache = vax_frame_cache (this_frame, this_cache);

  return trad_frame_get_prev_register (this_frame, cache->saved_regs, regnum);
}

static const struct frame_unwind vax_frame_unwind =
{
  "vax prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  vax_frame_this_id,
  vax_frame_prev_register,
  NULL,
  default_frame_sniffer
};


static CORE_ADDR
vax_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct vax_frame_cache *cache = vax_frame_cache (this_frame, this_cache);

  return cache->base;
}

static CORE_ADDR
vax_frame_args_address (frame_info_ptr this_frame, void **this_cache)
{
  return get_frame_register_unsigned (this_frame, VAX_AP_REGNUM);
}

static const struct frame_base vax_frame_base =
{
  &vax_frame_unwind,
  vax_frame_base_address,
  vax_frame_base_address,
  vax_frame_args_address
};

/* Return number of arguments for FRAME.  */

static int
vax_frame_num_args (frame_info_ptr frame)
{
  CORE_ADDR args;

  /* Assume that the argument pointer for the outermost frame is
     hosed, as is the case on NetBSD/vax ELF.  */
  if (get_frame_base_address (frame) == 0)
    return 0;

  args = get_frame_register_unsigned (frame, VAX_AP_REGNUM);
  return get_frame_memory_unsigned (frame, args, 1);
}



/* Initialize the current architecture based on INFO.  If possible, re-use an
   architecture from ARCHES, which is a list of architectures already created
   during this debugging session.

   Called e.g. at program startup, when reading a core file, and when reading
   a binary file.  */

static struct gdbarch *
vax_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;

  /* If there is already a candidate, use it.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  gdbarch = gdbarch_alloc (&info, NULL);

  set_gdbarch_float_format (gdbarch, floatformats_vax_f);
  set_gdbarch_double_format (gdbarch, floatformats_vax_d);
  set_gdbarch_long_double_format (gdbarch, floatformats_vax_d);
  set_gdbarch_long_double_bit (gdbarch, 64);

  /* Register info */
  set_gdbarch_num_regs (gdbarch, VAX_NUM_REGS);
  set_gdbarch_register_name (gdbarch, vax_register_name);
  set_gdbarch_register_type (gdbarch, vax_register_type);
  set_gdbarch_sp_regnum (gdbarch, VAX_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, VAX_PC_REGNUM);
  set_gdbarch_ps_regnum (gdbarch, VAX_PS_REGNUM);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, vax_iterate_over_regset_sections);

  /* Frame and stack info */
  set_gdbarch_skip_prologue (gdbarch, vax_skip_prologue);
  set_gdbarch_frame_num_args (gdbarch, vax_frame_num_args);
  set_gdbarch_frame_args_skip (gdbarch, 4);

  /* Stack grows downward.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  /* Return value info */
  set_gdbarch_return_value (gdbarch, vax_return_value);

  /* Call dummy code.  */
  set_gdbarch_push_dummy_call (gdbarch, vax_push_dummy_call);
  set_gdbarch_dummy_id (gdbarch, vax_dummy_id);

  /* Breakpoint info */
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, vax_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, vax_breakpoint::bp_from_kind);

  /* Misc info */
  set_gdbarch_deprecated_function_start_offset (gdbarch, 2);
  set_gdbarch_believe_pcc_promotion (gdbarch, 1);

  frame_base_set_default (gdbarch, &vax_frame_base);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  frame_unwind_append_unwinder (gdbarch, &vax_frame_unwind);

  return (gdbarch);
}

void _initialize_vax_tdep ();
void
_initialize_vax_tdep ()
{
  gdbarch_register (bfd_arch_vax, vax_gdbarch_init, NULL);
}
