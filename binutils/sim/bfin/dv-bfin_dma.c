/* Blackfin Direct Memory Access (DMA) Channel model.

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

#include <stdlib.h>

#include "sim-main.h"
#include "devices.h"
#include "hw-device.h"
#include "dv-bfin_dma.h"
#include "dv-bfin_dmac.h"

/* Note: This DMA implementation requires the producer to be the master when
         the peer is MDMA.  The source is always a slave.  This way we don't
         have the two DMA devices thrashing each other with one trying to
         write and the other trying to read.  */

struct bfin_dma
{
  /* This top portion matches common dv_bfin struct.  */
  bu32 base;
  struct hw *dma_master;
  bool acked;

  struct hw_event *handler;
  unsigned ele_size;
  struct hw *hw_peer;

  /* Order after here is important -- matches hardware MMR layout.  */
  union {
    struct { bu16 ndpl, ndph; };
    bu32 next_desc_ptr;
  };
  union {
    struct { bu16 sal, sah; };
    bu32 start_addr;
  };
  bu16 BFIN_MMR_16 (config);
  bu32 _pad0;
  bu16 BFIN_MMR_16 (x_count);
  bs16 BFIN_MMR_16 (x_modify);
  bu16 BFIN_MMR_16 (y_count);
  bs16 BFIN_MMR_16 (y_modify);
  bu32 curr_desc_ptr, curr_addr;
  bu16 BFIN_MMR_16 (irq_status);
  bu16 BFIN_MMR_16 (peripheral_map);
  bu16 BFIN_MMR_16 (curr_x_count);
  bu32 _pad1;
  bu16 BFIN_MMR_16 (curr_y_count);
  bu32 _pad2;
};
#define mmr_base()      offsetof(struct bfin_dma, next_desc_ptr)
#define mmr_offset(mmr) (offsetof(struct bfin_dma, mmr) - mmr_base())

static const char * const mmr_names[] =
{
  "NEXT_DESC_PTR", "START_ADDR", "CONFIG", "<INV>", "X_COUNT", "X_MODIFY",
  "Y_COUNT", "Y_MODIFY", "CURR_DESC_PTR", "CURR_ADDR", "IRQ_STATUS",
  "PERIPHERAL_MAP", "CURR_X_COUNT", "<INV>", "CURR_Y_COUNT", "<INV>",
};
#define mmr_name(off) mmr_names[(off) / 4]

static bool
bfin_dma_enabled (struct bfin_dma *dma)
{
  return (dma->config & DMAEN);
}

static bool
bfin_dma_running (struct bfin_dma *dma)
{
  return (dma->irq_status & DMA_RUN);
}

static struct hw *
bfin_dma_get_peer (struct hw *me, struct bfin_dma *dma)
{
  if (dma->hw_peer)
    return dma->hw_peer;
  return dma->hw_peer = bfin_dmac_get_peer (me, dma->peripheral_map);
}

