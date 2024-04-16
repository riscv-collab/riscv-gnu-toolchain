/* Native-dependent code for FreeBSD/sparc64.

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
#include "target.h"

#include "fbsd-nat.h"
#include "sparc64-tdep.h"
#include "sparc-nat.h"


/* Support for debugging kernel virtual memory images.  */

#include <sys/types.h>
#include <machine/pcb.h>

#include "bsd-kvm.h"

static int
sparc64fbsd_kvm_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  /* The following is true for FreeBSD 5.4:

     The pcb contains %sp and %pc.  Since the register windows are
     explicitly flushed, we can find the `local' and `in' registers on
     the stack.  */

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_sp == 0)
    return 0;

  regcache->raw_supply (SPARC_SP_REGNUM, &pcb->pcb_sp);
  regcache->raw_supply (SPARC64_PC_REGNUM, &pcb->pcb_pc);

  /* Synthesize %npc.  */
  pcb->pcb_pc += 4;
  regcache->raw_supply (SPARC64_NPC_REGNUM, &pcb->pcb_pc);

  /* Read `local' and `in' registers from the stack.  */
  sparc_supply_rwindow (regcache, pcb->pcb_sp, -1);

  return 1;
}

/* Add some extra features to the generic SPARC target.  */
static sparc_target<fbsd_nat_target> the_sparc64_fbsd_nat_target;

void _initialize_sparc64fbsd_nat ();
void
_initialize_sparc64fbsd_nat ()
{
  add_inf_child_target (&the_sparc64_fbsd_nat_target);

  sparc_gregmap = &sparc64fbsd_gregmap;

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (sparc64fbsd_kvm_supply_pcb);
}
