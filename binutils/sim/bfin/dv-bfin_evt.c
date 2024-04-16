/* Blackfin Event Vector Table (EVT) model.

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
#include "devices.h"
#include "dv-bfin_cec.h"
#include "dv-bfin_evt.h"

struct bfin_evt
{
  bu32 base;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 evt[16];
};
#define mmr_base()      offsetof(struct bfin_evt, evt[0])
#define mmr_offset(mmr) (offsetof(struct bfin_evt, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "EVT0", "EVT1", "EVT2", "EVT3", "EVT4", "EVT5", "EVT6", "EVT7", "EVT8",
  "EVT9", "EVT10", "EVT11", "EVT12", "EVT13", "EVT14", "EVT15",
};
#define mmr_name(off) mmr_names[(off) / 4]

static unsigned
bfin_evt_io_write_buffer (struct hw *me, const void *source,
			  int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_evt *evt = hw_data (me);
  bu32 mmr_off;
  bu32 value;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
    return 0;

  value = dv_load_4 (source);
  mmr_off = addr - evt->base;

  HW_TRACE_WRITE ();

  evt->evt[mmr_off / 4] = value;

  return nr_bytes;
}

static unsigned
bfin_evt_io_read_buffer (struct hw *me, void *dest,
			 int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_evt *evt = hw_data (me);
  bu32 mmr_off;
  bu32 value;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - evt->base;

  HW_TRACE_READ ();

  value = evt->evt[mmr_off / 4];

  dv_store_4 (dest, value);

  return nr_bytes;
}

static void
attach_bfin_evt_regs (struct hw *me, struct bfin_evt *evt)
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

  if (attach_size != BFIN_COREMMR_EVT_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_COREMMR_EVT_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  evt->base = attach_address;
}

static void
bfin_evt_finish (struct hw *me)
{
  struct bfin_evt *evt;

  evt = HW_ZALLOC (me, struct bfin_evt);

  set_hw_data (me, evt);
  set_hw_io_read_buffer (me, bfin_evt_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_evt_io_write_buffer);

  attach_bfin_evt_regs (me, evt);
}

const struct hw_descriptor dv_bfin_evt_descriptor[] =
{
  {"bfin_evt", bfin_evt_finish,},
  {NULL, NULL},
};

#define EVT_STATE(cpu) DV_STATE_CACHED (cpu, evt)

void
cec_set_evt (SIM_CPU *cpu, int ivg, bu32 handler_addr)
{
  if (ivg > IVG15 || ivg < 0)
    sim_io_error (CPU_STATE (cpu), "%s: ivg %i out of range !", __func__, ivg);

  EVT_STATE (cpu)->evt[ivg] = handler_addr;
}

bu32
cec_get_evt (SIM_CPU *cpu, int ivg)
{
  if (ivg > IVG15 || ivg < 0)
    sim_io_error (CPU_STATE (cpu), "%s: ivg %i out of range !", __func__, ivg);

  return EVT_STATE (cpu)->evt[ivg];
}

bu32
cec_get_reset_evt (SIM_CPU *cpu)
{
  /* XXX: This should tail into the model to get via BMODE pins.  */
  return 0xef000000;
}
