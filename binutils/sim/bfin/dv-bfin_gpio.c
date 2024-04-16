/* Blackfin General Purpose Ports (GPIO) model

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
#include "dv-bfin_gpio.h"

struct bfin_gpio
{
  bu32 base;

  bu16 int_state;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(data);
  bu16 BFIN_MMR_16(clear);
  bu16 BFIN_MMR_16(set);
  bu16 BFIN_MMR_16(toggle);
  bu16 BFIN_MMR_16(maska);
  bu16 BFIN_MMR_16(maska_clear);
  bu16 BFIN_MMR_16(maska_set);
  bu16 BFIN_MMR_16(maska_toggle);
  bu16 BFIN_MMR_16(maskb);
  bu16 BFIN_MMR_16(maskb_clear);
  bu16 BFIN_MMR_16(maskb_set);
  bu16 BFIN_MMR_16(maskb_toggle);
  bu16 BFIN_MMR_16(dir);
  bu16 BFIN_MMR_16(polar);
  bu16 BFIN_MMR_16(edge);
  bu16 BFIN_MMR_16(both);
  bu16 BFIN_MMR_16(inen);
};
#define mmr_base()      offsetof(struct bfin_gpio, data)
#define mmr_offset(mmr) (offsetof(struct bfin_gpio, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "PORTIO", "PORTIO_CLEAR", "PORTIO_SET", "PORTIO_TOGGLE", "PORTIO_MASKA",
  "PORTIO_MASKA_CLEAR", "PORTIO_MASKA_SET", "PORTIO_MASKA_TOGGLE",
  "PORTIO_MASKB", "PORTIO_MASKB_CLEAR", "PORTIO_MASKB_SET",
  "PORTIO_MASKB_TOGGLE", "PORTIO_DIR", "PORTIO_POLAR", "PORTIO_EDGE",
  "PORTIO_BOTH", "PORTIO_INEN",
};
#define mmr_name(off) mmr_names[(off) / 4]

static void
bfin_gpio_forward_int (struct hw *me, struct bfin_gpio *port, bu32 mask,
		       int dst_port)
{
  HW_TRACE ((me, "resending levels on port %c", 'a' + dst_port));
  hw_port_event (me, dst_port, !!(port->int_state & mask));
}
static void
bfin_gpio_forward_ints (struct hw *me, struct bfin_gpio *port)
{
  bfin_gpio_forward_int (me, port, port->maska, 0);
  bfin_gpio_forward_int (me, port, port->maskb, 1);
}

static void
bfin_gpio_forward_ouput (struct hw *me, struct bfin_gpio *port, bu32 odata)
{
  int pin, value, ovalue, bit;

  for (pin = 0; pin < 16; ++pin)
    {
      bit = 1 << pin;

      /* Make sure this is an output pin.  */
      if (!(port->dir & bit))
	continue;

      /* Only signal port if the pin changes value.  */
      value = !!(port->data & bit);
      ovalue = !!(odata & bit);
      if (value == ovalue)
	continue;

      HW_TRACE ((me, "outputting gpio %i changed to %i", pin, value));
      hw_port_event (me, pin, value);
    }
}

static unsigned
bfin_gpio_io_write_buffer (struct hw *me, const void *source, int space,
			   address_word addr, unsigned nr_bytes)
{
  struct bfin_gpio *port = hw_data (me);
  bu32 mmr_off;
  bu16 value;
  bu16 *valuep;
  bu32 data = port->data;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
    return 0;

  value = dv_load_2 (source);
  mmr_off = addr - port->base;
  valuep = (void *)((uintptr_t)port + mmr_base() + mmr_off);

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(data):
    case mmr_offset(maska):
    case mmr_offset(maskb):
    case mmr_offset(dir):
    case mmr_offset(polar):
    case mmr_offset(edge):
    case mmr_offset(both):
    case mmr_offset(inen):
      *valuep = value;
      break;
    case mmr_offset(clear):
    case mmr_offset(maska_clear):
    case mmr_offset(maskb_clear):
      /* We want to clear the related data MMR.  */
      valuep -= 2;
      dv_w1c_2 (valuep, value, -1);
      break;
    case mmr_offset(set):
    case mmr_offset(maska_set):
    case mmr_offset(maskb_set):
      /* We want to set the related data MMR.  */
      valuep -= 4;
      *valuep |= value;
      break;
    case mmr_offset(toggle):
    case mmr_offset(maska_toggle):
    case mmr_offset(maskb_toggle):
      /* We want to toggle the related data MMR.  */
      valuep -= 6;
      *valuep ^= value;
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  /* If updating masks, make sure we send updated port info.  */
  switch (mmr_off)
    {
    case mmr_offset(dir):
    case mmr_offset(data) ... mmr_offset(toggle):
      bfin_gpio_forward_ouput (me, port, data);
      break;
    case mmr_offset(maska) ... mmr_offset(maska_toggle):
      bfin_gpio_forward_int (me, port, port->maska, 0);
      break;
    case mmr_offset(maskb) ... mmr_offset(maskb_toggle):
      bfin_gpio_forward_int (me, port, port->maskb, 1);
      break;
    }

  return nr_bytes;
}

