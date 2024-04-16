/* Simulator for Motorola's MCore processor
   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#ifndef MCORE_SIM_H
#define MCORE_SIM_H

#include <stdint.h>

/* The machine state.
   This state is maintained in host byte order.  The
   fetch/store register functions must translate between host
   byte order and the target processor byte order.
   Keeping this data in target byte order simplifies the register
   read/write functions.  Keeping this data in native order improves
   the performance of the simulator.  Simulation speed is deemed more
   important.  */

/* The ordering of the mcore_regset structure is matched in the
   gdb/config/mcore/tm-mcore.h file in the REGISTER_NAMES macro.  */
struct mcore_regset
{
  int32_t gregs[16];		/* primary registers */
  int32_t alt_gregs[16];	/* alt register file */
  int32_t cregs[32];		/* control registers */
  int32_t pc;
};
#define LAST_VALID_CREG	32		/* only 0..12 implemented */
#define NUM_MCORE_REGS	(16 + 16 + LAST_VALID_CREG + 1)

struct mcore_sim_cpu {
  union
  {
    struct mcore_regset regs;
    /* Used by the fetch/store reg helpers to access registers linearly.  */
    int32_t asints[NUM_MCORE_REGS];
  };

  /* Used to switch between gregs/alt_gregs based on the control state.  */
  int32_t *active_gregs;

  int ticks;
  int stalls;
  int cycles;
  int insts;
};

#define MCORE_SIM_CPU(cpu) ((struct mcore_sim_cpu *) CPU_ARCH_DATA (cpu))

#endif
