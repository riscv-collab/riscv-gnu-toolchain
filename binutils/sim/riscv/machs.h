/* RISC-V simulator.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.
   Contributed by Mike Frysinger.

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

#ifndef RISCV_SIM_MACHS_H
#define RISCV_SIM_MACHS_H

typedef enum model_type {
#define M(ext) MODEL_RV32##ext,
#include "model_list.def"
#undef M
#define M(ext) MODEL_RV64##ext,
#include "model_list.def"
#undef M
#define M(ext) MODEL_RV128##ext,
#include "model_list.def"
#undef M
  MODEL_MAX
} MODEL_TYPE;

typedef enum mach_attr {
  MACH_BASE,
  MACH_RV32I,
  MACH_RV64I,
  MACH_RV128I,
  MACH_MAX
} MACH_ATTR;

#endif
