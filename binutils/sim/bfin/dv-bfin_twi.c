/* Blackfin Two Wire Interface (TWI) model

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
#include "dv-bfin_twi.h"

/* XXX: This is merely a stub.  */

struct bfin_twi
{
  /* This top portion matches common dv_bfin struct.  */
  bu32 base;
  struct hw *dma_master;
  bool acked;

  struct hw_event *handler;
  char saved_byte;
  int saved_count;

  bu16 xmt_fifo, rcv_fifo;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(clkdiv);
  bu16 BFIN_MMR_16(control);
  bu16 BFIN_MMR_16(slave_ctl);
  bu16 BFIN_MMR_16(slave_stat);
  bu16 BFIN_MMR_16(slave_addr);
  bu16 BFIN_MMR_16(master_ctl);
  bu16 BFIN_MMR_16(master_stat);
  bu16 BFIN_MMR_16(master_addr);
  bu16 BFIN_MMR_16(int_stat);
  bu16 BFIN_MMR_16(int_mask);
  bu16 BFIN_MMR_16(fifo_ctl);
  bu16 BFIN_MMR_16(fifo_stat);
  bu32 _pad0[20];
  bu16 BFIN_MMR_16(xmt_data8);
  bu16 BFIN_MMR_16(xmt_data16);
  bu16 BFIN_MMR_16(rcv_data8);
  bu16 BFIN_MMR_16(rcv_data16);
};
#define mmr_base()      offsetof(struct bfin_twi, clkdiv)
#define mmr_offset(mmr) (offsetof(struct bfin_twi, mmr) - mmr_base())
#define mmr_idx(mmr)    (mmr_offset (mmr) / 4)

static const char * const mmr_names[] =
{
  "TWI_CLKDIV", "TWI_CONTROL", "TWI_SLAVE_CTL", "TWI_SLAVE_STAT",
  "TWI_SLAVE_ADDR", "TWI_MASTER_CTL", "TWI_MASTER_STAT", "TWI_MASTER_ADDR",
  "TWI_INT_STAT", "TWI_INT_MASK", "TWI_FIFO_CTL", "TWI_FIFO_STAT",
  [mmr_idx (xmt_data8)] = "TWI_XMT_DATA8", "TWI_XMT_DATA16", "TWI_RCV_DATA8",
  "TWI_RCV_DATA16",
};
#define mmr_name(off) (mmr_names[(off) / 4] ? : "<INV>")

static unsigned
bfin_twi_io_write_buffer (struct hw *me, const void *source, int space,
			  address_word addr, unsigned nr_bytes)
{
  struct bfin_twi *twi = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu16 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
    return 0;

  value = dv_load_2 (source);
  mmr_off = addr - twi->base;
  valuep = (void *)((uintptr_t)twi + mmr_base() + mmr_off);

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(clkdiv):
    case mmr_offset(control):
    case mmr_offset(slave_ctl):
    case mmr_offset(slave_addr):
    case mmr_offset(master_ctl):
    case mmr_offset(master_addr):
    case mmr_offset(int_mask):
    case mmr_offset(fifo_ctl):
      *valuep = value;
      break;
    case mmr_offset(int_stat):
      dv_w1c_2 (valuep, value, -1);
      break;
    case mmr_offset(master_stat):
      dv_w1c_2 (valuep, value, BUFWRERR | BUFRDERR | DNAK | ANAK | LOSTARB);
      break;
    case mmr_offset(slave_stat):
    case mmr_offset(fifo_stat):
    case mmr_offset(rcv_data8):
    case mmr_offset(rcv_data16):
      /* These are all RO.  XXX: Does these throw error ?  */
      break;
    case mmr_offset(xmt_data8):
      value &= 0xff;
      ATTRIBUTE_FALLTHROUGH;
    case mmr_offset(xmt_data16):
      twi->xmt_fifo = value;
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_twi_io_read_buffer (struct hw *me, void *dest, int space,
			 address_word addr, unsigned nr_bytes)
{
  struct bfin_twi *twi = hw_data (me);
  bu32 mmr_off;
  bu16 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - twi->base;
  valuep = (void *)((uintptr_t)twi + mmr_base() + mmr_off);

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(clkdiv):
    case mmr_offset(control):
    case mmr_offset(slave_ctl):
    case mmr_offset(slave_stat):
    case mmr_offset(slave_addr):
    case mmr_offset(master_ctl):
    case mmr_offset(master_stat):
    case mmr_offset(master_addr):
    case mmr_offset(int_stat):
    case mmr_offset(int_mask):
    case mmr_offset(fifo_ctl):
    case mmr_offset(fifo_stat):
      dv_store_2 (dest, *valuep);
      break;
    case mmr_offset(rcv_data8):
    case mmr_offset(rcv_data16):
      dv_store_2 (dest, twi->rcv_fifo);
      break;
    case mmr_offset(xmt_data8):
    case mmr_offset(xmt_data16):
      /* These always read as 0.  */
      dv_store_2 (dest, 0);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

static const struct hw_port_descriptor bfin_twi_ports[] =
{
  { "stat", 0, 0, output_port, },
  { NULL, 0, 0, 0, },
};

static void
attach_bfin_twi_regs (struct hw *me, struct bfin_twi *twi)
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

  if (attach_size != BFIN_MMR_TWI_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_TWI_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  twi->base = attach_address;
}

static void
bfin_twi_finish (struct hw *me)
{
  struct bfin_twi *twi;

  twi = HW_ZALLOC (me, struct bfin_twi);

  set_hw_data (me, twi);
  set_hw_io_read_buffer (me, bfin_twi_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_twi_io_write_buffer);
  set_hw_ports (me, bfin_twi_ports);

  attach_bfin_twi_regs (me, twi);
}

const struct hw_descriptor dv_bfin_twi_descriptor[] =
{
  {"bfin_twi", bfin_twi_finish,},
  {NULL, NULL},
};
