/* Handle 0x900000xx addresses in the sim.

   Copyright (C) 2004-2024 Free Software Foundation, Inc.
   Contributed by Axis Communications.

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

struct cris_900000xx_hw
{
};

static unsigned
cris_io_write_buffer (struct hw *me, const void *source,
		      int space, address_word addr, unsigned nr_bytes)
{
  static const unsigned char ok[] = { 4, 0, 0, 0x90};
  static const unsigned char bad[] = { 8, 0, 0, 0x90};
  SIM_CPU *cpu = hw_system_cpu (me);
  SIM_DESC sd = CPU_STATE (cpu);
  sim_cia cia = CPU_PC_GET (cpu);

  if (addr == 0x90000004 && memcmp (source, ok, sizeof ok) == 0)
    return cris_break_13_handler (cpu, 1, 0, 0, 0, 0, 0, 0, cia);
  else if (addr == 0x90000008
	   && memcmp (source, bad, sizeof bad) == 0)
    return cris_break_13_handler (cpu, 1, 34, 0, 0, 0, 0, 0, cia);

  /* If it wasn't one of those, send an invalid-memory signal.  */
  sim_core_signal (sd, cpu, cia, 0, nr_bytes, addr,
		   write_transfer, sim_core_unmapped_signal);

  return 0;
}

/* Instance initializer function.  */

static void
attach_regs (struct hw *me, struct cris_900000xx_hw *hw)
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
cris_900000xx_finish (struct hw *me)
{
  struct cris_900000xx_hw *hw;

  hw = HW_ZALLOC (me, struct cris_900000xx_hw);
  set_hw_data (me, hw);
  set_hw_io_write_buffer (me, cris_io_write_buffer);

  attach_regs (me, hw);
}

const struct hw_descriptor dv_cris_900000xx_descriptor[] = {
  { "cris_900000xx", cris_900000xx_finish, },
  { NULL },
};
