/* Ravenscar PowerPC target support.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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
#include "ppc-tdep.h"
#include "inferior.h"
#include "ravenscar-thread.h"
#include "ppc-ravenscar-thread.h"

#define NO_OFFSET -1

/* See ppc-tdep.h for register numbers.  */

static const int powerpc_context_offsets[] =
{
  /* R0 - R32 */
  NO_OFFSET, 0,         4,         NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, 8,         12,        16,
  20,        24,        28,        32,
  36,        40,        44,        48,
  52,        56,        60,        64,
  68,        72,        76,        80,

  /* F0 - F31 */
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, 96,        104,
  112,       120,       128,       136,
  144,       152,       160,       168,
  176,       184,       192,       200,
  208,       216,       224,       232,

  /* PC, MSR, CR, LR */
  88,        NO_OFFSET, 84,        NO_OFFSET,

  /* CTR, XER, FPSCR  */
  NO_OFFSET, NO_OFFSET, 240
};

static const int e500_context_offsets[] =
{
  /* R0 - R32 */
  NO_OFFSET, 4,         12,        NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, 20,        28,        36,
  44,        52,        60,        68,
  76,        84,        92,        100,
  108,       116,       124,       132,
  140,       148,       156,       164,

  /* F0 - F31 */
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,

  /* PC, MSR, CR, LR */
  172,       NO_OFFSET, 168,       NO_OFFSET,

  /* CTR, XER, FPSCR, MQ  */
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,

  /* Upper R0-R32.  */
  NO_OFFSET, 0,         8,        NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, NO_OFFSET, NO_OFFSET, NO_OFFSET,
  NO_OFFSET, 16,        24,        32,
  40,        48,        56,        64,
  72,        80,        88,        96,
  104,       112,       120,       128,
  136,       144,       152,       160,

  /* ACC, FSCR */
  NO_OFFSET, 176
};

/* The ravenscar_arch_ops vector for most PowerPC targets.  */

static struct ravenscar_arch_ops ppc_ravenscar_powerpc_ops
     (powerpc_context_offsets);

/* Register ppc_ravenscar_powerpc_ops in GDBARCH.  */

void
register_ppc_ravenscar_ops (struct gdbarch *gdbarch)
{
  set_gdbarch_ravenscar_ops (gdbarch, &ppc_ravenscar_powerpc_ops);
}

/* The ravenscar_arch_ops vector for E500 targets.  */

static struct ravenscar_arch_ops ppc_ravenscar_e500_ops (e500_context_offsets);

/* Register ppc_ravenscar_e500_ops in GDBARCH.  */

void
register_e500_ravenscar_ops (struct gdbarch *gdbarch)
{
  set_gdbarch_ravenscar_ops (gdbarch, &ppc_ravenscar_e500_ops);
}
