/* Ravenscar Aarch64 target support.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#include "defs.h"
#include "gdbcore.h"
#include "regcache.h"
#include "aarch64-tdep.h"
#include "inferior.h"
#include "ravenscar-thread.h"
#include "aarch64-ravenscar-thread.h"
#include "gdbarch.h"

#define NO_OFFSET -1

/* See aarch64-tdep.h for register numbers.  */

static const int aarch64_context_offsets[] =
{
  /* X0 - X28 */
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, 0,
  8,         16,        24,        32,
  40,        48,        56,        64,
  72,

  /* FP, LR, SP, PC, CPSR */
  /* Note that as task switch is synchronous, PC is in fact the LR here */
  80,        88,        96,        88,
  NO_OFFSET,

  /* V0 - V31 */
  128,       144,       160,       176,
  192,       208,       224,       240,
  256,       272,       288,       304,
  320,       336,       352,       368,
  384,       400,       416,       432,
  448,       464,       480,       496,
  512,       528,       544,       560,
  576,       592,       608,       624,

  /* FPSR, FPCR */
  112,       116,
};

#define V_INIT_OFFSET 640

/* The ravenscar_arch_ops vector for most Aarch64 targets.  */

static struct ravenscar_arch_ops aarch64_ravenscar_ops
     (aarch64_context_offsets,
      -1, -1,
      V_INIT_OFFSET,
      /* The FPU context buffer starts with the FPSR register.  */
      aarch64_context_offsets[AARCH64_FPSR_REGNUM],
      AARCH64_V0_REGNUM, AARCH64_FPCR_REGNUM);

/* Register aarch64_ravenscar_ops in GDBARCH.  */

void
register_aarch64_ravenscar_ops (struct gdbarch *gdbarch)
{
  set_gdbarch_ravenscar_ops (gdbarch, &aarch64_ravenscar_ops);
}
