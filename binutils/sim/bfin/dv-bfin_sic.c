/* Blackfin System Interrupt Controller (SIC) model.

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
#include "dv-bfin_sic.h"
#include "dv-bfin_cec.h"

struct bfin_sic
{
  /* We assume first element is the base.  */
  bu32 base;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(swrst);
  bu16 BFIN_MMR_16(syscr);
  bu16 BFIN_MMR_16(rvect);  /* XXX: BF59x has a 32bit AUX_REVID here.  */
  union {
    struct {
      bu32 imask0;
      bu32 iar0, iar1, iar2, iar3;
      bu32 isr0, iwr0;
      bu32 _pad0[9];
      bu32 imask1;
      bu32 iar4, iar5, iar6, iar7;
      bu32 isr1, iwr1;
    } bf52x;
    struct {
      bu32 imask;
      bu32 iar0, iar1, iar2, iar3;
      bu32 isr, iwr;
    } bf537;
    struct {
      bu32 imask0, imask1, imask2;
      bu32 isr0, isr1, isr2;
      bu32 iwr0, iwr1, iwr2;
      bu32 iar0, iar1, iar2, iar3;
      bu32 iar4, iar5, iar6, iar7;
      bu32 iar8, iar9, iar10, iar11;
    } bf54x;
    struct {
      bu32 imask0, imask1;
      bu32 iar0, iar1, iar2, iar3;
      bu32 iar4, iar5, iar6, iar7;
      bu32 isr0, isr1;
      bu32 iwr0, iwr1;
    } bf561;
  };
};
#define mmr_base()      offsetof(struct bfin_sic, swrst)
#define mmr_offset(mmr) (offsetof(struct bfin_sic, mmr) - mmr_base())
#define mmr_idx(mmr)    (mmr_offset (mmr) / 4)

static const char * const bf52x_mmr_names[] =
{
  "SWRST", "SYSCR", "SIC_RVECT", "SIC_IMASK0", "SIC_IAR0", "SIC_IAR1",
  "SIC_IAR2", "SIC_IAR3", "SIC_ISR0", "SIC_IWR0",
  [mmr_idx (bf52x.imask1)] = "SIC_IMASK1", "SIC_IAR4", "SIC_IAR5",
  "SIC_IAR6", "SIC_IAR7", "SIC_ISR1", "SIC_IWR1",
};
static const char * const bf537_mmr_names[] =
{
  "SWRST", "SYSCR", "SIC_RVECT", "SIC_IMASK", "SIC_IAR0", "SIC_IAR1",
  "SIC_IAR2", "SIC_IAR3", "SIC_ISR", "SIC_IWR",
};
static const char * const bf54x_mmr_names[] =
{
  "SWRST", "SYSCR", "SIC_RVECT", "SIC_IMASK0", "SIC_IMASK1", "SIC_IMASK2",
  "SIC_ISR0", "SIC_ISR1", "SIC_ISR2", "SIC_IWR0", "SIC_IWR1", "SIC_IWR2",
  "SIC_IAR0", "SIC_IAR1", "SIC_IAR2", "SIC_IAR3",
  "SIC_IAR4", "SIC_IAR5", "SIC_IAR6", "SIC_IAR7",
  "SIC_IAR8", "SIC_IAR9", "SIC_IAR10", "SIC_IAR11",
};
static const char * const bf561_mmr_names[] =
{
  "SWRST", "SYSCR", "SIC_RVECT", "SIC_IMASK0", "SIC_IMASK1",
  "SIC_IAR0", "SIC_IAR1", "SIC_IAR2", "SIC_IAR3",
  "SIC_IAR4", "SIC_IAR5", "SIC_IAR6", "SIC_IAR7",
  "SIC_ISR0", "SIC_ISR1", "SIC_IWR0", "SIC_IWR1",
};
static const char * const *mmr_names;
#define mmr_name(off) (mmr_names[(off) / 4] ? : "<INV>")

