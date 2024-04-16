/* Blackfin Universal Asynchronous Receiver/Transmitter (UART) model.
   For "old style" UARTs on BF53x/etc... parts.

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
#include "dv-bfin_uart.h"

/* XXX: Should we bother emulating the TX/RX FIFOs ?  */

/* Internal state needs to be the same as bfin_uart2.  */
struct bfin_uart
{
  /* This top portion matches common dv_bfin struct.  */
  bu32 base;
  struct hw *dma_master;
  bool acked;

  struct hw_event *handler;
  char saved_byte;
  int saved_count;

  /* This is aliased to DLH.  */
  bu16 ier;
  /* These are aliased to DLL.  */
  bu16 thr, rbr;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(dll);
  bu16 BFIN_MMR_16(dlh);
  bu16 BFIN_MMR_16(iir);
  bu16 BFIN_MMR_16(lcr);
  bu16 BFIN_MMR_16(mcr);
  bu16 BFIN_MMR_16(lsr);
  bu16 BFIN_MMR_16(msr);
  bu16 BFIN_MMR_16(scr);
  bu16 _pad0[2];
  bu16 BFIN_MMR_16(gctl);
};
#define mmr_base()      offsetof(struct bfin_uart, dll)
#define mmr_offset(mmr) (offsetof(struct bfin_uart, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "UART_RBR/UART_THR", "UART_IER", "UART_IIR", "UART_LCR", "UART_MCR",
  "UART_LSR", "UART_MSR", "UART_SCR", "<INV>", "UART_GCTL",
};
static const char *mmr_name (struct bfin_uart *uart, bu32 idx)
{
  if (uart->lcr & DLAB)
    if (idx < 2)
      return idx == 0 ? "UART_DLL" : "UART_DLH";
  return mmr_names[idx];
}
#define mmr_name(off) mmr_name (uart, (off) / 4)

static void
bfin_uart_poll (struct hw *me, void *data)
{
  struct bfin_uart *uart = data;
  bu16 lsr;

  uart->handler = NULL;

  lsr = bfin_uart_get_status (me);
  if (lsr & DR)
    hw_port_event (me, DV_PORT_RX, 1);

  bfin_uart_reschedule (me);
}

void
bfin_uart_reschedule (struct hw *me)
{
  struct bfin_uart *uart = hw_data (me);

  if (uart->ier & ERBFI)
    {
      if (!uart->handler)
	uart->handler = hw_event_queue_schedule (me, 10000,
						 bfin_uart_poll, uart);
    }
  else
    {
      if (uart->handler)
	{
	  hw_event_queue_deschedule (me, uart->handler);
	  uart->handler = NULL;
	}
    }
}

bu16
bfin_uart_write_byte (struct hw *me, bu16 thr, bu16 mcr)
{
  struct bfin_uart *uart = hw_data (me);
  unsigned char ch = thr;

  if (mcr & LOOP_ENA)
    {
      /* XXX: This probably doesn't work exactly right with
              external FIFOs ...  */
      uart->saved_byte = thr;
      uart->saved_count = 1;
    }

  bfin_uart_write_buffer (me, &ch, 1);

  return thr;
}

