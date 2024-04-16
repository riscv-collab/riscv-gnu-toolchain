/* Native-dependent code for GNU/Linux TILE-Gx.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "gdbcore.h"
#include "regcache.h"
#include "linux-nat.h"
#include "inf-ptrace.h"

#include "nat/gdb_ptrace.h"

#include <sys/procfs.h>

/* Defines ps_err_e, struct ps_prochandle.  */
#include "gdb_proc_service.h"

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"

class tilegx_linux_nat_target final : public linux_nat_target
{
public:
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static tilegx_linux_nat_target the_tilegx_linux_nat_target;

/* The register sets used in GNU/Linux ELF core-dumps are identical to
   the register sets in `struct user' that is used for a.out
   core-dumps, and is also used by `ptrace'.  The corresponding types
   are `elf_gregset_t' for the general-purpose registers (with
   `elf_greg_t' the type of a single GP register) and `elf_fpregset_t'
   for the floating-point registers.

   Those types used to be available under the names `gregset_t' and
   `fpregset_t' too, and this file used those names in the past.  But
   those names are now used for the register sets used in the
   `mcontext_t' type, and have a different size and layout.  */

/* Mapping between the general-purpose registers in `struct user'
   format and GDB's register array layout.  Note that we map the
   first 56 registers (0 thru 55) one-to-one.  GDB maps the pc to
   slot 64, but ptrace returns it in slot 56.  */
static const int regmap[] =
{
   0,  1,  2,  3,  4,  5,  6,  7,
   8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39,
  40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55,
  -1, -1, -1, -1, -1, -1, -1, -1,
  56, 58
};

/* Transfering the general-purpose registers between GDB, inferiors
   and core files.  */

/* Fill GDB's register array with the general-purpose register values
   in *GREGSETP.  */

void
supply_gregset (struct regcache* regcache,
		const elf_gregset_t *gregsetp)
{
  elf_greg_t *regp = (elf_greg_t *) gregsetp;
  int i;

  for (i = 0; i < sizeof (regmap) / sizeof (regmap[0]); i++)
    if (regmap[i] >= 0)
      regcache->raw_supply (i, regp + regmap[i]);
}

/* Fill registers in *GREGSETPS with the values in GDB's
   register array.  */

void
fill_gregset (const struct regcache* regcache,
	      elf_gregset_t *gregsetp, int regno)
{
  elf_greg_t *regp = (elf_greg_t *) gregsetp;
  int i;

  for (i = 0; i < sizeof (regmap) / sizeof (regmap[0]); i++)
    if (regmap[i] >= 0)
      regcache->raw_collect (i, regp + regmap[i]);
}

/* Transfering floating-point registers between GDB, inferiors and cores.  */

/* Fill GDB's register array with the floating-point register values in
   *FPREGSETP.  */

void
supply_fpregset (struct regcache *regcache,
		 const elf_fpregset_t *fpregsetp)
{
  /* NOTE: There are no floating-point registers for TILE-Gx.  */
}

/* Fill register REGNO (if it is a floating-point register) in
   *FPREGSETP with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_fpregset (const struct regcache *regcache,
	       elf_fpregset_t *fpregsetp, int regno)
{
  /* NOTE: There are no floating-point registers for TILE-Gx.  */
}

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

void
tilegx_linux_nat_target::fetch_registers (struct regcache *regcache,
					  int regnum)
{
  elf_gregset_t regs;
  pid_t tid = get_ptrace_pid (regcache->ptid ());

  if (ptrace (PTRACE_GETREGS, tid, 0, (PTRACE_TYPE_ARG3) &regs) < 0)
    perror_with_name (_("Couldn't get registers"));

  supply_gregset (regcache, (const elf_gregset_t *)&regs);
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

void
tilegx_linux_nat_target::store_registers (struct regcache *regcache,
					  int regnum)
{
  elf_gregset_t regs;
  pid_t tid = get_ptrace_pid (regcache->ptid ());

  if (ptrace (PTRACE_GETREGS, tid, 0, (PTRACE_TYPE_ARG3) &regs) < 0)
    perror_with_name (_("Couldn't get registers"));

  fill_gregset (regcache, &regs, regnum);

  if (ptrace (PTRACE_SETREGS, tid, 0, (PTRACE_TYPE_ARG3) &regs) < 0)
    perror_with_name (_("Couldn't write registers"));
}

void _initialize_tile_linux_nat ();
void
_initialize_tile_linux_nat ()
{
  linux_target = &the_tilegx_linux_nat_target;
  add_inf_child_target (&the_tilegx_linux_nat_target);
}
