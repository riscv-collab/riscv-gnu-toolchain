/* Blackfin Direct Memory Access (DMA) Controller model.

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
#include "sim-hw.h"
#include "devices.h"
#include "hw-device.h"
#include "dv-bfin_dma.h"
#include "dv-bfin_dmac.h"

struct bfin_dmac
{
  /* This top portion matches common dv_bfin struct.  */
  bu32 base;
  struct hw *dma_master;
  bool acked;

  const char * const *pmap;
  unsigned int pmap_count;
};

struct hw *
bfin_dmac_get_peer (struct hw *dma, bu16 pmap)
{
  struct hw *ret, *me;
  struct bfin_dmac *dmac;
  char peer[100];

  me = hw_parent (dma);
  dmac = hw_data (me);
  if (pmap & CTYPE)
    {
      /* MDMA channel.  */
      unsigned int chan_num = dv_get_bus_num (dma);
      if (chan_num & 1)
	chan_num &= ~1;
      else
	chan_num |= 1;
      sprintf (peer, "%s/bfin_dma@%u", hw_path (me), chan_num);
    }
  else
    {
      unsigned int idx = pmap >> 12;
      if (idx >= dmac->pmap_count)
	hw_abort (me, "Invalid DMA peripheral_map %#x", pmap);
      else
	sprintf (peer, "/core/bfin_%s", dmac->pmap[idx]);
    }

  ret = hw_tree_find_device (me, peer);
  if (!ret)
    hw_abort (me, "Unable to locate peer for %s (pmap:%#x %s)",
	      hw_name (dma), pmap, peer);
  return ret;
}

bu16
bfin_dmac_default_pmap (struct hw *dma)
{
  unsigned int chan_num = dv_get_bus_num (dma);

  if (chan_num < BFIN_DMAC_MDMA_BASE)
    return (chan_num % 12) << 12;
  else
    return CTYPE;	/* MDMA */
}

static const char * const bfin_dmac_50x_pmap[] =
{
  "ppi@0", "rsi", "sport@0", "sport@0", "sport@1", "sport@1",
  "spi@0", "spi@1", "uart2@0", "uart2@0", "uart2@1", "uart2@1",
};

