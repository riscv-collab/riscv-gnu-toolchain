/* Blackfin External Bus Interface Unit (EBIU) Asynchronous Memory Controller
   (AMC) model.

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
#include "dv-bfin_ebiu_amc.h"

struct bfin_ebiu_amc
{
  bu32 base;
  int type;
  bu32 bank_base, bank_size;
  unsigned (*io_write) (struct hw *, const void *, int, address_word,
			unsigned, struct bfin_ebiu_amc *, bu32, bu32);
  unsigned (*io_read) (struct hw *, void *, int, address_word, unsigned,
		       struct bfin_ebiu_amc *, bu32, void *, bu16 *, bu32 *);
  struct hw *slaves[4];

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(amgctl);
  union {
    struct {
      bu32 ambctl0, ambctl1;
      bu32 _pad0[5];
      bu16 BFIN_MMR_16(mode);
      bu16 BFIN_MMR_16(fctl);
    } bf50x;
    struct {
      bu32 ambctl0, ambctl1;
    } bf53x;
    struct {
      bu32 ambctl0, ambctl1;
      bu32 mbsctl, arbstat, mode, fctl;
    } bf54x;
  };
};
#define mmr_base()      offsetof(struct bfin_ebiu_amc, amgctl)
#define mmr_offset(mmr) (offsetof(struct bfin_ebiu_amc, mmr) - mmr_base())
#define mmr_idx(mmr)    (mmr_offset (mmr) / 4)

static const char * const bf50x_mmr_names[] =
{
  "EBIU_AMGCTL", "EBIU_AMBCTL0", "EBIU_AMBCTL1",
  [mmr_idx (bf50x.mode)] = "EBIU_MODE", "EBIU_FCTL",
};
static const char * const bf53x_mmr_names[] =
{
  "EBIU_AMGCTL", "EBIU_AMBCTL0", "EBIU_AMBCTL1",
};
static const char * const bf54x_mmr_names[] =
{
  "EBIU_AMGCTL", "EBIU_AMBCTL0", "EBIU_AMBCTL1",
  "EBIU_MSBCTL", "EBIU_ARBSTAT", "EBIU_MODE", "EBIU_FCTL",
};
static const char * const *mmr_names;
#define mmr_name(off) (mmr_names[(off) / 4] ? : "<INV>")

static void
bfin_ebiu_amc_write_amgctl (struct hw *me, struct bfin_ebiu_amc *amc,
			    bu16 amgctl)
{
  bu32 amben_old, amben, addr, i;

  amben_old = min ((amc->amgctl >> 1) & 0x7, 4);
  amben = min ((amgctl >> 1) & 0x7, 4);

  HW_TRACE ((me, "reattaching banks: AMGCTL 0x%04x[%u] -> 0x%04x[%u]",
	     amc->amgctl, amben_old, amgctl, amben));

  for (i = 0; i < 4; ++i)
    {
      addr = amc->bank_base + i * amc->bank_size;

      if (i < amben_old)
	{
	  HW_TRACE ((me, "detaching bank %u (%#x base)", i, addr));
	  sim_core_detach (hw_system (me), NULL, 0, 0, addr);
	}

      if (i < amben)
	{
	  struct hw *slave = amc->slaves[i];

	  HW_TRACE ((me, "attaching bank %u (%#x base) to %s", i, addr,
		     slave ? hw_path (slave) : "<floating pins>"));

	  sim_core_attach (hw_system (me), NULL, 0, access_read_write_exec,
			   0, addr, amc->bank_size, 0, slave, NULL);
	}
    }

  amc->amgctl = amgctl;
}

static unsigned
bf50x_ebiu_amc_io_write_buffer (struct hw *me, const void *source, int space,
				address_word addr, unsigned nr_bytes,
				struct bfin_ebiu_amc *amc, bu32 mmr_off,
				bu32 value)
{
  switch (mmr_off)
    {
    case mmr_offset(amgctl):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      bfin_ebiu_amc_write_amgctl (me, amc, value);
      break;
    case mmr_offset(bf50x.ambctl0):
      amc->bf50x.ambctl0 = value;
      break;
    case mmr_offset(bf50x.ambctl1):
      amc->bf50x.ambctl1 = value;
      break;
    case mmr_offset(bf50x.mode):
      /* XXX: implement this.  */
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      break;
    case mmr_offset(bf50x.fctl):
      /* XXX: implement this.  */
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bf53x_ebiu_amc_io_write_buffer (struct hw *me, const void *source, int space,
				address_word addr, unsigned nr_bytes,
				struct bfin_ebiu_amc *amc, bu32 mmr_off,
				bu32 value)
{
  switch (mmr_off)
    {
    case mmr_offset(amgctl):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      bfin_ebiu_amc_write_amgctl (me, amc, value);
      break;
    case mmr_offset(bf53x.ambctl0):
      amc->bf53x.ambctl0 = value;
      break;
    case mmr_offset(bf53x.ambctl1):
      amc->bf53x.ambctl1 = value;
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bf54x_ebiu_amc_io_write_buffer (struct hw *me, const void *source, int space,
				address_word addr, unsigned nr_bytes,
				struct bfin_ebiu_amc *amc, bu32 mmr_off,
				bu32 value)
{
  switch (mmr_off)
    {
    case mmr_offset(amgctl):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      bfin_ebiu_amc_write_amgctl (me, amc, value);
      break;
    case mmr_offset(bf54x.ambctl0):
      amc->bf54x.ambctl0 = value;
      break;
    case mmr_offset(bf54x.ambctl1):
      amc->bf54x.ambctl1 = value;
      break;
    case mmr_offset(bf54x.mbsctl):
      /* XXX: implement this.  */
      break;
    case mmr_offset(bf54x.arbstat):
      /* XXX: implement this.  */
      break;
    case mmr_offset(bf54x.mode):
      /* XXX: implement this.  */
      break;
    case mmr_offset(bf54x.fctl):
      /* XXX: implement this.  */
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_ebiu_amc_io_write_buffer (struct hw *me, const void *source, int space,
			       address_word addr, unsigned nr_bytes)
{
  struct bfin_ebiu_amc *amc = hw_data (me);
  bu32 mmr_off;
  bu32 value;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, true))
    return 0;

  value = dv_load_4 (source);
  mmr_off = addr - amc->base;

  HW_TRACE_WRITE ();

  return amc->io_write (me, source, space, addr, nr_bytes,
			amc, mmr_off, value);
}

static unsigned
bf50x_ebiu_amc_io_read_buffer (struct hw *me, void *dest, int space,
			       address_word addr, unsigned nr_bytes,
			       struct bfin_ebiu_amc *amc, bu32 mmr_off,
			       void *valuep, bu16 *value16, bu32 *value32)
{
  switch (mmr_off)
    {
    case mmr_offset(amgctl):
    case mmr_offset(bf50x.fctl):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
	return 0;
      dv_store_2 (dest, *value16);
      break;
    case mmr_offset(bf50x.ambctl0):
    case mmr_offset(bf50x.ambctl1):
    case mmr_offset(bf50x.mode):
      dv_store_4 (dest, *value32);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bf53x_ebiu_amc_io_read_buffer (struct hw *me, void *dest, int space,
			       address_word addr, unsigned nr_bytes,
			       struct bfin_ebiu_amc *amc, bu32 mmr_off,
			       void *valuep, bu16 *value16, bu32 *value32)
{
  switch (mmr_off)
    {
    case mmr_offset(amgctl):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
	return 0;
      dv_store_2 (dest, *value16);
      break;
    case mmr_offset(bf53x.ambctl0):
    case mmr_offset(bf53x.ambctl1):
      dv_store_4 (dest, *value32);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bf54x_ebiu_amc_io_read_buffer (struct hw *me, void *dest, int space,
			       address_word addr, unsigned nr_bytes,
			       struct bfin_ebiu_amc *amc, bu32 mmr_off,
			       void *valuep, bu16 *value16, bu32 *value32)
{
  switch (mmr_off)
    {
    case mmr_offset(amgctl):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
	return 0;
      dv_store_2 (dest, *value16);
      break;
    case mmr_offset(bf54x.ambctl0):
    case mmr_offset(bf54x.ambctl1):
    case mmr_offset(bf54x.mbsctl):
    case mmr_offset(bf54x.arbstat):
    case mmr_offset(bf54x.mode):
    case mmr_offset(bf54x.fctl):
      dv_store_4 (dest, *value32);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_ebiu_amc_io_read_buffer (struct hw *me, void *dest, int space,
			      address_word addr, unsigned nr_bytes)
{
  struct bfin_ebiu_amc *amc = hw_data (me);
  bu32 mmr_off;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - amc->base;
  valuep = (void *)((uintptr_t)amc + mmr_base() + mmr_off);

  HW_TRACE_READ ();

  return amc->io_read (me, dest, space, addr, nr_bytes, amc,
		       mmr_off, valuep, valuep, valuep);
}

static void
bfin_ebiu_amc_attach_address_callback (struct hw *me,
				       int level,
				       int space,
				       address_word addr,
				       address_word nr_bytes,
				       struct hw *client)
{
  struct bfin_ebiu_amc *amc = hw_data (me);

  HW_TRACE ((me, "attach - level=%d, space=%d, addr=0x%lx, nr_bytes=%lu, client=%s",
	     level, space, (unsigned long) addr, (unsigned long) nr_bytes, hw_path (client)));

  if (addr + nr_bytes > ARRAY_SIZE (amc->slaves))
    hw_abort (me, "ebiu amc attaches are done in terms of banks");

  while (nr_bytes--)
    amc->slaves[addr + nr_bytes] = client;

  bfin_ebiu_amc_write_amgctl (me, amc, amc->amgctl);
}

static void
attach_bfin_ebiu_amc_regs (struct hw *me, struct bfin_ebiu_amc *amc,
			   unsigned reg_size)
{
  address_word attach_address;
  int attach_space;
  unsigned attach_size;
  reg_property_spec reg;

  if (hw_find_property (me, "reg") == NULL)
    hw_abort (me, "Missing \"reg\" property");

  if (!hw_find_reg_array_property (me, "reg", 0, &reg))
    hw_abort (me, "\"reg\" property must contain three addr/size entries");

  if (hw_find_property (me, "type") == NULL)
    hw_abort (me, "Missing \"type\" property");

  hw_unit_address_to_attach_address (hw_parent (me),
				     &reg.address,
				     &attach_space, &attach_address, me);
  hw_unit_size_to_attach_size (hw_parent (me), &reg.size, &attach_size, me);

  if (attach_size != reg_size)
    hw_abort (me, "\"reg\" size must be %#x", reg_size);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  amc->base = attach_address;
}

static void
bfin_ebiu_amc_finish (struct hw *me)
{
  struct bfin_ebiu_amc *amc;
  bu32 amgctl;
  unsigned reg_size;

  amc = HW_ZALLOC (me, struct bfin_ebiu_amc);

  set_hw_data (me, amc);
  set_hw_io_read_buffer (me, bfin_ebiu_amc_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_ebiu_amc_io_write_buffer);
  set_hw_attach_address (me, bfin_ebiu_amc_attach_address_callback);

  amc->type = hw_find_integer_property (me, "type");

  switch (amc->type)
    {
    case 500 ... 509:
      amc->io_write = bf50x_ebiu_amc_io_write_buffer;
      amc->io_read = bf50x_ebiu_amc_io_read_buffer;
      mmr_names = bf50x_mmr_names;
      reg_size = sizeof (amc->bf50x) + 4;

      /* Initialize the AMC.  */
      amc->bank_base     = BFIN_EBIU_AMC_BASE;
      amc->bank_size     = 1 * 1024 * 1024;
      amgctl             = 0x00F3;
      amc->bf50x.ambctl0 = 0x0000FFC2;
      amc->bf50x.ambctl1 = 0x0000FFC2;
      amc->bf50x.mode    = 0x0001;
      amc->bf50x.fctl    = 0x0002;
      break;
    case 540 ... 549:
      amc->io_write = bf54x_ebiu_amc_io_write_buffer;
      amc->io_read = bf54x_ebiu_amc_io_read_buffer;
      mmr_names = bf54x_mmr_names;
      reg_size = sizeof (amc->bf54x) + 4;

      /* Initialize the AMC.  */
      amc->bank_base     = BFIN_EBIU_AMC_BASE;
      amc->bank_size     = 64 * 1024 * 1024;
      amgctl             = 0x0002;
      amc->bf54x.ambctl0 = 0xFFC2FFC2;
      amc->bf54x.ambctl1 = 0xFFC2FFC2;
      amc->bf54x.fctl    = 0x0006;
      break;
    case 510 ... 519:
    case 522 ... 527:
    case 531 ... 533:
    case 534:
    case 536:
    case 537:
    case 538 ... 539:
    case 561:
      amc->io_write = bf53x_ebiu_amc_io_write_buffer;
      amc->io_read = bf53x_ebiu_amc_io_read_buffer;
      mmr_names = bf53x_mmr_names;
      reg_size = sizeof (amc->bf53x) + 4;

      /* Initialize the AMC.  */
      amc->bank_base     = BFIN_EBIU_AMC_BASE;
      if (amc->type == 561)
	amc->bank_size   = 64 * 1024 * 1024;
      else
	amc->bank_size   = 1 * 1024 * 1024;
      amgctl             = 0x00F2;
      amc->bf53x.ambctl0 = 0xFFC2FFC2;
      amc->bf53x.ambctl1 = 0xFFC2FFC2;
      break;
    case 590 ... 599: /* BF59x has no AMC.  */
    default:
      hw_abort (me, "no support for EBIU AMC on this Blackfin model yet");
    }

  attach_bfin_ebiu_amc_regs (me, amc, reg_size);

  bfin_ebiu_amc_write_amgctl (me, amc, amgctl);
}

const struct hw_descriptor dv_bfin_ebiu_amc_descriptor[] =
{
  {"bfin_ebiu_amc", bfin_ebiu_amc_finish,},
  {NULL, NULL},
};