static void
bfin_dma_process_desc (struct hw *me, struct bfin_dma *dma)
{
  bu8 ndsize = (dma->config & NDSIZE) >> NDSIZE_SHIFT;
  bu16 _flows[9], *flows = _flows;

  HW_TRACE ((me, "dma starting up %#x", dma->config));

  switch (dma->config & WDSIZE)
    {
    case WDSIZE_32:
      dma->ele_size = 4;
      break;
    case WDSIZE_16:
      dma->ele_size = 2;
      break;
    default:
      dma->ele_size = 1;
      break;
    }

  /* Address has to be mutiple of transfer size.  */
  if (dma->start_addr & (dma->ele_size - 1))
    dma->irq_status |= DMA_ERR;

  if (dma->ele_size != (unsigned) abs (dma->x_modify))
    hw_abort (me, "DMA config (striding) %#x not supported (x_modify: %d)",
	      dma->config, dma->x_modify);

  switch (dma->config & DMAFLOW)
    {
    case DMAFLOW_AUTO:
    case DMAFLOW_STOP:
      if (ndsize)
	hw_abort (me, "DMA config error: DMAFLOW_{AUTO,STOP} requires NDSIZE_0");
      break;
    case DMAFLOW_ARRAY:
      if (ndsize == 0 || ndsize > 7)
	hw_abort (me, "DMA config error: DMAFLOW_ARRAY requires NDSIZE 1...7");
      sim_read (hw_system (me), dma->curr_desc_ptr, flows, ndsize * 2);
      break;
    case DMAFLOW_SMALL:
      if (ndsize == 0 || ndsize > 8)
	hw_abort (me, "DMA config error: DMAFLOW_SMALL requires NDSIZE 1...8");
      sim_read (hw_system (me), dma->next_desc_ptr, flows, ndsize * 2);
      break;
    case DMAFLOW_LARGE:
      if (ndsize == 0 || ndsize > 9)
	hw_abort (me, "DMA config error: DMAFLOW_LARGE requires NDSIZE 1...9");
      sim_read (hw_system (me), dma->next_desc_ptr, flows, ndsize * 2);
      break;
    default:
      hw_abort (me, "DMA config error: invalid DMAFLOW %#x", dma->config);
    }

  if (ndsize)
    {
      bu8 idx;
      bu16 *stores[] = {
	&dma->sal,
	&dma->sah,
	&dma->config,
	&dma->x_count,
	(void *) &dma->x_modify,
	&dma->y_count,
	(void *) &dma->y_modify,
      };

      switch (dma->config & DMAFLOW)
	{
	case DMAFLOW_LARGE:
	  dma->ndph = _flows[1];
	  --ndsize;
	  ++flows;
	  ATTRIBUTE_FALLTHROUGH;
	case DMAFLOW_SMALL:
	  dma->ndpl = _flows[0];
	  --ndsize;
	  ++flows;
	  break;
	}

      for (idx = 0; idx < ndsize; ++idx)
	*stores[idx] = flows[idx];
    }

  dma->curr_desc_ptr = dma->next_desc_ptr;
  dma->curr_addr = dma->start_addr;
  dma->curr_x_count = dma->x_count ? : 0xffff;
  dma->curr_y_count = dma->y_count ? : 0xffff;
}

static int
bfin_dma_finish_x (struct hw *me, struct bfin_dma *dma)
{
  /* XXX: This would be the time to process the next descriptor.  */
  /* XXX: Should this toggle Enable in dma->config ?  */

  if (dma->config & DI_EN)
    hw_port_event (me, 0, 1);

  if ((dma->config & DMA2D) && dma->curr_y_count > 1)
    {
      dma->curr_y_count -= 1;
      dma->curr_x_count = dma->x_count;

      /* With 2D, last X transfer does not modify curr_addr.  */
      dma->curr_addr = dma->curr_addr - dma->x_modify + dma->y_modify;

      return 1;
    }

  switch (dma->config & DMAFLOW)
    {
    case DMAFLOW_STOP:
      HW_TRACE ((me, "dma is complete"));
      dma->irq_status = (dma->irq_status & ~DMA_RUN) | DMA_DONE;
      return 0;
    default:
      bfin_dma_process_desc (me, dma);
      return 1;
    }
}

static void bfin_dma_hw_event_callback (struct hw *, void *);

static void
bfin_dma_reschedule (struct hw *me, unsigned delay)
{
  struct bfin_dma *dma = hw_data (me);
  if (dma->handler)
    {
      hw_event_queue_deschedule (me, dma->handler);
      dma->handler = NULL;
    }
  if (!delay)
    return;
  HW_TRACE ((me, "scheduling next process in %u", delay));
  dma->handler = hw_event_queue_schedule (me, delay,
					  bfin_dma_hw_event_callback, dma);
}

