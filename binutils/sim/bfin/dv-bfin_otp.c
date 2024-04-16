/* Blackfin One-Time Programmable Memory (OTP) model

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
#include "dv-bfin_otp.h"

/* XXX: No public documentation on this interface.  This seems to work
        with the on-chip ROM functions though and was figured out by
        disassembling & walking that code.  */
/* XXX: About only thing that should be done here are CRC fields.  And
        supposedly there is an interrupt that could be generated.  */

struct bfin_otp
{
  bu32 base;

  /* The actual OTP storage -- 0x200 pages, each page is 128bits.
     While certain pages have predefined and/or secure access, we don't
     bother trying to implement that coverage.  All pages are open for
     reading & writing.  */
  bu32 mem[0x200 * 4];

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(control);
  bu16 BFIN_MMR_16(ben);
  bu16 BFIN_MMR_16(status);
  bu32 timing;
  bu32 _pad0[28];
  bu32 data0, data1, data2, data3;
};
#define mmr_base()      offsetof(struct bfin_otp, control)
#define mmr_offset(mmr) (offsetof(struct bfin_otp, mmr) - mmr_base())
#define mmr_idx(mmr)    (mmr_offset (mmr) / 4)

static const char * const mmr_names[] =
{
  "OTP_CONTROL", "OTP_BEN", "OTP_STATUS", "OTP_TIMING",
  [mmr_idx (data0)] = "OTP_DATA0", "OTP_DATA1", "OTP_DATA2", "OTP_DATA3",
};
#define mmr_name(off) (mmr_names[(off) / 4] ? : "<INV>")

/* XXX: This probably misbehaves with big endian hosts.  */
static void
bfin_otp_transfer (struct bfin_otp *otp, void *vdst, void *vsrc)
{
  bu8 *dst = vdst, *src = vsrc;
  int bidx;
  for (bidx = 0; bidx < 16; ++bidx)
    if (otp->ben & (1 << bidx))
      dst[bidx] = src[bidx];
}

static void
bfin_otp_read_page (struct bfin_otp *otp, bu16 page)
{
  bfin_otp_transfer (otp, &otp->data0, &otp->mem[page * 4]);
}

static void
bfin_otp_write_page_val (struct bfin_otp *otp, bu16 page, bu64 val[2])
{
  bfin_otp_transfer (otp, &otp->mem[page * 4], val);
}
static void
bfin_otp_write_page_val2 (struct bfin_otp *otp, bu16 page, bu64 lo, bu64 hi)
{
  bu64 val[2] = { lo, hi };
  bfin_otp_write_page_val (otp, page, val);
}
static void
bfin_otp_write_page (struct bfin_otp *otp, bu16 page)
{
  bfin_otp_write_page_val2 (otp, page, ((bu64)otp->data1 << 32) | otp->data0,
			    ((bu64)otp->data3 << 32) | otp->data2);
}

static unsigned
bfin_otp_io_write_buffer (struct hw *me, const void *source, int space,
			  address_word addr, unsigned nr_bytes)
{
  struct bfin_otp *otp = hw_data (me);
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

  mmr_off = addr - otp->base;
  valuep = (void *)((uintptr_t)otp + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(control):
      {
	int page;

	if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	  return 0;
	/* XXX: Seems like these bits aren't writable.  */
	*value16p = value & 0x39FF;

	/* Low bits seem to be the page address.  */
	page = value & PAGE_ADDR;

	/* Write operation.  */
	if (value & DO_WRITE)
	  bfin_otp_write_page (otp, page);

	/* Read operation.  */
	if (value & DO_READ)
	  bfin_otp_read_page (otp, page);

	otp->status |= STATUS_DONE;

	break;
      }
    case mmr_offset(ben):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      /* XXX: All bits seem to be writable.  */
      *value16p = value;
      break;
    case mmr_offset(status):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, true))
	return 0;
      /* XXX: All bits seem to be W1C.  */
      dv_w1c_2 (value16p, value, -1);
      break;
    case mmr_offset(timing):
    case mmr_offset(data0):
    case mmr_offset(data1):
    case mmr_offset(data2):
    case mmr_offset(data3):
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
bfin_otp_io_read_buffer (struct hw *me, void *dest, int space,
			 address_word addr, unsigned nr_bytes)
{
  struct bfin_otp *otp = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - otp->base;
  valuep = (void *)((uintptr_t)otp + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(control):
    case mmr_offset(ben):
    case mmr_offset(status):
      if (!dv_bfin_mmr_require_16 (me, addr, nr_bytes, false))
	return 0;
      dv_store_2 (dest, *value16p);
      break;
    case mmr_offset(timing):
    case mmr_offset(data0):
    case mmr_offset(data1):
    case mmr_offset(data2):
    case mmr_offset(data3):
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

static void
attach_bfin_otp_regs (struct hw *me, struct bfin_otp *otp)
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

  if (attach_size != BFIN_MMR_OTP_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_OTP_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  otp->base = attach_address;
}

static const struct hw_port_descriptor bfin_otp_ports[] =
{
  { "stat", 0, 0, output_port, },
  { NULL, 0, 0, 0, },
};

static void
bfin_otp_finish (struct hw *me)
{
  char part_str[16];
  struct bfin_otp *otp;
  unsigned int fps03;
  int type = hw_find_integer_property (me, "type");

  otp = HW_ZALLOC (me, struct bfin_otp);

  set_hw_data (me, otp);
  set_hw_io_read_buffer (me, bfin_otp_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_otp_io_write_buffer);
  set_hw_ports (me, bfin_otp_ports);

  attach_bfin_otp_regs (me, otp);

  /* Initialize the OTP.  */
  otp->ben     = 0xFFFF;
  otp->timing  = 0x00001485;

  /* Semi-random value for unique chip id.  */
  bfin_otp_write_page_val2 (otp, FPS00, (uintptr_t)otp, ~(uintptr_t)otp);

  memset (part_str, 0, sizeof (part_str));
  sprintf (part_str, "ADSP-BF%iX", type);
  switch (type)
    {
    case 512:
      fps03 = FPS03_BF512;
      break;
    case 514:
      fps03 = FPS03_BF514;
      break;
    case 516:
      fps03 = FPS03_BF516;
      break;
    case 518:
      fps03 = FPS03_BF518;
      break;
    case 522:
      fps03 = FPS03_BF522;
      break;
    case 523:
      fps03 = FPS03_BF523;
      break;
    case 524:
      fps03 = FPS03_BF524;
      break;
    case 525:
      fps03 = FPS03_BF525;
      break;
    case 526:
      fps03 = FPS03_BF526;
      break;
    case 527:
      fps03 = FPS03_BF527;
      break;
    default:
      fps03 = 0;
      break;
    }
  part_str[14] = (fps03 >> 0);
  part_str[15] = (fps03 >> 8);
  bfin_otp_write_page_val (otp, FPS03, (void *)part_str);
}

const struct hw_descriptor dv_bfin_otp_descriptor[] =
{
  {"bfin_otp", bfin_otp_finish,},
  {NULL, NULL},
};
