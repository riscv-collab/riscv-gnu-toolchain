/* Traditional frame unwind support, for GDB the GNU Debugger.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "trad-frame.h"
#include "regcache.h"
#include "frame-unwind.h"
#include "target.h"
#include "value.h"
#include "gdbarch.h"
#include "gdbsupport/traits.h"

struct trad_frame_cache
{
  frame_info_ptr this_frame;
  CORE_ADDR this_base;
  trad_frame_saved_reg *prev_regs;
  struct frame_id this_id;
};

struct trad_frame_cache *
trad_frame_cache_zalloc (frame_info_ptr this_frame)
{
  struct trad_frame_cache *this_trad_cache;

  this_trad_cache = FRAME_OBSTACK_ZALLOC (struct trad_frame_cache);
  this_trad_cache->prev_regs = trad_frame_alloc_saved_regs (this_frame);
  this_trad_cache->this_frame = this_frame;
  return this_trad_cache;
}

/* See trad-frame.h.  */

void
trad_frame_reset_saved_regs (struct gdbarch *gdbarch,
			     trad_frame_saved_reg *regs)
{
  int numregs = gdbarch_num_cooked_regs (gdbarch);

  for (int regnum = 0; regnum < numregs; regnum++)
    regs[regnum].set_realreg (regnum);
}

trad_frame_saved_reg *
trad_frame_alloc_saved_regs (struct gdbarch *gdbarch)
{
#ifdef HAVE_IS_TRIVIALLY_CONSTRUCTIBLE
  static_assert (std::is_trivially_constructible<trad_frame_saved_reg>::value);
#endif

  int numregs = gdbarch_num_cooked_regs (gdbarch);
  trad_frame_saved_reg *this_saved_regs
    = FRAME_OBSTACK_CALLOC (numregs, trad_frame_saved_reg);

  /* For backwards compatibility, initialize all the register values to
     REALREG, with register 0 stored in 0, register 1 stored in 1 and so
     on.  */
  trad_frame_reset_saved_regs (gdbarch, this_saved_regs);

  return this_saved_regs;
}

/* A traditional frame is unwound by analysing the function prologue
   and using the information gathered to track registers.  For
   non-optimized frames, the technique is reliable (just need to check
   for all potential instruction sequences).  */

trad_frame_saved_reg *
trad_frame_alloc_saved_regs (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);

  return trad_frame_alloc_saved_regs (gdbarch);
}

void
trad_frame_set_reg_value (struct trad_frame_cache *this_trad_cache,
			  int regnum, LONGEST val)
{
  /* External interface for users of trad_frame_cache
     (who cannot access the prev_regs object directly).  */
  this_trad_cache->prev_regs[regnum].set_value (val);
}

void
trad_frame_set_reg_realreg (struct trad_frame_cache *this_trad_cache,
			    int regnum, int realreg)
{
  this_trad_cache->prev_regs[regnum].set_realreg (realreg);
}

void
trad_frame_set_reg_addr (struct trad_frame_cache *this_trad_cache,
			 int regnum, CORE_ADDR addr)
{
  this_trad_cache->prev_regs[regnum].set_addr (addr);
}

void
trad_frame_set_reg_regmap (struct trad_frame_cache *this_trad_cache,
			   const struct regcache_map_entry *regmap,
			   CORE_ADDR addr, size_t size)
{
  struct gdbarch *gdbarch = get_frame_arch (this_trad_cache->this_frame);
  int offs = 0, count;

  for (; (count = regmap->count) != 0; regmap++)
    {
      int regno = regmap->regno;
      int slot_size = regmap->size;

      if (slot_size == 0 && regno != REGCACHE_MAP_SKIP)
	slot_size = register_size (gdbarch, regno);

      if (offs + slot_size > size)
	break;

      if (regno == REGCACHE_MAP_SKIP)
	offs += count * slot_size;
      else
	for (; count--; regno++, offs += slot_size)
	  {
	    /* Mimic the semantics of regcache::transfer_regset if a
	       register slot's size does not match the size of a
	       register.

	       If a register slot is larger than a register, assume
	       the register's value is stored in the first N bytes of
	       the slot and ignore the remaining bytes.

	       If the register slot is smaller than the register,
	       assume that the slot contains the low N bytes of the
	       register's value.  Since trad_frame assumes that
	       registers stored by address are sized according to the
	       register, read the low N bytes and zero-extend them to
	       generate a register value.  */
	    if (slot_size >= register_size (gdbarch, regno))
	      trad_frame_set_reg_addr (this_trad_cache, regno, addr + offs);
	    else
	      {
		enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
		gdb_byte buf[slot_size];

		if (target_read_memory (addr + offs, buf, sizeof buf) == 0)
		  {
		    LONGEST val
		      = extract_unsigned_integer (buf, sizeof buf, byte_order);
		    trad_frame_set_reg_value (this_trad_cache, regno, val);
		  }
	      }
	  }
    }
}

/* See trad-frame.h.  */

void
trad_frame_set_reg_value_bytes (struct trad_frame_cache *this_trad_cache,
				int regnum,
				gdb::array_view<const gdb_byte> bytes)
{
  /* External interface for users of trad_frame_cache
     (who cannot access the prev_regs object directly).  */
  this_trad_cache->prev_regs[regnum].set_value_bytes (bytes);
}



struct value *
trad_frame_get_prev_register (frame_info_ptr this_frame,
			      trad_frame_saved_reg this_saved_regs[],
			      int regnum)
{
  if (this_saved_regs[regnum].is_addr ())
    /* The register was saved in memory.  */
    return frame_unwind_got_memory (this_frame, regnum,
				    this_saved_regs[regnum].addr ());
  else if (this_saved_regs[regnum].is_realreg ())
    return frame_unwind_got_register (this_frame, regnum,
				      this_saved_regs[regnum].realreg ());
  else if (this_saved_regs[regnum].is_value ())
    /* The register's value is available.  */
    return frame_unwind_got_constant (this_frame, regnum,
				      this_saved_regs[regnum].value ());
  else if (this_saved_regs[regnum].is_value_bytes ())
    /* The register's value is available as a sequence of bytes.  */
    return frame_unwind_got_bytes (this_frame, regnum,
				   this_saved_regs[regnum].value_bytes ());
  else
    return frame_unwind_got_optimized (this_frame, regnum);
}

struct value *
trad_frame_get_register (struct trad_frame_cache *this_trad_cache,
			 frame_info_ptr this_frame,
			 int regnum)
{
  return trad_frame_get_prev_register (this_frame, this_trad_cache->prev_regs,
				       regnum);
}

void
trad_frame_set_id (struct trad_frame_cache *this_trad_cache,
		   struct frame_id this_id)
{
  this_trad_cache->this_id = this_id;
}

void
trad_frame_get_id (struct trad_frame_cache *this_trad_cache,
		   struct frame_id *this_id)
{
  (*this_id) = this_trad_cache->this_id;
}

void
trad_frame_set_this_base (struct trad_frame_cache *this_trad_cache,
			  CORE_ADDR this_base)
{
  this_trad_cache->this_base = this_base;
}

CORE_ADDR
trad_frame_get_this_base (struct trad_frame_cache *this_trad_cache)
{
  return this_trad_cache->this_base;
}
