/* Native-dependent code for OpenBSD/mips64.

   Copyright (C) 2004-2024 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "regcache.h"
#include "target.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "mips-tdep.h"
#include "inf-ptrace.h"
#include "obsd-nat.h"

/* Shorthand for some register numbers used below.  */
#define MIPS_PC_REGNUM	MIPS_EMBED_PC_REGNUM
#define MIPS_FP0_REGNUM	MIPS_EMBED_FP0_REGNUM
#define MIPS_FSR_REGNUM MIPS_EMBED_FP0_REGNUM + 32

struct mips64_obsd_nat_target final : public obsd_nat_target
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static mips64_obsd_nat_target the_mips64_obsd_nat_target;

/* Supply the general-purpose registers stored in GREGS to REGCACHE.  */

static void
mips64obsd_supply_gregset (struct regcache *regcache, const void *gregs)
{
  const char *regs = gregs;
  int regnum;

  for (regnum = MIPS_ZERO_REGNUM; regnum <= MIPS_PC_REGNUM; regnum++)
    regcache->raw_supply (regnum, regs + regnum * 8);

  for (regnum = MIPS_FP0_REGNUM; regnum <= MIPS_FSR_REGNUM; regnum++)
    regcache->raw_supply (regnum, regs + (regnum + 2) * 8);
}

/* Collect the general-purpose registers from REGCACHE and store them
   in GREGS.  */

static void
mips64obsd_collect_gregset (const struct regcache *regcache,
			    void *gregs, int regnum)
{
  char *regs = gregs;
  int i;

  for (i = MIPS_ZERO_REGNUM; i <= MIPS_PC_REGNUM; i++)
    {
      if (regnum == -1 || regnum == i)
	regcache->raw_collect (i, regs + i * 8);
    }

  for (i = MIPS_FP0_REGNUM; i <= MIPS_FSR_REGNUM; i++)
    {
      if (regnum == -1 || regnum == i)
	regcache->raw_collect (i, regs + (i + 2) * 8);
    }
}


/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

void
mips64_obsd_nat_target::fetch_registers (struct regcache *regcache, int regnum)
{
  struct reg regs;
  pid_t pid = regcache->ptid ().pid ();

  if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
    perror_with_name (_("Couldn't get registers"));

  mips64obsd_supply_gregset (regcache, &regs);
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

static void
mips64_obsd_nat_target::store_registers (struct regcache *regcache, int regnum)
{
  struct reg regs;
  pid_t pid = regcache->ptid ().pid ();

  if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
    perror_with_name (_("Couldn't get registers"));

  mips64obsd_collect_gregset (regcache, &regs, regnum);

  if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
    perror_with_name (_("Couldn't write registers"));
}

void _initialize_mips64obsd_nat ();
void
_initialize_mips64obsd_nat ()
{
  add_inf_child_target (&the_mips64_obsd_nat_target);
}