static void
bfin_sic_forward_interrupts (struct hw *me, bu32 *isr, bu32 *imask, bu32 *iar)
{
  int my_port;
  bu32 ipend;

  /* Process pending and unmasked interrupts.  */
  ipend = *isr & *imask;

  /* Usually none are pending unmasked, so avoid bit twiddling.  */
  if (!ipend)
    return;

  for (my_port = 0; my_port < 32; ++my_port)
    {
      bu32 iar_idx, iar_off, iar_val;
      bu32 bit = (1 << my_port);

      /* This bit isn't pending, so check next one.  */
      if (!(ipend & bit))
	continue;

      /* The IAR registers map the System input to the Core output.
         Every 4 bits in the IAR are used to map to IVG{7..15}.  */
      iar_idx = my_port / 8;
      iar_off = (my_port % 8) * 4;
      iar_val = (iar[iar_idx] & (0xf << iar_off)) >> iar_off;
      HW_TRACE ((me, "forwarding int %i to CEC", IVG7 + iar_val));
      hw_port_event (me, IVG7 + iar_val, 1);
    }
}

static void
bfin_sic_52x_forward_interrupts (struct hw *me, struct bfin_sic *sic)
{
  bfin_sic_forward_interrupts (me, &sic->bf52x.isr0, &sic->bf52x.imask0, &sic->bf52x.iar0);
  bfin_sic_forward_interrupts (me, &sic->bf52x.isr1, &sic->bf52x.imask1, &sic->bf52x.iar4);
}

static unsigned
bfin_sic_52x_io_write_buffer (struct hw *me, const void *source, int space,
			      address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, true))
    return 0;

  if (nr_bytes == 4)
    value = dv_load_4 (source);
  else
    value = dv_load_2 (source);

  mmr_off = addr - sic->base;
  valuep = (void *)((uintptr_t)sic + mmr_base() + mmr_off);
  value32p = valuep;

  HW_TRACE_WRITE ();

  /* XXX: Discard all SIC writes for now.  */
  switch (mmr_off)
    {
    case mmr_offset(swrst):
      /* XXX: This should trigger a software reset ...  */
      break;
    case mmr_offset(syscr):
      /* XXX: what to do ...  */
      break;
    case mmr_offset(bf52x.imask0):
    case mmr_offset(bf52x.imask1):
      bfin_sic_52x_forward_interrupts (me, sic);
      *value32p = value;
      break;
    case mmr_offset(bf52x.iar0) ... mmr_offset(bf52x.iar3):
    case mmr_offset(bf52x.iar4) ... mmr_offset(bf52x.iar7):
    case mmr_offset(bf52x.iwr0):
    case mmr_offset(bf52x.iwr1):
      *value32p = value;
      break;
    case mmr_offset(bf52x.isr0):
    case mmr_offset(bf52x.isr1):
      /* ISR is read-only.  */
      break;
    default:
      /* XXX: Should discard other writes.  */
      ;
    }

  return nr_bytes;
}

static unsigned
bfin_sic_52x_io_read_buffer (struct hw *me, void *dest, int space,
			     address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - sic->base;
  valuep = (void *)((uintptr_t)sic + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(swrst):
    case mmr_offset(syscr):
    case mmr_offset(rvect):
      dv_store_2 (dest, *value16p);
      break;
    case mmr_offset(bf52x.imask0):
    case mmr_offset(bf52x.imask1):
    case mmr_offset(bf52x.iar0) ... mmr_offset(bf52x.iar3):
    case mmr_offset(bf52x.iar4) ... mmr_offset(bf52x.iar7):
    case mmr_offset(bf52x.iwr0):
    case mmr_offset(bf52x.iwr1):
    case mmr_offset(bf52x.isr0):
    case mmr_offset(bf52x.isr1):
      dv_store_4 (dest, *value32p);
      break;
    default:
      if (nr_bytes == 2)
	dv_store_2 (dest, 0);
      else
	dv_store_4 (dest, 0);
      break;
    }

  return nr_bytes;
}

static void
bfin_sic_537_forward_interrupts (struct hw *me, struct bfin_sic *sic)
{
  bfin_sic_forward_interrupts (me, &sic->bf537.isr, &sic->bf537.imask, &sic->bf537.iar0);
}

