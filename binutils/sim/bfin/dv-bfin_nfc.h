/* Blackfin NAND Flash Memory Controller (NFC) model

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

#ifndef DV_BFIN_NFC_H
#define DV_BFIN_NFC_H

/* NFC_STAT masks.  */
#define NBUSY		(1 << 0)
#define WB_FULL		(1 << 1)
#define PG_WR_STAT	(1 << 2)
#define PG_RD_STAT	(1 << 3)
#define WB_EMPTY	(1 << 4)

/* NFC_IRQSTAT masks.  */
#define NBUSYIRQ	(1 << 0)
#define WB_OVF		(1 << 1)
#define WB_EDGE		(1 << 2)
#define RD_RDY		(1 << 3)
#define WR_DONE		(1 << 4)

#endif
