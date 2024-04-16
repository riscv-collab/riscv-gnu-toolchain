/* Blackfin Serial Peripheral Interface (SPI) model

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
#include "dv-bfin_spi.h"

/* XXX: This is merely a stub.  */

struct bfin_spi
{
  /* This top portion matches common dv_bfin struct.  */
  bu32 base;
  struct hw *dma_master;
  bool acked;

  struct hw_event *handler;
  char saved_byte;
  int saved_count;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(ctl);
  bu16 BFIN_MMR_16(flg);
  bu16 BFIN_MMR_16(stat);
  bu16 BFIN_MMR_16(tdbr);
  bu16 BFIN_MMR_16(rdbr);
  bu16 BFIN_MMR_16(baud);
  bu16 BFIN_MMR_16(shadow);
};
#define mmr_base()      offsetof(struct bfin_spi, ctl)
#define mmr_offset(mmr) (offsetof(struct bfin_spi, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "SPI_CTL", "SPI_FLG", "SPI_STAT", "SPI_TDBR",
  "SPI_RDBR", "SPI_BAUD", "SPI_SHADOW",
};
#define mmr_name(off) mmr_names[(off) / 4]

static bool
bfin_spi_enabled (struct bfin_spi *spi)
{
  return (spi->ctl & SPE);
}

static bu16
bfin_spi_timod (struct bfin_spi *spi)
{
  return (spi->ctl & TIMOD);
}

static unsigned
bfin_spi_io_write_buffer (struct hw *me, const void *source, int space,
			  address_word addr, unsigned nr_bytes)
{
  struct bfin_spi *spi = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu16 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
    return 0;

  value = dv_load_2 (source);
  mmr_off = addr - spi->base;
  valuep = (void *)((uintptr_t)spi + mmr_base() + mmr_off);

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(stat):
      dv_w1c_2 (valuep, value, ~(SPIF | TXS | RXS));
      break;
    case mmr_offset(tdbr):
      *valuep = value;
      if (bfin_spi_enabled (spi) && bfin_spi_timod (spi) == TDBR_CORE)
	{
	  spi->stat |= RXS;
	  spi->stat &= ~TXS;
	}
      break;
    case mmr_offset(rdbr):
    case mmr_offset(ctl):
    case mmr_offset(flg):
    case mmr_offset(baud):
    case mmr_offset(shadow):
      *valuep = value;
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_spi_io_read_buffer (struct hw *me, void *dest, int space,
			 address_word addr, unsigned nr_bytes)
{
  struct bfin_spi *spi = hw_data (me);
  bu32 mmr_off;
  bu16 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - spi->base;
  valuep = (void *)((uintptr_t)spi + mmr_base() + mmr_off);

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(rdbr):
      dv_store_2 (dest, *valuep);
      if (bfin_spi_enabled (spi) && bfin_spi_timod (spi) == RDBR_CORE)
	spi->stat &= ~(RXS | TXS);
      break;
    case mmr_offset(ctl):
    case mmr_offset(stat):
    case mmr_offset(flg):
    case mmr_offset(tdbr):
    case mmr_offset(baud):
    case mmr_offset(shadow):
      dv_store_2 (dest, *valuep);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_spi_dma_read_buffer (struct hw *me, void *dest, int space,
			  unsigned_word addr, unsigned nr_bytes)
{
  HW_TRACE_DMA_READ ();
  return 0;
}

static unsigned
bfin_spi_dma_write_buffer (struct hw *me, const void *source,
			   int space, unsigned_word addr,
			   unsigned nr_bytes,
			   int violate_read_only_section)
{
  HW_TRACE_DMA_WRITE ();
  return 0;
}

static const struct hw_port_descriptor bfin_spi_ports[] =
{
  { "stat", 0, 0, output_port, },
  { NULL, 0, 0, 0, },
};

static void
attach_bfin_spi_regs (struct hw *me, struct bfin_spi *spi)
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

  if (attach_size != BFIN_MMR_SPI_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_SPI_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  spi->base = attach_address;
}

static void
bfin_spi_finish (struct hw *me)
{
  struct bfin_spi *spi;

  spi = HW_ZALLOC (me, struct bfin_spi);

  set_hw_data (me, spi);
  set_hw_io_read_buffer (me, bfin_spi_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_spi_io_write_buffer);
  set_hw_dma_read_buffer (me, bfin_spi_dma_read_buffer);
  set_hw_dma_write_buffer (me, bfin_spi_dma_write_buffer);
  set_hw_ports (me, bfin_spi_ports);

  attach_bfin_spi_regs (me, spi);

  /* Initialize the SPI.  */
  spi->ctl  = 0x0400;
  spi->flg  = 0xFF00;
  spi->stat = 0x0001;
}

const struct hw_descriptor dv_bfin_spi_descriptor[] =
{
  {"bfin_spi", bfin_spi_finish,},
  {NULL, NULL},
};
