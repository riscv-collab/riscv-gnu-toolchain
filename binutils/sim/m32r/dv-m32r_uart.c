/* UART model.

   Copyright (C) 1996-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions and Mike Frysinger.

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
#include "hw-main.h"

#include "dv-sockser.h"
#include "dv-m32r_uart.h"

struct m32r_uart
{
};

static unsigned
m32r_uart_io_write_buffer (struct hw *me, const void *source,
			   int space, address_word addr, unsigned nr_bytes)
{
  SIM_DESC sd = hw_system (me);
  int status = dv_sockser_status (sd);

  switch (addr)
    {
    case UART_OUTCHAR_ADDR:
      if (status & DV_SOCKSER_DISCONNECTED)
	{
	  sim_io_write_stdout (sd, source, nr_bytes);
	  sim_io_flush_stdout (sd);
	}
      else
	{
	  /* Normalize errors to a value of 0.  */
	  int ret = dv_sockser_write_buffer (sd, source, nr_bytes);
	  if (ret < 0)
	    nr_bytes = 0;
	}
      break;
    }

  return nr_bytes;
}

static unsigned
m32r_uart_io_read_buffer (struct hw *me, void *dest,
			  int space, address_word addr, unsigned nr_bytes)
{
  SIM_DESC sd = hw_system (me);
  int status = dv_sockser_status (sd);

  switch (addr)
    {
    case UART_INCHAR_ADDR:
      if (status & DV_SOCKSER_DISCONNECTED)
	{
	  int ret = sim_io_poll_read (sd, 0/*STDIN*/, dest, 1);
	  return (ret < 0) ? 0 : 1;
        }
      else
	{
	  char *buffer = dest;
	  buffer[0] = dv_sockser_read (sd);
	  return 1;
	}
    case UART_STATUS_ADDR:
      {
	unsigned char *p = dest;
	p[0] = 0;
	p[1] = (((status & DV_SOCKSER_INPUT_EMPTY)
#ifdef UART_INPUT_READY0
		 ? UART_INPUT_READY : 0)
#else
		 ? 0 : UART_INPUT_READY)
#endif
		+ ((status & DV_SOCKSER_OUTPUT_EMPTY) ? UART_OUTPUT_READY : 0));
	return 2;
      }
    }

  return nr_bytes;
}

static void
attach_m32r_uart_regs (struct hw *me, struct m32r_uart *uart)
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

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);
}

static void
m32r_uart_finish (struct hw *me)
{
  struct m32r_uart *uart;

  uart = HW_ZALLOC (me, struct m32r_uart);

  set_hw_data (me, uart);
  set_hw_io_read_buffer (me, m32r_uart_io_read_buffer);
  set_hw_io_write_buffer (me, m32r_uart_io_write_buffer);

  attach_m32r_uart_regs (me, uart);
}

const struct hw_descriptor dv_m32r_uart_descriptor[] =
{
  {"m32r_uart", m32r_uart_finish,},
  {NULL, NULL},
};
