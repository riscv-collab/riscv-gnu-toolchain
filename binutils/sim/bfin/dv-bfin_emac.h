/* Blackfin Ethernet Media Access Controller (EMAC) model.

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

#ifndef DV_BFIN_EMAC_H
#define DV_BFIN_EMAC_H

/* EMAC_OPMODE Masks */
#define RE		(1 << 0)
#define ASTP		(1 << 1)
#define PR		(1 << 7)
#define TE		(1 << 16)

/* EMAC_STAADD Masks */
#define STABUSY		(1 << 0)
#define STAOP		(1 << 1)
#define STADISPRE	(1 << 2)
#define STAIE		(1 << 3)
#define REGAD_SHIFT	6
#define REGAD_MASK	(0x1f << REGAD_SHIFT)
#define REGAD(val)	(((val) & REGAD_MASK) >> REGAD_SHIFT)
#define PHYAD_SHIFT	11
#define PHYAD_MASK	(0x1f << PHYAD_SHIFT)
#define PHYAD(val)	(((val) & PHYAD_MASK) >> PHYAD_SHIFT)

/* EMAC_SYSCTL Masks */
#define PHYIE		(1 << 0)
#define RXDWA		(1 << 1)
#define RXCKS		(1 << 2)
#define TXDWA		(1 << 4)

/* EMAC_RX_STAT Masks */
#define RX_FRLEN	0x7ff
#define RX_COMP		(1 << 12)
#define RX_OK		(1 << 13)
#define RX_ACCEPT	(1 << 31)

/* EMAC_TX_STAT Masks */
#define TX_COMP		(1 << 0)
#define TX_OK		(1 << 1)

#endif
