/* Blackfin External Bus Interface Unit (EBIU) DDR Controller (DDRC) Model.

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
#include "dv-bfin_ebiu_ddrc.h"

struct bfin_ebiu_ddrc
{
  bu32 base, reg_size, bank_size;

  /* Order after here is important -- matches hardware MMR layout.  */
  union {
    struct { bu32 ddrctl0, ddrctl1, ddrctl2, ddrctl3; };
    bu32 ddrctl[4];
  };
  bu32 ddrque, erradd;
  bu16 BFIN_MMR_16(errmst);
  bu16 BFIN_MMR_16(rstctl);
  bu32 ddrbrc[8], ddrbwc[8];
  bu32 ddracct, ddrtact, ddrarct;
  bu32 ddrgc[4];
  bu32 ddrmcen, ddrmccl;
};
#define mmr_base()      offsetof(struct bfin_ebiu_ddrc, ddrctl0)
#define mmr_offset(mmr) (offsetof(struct bfin_ebiu_ddrc, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "EBIU_DDRCTL0", "EBIU_DDRCTL1", "EBIU_DDRCTL2", "EBIU_DDRCTL3", "EBIU_DDRQUE",
  "EBIU_ERRADD", "EBIU_ERRMST", "EBIU_RSTCTL", "EBIU_DDRBRC0", "EBIU_DDRBRC1",
  "EBIU_DDRBRC2", "EBIU_DDRBRC3", "EBIU_DDRBRC4", "EBIU_DDRBRC5",
  "EBIU_DDRBRC6", "EBIU_DDRBRC7", "EBIU_DDRBWC0", "EBIU_DDRBWC1"
  "EBIU_DDRBWC2", "EBIU_DDRBWC3", "EBIU_DDRBWC4", "EBIU_DDRBWC5",
  "EBIU_DDRBWC6", "EBIU_DDRBWC7", "EBIU_DDRACCT", "EBIU_DDRTACT",
  "EBIU_ARCT", "EBIU_DDRGC0", "EBIU_DDRGC1", "EBIU_DDRGC2", "EBIU_DDRGC3",
  "EBIU_DDRMCEN", "EBIU_DDRMCCL",
};
#define mmr_name(off) mmr_names[(off) / 4]

static unsigned
bfin_ebiu_ddrc_io_write_buffer (struct hw *me, const void *source,
			       int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_ebiu_ddrc *ddrc = hw_data (me);
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

  mmr_off = addr - ddrc->base;
  valuep = (void *)((uintptr_t)ddrc + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(errmst):
    case mmr_offset(rstctl):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      *value16p = value;
      break;
    default:
      if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
	return 0;
      *value32p = value;
      break;
    }

  return nr_bytes;
}

static unsigned
bfin_ebiu_ddrc_io_read_buffer (struct hw *me, void *dest,
			      int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_ebiu_ddrc *ddrc = hw_data (me);
  bu32 mmr_off;
  bu32 *value32p;
  bu16 *value16p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, true))
    return 0;

  mmr_off = addr - ddrc->base;
  valuep = (void *)((uintptr_t)ddrc + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(errmst):
    case mmr_offset(rstctl):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
	return 0;
      dv_store_2 (dest, *value16p);
      break;
    default:
      if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
	return 0;
      dv_store_4 (dest, *value32p);
      break;
    }

  return nr_bytes;
}

static void
attach_bfin_ebiu_ddrc_regs (struct hw *me, struct bfin_ebiu_ddrc *ddrc)
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

  if (attach_size != BFIN_MMR_EBIU_DDRC_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_EBIU_DDRC_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  ddrc->base = attach_address;
}

static void
bfin_ebiu_ddrc_finish (struct hw *me)
{
  struct bfin_ebiu_ddrc *ddrc;

  ddrc = HW_ZALLOC (me, struct bfin_ebiu_ddrc);

  set_hw_data (me, ddrc);
  set_hw_io_read_buffer (me, bfin_ebiu_ddrc_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_ebiu_ddrc_io_write_buffer);

  attach_bfin_ebiu_ddrc_regs (me, ddrc);

  /* Initialize the DDRC.  */
  ddrc->ddrctl0 = 0x098E8411;
  ddrc->ddrctl1 = 0x10026223;
  ddrc->ddrctl2 = 0x00000021;
  ddrc->ddrctl3 = 0x00000003; /* XXX: MDDR is 0x20 ...  */
  ddrc->ddrque = 0x00001115;
  ddrc->rstctl = 0x0002;
}

const struct hw_descriptor dv_bfin_ebiu_ddrc_descriptor[] =
{
  {"bfin_ebiu_ddrc", bfin_ebiu_ddrc_finish,},
  {NULL, NULL},
};
