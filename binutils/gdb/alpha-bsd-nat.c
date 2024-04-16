/* Native-dependent code for Alpha BSD's.

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

/* We define this to get types like register_t.  */
#define _KERNTYPES
#include "defs.h"
#include "inferior.h"
#include "regcache.h"

#include "alpha-tdep.h"
#include "alpha-bsd-tdep.h"
#include "inf-ptrace.h"
#include "netbsd-nat.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

struct alpha_bsd_nat_target final : public nbsd_nat_target
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static alpha_bsd_nat_target the_alpha_bsd_nat_target;

/* Determine if PT_GETREGS fetches this register.  */

static int
getregs_supplies (int regno)
{
  return ((regno >= ALPHA_V0_REGNUM && regno <= ALPHA_ZERO_REGNUM)
	  || regno >= ALPHA_PC_REGNUM);
}

/* Fetch register REGNO from the inferior.  If REGNO is -1, do this
   for all registers (including the floating point registers).  */

void
alpha_bsd_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  int lwp = regcache->ptid ().lwp ();

  if (regno == -1 || getregs_supplies (regno))
    {
      struct reg gregs;

      if (ptrace (PT_GETREGS, regcache->ptid ().pid (),
		  (PTRACE_TYPE_ARG3) &gregs, lwp) == -1)
	perror_with_name (_("Couldn't get registers"));

      alphabsd_supply_reg (regcache, (char *) &gregs, regno);
      if (regno != -1)
	return;
    }

  if (regno == -1
      || regno >= gdbarch_fp0_regnum (regcache->arch ()))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, regcache->ptid ().pid (),
		  (PTRACE_TYPE_ARG3) &fpregs, lwp) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      alphabsd_supply_fpreg (regcache, (char *) &fpregs, regno);
    }
}

/* Store register REGNO back into the inferior.  If REGNO is -1, do
   this for all registers (including the floating point registers).  */

void
alpha_bsd_nat_target::store_registers (struct regcache *regcache, int regno)
{
  int lwp = regcache->ptid ().lwp ();

  if (regno == -1 || getregs_supplies (regno))
    {
      struct reg gregs;
      if (ptrace (PT_GETREGS, regcache->ptid ().pid (),
		  (PTRACE_TYPE_ARG3) &gregs, lwp) == -1)
	perror_with_name (_("Couldn't get registers"));

      alphabsd_fill_reg (regcache, (char *) &gregs, regno);

      if (ptrace (PT_SETREGS, regcache->ptid ().pid (),
		  (PTRACE_TYPE_ARG3) &gregs, lwp) == -1)
	perror_with_name (_("Couldn't write registers"));

      if (regno != -1)
	return;
    }

  if (regno == -1
      || regno >= gdbarch_fp0_regnum (regcache->arch ()))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, regcache->ptid ().pid (),
		  (PTRACE_TYPE_ARG3) &fpregs, lwp) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      alphabsd_fill_fpreg (regcache, (char *) &fpregs, regno);

      if (ptrace (PT_SETFPREGS, regcache->ptid ().pid (),
		  (PTRACE_TYPE_ARG3) &fpregs, lwp) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}


/* Support for debugging kernel virtual memory images.  */

#include <sys/signal.h>
#include <machine/pcb.h>

#include "bsd-kvm.h"

static int
alphabsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  int regnum;

  /* The following is true for OpenBSD 3.9:

     The pcb contains the register state at the context switch inside
     cpu_switch().  */

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_hw.apcb_ksp == 0)
    return 0;

  regcache->raw_supply (ALPHA_SP_REGNUM, &pcb->pcb_hw.apcb_ksp);

  for (regnum = ALPHA_S0_REGNUM; regnum < ALPHA_A0_REGNUM; regnum++)
    regcache->raw_supply (regnum, &pcb->pcb_context[regnum - ALPHA_S0_REGNUM]);
  regcache->raw_supply (ALPHA_RA_REGNUM, &pcb->pcb_context[7]);

  return 1;
}


void _initialize_alphabsd_nat ();
void
_initialize_alphabsd_nat ()
{
  add_inf_child_target (&the_alpha_bsd_nat_target);

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (alphabsd_supply_pcb);
}
