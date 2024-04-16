/* aarch64-sim.h -- Internal aarch64 settings.

   Copyright (C) 2015-2024 Free Software Foundation, Inc.

   Contributed by Red Hat.

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

#ifndef AARCH64_SIM_H
#define AARCH64_SIM_H

#include <stdint.h>

#include "cpustate.h"

/* A per-core state structure.  */
struct aarch64_sim_cpu
{
  GRegister    gr[33];	/* Extra register at index 32 is used to hold zero value.  */
  FRegister    fr[32];

  uint64_t     pc;
  uint32_t     CPSR;
  uint32_t     FPSR; /* Floating point Status register.  */
  uint32_t     FPCR; /* Floating point Control register.  */

  uint64_t     nextpc;
  uint32_t     instr;

  uint64_t     tpidr;  /* Thread pointer id.  */
};

#define AARCH64_SIM_CPU(cpu) ((struct aarch64_sim_cpu *) CPU_ARCH_DATA (cpu))

typedef enum
{
  AARCH64_MIN_GR     = 0,
  AARCH64_MAX_GR     = 31,
  AARCH64_MIN_FR     = 32,
  AARCH64_MAX_FR     = 63,
  AARCH64_PC_REGNO   = 64,
  AARCH64_CPSR_REGNO = 65,
  AARCH64_FPSR_REGNO = 66,
  AARCH64_MAX_REGNO  = 67
} aarch64_regno;

#endif /* AARCH64_SIM_H */
