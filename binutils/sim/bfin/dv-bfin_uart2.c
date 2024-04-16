/* Blackfin Universal Asynchronous Receiver/Transmitter (UART) model.
   For "new style" UARTs on BF50x/BF54x parts.

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
#include "dv-bfin_uart2.h"

/* XXX: Should we bother emulating the TX/RX FIFOs ?  */

/* Internal state needs to be the same as bfin_uart.  */
struct bfin_uart
{
  /* This top portion matches common dv_bfin struct.  */
  bu32 base;
  struct hw *dma_master;
  bool acked;

  struct hw_event *handler;
  char saved_byte;
  int saved_count;

  /* Accessed indirectly by ier_{set,clear}.  */
  bu16 ier;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(dll);
  bu16 BFIN_MMR_16(dlh);
  bu16 BFIN_MMR_16(gctl);
  bu16 BFIN_MMR_16(lcr);
  bu16 BFIN_MMR_16(mcr);
  bu16 BFIN_MMR_16(lsr);
  bu16 BFIN_MMR_16(msr);
  bu16 BFIN_MMR_16(scr);
  bu16 BFIN_MMR_16(ier_set);
  bu16 BFIN_MMR_16(ier_clear);
  bu16 BFIN_MMR_16(thr);
  bu16 BFIN_MMR_16(rbr);
};
#define mmr_base()      offsetof(struct bfin_uart, dll)
#define mmr_offset(mmr) (offsetof(struct bfin_uart, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "UART_DLL", "UART_DLH", "UART_GCTL", "UART_LCR", "UART_MCR", "UART_LSR",
  "UART_MSR", "UART_SCR", "UART_IER_SET", "UART_IER_CLEAR", "UART_THR",
  "UART_RBR",
};
#define mmr_name(off) mmr_names[(off) / 4]

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
    case mmr_offset(thr):
      uart->thr = bfin_uart_write_byte (me, value, uart->mcr);
      if (uart->ier & ETBEI)
	hw_port_event (me, DV_PORT_TX, 1);
      break;
    case mmr_offset(ier_set):
      uart->ier |= value;
      break;
    case mmr_offset(ier_clear):
      dv_w1c_2 (&uart->ier, value, -1);
      break;
    case mmr_offset(lsr):
      dv_w1c_2 (valuep, value, TFI | BI | FE | PE | OE);
      break;
    case mmr_offset(rbr):
      /* XXX: Writes are ignored ?  */
      break;
    case mmr_offset(msr):
      dv_w1c_2 (valuep, value, SCTS);
      break;
    case mmr_offset(dll):
    case mmr_offset(dlh):
    case mmr_offset(gctl):
    case mmr_offset(lcr):
    case mmr_offset(mcr):
    case mmr_offset(scr):
      *valuep = value;
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
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
    case mmr_offset(rbr):
      uart->rbr = bfin_uart_get_next_byte (me, uart->rbr, uart->mcr, NULL);
      dv_store_2 (dest, uart->rbr);
      break;
    case mmr_offset(ier_set):
    case mmr_offset(ier_clear):
      dv_store_2 (dest, uart->ier);
      bfin_uart_reschedule (me);
      break;
    case mmr_offset(lsr):
      uart->lsr &= ~(DR | THRE | TEMT);
      uart->lsr |= bfin_uart_get_status (me);
      ATTRIBUTE_FALLTHROUGH;
    case mmr_offset(thr):
    case mmr_offset(msr):
    case mmr_offset(dll):
    case mmr_offset(dlh):
    case mmr_offset(gctl):
    case mmr_offset(lcr):
    case mmr_offset(mcr):
    case mmr_offset(scr):
      dv_store_2 (dest, *valuep);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_uart_dma_read_buffer (struct hw *me, void *dest, int space,
			   unsigned_word addr, unsigned nr_bytes)
{
  HW_TRACE_DMA_READ ();
  return bfin_uart_read_buffer (me, dest, nr_bytes);
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

  if (attach_size != BFIN_MMR_UART2_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_UART2_SIZE);

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
  uart->lsr = 0x0060;
}

const struct hw_descriptor dv_bfin_uart2_descriptor[] =
{
  {"bfin_uart2", bfin_uart_finish,},
  {NULL, NULL},
};
