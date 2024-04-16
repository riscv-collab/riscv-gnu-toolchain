/* Blackfin Parallel Port Interface (PPI) model
   For "old style" PPIs on BF53x/etc... parts.

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
#include "dv-bfin_ppi.h"
#include "gui.h"

/* XXX: TX is merely a stub.  */

struct bfin_ppi
{
  /* This top portion matches common dv_bfin struct.  */
  bu32 base;
  struct hw *dma_master;
  bool acked;

  struct hw_event *handler;
  char saved_byte;
  int saved_count;

  /* GUI state.  */
  void *gui_state;
  int color;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(control);
  bu16 BFIN_MMR_16(status);
  bu16 BFIN_MMR_16(count);
  bu16 BFIN_MMR_16(delay);
  bu16 BFIN_MMR_16(frame);
};
#define mmr_base()      offsetof(struct bfin_ppi, control)
#define mmr_offset(mmr) (offsetof(struct bfin_ppi, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "PPI_CONTROL", "PPI_STATUS", "PPI_COUNT", "PPI_DELAY", "PPI_FRAME",
};
#define mmr_name(off) mmr_names[(off) / 4]

static void
bfin_ppi_gui_setup (struct bfin_ppi *ppi)
{
  int bpp;

  /* If we are in RX mode, nothing to do.  */
  if (!(ppi->control & PORT_DIR))
    return;

  bpp = bfin_gui_color_depth (ppi->color);
  ppi->gui_state = bfin_gui_setup (ppi->gui_state,
				   ppi->control & PORT_EN,
				   (ppi->count + 1) / (bpp / 8),
				   ppi->frame,
				   ppi->color);
}

static unsigned
bfin_ppi_io_write_buffer (struct hw *me, const void *source, int space,
			  address_word addr, unsigned nr_bytes)
{
  struct bfin_ppi *ppi = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu16 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
    return 0;

  value = dv_load_2 (source);
  mmr_off = addr - ppi->base;
  valuep = (void *)((uintptr_t)ppi + mmr_base() + mmr_off);

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(control):
      *valuep = value;
      bfin_ppi_gui_setup (ppi);
      break;
    case mmr_offset(count):
    case mmr_offset(delay):
    case mmr_offset(frame):
      *valuep = value;
      break;
    case mmr_offset(status):
      dv_w1c_2 (valuep, value, ~(1 << 10));
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_ppi_io_read_buffer (struct hw *me, void *dest, int space,
			 address_word addr, unsigned nr_bytes)
{
  struct bfin_ppi *ppi = hw_data (me);
  bu32 mmr_off;
  bu16 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - ppi->base;
  valuep = (void *)((uintptr_t)ppi + mmr_base() + mmr_off);

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(control):
    case mmr_offset(count):
    case mmr_offset(delay):
    case mmr_offset(frame):
    case mmr_offset(status):
      dv_store_2 (dest, *valuep);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_ppi_dma_read_buffer (struct hw *me, void *dest, int space,
			  unsigned_word addr, unsigned nr_bytes)
{
  HW_TRACE_DMA_READ ();
  return 0;
}

static unsigned
bfin_ppi_dma_write_buffer (struct hw *me, const void *source,
			   int space, unsigned_word addr,
			   unsigned nr_bytes,
			   int violate_read_only_section)
{
  struct bfin_ppi *ppi = hw_data (me);

  HW_TRACE_DMA_WRITE ();

  return bfin_gui_update (ppi->gui_state, source, nr_bytes);
}

static const struct hw_port_descriptor bfin_ppi_ports[] =
{
  { "stat", 0, 0, output_port, },
  { NULL, 0, 0, 0, },
};

static void
attach_bfin_ppi_regs (struct hw *me, struct bfin_ppi *ppi)
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

  if (attach_size != BFIN_MMR_PPI_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_PPI_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  ppi->base = attach_address;
}

static void
bfin_ppi_finish (struct hw *me)
{
  struct bfin_ppi *ppi;
  const char *color;

  ppi = HW_ZALLOC (me, struct bfin_ppi);

  set_hw_data (me, ppi);
  set_hw_io_read_buffer (me, bfin_ppi_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_ppi_io_write_buffer);
  set_hw_dma_read_buffer (me, bfin_ppi_dma_read_buffer);
  set_hw_dma_write_buffer (me, bfin_ppi_dma_write_buffer);
  set_hw_ports (me, bfin_ppi_ports);

  attach_bfin_ppi_regs (me, ppi);

  /* Initialize the PPI.  */
  if (hw_find_property (me, "color"))
    color = hw_find_string_property (me, "color");
  else
    color = NULL;
  ppi->color = bfin_gui_color (color);
}

const struct hw_descriptor dv_bfin_ppi_descriptor[] =
{
  {"bfin_ppi", bfin_ppi_finish,},
  {NULL, NULL},
};
