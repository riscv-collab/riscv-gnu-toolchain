/* Blackfin Real Time Clock (RTC) model.

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

#include <time.h>
#include "sim-main.h"
#include "dv-sockser.h"
#include "devices.h"
#include "dv-bfin_rtc.h"

/* XXX: This read-only stub setup is based on host system clock.  */

struct bfin_rtc
{
  bu32 base;
  bu32 stat_shadow;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 stat;
  bu16 BFIN_MMR_16(ictl);
  bu16 BFIN_MMR_16(istat);
  bu16 BFIN_MMR_16(swcnt);
  bu32 alarm;
  bu16 BFIN_MMR_16(pren);
};
#define mmr_base()      offsetof(struct bfin_rtc, stat)
#define mmr_offset(mmr) (offsetof(struct bfin_rtc, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "RTC_STAT", "RTC_ICTL", "RTC_ISTAT", "RTC_SWCNT", "RTC_ALARM", "RTC_PREN",
};
#define mmr_name(off) mmr_names[(off) / 4]

static unsigned
bfin_rtc_io_write_buffer (struct hw *me, const void *source,
			  int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_rtc *rtc = hw_data (me);
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

  mmr_off = addr - rtc->base;
  valuep = (void *)((uintptr_t)rtc + mmr_base() + mmr_off);
  value16p = valuep;

  HW_TRACE_WRITE ();

  /* XXX: These probably need more work.  */
  switch (mmr_off)
    {
    case mmr_offset(stat):
      /* XXX: Ignore these since we are wired to host.  */
      break;
    case mmr_offset(istat):
      dv_w1c_2 (value16p, value, ~(1 << 14));
      break;
    case mmr_offset(alarm):
      break;
    case mmr_offset(ictl):
      /* XXX: This should schedule an event handler.  */
    case mmr_offset(swcnt):
    case mmr_offset(pren):
      break;
    }

  return nr_bytes;
}

static unsigned
bfin_rtc_io_read_buffer (struct hw *me, void *dest,
			 int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_rtc *rtc = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - rtc->base;
  valuep = (void *)((uintptr_t)rtc + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(stat):
      {
	time_t t = time (NULL);
	struct tm *tm = localtime (&t);
	bu32 value =
	  (((tm->tm_year - 70) * 365 + tm->tm_yday) << 17) |
	  (tm->tm_hour << 12) |
	  (tm->tm_min << 6) |
	  (tm->tm_sec << 0);
	dv_store_4 (dest, value);
	break;
      }
    case mmr_offset(alarm):
      dv_store_4 (dest, *value32p);
      break;
    case mmr_offset(istat):
    case mmr_offset(ictl):
    case mmr_offset(swcnt):
    case mmr_offset(pren):
      dv_store_2 (dest, *value16p);
      break;
    }

  return nr_bytes;
}

static const struct hw_port_descriptor bfin_rtc_ports[] =
{
  { "rtc", 0, 0, output_port, },
  { NULL, 0, 0, 0, },
};

static void
attach_bfin_rtc_regs (struct hw *me, struct bfin_rtc *rtc)
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

  if (attach_size != BFIN_MMR_RTC_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_RTC_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  rtc->base = attach_address;
}

static void
bfin_rtc_finish (struct hw *me)
{
  struct bfin_rtc *rtc;

  rtc = HW_ZALLOC (me, struct bfin_rtc);

  set_hw_data (me, rtc);
  set_hw_io_read_buffer (me, bfin_rtc_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_rtc_io_write_buffer);
  set_hw_ports (me, bfin_rtc_ports);

  attach_bfin_rtc_regs (me, rtc);

  /* Initialize the RTC.  */
}

const struct hw_descriptor dv_bfin_rtc_descriptor[] =
{
  {"bfin_rtc", bfin_rtc_finish,},
  {NULL, NULL},
};
