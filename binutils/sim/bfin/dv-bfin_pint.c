/* Blackfin Pin Interrupt (PINT) model

   Copyright (C) 2010-2024 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc. and Mike Frysinger.

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
#include "dv-bfin_pint.h"

struct bfin_pint
{
  bu32 base;

  /* Only accessed indirectly via the associated set/clear MMRs.  */
  bu32 mask, edge, invert;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 mask_set;
  bu32 mask_clear;
  bu32 request;
  bu32 assign;
  bu32 edge_set;
  bu32 edge_clear;
  bu32 invert_set;
  bu32 invert_clear;
  bu32 pinstate;
  bu32 latch;
};
#define mmr_base()      offsetof(struct bfin_pint, mask_set)
#define mmr_offset(mmr) (offsetof(struct bfin_pint, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "PINT_MASK_SET", "PINT_MASK_CLEAR", "PINT_REQUEST", "PINT_ASSIGN",
  "PINT_EDGE_SET", "PINT_EDGE_CLEAR", "PINT_INVERT_SET",
  "PINT_INVERT_CLEAR", "PINT_PINSTATE", "PINT_LATCH",
};
#define mmr_name(off) mmr_names[(off) / 4]

static unsigned
bfin_pint_io_write_buffer (struct hw *me, const void *source, int space,
			   address_word addr, unsigned nr_bytes)
{
  struct bfin_pint *pint = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu32 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  /* XXX: The hardware allows 16 or 32 bit accesses ...  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
    return 0;

  if (nr_bytes == 4)
    value = dv_load_4 (source);
  else
    value = dv_load_2 (source);
  mmr_off = addr - pint->base;
  valuep = (void *)((uintptr_t)pint + mmr_base() + mmr_off);

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(request):
    case mmr_offset(assign):
    case mmr_offset(pinstate):
    case mmr_offset(latch):
      *valuep = value;
      break;
    case mmr_offset(mask_set):
      dv_w1c_4 (&pint->mask, value, -1);
      break;
    case mmr_offset(mask_clear):
      pint->mask |= value;
      break;
    case mmr_offset(edge_set):
      dv_w1c_4 (&pint->edge, value, -1);
      break;
    case mmr_offset(edge_clear):
      pint->edge |= value;
      break;
    case mmr_offset(invert_set):
      dv_w1c_4 (&pint->invert, value, -1);
      break;
    case mmr_offset(invert_clear):
      pint->invert |= value;
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

#if 0
  /* If updating masks, make sure we send updated port info.  */
  switch (mmr_off)
    {
    case mmr_offset(dir):
    case mmr_offset(data) ... mmr_offset(toggle):
      bfin_pint_forward_ouput (me, pint, data);
      break;
    case mmr_offset(maska) ... mmr_offset(maska_toggle):
      bfin_pint_forward_int (me, pint, pint->maska, 0);
      break;
    case mmr_offset(maskb) ... mmr_offset(maskb_toggle):
      bfin_pint_forward_int (me, pint, pint->maskb, 1);
      break;
    }
#endif

  return nr_bytes;
}

