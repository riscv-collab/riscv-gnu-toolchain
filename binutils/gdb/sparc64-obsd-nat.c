/* Native-dependent code for OpenBSD/sparc64.

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
#include "gdbcore.h"
#include "regcache.h"
#include "target.h"

#include "sparc64-tdep.h"
#include "sparc-nat.h"
#include "obsd-nat.h"

/* Determine whether `gregset_t' contains register REGNUM.  */

static int
sparc64obsd_gregset_supplies_p (struct gdbarch *gdbarch, int regnum)
{
  /* Integer registers.  */
  if ((regnum >= SPARC_G1_REGNUM && regnum <= SPARC_G7_REGNUM)
      || (regnum >= SPARC_O0_REGNUM && regnum <= SPARC_O7_REGNUM)
      || (regnum >= SPARC_L0_REGNUM && regnum <= SPARC_L7_REGNUM)
      || (regnum >= SPARC_I0_REGNUM && regnum <= SPARC_I7_REGNUM))
    return 1;

  /* Control registers.  */
  if (regnum == SPARC64_PC_REGNUM
      || regnum == SPARC64_NPC_REGNUM
      || regnum == SPARC64_STATE_REGNUM
      || regnum == SPARC64_Y_REGNUM)
    return 1;

  return 0;
}

/* Determine whether `fpregset_t' contains register REGNUM.  */

static int
sparc64obsd_fpregset_supplies_p (struct gdbarch *gdbarch, int regnum)
{
  /* Floating-point registers.  */
  if ((regnum >= SPARC_F0_REGNUM && regnum <= SPARC_F31_REGNUM)
      || (regnum >= SPARC64_F32_REGNUM && regnum <= SPARC64_F62_REGNUM))
    return 1;

  /* Control registers.  */
  if (regnum == SPARC64_FSR_REGNUM)
    return 1;

  return 0;
}


/* Support for debugging kernel virtual memory images.  */

#include <sys/types.h>
#include <machine/pcb.h>

#include "bsd-kvm.h"

static int
sparc64obsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  u_int64_t state;
  int regnum;

  /* The following is true for OpenBSD 3.0:

     The pcb contains %sp and %pc, %pstate and %cwp.  From this
     information we reconstruct the register state as it would look
     when we just returned from cpu_switch().  */

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_sp == 0)
    return 0;

  /* If the program counter is zero, this is probably a core dump, and
     we can get %pc from the stack.  */
  if (pcb->pcb_pc == 0)
      read_memory(pcb->pcb_sp + BIAS - 176 + (11 * 8), 
		  (gdb_byte *)&pcb->pcb_pc, sizeof pcb->pcb_pc);

  regcache->raw_supply (SPARC_SP_REGNUM, &pcb->pcb_sp);
  regcache->raw_supply (SPARC64_PC_REGNUM, &pcb->pcb_pc);

  state = pcb->pcb_pstate << 8 | pcb->pcb_cwp;
  regcache->raw_supply (SPARC64_STATE_REGNUM, &state);

  sparc_supply_rwindow (regcache, pcb->pcb_sp, -1);

  return 1;
}

/* Add some extra features to the generic SPARC target.  */
static sparc_target<obsd_nat_target> the_sparc64_obsd_nat_target;

void _initialize_sparc64obsd_nat ();
void
_initialize_sparc64obsd_nat ()
{
  sparc_supply_gregset = sparc64_supply_gregset;
  sparc_collect_gregset = sparc64_collect_gregset;
  sparc_supply_fpregset = sparc64_supply_fpregset;
  sparc_collect_fpregset = sparc64_collect_fpregset;
  sparc_gregset_supplies_p = sparc64obsd_gregset_supplies_p;
  sparc_fpregset_supplies_p = sparc64obsd_fpregset_supplies_p;

  sparc_gregmap = &sparc64nbsd_gregmap;
  sparc_fpregmap = &sparc64_bsd_fpregmap;

  /* Add some extra features to the generic SPARC target.  */
  add_inf_child_target (&the_sparc64_obsd_nat_target);

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (sparc64obsd_supply_pcb);
}
