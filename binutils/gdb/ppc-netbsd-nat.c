/* Native-dependent code for NetBSD/powerpc.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

   Contributed by Wasabi Systems, Inc.

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

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/pcb.h>

#include "gdbcore.h"
#include "inferior.h"
#include "regcache.h"

#include "ppc-tdep.h"
#include "ppc-netbsd-tdep.h"
#include "bsd-kvm.h"
#include "inf-ptrace.h"
#include "netbsd-nat.h"

struct ppc_nbsd_nat_target final : public nbsd_nat_target
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static ppc_nbsd_nat_target the_ppc_nbsd_nat_target;

/* Returns true if PT_GETREGS fetches this register.  */

static int
getregs_supplies (struct gdbarch *gdbarch, int regnum)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  return ((regnum >= tdep->ppc_gp0_regnum
	   && regnum < tdep->ppc_gp0_regnum + ppc_num_gprs)
	  || regnum == tdep->ppc_lr_regnum
	  || regnum == tdep->ppc_cr_regnum
	  || regnum == tdep->ppc_xer_regnum
	  || regnum == tdep->ppc_ctr_regnum
	  || regnum == gdbarch_pc_regnum (gdbarch));
}

/* Like above, but for PT_GETFPREGS.  */

static int
getfpregs_supplies (struct gdbarch *gdbarch, int regnum)
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

  return ((regnum >= tdep->ppc_fp0_regnum
	   && regnum < tdep->ppc_fp0_regnum + ppc_num_fprs)
	  || regnum == tdep->ppc_fpscr_regnum);
}

void
ppc_nbsd_nat_target::fetch_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  pid_t pid = regcache->ptid ().pid ();
  int lwp = regcache->ptid ().lwp ();

  if (regnum == -1 || getregs_supplies (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp) == -1)
	perror_with_name (_("Couldn't get registers"));

      ppc_supply_gregset (&ppcnbsd_gregset, regcache,
			  regnum, &regs, sizeof regs);
    }

  if (regnum == -1 || getfpregs_supplies (gdbarch, regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp) == -1)
	perror_with_name (_("Couldn't get FP registers"));

      ppc_supply_fpregset (&ppcnbsd_fpregset, regcache,
			   regnum, &fpregs, sizeof fpregs);
    }
}

void
ppc_nbsd_nat_target::store_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  pid_t pid = regcache->ptid ().pid ();
  int lwp = regcache->ptid ().lwp ();

  if (regnum == -1 || getregs_supplies (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp) == -1)
	perror_with_name (_("Couldn't get registers"));

      ppc_collect_gregset (&ppcnbsd_gregset, regcache,
			   regnum, &regs, sizeof regs);

      if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp) == -1)
	perror_with_name (_("Couldn't write registers"));
    }

  if (regnum == -1 || getfpregs_supplies (gdbarch, regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp) == -1)
	perror_with_name (_("Couldn't get FP registers"));

      ppc_collect_fpregset (&ppcnbsd_fpregset, regcache,
			    regnum, &fpregs, sizeof fpregs);

      if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp) == -1)
	perror_with_name (_("Couldn't set FP registers"));
    }
}

static int
ppcnbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  struct switchframe sf;
  struct callframe cf;
  struct gdbarch *gdbarch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int i;

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_sp == 0)
    return 0;

  read_memory (pcb->pcb_sp, (gdb_byte *)&sf, sizeof sf);
  regcache->raw_supply (tdep->ppc_cr_regnum, &sf.cr);
  regcache->raw_supply (tdep->ppc_gp0_regnum + 2, &sf.fixreg2);
  for (i = 0 ; i < 19 ; i++)
    regcache->raw_supply (tdep->ppc_gp0_regnum + 13 + i, &sf.fixreg[i]);

  read_memory(sf.sp, (gdb_byte *)&cf, sizeof(cf));
  regcache->raw_supply (tdep->ppc_gp0_regnum + 30, &cf.r30);
  regcache->raw_supply (tdep->ppc_gp0_regnum + 31, &cf.r31);
  regcache->raw_supply (tdep->ppc_gp0_regnum + 1, &cf.sp);

  read_memory(cf.sp, (gdb_byte *)&cf, sizeof(cf));
  regcache->raw_supply (tdep->ppc_lr_regnum, &cf.lr);
  regcache->raw_supply (gdbarch_pc_regnum (gdbarch), &cf.lr);

  return 1;
}

void _initialize_ppcnbsd_nat ();
void
_initialize_ppcnbsd_nat ()
{
  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (ppcnbsd_supply_pcb);

  add_inf_child_target (&the_ppc_nbsd_nat_target);
}