/* Chew through the DMA over and over.  */
static void
bfin_dma_hw_event_callback (struct hw *me, void *data)
{
  struct bfin_dma *dma = data;
  struct hw *peer;
  struct dv_bfin *bfin_peer;
  bu8 buf[4096];
  unsigned ret, nr_bytes, ele_count;

  dma->handler = NULL;
  peer = bfin_dma_get_peer (me, dma);
  bfin_peer = hw_data (peer);
  ret = 0;
  if (dma->x_modify < 0)
    /* XXX: This sucks performance wise.  */
    nr_bytes = dma->ele_size;
  else
    nr_bytes = min (sizeof (buf), dma->curr_x_count * dma->ele_size);

  /* Pumping a chunk!  */
  bfin_peer->dma_master = me;
  bfin_peer->acked = false;
  if (dma->config & WNR)
    {
      HW_TRACE ((me, "dma transfer to 0x%08lx length %u",
		 (unsigned long) dma->curr_addr, nr_bytes));

      ret = hw_dma_read_buffer (peer, buf, 0, dma->curr_addr, nr_bytes);
      /* Has the DMA stalled ?  abort for now.  */
      if (ret == 0)
	goto reschedule;
      /* XXX: How to handle partial DMA transfers ?  */
      if (ret % dma->ele_size)
	goto error;
      ret = sim_write (hw_system (me), dma->curr_addr, buf, ret);
    }
  else
    {
      HW_TRACE ((me, "dma transfer from 0x%08lx length %u",
		 (unsigned long) dma->curr_addr, nr_bytes));

      ret = sim_read (hw_system (me), dma->curr_addr, buf, nr_bytes);
      if (ret == 0)
	goto reschedule;
      /* XXX: How to handle partial DMA transfers ?  */
      if (ret % dma->ele_size)
	goto error;
      ret = hw_dma_write_buffer (peer, buf, 0, dma->curr_addr, ret, 0);
      if (ret == 0)
	goto reschedule;
    }

  /* Ignore partial writes.  */
  ele_count = ret / dma->ele_size;
  dma->curr_addr += ele_count * dma->x_modify;
  dma->curr_x_count -= ele_count;

  if ((!dma->acked && dma->curr_x_count) || bfin_dma_finish_x (me, dma))
    /* Still got work to do, so schedule again.  */
 reschedule:
    bfin_dma_reschedule (me, ret ? 1 : 5000);

  return;

 error:
  /* Don't reschedule on errors ...  */
  dma->irq_status |= DMA_ERR;
}

