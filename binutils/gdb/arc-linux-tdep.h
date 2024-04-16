/* Target dependent code for GNU/Linux ARC.

   Copyright 2020-2024 Free Software Foundation, Inc.

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

#ifndef ARC_LINUX_TDEP_H
#define ARC_LINUX_TDEP_H

#include "gdbarch.h"
#include "regset.h"

#define ARC_LINUX_SIZEOF_V2_REGSET (3 * ARC_REGISTER_SIZE)

/* Reads registers from the NT_PRSTATUS data array into the regcache.  */

void arc_linux_supply_gregset (const struct regset *regset,
			       struct regcache *regcache, int regnum,
			       const void *gregs, size_t size);

/* Reads registers from the NT_ARC_V2 data array into the regcache.  */

void arc_linux_supply_v2_regset (const struct regset *regset,
				 struct regcache *regcache, int regnum,
				 const void *v2_regs, size_t size);

/* Writes registers from the regcache into the NT_PRSTATUS data array.  */

void arc_linux_collect_gregset (const struct regset *regset,
				const struct regcache *regcache,
				int regnum, void *gregs, size_t size);

/* Writes registers from the regcache into the NT_ARC_V2 data array.  */

void arc_linux_collect_v2_regset (const struct regset *regset,
				  const struct regcache *regcache,
				  int regnum, void *v2_regs, size_t size);

#endif /* ARC_LINUX_TDEP_H */
