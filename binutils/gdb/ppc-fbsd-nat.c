/* Native-dependent code for PowerPC's running FreeBSD, for GDB.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "regcache.h"

#include <sys/types.h>
#include <sys/procfs.h>
#include <sys/ptrace.h>
#include <sys/signal.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/reg.h>

#include "fbsd-nat.h"
#include "gregset.h"
#include "ppc-tdep.h"
#include "ppc-fbsd-tdep.h"
#include "inf-ptrace.h"
#include "bsd-kvm.h"

struct ppc_fbsd_nat_target final : public fbsd_nat_target
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static ppc_fbsd_nat_target the_ppc_fbsd_nat_target;

/* Fill GDB's register array with the general-purpose register values
   in *GREGSETP.  */

void
supply_gregset (struct regcache *regcache, const gdb_gregset_t *gregsetp)
{
  const struct regset *regset = ppc_fbsd_gregset (sizeof (long));

  ppc_supply_gregset (regset, regcache, -1, gregsetp, sizeof (*gregsetp));
}

/* Fill register REGNO (if a gpr) in *GREGSETP with the value in GDB's
   register array. If REGNO is -1 do it for all registers.  */

void
fill_gregset (const struct regcache *regcache,
	      gdb_gregset_t *gregsetp, int regno)
{
  const struct regset *regset = ppc_fbsd_gregset (sizeof (long));

  if (regno == -1)
    memset (gregsetp, 0, sizeof (*gregsetp));
  ppc_collect_gregset (regset, regcache, regno, gregsetp, sizeof (*gregsetp));
}

/* Fill GDB's register array with the floating-point register values
   in *FPREGSETP.  */

void
supply_fpregset (struct regcache *regcache, const gdb_fpregset_t * fpregsetp)
{
  const struct regset *regset = ppc_fbsd_fpregset ();

  ppc_supply_fpregset (regset, regcache, -1,
		       fpregsetp, sizeof (*fpregsetp));
}

/* Fill register REGNO in *FGREGSETP with the value in GDB's
   register array. If REGNO is -1 do it for all registers.  */

void
fill_fpregset (const struct regcache *regcache,
	       gdb_fpregset_t *fpregsetp, int regno)
{
  const struct regset *regset = ppc_fbsd_fpregset ();

  ppc_collect_fpregset (regset, regcache, regno,
			fpregsetp, sizeof (*fpregsetp));
}

/* Returns true if PT_GETFPREGS fetches this register.  */

static int
getfpregs_supplies (struct gdbarch *gdbarch, int regno)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  /* FIXME: jimb/2004-05-05: Some PPC variants don't have floating
	 point registers.  Traditionally, GDB's register set has still
	 listed the floating point registers for such machines, so this
	 code is harmless.  However, the new E500 port actually omits the
	 floating point registers entirely from the register set --- they
	 don't even have register numbers assigned to them.

	 It's not clear to me how best to update this code, so this assert
	 will alert the first person to encounter the NetBSD/E500
	 combination to the problem.  */

  gdb_assert (ppc_floating_point_unit_p (gdbarch));

  return ((regno >= tdep->ppc_fp0_regnum
	   && regno < tdep->ppc_fp0_regnum + ppc_num_fprs)
	  || regno == tdep->ppc_fpscr_regnum);
}

/* Fetch register REGNO from the child process. If REGNO is -1, do it
   for all registers.  */

void
ppc_fbsd_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  gdb_gregset_t regs;
  pid_t pid = regcache->ptid ().lwp ();

  if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
    perror_with_name (_("Couldn't get registers"));

  supply_gregset (regcache, &regs);

  if (regno == -1 || getfpregs_supplies (regcache->arch (), regno))
    {
      const struct regset *fpregset = ppc_fbsd_fpregset ();
      gdb_fpregset_t fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get FP registers"));

      ppc_supply_fpregset (fpregset, regcache, regno, &fpregs, sizeof fpregs);
    }
}

/* Store register REGNO back into the child process. If REGNO is -1,
   do this for all registers.  */

void
ppc_fbsd_nat_target::store_registers (struct regcache *regcache, int regno)
{
  gdb_gregset_t regs;
  pid_t pid = regcache->ptid ().lwp ();

  if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
    perror_with_name (_("Couldn't get registers"));

  fill_gregset (regcache, &regs, regno);

  if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
    perror_with_name (_("Couldn't write registers"));

  if (regno == -1 || getfpregs_supplies (regcache->arch (), regno))
    {
      gdb_fpregset_t fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get FP registers"));

      fill_fpregset (regcache, &fpregs, regno);

      if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't set FP registers"));
    }
}

/* Architecture specific function that reconstructs the
   register state from PCB (Process Control Block) and supplies it
   to REGCACHE.  */

static int
ppcfbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  struct gdbarch *gdbarch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int i, regnum;

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_sp == 0)
    return 0;

  regcache->raw_supply (gdbarch_sp_regnum (gdbarch), &pcb->pcb_sp);
  regcache->raw_supply (tdep->ppc_cr_regnum, &pcb->pcb_cr);
  regcache->raw_supply (tdep->ppc_lr_regnum, &pcb->pcb_lr);
  for (i = 0, regnum = tdep->ppc_gp0_regnum + 14; i < 20; i++, regnum++)
    regcache->raw_supply (regnum, &pcb->pcb_context[i]);

  return 1;
}

void _initialize_ppcfbsd_nat ();
void
_initialize_ppcfbsd_nat ()
{
  add_inf_child_target (&the_ppc_fbsd_nat_target);

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (ppcfbsd_supply_pcb);
}
