/* Blackfin Two Wire Interface (TWI) model

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

#ifndef DV_BFIN_TWI_H
#define DV_BFIN_TWI_H

/* TWI_MASTER_STAT Masks */
#define MPROG		(1 << 0)
#define LOSTARB		(1 << 1)
#define ANAK		(1 << 2)
#define DNAK		(1 << 3)
#define BUFRDERR	(1 << 4)
#define BUFWRERR	(1 << 5)
#define SDASEN		(1 << 6)
#define SCLSEN		(1 << 7)
#define BUSBUSY		(1 << 8)

#endif
