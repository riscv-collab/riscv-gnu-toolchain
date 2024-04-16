/* memory.h -- Prototypes for AArch64 memory accessor functions.

   Copyright (C) 2015-2024 Free Software Foundation, Inc.

   Contributed by Red Hat.

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

#ifndef _MEMORY_H
#define _MEMORY_H

#include <sys/types.h>
#include "simulator.h"

extern void         aarch64_get_mem_long_double (sim_cpu *, uint64_t, FRegister *);
extern uint64_t     aarch64_get_mem_u64 (sim_cpu *, uint64_t);
extern int64_t      aarch64_get_mem_s64 (sim_cpu *, uint64_t);
extern uint32_t     aarch64_get_mem_u32 (sim_cpu *, uint64_t);
extern int32_t      aarch64_get_mem_s32 (sim_cpu *, uint64_t);
extern uint32_t     aarch64_get_mem_u16 (sim_cpu *, uint64_t);
extern int32_t      aarch64_get_mem_s16 (sim_cpu *, uint64_t);
extern uint32_t     aarch64_get_mem_u8  (sim_cpu *, uint64_t);
extern int32_t      aarch64_get_mem_s8  (sim_cpu *, uint64_t);
extern void         aarch64_get_mem_blk (sim_cpu *, uint64_t, char *, unsigned);
extern const char * aarch64_get_mem_ptr (sim_cpu *, uint64_t);

extern void         aarch64_set_mem_long_double (sim_cpu *, uint64_t, FRegister);
extern void         aarch64_set_mem_u64 (sim_cpu *, uint64_t, uint64_t);
extern void         aarch64_set_mem_s64 (sim_cpu *, uint64_t, int64_t);
extern void         aarch64_set_mem_u32 (sim_cpu *, uint64_t, uint32_t);
extern void         aarch64_set_mem_s32 (sim_cpu *, uint64_t, int32_t);
extern void         aarch64_set_mem_u16 (sim_cpu *, uint64_t, uint16_t);
extern void         aarch64_set_mem_s16 (sim_cpu *, uint64_t, int16_t);
extern void         aarch64_set_mem_u8  (sim_cpu *, uint64_t, uint8_t);
extern void         aarch64_set_mem_s8  (sim_cpu *, uint64_t, int8_t);

#define STACK_TOP   0x07FFFF00

extern uint64_t     aarch64_get_heap_start (sim_cpu *);
extern uint64_t     aarch64_get_stack_start (sim_cpu *);

#endif /* _MEMORY_H */
