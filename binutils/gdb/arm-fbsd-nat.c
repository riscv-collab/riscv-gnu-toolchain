/* Native-dependent code for FreeBSD/arm.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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
#include "target.h"

#include "elf/common.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "fbsd-nat.h"
#include "arm-tdep.h"
#include "arm-fbsd-tdep.h"
#include "inf-ptrace.h"

struct arm_fbsd_nat_target : public fbsd_nat_target
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
  const struct target_desc *read_description () override;
};

static arm_fbsd_nat_target the_arm_fbsd_nat_target;

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

void
arm_fbsd_nat_target::fetch_registers (struct regcache *regcache, int regnum)
{
  fetch_register_set<struct reg> (regcache, regnum, PT_GETREGS,
				  &arm_fbsd_gregset);
#ifdef PT_GETVFPREGS
  fetch_register_set<struct vfpreg> (regcache, regnum, PT_GETVFPREGS,
				     &arm_fbsd_vfpregset);
#endif
#ifdef PT_GETREGSET
  gdbarch *gdbarch = regcache->arch ();
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (tdep->tls_regnum > 0)
    fetch_regset<uint32_t> (regcache, regnum, NT_ARM_TLS, &arm_fbsd_tls_regset,
			    tdep->tls_regnum);
#endif
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

void
arm_fbsd_nat_target::store_registers (struct regcache *regcache, int regnum)
{
  store_register_set<struct reg> (regcache, regnum, PT_GETREGS, PT_SETREGS,
				  &arm_fbsd_gregset);
#ifdef PT_GETVFPREGS
  store_register_set<struct vfpreg> (regcache, regnum, PT_GETVFPREGS,
				     PT_SETVFPREGS, &arm_fbsd_vfpregset);
#endif
#ifdef PT_GETREGSET
  gdbarch *gdbarch = regcache->arch ();
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (tdep->tls_regnum > 0)
    store_regset<uint32_t> (regcache, regnum, NT_ARM_TLS, &arm_fbsd_tls_regset,
			    tdep->tls_regnum);
#endif
}

/* Implement the to_read_description method.  */

const struct target_desc *
arm_fbsd_nat_target::read_description ()
{
  const struct target_desc *desc;
  bool tls = false;

  if (inferior_ptid == null_ptid)
    return this->beneath ()->read_description ();

#ifdef PT_GETREGSET
  tls = have_regset (inferior_ptid, NT_ARM_TLS) != 0;
#endif
  desc = arm_fbsd_read_description_auxv (tls);
  if (desc == NULL)
    desc = this->beneath ()->read_description ();
  return desc;
}

void _initialize_arm_fbsd_nat ();
void
_initialize_arm_fbsd_nat ()
{
  add_inf_child_target (&the_arm_fbsd_nat_target);
}
