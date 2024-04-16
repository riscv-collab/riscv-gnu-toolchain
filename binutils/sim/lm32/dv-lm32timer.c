/*  Lattice Mico32 timer model.
    Contributed by Jon Beniston <jon@beniston.com>
    
   Copyright (C) 2009-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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
#include "hw-main.h"
#include "sim-assert.h"

struct lm32timer
{
  unsigned base;		/* Base address of this timer.  */
  unsigned limit;		/* Limit address of this timer.  */
  unsigned int status;
  unsigned int control;
  unsigned int period;
  unsigned int snapshot;
  struct hw_event *event;
};

/* Timer registers.  */
#define LM32_TIMER_STATUS       0x0
#define LM32_TIMER_CONTROL      0x4
#define LM32_TIMER_PERIOD       0x8
#define LM32_TIMER_SNAPSHOT     0xc

/* Timer ports.  */

enum
{
  INT_PORT
};

static const struct hw_port_descriptor lm32timer_ports[] = {
  {"int", INT_PORT, 0, output_port},
  {}
};

static void
do_timer_event (struct hw *me, void *data)
{
  struct lm32timer *timer = hw_data (me);

  /* Is timer started? */
  if (timer->control & 0x4)
    {
      if (timer->snapshot)
	{
	  /* Decrement timer.  */
	  timer->snapshot--;
	}
      else if (timer->control & 1)
	{
	  /* Restart timer.  */
	  timer->snapshot = timer->period;
	}
    }
  /* Generate interrupt when timer is at 0, and interrupt enable is 1.  */
  if ((timer->snapshot == 0) && (timer->control & 1))
    {
      /* Generate interrupt.  */
      hw_port_event (me, INT_PORT, 1);
    }
  /* If timer is started, schedule another event to decrement the timer again.  */
  if (timer->control & 4)
    hw_event_queue_schedule (me, 1, do_timer_event, 0);
}

static unsigned
lm32timer_io_write_buffer (struct hw *me,
			   const void *source,
			   int space, unsigned_word base, unsigned nr_bytes)
{
  struct lm32timer *timers = hw_data (me);
  int timer_reg;
  const unsigned char *source_bytes = source;
  int value = 0;

  HW_TRACE ((me, "write to 0x%08lx length %d with 0x%x", (long) base,
	     (int) nr_bytes, value));

  if (nr_bytes == 4)
    value = (source_bytes[0] << 24)
      | (source_bytes[1] << 16) | (source_bytes[2] << 8) | (source_bytes[3]);
  else
    hw_abort (me, "write with invalid number of bytes: %d", nr_bytes);

  timer_reg = base - timers->base;

  switch (timer_reg)
    {
    case LM32_TIMER_STATUS:
      timers->status = value;
      break;
    case LM32_TIMER_CONTROL:
      timers->control = value;
      if (timers->control & 0x4)
	{
	  /* Timer is started.  */
	  hw_event_queue_schedule (me, 1, do_timer_event, 0);
	}
      break;
    case LM32_TIMER_PERIOD:
      timers->period = value;
      break;
    default:
      hw_abort (me, "invalid register address: 0x%x.", timer_reg);
    }

  return nr_bytes;
}

static unsigned
lm32timer_io_read_buffer (struct hw *me,
			  void *dest,
			  int space, unsigned_word base, unsigned nr_bytes)
{
  struct lm32timer *timers = hw_data (me);
  int timer_reg;
  int value;
  unsigned char *dest_bytes = dest;

  HW_TRACE ((me, "read 0x%08lx length %d", (long) base, (int) nr_bytes));

  timer_reg = base - timers->base;

  switch (timer_reg)
    {
    case LM32_TIMER_STATUS:
      value = timers->status;
      break;
    case LM32_TIMER_CONTROL:
      value = timers->control;
      break;
    case LM32_TIMER_PERIOD:
      value = timers->period;
      break;
    case LM32_TIMER_SNAPSHOT:
      value = timers->snapshot;
      break;
    default:
      hw_abort (me, "invalid register address: 0x%x.", timer_reg);
    }

  if (nr_bytes == 4)
    {
      dest_bytes[0] = value >> 24;
      dest_bytes[1] = value >> 16;
      dest_bytes[2] = value >> 8;
      dest_bytes[3] = value;
    }
  else
    hw_abort (me, "read of unsupported number of bytes: %d", nr_bytes);

  return nr_bytes;
}

static void
attach_lm32timer_regs (struct hw *me, struct lm32timer *timers)
{
  unsigned_word attach_address;
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
  timers->base = attach_address;
  hw_unit_size_to_attach_size (hw_parent (me), &reg.size, &attach_size, me);
  timers->limit = attach_address + (attach_size - 1);
  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);
}

static void
lm32timer_finish (struct hw *me)
{
  struct lm32timer *timers;

  timers = HW_ZALLOC (me, struct lm32timer);
  set_hw_data (me, timers);
  set_hw_io_read_buffer (me, lm32timer_io_read_buffer);
  set_hw_io_write_buffer (me, lm32timer_io_write_buffer);
  set_hw_ports (me, lm32timer_ports);

  /* Attach ourself to our parent bus.  */
  attach_lm32timer_regs (me, timers);

  /* Initialize the timers.  */
  timers->status = 0;
  timers->control = 0;
  timers->period = 0;
  timers->snapshot = 0;
}

const struct hw_descriptor dv_lm32timer_descriptor[] = {
  {"lm32timer", lm32timer_finish,},
  {NULL},
};
