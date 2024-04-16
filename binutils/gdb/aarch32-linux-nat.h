/* Copyright (C) 1999-2024 Free Software Foundation, Inc.

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

#ifndef AARCH32_LINUX_NAT_H
#define AARCH32_LINUX_NAT_H

void aarch32_gp_regcache_supply (struct regcache *regcache, uint32_t *regs,
				 int arm_apcs_32);

void aarch32_gp_regcache_collect (const struct regcache *regcache,
				  uint32_t *regs, int arm_apcs_32);

void aarch32_vfp_regcache_supply (struct regcache *regcache, gdb_byte *regs,
				  const int vfp_register_count);

void aarch32_vfp_regcache_collect (const struct regcache *regcache,
				   gdb_byte *regs,
				   const int vfp_register_count);

#endif /* AARCH32_LINUX_NAT_H */