static unsigned
bfin_dma_io_write_buffer (struct hw *me, const void *source, int space,
			  address_word addr, unsigned nr_bytes)
{
  struct bfin_dma *dma = hw_data (me);
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

  mmr_off = addr % dma->base;
  valuep = (void *)((uintptr_t)dma + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_WRITE ();

  /* XXX: All registers are RO when DMA is enabled (except IRQ_STATUS).
          But does the HW discard writes or send up IVGHW ?  The sim
          simply discards atm ... */
  switch (mmr_off)
    {
    case mmr_offset(next_desc_ptr):
    case mmr_offset(start_addr):
    case mmr_offset(curr_desc_ptr):
    case mmr_offset(curr_addr):
      /* Don't require 32bit access as all DMA MMRs can be used as 16bit.  */
      if (!bfin_dma_running (dma))
	{
	  if (nr_bytes == 4)
	    *value32p = value;
	  else
	    *value16p = value;
	}
      else
	HW_TRACE ((me, "discarding write while dma running"));
      break;
    case mmr_offset(x_count):
    case mmr_offset(x_modify):
    case mmr_offset(y_count):
    case mmr_offset(y_modify):
      if (!bfin_dma_running (dma))
	*value16p = value;
      break;
    case mmr_offset(peripheral_map):
      if (!bfin_dma_running (dma))
	{
	  *value16p = (*value16p & CTYPE) | (value & ~CTYPE);
	  /* Clear peripheral peer so it gets looked up again.  */
	  dma->hw_peer = NULL;
	}
      else
	HW_TRACE ((me, "discarding write while dma running"));
      break;
    case mmr_offset(config):
      /* XXX: How to handle updating CONFIG of a running channel ?  */
      if (nr_bytes == 4)
	*value32p = value;
      else
	*value16p = value;

      if (bfin_dma_enabled (dma))
	{
	  dma->irq_status |= DMA_RUN;
	  bfin_dma_process_desc (me, dma);
	  /* The writer is the master.  */
	  if (!(dma->peripheral_map & CTYPE) || (dma->config & WNR))
	    bfin_dma_reschedule (me, 1);
	}
      else
	{
	  dma->irq_status &= ~DMA_RUN;
	  bfin_dma_reschedule (me, 0);
	}
      break;
    case mmr_offset(irq_status):
      dv_w1c_2 (value16p, value, DMA_DONE | DMA_ERR);
      break;
    case mmr_offset(curr_x_count):
    case mmr_offset(curr_y_count):
      if (!bfin_dma_running (dma))
	*value16p = value;
      else
	HW_TRACE ((me, "discarding write while dma running"));
      break;
    default:
      /* XXX: The HW lets the pad regions be read/written ...  */
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_dma_io_read_buffer (struct hw *me, void *dest, int space,
			 address_word addr, unsigned nr_bytes)
{
  struct bfin_dma *dma = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  if (!dv_bfin_mmr_require_16_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr % dma->base;
  valuep = (void *)((uintptr_t)dma + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  /* Hardware lets you read all MMRs as 16 or 32 bits, even reserved.  */
  if (nr_bytes == 4)
    dv_store_4 (dest, *value32p);
  else
    dv_store_2 (dest, *value16p);

  return nr_bytes;
}

static unsigned
bfin_dma_dma_read_buffer (struct hw *me, void *dest, int space,
			  unsigned_word addr, unsigned nr_bytes)
{
  struct bfin_dma *dma = hw_data (me);
  unsigned ret, ele_count;

  HW_TRACE_DMA_READ ();

  /* If someone is trying to read from me, I have to be enabled.  */
  if (!bfin_dma_enabled (dma) && !bfin_dma_running (dma))
    return 0;

  /* XXX: handle x_modify ...  */
  ret = sim_read (hw_system (me), dma->curr_addr, dest, nr_bytes);
  /* Ignore partial writes.  */
  ele_count = ret / dma->ele_size;
  /* Has the DMA stalled ?  abort for now.  */
  if (!ele_count)
    return 0;

  dma->curr_addr += ele_count * dma->x_modify;
  dma->curr_x_count -= ele_count;

  if (dma->curr_x_count == 0)
    bfin_dma_finish_x (me, dma);

  return ret;
}

static unsigned
bfin_dma_dma_write_buffer (struct hw *me, const void *source,
			   int space, unsigned_word addr,
			   unsigned nr_bytes,
			   int violate_read_only_section)
{
  struct bfin_dma *dma = hw_data (me);
  unsigned ret, ele_count;

  HW_TRACE_DMA_WRITE ();

  /* If someone is trying to write to me, I have to be enabled.  */
  if (!bfin_dma_enabled (dma) && !bfin_dma_running (dma))
    return 0;

  /* XXX: handle x_modify ...  */
  ret = sim_write (hw_system (me), dma->curr_addr, source, nr_bytes);
  /* Ignore partial writes.  */
  ele_count = ret / dma->ele_size;
  /* Has the DMA stalled ?  abort for now.  */
  if (!ele_count)
    return 0;

  dma->curr_addr += ele_count * dma->x_modify;
  dma->curr_x_count -= ele_count;

  if (dma->curr_x_count == 0)
    bfin_dma_finish_x (me, dma);

  return ret;
}

static const struct hw_port_descriptor bfin_dma_ports[] =
{
  { "di", 0, 0, output_port, }, /* DMA Interrupt */
  { NULL, 0, 0, 0, },
};

static void
attach_bfin_dma_regs (struct hw *me, struct bfin_dma *dma)
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

  if (attach_size != BFIN_MMR_DMA_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_DMA_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  dma->base = attach_address;
}

static void
bfin_dma_finish (struct hw *me)
{
  struct bfin_dma *dma;

  dma = HW_ZALLOC (me, struct bfin_dma);

  set_hw_data (me, dma);
  set_hw_io_read_buffer (me, bfin_dma_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_dma_io_write_buffer);
  set_hw_dma_read_buffer (me, bfin_dma_dma_read_buffer);
  set_hw_dma_write_buffer (me, bfin_dma_dma_write_buffer);
  set_hw_ports (me, bfin_dma_ports);

  attach_bfin_dma_regs (me, dma);

  /* Initialize the DMA Channel.  */
  dma->peripheral_map = bfin_dmac_default_pmap (me);
}

const struct hw_descriptor dv_bfin_dma_descriptor[] =
{
  {"bfin_dma", bfin_dma_finish,},
  {NULL, NULL},
};
