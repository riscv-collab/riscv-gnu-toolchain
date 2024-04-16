/* Blackfin Core Timer model.

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
#include "dv-bfin_ctimer.h"

struct bfin_ctimer
{
  bu32 base;
  struct hw_event *handler;
  int64_t timeout;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 tcntl, tperiod, tscale, tcount;
};
#define mmr_base()      offsetof(struct bfin_ctimer, tcntl)
#define mmr_offset(mmr) (offsetof(struct bfin_ctimer, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "TCNTL", "TPERIOD", "TSCALE", "TCOUNT",
};
#define mmr_name(off) mmr_names[(off) / 4]

static bool
bfin_ctimer_enabled (struct bfin_ctimer *ctimer)
{
  return (ctimer->tcntl & TMPWR) && (ctimer->tcntl & TMREN);
}

static bu32
bfin_ctimer_scale (struct bfin_ctimer *ctimer)
{
  /* Only low 8 bits are actually checked.  */
  return (ctimer->tscale & 0xff) + 1;
}

static void
bfin_ctimer_schedule (struct hw *me, struct bfin_ctimer *ctimer);

static void
bfin_ctimer_expire (struct hw *me, void *data)
{
  struct bfin_ctimer *ctimer = data;

  ctimer->tcntl |= TINT;
  if (ctimer->tcntl & TAUTORLD)
    {
      ctimer->tcount = ctimer->tperiod;
      bfin_ctimer_schedule (me, ctimer);
    }
  else
    {
      ctimer->tcount = 0;
      ctimer->handler = NULL;
    }

  hw_port_event (me, IVG_IVTMR, 1);
}

static void
bfin_ctimer_update_count (struct hw *me, struct bfin_ctimer *ctimer)
{
  bu32 scale, ticks;
  int64_t timeout;

  /* If the timer was enabled w/out autoreload and has expired, then
     there's nothing to calculate here.  */
  if (ctimer->handler == NULL)
    return;

  scale = bfin_ctimer_scale (ctimer);
  timeout = hw_event_remain_time (me, ctimer->handler);
  ticks = ctimer->timeout - timeout;
  ctimer->tcount -= (scale * ticks);
  ctimer->timeout = timeout;
}

static void
bfin_ctimer_deschedule (struct hw *me, struct bfin_ctimer *ctimer)
{
  if (ctimer->handler)
    {
      hw_event_queue_deschedule (me, ctimer->handler);
      ctimer->handler = NULL;
    }
}

static void
bfin_ctimer_schedule (struct hw *me, struct bfin_ctimer *ctimer)
{
  bu32 scale = bfin_ctimer_scale (ctimer);
  ctimer->timeout = (ctimer->tcount / scale) + !!(ctimer->tcount % scale);
  ctimer->handler = hw_event_queue_schedule (me, ctimer->timeout,
					     bfin_ctimer_expire,
					     ctimer);
}

static unsigned
bfin_ctimer_io_write_buffer (struct hw *me, const void *source,
			     int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_ctimer *ctimer = hw_data (me);
  bool curr_enabled;
  bu32 mmr_off;
  bu32 value;
  bu32 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
    return 0;

  value = dv_load_4 (source);
  mmr_off = addr - ctimer->base;
  valuep = (void *)((uintptr_t)ctimer + mmr_base() + mmr_off);

  HW_TRACE_WRITE ();

  curr_enabled = bfin_ctimer_enabled (ctimer);
  switch (mmr_off)
    {
    case mmr_offset(tcntl):
      /* HRM describes TINT as sticky, but it isn't W1C.  */
      *valuep = value;

      if (bfin_ctimer_enabled (ctimer) == curr_enabled)
	{
	  /* Do nothing.  */
	}
      else if (curr_enabled)
	{
	  bfin_ctimer_update_count (me, ctimer);
	  bfin_ctimer_deschedule (me, ctimer);
	}
      else
	bfin_ctimer_schedule (me, ctimer);

      break;
    case mmr_offset(tcount):
      /* HRM says writes are discarded when enabled.  */
      /* XXX: But hardware seems to be writeable all the time ?  */
      /* if (!curr_enabled) */
	*valuep = value;
      break;
    case mmr_offset(tperiod):
      /* HRM says writes are discarded when enabled.  */
      /* XXX: But hardware seems to be writeable all the time ?  */
      /* if (!curr_enabled) */
	{
	  /* Writes are mirrored into TCOUNT.  */
	  ctimer->tcount = value;
	  *valuep = value;
	}
      break;
    case mmr_offset(tscale):
      if (curr_enabled)
	{
	  bfin_ctimer_update_count (me, ctimer);
	  bfin_ctimer_deschedule (me, ctimer);
	}
      *valuep = value;
      if (curr_enabled)
	bfin_ctimer_schedule (me, ctimer);
      break;
    }

  return nr_bytes;
}

static unsigned
bfin_ctimer_io_read_buffer (struct hw *me, void *dest,
			    int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_ctimer *ctimer = hw_data (me);
  bu32 mmr_off;
  bu32 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - ctimer->base;
  valuep = (void *)((uintptr_t)ctimer + mmr_base() + mmr_off);

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(tcount):
      /* Since we're optimizing events here, we need to calculate
         the new tcount value.  */
      if (bfin_ctimer_enabled (ctimer))
	bfin_ctimer_update_count (me, ctimer);
      break;
    }

  dv_store_4 (dest, *valuep);

  return nr_bytes;
}

static const struct hw_port_descriptor bfin_ctimer_ports[] =
{
  { "ivtmr", IVG_IVTMR, 0, output_port, },
  { NULL, 0, 0, 0, },
};

static void
attach_bfin_ctimer_regs (struct hw *me, struct bfin_ctimer *ctimer)
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

  if (attach_size != BFIN_COREMMR_CTIMER_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_COREMMR_CTIMER_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  ctimer->base = attach_address;
}

static void
bfin_ctimer_finish (struct hw *me)
{
  struct bfin_ctimer *ctimer;

  ctimer = HW_ZALLOC (me, struct bfin_ctimer);

  set_hw_data (me, ctimer);
  set_hw_io_read_buffer (me, bfin_ctimer_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_ctimer_io_write_buffer);
  set_hw_ports (me, bfin_ctimer_ports);

  attach_bfin_ctimer_regs (me, ctimer);

  /* Initialize the Core Timer.  */
}

const struct hw_descriptor dv_bfin_ctimer_descriptor[] =
{
  {"bfin_ctimer", bfin_ctimer_finish,},
  {NULL, NULL},
};