static unsigned
bfin_sic_537_io_write_buffer (struct hw *me, const void *source, int space,
			      address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, true))
    return 0;

  if (nr_bytes == 4)
    value = dv_load_4 (source);
  else
    value = dv_load_2 (source);

  mmr_off = addr - sic->base;
  valuep = (void *)((uintptr_t)sic + mmr_base() + mmr_off);
  value32p = valuep;

  HW_TRACE_WRITE ();

  /* XXX: Discard all SIC writes for now.  */
  switch (mmr_off)
    {
    case mmr_offset(swrst):
      /* XXX: This should trigger a software reset ...  */
      break;
    case mmr_offset(syscr):
      /* XXX: what to do ...  */
      break;
    case mmr_offset(bf537.imask):
      bfin_sic_537_forward_interrupts (me, sic);
      *value32p = value;
      break;
    case mmr_offset(bf537.iar0):
    case mmr_offset(bf537.iar1):
    case mmr_offset(bf537.iar2):
    case mmr_offset(bf537.iar3):
    case mmr_offset(bf537.iwr):
      *value32p = value;
      break;
    case mmr_offset(bf537.isr):
      /* ISR is read-only.  */
      break;
    default:
      /* XXX: Should discard other writes.  */
      ;
    }

  return nr_bytes;
}

static unsigned
bfin_sic_537_io_read_buffer (struct hw *me, void *dest, int space,
			     address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - sic->base;
  valuep = (void *)((uintptr_t)sic + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(swrst):
    case mmr_offset(syscr):
    case mmr_offset(rvect):
      dv_store_2 (dest, *value16p);
      break;
    case mmr_offset(bf537.imask):
    case mmr_offset(bf537.iar0):
    case mmr_offset(bf537.iar1):
    case mmr_offset(bf537.iar2):
    case mmr_offset(bf537.iar3):
    case mmr_offset(bf537.isr):
    case mmr_offset(bf537.iwr):
      dv_store_4 (dest, *value32p);
      break;
    default:
      if (nr_bytes == 2)
	dv_store_2 (dest, 0);
      else
	dv_store_4 (dest, 0);
      break;
    }

  return nr_bytes;
}

static void
bfin_sic_54x_forward_interrupts (struct hw *me, struct bfin_sic *sic)
{
  bfin_sic_forward_interrupts (me, &sic->bf54x.isr0, &sic->bf54x.imask0, &sic->bf54x.iar0);
  bfin_sic_forward_interrupts (me, &sic->bf54x.isr1, &sic->bf54x.imask1, &sic->bf54x.iar4);
  bfin_sic_forward_interrupts (me, &sic->bf54x.isr2, &sic->bf54x.imask2, &sic->bf54x.iar8);
}

static unsigned
bfin_sic_54x_io_write_buffer (struct hw *me, const void *source, int space,
			      address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, true))
    return 0;

  if (nr_bytes == 4)
    value = dv_load_4 (source);
  else
    value = dv_load_2 (source);

  mmr_off = addr - sic->base;
  valuep = (void *)((uintptr_t)sic + mmr_base() + mmr_off);
  value32p = valuep;

  HW_TRACE_WRITE ();

  /* XXX: Discard all SIC writes for now.  */
  switch (mmr_off)
    {
    case mmr_offset(swrst):
      /* XXX: This should trigger a software reset ...  */
      break;
    case mmr_offset(syscr):
      /* XXX: what to do ...  */
      break;
    case mmr_offset(bf54x.imask0) ... mmr_offset(bf54x.imask2):
      bfin_sic_54x_forward_interrupts (me, sic);
      *value32p = value;
      break;
    case mmr_offset(bf54x.iar0) ... mmr_offset(bf54x.iar11):
    case mmr_offset(bf54x.iwr0) ... mmr_offset(bf54x.iwr2):
      *value32p = value;
      break;
    case mmr_offset(bf54x.isr0) ... mmr_offset(bf54x.isr2):
      /* ISR is read-only.  */
      break;
    default:
      /* XXX: Should discard other writes.  */
      ;
    }

  return nr_bytes;
}

static unsigned
bfin_sic_54x_io_read_buffer (struct hw *me, void *dest, int space,
			     address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - sic->base;
  valuep = (void *)((uintptr_t)sic + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(swrst):
    case mmr_offset(syscr):
    case mmr_offset(rvect):
      dv_store_2 (dest, *value16p);
      break;
    case mmr_offset(bf54x.imask0) ... mmr_offset(bf54x.imask2):
    case mmr_offset(bf54x.iar0) ... mmr_offset(bf54x.iar11):
    case mmr_offset(bf54x.iwr0) ... mmr_offset(bf54x.iwr2):
    case mmr_offset(bf54x.isr0) ... mmr_offset(bf54x.isr2):
      dv_store_4 (dest, *value32p);
      break;
    default:
      if (nr_bytes == 2)
	dv_store_2 (dest, 0);
      else
	dv_store_4 (dest, 0);
      break;
    }

  return nr_bytes;
}