static unsigned
bfin_pint_io_read_buffer (struct hw *me, void *dest, int space,
			  address_word addr, unsigned nr_bytes)
{
  struct bfin_pint *pint = hw_data (me);
  bu32 mmr_off;
  bu32 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  /* XXX: The hardware allows 16 or 32 bit accesses ...  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - pint->base;
  valuep = (void *)((uintptr_t)pint + mmr_base() + mmr_off);

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(request):
    case mmr_offset(assign):
    case mmr_offset(pinstate):
    case mmr_offset(latch):
      dv_store_4 (dest, *valuep);
      break;
    case mmr_offset(mask_set):
    case mmr_offset(mask_clear):
      dv_store_4 (dest, pint->mask);
      break;
    case mmr_offset(edge_set):
    case mmr_offset(edge_clear):
      dv_store_4 (dest, pint->edge);
      break;
    case mmr_offset(invert_set):
    case mmr_offset(invert_clear):
      dv_store_4 (dest, pint->invert);
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

#define ENC(bmap, piq) (((bmap) << 8) + (piq))

#define PIQ_PORTS(n) \
  { "piq0@"#n,   ENC(n,  0), 0, input_port, }, \
  { "piq1@"#n,   ENC(n,  1), 0, input_port, }, \
  { "piq2@"#n,   ENC(n,  2), 0, input_port, }, \
  { "piq3@"#n,   ENC(n,  3), 0, input_port, }, \
  { "piq4@"#n,   ENC(n,  4), 0, input_port, }, \
  { "piq5@"#n,   ENC(n,  5), 0, input_port, }, \
  { "piq6@"#n,   ENC(n,  6), 0, input_port, }, \
  { "piq7@"#n,   ENC(n,  7), 0, input_port, }, \
  { "piq8@"#n,   ENC(n,  8), 0, input_port, }, \
  { "piq9@"#n,   ENC(n,  9), 0, input_port, }, \
  { "piq10@"#n,  ENC(n, 10), 0, input_port, }, \
  { "piq11@"#n,  ENC(n, 11), 0, input_port, }, \
  { "piq12@"#n,  ENC(n, 12), 0, input_port, }, \
  { "piq13@"#n,  ENC(n, 13), 0, input_port, }, \
  { "piq14@"#n,  ENC(n, 14), 0, input_port, }, \
  { "piq15@"#n,  ENC(n, 15), 0, input_port, }, \
  { "piq16@"#n,  ENC(n, 16), 0, input_port, }, \
  { "piq17@"#n,  ENC(n, 17), 0, input_port, }, \
  { "piq18@"#n,  ENC(n, 18), 0, input_port, }, \
  { "piq19@"#n,  ENC(n, 19), 0, input_port, }, \
  { "piq20@"#n,  ENC(n, 20), 0, input_port, }, \
  { "piq21@"#n,  ENC(n, 21), 0, input_port, }, \
  { "piq22@"#n,  ENC(n, 22), 0, input_port, }, \
  { "piq23@"#n,  ENC(n, 23), 0, input_port, }, \
  { "piq24@"#n,  ENC(n, 24), 0, input_port, }, \
  { "piq25@"#n,  ENC(n, 25), 0, input_port, }, \
  { "piq26@"#n,  ENC(n, 26), 0, input_port, }, \
  { "piq27@"#n,  ENC(n, 27), 0, input_port, }, \
  { "piq28@"#n,  ENC(n, 28), 0, input_port, }, \
  { "piq29@"#n,  ENC(n, 29), 0, input_port, }, \
  { "piq30@"#n,  ENC(n, 30), 0, input_port, }, \
  { "piq31@"#n,  ENC(n, 31), 0, input_port, },

static const struct hw_port_descriptor bfin_pint_ports[] =
{
  { "stat", 0, 0, output_port, },
  PIQ_PORTS(0)
  PIQ_PORTS(1)
  PIQ_PORTS(2)
  PIQ_PORTS(3)
  PIQ_PORTS(4)
  PIQ_PORTS(5)
  PIQ_PORTS(6)
  PIQ_PORTS(7)
  { NULL, 0, 0, 0, },
};

static void
bfin_pint_port_event (struct hw *me, int my_port, struct hw *source,
		      int source_port, int level)
{
  /* XXX: TODO.  */
}

static void
attach_bfin_pint_regs (struct hw *me, struct bfin_pint *pint)
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

  if (attach_size != BFIN_MMR_PINT_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_PINT_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  pint->base = attach_address;
}

static void
bfin_pint_finish (struct hw *me)
{
  struct bfin_pint *pint;

  pint = HW_ZALLOC (me, struct bfin_pint);

  set_hw_data (me, pint);
  set_hw_io_read_buffer (me, bfin_pint_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_pint_io_write_buffer);
  set_hw_ports (me, bfin_pint_ports);
  set_hw_port_event (me, bfin_pint_port_event);

  /* Initialize the PINT.  */
  switch (dv_get_bus_num (me))
    {
    case 0:
      pint->assign = 0x00000101;
      break;
    case 1:
      pint->assign = 0x01010000;
      break;
    case 2:
      pint->assign = 0x00000101;
      break;
    case 3:
      pint->assign = 0x02020303;
      break;
    default:
      /* XXX: Should move this default into device tree.  */
      hw_abort (me, "no support for PINT at this address yet");
    }

  attach_bfin_pint_regs (me, pint);
}

const struct hw_descriptor dv_bfin_pint_descriptor[] =
{
  {"bfin_pint", bfin_pint_finish,},
  {NULL, NULL},
};
