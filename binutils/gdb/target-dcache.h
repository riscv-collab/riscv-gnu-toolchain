/* Copyright (C) 1992-2024 Free Software Foundation, Inc.

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

#ifndef TARGET_DCACHE_H
#define TARGET_DCACHE_H

#include "dcache.h"
#include "progspace.h"

extern void target_dcache_invalidate (address_space_ref_ptr aspace);

extern DCACHE *target_dcache_get (address_space_ref_ptr aspace);

extern DCACHE *target_dcache_get_or_init (address_space_ref_ptr aspace);

extern int target_dcache_init_p (address_space_ref_ptr aspace);

extern int stack_cache_enabled_p (void);

extern int code_cache_enabled_p (void);

#endif /* TARGET_DCACHE_H */
