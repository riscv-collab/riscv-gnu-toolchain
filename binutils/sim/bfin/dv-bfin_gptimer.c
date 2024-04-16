/* Blackfin General Purpose Timers (GPtimer) model

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
#include "dv-bfin_gptimer.h"

/* XXX: This is merely a stub.  */

struct bfin_gptimer
{
  /* This top portion matches common dv_bfin struct.  */
  bu32 base;
  struct hw *dma_master;
  bool acked;

  struct hw_event *handler;
  char saved_byte;
  int saved_count;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(config);
  bu32 counter, period, width;
};
#define mmr_base()      offsetof(struct bfin_gptimer, config)
#define mmr_offset(mmr) (offsetof(struct bfin_gptimer, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "TIMER_CONFIG", "TIMER_COUNTER", "TIMER_PERIOD", "TIMER_WIDTH",
};
#define mmr_name(off) mmr_names[(off) / 4]

static unsigned
bfin_gptimer_io_write_buffer (struct hw *me, const void *source, int space,
			      address_word addr, unsigned nr_bytes)
{
  struct bfin_gptimer *gptimer = hw_data (me);
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

  mmr_off = addr - gptimer->base;
  valuep = (void *)((uintptr_t)gptimer + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(config):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      *value16p = value;
      break;
    case mmr_offset(counter):
    case mmr_offset(period):
    case mmr_offset(width):
      if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
	return 0;
      *value32p = value;
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_gptimer_io_read_buffer (struct hw *me, void *dest, int space,
			     address_word addr, unsigned nr_bytes)
{
  struct bfin_gptimer *gptimer = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - gptimer->base;
  valuep = (void *)((uintptr_t)gptimer + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(config):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
	return 0;
      dv_store_2 (dest, *value16p);
      break;
    case mmr_offset(counter):
    case mmr_offset(period):
    case mmr_offset(width):
      if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
	return 0;
      dv_store_4 (dest, *value32p);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

static const struct hw_port_descriptor bfin_gptimer_ports[] =
{
  { "stat", 0, 0, output_port, },
  { NULL, 0, 0, 0, },
};

static void
attach_bfin_gptimer_regs (struct hw *me, struct bfin_gptimer *gptimer)
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

  if (attach_size != BFIN_MMR_GPTIMER_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_GPTIMER_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  gptimer->base = attach_address;
}

static void
bfin_gptimer_finish (struct hw *me)
{
  struct bfin_gptimer *gptimer;

  gptimer = HW_ZALLOC (me, struct bfin_gptimer);

  set_hw_data (me, gptimer);
  set_hw_io_read_buffer (me, bfin_gptimer_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_gptimer_io_write_buffer);
  set_hw_ports (me, bfin_gptimer_ports);

  attach_bfin_gptimer_regs (me, gptimer);
}

const struct hw_descriptor dv_bfin_gptimer_descriptor[] =
{
  {"bfin_gptimer", bfin_gptimer_finish,},
  {NULL, NULL},
};
