/* memory.c -- Memory accessor functions for the AArch64 simulator

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

/* This must come before any other includes.  */
#include "defs.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libiberty.h"

#include "memory.h"
#include "simulator.h"

#include "sim-core.h"
#include "sim-signal.h"

static inline void
mem_error (sim_cpu *cpu, const char *message, uint64_t addr)
{
  TRACE_MEMORY (cpu, "ERROR: %s: %" PRIx64, message, addr);
}

/* FIXME: AArch64 requires aligned memory access if SCTRLR_ELx.A is set,
   but we are not implementing that here.  */
#define FETCH_FUNC64(RETURN_TYPE, ACCESS_TYPE, NAME, N)			\
  RETURN_TYPE								\
  aarch64_get_mem_##NAME (sim_cpu *cpu, uint64_t address)		\
  {									\
    RETURN_TYPE val = (RETURN_TYPE) (ACCESS_TYPE)			\
      sim_core_read_unaligned_##N (cpu, 0, read_map, address);		\
    TRACE_MEMORY (cpu, "read of %" PRIx64 " (%d bytes) from %" PRIx64,	\
		  val, N, address);					\
									\
    return val;								\
  }

FETCH_FUNC64 (uint64_t, uint64_t, u64, 8)
FETCH_FUNC64 (int64_t,   int64_t, s64, 8)

#define FETCH_FUNC32(RETURN_TYPE, ACCESS_TYPE, NAME, N)			\
  RETURN_TYPE								\
  aarch64_get_mem_##NAME (sim_cpu *cpu, uint64_t address)		\
  {									\
    RETURN_TYPE val = (RETURN_TYPE) (ACCESS_TYPE)			\
      sim_core_read_unaligned_##N (cpu, 0, read_map, address);		\
    TRACE_MEMORY (cpu, "read of %8x (%d bytes) from %" PRIx64,		\
		  val, N, address);					\
									\
    return val;								\
  }

FETCH_FUNC32 (uint32_t, uint32_t, u32, 4)
FETCH_FUNC32 (int32_t,   int32_t, s32, 4)
FETCH_FUNC32 (uint32_t, uint16_t, u16, 2)
FETCH_FUNC32 (int32_t,   int16_t, s16, 2)
FETCH_FUNC32 (uint32_t,  uint8_t, u8, 1)
FETCH_FUNC32 (int32_t,    int8_t, s8, 1)

void
aarch64_get_mem_long_double (sim_cpu *cpu, uint64_t address, FRegister *a)
{
  a->v[0] = sim_core_read_unaligned_8 (cpu, 0, read_map, address);
  a->v[1] = sim_core_read_unaligned_8 (cpu, 0, read_map, address + 8);
}

/* FIXME: Aarch64 requires aligned memory access if SCTRLR_ELx.A is set,
   but we are not implementing that here.  */
#define STORE_FUNC(TYPE, NAME, N)					\
  void									\
  aarch64_set_mem_##NAME (sim_cpu *cpu, uint64_t address, TYPE value)	\
  {									\
    TRACE_MEMORY (cpu,							\
		  "write of %" PRIx64 " (%d bytes) to %" PRIx64,	\
		  (uint64_t) value, N, address);			\
									\
    sim_core_write_unaligned_##N (cpu, 0, write_map, address, value);	\
  }

STORE_FUNC (uint64_t, u64, 8)
STORE_FUNC (int64_t,  s64, 8)
STORE_FUNC (uint32_t, u32, 4)
STORE_FUNC (int32_t,  s32, 4)
STORE_FUNC (uint16_t, u16, 2)
STORE_FUNC (int16_t,  s16, 2)
STORE_FUNC (uint8_t,  u8, 1)
STORE_FUNC (int8_t,   s8, 1)

void
aarch64_set_mem_long_double (sim_cpu *cpu, uint64_t address, FRegister a)
{
  TRACE_MEMORY (cpu,
		"write of long double %" PRIx64 " %" PRIx64 " to %" PRIx64,
		a.v[0], a.v[1], address);

  sim_core_write_unaligned_8 (cpu, 0, write_map, address, a.v[0]);
  sim_core_write_unaligned_8 (cpu, 0, write_map, address + 8, a.v[1]);
}

void
aarch64_get_mem_blk (sim_cpu *  cpu,
		     uint64_t   address,
		     char *     buffer,
		     unsigned   length)
{
  unsigned len;

  len = sim_core_read_buffer (CPU_STATE (cpu), cpu, read_map,
			      buffer, address, length);
  if (len == length)
    return;

  memset (buffer, 0, length);
  if (cpu)
    mem_error (cpu, "read of non-existant mem block at", address);

  sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu),
		   sim_stopped, SIM_SIGBUS);
}

const char *
aarch64_get_mem_ptr (sim_cpu *cpu, uint64_t address)
{
  char *addr = sim_core_trans_addr (CPU_STATE (cpu), cpu, read_map, address);

  if (addr == NULL)
    {
      mem_error (cpu, "request for non-existant mem addr of", address);
      sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu),
		       sim_stopped, SIM_SIGBUS);
    }

  return addr;
}

/* We implement a combined stack and heap.  That way the sbrk()
   function in libgloss/aarch64/syscalls.c has a chance to detect
   an out-of-memory condition by noticing a stack/heap collision.

   The heap starts at the end of loaded memory and carries on up
   to an arbitary 2Gb limit.  */

uint64_t
aarch64_get_heap_start (sim_cpu *cpu)
{
  uint64_t heap = trace_sym_value (CPU_STATE (cpu), "end");

  if (heap == 0)
    heap = trace_sym_value (CPU_STATE (cpu), "_end");
  if (heap == 0)
    {
      heap = STACK_TOP - 0x100000;
      sim_io_eprintf (CPU_STATE (cpu),
		      "Unable to find 'end' symbol - using addr based "
		      "upon stack instead %" PRIx64 "\n",
		      heap);
    }
  return heap;
}

uint64_t
aarch64_get_stack_start (sim_cpu *cpu)
{
  if (aarch64_get_heap_start (cpu) >= STACK_TOP)
    mem_error (cpu, "executable is too big", aarch64_get_heap_start (cpu));
  return STACK_TOP;
}
