/* Blackfin Serial Peripheral Interface (SPI) model

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

#ifndef DV_BFIN_SPI_H
#define DV_BFIN_SPI_H

/* SPI_CTL Masks.  */
#define TIMOD		(3 << 0)
#define RDBR_CORE	(0 << 0)
#define TDBR_CORE	(1 << 0)
#define RDBR_DMA	(2 << 0)
#define TDBR_DMA	(3 << 0)
#define SZ		(1 << 2)
#define GM		(1 << 3)
#define PSSE		(1 << 4)
#define EMISO		(1 << 5)
#define SZE		(1 << 8)
#define LSBF		(1 << 9)
#define CPHA		(1 << 10)
#define CPOL		(1 << 11)
#define MSTR		(1 << 12)
#define WOM		(1 << 13)
#define SPE		(1 << 14)

/* SPI_STAT Masks.  */
#define SPIF		(1 << 0)
#define MODF		(1 << 1)
#define TXE		(1 << 2)
#define TXS		(1 << 3)
#define RBSY		(1 << 4)
#define RXS		(1 << 5)
#define TXCOL		(1 << 6)

#endif
