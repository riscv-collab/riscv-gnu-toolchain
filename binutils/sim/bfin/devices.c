/* Blackfin device support.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

   This file is part of simulators.

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
#include "sim-hw.h"
#include "hw-device.h"
#include "devices.h"
#include "dv-bfin_cec.h"
#include "dv-bfin_mmu.h"

static void
bfin_mmr_invalid (struct hw *me, address_word addr,
		  unsigned nr_bytes, bool write, bool missing)
{
  SIM_CPU *cpu = hw_system_cpu (me);
  const char *rw = write ? "write" : "read";
  const char *reason =
    missing ? "no such register" :
              (addr & 3) ? "must be 32-bit aligned" : "invalid length";

  /* Only throw a fit if the cpu is doing the access.  DMA/GDB simply
     go unnoticed.  Not exactly hardware behavior, but close enough.  */
  if (!cpu)
    {
      sim_io_eprintf (hw_system (me),
		      "%s: invalid MMR %s at %#x length %u: %s\n",
		      hw_path (me), rw, addr, nr_bytes, reason);
      return;
    }

  HW_TRACE ((me, "invalid MMR %s at %#x length %u: %s",
	     rw, addr, nr_bytes, reason));

  /* XXX: is this what hardware does ?  What about priority of unaligned vs
     wrong length vs missing register ?  What about system-vs-core ?  */
  /* XXX: We should move this addr check to a model property so we get the
     same behavior regardless of where we map the model.  */
  if (addr >= BFIN_CORE_MMR_BASE)
    /* XXX: This should be setting up CPLB fault addrs ?  */
    mmu_process_fault (cpu, addr, write, false, false, true);
  else
    /* XXX: Newer parts set up an interrupt from EBIU and program
            EBIU_ERRADDR with the address.  */
    cec_hwerr (cpu, HWERR_SYSTEM_MMR);
}

void
dv_bfin_mmr_invalid (struct hw *me, address_word addr, unsigned nr_bytes,
		     bool write)
{
  bfin_mmr_invalid (me, addr, nr_bytes, write, true);
}

bool
dv_bfin_mmr_require (struct hw *me, address_word addr, unsigned nr_bytes,
		     unsigned size, bool write)
{
  if ((addr & 0x3) == 0 && nr_bytes == size)
    return true;

  bfin_mmr_invalid (me, addr, nr_bytes, write, false);
  return false;
}

/* For 32-bit memory mapped registers that allow 16-bit or 32-bit access.  */
bool
dv_bfin_mmr_require_16_32 (struct hw *me, address_word addr, unsigned nr_bytes,
			   bool write)
{
  if ((addr & 0x3) == 0 && (nr_bytes == 2 || nr_bytes == 4))
    return true;

  bfin_mmr_invalid (me, addr, nr_bytes, write, false);
  return false;
}

unsigned int dv_get_bus_num (struct hw *me)
{
  const hw_unit *unit = hw_unit_address (me);
  return unit->cells[unit->nr_cells - 1];
}
