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
#include "arch/amd64.h"
#include "x86-tdesc.h"
#include "tdesc.h"

/* The index of various registers inside the regcache.  */

enum netbsd_x86_64_gdb_regnum
{
  AMD64_RAX_REGNUM,	     /* %rax */
  AMD64_RBX_REGNUM,	     /* %rbx */
  AMD64_RCX_REGNUM,	     /* %rcx */
  AMD64_RDX_REGNUM,	     /* %rdx */
  AMD64_RSI_REGNUM,	     /* %rsi */
  AMD64_RDI_REGNUM,	     /* %rdi */
  AMD64_RBP_REGNUM,	     /* %rbp */
  AMD64_RSP_REGNUM,	     /* %rsp */
  AMD64_R8_REGNUM,	      /* %r8 */
  AMD64_R9_REGNUM,	      /* %r9 */
  AMD64_R10_REGNUM,	     /* %r10 */
  AMD64_R11_REGNUM,	     /* %r11 */
  AMD64_R12_REGNUM,	     /* %r12 */
  AMD64_R13_REGNUM,	     /* %r13 */
  AMD64_R14_REGNUM,	     /* %r14 */
  AMD64_R15_REGNUM,	     /* %r15 */
  AMD64_RIP_REGNUM,	     /* %rip */
  AMD64_EFLAGS_REGNUM,	  /* %eflags */
  AMD64_CS_REGNUM,	      /* %cs */
  AMD64_SS_REGNUM,	      /* %ss */
  AMD64_DS_REGNUM,	      /* %ds */
  AMD64_ES_REGNUM,	      /* %es */
  AMD64_FS_REGNUM,	      /* %fs */
  AMD64_GS_REGNUM,	      /* %gs */
  AMD64_ST0_REGNUM = 24,     /* %st0 */
  AMD64_ST1_REGNUM,	     /* %st1 */
  AMD64_FCTRL_REGNUM = AMD64_ST0_REGNUM + 8,
  AMD64_FSTAT_REGNUM = AMD64_ST0_REGNUM + 9,
  AMD64_FTAG_REGNUM = AMD64_ST0_REGNUM + 10,
  AMD64_XMM0_REGNUM = 40,   /* %xmm0 */
  AMD64_XMM1_REGNUM,	    /* %xmm1 */
  AMD64_MXCSR_REGNUM = AMD64_XMM0_REGNUM + 16,
  AMD64_YMM0H_REGNUM,	   /* %ymm0h */
  AMD64_YMM15H_REGNUM = AMD64_YMM0H_REGNUM + 15,
  AMD64_BND0R_REGNUM = AMD64_YMM15H_REGNUM + 1,
  AMD64_BND3R_REGNUM = AMD64_BND0R_REGNUM + 3,
  AMD64_BNDCFGU_REGNUM,
  AMD64_BNDSTATUS_REGNUM,
  AMD64_XMM16_REGNUM,
  AMD64_XMM31_REGNUM = AMD64_XMM16_REGNUM + 15,
  AMD64_YMM16H_REGNUM,
  AMD64_YMM31H_REGNUM = AMD64_YMM16H_REGNUM + 15,
  AMD64_K0_REGNUM,
  AMD64_K7_REGNUM = AMD64_K0_REGNUM + 7,
  AMD64_ZMM0H_REGNUM,
  AMD64_ZMM31H_REGNUM = AMD64_ZMM0H_REGNUM + 31,
  AMD64_PKRU_REGNUM,
  AMD64_FSBASE_REGNUM,
  AMD64_GSBASE_REGNUM
};

/* The fill_function for the general-purpose register set.  */