static void
bfin_sic_561_forward_interrupts (struct hw *me, struct bfin_sic *sic)
{
  bfin_sic_forward_interrupts (me, &sic->bf561.isr0, &sic->bf561.imask0, &sic->bf561.iar0);
  bfin_sic_forward_interrupts (me, &sic->bf561.isr1, &sic->bf561.imask1, &sic->bf561.iar4);
}

static unsigned
bfin_sic_561_io_write_buffer (struct hw *me, const void *source, int space,
			      address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, true))
    return 0;

  if (nr_bytes == 4)
    value = dv_load_4 (source);
  else
    value = dv_load_2 (source);

  mmr_off = addr - sic->base;
  valuep = (void *)((uintptr_t)sic + mmr_base() + mmr_off);
  value32p = valuep;

  HW_TRACE_WRITE ();

  /* XXX: Discard all SIC writes for now.  */
  switch (mmr_off)
    {
    case mmr_offset(swrst):
      /* XXX: This should trigger a software reset ...  */
      break;
    case mmr_offset(syscr):
      /* XXX: what to do ...  */
      break;
    case mmr_offset(bf561.imask0):
    case mmr_offset(bf561.imask1):
      bfin_sic_561_forward_interrupts (me, sic);
      *value32p = value;
      break;
    case mmr_offset(bf561.iar0) ... mmr_offset(bf561.iar3):
    case mmr_offset(bf561.iar4) ... mmr_offset(bf561.iar7):
    case mmr_offset(bf561.iwr0):
    case mmr_offset(bf561.iwr1):
      *value32p = value;
      break;
    case mmr_offset(bf561.isr0):
    case mmr_offset(bf561.isr1):
      /* ISR is read-only.  */
      break;
    default:
      /* XXX: Should discard other writes.  */
      ;
    }

  return nr_bytes;
}

static unsigned
bfin_sic_561_io_read_buffer (struct hw *me, void *dest, int space,
			     address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - sic->base;
  valuep = (void *)((uintptr_t)sic + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(swrst):
    case mmr_offset(syscr):
    case mmr_offset(rvect):
      dv_store_2 (dest, *value16p);
      break;
    case mmr_offset(bf561.imask0):
    case mmr_offset(bf561.imask1):
    case mmr_offset(bf561.iar0) ... mmr_offset(bf561.iar3):
    case mmr_offset(bf561.iar4) ... mmr_offset(bf561.iar7):
    case mmr_offset(bf561.iwr0):
    case mmr_offset(bf561.iwr1):
    case mmr_offset(bf561.isr0):
    case mmr_offset(bf561.isr1):
      dv_store_4 (dest, *value32p);
      break;
    default:
      if (nr_bytes == 2)
	dv_store_2 (dest, 0);
      else
	dv_store_4 (dest, 0);
      break;
    }

  return nr_bytes;
}

/* Give each SIC its own base to make it easier to extract the pin at
   runtime.  The pin is used as its bit position in the SIC MMRs.  */
#define ENC(sic, pin) (((sic) << 8) + (pin))
#define DEC_PIN(pin) ((pin) % 0x100)
#define DEC_SIC(pin) ((pin) >> 8)

/* It would be nice to declare just one set of input_ports, and then
   have the device tree instantiate multiple SICs, but the MMR layout
   on the BF54x/BF561 makes this pretty hard to pull off since their
   regs are interwoven in the address space.  */

#define BFIN_SIC_TO_CEC_PORTS \
  { "ivg7",  IVG7,  0, output_port, }, \
  { "ivg8",  IVG8,  0, output_port, }, \
  { "ivg9",  IVG9,  0, output_port, }, \
  { "ivg10", IVG10, 0, output_port, }, \
  { "ivg11", IVG11, 0, output_port, }, \
  { "ivg12", IVG12, 0, output_port, }, \
  { "ivg13", IVG13, 0, output_port, }, \
  { "ivg14", IVG14, 0, output_port, }, \
  { "ivg15", IVG15, 0, output_port, },

