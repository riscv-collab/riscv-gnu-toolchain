/* Ravenscar RISC-V target support.

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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
#include "gdbarch.h"
#include "gdbcore.h"
#include "regcache.h"
#include "riscv-tdep.h"
#include "inferior.h"
#include "ravenscar-thread.h"
#include "riscv-ravenscar-thread.h"

#define LAST_REGISTER (RISCV_FIRST_FP_REGNUM + 14)

struct riscv_ravenscar_ops : public ravenscar_arch_ops
{
  int reg_offsets[LAST_REGISTER + 1];

  riscv_ravenscar_ops (struct gdbarch *arch);
};

riscv_ravenscar_ops::riscv_ravenscar_ops (struct gdbarch *arch)
  : ravenscar_arch_ops (gdb::make_array_view (reg_offsets, LAST_REGISTER + 1))
{
  int reg_size = riscv_isa_xlen (arch);

  for (int regnum = 0; regnum <= LAST_REGISTER; ++regnum)
    {
      int offset;
      if (regnum == RISCV_RA_REGNUM || regnum == RISCV_PC_REGNUM)
	offset = 0;
      else if (regnum == RISCV_SP_REGNUM)
	offset = 1;
      else if (regnum == RISCV_ZERO_REGNUM + 8) /* S0 */
	offset = 2;
      else if (regnum == RISCV_ZERO_REGNUM + 9) /* S1 */
	offset = 3;
      else if (regnum >= RISCV_ZERO_REGNUM + 19
	       && regnum <= RISCV_ZERO_REGNUM + 27) /* S2..S11 */
	offset = regnum - (RISCV_ZERO_REGNUM + 19) + 4;
      else if (regnum >= RISCV_FIRST_FP_REGNUM
	       && regnum <= RISCV_FIRST_FP_REGNUM + 11)
	offset = regnum - RISCV_FIRST_FP_REGNUM + 14; /* FS0..FS11 */
      else
	offset = -1;

      if (offset != -1)
	offset *= reg_size;

      reg_offsets[regnum] = offset;
    }
}

/* Register riscv_ravenscar_ops in GDBARCH.  */

void
register_riscv_ravenscar_ops (struct gdbarch *gdbarch)
{
  set_gdbarch_ravenscar_ops (gdbarch, new riscv_ravenscar_ops (gdbarch));
}
