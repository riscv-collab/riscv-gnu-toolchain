/* Native-dependent code for NetBSD/sparc.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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
#include "target.h"

#include "sparc-tdep.h"
#include "sparc-nat.h"

/* Support for debugging kernel virtual memory images.  */

#include <sys/types.h>
#include <machine/pcb.h>

#include "bsd-kvm.h"

static int
sparc32nbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  /* The following is true for NetBSD 1.6.2:

     The pcb contains %sp, %pc, %psr and %wim.  From this information
     we reconstruct the register state as it would look when we just
     returned from cpu_switch().  */

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_sp == 0)
    return 0;

  regcache->raw_supply (SPARC_SP_REGNUM, &pcb->pcb_sp);
  regcache->raw_supply (SPARC_O7_REGNUM, &pcb->pcb_pc);
  regcache->raw_supply (SPARC32_PSR_REGNUM, &pcb->pcb_psr);
  regcache->raw_supply (SPARC32_WIM_REGNUM, &pcb->pcb_wim);
  regcache->raw_supply (SPARC32_PC_REGNUM, &pcb->pcb_pc);

  sparc_supply_rwindow (regcache, pcb->pcb_sp, -1);

  return 1;
}

static sparc_target<inf_ptrace_target> the_sparc_nbsd_nat_target;

void _initialize_sparcnbsd_nat ();
void
_initialize_sparcnbsd_nat ()
{
  sparc_gregmap = &sparc32nbsd_gregmap;
  sparc_fpregmap = &sparc32_bsd_fpregmap;

  add_inf_child_target (&sparc_nbsd_nat_target);

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (sparc32nbsd_supply_pcb);
}