static unsigned
bfin_uart_io_write_buffer (struct hw *me, const void *source,
			   int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_uart *uart = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu16 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
    return 0;

  value = dv_load_2 (source);
  mmr_off = addr - uart->base;
  valuep = (void *)((uintptr_t)uart + mmr_base() + mmr_off);

  HW_TRACE_WRITE ();

  /* XXX: All MMRs are "8bit" ... what happens to high 8bits ?  */
  switch (mmr_off)
    {
    case mmr_offset(dll):
      if (uart->lcr & DLAB)
	uart->dll = value;
      else
	{
	  uart->thr = bfin_uart_write_byte (me, value, uart->mcr);

	  if (uart->ier & ETBEI)
	    hw_port_event (me, DV_PORT_TX, 1);
	}
      break;
    case mmr_offset(dlh):
      if (uart->lcr & DLAB)
	uart->dlh = value;
      else
	{
	  uart->ier = value;
	  bfin_uart_reschedule (me);
	}
      break;
    case mmr_offset(iir):
    case mmr_offset(lsr):
      /* XXX: Writes are ignored ?  */
      break;
    case mmr_offset(lcr):
    case mmr_offset(mcr):
    case mmr_offset(scr):
    case mmr_offset(gctl):
      *valuep = value;
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

/* Switch between socket and stdin on the fly.  */
bu16
bfin_uart_get_next_byte (struct hw *me, bu16 rbr, bu16 mcr, bool *fresh)
{
  SIM_DESC sd = hw_system (me);
  struct bfin_uart *uart = hw_data (me);
  int status = dv_sockser_status (sd);
  bool _fresh;

  /* NB: The "uart" here may only use interal state.  */

  if (!fresh)
    fresh = &_fresh;

  *fresh = false;

  if (uart->saved_count > 0)
    {
      *fresh = true;
      rbr = uart->saved_byte;
      --uart->saved_count;
    }
  else if (mcr & LOOP_ENA)
    {
      /* RX is disconnected, so only return local data.  */
    }
  else if (status & DV_SOCKSER_DISCONNECTED)
    {
      char byte;
      int ret = sim_io_poll_read (sd, 0/*STDIN*/, &byte, 1);

      if (ret > 0)
	{
	  *fresh = true;
	  rbr = byte;
	}
    }
  else
    rbr = dv_sockser_read (sd);

  return rbr;
}

bu16
bfin_uart_get_status (struct hw *me)
{
  SIM_DESC sd = hw_system (me);
  struct bfin_uart *uart = hw_data (me);
  int status = dv_sockser_status (sd);
  bu16 lsr = 0;

  if (status & DV_SOCKSER_DISCONNECTED)
    {
      if (uart->saved_count <= 0)
	uart->saved_count = sim_io_poll_read (sd, 0/*STDIN*/,
					      &uart->saved_byte, 1);
      lsr |= TEMT | THRE | (uart->saved_count > 0 ? DR : 0);
    }
  else
    lsr |= (status & DV_SOCKSER_INPUT_EMPTY ? 0 : DR) |
	   (status & DV_SOCKSER_OUTPUT_EMPTY ? TEMT | THRE : 0);

  return lsr;
}

static unsigned
bfin_uart_io_read_buffer (struct hw *me, void *dest,
			  int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_uart *uart = hw_data (me);
  bu32 mmr_off;
  bu16 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - uart->base;
  valuep = (void *)((uintptr_t)uart + mmr_base() + mmr_off);

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(dll):
      if (uart->lcr & DLAB)
	dv_store_2 (dest, uart->dll);
      else
	{
	  uart->rbr = bfin_uart_get_next_byte (me, uart->rbr, uart->mcr, NULL);
	  dv_store_2 (dest, uart->rbr);
	}
      break;
    case mmr_offset(dlh):
      if (uart->lcr & DLAB)
	dv_store_2 (dest, uart->dlh);
      else
	dv_store_2 (dest, uart->ier);
      break;
    case mmr_offset(lsr):
      /* XXX: Reads are destructive on most parts, but not all ...  */
      uart->lsr |= bfin_uart_get_status (me);
      dv_store_2 (dest, *valuep);
      uart->lsr = 0;
      break;
    case mmr_offset(iir):
      /* XXX: Reads are destructive ...  */
    case mmr_offset(lcr):
    case mmr_offset(mcr):
    case mmr_offset(scr):
    case mmr_offset(gctl):
      dv_store_2 (dest, *valuep);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

unsigned
bfin_uart_read_buffer (struct hw *me, unsigned char *buffer, unsigned nr_bytes)
{
  SIM_DESC sd = hw_system (me);
  struct bfin_uart *uart = hw_data (me);
  int status = dv_sockser_status (sd);
  unsigned i = 0;

  if (status & DV_SOCKSER_DISCONNECTED)
    {
      int ret;

      while (uart->saved_count > 0 && i < nr_bytes)
	{
	  buffer[i++] = uart->saved_byte;
	  --uart->saved_count;
	}

      ret = sim_io_poll_read (sd, 0/*STDIN*/, (char *) buffer, nr_bytes - i);
      if (ret > 0)
	i += ret;
    }
  else
    buffer[i++] = dv_sockser_read (sd);

  return i;
}

static unsigned
bfin_uart_dma_read_buffer (struct hw *me, void *dest, int space,
			   unsigned_word addr, unsigned nr_bytes)
{
  HW_TRACE_DMA_READ ();
  return bfin_uart_read_buffer (me, dest, nr_bytes);
}

unsigned
bfin_uart_write_buffer (struct hw *me, const unsigned char *buffer,
			unsigned nr_bytes)
{
  SIM_DESC sd = hw_system (me);
  int status = dv_sockser_status (sd);

  if (status & DV_SOCKSER_DISCONNECTED)
    {
      sim_io_write_stdout (sd, (const char *) buffer, nr_bytes);
      sim_io_flush_stdout (sd);
    }
  else
    {
      /* Normalize errors to a value of 0.  */
      int ret = dv_sockser_write_buffer (sd, buffer, nr_bytes);
      nr_bytes = CLAMP (ret, 0, nr_bytes);
    }

  return nr_bytes;
}

static unsigned
bfin_uart_dma_write_buffer (struct hw *me, const void *source,
			    int space, unsigned_word addr,
			    unsigned nr_bytes,
			    int violate_read_only_section)
{
  struct bfin_uart *uart = hw_data (me);
  unsigned ret;

  HW_TRACE_DMA_WRITE ();

  ret = bfin_uart_write_buffer (me, source, nr_bytes);

  if (ret == nr_bytes && (uart->ier & ETBEI))
    hw_port_event (me, DV_PORT_TX, 1);

  return ret;
}

static const struct hw_port_descriptor bfin_uart_ports[] =
{
  { "tx",   DV_PORT_TX,   0, output_port, },
  { "rx",   DV_PORT_RX,   0, output_port, },
  { "stat", DV_PORT_STAT, 0, output_port, },
  { NULL, 0, 0, 0, },
};

static void
attach_bfin_uart_regs (struct hw *me, struct bfin_uart *uart)
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

  if (attach_size != BFIN_MMR_UART_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_UART_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  uart->base = attach_address;
}

static void
bfin_uart_finish (struct hw *me)
{
  struct bfin_uart *uart;

  uart = HW_ZALLOC (me, struct bfin_uart);

  set_hw_data (me, uart);
  set_hw_io_read_buffer (me, bfin_uart_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_uart_io_write_buffer);
  set_hw_dma_read_buffer (me, bfin_uart_dma_read_buffer);
  set_hw_dma_write_buffer (me, bfin_uart_dma_write_buffer);
  set_hw_ports (me, bfin_uart_ports);

  attach_bfin_uart_regs (me, uart);

  /* Initialize the UART.  */
  uart->dll = 0x0001;
  uart->iir = 0x0001;
  uart->lsr = 0x0060;
}

const struct hw_descriptor dv_bfin_uart_descriptor[] =
{
  {"bfin_uart", bfin_uart_finish,},
  {NULL, NULL},
};
