/* Blackfin Enhanced Parallel Port Interface (EPPI) model
   For "new style" PPIs on BF54x/etc... parts.

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
#include "dv-bfin_eppi.h"
#include "gui.h"

/* XXX: TX is merely a stub.  */

struct bfin_eppi
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
  bu16 BFIN_MMR_16(status);
  bu16 BFIN_MMR_16(hcount);
  bu16 BFIN_MMR_16(hdelay);
  bu16 BFIN_MMR_16(vcount);
  bu16 BFIN_MMR_16(vdelay);
  bu16 BFIN_MMR_16(frame);
  bu16 BFIN_MMR_16(line);
  bu16 BFIN_MMR_16(clkdiv);
  bu32 control, fs1w_hbl, fs1p_avpl, fsw2_lvb, fs2p_lavf, clip, err;
};
#define mmr_base()      offsetof(struct bfin_eppi, status)
#define mmr_offset(mmr) (offsetof(struct bfin_eppi, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "EPPI_STATUS", "EPPI_HCOUNT", "EPPI_HDELAY", "EPPI_VCOUNT", "EPPI_VDELAY",
  "EPPI_FRAME", "EPPI_LINE", "EPPI_CLKDIV", "EPPI_CONTROL", "EPPI_FS1W_HBL",
  "EPPI_FS1P_AVPL", "EPPI_FS2W_LVB", "EPPI_FS2P_LAVF", "EPPI_CLIP", "EPPI_ERR",
};
#define mmr_name(off) (mmr_names[(off) / 4] ? : "<INV>")

static void
bfin_eppi_gui_setup (struct bfin_eppi *eppi)
{
  /* If we are in RX mode, nothing to do.  */
  if (!(eppi->control & PORT_DIR))
    return;

  eppi->gui_state = bfin_gui_setup (eppi->gui_state,
				    eppi->control & PORT_EN,
				    eppi->hcount,
				    eppi->vcount,
				    eppi->color);
}

static unsigned
bfin_eppi_io_write_buffer (struct hw *me, const void *source,
			   int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_eppi *eppi = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, true))
    return 0;

  if (nr_bytes == 4)
    value = dv_load_4 (source);
  else
    value = dv_load_2 (source);

  mmr_off = addr - eppi->base;
  valuep = (void *)((uintptr_t)eppi + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(status):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      dv_w1c_2 (value16p, value, 0x1ff);
      break;
    case mmr_offset(hcount):
    case mmr_offset(hdelay):
    case mmr_offset(vcount):
    case mmr_offset(vdelay):
    case mmr_offset(frame):
    case mmr_offset(line):
    case mmr_offset(clkdiv):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      *value16p = value;
      break;
    case mmr_offset(control):
      *value32p = value;
      bfin_eppi_gui_setup (eppi);
      break;
    case mmr_offset(fs1w_hbl):
    case mmr_offset(fs1p_avpl):
    case mmr_offset(fsw2_lvb):
    case mmr_offset(fs2p_lavf):
    case mmr_offset(clip):
    case mmr_offset(err):
      if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
	return 0;
      *value32p = value;
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_eppi_io_read_buffer (struct hw *me, void *dest,
			  int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_eppi *eppi = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, true))
    return 0;

  mmr_off = addr - eppi->base;
  valuep = (void *)((uintptr_t)eppi + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(status):
    case mmr_offset(hcount):
    case mmr_offset(hdelay):
    case mmr_offset(vcount):
    case mmr_offset(vdelay):
    case mmr_offset(frame):
    case mmr_offset(line):
    case mmr_offset(clkdiv):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
	return 0;
      dv_store_2 (dest, *value16p);
      break;
    case mmr_offset(control):
    case mmr_offset(fs1w_hbl):
    case mmr_offset(fs1p_avpl):
    case mmr_offset(fsw2_lvb):
    case mmr_offset(fs2p_lavf):
    case mmr_offset(clip):
    case mmr_offset(err):
      if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
	return 0;
      dv_store_4 (dest, *value32p);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_eppi_dma_read_buffer (struct hw *me, void *dest, int space,
			   unsigned_word addr, unsigned nr_bytes)
{
  HW_TRACE_DMA_READ ();
  return 0;
}

static unsigned
bfin_eppi_dma_write_buffer (struct hw *me, const void *source,
			    int space, unsigned_word addr,
			    unsigned nr_bytes,
			    int violate_read_only_section)
{
  struct bfin_eppi *eppi = hw_data (me);

  HW_TRACE_DMA_WRITE ();

  return bfin_gui_update (eppi->gui_state, source, nr_bytes);
}

static const struct hw_port_descriptor bfin_eppi_ports[] =
{
  { "stat", 0, 0, output_port, },
  { NULL, 0, 0, 0, },
};

static void
attach_bfin_eppi_regs (struct hw *me, struct bfin_eppi *eppi)
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

  if (attach_size != BFIN_MMR_EPPI_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_EPPI_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  eppi->base = attach_address;
}

static void
bfin_eppi_finish (struct hw *me)
{
  struct bfin_eppi *eppi;
  const char *color;

  eppi = HW_ZALLOC (me, struct bfin_eppi);

  set_hw_data (me, eppi);
  set_hw_io_read_buffer (me, bfin_eppi_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_eppi_io_write_buffer);
  set_hw_dma_read_buffer (me, bfin_eppi_dma_read_buffer);
  set_hw_dma_write_buffer (me, bfin_eppi_dma_write_buffer);
  set_hw_ports (me, bfin_eppi_ports);

  attach_bfin_eppi_regs (me, eppi);

  /* Initialize the EPPI.  */
  if (hw_find_property (me, "color"))
    color = hw_find_string_property (me, "color");
  else
    color = NULL;
  eppi->color = bfin_gui_color (color);
}

const struct hw_descriptor dv_bfin_eppi_descriptor[] =
{
  {"bfin_eppi", bfin_eppi_finish,},
  {NULL, NULL},
};
