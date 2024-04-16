/* Process record and replay target for GDB, the GNU debugger.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

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

#ifndef RECORD_FULL_H
#define RECORD_FULL_H

extern bool record_full_memory_query;

extern int record_full_arch_list_add_reg (struct regcache *regcache, int num);
extern int record_full_arch_list_add_mem (CORE_ADDR addr, int len);
extern int record_full_arch_list_add_end (void);

/* Returns true if the process record target is open.  */
extern int record_full_is_used (void);

extern scoped_restore_tmpl<int> record_full_gdb_operation_disable_set ();

#endif /* RECORD_FULL_H */
