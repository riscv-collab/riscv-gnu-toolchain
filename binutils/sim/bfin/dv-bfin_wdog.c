/* Blackfin Watchdog (WDOG) model.

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
#include "dv-sockser.h"
#include "devices.h"
#include "dv-bfin_wdog.h"

/* XXX: Should we bother emulating the TX/RX FIFOs ?  */

struct bfin_wdog
{
  bu32 base;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(ctl);
  bu32 cnt, stat;
};
#define mmr_base()      offsetof(struct bfin_wdog, ctl)
#define mmr_offset(mmr) (offsetof(struct bfin_wdog, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "WDOG_CTL", "WDOG_CNT", "WDOG_STAT",
};
#define mmr_name(off) mmr_names[(off) / 4]

static bool
bfin_wdog_enabled (struct bfin_wdog *wdog)
{
  return ((wdog->ctl & WDEN) != WDDIS);
}

static unsigned
bfin_wdog_io_write_buffer (struct hw *me, const void *source,
			   int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_wdog *wdog = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, true))
    return 0;

  if (nr_bytes == 4)
    value = dv_load_4 (source);
  else
    value = dv_load_2 (source);

  mmr_off = addr - wdog->base;
  valuep = (void *)((uintptr_t)wdog + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(ctl):
      dv_w1c_2_partial (value16p, value, WDRO);
      /* XXX: Should enable an event here to handle timeouts.  */
      break;

    case mmr_offset(cnt):
      /* Writes are discarded when enabeld.  */
      if (!bfin_wdog_enabled (wdog))
	{
	  *value32p = value;
	  /* Writes to CNT preloads the STAT.  */
	  wdog->stat = wdog->cnt;
	}
      break;

    case mmr_offset(stat):
      /* When enabled, writes to STAT reload the counter.  */
      if (bfin_wdog_enabled (wdog))
	wdog->stat = wdog->cnt;
      /* XXX: When disabled, are writes just ignored ?  */
      break;
    }

  return nr_bytes;
}

static unsigned
bfin_wdog_io_read_buffer (struct hw *me, void *dest,
			  int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_wdog *wdog = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - wdog->base;
  valuep = (void *)((uintptr_t)wdog + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(ctl):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
	return 0;
      dv_store_2 (dest, *value16p);
      break;

    case mmr_offset(cnt):
    case mmr_offset(stat):
      dv_store_4 (dest, *value32p);
      break;
    }

  return nr_bytes;
}

static const struct hw_port_descriptor bfin_wdog_ports[] =
{
  { "reset", WDEV_RESET, 0, output_port, },
  { "nmi",   WDEV_NMI,   0, output_port, },
  { "gpi",   WDEV_GPI,   0, output_port, },
  { NULL, 0, 0, 0, },
};

static void
bfin_wdog_port_event (struct hw *me, int my_port, struct hw *source,
		      int source_port, int level)
{
  struct bfin_wdog *wdog = hw_data (me);
  bu16 wdev;

  wdog->ctl |= WDRO;
  wdev = (wdog->ctl & WDEV);
  if (wdev != WDEV_NONE)
    hw_port_event (me, wdev, 1);
}

static void
attach_bfin_wdog_regs (struct hw *me, struct bfin_wdog *wdog)
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

  if (attach_size != BFIN_MMR_WDOG_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_WDOG_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  wdog->base = attach_address;
}

static void
bfin_wdog_finish (struct hw *me)
{
  struct bfin_wdog *wdog;

  wdog = HW_ZALLOC (me, struct bfin_wdog);

  set_hw_data (me, wdog);
  set_hw_io_read_buffer (me, bfin_wdog_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_wdog_io_write_buffer);
  set_hw_ports (me, bfin_wdog_ports);
  set_hw_port_event (me, bfin_wdog_port_event);

  attach_bfin_wdog_regs (me, wdog);

  /* Initialize the Watchdog.  */
  wdog->ctl = WDDIS;
}

const struct hw_descriptor dv_bfin_wdog_descriptor[] =
{
  {"bfin_wdog", bfin_wdog_finish,},
  {NULL, NULL},
};
