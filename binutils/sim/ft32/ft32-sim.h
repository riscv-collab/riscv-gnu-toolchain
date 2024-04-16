/* Simulator for the FT32 processor

   Copyright (C) 2008-2024 Free Software Foundation, Inc.
   Contributed by FTDI <support@ftdichip.com>

   This file is part of simulators.

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

#ifndef _FT32_SIM_H_
#define _FT32_SIM_H_

#include <stdint.h>

#include "sim/sim-ft32.h"

#define FT32_HARD_FP 29
#define FT32_HARD_CC 30
#define FT32_HARD_SP 31

struct ft32_cpu_state {
  uint32_t regs[32];
  uint32_t pc;
  uint64_t num_i;
  uint64_t cycles;
  uint64_t next_tick_cycle;
  int pm_unlock;
  uint32_t pm_addr;
  int exception;
};

#define FT32_SIM_CPU(cpu) ((struct ft32_cpu_state *) CPU_ARCH_DATA (cpu))

#endif  /* _FT32_SIM_H_ */
