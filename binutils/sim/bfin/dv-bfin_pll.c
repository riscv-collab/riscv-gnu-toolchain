/* Blackfin Phase Lock Loop (PLL) model.

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
#include "dv-bfin_pll.h"

struct bfin_pll
{
  bu32 base;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(pll_ctl);
  bu16 BFIN_MMR_16(pll_div);
  bu16 BFIN_MMR_16(vr_ctl);
  bu16 BFIN_MMR_16(pll_stat);
  bu16 BFIN_MMR_16(pll_lockcnt);

  /* XXX: Not really the best place for this ...  */
  bu32 chipid;
};
#define mmr_base()      offsetof(struct bfin_pll, pll_ctl)
#define mmr_offset(mmr) (offsetof(struct bfin_pll, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "PLL_CTL", "PLL_DIV", "VR_CTL", "PLL_STAT", "PLL_LOCKCNT", "CHIPID",
};
#define mmr_name(off) mmr_names[(off) / 4]

static unsigned
bfin_pll_io_write_buffer (struct hw *me, const void *source,
			  int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_pll *pll = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu16 *value16p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, true))
    return 0;

  if (nr_bytes == 4)
    value = dv_load_4 (source);
  else
    value = dv_load_2 (source);

  mmr_off = addr - pll->base;
  valuep = (void *)((uintptr_t)pll + mmr_base() + mmr_off);
  value16p = valuep;

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(pll_stat):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
    case mmr_offset(chipid):
      /* Discard writes.  */
      break;
    default:
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      *value16p = value;
      break;
    }

  return nr_bytes;
}

static unsigned
bfin_pll_io_read_buffer (struct hw *me, void *dest,
			 int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_pll *pll = hw_data (me);
  bu32 mmr_off;
  bu32 *value32p;
  bu16 *value16p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - pll->base;
  valuep = (void *)((uintptr_t)pll + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(chipid):
      dv_store_4 (dest, *value32p);
      break;
    default:
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
	return 0;
      dv_store_2 (dest, *value16p);
      break;
    }

  return nr_bytes;
}

static const struct hw_port_descriptor bfin_pll_ports[] =
{
  { "pll", 0, 0, output_port, },
  { NULL, 0, 0, 0, },
};

static void
attach_bfin_pll_regs (struct hw *me, struct bfin_pll *pll)
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

  if (attach_size != BFIN_MMR_PLL_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_PLL_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  pll->base = attach_address;
}

static void
bfin_pll_finish (struct hw *me)
{
  struct bfin_pll *pll;

  pll = HW_ZALLOC (me, struct bfin_pll);

  set_hw_data (me, pll);
  set_hw_io_read_buffer (me, bfin_pll_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_pll_io_write_buffer);
  set_hw_ports (me, bfin_pll_ports);

  attach_bfin_pll_regs (me, pll);

  /* Initialize the PLL.  */
  /* XXX: Depends on part ?  */
  pll->pll_ctl = 0x1400;
  pll->pll_div = 0x0005;
  pll->vr_ctl = 0x40DB;
  pll->pll_stat = 0x00A2;
  pll->pll_lockcnt = 0x0200;
  pll->chipid = bfin_model_get_chipid (hw_system (me));

  /* XXX: slow it down!  */
  pll->pll_ctl = 0xa800;
  pll->pll_div = 0x4;
  pll->vr_ctl = 0x40fb;
  pll->pll_stat = 0xa2;
  pll->pll_lockcnt = 0x300;
}

const struct hw_descriptor dv_bfin_pll_descriptor[] =
{
  {"bfin_pll", bfin_pll_finish,},
  {NULL, NULL},
};
