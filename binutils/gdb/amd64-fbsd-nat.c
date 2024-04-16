/* Native-dependent code for FreeBSD/amd64.

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
#include "inferior.h"
#include "regcache.h"
#include "target.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <machine/reg.h>

#include "amd64-tdep.h"
#include "amd64-fbsd-tdep.h"
#include "i387-tdep.h"
#include "amd64-nat.h"
#include "x86-nat.h"
#include "x86-fbsd-nat.h"

class amd64_fbsd_nat_target final : public x86_fbsd_nat_target
{
public:
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  const struct target_desc *read_description () override;
};

static amd64_fbsd_nat_target the_amd64_fbsd_nat_target;

/* This is a layout of the amd64 'struct reg' but with i386
   registers.  */

static const struct regcache_map_entry amd64_fbsd32_gregmap[] =
{
  { 8, REGCACHE_MAP_SKIP, 8 },
  { 1, I386_EDI_REGNUM, 8 },
  { 1, I386_ESI_REGNUM, 8 },
  { 1, I386_EBP_REGNUM, 8 },
  { 1, I386_EBX_REGNUM, 8 },
  { 1, I386_EDX_REGNUM, 8 },
  { 1, I386_ECX_REGNUM, 8 },
  { 1, I386_EAX_REGNUM, 8 },
  { 1, REGCACHE_MAP_SKIP, 4 },	/* trapno */
  { 1, I386_FS_REGNUM, 2 },
  { 1, I386_GS_REGNUM, 2 },
  { 1, REGCACHE_MAP_SKIP, 4 },	/* err */
  { 1, I386_ES_REGNUM, 2 },
  { 1, I386_DS_REGNUM, 2 },
  { 1, I386_EIP_REGNUM, 8 },
  { 1, I386_CS_REGNUM, 8 },
  { 1, I386_EFLAGS_REGNUM, 8 },
  { 1, I386_ESP_REGNUM, 0 },
  { 1, I386_SS_REGNUM, 8 },
  { 0 }
};

static const struct regset amd64_fbsd32_gregset =
{
  amd64_fbsd32_gregmap, regcache_supply_regset, regcache_collect_regset
};

/* Return the regset to use for 'struct reg' for the GDBARCH.  */

static const struct regset *
find_gregset (struct gdbarch *gdbarch)
{
  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    return &amd64_fbsd32_gregset;
  else
    return &amd64_fbsd_gregset;
}

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

void
amd64_fbsd_nat_target::fetch_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
#if defined(PT_GETFSBASE) || defined(PT_GETGSBASE)
  const i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
#endif
  pid_t pid = get_ptrace_pid (regcache->ptid ());
  const struct regset *gregset = find_gregset (gdbarch);

  if (fetch_register_set<struct reg> (regcache, regnum, PT_GETREGS, gregset))
    {
      if (regnum != -1)
	return;
    }

#ifdef PT_GETFSBASE
  if (regnum == -1 || regnum == tdep->fsbase_regnum)
    {
      register_t base;

      if (ptrace (PT_GETFSBASE, pid, (PTRACE_TYPE_ARG3) &base, 0) == -1)
	perror_with_name (_("Couldn't get segment register fs_base"));

      regcache->raw_supply (tdep->fsbase_regnum, &base);
      if (regnum != -1)
	return;
    }
#endif
#ifdef PT_GETGSBASE
  if (regnum == -1 || regnum == tdep->fsbase_regnum + 1)
    {
      register_t base;

      if (ptrace (PT_GETGSBASE, pid, (PTRACE_TYPE_ARG3) &base, 0) == -1)
	perror_with_name (_("Couldn't get segment register gs_base"));

      regcache->raw_supply (tdep->fsbase_regnum + 1, &base);
      if (regnum != -1)
	return;
    }
#endif

  /* There is no amd64_fxsave_supplies or amd64_xsave_supplies.
     Instead, the earlier register sets return early if the request
     was for a specific register that was already satisified to avoid
     fetching the FPU/XSAVE state unnecessarily.  */

#ifdef PT_GETXSTATE_INFO
  if (m_xsave_info.xsave_len != 0)
    {
      void *xstateregs = alloca (m_xsave_info.xsave_len);

      if (ptrace (PT_GETXSTATE, pid, (PTRACE_TYPE_ARG3) xstateregs, 0) == -1)
	perror_with_name (_("Couldn't get extended state status"));

      amd64_supply_xsave (regcache, regnum, xstateregs);
      return;
    }
#endif

  struct fpreg fpregs;

  if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
    perror_with_name (_("Couldn't get floating point status"));

  amd64_supply_fxsave (regcache, regnum, &fpregs);
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

void
amd64_fbsd_nat_target::store_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
#if defined(PT_GETFSBASE) || defined(PT_GETGSBASE)
  const i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
#endif
  pid_t pid = get_ptrace_pid (regcache->ptid ());
  const struct regset *gregset = find_gregset (gdbarch);

  if (store_register_set<struct reg> (regcache, regnum, PT_GETREGS, PT_SETREGS,
				      gregset))
    {
      if (regnum != -1)
	return;
    }

#ifdef PT_SETFSBASE
  if (regnum == -1 || regnum == tdep->fsbase_regnum)
    {
      register_t base;

      /* Clear the full base value to support 32-bit targets.  */
      base = 0;
      regcache->raw_collect (tdep->fsbase_regnum, &base);

      if (ptrace (PT_SETFSBASE, pid, (PTRACE_TYPE_ARG3) &base, 0) == -1)
	perror_with_name (_("Couldn't write segment register fs_base"));
      if (regnum != -1)
	return;
    }
