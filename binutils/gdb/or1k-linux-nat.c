/* Native-dependent code for GNU/Linux OpenRISC.
   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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
#include "gregset.h"
#include "linux-nat.h"
#include "or1k-tdep.h"
#include "or1k-linux-tdep.h"
#include "inferior.h"

#include "elf/common.h"

#include <sys/ptrace.h>

/* OpenRISC Linux native additions to the default linux support.  */

class or1k_linux_nat_target final : public linux_nat_target
{
public:
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *regcache, int regnum) override;
  void store_registers (struct regcache *regcache, int regnum) override;

  /* Read suitable target description.  */
  const struct target_desc *read_description () override;
};

static or1k_linux_nat_target the_or1k_linux_nat_target;

/* Copy general purpose register REGNUM (or all gp regs if REGNUM == -1)
   from regset GREGS into REGCACHE.  */

static void
supply_gregset_regnum (struct regcache *regcache, const prgregset_t *gregs,
		       int regnum)
{
  int i;
  const elf_greg_t *regp = *gregs;

  /* Access all registers */
  if (regnum == -1)
    {
      /* We fill the general purpose registers.  */
      for (i = OR1K_ZERO_REGNUM + 1; i < OR1K_MAX_GPR_REGS; i++)
	regcache->raw_supply (i, regp + i);

      /* Supply OR1K_NPC_REGNUM from index 32.  */
      regcache->raw_supply (OR1K_NPC_REGNUM, regp + 32);

      /* Fill the inaccessible zero register with zero.  */
      regcache->raw_supply_zeroed (0);
    }
  else if (regnum == OR1K_ZERO_REGNUM)
    regcache->raw_supply_zeroed (0);
  else if (regnum == OR1K_NPC_REGNUM)
    regcache->raw_supply (OR1K_NPC_REGNUM, regp + 32);
  else if (regnum > OR1K_ZERO_REGNUM && regnum < OR1K_MAX_GPR_REGS)
    regcache->raw_supply (regnum, regp + regnum);
}

/* Copy all general purpose registers from regset GREGS into REGCACHE.  */

void
supply_gregset (struct regcache *regcache, const prgregset_t *gregs)
{
  supply_gregset_regnum (regcache, gregs, -1);
}

/* Copy general purpose register REGNUM (or all gp regs if REGNUM == -1)
   from REGCACHE into regset GREGS.  */

void
fill_gregset (const struct regcache *regcache, prgregset_t *gregs, int regnum)
{
  elf_greg_t *regp = *gregs;

  if (regnum == -1)
    {
      /* We fill the general purpose registers.  */
      for (int i = OR1K_ZERO_REGNUM + 1; i < OR1K_MAX_GPR_REGS; i++)
	regcache->raw_collect (i, regp + i);

      regcache->raw_collect (OR1K_NPC_REGNUM, regp + 32);
    }
  else if (regnum == OR1K_ZERO_REGNUM)
    /* Nothing to do here.  */
    ;
  else if (regnum > OR1K_ZERO_REGNUM && regnum < OR1K_MAX_GPR_REGS)
    regcache->raw_collect (regnum, regp + regnum);
  else if (regnum == OR1K_NPC_REGNUM)
    regcache->raw_collect (OR1K_NPC_REGNUM, regp + 32);
}

/* Transfering floating-point registers between GDB, inferiors and cores.
   Since OpenRISC floating-point registers are the same as GPRs these do
   nothing.  */

void
supply_fpregset (struct regcache *regcache, const gdb_fpregset_t *fpregs)
{
}

void
fill_fpregset (const struct regcache *regcache,
	       gdb_fpregset_t *fpregs, int regno)
{
}

/* Return a target description for the current target.  */

const struct target_desc *
or1k_linux_nat_target::read_description ()
{
  return tdesc_or1k_linux;
}

/* Fetch REGNUM (or all registers if REGNUM == -1) from the target
   into REGCACHE using PTRACE_GETREGSET.  */

void
or1k_linux_nat_target::fetch_registers (struct regcache *regcache, int regnum)
{
  int tid;

  tid = get_ptrace_pid (regcache->ptid());

  if ((regnum >= OR1K_ZERO_REGNUM && regnum < OR1K_MAX_GPR_REGS)
      || (regnum == OR1K_NPC_REGNUM)
      || (regnum == -1))
    {
      struct iovec iov;
      elf_gregset_t regs;

      iov.iov_base = &regs;
      iov.iov_len = sizeof (regs);

      if (ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS,
		  (PTRACE_TYPE_ARG3) &iov) == -1)
	perror_with_name (_("Couldn't get registers"));
      else
	supply_gregset_regnum (regcache, &regs, regnum);
    }

  /* Access to other SPRs has potential security issues, don't support them for
     now.  */
}

/* Store REGNUM (or all registers if REGNUM == -1) to the target
   from REGCACHE using PTRACE_SETREGSET.  */

void
or1k_linux_nat_target::store_registers (struct regcache *regcache, int regnum)
{
  int tid;

  tid = get_ptrace_pid (regcache->ptid ());

  if ((regnum >= OR1K_ZERO_REGNUM && regnum < OR1K_MAX_GPR_REGS)
      || (regnum == OR1K_NPC_REGNUM)
      || (regnum == -1))
    {
      struct iovec iov;
      elf_gregset_t regs;

      iov.iov_base = &regs;
      iov.iov_len = sizeof (regs);

      if (ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS,
		  (PTRACE_TYPE_ARG3) &iov) == -1)
	perror_with_name (_("Couldn't get registers"));
      else
	{
	  fill_gregset (regcache, &regs, regnum);

	  if (ptrace (PTRACE_SETREGSET, tid, NT_PRSTATUS,
		      (PTRACE_TYPE_ARG3) &iov) == -1)
	    perror_with_name (_("Couldn't set registers"));
	}
    }

  /* Access to SPRs has potential security issues, don't support them for
     now.  */
}

/* Initialize OpenRISC Linux native support.  */

void _initialize_or1k_linux_nat ();
void
_initialize_or1k_linux_nat ()
{
  /* Register the target.  */
  linux_target = &the_or1k_linux_nat_target;
  add_inf_child_target (&the_or1k_linux_nat_target);
}
