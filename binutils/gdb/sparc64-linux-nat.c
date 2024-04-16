/* Native-dependent code for GNU/Linux UltraSPARC.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "regcache.h"

#include <sys/procfs.h>
#include "gregset.h"

#include "sparc64-tdep.h"
#include "sparc-tdep.h"
#include "sparc-nat.h"
#include "inferior.h"
#include "target.h"
#include "linux-nat.h"

class sparc64_linux_nat_target final : public linux_nat_target
{
public:
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *regcache, int regnum) override
  { sparc_fetch_inferior_registers (this, regcache, regnum); }

  void store_registers (struct regcache *regcache, int regnum) override
  { sparc_store_inferior_registers (this, regcache, regnum); }

  /* Override linux_nat_target low methods.  */

  /* ADI support */
  void low_forget_process (pid_t pid) override
  { sparc64_forget_process (pid); }
};

static sparc64_linux_nat_target the_sparc64_linux_nat_target;

static const struct sparc_gregmap sparc64_linux_ptrace_gregmap =
{
  16 * 8,			/* "tstate" */
  17 * 8,			/* %pc */
  18 * 8,			/* %npc */
  19 * 8,			/* %y */
  -1,				/* %wim */
  -1,				/* %tbr */
  0 * 8,			/* %g1 */
  -1,				/* %l0 */
  4				/* sizeof (%y) */
};


void
supply_gregset (struct regcache *regcache, const prgregset_t *gregs)
{
  sparc64_supply_gregset (sparc_gregmap, regcache, -1, gregs);
}

void
supply_fpregset (struct regcache *regcache, const prfpregset_t *fpregs)
{
  sparc64_supply_fpregset (&sparc64_bsd_fpregmap, regcache, -1, fpregs);
}

void
fill_gregset (const struct regcache *regcache, prgregset_t *gregs, int regnum)
{
  sparc64_collect_gregset (sparc_gregmap, regcache, regnum, gregs);
}

void
fill_fpregset (const struct regcache *regcache,
	       prfpregset_t *fpregs, int regnum)
{
  sparc64_collect_fpregset (&sparc64_bsd_fpregmap, regcache, regnum, fpregs);
}

void _initialize_sparc64_linux_nat ();
void
_initialize_sparc64_linux_nat ()
{
  sparc_fpregmap = &sparc64_bsd_fpregmap;

  /* Register the target.  */
  linux_target = &the_sparc64_linux_nat_target;
  add_inf_child_target (&the_sparc64_linux_nat_target);

  sparc_gregmap = &sparc64_linux_ptrace_gregmap;
}
