/* Blackfin Trace (TBUF) model.

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
#include "dv-bfin_cec.h"
#include "dv-bfin_trace.h"

/* Note: The circular buffering here might look a little buggy wrt mid-reads
         and consuming the top entry, but this is simulating hardware behavior.
         The hardware is simple, dumb, and fast.  Don't write dumb Blackfin
         software and you won't have a problem.  */

/* The hardware is limited to 16 entries and defines TBUFCTL.  Let's extend it ;).  */
#ifndef SIM_BFIN_TRACE_DEPTH
#define SIM_BFIN_TRACE_DEPTH 6
#endif
#define SIM_BFIN_TRACE_LEN (1 << SIM_BFIN_TRACE_DEPTH)
#define SIM_BFIN_TRACE_LEN_MASK (SIM_BFIN_TRACE_LEN - 1)

struct bfin_trace_entry
{
  bu32 src, dst;
};
struct bfin_trace
{
  bu32 base;
  struct bfin_trace_entry buffer[SIM_BFIN_TRACE_LEN];
  int top, bottom;
  bool mid;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 tbufctl, tbufstat;
  char _pad[0x100 - 0x8];
  bu32 tbuf;
};
#define mmr_base()      offsetof(struct bfin_trace, tbufctl)
#define mmr_offset(mmr) (offsetof(struct bfin_trace, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "TBUFCTL", "TBUFSTAT", [mmr_offset (tbuf) / 4] = "TBUF",
};
#define mmr_name(off) (mmr_names[(off) / 4] ? : "<INV>")

/* Ugh, circular buffers.  */
#define TBUF_LEN(t) ((t)->top - (t)->bottom)
#define TBUF_IDX(i) ((i) & SIM_BFIN_TRACE_LEN_MASK)
/* TOP is the next slot to fill.  */
#define TBUF_TOP(t) (&(t)->buffer[TBUF_IDX ((t)->top)])
/* LAST is the latest valid slot.  */
#define TBUF_LAST(t) (&(t)->buffer[TBUF_IDX ((t)->top - 1)])
/* LAST_LAST is the second-to-last valid slot.  */
#define TBUF_LAST_LAST(t) (&(t)->buffer[TBUF_IDX ((t)->top - 2)])

static unsigned
bfin_trace_io_write_buffer (struct hw *me, const void *source,
			    int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_trace *trace = hw_data (me);
  bu32 mmr_off;
  bu32 value;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
    return 0;

  value = dv_load_4 (source);
  mmr_off = addr - trace->base;

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(tbufctl):
      trace->tbufctl = value;
      break;
    case mmr_offset(tbufstat):
    case mmr_offset(tbuf):
      /* Discard writes to these.  */
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_trace_io_read_buffer (struct hw *me, void *dest,
			   int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_trace *trace = hw_data (me);
  bu32 mmr_off;
  bu32 value;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - trace->base;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(tbufctl):
      value = trace->tbufctl;
      break;
    case mmr_offset(tbufstat):
      /* Hardware is limited to 16 entries, so to stay compatible with
         software, limit the value to 16.  For software algorithms that
         keep reading while (TBUFSTAT != 0), they'll get all of it.  */
      value = min (TBUF_LEN (trace), 16);
      break;
    case mmr_offset(tbuf):
      {
	struct bfin_trace_entry *e;

	if (TBUF_LEN (trace) == 0)
	  {
	    value = 0;
	    break;
	  }

	e = TBUF_LAST (trace);
	if (trace->mid)
	  {
	    value = e->src;
	    --trace->top;
	  }
	else
	  value = e->dst;
	trace->mid = !trace->mid;

	break;
      }
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  dv_store_4 (dest, value);

  return nr_bytes;
}

static void
attach_bfin_trace_regs (struct hw *me, struct bfin_trace *trace)
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

  if (attach_size != BFIN_COREMMR_TRACE_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_COREMMR_TRACE_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  trace->base = attach_address;
}

static void
bfin_trace_finish (struct hw *me)
{
  struct bfin_trace *trace;

  trace = HW_ZALLOC (me, struct bfin_trace);

  set_hw_data (me, trace);
  set_hw_io_read_buffer (me, bfin_trace_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_trace_io_write_buffer);

  attach_bfin_trace_regs (me, trace);
}

const struct hw_descriptor dv_bfin_trace_descriptor[] =
{
  {"bfin_trace", bfin_trace_finish,},
  {NULL, NULL},
};

#define TRACE_STATE(cpu) DV_STATE_CACHED (cpu, trace)

/* This is not re-entrant, but neither is the cpu state, so this shouldn't
   be a big deal ...  */
void bfin_trace_queue (SIM_CPU *cpu, bu32 src_pc, bu32 dst_pc, int hwloop)
{
  struct bfin_trace *trace = TRACE_STATE (cpu);
  struct bfin_trace_entry *e;
  int len, ivg;

  /* Only queue if powered.  */
  if (!(trace->tbufctl & TBUFPWR))
    return;

  /* Only queue if enabled.  */
  if (!(trace->tbufctl & TBUFEN))
    return;

  /* Ignore hardware loops.
     XXX: This is what the hardware does, but an option to ignore
     could be useful for debugging ...  */
  if (hwloop >= 0)
    return;

  /* Only queue if at right level.  */
  ivg = cec_get_ivg (cpu);
  if (ivg == IVG_RST)
    /* XXX: This is what the hardware does, but an option to ignore
            could be useful for debugging ...  */
    return;
  if (ivg <= IVG_EVX && (trace->tbufctl & TBUFOVF))
    /* XXX: This is what the hardware does, but an option to ignore
            could be useful for debugging ... just don't throw an
            exception when full and in EVT{0..3}.  */
    return;

  /* Are we full ?  */
  len = TBUF_LEN (trace);
  if (len == SIM_BFIN_TRACE_LEN)
    {
      if (trace->tbufctl & TBUFOVF)
	{
	  cec_exception (cpu, VEC_OVFLOW);
	  return;
	}

      /* Overwrite next entry.  */
      ++trace->bottom;
    }

  /* One level compression.  */
  if (len >= 1 && (trace->tbufctl & TBUFCMPLP))
    {
      e = TBUF_LAST (trace);
      if (src_pc == (e->src & ~1) && dst_pc == (e->dst & ~1))
	{
	  /* Hardware sets LSB when level is compressed.  */
	  e->dst |= 1;
	  return;
	}
    }

  /* Two level compression.  */
  if (len >= 2 && (trace->tbufctl & TBUFCMPLP_DOUBLE))
    {
      e = TBUF_LAST_LAST (trace);
      if (src_pc == (e->src & ~1) && dst_pc == (e->dst & ~1))
	{
	  /* Hardware sets LSB when level is compressed.  */
	  e->src |= 1;
	  return;
	}
    }

  e = TBUF_TOP (trace);
  e->dst = dst_pc;
  e->src = src_pc;
  ++trace->top;
}
