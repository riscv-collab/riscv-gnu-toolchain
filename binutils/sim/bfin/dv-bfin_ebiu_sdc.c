/* Blackfin External Bus Interface Unit (EBIU) SDRAM Controller (SDC) Model.

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
#include "dv-bfin_ebiu_sdc.h"

struct bfin_ebiu_sdc
{
  bu32 base;
  int type;
  bu32 reg_size, bank_size;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 sdgctl;
  bu32 sdbctl;	/* 16bit on most parts ... */
  bu16 BFIN_MMR_16(sdrrc);
  bu16 BFIN_MMR_16(sdstat);
};
#define mmr_base()      offsetof(struct bfin_ebiu_sdc, sdgctl)
#define mmr_offset(mmr) (offsetof(struct bfin_ebiu_sdc, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "EBIU_SDGCTL", "EBIU_SDBCTL", "EBIU_SDRRC", "EBIU_SDSTAT",
};
#define mmr_name(off) mmr_names[(off) / 4]

static unsigned
bfin_ebiu_sdc_io_write_buffer (struct hw *me, const void *source,
			       int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_ebiu_sdc *sdc = hw_data (me);
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

  mmr_off = addr - sdc->base;
  valuep = (void *)((uintptr_t)sdc + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(sdgctl):
      /* XXX: SRFS should make external mem unreadable.  */
      *value32p = value;
      break;
    case mmr_offset(sdbctl):
      if (sdc->type == 561)
	{
	  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
	    return 0;
	  *value32p = value;
	}
      else
	{
	  if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	    return 0;
	  *value16p = value;
	}
      break;
    case mmr_offset(sdrrc):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      *value16p = value;
      break;
    case mmr_offset(sdstat):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      /* XXX: Some bits are W1C ...  */
      break;
    }

  return nr_bytes;
}

static unsigned
bfin_ebiu_sdc_io_read_buffer (struct hw *me, void *dest,
			      int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_ebiu_sdc *sdc = hw_data (me);
  bu32 mmr_off;
  bu32 *value32p;
  bu16 *value16p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - sdc->base;
  valuep = (void *)((uintptr_t)sdc + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(sdgctl):
      dv_store_4 (dest, *value32p);
      break;
    case mmr_offset(sdbctl):
      if (sdc->type == 561)
	{
	  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
	    return 0;
	  dv_store_4 (dest, *value32p);
	}
      else
	{
	  if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
	    return 0;
	  dv_store_2 (dest, *value16p);
	}
      break;
    case mmr_offset(sdrrc):
    case mmr_offset(sdstat):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
	return 0;
      dv_store_2 (dest, *value16p);
      break;
    }

  return nr_bytes;
}

static void
attach_bfin_ebiu_sdc_regs (struct hw *me, struct bfin_ebiu_sdc *sdc)
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

  if (attach_size != BFIN_MMR_EBIU_SDC_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_EBIU_SDC_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  sdc->base = attach_address;
}

static void
bfin_ebiu_sdc_finish (struct hw *me)
{
  struct bfin_ebiu_sdc *sdc;

  sdc = HW_ZALLOC (me, struct bfin_ebiu_sdc);

  set_hw_data (me, sdc);
  set_hw_io_read_buffer (me, bfin_ebiu_sdc_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_ebiu_sdc_io_write_buffer);

  attach_bfin_ebiu_sdc_regs (me, sdc);

  sdc->type = hw_find_integer_property (me, "type");

  /* Initialize the SDC.  */
  sdc->sdgctl = 0xE0088849;
  sdc->sdbctl = 0x00000000;
  sdc->sdrrc = 0x081A;
  sdc->sdstat = 0x0008;

  /* XXX: We boot with 64M external memory by default ...  */
  sdc->sdbctl |= EBE | EBSZ_64 | EBCAW_10;
}

const struct hw_descriptor dv_bfin_ebiu_sdc_descriptor[] =
{
  {"bfin_ebiu_sdc", bfin_ebiu_sdc_finish,},
  {NULL, NULL},
};
