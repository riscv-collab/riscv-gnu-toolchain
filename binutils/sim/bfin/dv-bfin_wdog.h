/* Blackfin Watchdog (WDOG) model.

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

#ifndef DV_BFIN_WDOG_H
#define DV_BFIN_WDOG_H

/* WDOG_CTL */
#define WDEV            0x0006  /* event generated on roll over */
#define WDEV_RESET      0x0000  /* generate reset event on roll over */
#define WDEV_NMI        0x0002  /* generate NMI event on roll over */
#define WDEV_GPI        0x0004  /* generate GP IRQ on roll over */
#define WDEV_NONE       0x0006  /* no event on roll over */
#define WDEN            0x0FF0  /* enable watchdog */
#define WDDIS           0x0AD0  /* disable watchdog */
#define WDRO            0x8000  /* watchdog rolled over latch */

#endif