/* XXX: Need to figure out how to handle portmuxed DMA channels.  */
static const struct hw_port_descriptor bfin_dmac_50x_ports[] =
{
  { "ppi@0",       0, 0, input_port, },
  { "rsi",         1, 0, input_port, },
  { "sport@0_rx",  2, 0, input_port, },
  { "sport@0_tx",  3, 0, input_port, },
  { "sport@1_tx",  4, 0, input_port, },
  { "sport@1_rx",  5, 0, input_port, },
  { "spi@0",       6, 0, input_port, },
  { "spi@1",       7, 0, input_port, },
  { "uart2@0_rx",  8, 0, input_port, },
  { "uart2@0_tx",  9, 0, input_port, },
  { "uart2@1_rx", 10, 0, input_port, },
  { "uart2@1_tx", 11, 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const char * const bfin_dmac_51x_pmap[] =
{
  "ppi@0", "emac", "emac", "sport@0", "sport@0", "sport@1",
  "sport@1", "spi@0", "uart@0", "uart@0", "uart@1", "uart@1",
};

/* XXX: Need to figure out how to handle portmuxed DMA channels.  */
static const struct hw_port_descriptor bfin_dmac_51x_ports[] =
{
  { "ppi@0",       0, 0, input_port, },
  { "emac_rx",     1, 0, input_port, },
  { "emac_tx",     2, 0, input_port, },
  { "sport@0_rx",  3, 0, input_port, },
  { "sport@0_tx",  4, 0, input_port, },
/*{ "rsi",         4, 0, input_port, },*/
  { "sport@1_tx",  5, 0, input_port, },
/*{ "spi@1",       5, 0, input_port, },*/
  { "sport@1_rx",  6, 0, input_port, },
  { "spi@0",       7, 0, input_port, },
  { "uart@0_rx",   8, 0, input_port, },
  { "uart@0_tx",   9, 0, input_port, },
  { "uart@1_rx",  10, 0, input_port, },
  { "uart@1_tx",  11, 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const char * const bfin_dmac_52x_pmap[] =
{
  "ppi@0", "emac", "emac", "sport@0", "sport@0", "sport@1",
  "sport@1", "spi", "uart@0", "uart@0", "uart@1", "uart@1",
};

/* XXX: Need to figure out how to handle portmuxed DMA channels
        like PPI/NFC here which share DMA0.  */
static const struct hw_port_descriptor bfin_dmac_52x_ports[] =
{
  { "ppi@0",       0, 0, input_port, },
/*{ "nfc",         0, 0, input_port, },*/
  { "emac_rx",     1, 0, input_port, },
/*{ "hostdp",      1, 0, input_port, },*/
  { "emac_tx",     2, 0, input_port, },
/*{ "nfc",         2, 0, input_port, },*/
  { "sport@0_tx",  3, 0, input_port, },
  { "sport@0_rx",  4, 0, input_port, },
  { "sport@1_tx",  5, 0, input_port, },
  { "sport@1_rx",  6, 0, input_port, },
  { "spi",         7, 0, input_port, },
  { "uart@0_tx",   8, 0, input_port, },
  { "uart@0_rx",   9, 0, input_port, },
  { "uart@1_tx",  10, 0, input_port, },
  { "uart@1_rx",  11, 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const char * const bfin_dmac_533_pmap[] =
{
  "ppi@0", "sport@0", "sport@0", "sport@1", "sport@1", "spi",
  "uart@0", "uart@0",
};

static const struct hw_port_descriptor bfin_dmac_533_ports[] =
{
  { "ppi@0",       0, 0, input_port, },
  { "sport@0_tx",  1, 0, input_port, },
  { "sport@0_rx",  2, 0, input_port, },
  { "sport@1_tx",  3, 0, input_port, },
  { "sport@1_rx",  4, 0, input_port, },
  { "spi",         5, 0, input_port, },
  { "uart@0_tx",   6, 0, input_port, },
  { "uart@0_rx",   7, 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const char * const bfin_dmac_537_pmap[] =
{
  "ppi@0", "emac", "emac", "sport@0", "sport@0", "sport@1",
  "sport@1", "spi", "uart@0", "uart@0", "uart@1", "uart@1",
};

static const struct hw_port_descriptor bfin_dmac_537_ports[] =
{
  { "ppi@0",       0, 0, input_port, },
  { "emac_rx",     1, 0, input_port, },
  { "emac_tx",     2, 0, input_port, },
  { "sport@0_tx",  3, 0, input_port, },
  { "sport@0_rx",  4, 0, input_port, },
  { "sport@1_tx",  5, 0, input_port, },
  { "sport@1_rx",  6, 0, input_port, },
  { "spi",         7, 0, input_port, },
  { "uart@0_tx",   8, 0, input_port, },
  { "uart@0_rx",   9, 0, input_port, },
  { "uart@1_tx",  10, 0, input_port, },
  { "uart@1_rx",  11, 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const char * const bfin_dmac0_538_pmap[] =
{
  "ppi@0", "sport@0", "sport@0", "sport@1", "sport@1", "spi@0",
  "uart@0", "uart@0",
};

static const struct hw_port_descriptor bfin_dmac0_538_ports[] =
{
  { "ppi@0",       0, 0, input_port, },
  { "sport@0_rx",  1, 0, input_port, },
  { "sport@0_tx",  2, 0, input_port, },
  { "sport@1_rx",  3, 0, input_port, },
  { "sport@1_tx",  4, 0, input_port, },
  { "spi@0",       5, 0, input_port, },
  { "uart@0_rx",   6, 0, input_port, },
  { "uart@0_tx",   7, 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const char * const bfin_dmac1_538_pmap[] =
{
  "sport@2", "sport@2", "sport@3", "sport@3", NULL, NULL,
  "spi@1", "spi@2", "uart@1", "uart@1", "uart@2", "uart@2",
};

static const struct hw_port_descriptor bfin_dmac1_538_ports[] =
{
  { "sport@2_rx",  0, 0, input_port, },
  { "sport@2_tx",  1, 0, input_port, },
  { "sport@3_rx",  2, 0, input_port, },
  { "sport@3_tx",  3, 0, input_port, },
  { "spi@1",       6, 0, input_port, },
  { "spi@2",       7, 0, input_port, },
  { "uart@1_rx",   8, 0, input_port, },
  { "uart@1_tx",   9, 0, input_port, },
  { "uart@2_rx",  10, 0, input_port, },
  { "uart@2_tx",  11, 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const char * const bfin_dmac0_54x_pmap[] =
{
  "sport@0", "sport@0", "sport@1", "sport@1", "spi@0", "spi@1",
  "uart2@0", "uart2@0", "uart2@1", "uart2@1", "atapi", "atapi",
};

static const struct hw_port_descriptor bfin_dmac0_54x_ports[] =
{
  { "sport@0_rx",  0, 0, input_port, },
  { "sport@0_tx",  1, 0, input_port, },
  { "sport@1_rx",  2, 0, input_port, },
  { "sport@1_tx",  3, 0, input_port, },
  { "spi@0",       4, 0, input_port, },
  { "spi@1",       5, 0, input_port, },
  { "uart2@0_rx",  6, 0, input_port, },
  { "uart2@0_tx",  7, 0, input_port, },
  { "uart2@1_rx",  8, 0, input_port, },
  { "uart2@1_tx",  9, 0, input_port, },
  { "atapi",      10, 0, input_port, },
  { "atapi",      11, 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const char * const bfin_dmac1_54x_pmap[] =
{
  "eppi@0", "eppi@1", "eppi@2", "pixc", "pixc", "pixc",
  "sport@2", "sport@2", "sport@3", "sport@3", "sdh",
  "spi@2", "uart2@2", "uart2@2", "uart2@3", "uart2@3",
};

static const struct hw_port_descriptor bfin_dmac1_54x_ports[] =
{
  { "eppi@0",      0, 0, input_port, },
  { "eppi@1",      1, 0, input_port, },
  { "eppi@2",      2, 0, input_port, },
  { "pixc",        3, 0, input_port, },
  { "pixc",        4, 0, input_port, },
  { "pixc",        5, 0, input_port, },
  { "sport@2_rx",  6, 0, input_port, },
  { "sport@2_tx",  7, 0, input_port, },
  { "sport@3_rx",  8, 0, input_port, },
  { "sport@3_tx",  9, 0, input_port, },
  { "sdh",        10, 0, input_port, },
/*{ "nfc",        10, 0, input_port, },*/
  { "spi@2",      11, 0, input_port, },
  { "uart2@2_rx", 12, 0, input_port, },
  { "uart2@2_tx", 13, 0, input_port, },
  { "uart2@3_rx", 14, 0, input_port, },
  { "uart2@3_tx", 15, 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const char * const bfin_dmac0_561_pmap[] =
{
  "sport@0", "sport@0", "sport@1", "sport@1", "spi", "uart@0", "uart@0",
};

static const struct hw_port_descriptor bfin_dmac0_561_ports[] =
{
  { "sport@0_rx",  0, 0, input_port, },
  { "sport@0_tx",  1, 0, input_port, },
  { "sport@1_rx",  2, 0, input_port, },
  { "sport@1_tx",  3, 0, input_port, },
  { "spi@0",       4, 0, input_port, },
  { "uart@0_rx",   5, 0, input_port, },
  { "uart@0_tx",   6, 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const char * const bfin_dmac1_561_pmap[] =
{
  "ppi@0", "ppi@1",
};

static const struct hw_port_descriptor bfin_dmac1_561_ports[] =
{
  { "ppi@0",       0, 0, input_port, },
  { "ppi@1",       1, 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const char * const bfin_dmac_59x_pmap[] =
{
  "ppi@0", "sport@0", "sport@0", "sport@1", "sport@1", "spi@0",
  "spi@1", "uart@0", "uart@0",
};

static const struct hw_port_descriptor bfin_dmac_59x_ports[] =
{
  { "ppi@0",       0, 0, input_port, },
  { "sport@0_tx",  1, 0, input_port, },
  { "sport@0_rx",  2, 0, input_port, },
  { "sport@1_tx",  3, 0, input_port, },
  { "sport@1_rx",  4, 0, input_port, },
  { "spi@0",       5, 0, input_port, },
  { "spi@1",       6, 0, input_port, },
  { "uart@0_rx",   7, 0, input_port, },
  { "uart@0_tx",   8, 0, input_port, },
  { NULL, 0, 0, 0, },
};

static void
bfin_dmac_port_event (struct hw *me, int my_port, struct hw *source,
		      int source_port, int level)
{
  SIM_DESC sd = hw_system (me);
  struct bfin_dmac *dmac = hw_data (me);
  struct hw *dma = hw_child (me);

  while (dma)
    {
      bu16 pmap;
      sim_hw_io_read_buffer (sd, dma, &pmap, 0, 0x2c, sizeof (pmap));
      pmap >>= 12;
      if (pmap == my_port)
	break;
      dma = hw_sibling (dma);
    }

  if (!dma)
    hw_abort (me, "no valid dma mapping found for %s", dmac->pmap[my_port]);

  /* Have the DMA channel raise its interrupt to the SIC.  */
  hw_port_event (dma, 0, 1);
}

static void
bfin_dmac_finish (struct hw *me)
{
  struct bfin_dmac *dmac;
  unsigned int dmac_num = dv_get_bus_num (me);

  dmac = HW_ZALLOC (me, struct bfin_dmac);

  set_hw_data (me, dmac);
  set_hw_port_event (me, bfin_dmac_port_event);

  /* Initialize the DMA Controller.  */
  if (hw_find_property (me, "type") == NULL)
    hw_abort (me, "Missing \"type\" property");

  switch (hw_find_integer_property (me, "type"))
    {
    case 500 ... 509:
      if (dmac_num != 0)
	hw_abort (me, "this Blackfin only has a DMAC0");
      dmac->pmap = bfin_dmac_50x_pmap;
      dmac->pmap_count = ARRAY_SIZE (bfin_dmac_50x_pmap);
      set_hw_ports (me, bfin_dmac_50x_ports);
      break;
    case 510 ... 519:
      if (dmac_num != 0)
	hw_abort (me, "this Blackfin only has a DMAC0");
      dmac->pmap = bfin_dmac_51x_pmap;
      dmac->pmap_count = ARRAY_SIZE (bfin_dmac_51x_pmap);
      set_hw_ports (me, bfin_dmac_51x_ports);
      break;
    case 522 ... 527:
      if (dmac_num != 0)
	hw_abort (me, "this Blackfin only has a DMAC0");
      dmac->pmap = bfin_dmac_52x_pmap;
      dmac->pmap_count = ARRAY_SIZE (bfin_dmac_52x_pmap);
      set_hw_ports (me, bfin_dmac_52x_ports);
      break;
    case 531 ... 533:
      if (dmac_num != 0)
	hw_abort (me, "this Blackfin only has a DMAC0");
      dmac->pmap = bfin_dmac_533_pmap;
      dmac->pmap_count = ARRAY_SIZE (bfin_dmac_533_pmap);
      set_hw_ports (me, bfin_dmac_533_ports);
      break;
    case 534:
    case 536:
    case 537:
      if (dmac_num != 0)
	hw_abort (me, "this Blackfin only has a DMAC0");
      dmac->pmap = bfin_dmac_537_pmap;
      dmac->pmap_count = ARRAY_SIZE (bfin_dmac_537_pmap);
      set_hw_ports (me, bfin_dmac_537_ports);
      break;
    case 538 ... 539:
      switch (dmac_num)
	{
	case 0:
	  dmac->pmap = bfin_dmac0_538_pmap;
	  dmac->pmap_count = ARRAY_SIZE (bfin_dmac0_538_pmap);
	  set_hw_ports (me, bfin_dmac0_538_ports);
	  break;
	case 1:
	  dmac->pmap = bfin_dmac1_538_pmap;
	  dmac->pmap_count = ARRAY_SIZE (bfin_dmac1_538_pmap);
	  set_hw_ports (me, bfin_dmac1_538_ports);
	  break;
	default:
	  hw_abort (me, "this Blackfin only has a DMAC0 & DMAC1");
	}
      break;
    case 540 ... 549:
      switch (dmac_num)
	{
	case 0:
	  dmac->pmap = bfin_dmac0_54x_pmap;
	  dmac->pmap_count = ARRAY_SIZE (bfin_dmac0_54x_pmap);
	  set_hw_ports (me, bfin_dmac0_54x_ports);
	  break;
	case 1:
	  dmac->pmap = bfin_dmac1_54x_pmap;
	  dmac->pmap_count = ARRAY_SIZE (bfin_dmac1_54x_pmap);
	  set_hw_ports (me, bfin_dmac1_54x_ports);
	  break;
	default:
	  hw_abort (me, "this Blackfin only has a DMAC0 & DMAC1");
	}
      break;
    case 561:
      switch (dmac_num)
	{
	case 0:
	  dmac->pmap = bfin_dmac0_561_pmap;
	  dmac->pmap_count = ARRAY_SIZE (bfin_dmac0_561_pmap);
	  set_hw_ports (me, bfin_dmac0_561_ports);
	  break;
	case 1:
	  dmac->pmap = bfin_dmac1_561_pmap;
	  dmac->pmap_count = ARRAY_SIZE (bfin_dmac1_561_pmap);
	  set_hw_ports (me, bfin_dmac1_561_ports);
	  break;
	default:
	  hw_abort (me, "this Blackfin only has a DMAC0 & DMAC1");
	}
      break;
    case 590 ... 599:
      if (dmac_num != 0)
	hw_abort (me, "this Blackfin only has a DMAC0");
      dmac->pmap = bfin_dmac_59x_pmap;
      dmac->pmap_count = ARRAY_SIZE (bfin_dmac_59x_pmap);
      set_hw_ports (me, bfin_dmac_59x_ports);
      break;
    default:
      hw_abort (me, "no support for DMAC on this Blackfin model yet");
    }
}

const struct hw_descriptor dv_bfin_dmac_descriptor[] =
{
  {"bfin_dmac", bfin_dmac_finish,},
  {NULL, NULL},
};
