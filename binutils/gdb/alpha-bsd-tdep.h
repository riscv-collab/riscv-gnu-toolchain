/* Common target dependent code for Alpha BSD's.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

#ifndef ALPHA_BSD_TDEP_H
#define ALPHA_BSD_TDEP_H

#include "gdbarch.h"

struct regcache;

void alphabsd_supply_reg (struct regcache *, const char *, int);
void alphabsd_fill_reg (const struct regcache *, char *, int);

void alphabsd_supply_fpreg (struct regcache *, const char *, int);
void alphabsd_fill_fpreg (const struct regcache *, char *, int);


/* Functions exported from alpha-netbsd-tdep.c.  */

/* Iterate over supported core file register note sections. */
void alphanbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
					     iterate_over_regset_sections_cb *cb,
					     void *cb_data,
					     const struct regcache *regcache);

#endif /* alpha-bsd-tdep.h */
