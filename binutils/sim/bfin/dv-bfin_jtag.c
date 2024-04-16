/* Blackfin JTAG model.

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
#include "dv-bfin_jtag.h"

/* XXX: This is mostly a stub.  There are more registers, but they're only
        accessible via the JTAG scan chain and not the MMR interface.  */

struct bfin_jtag
{
  bu32 base;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 dspid;
  bu32 _pad0;
  bu32 dbgstat;
};
#define mmr_base()      offsetof(struct bfin_jtag, dspid)
#define mmr_offset(mmr) (offsetof(struct bfin_jtag, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "DSPID", NULL, "DBGSTAT",
};
#define mmr_name(off) (mmr_names[(off) / 4] ? : "<INV>")

static unsigned
bfin_jtag_io_write_buffer (struct hw *me, const void *source, int space,
			   address_word addr, unsigned nr_bytes)
{
  struct bfin_jtag *jtag = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu32 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
    return 0;

  value = dv_load_4 (source);
  mmr_off = addr - jtag->base;
  valuep = (void *)((uintptr_t)jtag + mmr_base() + mmr_off);

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(dbgstat):
      dv_w1c_4 (valuep, value, 0xc);
      break;
    case mmr_offset(dspid):
      /* Discard writes to these.  */
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_jtag_io_read_buffer (struct hw *me, void *dest, int space,
			  address_word addr, unsigned nr_bytes)
{
  struct bfin_jtag *jtag = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu32 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - jtag->base;
  valuep = (void *)((uintptr_t)jtag + mmr_base() + mmr_off);

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(dbgstat):
    case mmr_offset(dspid):
      value = *valuep;
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  dv_store_4 (dest, value);

  return nr_bytes;
}

static void
attach_bfin_jtag_regs (struct hw *me, struct bfin_jtag *jtag)
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

  if (attach_size != BFIN_COREMMR_JTAG_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_COREMMR_JTAG_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  jtag->base = attach_address;
}

static void
bfin_jtag_finish (struct hw *me)
{
  struct bfin_jtag *jtag;

  jtag = HW_ZALLOC (me, struct bfin_jtag);

  set_hw_data (me, jtag);
  set_hw_io_read_buffer (me, bfin_jtag_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_jtag_io_write_buffer);

  attach_bfin_jtag_regs (me, jtag);

  /* Initialize the JTAG state.  */
  jtag->dspid = bfin_model_get_dspid (hw_system (me));
}

const struct hw_descriptor dv_bfin_jtag_descriptor[] =
{
  {"bfin_jtag", bfin_jtag_finish,},
  {NULL, NULL},
};
