/* Handle cache related addresses.

   Copyright (C) 1996-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions and Mike Frysinger.

   This file is part of the GNU simulators.

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

/* This must come before any other includes.  */
#include "defs.h"

#include "sim-main.h"
#include "hw-main.h"

#include "dv-m32r_cache.h"

struct m32r_cache_hw
{
};

static unsigned
cris_io_write_buffer (struct hw *me, const void *source,
		      int space, address_word addr, unsigned nr_bytes)
{
  SIM_DESC sd = hw_system (me);

#if WITH_SCACHE
  /* MSPR support is deprecated but is kept in for upward compatibility
     with existing overlay support.  */
  switch (addr)
    {
    case MSPR_ADDR:
      if ((*(const char *) source & MSPR_PURGE) != 0)
	scache_flush (sd);
      break;

    case MCCR_ADDR:
      if ((*(const char *) source & MCCR_CP) != 0)
	scache_flush (sd);
      break;
    }
#endif

  return nr_bytes;
}

static void
attach_regs (struct hw *me, struct m32r_cache_hw *hw)
{
  address_word attach_address;
  int attach_space;
  unsigned attach_size;
  reg_property_spec reg;

  if (hw_find_property (me, "reg") == NULL)
    hw_abort (me, "Missing \"reg\" property");

  if (!hw_find_reg_array_property (me, "reg", 0, &reg))
    hw_abort (me, "\"reg\" property must contain three addr/size entries");

  hw_unit_address_to_attach_address (hw_parent (me),
				     &reg.address,
				     &attach_space, &attach_address, me);
  hw_unit_size_to_attach_size (hw_parent (me), &reg.size, &attach_size, me);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);
}

static void
m32r_cache_finish (struct hw *me)
{
  struct m32r_cache_hw *hw;

  hw = HW_ZALLOC (me, struct m32r_cache_hw);
  set_hw_data (me, hw);
  set_hw_io_write_buffer (me, cris_io_write_buffer);

  attach_regs (me, hw);
}

const struct hw_descriptor dv_m32r_cache_descriptor[] = {
  { "m32r_cache", m32r_cache_finish, },
  { NULL },
};
