/* Native-dependent code for modern VAX BSD's.

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

/* We define this to get types like register_t.  */
#define _KERNTYPES
#include "defs.h"
#include "inferior.h"
#include "regcache.h"
#include "target.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "vax-tdep.h"
#include "inf-ptrace.h"
#include "netbsd-nat.h"

struct vax_bsd_nat_target final : public nbsd_nat_target
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static vax_bsd_nat_target the_vax_bsd_nat_target;

/* Supply the general-purpose registers stored in GREGS to REGCACHE.  */

static void
vaxbsd_supply_gregset (struct regcache *regcache, const void *gregs)
{
  const gdb_byte *regs = (const gdb_byte *)gregs;
  int regnum;

  for (regnum = 0; regnum < VAX_NUM_REGS; regnum++)
    regcache->raw_supply (regnum, regs + regnum * 4);
}

/* Collect the general-purpose registers from REGCACHE and store them
   in GREGS.  */

static void
vaxbsd_collect_gregset (const struct regcache *regcache,
			void *gregs, int regnum)
{
  gdb_byte *regs = (void *)gregs;
  int i;

  for (i = 0; i <= VAX_NUM_REGS; i++)
    {
      if (regnum == -1 || regnum == i)
	regcache->raw_collect (i, regs + i * 4);
    }
}


/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

void
vax_bsd_nat_target::fetch_registers (struct regcache *regcache, int regnum)
{
  struct reg regs;
  pid_t pid = regcache->ptid ().pid ();
  int lwp = regcache->ptid ().lwp ();

  if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp) == -1)
    perror_with_name (_("Couldn't get registers"));

  vaxbsd_supply_gregset (regcache, &regs);
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

void
vax_bsd_nat_target::store_registers (struct regcache *regcache, int regnum)
{
  struct reg regs;
  pid_t pid = regcache->ptid ().pid ();
  int lwp = regcache->ptid ().lwp ();

  if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp) == -1)
    perror_with_name (_("Couldn't get registers"));

  vaxbsd_collect_gregset (regcache, &regs, regnum);

  if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp) == -1)
    perror_with_name (_("Couldn't write registers"));
}


/* Support for debugging kernel virtual memory images.  */

#include <machine/pcb.h>

#include "bsd-kvm.h"

static int
vaxbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  int regnum;

  /* The following is true for OpenBSD 3.5:

     The pcb contains the register state at the context switch inside
     cpu_switch().  */

  /* The stack pointer shouldn't be zero.  */
  if (pcb->KSP == 0)
    return 0;

  for (regnum = VAX_R0_REGNUM; regnum < VAX_AP_REGNUM; regnum++)
    regcache->raw_supply (regnum, &pcb->R[regnum - VAX_R0_REGNUM]);
  regcache->raw_supply (VAX_AP_REGNUM, &pcb->AP);
  regcache->raw_supply (VAX_FP_REGNUM, &pcb->FP);
  regcache->raw_supply (VAX_SP_REGNUM, &pcb->KSP);
  regcache->raw_supply (VAX_PC_REGNUM, &pcb->PC);
  regcache->raw_supply (VAX_PS_REGNUM, &pcb->PSL);

  return 1;
}

void _initialize_vaxbsd_nat ();
void
_initialize_vaxbsd_nat ()
{
  add_inf_child_target (&the_vax_bsd_nat_target);

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (vaxbsd_supply_pcb);
}