static void
netbsd_x86_64_fill_gregset (struct regcache *regcache, char *buf)
{
  struct reg *r = (struct reg *) buf;

#define netbsd_x86_64_collect_gp(regnum, fld) do {		\
    collect_register (regcache, regnum, &r->regs[_REG_##fld]);	\
  } while (0)

  netbsd_x86_64_collect_gp (AMD64_RAX_REGNUM, RAX);
  netbsd_x86_64_collect_gp (AMD64_RBX_REGNUM, RBX);
  netbsd_x86_64_collect_gp (AMD64_RCX_REGNUM, RCX);
  netbsd_x86_64_collect_gp (AMD64_RDX_REGNUM, RDX);
  netbsd_x86_64_collect_gp (AMD64_RSI_REGNUM, RSI);
  netbsd_x86_64_collect_gp (AMD64_RDI_REGNUM, RDI);
  netbsd_x86_64_collect_gp (AMD64_RBP_REGNUM, RBP);
  netbsd_x86_64_collect_gp (AMD64_RSP_REGNUM, RSP);
  netbsd_x86_64_collect_gp (AMD64_R8_REGNUM, R8);
  netbsd_x86_64_collect_gp (AMD64_R9_REGNUM, R9);
  netbsd_x86_64_collect_gp (AMD64_R10_REGNUM, R10);
  netbsd_x86_64_collect_gp (AMD64_R11_REGNUM, R11);
  netbsd_x86_64_collect_gp (AMD64_R12_REGNUM, R12);
  netbsd_x86_64_collect_gp (AMD64_R13_REGNUM, R13);
  netbsd_x86_64_collect_gp (AMD64_R14_REGNUM, R14);
  netbsd_x86_64_collect_gp (AMD64_R15_REGNUM, R15);
  netbsd_x86_64_collect_gp (AMD64_RIP_REGNUM, RIP);
  netbsd_x86_64_collect_gp (AMD64_EFLAGS_REGNUM, RFLAGS);
  netbsd_x86_64_collect_gp (AMD64_CS_REGNUM, CS);
  netbsd_x86_64_collect_gp (AMD64_SS_REGNUM, SS);
  netbsd_x86_64_collect_gp (AMD64_DS_REGNUM, DS);
  netbsd_x86_64_collect_gp (AMD64_ES_REGNUM, ES);
  netbsd_x86_64_collect_gp (AMD64_FS_REGNUM, FS);
  netbsd_x86_64_collect_gp (AMD64_GS_REGNUM, GS);
}

/* The store_function for the general-purpose register set.  */

static void
netbsd_x86_64_store_gregset (struct regcache *regcache, const char *buf)
{
  struct reg *r = (struct reg *) buf;

#define netbsd_x86_64_supply_gp(regnum, fld) do {		\
    supply_register (regcache, regnum, &r->regs[_REG_##fld]);	\
  } while(0)

  netbsd_x86_64_supply_gp (AMD64_RAX_REGNUM, RAX);
  netbsd_x86_64_supply_gp (AMD64_RBX_REGNUM, RBX);
  netbsd_x86_64_supply_gp (AMD64_RCX_REGNUM, RCX);
  netbsd_x86_64_supply_gp (AMD64_RDX_REGNUM, RDX);
  netbsd_x86_64_supply_gp (AMD64_RSI_REGNUM, RSI);
  netbsd_x86_64_supply_gp (AMD64_RDI_REGNUM, RDI);
  netbsd_x86_64_supply_gp (AMD64_RBP_REGNUM, RBP);
  netbsd_x86_64_supply_gp (AMD64_RSP_REGNUM, RSP);
  netbsd_x86_64_supply_gp (AMD64_R8_REGNUM, R8);
  netbsd_x86_64_supply_gp (AMD64_R9_REGNUM, R9);
  netbsd_x86_64_supply_gp (AMD64_R10_REGNUM, R10);
  netbsd_x86_64_supply_gp (AMD64_R11_REGNUM, R11);
  netbsd_x86_64_supply_gp (AMD64_R12_REGNUM, R12);
  netbsd_x86_64_supply_gp (AMD64_R13_REGNUM, R13);
  netbsd_x86_64_supply_gp (AMD64_R14_REGNUM, R14);
  netbsd_x86_64_supply_gp (AMD64_R15_REGNUM, R15);
  netbsd_x86_64_supply_gp (AMD64_RIP_REGNUM, RIP);
  netbsd_x86_64_supply_gp (AMD64_EFLAGS_REGNUM, RFLAGS);
  netbsd_x86_64_supply_gp (AMD64_CS_REGNUM, CS);
  netbsd_x86_64_supply_gp (AMD64_SS_REGNUM, SS);
  netbsd_x86_64_supply_gp (AMD64_DS_REGNUM, DS);
  netbsd_x86_64_supply_gp (AMD64_ES_REGNUM, ES);
  netbsd_x86_64_supply_gp (AMD64_FS_REGNUM, FS);
  netbsd_x86_64_supply_gp (AMD64_GS_REGNUM, GS);
}

/* Description of all the x86-netbsd register sets.  */

static const struct netbsd_regset_info netbsd_target_regsets[] =
{
  /* General Purpose Registers.  */
  {PT_GETREGS, PT_SETREGS, sizeof (struct reg),
   netbsd_x86_64_fill_gregset, netbsd_x86_64_store_gregset},
  /* End of list marker.  */
  {0, 0, -1, NULL, NULL }
};

/* NetBSD target op definitions for the amd64 architecture.  */

class netbsd_amd64_target : public netbsd_process_target
{
protected:
  const netbsd_regset_info *get_regs_info () override;

  void low_arch_setup () override;
};

/* Return the information to access registers.  */

const netbsd_regset_info *
netbsd_amd64_target::get_regs_info ()
{
  return netbsd_target_regsets;
}

/* Architecture-specific setup for the current process.  */

void
netbsd_amd64_target::low_arch_setup ()
{
  target_desc *tdesc
    = amd64_create_target_description (X86_XSTATE_SSE_MASK, false, false, false);

  init_target_desc (tdesc, amd64_expedite_regs);

  current_process ()->tdesc = tdesc;
}

/* The singleton target ops object.  */

static netbsd_amd64_target the_netbsd_amd64_target;

/* The NetBSD target ops object.  */

netbsd_process_target *the_netbsd_target = &the_netbsd_amd64_target;