#endif
#ifdef PT_SETGSBASE
  if (regnum == -1 || regnum == tdep->fsbase_regnum + 1)
    {
      register_t base;

      /* Clear the full base value to support 32-bit targets.  */
      base = 0;
      regcache->raw_collect (tdep->fsbase_regnum + 1, &base);

      if (ptrace (PT_SETGSBASE, pid, (PTRACE_TYPE_ARG3) &base, 0) == -1)
	perror_with_name (_("Couldn't write segment register gs_base"));
      if (regnum != -1)
	return;
    }
#endif

  /* There is no amd64_fxsave_supplies or amd64_xsave_supplies.
     Instead, the earlier register sets return early if the request
     was for a specific register that was already satisified to avoid
     fetching the FPU/XSAVE state unnecessarily.  */

#ifdef PT_GETXSTATE_INFO
  if (m_xsave_info.xsave_len != 0)
    {
      void *xstateregs = alloca (m_xsave_info.xsave_len);

      if (ptrace (PT_GETXSTATE, pid, (PTRACE_TYPE_ARG3) xstateregs, 0) == -1)
	perror_with_name (_("Couldn't get extended state status"));

      amd64_collect_xsave (regcache, regnum, xstateregs, 0);

      if (ptrace (PT_SETXSTATE, pid, (PTRACE_TYPE_ARG3) xstateregs,
		  m_xsave_info.xsave_len) == -1)
	perror_with_name (_("Couldn't write extended state status"));
      return;
    }
#endif

  struct fpreg fpregs;

  if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
    perror_with_name (_("Couldn't get floating point status"));

  amd64_collect_fxsave (regcache, regnum, &fpregs);

  if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
    perror_with_name (_("Couldn't write floating point status"));
}

/* Support for debugging kernel virtual memory images.  */

#include <machine/pcb.h>
#include <osreldate.h>

#include "bsd-kvm.h"

static int
amd64fbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  /* The following is true for FreeBSD 5.2:

     The pcb contains %rip, %rbx, %rsp, %rbp, %r12, %r13, %r14, %r15,
     %ds, %es, %fs and %gs.  This accounts for all callee-saved
     registers specified by the psABI and then some.  Here %esp
     contains the stack pointer at the point just after the call to
     cpu_switch().  From this information we reconstruct the register
     state as it would like when we just returned from cpu_switch().  */

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_rsp == 0)
    return 0;

  pcb->pcb_rsp += 8;
  regcache->raw_supply (AMD64_RIP_REGNUM, &pcb->pcb_rip);
  regcache->raw_supply (AMD64_RBX_REGNUM, &pcb->pcb_rbx);
  regcache->raw_supply (AMD64_RSP_REGNUM, &pcb->pcb_rsp);
  regcache->raw_supply (AMD64_RBP_REGNUM, &pcb->pcb_rbp);
  regcache->raw_supply (12, &pcb->pcb_r12);
  regcache->raw_supply (13, &pcb->pcb_r13);
  regcache->raw_supply (14, &pcb->pcb_r14);
  regcache->raw_supply (15, &pcb->pcb_r15);
#if (__FreeBSD_version < 800075) && (__FreeBSD_kernel_version < 800075)
  /* struct pcb provides the pcb_ds/pcb_es/pcb_fs/pcb_gs fields only
     up until __FreeBSD_version 800074: The removal of these fields
     occurred on 2009-04-01 while the __FreeBSD_version number was
     bumped to 800075 on 2009-04-06.  So 800075 is the closest version
     number where we should not try to access these fields.  */
  regcache->raw_supply (AMD64_DS_REGNUM, &pcb->pcb_ds);
  regcache->raw_supply (AMD64_ES_REGNUM, &pcb->pcb_es);
  regcache->raw_supply (AMD64_FS_REGNUM, &pcb->pcb_fs);
  regcache->raw_supply (AMD64_GS_REGNUM, &pcb->pcb_gs);
#endif

  return 1;
}


/* Implement the read_description method.  */

const struct target_desc *
amd64_fbsd_nat_target::read_description ()
{
  struct reg regs;
  int is64;

  if (inferior_ptid == null_ptid)
    return this->beneath ()->read_description ();

  if (ptrace (PT_GETREGS, inferior_ptid.pid (),
	      (PTRACE_TYPE_ARG3) &regs, 0) == -1)
    perror_with_name (_("Couldn't get registers"));
  is64 = (regs.r_cs == GSEL (GUCODE_SEL, SEL_UPL));
#ifdef PT_GETXSTATE_INFO
  probe_xsave_layout (inferior_ptid.pid ());
  if (m_xsave_info.xsave_len != 0)
    {
      if (is64)
	return amd64_target_description (m_xsave_info.xsave_mask, true);
      else
	return i386_target_description (m_xsave_info.xsave_mask, true);
    }
#endif
  if (is64)
    return amd64_target_description (X86_XSTATE_SSE_MASK, true);
  else
    return i386_target_description (X86_XSTATE_SSE_MASK, true);
}

void _initialize_amd64fbsd_nat ();
void
_initialize_amd64fbsd_nat ()
{
  add_inf_child_target (&the_amd64_fbsd_nat_target);

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (amd64fbsd_supply_pcb);
}
