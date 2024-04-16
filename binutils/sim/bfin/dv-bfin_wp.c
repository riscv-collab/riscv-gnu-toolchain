/* Blackfin Watchpoint (WP) model.

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
#include "dv-bfin_wp.h"

/* XXX: This is mostly a stub.  */

#define WPI_NUM 6	/* 6 instruction watchpoints.  */
#define WPD_NUM 2	/* 2 data watchpoints.  */

struct bfin_wp
{
  bu32 base;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 iactl;
  bu32 _pad0[15];
  bu32 ia[WPI_NUM];
  bu32 _pad1[16 - WPI_NUM];
  bu32 iacnt[WPI_NUM];
  bu32 _pad2[32 - WPI_NUM];

  bu32 dactl;
  bu32 _pad3[15];
  bu32 da[WPD_NUM];
  bu32 _pad4[16 - WPD_NUM];
  bu32 dacnt[WPD_NUM];
  bu32 _pad5[32 - WPD_NUM];

  bu32 stat;
};
#define mmr_base()      offsetof(struct bfin_wp, iactl)
#define mmr_offset(mmr) (offsetof(struct bfin_wp, mmr) - mmr_base())
#define mmr_idx(mmr)    (mmr_offset (mmr) / 4)

static const char * const mmr_names[] =
{
  [mmr_idx (iactl)] = "WPIACTL",
  [mmr_idx (ia)]    = "WPIA0", "WPIA1", "WPIA2", "WPIA3", "WPIA4", "WPIA5",
  [mmr_idx (iacnt)] = "WPIACNT0", "WPIACNT1", "WPIACNT2",
		      "WPIACNT3", "WPIACNT4", "WPIACNT5",
  [mmr_idx (dactl)] = "WPDACTL",
  [mmr_idx (da)]    = "WPDA0", "WPDA1", "WPDA2", "WPDA3", "WPDA4", "WPDA5",
  [mmr_idx (dacnt)] = "WPDACNT0", "WPDACNT1", "WPDACNT2",
		      "WPDACNT3", "WPDACNT4", "WPDACNT5",
  [mmr_idx (stat)]  = "WPSTAT",
};
#define mmr_name(off) (mmr_names[(off) / 4] ? : "<INV>")

static unsigned
bfin_wp_io_write_buffer (struct hw *me, const void *source, int space,
			 address_word addr, unsigned nr_bytes)
{
  struct bfin_wp *wp = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu32 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
    return 0;

  value = dv_load_4 (source);
  mmr_off = addr - wp->base;
  valuep = (void *)((uintptr_t)wp + mmr_base() + mmr_off);

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(iactl):
    case mmr_offset(ia[0]) ... mmr_offset(ia[WPI_NUM - 1]):
    case mmr_offset(iacnt[0]) ... mmr_offset(iacnt[WPI_NUM - 1]):
    case mmr_offset(dactl):
    case mmr_offset(da[0]) ... mmr_offset(da[WPD_NUM - 1]):
    case mmr_offset(dacnt[0]) ... mmr_offset(dacnt[WPD_NUM - 1]):
      *valuep = value;
      break;
    case mmr_offset(stat):
      /* Yes, the hardware is this dumb -- clear all bits on any write.  */
      *valuep = 0;
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_wp_io_read_buffer (struct hw *me, void *dest, int space,
			address_word addr, unsigned nr_bytes)
{
  struct bfin_wp *wp = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu32 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - wp->base;
  valuep = (void *)((uintptr_t)wp + mmr_base() + mmr_off);

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(iactl):
    case mmr_offset(ia[0]) ... mmr_offset(ia[WPI_NUM - 1]):
    case mmr_offset(iacnt[0]) ... mmr_offset(iacnt[WPI_NUM - 1]):
    case mmr_offset(dactl):
    case mmr_offset(da[0]) ... mmr_offset(da[WPD_NUM - 1]):
    case mmr_offset(dacnt[0]) ... mmr_offset(dacnt[WPD_NUM - 1]):
    case mmr_offset(stat):
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
attach_bfin_wp_regs (struct hw *me, struct bfin_wp *wp)
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

  if (attach_size != BFIN_COREMMR_WP_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_COREMMR_WP_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  wp->base = attach_address;
}

static void
bfin_wp_finish (struct hw *me)
{
  struct bfin_wp *wp;

  wp = HW_ZALLOC (me, struct bfin_wp);

  set_hw_data (me, wp);
  set_hw_io_read_buffer (me, bfin_wp_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_wp_io_write_buffer);

  attach_bfin_wp_regs (me, wp);
}

const struct hw_descriptor dv_bfin_wp_descriptor[] =
{
  {"bfin_wp", bfin_wp_finish,},
  {NULL, NULL},
};
