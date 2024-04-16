/* Target-dependent code for ARM BSD's.

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

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
#include "osabi.h"
#include "regcache.h"
#include "regset.h"

#include "arm-tdep.h"

/* Core file support.  */

/* Sizeof `struct reg' in <machine/reg.h>.  */
#define ARMBSD_SIZEOF_GREGS	(17 * 4)

/* Sizeof `struct fpreg' in <machine/reg.h.  */
#define ARMBSD_SIZEOF_FPREGS	((1 + (8 * 3)) * 4)

static int
armbsd_fpreg_offset (int regnum)
{
  if (regnum == ARM_FPS_REGNUM)
    return 0;

  return 4 + (regnum - ARM_F0_REGNUM) * 12;
}

/* Supply register REGNUM from the buffer specified by FPREGS and LEN
   in the floating-point register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
armbsd_supply_fpregset (const struct regset *regset,
			struct regcache *regcache,
			int regnum, const void *fpregs, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) fpregs;
  int i;

  gdb_assert (len >= ARMBSD_SIZEOF_FPREGS);

  for (i = ARM_F0_REGNUM; i <= ARM_FPS_REGNUM; i++)
    {
      if (regnum == i || regnum == -1)
	regcache->raw_supply (i, regs + armbsd_fpreg_offset (i));
    }
}

/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
armbsd_supply_gregset (const struct regset *regset,
		       struct regcache *regcache,
		       int regnum, const void *gregs, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) gregs;
  int i;

  gdb_assert (len >= ARMBSD_SIZEOF_GREGS);

  for (i = ARM_A1_REGNUM; i <= ARM_PC_REGNUM; i++)
    {
      if (regnum == i || regnum == -1)
	regcache->raw_supply (i, regs + i * 4);
    }

  if (regnum == ARM_PS_REGNUM || regnum == -1)
    regcache->raw_supply (i, regs + 16 * 4);

  if (len >= ARMBSD_SIZEOF_GREGS + ARMBSD_SIZEOF_FPREGS)
    {
      regs += ARMBSD_SIZEOF_GREGS;
      len -= ARMBSD_SIZEOF_GREGS;
      armbsd_supply_fpregset (regset, regcache, regnum, regs, len);
    }
}

/* ARM register sets.  */

static const struct regset armbsd_gregset =
{
  NULL,
  armbsd_supply_gregset,
  NULL,
  REGSET_VARIABLE_SIZE
};

static const struct regset armbsd_fpregset =
{
  NULL,
  armbsd_supply_fpregset
};

/* Iterate over supported core file register note sections. */

void
armbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
				     iterate_over_regset_sections_cb *cb,
				     void *cb_data,
				     const struct regcache *regcache)
{
  cb (".reg", ARMBSD_SIZEOF_GREGS, ARMBSD_SIZEOF_GREGS, &armbsd_gregset, NULL,
      cb_data);
  cb (".reg2", ARMBSD_SIZEOF_FPREGS, ARMBSD_SIZEOF_FPREGS, &armbsd_fpregset,
      NULL, cb_data);
}
