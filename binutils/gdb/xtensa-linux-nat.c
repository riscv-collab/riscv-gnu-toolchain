/* Xtensa GNU/Linux native support.

   Copyright (C) 2007-2024 Free Software Foundation, Inc.

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
#include "frame.h"
#include "inferior.h"
#include "gdbcore.h"
#include "regcache.h"
#include "target.h"
#include "linux-nat.h"
#include <sys/types.h>
#include <signal.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include "gdbsupport/gdb_wait.h"
#include <fcntl.h>
#include <sys/procfs.h>
#include "nat/gdb_ptrace.h"
#include <asm/ptrace.h>

#include "gregset.h"
#include "xtensa-tdep.h"

/* Defines ps_err_e, struct ps_prochandle.  */
#include "gdb_proc_service.h"

/* Extended register set depends on hardware configs.
   Keeping these definitions separately allows to introduce
   hardware-specific overlays.  */
#include "xtensa-xtregs.c"

class xtensa_linux_nat_target final : public linux_nat_target
{
public:
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static xtensa_linux_nat_target the_xtensa_linux_nat_target;

void
fill_gregset (const struct regcache *regcache,
	      gdb_gregset_t *gregsetp, int regnum)
{
  int i;
  xtensa_elf_gregset_t *regs = (xtensa_elf_gregset_t *) gregsetp;
  struct gdbarch *gdbarch = regcache->arch ();
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  if (regnum == gdbarch_pc_regnum (gdbarch) || regnum == -1)
    regcache->raw_collect (gdbarch_pc_regnum (gdbarch), &regs->pc);
  if (regnum == gdbarch_ps_regnum (gdbarch) || regnum == -1)
    regcache->raw_collect (gdbarch_ps_regnum (gdbarch), &regs->ps);

  if (regnum == tdep->wb_regnum || regnum == -1)
    regcache->raw_collect (tdep->wb_regnum,
			   &regs->windowbase);
  if (regnum == tdep->ws_regnum || regnum == -1)
    regcache->raw_collect (tdep->ws_regnum,
			   &regs->windowstart);
  if (regnum == tdep->lbeg_regnum || regnum == -1)
    regcache->raw_collect (tdep->lbeg_regnum,
			   &regs->lbeg);
  if (regnum == tdep->lend_regnum || regnum == -1)
    regcache->raw_collect (tdep->lend_regnum,
			   &regs->lend);
  if (regnum == tdep->lcount_regnum || regnum == -1)
    regcache->raw_collect (tdep->lcount_regnum,
			   &regs->lcount);
  if (regnum == tdep->sar_regnum || regnum == -1)
    regcache->raw_collect (tdep->sar_regnum,
			   &regs->sar);
  if (regnum == tdep->threadptr_regnum || regnum == -1)
    regcache->raw_collect (tdep->threadptr_regnum,
			   &regs->threadptr);
  if (regnum >=tdep->ar_base
      && regnum < tdep->ar_base
		    + tdep->num_aregs)
    regcache->raw_collect (regnum,
			   &regs->ar[regnum - tdep->ar_base]);
  else if (regnum == -1)
    {
      for (i = 0; i < tdep->num_aregs; ++i)
	regcache->raw_collect (tdep->ar_base + i,
			       &regs->ar[i]);
    }
  if (regnum >= tdep->a0_base
      && regnum < tdep->a0_base + C0_NREGS)
    regcache->raw_collect (regnum,
			   &regs->ar[(4 * regs->windowbase + regnum
				      - tdep->a0_base)
			  % tdep->num_aregs]);
  else if (regnum == -1)
    {
      for (i = 0; i < C0_NREGS; ++i)
	regcache->raw_collect (tdep->a0_base + i,
			       (&regs->ar[(4 * regs->windowbase + i)
				% tdep->num_aregs]));
    }
}

static void
supply_gregset_reg (struct regcache *regcache,
		    const gdb_gregset_t *gregsetp, int regnum)
{
  int i;
  xtensa_elf_gregset_t *regs = (xtensa_elf_gregset_t *) gregsetp;

  struct gdbarch *gdbarch = regcache->arch ();
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  if (regnum == gdbarch_pc_regnum (gdbarch) || regnum == -1)
    regcache->raw_supply (gdbarch_pc_regnum (gdbarch), &regs->pc);
  if (regnum == gdbarch_ps_regnum (gdbarch) || regnum == -1)
    regcache->raw_supply (gdbarch_ps_regnum (gdbarch), &regs->ps);

  if (regnum == tdep->wb_regnum || regnum == -1)
    regcache->raw_supply (tdep->wb_regnum,
			  &regs->windowbase);
  if (regnum == tdep->ws_regnum || regnum == -1)
    regcache->raw_supply (tdep->ws_regnum,
			  &regs->windowstart);
  if (regnum == tdep->lbeg_regnum || regnum == -1)
    regcache->raw_supply (tdep->lbeg_regnum,
			  &regs->lbeg);
  if (regnum == tdep->lend_regnum || regnum == -1)
    regcache->raw_supply (tdep->lend_regnum,
			  &regs->lend);
  if (regnum == tdep->lcount_regnum || regnum == -1)
    regcache->raw_supply (tdep->lcount_regnum,
			  &regs->lcount);
  if (regnum == tdep->sar_regnum || regnum == -1)
    regcache->raw_supply (tdep->sar_regnum,
			  &regs->sar);
  if (regnum == tdep->threadptr_regnum || regnum == -1)
    regcache->raw_supply (tdep->threadptr_regnum,
			  &regs->threadptr);
  if (regnum >=tdep->ar_base
      && regnum < tdep->ar_base
		    + tdep->num_aregs)
    regcache->raw_supply (regnum,
			  &regs->ar[regnum - tdep->ar_base]);
  else if (regnum == -1)
    {
      for (i = 0; i < tdep->num_aregs; ++i)
	regcache->raw_supply (tdep->ar_base + i,
			      &regs->ar[i]);
    }
  if (regnum >= tdep->a0_base
      && regnum < tdep->a0_base + C0_NREGS)
    regcache->raw_supply (regnum,
			  &regs->ar[(4 * regs->windowbase + regnum
				     - tdep->a0_base)
			 % tdep->num_aregs]);
  else if (regnum == -1)
    {
      for (i = 0; i < C0_NREGS; ++i)
	regcache->raw_supply (tdep->a0_base + i,
			      &regs->ar[(4 * regs->windowbase + i)
					% tdep->num_aregs]);
    }
}

void
supply_gregset (struct regcache *regcache, const gdb_gregset_t *gregsetp)
{
  supply_gregset_reg (regcache, gregsetp, -1);
}

void
fill_fpregset (const struct regcache *regcache,
	       gdb_fpregset_t *fpregsetp, int regnum)
{
  return;
}

void 
supply_fpregset (struct regcache *regcache,
		 const gdb_fpregset_t *fpregsetp)
{
  return;
}

/* Fetch greg-register(s) from process/thread TID
   and store value(s) in GDB's register array.  */

static void
fetch_gregs (struct regcache *regcache, int regnum)
{
  int tid = regcache->ptid ().lwp ();
  gdb_gregset_t regs;
  
  if (ptrace (PTRACE_GETREGS, tid, 0, (long) &regs) < 0)
    {
      perror_with_name (_("Couldn't get registers"));
      return;
    }
 
  supply_gregset_reg (regcache, &regs, regnum);
}

/* Store greg-register(s) in GDB's register 
   array into the process/thread specified by TID.  */

static void
store_gregs (struct regcache *regcache, int regnum)
{
  int tid = regcache->ptid ().lwp ();
  gdb_gregset_t regs;

  if (ptrace (PTRACE_GETREGS, tid, 0, (long) &regs) < 0)
    {
      perror_with_name (_("Couldn't get registers"));
      return;
    }

  fill_gregset (regcache, &regs, regnum);

  if (ptrace (PTRACE_SETREGS, tid, 0, (long) &regs) < 0)
    {
      perror_with_name (_("Couldn't write registers"));
      return;
    }
}

static int xtreg_lo;
static int xtreg_high;

/* Fetch/Store Xtensa TIE registers.  Xtensa GNU/Linux PTRACE
   interface provides special requests for this.  */

static void
fetch_xtregs (struct regcache *regcache, int regnum)
{
  int tid = regcache->ptid ().lwp ();
  const xtensa_regtable_t *ptr;
  char xtregs [XTENSA_ELF_XTREG_SIZE];

  if (ptrace (PTRACE_GETXTREGS, tid, 0, (long)&xtregs) < 0)
    perror_with_name (_("Couldn't get extended registers"));

  for (ptr = xtensa_regmap_table; ptr->name; ptr++)
    if (regnum == ptr->gdb_regnum || regnum == -1)
      regcache->raw_supply (ptr->gdb_regnum, xtregs + ptr->ptrace_offset);
}

static void
store_xtregs (struct regcache *regcache, int regnum)
{
  int tid = regcache->ptid ().lwp ();
  const xtensa_regtable_t *ptr;
  char xtregs [XTENSA_ELF_XTREG_SIZE];

  if (ptrace (PTRACE_GETXTREGS, tid, 0, (long)&xtregs) < 0)
    perror_with_name (_("Couldn't get extended registers"));

  for (ptr = xtensa_regmap_table; ptr->name; ptr++)
    if (regnum == ptr->gdb_regnum || regnum == -1)
      regcache->raw_collect (ptr->gdb_regnum, xtregs + ptr->ptrace_offset);

  if (ptrace (PTRACE_SETXTREGS, tid, 0, (long)&xtregs) < 0)
    perror_with_name (_("Couldn't write extended registers"));
}

void
xtensa_linux_nat_target::fetch_registers (struct regcache *regcache,
					  int regnum)
{
  if (regnum == -1)
    {
      fetch_gregs (regcache, regnum);
      fetch_xtregs (regcache, regnum);
    }
  else if ((regnum < xtreg_lo) || (regnum > xtreg_high))
    fetch_gregs (regcache, regnum);
  else
    fetch_xtregs (regcache, regnum);
}

void
xtensa_linux_nat_target::store_registers (struct regcache *regcache,
					  int regnum)
{
  if (regnum == -1)
    {
      store_gregs (regcache, regnum);
      store_xtregs (regcache, regnum);
    }
  else if ((regnum < xtreg_lo) || (regnum > xtreg_high))
    store_gregs (regcache, regnum);
  else
    store_xtregs (regcache, regnum);
}

/* Called by libthread_db.  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  xtensa_elf_gregset_t regs;

  if (ptrace (PTRACE_GETREGS, lwpid, NULL, &regs) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) ((char *) regs.threadptr - idx);

  return PS_OK;
}

void _initialize_xtensa_linux_nat ();
void
_initialize_xtensa_linux_nat ()
{
  const xtensa_regtable_t *ptr;

  /* Calculate the number range for extended registers.  */
  xtreg_lo = 1000000000;
  xtreg_high = -1;
  for (ptr = xtensa_regmap_table; ptr->name; ptr++)
    {
      if (ptr->gdb_regnum < xtreg_lo)
	xtreg_lo = ptr->gdb_regnum;
      if (ptr->gdb_regnum > xtreg_high)
	xtreg_high = ptr->gdb_regnum;
    }

  linux_target = &the_xtensa_linux_nat_target;
  add_inf_child_target (&the_xtensa_linux_nat_target);
}
