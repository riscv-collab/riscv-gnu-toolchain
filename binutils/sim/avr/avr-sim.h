/* AVR Simulator definition.
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

#ifndef AVR_SIM_H
#define AVR_SIM_H

#include <stdint.h>

struct avr_sim_cpu {
  /* The only real register.  */
  uint32_t pc;

  /* We update a cycle counter.  */
  uint32_t cycles;
};

#define AVR_SIM_CPU(cpu) ((struct avr_sim_cpu *) CPU_ARCH_DATA (cpu))

struct avr_sim_state {
  /* If true, the pc needs more than 2 bytes.  */
  int avr_pc22;
};

#define AVR_SIM_STATE(sd) ((struct avr_sim_state *) STATE_ARCH_DATA (sd))

#endif
