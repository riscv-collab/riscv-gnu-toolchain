/* CPU support.
   Copyright (C) 1998-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions.

This file is part of GDB, the GNU debugger.

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

#include <stdlib.h>

#include "bfd.h"

#include "sim-main.h"

/* Allocate space for all cpus in the simulator.
   Space for the cpu must currently exist prior to parsing ARGV.  */
/* ??? wip.  better solution must wait.  */

SIM_RC
sim_cpu_alloc_all_extra (SIM_DESC sd, int ncpus, size_t extra_bytes)
{
  int c;

  /* TODO: This should be a command line option for users to control.  */
  if (ncpus == 0)
    ncpus = MAX_NR_PROCESSORS;

  for (c = 0; c < ncpus; ++c)
    STATE_CPU (sd, c) = sim_cpu_alloc_extra (sd, extra_bytes);

  return SIM_RC_OK;
}

/* Allocate space for a cpu object.
   EXTRA_BYTES is additional space to allocate for the sim_cpu struct.  */

sim_cpu *
sim_cpu_alloc_extra (SIM_DESC sd, size_t extra_bytes)
{
  sim_cpu *cpu = zalloc (sizeof (*cpu));

#ifndef CGEN_ARCH
# define cgen_cpu_max_extra_bytes(sd) 0
#endif
  extra_bytes += cgen_cpu_max_extra_bytes (sd);
  if (extra_bytes)
    CPU_ARCH_DATA (cpu) = zalloc (extra_bytes);

  return cpu;
}

/* Free all resources held by all cpus.  */

void
sim_cpu_free_all (SIM_DESC sd)
{
  int c;

  for (c = 0; c < MAX_NR_PROCESSORS; ++c)
    if (STATE_CPU (sd, c))
      sim_cpu_free (STATE_CPU (sd, c));
}

/* Free all resources used by CPU.  */

void
sim_cpu_free (sim_cpu *cpu)
{
  free (CPU_ARCH_DATA (cpu));
  free (cpu);
}

/* PC utilities.  */

sim_cia
sim_pc_get (sim_cpu *cpu)
{
  return (* CPU_PC_FETCH (cpu)) (cpu);
}

void
sim_pc_set (sim_cpu *cpu, sim_cia newval)
{
  (* CPU_PC_STORE (cpu)) (cpu, newval);
}
