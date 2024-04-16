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

/* This must come before any other includes.  */
#include "defs.h"

#include "sim-main.h"
#include "machs.h"

static void
riscv_model_init (SIM_CPU *cpu)
{
}

static void
riscv_init_cpu (SIM_CPU *cpu)
{
}

static void
riscv_prepare_run (SIM_CPU *cpu)
{
}

static const SIM_MACH_IMP_PROPERTIES riscv_imp_properties =
{
  sizeof (SIM_CPU),
  0,
};

#if WITH_TARGET_WORD_BITSIZE >= 32

static const SIM_MACH rv32i_mach;

static const SIM_MODEL rv32_models[] =
{
#define M(ext) { "RV32"#ext, &rv32i_mach, MODEL_RV32##ext, NULL, riscv_model_init },
#include "model_list.def"
#undef M
  { 0, NULL, 0, NULL, NULL, }
};

static const SIM_MACH rv32i_mach =
{
  "rv32i", "riscv:rv32", MACH_RV32I,
  32, 32, &rv32_models[0], &riscv_imp_properties,
  riscv_init_cpu,
  riscv_prepare_run
};

#endif

#if WITH_TARGET_WORD_BITSIZE >= 64

static const SIM_MACH rv64i_mach;

static const SIM_MODEL rv64_models[] =
{
#define M(ext) { "RV64"#ext, &rv64i_mach, MODEL_RV64##ext, NULL, riscv_model_init },
#include "model_list.def"
#undef M
  { 0, NULL, 0, NULL, NULL, }
};

static const SIM_MACH rv64i_mach =
{
  "rv64i", "riscv:rv64", MACH_RV64I,
  64, 64, &rv64_models[0], &riscv_imp_properties,
  riscv_init_cpu,
  riscv_prepare_run
};

#endif

#if WITH_TARGET_WORD_BITSIZE >= 128

static const SIM_MACH rv128i_mach;

static const SIM_MODEL rv128_models[] =
{
#define M(ext) { "RV128"#ext, &rv128i_mach, MODEL_RV128##ext, NULL, riscv_model_init },
#include "model_list.def"
#undef M
  { 0, NULL, 0, NULL, NULL, }
};

static const SIM_MACH rv128i_mach =
{
  "rv128i", "riscv:rv128", MACH_RV128I,
  128, 128, &rv128_models[0], &riscv_imp_properties,
  riscv_init_cpu,
  riscv_prepare_run
};

#endif

/* Order matters here.  */
const SIM_MACH * const riscv_sim_machs[] =
{
#if WITH_TARGET_WORD_BITSIZE >= 128
  &rv128i_mach,
#endif
#if WITH_TARGET_WORD_BITSIZE >= 64
  &rv64i_mach,
#endif
#if WITH_TARGET_WORD_BITSIZE >= 32
  &rv32i_mach,
#endif
  NULL
};