static unsigned
bfin_gpio_io_read_buffer (struct hw *me, void *dest, int space,
			  address_word addr, unsigned nr_bytes)
{
  struct bfin_gpio *port = hw_data (me);
  bu32 mmr_off;
  bu16 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - port->base;
  valuep = (void *)((uintptr_t)port + mmr_base() + mmr_off);

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(data):
    case mmr_offset(clear):
    case mmr_offset(set):
    case mmr_offset(toggle):
      dv_store_2 (dest, port->data);
      break;
    case mmr_offset(maska):
    case mmr_offset(maska_clear):
    case mmr_offset(maska_set):
    case mmr_offset(maska_toggle):
      dv_store_2 (dest, port->maska);
      break;
    case mmr_offset(maskb):
    case mmr_offset(maskb_clear):
    case mmr_offset(maskb_set):
    case mmr_offset(maskb_toggle):
      dv_store_2 (dest, port->maskb);
      break;
    case mmr_offset(dir):
    case mmr_offset(polar):
    case mmr_offset(edge):
    case mmr_offset(both):
    case mmr_offset(inen):
      dv_store_2 (dest, *valuep);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

static const struct hw_port_descriptor bfin_gpio_ports[] =
{
  { "mask_a", 0, 0, output_port, },
  { "mask_b", 1, 0, output_port, },
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
  bool olvl, nlvl;
  bu32 bit = (1 << my_port);

  /* Normalize the level value.  A simulated device can send any value
     it likes to us, but in reality we only care about 0 and 1.  This
     lets us assume only those two values below.  */
  level = !!level;

  HW_TRACE ((me, "pin %i set to %i", my_port, level));

  /* Only screw with state if this pin is set as an input, and the
     input is actually enabled.  */
  if ((port->dir & bit) || !(port->inen & bit))
    {
      HW_TRACE ((me, "ignoring level/int due to DIR=%i INEN=%i",
		 !!(port->dir & bit), !!(port->inen & bit)));
      return;
    }

  /* Get the old pin state for calculating an interrupt.  */
  olvl = !!(port->data & bit);

  /* Update the new pin state.  */
  port->data = (port->data & ~bit) | (level << my_port);

  /* See if this state transition will generate an interrupt.  */
  nlvl = !!(port->data & bit);

  if (port->edge & bit)
    {
      /* Pin is edge triggered.  */
      if (port->both & bit)
	{
	  /* Both edges.  */
	  if (olvl == nlvl)
	    {
	      HW_TRACE ((me, "ignoring int due to EDGE=%i BOTH=%i lvl=%i->%i",
			 !!(port->edge & bit), !!(port->both & bit),
			 olvl, nlvl));
	      return;
	    }
	}
      else
	{
	  /* Just one edge.  */
	  if (!(((port->polar & bit) && olvl > nlvl)
		|| (!(port->polar & bit) && olvl < nlvl)))
	    {
	      HW_TRACE ((me, "ignoring int due to EDGE=%i POLAR=%i lvl=%i->%i",
			 !!(port->edge & bit), !!(port->polar & bit),
			 olvl, nlvl));
	      return;
	    }
	}

      /* Send the signal up, and then fall through to clear it.  */
      port->int_state |= bit;
      bfin_gpio_forward_ints (me, port);
      port->int_state &= ~bit;
    }
  else
    {
      /* Pin is level triggered.  */
      if (nlvl == !!(port->polar & bit))
	{
	  HW_TRACE ((me, "ignoring int due to EDGE=%i POLAR=%i lvl=%i",
		     !!(port->edge & bit), !!(port->polar & bit), nlvl));
	  /* We still need to signal SIC to clear the int, so don't return.  */
	  port->int_state &= ~bit;
	}
      else
	port->int_state |= bit;
    }

  bfin_gpio_forward_ints (me, port);
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

  if (attach_size != BFIN_MMR_GPIO_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_GPIO_SIZE);

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

const struct hw_descriptor dv_bfin_gpio_descriptor[] =
{
  {"bfin_gpio", bfin_gpio_finish,},
  {NULL, NULL},
};
