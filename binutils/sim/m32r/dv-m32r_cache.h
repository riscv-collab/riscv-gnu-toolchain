/* Handle cache related addresses.

   Copyright (C) 1996-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions and Mike Frysinger.

   This file is part of the GNU simulators.

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

#ifndef DV_M32R_CACHE_H
#define DV_M32R_CACHE_H

/* Support for the MSPR register (Cache Purge Control Register)
   and the MCCR register (Cache Control Register) are needed in order for
   overlays to work correctly with the scache.
   MSPR no longer exists but is supported for upward compatibility with
   early overlay support.  */

/* Cache Purge Control (only exists on early versions of chips) */
#define MSPR_ADDR 0xfffffff7
#define MSPR_PURGE 1

/* Lock Control Register (not supported) */
#define MLCR_ADDR 0xfffffff7
#define MLCR_LM 1

/* Power Management Control Register (not supported) */
#define MPMR_ADDR 0xfffffffb

/* Cache Control Register */
#define MCCR_ADDR 0xffffffff
#define MCCR_CP 0x80
/* not supported */
#define MCCR_CM0 2
#define MCCR_CM1 1

#endif
