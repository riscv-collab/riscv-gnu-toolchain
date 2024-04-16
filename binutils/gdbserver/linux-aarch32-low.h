/* Copyright (C) 1995-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_LINUX_AARCH32_LOW_H
#define GDBSERVER_LINUX_AARCH32_LOW_H

extern struct regs_info regs_info_aarch32;

void arm_fill_gregset (struct regcache *regcache, void *buf);
void arm_store_gregset (struct regcache *regcache, const void *buf);
void arm_fill_vfpregset_num (struct regcache *regcache, void *buf, int num);
void arm_store_vfpregset_num (struct regcache *regcache, const void *buf,
			      int num);

int arm_breakpoint_kind_from_pc (CORE_ADDR *pcptr);
const gdb_byte *arm_sw_breakpoint_from_kind (int kind , int *size);
int arm_breakpoint_kind_from_current_state (CORE_ADDR *pcptr);
int arm_breakpoint_at (CORE_ADDR where);

void initialize_low_arch_aarch32 (void);

void init_registers_arm_with_neon (void);
int arm_is_thumb_mode (void);

#endif /* GDBSERVER_LINUX_AARCH32_LOW_H */