#define SIC_PORTS(n) \
  { "int0@"#n,   ENC(n,  0), 0, input_port, }, \
  { "int1@"#n,   ENC(n,  1), 0, input_port, }, \
  { "int2@"#n,   ENC(n,  2), 0, input_port, }, \
  { "int3@"#n,   ENC(n,  3), 0, input_port, }, \
  { "int4@"#n,   ENC(n,  4), 0, input_port, }, \
  { "int5@"#n,   ENC(n,  5), 0, input_port, }, \
  { "int6@"#n,   ENC(n,  6), 0, input_port, }, \
  { "int7@"#n,   ENC(n,  7), 0, input_port, }, \
  { "int8@"#n,   ENC(n,  8), 0, input_port, }, \
  { "int9@"#n,   ENC(n,  9), 0, input_port, }, \
  { "int10@"#n,  ENC(n, 10), 0, input_port, }, \
  { "int11@"#n,  ENC(n, 11), 0, input_port, }, \
  { "int12@"#n,  ENC(n, 12), 0, input_port, }, \
  { "int13@"#n,  ENC(n, 13), 0, input_port, }, \
  { "int14@"#n,  ENC(n, 14), 0, input_port, }, \
  { "int15@"#n,  ENC(n, 15), 0, input_port, }, \
  { "int16@"#n,  ENC(n, 16), 0, input_port, }, \
  { "int17@"#n,  ENC(n, 17), 0, input_port, }, \
  { "int18@"#n,  ENC(n, 18), 0, input_port, }, \
  { "int19@"#n,  ENC(n, 19), 0, input_port, }, \
  { "int20@"#n,  ENC(n, 20), 0, input_port, }, \
  { "int21@"#n,  ENC(n, 21), 0, input_port, }, \
  { "int22@"#n,  ENC(n, 22), 0, input_port, }, \
  { "int23@"#n,  ENC(n, 23), 0, input_port, }, \
  { "int24@"#n,  ENC(n, 24), 0, input_port, }, \
  { "int25@"#n,  ENC(n, 25), 0, input_port, }, \
  { "int26@"#n,  ENC(n, 26), 0, input_port, }, \
  { "int27@"#n,  ENC(n, 27), 0, input_port, }, \
  { "int28@"#n,  ENC(n, 28), 0, input_port, }, \
  { "int29@"#n,  ENC(n, 29), 0, input_port, }, \
  { "int30@"#n,  ENC(n, 30), 0, input_port, }, \
  { "int31@"#n,  ENC(n, 31), 0, input_port, },

static const struct hw_port_descriptor bfin_sic1_ports[] =
{
  BFIN_SIC_TO_CEC_PORTS
  SIC_PORTS(0)
  { NULL, 0, 0, 0, },
};

static const struct hw_port_descriptor bfin_sic2_ports[] =
{
  BFIN_SIC_TO_CEC_PORTS
  SIC_PORTS(0)
  SIC_PORTS(1)
  { NULL, 0, 0, 0, },
};

static const struct hw_port_descriptor bfin_sic3_ports[] =
{
  BFIN_SIC_TO_CEC_PORTS
  SIC_PORTS(0)
  SIC_PORTS(1)
  SIC_PORTS(2)
  { NULL, 0, 0, 0, },
};

static const struct hw_port_descriptor bfin_sic_561_ports[] =
{
  { "sup_irq@0", 0, 0, output_port, },
  { "sup_irq@1", 1, 0, output_port, },
  BFIN_SIC_TO_CEC_PORTS
  SIC_PORTS(0)
  SIC_PORTS(1)
  { NULL, 0, 0, 0, },
};

static void
bfin_sic_port_event (struct hw *me, bu32 *isr, bu32 bit, int level)
{
  if (level)
    *isr |= bit;
  else
    *isr &= ~bit;
}

static void
bfin_sic_52x_port_event (struct hw *me, int my_port, struct hw *source,
			 int source_port, int level)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 idx = DEC_SIC (my_port);
  bu32 pin = DEC_PIN (my_port);
  bu32 bit = 1 << pin;

  HW_TRACE ((me, "processing level %i from port %i (SIC %u pin %u)",
	     level, my_port, idx, pin));

  /* SIC only exists to forward interrupts from the system to the CEC.  */
  switch (idx)
    {
    case 0: bfin_sic_port_event (me, &sic->bf52x.isr0, bit, level); break;
    case 1: bfin_sic_port_event (me, &sic->bf52x.isr1, bit, level); break;
    }

  /* XXX: Handle SIC wakeup source ?
  if (sic->bf52x.iwr0 & bit)
    What to do ?;
  if (sic->bf52x.iwr1 & bit)
    What to do ?;
   */

  bfin_sic_52x_forward_interrupts (me, sic);
}

static void
bfin_sic_537_port_event (struct hw *me, int my_port, struct hw *source,
			 int source_port, int level)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 idx = DEC_SIC (my_port);
  bu32 pin = DEC_PIN (my_port);
  bu32 bit = 1 << pin;

  HW_TRACE ((me, "processing level %i from port %i (SIC %u pin %u)",
	     level, my_port, idx, pin));

  /* SIC only exists to forward interrupts from the system to the CEC.  */
  bfin_sic_port_event (me, &sic->bf537.isr, bit, level);

  /* XXX: Handle SIC wakeup source ?
  if (sic->bf537.iwr & bit)
    What to do ?;
   */

  bfin_sic_537_forward_interrupts (me, sic);
}

static void
bfin_sic_54x_port_event (struct hw *me, int my_port, struct hw *source,
			 int source_port, int level)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 idx = DEC_SIC (my_port);
  bu32 pin = DEC_PIN (my_port);
  bu32 bit = 1 << pin;

  HW_TRACE ((me, "processing level %i from port %i (SIC %u pin %u)",
	     level, my_port, idx, pin));

  /* SIC only exists to forward interrupts from the system to the CEC.  */
  switch (idx)
    {
    case 0: bfin_sic_port_event (me, &sic->bf54x.isr0, bit, level); break;
    case 1: bfin_sic_port_event (me, &sic->bf54x.isr0, bit, level); break;
    case 2: bfin_sic_port_event (me, &sic->bf54x.isr0, bit, level); break;
    }

  /* XXX: Handle SIC wakeup source ?
  if (sic->bf54x.iwr0 & bit)
    What to do ?;
  if (sic->bf54x.iwr1 & bit)
    What to do ?;
  if (sic->bf54x.iwr2 & bit)
    What to do ?;
   */

  bfin_sic_54x_forward_interrupts (me, sic);
}

static void
bfin_sic_561_port_event (struct hw *me, int my_port, struct hw *source,
			 int source_port, int level)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 idx = DEC_SIC (my_port);
  bu32 pin = DEC_PIN (my_port);
  bu32 bit = 1 << pin;

  HW_TRACE ((me, "processing level %i from port %i (SIC %u pin %u)",
	     level, my_port, idx, pin));

  /* SIC only exists to forward interrupts from the system to the CEC.  */
  switch (idx)
    {
    case 0: bfin_sic_port_event (me, &sic->bf561.isr0, bit, level); break;
    case 1: bfin_sic_port_event (me, &sic->bf561.isr1, bit, level); break;
    }

  /* XXX: Handle SIC wakeup source ?
  if (sic->bf561.iwr0 & bit)
    What to do ?;
  if (sic->bf561.iwr1 & bit)
    What to do ?;
   */

  bfin_sic_561_forward_interrupts (me, sic);
}

static void
attach_bfin_sic_regs (struct hw *me, struct bfin_sic *sic)
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

  if (attach_size != BFIN_MMR_SIC_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_SIC_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  sic->base = attach_address;
}

static void
bfin_sic_finish (struct hw *me)
{
  struct bfin_sic *sic;

  sic = HW_ZALLOC (me, struct bfin_sic);

  set_hw_data (me, sic);
  attach_bfin_sic_regs (me, sic);

  switch (hw_find_integer_property (me, "type"))
    {
    case 500 ... 509:
      set_hw_io_read_buffer (me, bfin_sic_52x_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_52x_io_write_buffer);
      set_hw_ports (me, bfin_sic2_ports);
      set_hw_port_event (me, bfin_sic_52x_port_event);
      mmr_names = bf52x_mmr_names;

      /* Initialize the SIC.  */
      sic->bf52x.imask0 = sic->bf52x.imask1 = 0;
      sic->bf52x.isr0 = sic->bf52x.isr1 = 0;
      sic->bf52x.iwr0 = sic->bf52x.iwr1 = 0xFFFFFFFF;
      sic->bf52x.iar0 = 0x00000000;
      sic->bf52x.iar1 = 0x22111000;
      sic->bf52x.iar2 = 0x33332222;
      sic->bf52x.iar3 = 0x44444433;
      sic->bf52x.iar4 = 0x55555555;
      sic->bf52x.iar5 = 0x06666655;
      sic->bf52x.iar6 = 0x33333003;
      sic->bf52x.iar7 = 0x00000000;	/* XXX: Find and fix */
      break;
    case 510 ... 519:
      set_hw_io_read_buffer (me, bfin_sic_52x_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_52x_io_write_buffer);
      set_hw_ports (me, bfin_sic2_ports);
      set_hw_port_event (me, bfin_sic_52x_port_event);
      mmr_names = bf52x_mmr_names;

      /* Initialize the SIC.  */
      sic->bf52x.imask0 = sic->bf52x.imask1 = 0;
      sic->bf52x.isr0 = sic->bf52x.isr1 = 0;
      sic->bf52x.iwr0 = sic->bf52x.iwr1 = 0xFFFFFFFF;
      sic->bf52x.iar0 = 0x00000000;
      sic->bf52x.iar1 = 0x11000000;
      sic->bf52x.iar2 = 0x33332222;
      sic->bf52x.iar3 = 0x44444433;
      sic->bf52x.iar4 = 0x55555555;
      sic->bf52x.iar5 = 0x06666655;
      sic->bf52x.iar6 = 0x33333000;
      sic->bf52x.iar7 = 0x00000000;	/* XXX: Find and fix */
      break;
    case 522 ... 527:
      set_hw_io_read_buffer (me, bfin_sic_52x_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_52x_io_write_buffer);
      set_hw_ports (me, bfin_sic2_ports);
      set_hw_port_event (me, bfin_sic_52x_port_event);
      mmr_names = bf52x_mmr_names;

      /* Initialize the SIC.  */
      sic->bf52x.imask0 = sic->bf52x.imask1 = 0;
      sic->bf52x.isr0 = sic->bf52x.isr1 = 0;
      sic->bf52x.iwr0 = sic->bf52x.iwr1 = 0xFFFFFFFF;
      sic->bf52x.iar0 = 0x00000000;
      sic->bf52x.iar1 = 0x11000000;
      sic->bf52x.iar2 = 0x33332222;
      sic->bf52x.iar3 = 0x44444433;
      sic->bf52x.iar4 = 0x55555555;
      sic->bf52x.iar5 = 0x06666655;
      sic->bf52x.iar6 = 0x33333000;
      sic->bf52x.iar7 = 0x00000000;	/* XXX: Find and fix */
      break;
    case 531 ... 533:
      set_hw_io_read_buffer (me, bfin_sic_537_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_537_io_write_buffer);
      set_hw_ports (me, bfin_sic1_ports);
      set_hw_port_event (me, bfin_sic_537_port_event);
      mmr_names = bf537_mmr_names;

      /* Initialize the SIC.  */
      sic->bf537.imask = 0;
      sic->bf537.isr = 0;
      sic->bf537.iwr = 0xFFFFFFFF;
      sic->bf537.iar0 = 0x10000000;
      sic->bf537.iar1 = 0x33322221;
      sic->bf537.iar2 = 0x66655444;
      sic->bf537.iar3 = 0; /* XXX: fix this */
      break;
    case 534:
    case 536:
    case 537:
      set_hw_io_read_buffer (me, bfin_sic_537_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_537_io_write_buffer);
      set_hw_ports (me, bfin_sic1_ports);
      set_hw_port_event (me, bfin_sic_537_port_event);
      mmr_names = bf537_mmr_names;

      /* Initialize the SIC.  */
      sic->bf537.imask = 0;
      sic->bf537.isr = 0;
      sic->bf537.iwr = 0xFFFFFFFF;
      sic->bf537.iar0 = 0x22211000;
      sic->bf537.iar1 = 0x43333332;
      sic->bf537.iar2 = 0x55555444;
      sic->bf537.iar3 = 0x66655555;
      break;
    case 538 ... 539:
      set_hw_io_read_buffer (me, bfin_sic_52x_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_52x_io_write_buffer);
      set_hw_ports (me, bfin_sic2_ports);
      set_hw_port_event (me, bfin_sic_52x_port_event);
      mmr_names = bf52x_mmr_names;

      /* Initialize the SIC.  */
      sic->bf52x.imask0 = sic->bf52x.imask1 = 0;
      sic->bf52x.isr0 = sic->bf52x.isr1 = 0;
      sic->bf52x.iwr0 = sic->bf52x.iwr1 = 0xFFFFFFFF;
      sic->bf52x.iar0 = 0x10000000;
      sic->bf52x.iar1 = 0x33322221;
      sic->bf52x.iar2 = 0x66655444;
      sic->bf52x.iar3 = 0x00000000;
      sic->bf52x.iar4 = 0x32222220;
      sic->bf52x.iar5 = 0x44433333;
      sic->bf52x.iar6 = 0x00444664;
      sic->bf52x.iar7 = 0x00000000;	/* XXX: Find and fix */
      break;
    case 540 ... 549:
      set_hw_io_read_buffer (me, bfin_sic_54x_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_54x_io_write_buffer);
      set_hw_ports (me, bfin_sic3_ports);
      set_hw_port_event (me, bfin_sic_54x_port_event);
      mmr_names = bf54x_mmr_names;

      /* Initialize the SIC.  */
      sic->bf54x.imask0 = sic->bf54x.imask1 = sic->bf54x.imask2 = 0;
      sic->bf54x.isr0 = sic->bf54x.isr1 = sic->bf54x.isr2 = 0;
      sic->bf54x.iwr0 = sic->bf54x.iwr1 = sic->bf54x.iwr2 = 0xFFFFFFFF;
      sic->bf54x.iar0 = 0x10000000;
      sic->bf54x.iar1 = 0x33322221;
      sic->bf54x.iar2 = 0x66655444;
      sic->bf54x.iar3 = 0x00000000;
      sic->bf54x.iar4 = 0x32222220;
      sic->bf54x.iar5 = 0x44433333;
      sic->bf54x.iar6 = 0x00444664;
      sic->bf54x.iar7 = 0x00000000;
      sic->bf54x.iar8 = 0x44111111;
      sic->bf54x.iar9 = 0x44444444;
      sic->bf54x.iar10 = 0x44444444;
      sic->bf54x.iar11 = 0x55444444;
      break;
    case 561:
      set_hw_io_read_buffer (me, bfin_sic_561_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_561_io_write_buffer);
      set_hw_ports (me, bfin_sic_561_ports);
      set_hw_port_event (me, bfin_sic_561_port_event);
      mmr_names = bf561_mmr_names;

      /* Initialize the SIC.  */
      sic->bf561.imask0 = sic->bf561.imask1 = 0;
      sic->bf561.isr0 = sic->bf561.isr1 = 0;
      sic->bf561.iwr0 = sic->bf561.iwr1 = 0xFFFFFFFF;
      sic->bf561.iar0 = 0x00000000;
      sic->bf561.iar1 = 0x11111000;
      sic->bf561.iar2 = 0x21111111;
      sic->bf561.iar3 = 0x22222222;
      sic->bf561.iar4 = 0x33333222;
      sic->bf561.iar5 = 0x43333333;
      sic->bf561.iar6 = 0x21144444;
      sic->bf561.iar7 = 0x00006552;
      break;
    case 590 ... 599:
      set_hw_io_read_buffer (me, bfin_sic_537_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_537_io_write_buffer);
      set_hw_ports (me, bfin_sic1_ports);
      set_hw_port_event (me, bfin_sic_537_port_event);
      mmr_names = bf537_mmr_names;

      /* Initialize the SIC.  */
      sic->bf537.imask = 0;
      sic->bf537.isr = 0;
      sic->bf537.iwr = 0xFFFFFFFF;
      sic->bf537.iar0 = 0x00000000;
      sic->bf537.iar1 = 0x33322221;
      sic->bf537.iar2 = 0x55444443;
      sic->bf537.iar3 = 0x66600005;
      break;
    default:
      hw_abort (me, "no support for SIC on this Blackfin model yet");
    }
}

const struct hw_descriptor dv_bfin_sic_descriptor[] =
{
  {"bfin_sic", bfin_sic_finish,},
  {NULL, NULL},
};
