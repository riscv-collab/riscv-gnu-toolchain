/* Blackfin General Purpose Ports (GPIO) model
   For "new style" GPIOs on BF54x parts.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc. and Mike Frysinger.

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
#include "dv-bfin_gpio2.h"

struct bfin_gpio
{
  bu32 base;

  /* Only accessed indirectly via dir_{set,clear}.  */
  bu16 dir;

  /* Make sure hardware MMRs are aligned.  */
  bu16 _pad;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(fer);
  bu16 BFIN_MMR_16(data);
  bu16 BFIN_MMR_16(set);
  bu16 BFIN_MMR_16(clear);
  bu16 BFIN_MMR_16(dir_set);
  bu16 BFIN_MMR_16(dir_clear);
  bu16 BFIN_MMR_16(inen);
  bu32 mux;
};
#define mmr_base()      offsetof(struct bfin_gpio, fer)
#define mmr_offset(mmr) (offsetof(struct bfin_gpio, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "PORTIO_FER", "PORTIO", "PORTIO_SET", "PORTIO_CLEAR", "PORTIO_DIR_SET",
  "PORTIO_DIR_CLEAR", "PORTIO_INEN", "PORTIO_MUX",
};
#define mmr_name(off) mmr_names[(off) / 4]

static unsigned
bfin_gpio_io_write_buffer (struct hw *me, const void *source, int space,
			   address_word addr, unsigned nr_bytes)
{
  struct bfin_gpio *port = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  mmr_off = addr - port->base;

  /* Invalid access mode is higher priority than missing register.  */
  if (mmr_off == mmr_offset (mux))
    {
      if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
	return 0;
    }
  else
    if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;

  if (nr_bytes == 4)
    value = dv_load_4 (source);
  else
    value = dv_load_2 (source);
  valuep = (void *)((uintptr_t)port + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(fer):
    case mmr_offset(data):
    case mmr_offset(inen):
      *value16p = value;
      break;
    case mmr_offset(clear):
      /* We want to clear the related data MMR.  */
      dv_w1c_2 (&port->data, value, -1);
      break;
    case mmr_offset(set):
      /* We want to set the related data MMR.  */
      port->data |= value;
      break;
    case mmr_offset(dir_clear):
      dv_w1c_2 (&port->dir, value, -1);
      break;
    case mmr_offset(dir_set):
      port->dir |= value;
      break;
    case mmr_offset(mux):
      *value32p = value;
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  /* If tweaking output pins, make sure we send updated port info.  */
  switch (mmr_off)
    {
    case mmr_offset(data):
    case mmr_offset(set):
    case mmr_offset(clear):
    case mmr_offset(dir_set):
      {
	int i;
	bu32 bit;

	for (i = 0; i < 16; ++i)
	  {
	    bit = (1 << i);

	    if (!(port->inen & bit))
	      hw_port_event (me, i, !!(port->data & bit));
	  }

	break;
      }
    }

  return nr_bytes;
}

static unsigned
bfin_gpio_io_read_buffer (struct hw *me, void *dest, int space,
			  address_word addr, unsigned nr_bytes)
{
  struct bfin_gpio *port = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  mmr_off = addr - port->base;

  /* Invalid access mode is higher priority than missing register.  */
  if (mmr_off == mmr_offset (mux))
    {
      if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
	return 0;
    }
  else
    if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
      return 0;

  valuep = (void *)((uintptr_t)port + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(data):
    case mmr_offset(clear):
    case mmr_offset(set):
      dv_store_2 (dest, port->data);
      break;
    case mmr_offset(dir_clear):
    case mmr_offset(dir_set):
      dv_store_2 (dest, port->dir);
      break;
    case mmr_offset(fer):
    case mmr_offset(inen):
      dv_store_2 (dest, *value16p);
      break;
    case mmr_offset(mux):
      dv_store_4 (dest, *value32p);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

static const struct hw_port_descriptor bfin_gpio_ports[] =
{
  { "p0",     0, 0, bidirect_port, },
  { "p1",     1, 0, bidirect_port, },
  { "p2",     2, 0, bidirect_port, },
  { "p3",     3, 0, bidirect_port, },
  { "p4",     4, 0, bidirect_port, },
  { "p5",     5, 0, bidirect_port, },
  { "p6",     6, 0, bidirect_port, },
  { "p7",     7, 0, bidirect_port, },
  { "p8",     8, 0, bidirect_port, },
  { "p9",     9, 0, bidirect_port, },
  { "p10",   10, 0, bidirect_port, },
  { "p11",   11, 0, bidirect_port, },
  { "p12",   12, 0, bidirect_port, },
  { "p13",   13, 0, bidirect_port, },
  { "p14",   14, 0, bidirect_port, },
  { "p15",   15, 0, bidirect_port, },
  { NULL, 0, 0, 0, },
};

static void
bfin_gpio_port_event (struct hw *me, int my_port, struct hw *source,
		      int source_port, int level)
{
  struct bfin_gpio *port = hw_data (me);
  bu32 bit = (1 << my_port);

  /* Normalize the level value.  A simulated device can send any value
     it likes to us, but in reality we only care about 0 and 1.  This
     lets us assume only those two values below.  */
  level = !!level;

  HW_TRACE ((me, "pin %i set to %i", my_port, level));

  /* Only screw with state if this pin is set as an input, and the
     input is actually enabled, and it isn't in peripheral mode.  */
  if ((port->dir & bit) || !(port->inen & bit) || !(port->fer & bit))
    {
      HW_TRACE ((me, "ignoring level due to DIR=%i INEN=%i FER=%i",
		 !!(port->dir & bit), !!(port->inen & bit),
		 !!(port->fer & bit)));
      return;
    }

  hw_port_event (me, my_port, level);
}

static void
attach_bfin_gpio_regs (struct hw *me, struct bfin_gpio *port)
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

  if (attach_size != BFIN_MMR_GPIO2_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_GPIO2_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  port->base = attach_address;
}

static void
bfin_gpio_finish (struct hw *me)
{
  struct bfin_gpio *port;

  port = HW_ZALLOC (me, struct bfin_gpio);

  set_hw_data (me, port);
  set_hw_io_read_buffer (me, bfin_gpio_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_gpio_io_write_buffer);
  set_hw_ports (me, bfin_gpio_ports);
  set_hw_port_event (me, bfin_gpio_port_event);

  attach_bfin_gpio_regs (me, port);
}

const struct hw_descriptor dv_bfin_gpio2_descriptor[] =
{
  {"bfin_gpio2", bfin_gpio_finish,},
  {NULL, NULL},
};
