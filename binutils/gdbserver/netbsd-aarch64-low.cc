/* Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <sys/ptrace.h>
#include <limits.h>

#include "server.h"
#include "netbsd-low.h"
#include "arch/aarch64.h"
#include "arch/aarch64-insn.h"
#include "tdesc.h"

/* The fill_function for the general-purpose register set.  */

static void
netbsd_aarch64_fill_gregset (struct regcache *regcache, char *buf)
{
  struct reg *r = (struct reg *) buf;

#define netbsd_aarch64_collect_gp(regnum, fld) do {		\
    collect_register (regcache, regnum, &r->fld);		\
  } while (0)

  for (size_t i = 0; i < ARRAY_SIZE (r->r_reg); i++)
    netbsd_aarch64_collect_gp (AARCH64_X0_REGNUM + i, r_reg[i]);

  netbsd_aarch64_collect_gp (AARCH64_SP_REGNUM, r_sp);
  netbsd_aarch64_collect_gp (AARCH64_PC_REGNUM, r_pc);
}

/* The store_function for the general-purpose register set.  */

static void
netbsd_aarch64_store_gregset (struct regcache *regcache, const char *buf)
{
  struct reg *r = (struct reg *) buf;

#define netbsd_aarch64_supply_gp(regnum, fld) do {		\
    supply_register (regcache, regnum, &r->fld);		\
  } while(0)

  for (size_t i = 0; i < ARRAY_SIZE (r->r_reg); i++)
    netbsd_aarch64_supply_gp (AARCH64_X0_REGNUM + i, r_reg[i]);

  netbsd_aarch64_supply_gp (AARCH64_SP_REGNUM, r_sp);
  netbsd_aarch64_supply_gp (AARCH64_PC_REGNUM, r_pc);
}

/* Description of all the aarch64-netbsd register sets.  */

static const struct netbsd_regset_info netbsd_target_regsets[] =
{
  /* General Purpose Registers.  */
  {PT_GETREGS, PT_SETREGS, sizeof (struct reg),
  netbsd_aarch64_fill_gregset, netbsd_aarch64_store_gregset},
  /* End of list marker.  */
  {0, 0, -1, NULL, NULL }
};

/* NetBSD target op definitions for the aarch64 architecture.  */

class netbsd_aarch64_target : public netbsd_process_target
{
protected:
  const netbsd_regset_info *get_regs_info () override;

  void low_arch_setup () override;
};

/* Return the information to access registers.  */

const netbsd_regset_info *
netbsd_aarch64_target::get_regs_info ()
{
  return netbsd_target_regsets;
}

/* Architecture-specific setup for the current process.  */

void
netbsd_aarch64_target::low_arch_setup ()
{
  target_desc *tdesc
    = aarch64_create_target_description ({});

  static const char *expedite_regs_aarch64[] = { "x29", "sp", "pc", NULL };
  init_target_desc (tdesc, expedite_regs_aarch64);

  current_process ()->tdesc = tdesc;
}

/* The singleton target ops object.  */

static netbsd_aarch64_target the_netbsd_aarch64_target;

/* The NetBSD target ops object.  */

netbsd_process_target *the_netbsd_target = &the_netbsd_aarch64_target;
