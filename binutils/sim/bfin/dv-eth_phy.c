/* Ethernet Physical Receiver model.

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

#if defined (HAVE_LINUX_MII_H) && defined (HAVE_LINUX_TYPES_H)

/* Workaround old/broken linux headers.  */
#include <linux/types.h>
#include <linux/mii.h>

#define REG_PHY_SIZE 0x20

struct eth_phy
{
  bu32 base;
  bu16 regs[REG_PHY_SIZE];
};
#define reg_base()      offsetof(struct eth_phy, regs[0])
#define reg_offset(reg) (offsetof(struct eth_phy, reg) - reg_base())
#define reg_idx(reg)    (reg_offset (reg) / 4)

static const char * const reg_names[] =
{
  [MII_BMCR       ] = "MII_BMCR",
  [MII_BMSR       ] = "MII_BMSR",
  [MII_PHYSID1    ] = "MII_PHYSID1",
  [MII_PHYSID2    ] = "MII_PHYSID2",
  [MII_ADVERTISE  ] = "MII_ADVERTISE",
  [MII_LPA        ] = "MII_LPA",
  [MII_EXPANSION  ] = "MII_EXPANSION",
#ifdef MII_CTRL1000
  [MII_CTRL1000   ] = "MII_CTRL1000",
#endif
#ifdef MII_STAT1000
  [MII_STAT1000   ] = "MII_STAT1000",
#endif
#ifdef MII_ESTATUS
  [MII_ESTATUS    ] = "MII_ESTATUS",
#endif
  [MII_DCOUNTER   ] = "MII_DCOUNTER",
  [MII_FCSCOUNTER ] = "MII_FCSCOUNTER",
  [MII_NWAYTEST   ] = "MII_NWAYTEST",
  [MII_RERRCOUNTER] = "MII_RERRCOUNTER",
  [MII_SREVISION  ] = "MII_SREVISION",
  [MII_RESV1      ] = "MII_RESV1",
  [MII_LBRERROR   ] = "MII_LBRERROR",
  [MII_PHYADDR    ] = "MII_PHYADDR",
  [MII_RESV2      ] = "MII_RESV2",
  [MII_TPISTATUS  ] = "MII_TPISTATUS",
  [MII_NCONFIG    ] = "MII_NCONFIG",
};
#define mmr_name(off) (reg_names[off] ? : "<INV>")
#define mmr_off reg_off

static unsigned
eth_phy_io_write_buffer (struct hw *me, const void *source,
			 int space, address_word addr, unsigned nr_bytes)
{
  struct eth_phy *phy = hw_data (me);
  bu16 reg_off;
  bu16 value;
  bu16 *valuep;

  value = dv_load_2 (source);

  reg_off = addr - phy->base;
  valuep = (void *)((uintptr_t)phy + reg_base() + reg_off);

  HW_TRACE_WRITE ();

  switch (reg_off)
    {
    case MII_BMCR:
      *valuep = value;
      break;
    case MII_PHYSID1:
    case MII_PHYSID2:
      /* Discard writes to these.  */
      break;
    default:
      /* XXX: Discard writes to unknown regs ?  */
      *valuep = value;
      break;
    }

  return nr_bytes;
}

static unsigned
eth_phy_io_read_buffer (struct hw *me, void *dest,
			int space, address_word addr, unsigned nr_bytes)
{
  struct eth_phy *phy = hw_data (me);
  bu16 reg_off;
  bu16 *valuep;

  reg_off = addr - phy->base;
  valuep = (void *)((uintptr_t)phy + reg_base() + reg_off);

  HW_TRACE_READ ();

  switch (reg_off)
    {
    case MII_BMCR:
      dv_store_2 (dest, *valuep);
      break;
    case MII_BMSR:
      /* XXX: Let people control this ?  */
      *valuep = BMSR_100FULL | BMSR_100HALF | BMSR_10FULL | BMSR_10HALF |
		BMSR_ANEGCOMPLETE | BMSR_ANEGCAPABLE | BMSR_LSTATUS;
      dv_store_2 (dest, *valuep);
      break;
    case MII_LPA:
      /* XXX: Let people control this ?  */
      *valuep = LPA_100FULL | LPA_100HALF | LPA_10FULL | LPA_10HALF;
      dv_store_2 (dest, *valuep);
      break;
    default:
      dv_store_2 (dest, *valuep);
      break;
    }

  return nr_bytes;
}

static void
attach_eth_phy_regs (struct hw *me, struct eth_phy *phy)
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

  if (attach_size != REG_PHY_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", REG_PHY_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  phy->base = attach_address;
}

static void
eth_phy_finish (struct hw *me)
{
  struct eth_phy *phy;

  phy = HW_ZALLOC (me, struct eth_phy);

  set_hw_data (me, phy);
  set_hw_io_read_buffer (me, eth_phy_io_read_buffer);
  set_hw_io_write_buffer (me, eth_phy_io_write_buffer);

  attach_eth_phy_regs (me, phy);

  /* Initialize the PHY.  */
  phy->regs[MII_PHYSID1] = 0;    /* Unassigned Vendor */
  phy->regs[MII_PHYSID2] = 0xAD; /* Product */
}

#else

static void
eth_phy_finish (struct hw *me)
{
  HW_TRACE ((me, "No linux/mii.h support found"));
}

#endif

const struct hw_descriptor dv_eth_phy_descriptor[] =
{
  {"eth_phy", eth_phy_finish,},
  {NULL, NULL},
};
