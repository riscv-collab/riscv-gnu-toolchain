/*  Lattice Mico32 UART model.
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

#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>

struct lm32uart
{
  unsigned base;		/* Base address of this UART.  */
  unsigned limit;		/* Limit address of this UART.  */
  unsigned char rbr;
  unsigned char thr;
  unsigned char ier;
  unsigned char iir;
  unsigned char lcr;
  unsigned char mcr;
  unsigned char lsr;
  unsigned char msr;
  unsigned char div;
  struct hw_event *event;
};

/* UART registers.  */

#define LM32_UART_RBR           0x0
#define LM32_UART_THR           0x0
#define LM32_UART_IER           0x4
#define LM32_UART_IIR           0x8
#define LM32_UART_LCR           0xc
#define LM32_UART_MCR           0x10
#define LM32_UART_LSR           0x14
#define LM32_UART_MSR           0x18
#define LM32_UART_DIV           0x1c

#define LM32_UART_IER_RX_INT    0x1
#define LM32_UART_IER_TX_INT    0x2

#define MICOUART_IIR_TXRDY      0x2
#define MICOUART_IIR_RXRDY      0x4

#define LM32_UART_LSR_RX_RDY    0x01
#define LM32_UART_LSR_TX_RDY    0x20

#define LM32_UART_LCR_WLS_MASK  0x3
#define LM32_UART_LCR_WLS_5     0x0
#define LM32_UART_LCR_WLS_6     0x1
#define LM32_UART_LCR_WLS_7     0x2
#define LM32_UART_LCR_WLS_8     0x3

/* UART ports.  */

enum
{
  INT_PORT
};

static const struct hw_port_descriptor lm32uart_ports[] = {
  {"int", INT_PORT, 0, output_port},
  {}
};

static void
do_uart_tx_event (struct hw *me, void *data)
{
  struct lm32uart *uart = hw_data (me);
  char c;

  /* Generate interrupt when transmission is complete.  */
  if (uart->ier & LM32_UART_IER_TX_INT)
    {
      /* Generate interrupt */
      hw_port_event (me, INT_PORT, 1);
    }

  /* Indicate which interrupt has occured.  */
  uart->iir = MICOUART_IIR_TXRDY;

  /* Indicate THR is empty.  */
  uart->lsr |= LM32_UART_LSR_TX_RDY;

  /* Output the character in the THR.  */
  c = (char) uart->thr;

  /* WLS field in LCR register specifies the number of bits to output.  */
  switch (uart->lcr & LM32_UART_LCR_WLS_MASK)
    {
    case LM32_UART_LCR_WLS_5:
      c &= 0x1f;
      break;
    case LM32_UART_LCR_WLS_6:
      c &= 0x3f;
      break;
    case LM32_UART_LCR_WLS_7:
      c &= 0x7f;
      break;
    }
  printf ("%c", c);
}

static unsigned
lm32uart_io_write_buffer (struct hw *me,
			  const void *source,
			  int space, unsigned_word base, unsigned nr_bytes)
{
  struct lm32uart *uart = hw_data (me);
  int uart_reg;
  const unsigned char *source_bytes = source;
  int value = 0;

  HW_TRACE ((me, "write to 0x%08lx length %d with 0x%x", (long) base,
	     (int) nr_bytes, value));

  if (nr_bytes == 4)
    value = (source_bytes[0] << 24)
      | (source_bytes[1] << 16) | (source_bytes[2] << 8) | (source_bytes[3]);
  else
    hw_abort (me, "write of unsupported number of bytes: %d.", nr_bytes);

  uart_reg = base - uart->base;

  switch (uart_reg)
    {
    case LM32_UART_THR:
      /* Buffer the character to output.  */
      uart->thr = value;

      /* Indicate the THR is full.  */
      uart->lsr &= ~LM32_UART_LSR_TX_RDY;

      /* deassert interrupt when IER is loaded.  */
      uart->iir &= ~MICOUART_IIR_TXRDY;

      /* schedule an event to output the character.  */
      hw_event_queue_schedule (me, 1, do_uart_tx_event, 0);

      break;
    case LM32_UART_IER:
      uart->ier = value;
      if ((value & LM32_UART_IER_TX_INT)
	  && (uart->lsr & LM32_UART_LSR_TX_RDY))
	{
	  /* hw_event_queue_schedule (me, 1, do_uart_tx_event, 0); */
	  uart->lsr |= LM32_UART_LSR_TX_RDY;
	  uart->iir |= MICOUART_IIR_TXRDY;
	  hw_port_event (me, INT_PORT, 1);
	}
      else if ((value & LM32_UART_IER_TX_INT) == 0)
	{
	  hw_port_event (me, INT_PORT, 0);
	}
      break;
    case LM32_UART_IIR:
      uart->iir = value;
      break;
    case LM32_UART_LCR:
      uart->lcr = value;
      break;
    case LM32_UART_MCR:
      uart->mcr = value;
      break;
    case LM32_UART_LSR:
      uart->lsr = value;
      break;
    case LM32_UART_MSR:
      uart->msr = value;
      break;
    case LM32_UART_DIV:
      uart->div = value;
      break;
    default:
      hw_abort (me, "write to invalid register address: 0x%x.", uart_reg);
    }

  return nr_bytes;
}

