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
#include "gdbsupport/x86-xstate.h"
#include "arch/i386.h"
#include "x86-tdesc.h"
#include "tdesc.h"

/* The index of various registers inside the regcache.  */

enum netbsd_i386_gdb_regnum
{
  I386_EAX_REGNUM,              /* %eax */
  I386_ECX_REGNUM,              /* %ecx */
  I386_EDX_REGNUM,              /* %edx */
  I386_EBX_REGNUM,              /* %ebx */
  I386_ESP_REGNUM,              /* %esp */
  I386_EBP_REGNUM,              /* %ebp */
  I386_ESI_REGNUM,              /* %esi */
  I386_EDI_REGNUM,              /* %edi */
  I386_EIP_REGNUM,              /* %eip */
  I386_EFLAGS_REGNUM,           /* %eflags */
  I386_CS_REGNUM,               /* %cs */
  I386_SS_REGNUM,               /* %ss */
  I386_DS_REGNUM,               /* %ds */
  I386_ES_REGNUM,               /* %es */
  I386_FS_REGNUM,               /* %fs */
  I386_GS_REGNUM,               /* %gs */
  I386_ST0_REGNUM               /* %st(0) */
};

/* The fill_function for the general-purpose register set.  */

static void
netbsd_i386_fill_gregset (struct regcache *regcache, char *buf)
{
  struct reg *r = (struct reg *) buf;

#define netbsd_i386_collect_gp(regnum, fld) do {		\
    collect_register (regcache, regnum, &r->r_##fld);		\
  } while (0)

  netbsd_i386_collect_gp (I386_EAX_REGNUM, eax);
  netbsd_i386_collect_gp (I386_EBX_REGNUM, ebx);
  netbsd_i386_collect_gp (I386_ECX_REGNUM, ecx);
  netbsd_i386_collect_gp (I386_EDX_REGNUM, edx);
  netbsd_i386_collect_gp (I386_ESP_REGNUM, esp);
  netbsd_i386_collect_gp (I386_EBP_REGNUM, ebp);
  netbsd_i386_collect_gp (I386_ESI_REGNUM, esi);
  netbsd_i386_collect_gp (I386_EDI_REGNUM, edi);
  netbsd_i386_collect_gp (I386_EIP_REGNUM, eip);
  netbsd_i386_collect_gp (I386_EFLAGS_REGNUM, eflags);
  netbsd_i386_collect_gp (I386_CS_REGNUM, cs);
  netbsd_i386_collect_gp (I386_SS_REGNUM, ss);
  netbsd_i386_collect_gp (I386_DS_REGNUM, ds);
  netbsd_i386_collect_gp (I386_ES_REGNUM, es);
  netbsd_i386_collect_gp (I386_FS_REGNUM, fs);
  netbsd_i386_collect_gp (I386_GS_REGNUM, gs);
}

/* The store_function for the general-purpose register set.  */

static void
netbsd_i386_store_gregset (struct regcache *regcache, const char *buf)
{
  struct reg *r = (struct reg *) buf;

#define netbsd_i386_supply_gp(regnum, fld) do {		\
    supply_register (regcache, regnum, &r->r_##fld);	\
  } while(0)

  netbsd_i386_supply_gp (I386_EAX_REGNUM, eax);
  netbsd_i386_supply_gp (I386_EBX_REGNUM, ebx);
  netbsd_i386_supply_gp (I386_ECX_REGNUM, ecx);
  netbsd_i386_supply_gp (I386_EDX_REGNUM, edx);
  netbsd_i386_supply_gp (I386_ESP_REGNUM, esp);
  netbsd_i386_supply_gp (I386_EBP_REGNUM, ebp);
  netbsd_i386_supply_gp (I386_ESI_REGNUM, esi);
  netbsd_i386_supply_gp (I386_EDI_REGNUM, edi);
  netbsd_i386_supply_gp (I386_EIP_REGNUM, eip);
  netbsd_i386_supply_gp (I386_EFLAGS_REGNUM, eflags);
  netbsd_i386_supply_gp (I386_CS_REGNUM, cs);
  netbsd_i386_supply_gp (I386_SS_REGNUM, ss);
  netbsd_i386_supply_gp (I386_DS_REGNUM, ds);
  netbsd_i386_supply_gp (I386_ES_REGNUM, es);
  netbsd_i386_supply_gp (I386_FS_REGNUM, fs);
  netbsd_i386_supply_gp (I386_GS_REGNUM, gs);
}

/* Description of all the x86-netbsd register sets.  */

static const struct netbsd_regset_info netbsd_target_regsets[] =
{
  /* General Purpose Registers.  */
  {PT_GETREGS, PT_SETREGS, sizeof (struct reg),
  netbsd_i386_fill_gregset, netbsd_i386_store_gregset},
  /* End of list marker.  */
  {0, 0, -1, NULL, NULL }
};

/* NetBSD target op definitions for the amd64 architecture.  */

class netbsd_i386_target : public netbsd_process_target
{
public:
  const netbsd_regset_info *get_regs_info () override;

  void low_arch_setup () override;
};

const netbsd_regset_info *
netbsd_i386_target::get_regs_info ()
{
  return netbsd_target_regsets;
}

/* Initialize the target description for the architecture of the
   inferior.  */

void
netbsd_i386_target::low_arch_setup ()
{
  target_desc *tdesc
    = i386_create_target_description (X86_XSTATE_SSE_MASK, false, false);

  init_target_desc (tdesc, i386_expedite_regs);

  current_process ()->tdesc = tdesc;
}

/* The singleton target ops object.  */

static netbsd_i386_target the_netbsd_i386_target;

/* The NetBSD target ops object.  */

netbsd_process_target *the_netbsd_target = &the_netbsd_i386_target;