static unsigned
lm32uart_io_read_buffer (struct hw *me,
			 void *dest,
			 int space, unsigned_word base, unsigned nr_bytes)
{
  struct lm32uart *uart = hw_data (me);
  int uart_reg;
  int value;
  unsigned char *dest_bytes = dest;
  fd_set fd;
  struct timeval tv;

  HW_TRACE ((me, "read 0x%08lx length %d", (long) base, (int) nr_bytes));

  uart_reg = base - uart->base;

  switch (uart_reg)
    {
    case LM32_UART_RBR:
      value = getchar ();
      uart->lsr &= ~LM32_UART_LSR_RX_RDY;
      break;
    case LM32_UART_IER:
      value = uart->ier;
      break;
    case LM32_UART_IIR:
      value = uart->iir;
      break;
    case LM32_UART_LCR:
      value = uart->lcr;
      break;
    case LM32_UART_MCR:
      value = uart->mcr;
      break;
    case LM32_UART_LSR:
      /* Check to see if any data waiting in stdin.  */
      FD_ZERO (&fd);
      FD_SET (fileno (stdin), &fd);
      tv.tv_sec = 0;
      tv.tv_usec = 1;
      if (select (fileno (stdin) + 1, &fd, NULL, NULL, &tv))
	uart->lsr |= LM32_UART_LSR_RX_RDY;
      value = uart->lsr;
      break;
    case LM32_UART_MSR:
      value = uart->msr;
      break;
    case LM32_UART_DIV:
      value = uart->div;
      break;
    default:
      hw_abort (me, "read from invalid register address: 0x%x.", uart_reg);
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
attach_lm32uart_regs (struct hw *me, struct lm32uart *uart)
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
  uart->base = attach_address;
  hw_unit_size_to_attach_size (hw_parent (me), &reg.size, &attach_size, me);
  uart->limit = attach_address + (attach_size - 1);
  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);
}

static void
lm32uart_finish (struct hw *me)
{
  struct lm32uart *uart;

  uart = HW_ZALLOC (me, struct lm32uart);
  set_hw_data (me, uart);
  set_hw_io_read_buffer (me, lm32uart_io_read_buffer);
  set_hw_io_write_buffer (me, lm32uart_io_write_buffer);
  set_hw_ports (me, lm32uart_ports);

  /* Attach ourself to our parent bus.  */
  attach_lm32uart_regs (me, uart);

  /* Initialize the UART.  */
  uart->rbr = 0;
  uart->thr = 0;
  uart->ier = 0;
  uart->iir = 0;
  uart->lcr = 0;
  uart->mcr = 0;
  uart->lsr = LM32_UART_LSR_TX_RDY;
  uart->msr = 0;
  uart->div = 0;		/* By setting to zero, characters are output immediately.  */
}

const struct hw_descriptor dv_lm32uart_descriptor[] = {
  {"lm32uart", lm32uart_finish,},
  {NULL},
};
