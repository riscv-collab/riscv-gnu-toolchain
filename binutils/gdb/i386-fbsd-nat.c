/* Native-dependent code for FreeBSD/i386.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/sysctl.h>
#include <sys/user.h>

#include "i386-tdep.h"
#include "i386-fbsd-tdep.h"
#include "i387-tdep.h"
#include "x86-nat.h"
#include "x86-fbsd-nat.h"

class i386_fbsd_nat_target final : public x86_fbsd_nat_target
{
public:
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  const struct target_desc *read_description () override;

  void resume (ptid_t, int, enum gdb_signal) override;
};

static i386_fbsd_nat_target the_i386_fbsd_nat_target;

static int have_ptrace_xmmregs;

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

void
i386_fbsd_nat_target::fetch_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  pid_t pid = get_ptrace_pid (regcache->ptid ());

  if (fetch_register_set<struct reg> (regcache, regnum, PT_GETREGS,
				      &i386_fbsd_gregset))
    {
      if (regnum != -1)
	return;
    }

#ifdef PT_GETFSBASE
  if (regnum == -1 || regnum == I386_FSBASE_REGNUM)
    {
      register_t base;

      if (ptrace (PT_GETFSBASE, pid, (PTRACE_TYPE_ARG3) &base, 0) == -1)
	perror_with_name (_("Couldn't get segment register fs_base"));

      regcache->raw_supply (I386_FSBASE_REGNUM, &base);
      if (regnum != -1)
	return;
    }
#endif
#ifdef PT_GETGSBASE
  if (regnum == -1 || regnum == I386_GSBASE_REGNUM)
    {
      register_t base;

      if (ptrace (PT_GETGSBASE, pid, (PTRACE_TYPE_ARG3) &base, 0) == -1)
	perror_with_name (_("Couldn't get segment register gs_base"));

      regcache->raw_supply (I386_GSBASE_REGNUM, &base);
      if (regnum != -1)
	return;
    }
#endif

  /* There is no i386_fxsave_supplies or i386_xsave_supplies.
     Instead, the earlier register sets return early if the request
     was for a specific register that was already satisified to avoid
     fetching the FPU/XSAVE state unnecessarily.  */

#ifdef PT_GETXSTATE_INFO
  if (m_xsave_info.xsave_len != 0)
    {
      void *xstateregs = alloca (m_xsave_info.xsave_len);

      if (ptrace (PT_GETXSTATE, pid, (PTRACE_TYPE_ARG3) xstateregs, 0) == -1)
	perror_with_name (_("Couldn't get extended state status"));

      i387_supply_xsave (regcache, regnum, xstateregs);
      return;
    }
#endif
  if (have_ptrace_xmmregs != 0)
    {
      char xmmregs[I387_SIZEOF_FXSAVE];

      if (ptrace(PT_GETXMMREGS, pid, (PTRACE_TYPE_ARG3) xmmregs, 0) == -1)
	perror_with_name (_("Couldn't get XMM registers"));

      i387_supply_fxsave (regcache, regnum, xmmregs);
      return;
    }

  struct fpreg fpregs;

  if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
    perror_with_name (_("Couldn't get floating point status"));

  i387_supply_fsave (regcache, regnum, &fpregs);
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

void
i386_fbsd_nat_target::store_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  pid_t pid = get_ptrace_pid (regcache->ptid ());

  if (store_register_set<struct reg> (regcache, regnum, PT_GETREGS, PT_SETREGS,
				      &i386_fbsd_gregset))
    {
      if (regnum != -1)
	return;
    }

#ifdef PT_SETFSBASE
  if (regnum == -1 || regnum == I386_FSBASE_REGNUM)
    {
      register_t base;

      regcache->raw_collect (I386_FSBASE_REGNUM, &base);

      if (ptrace (PT_SETFSBASE, pid, (PTRACE_TYPE_ARG3) &base, 0) == -1)
	perror_with_name (_("Couldn't write segment register fs_base"));
      if (regnum != -1)
	return;
    }
#endif
#ifdef PT_SETGSBASE
  if (regnum == -1 || regnum == I386_GSBASE_REGNUM)
    {
      register_t base;

      regcache->raw_collect (I386_GSBASE_REGNUM, &base);

      if (ptrace (PT_SETGSBASE, pid, (PTRACE_TYPE_ARG3) &base, 0) == -1)
	perror_with_name (_("Couldn't write segment register gs_base"));
      if (regnum != -1)
	return;
    }
#endif

  /* There is no i386_fxsave_supplies or i386_xsave_supplies.
     Instead, the earlier register sets return early if the request
     was for a specific register that was already satisified to avoid
     fetching the FPU/XSAVE state unnecessarily.  */

#ifdef PT_GETXSTATE_INFO
  if (m_xsave_info.xsave_len != 0)
    {
      void *xstateregs = alloca (m_xsave_info.xsave_len);

      if (ptrace (PT_GETXSTATE, pid, (PTRACE_TYPE_ARG3) xstateregs, 0) == -1)
	perror_with_name (_("Couldn't get extended state status"));

      i387_collect_xsave (regcache, regnum, xstateregs, 0);

      if (ptrace (PT_SETXSTATE, pid, (PTRACE_TYPE_ARG3) xstateregs,
		  m_xsave_info.xsave_len) == -1)
	perror_with_name (_("Couldn't write extended state status"));
      return;
    }
#endif
  if (have_ptrace_xmmregs != 0)
    {
      char xmmregs[I387_SIZEOF_FXSAVE];

      if (ptrace(PT_GETXMMREGS, pid, (PTRACE_TYPE_ARG3) xmmregs, 0) == -1)
	perror_with_name (_("Couldn't get XMM registers"));

      i387_collect_fxsave (regcache, regnum, xmmregs);

      if (ptrace (PT_SETXMMREGS, pid, (PTRACE_TYPE_ARG3) xmmregs, 0) == -1)
	perror_with_name (_("Couldn't write XMM registers"));
      return;
    }

  struct fpreg fpregs;

  if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
    perror_with_name (_("Couldn't get floating point status"));

  i387_collect_fsave (regcache, regnum, &fpregs);

  if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
    perror_with_name (_("Couldn't write floating point status"));
}

/* Resume execution of the inferior process.  If STEP is nonzero,
   single-step it.  If SIGNAL is nonzero, give it that signal.  */

void
i386_fbsd_nat_target::resume (ptid_t ptid, int step, enum gdb_signal signal)
{
  pid_t pid = ptid.pid ();
  int request = PT_STEP;

  if (pid == -1)
    /* Resume all threads.  This only gets used in the non-threaded
       case, where "resume all threads" and "resume inferior_ptid" are
       the same.  */
    pid = inferior_ptid.pid ();

  if (!step)
    {
      regcache *regcache = get_thread_regcache (inferior_thread ());
      ULONGEST eflags;

      /* Workaround for a bug in FreeBSD.  Make sure that the trace
	 flag is off when doing a continue.  There is a code path
	 through the kernel which leaves the flag set when it should
	 have been cleared.  If a process has a signal pending (such
	 as SIGALRM) and we do a PT_STEP, the process never really has
	 a chance to run because the kernel needs to notify the
	 debugger that a signal is being sent.  Therefore, the process
	 never goes through the kernel's trap() function which would
	 normally clear it.  */

      regcache_cooked_read_unsigned (regcache, I386_EFLAGS_REGNUM,
				     &eflags);
      if (eflags & 0x0100)
	regcache_cooked_write_unsigned (regcache, I386_EFLAGS_REGNUM,
					eflags & ~0x0100);

      request = PT_CONTINUE;
    }

  /* An addres of (caddr_t) 1 tells ptrace to continue from where it
     was.  (If GDB wanted it to start some other way, we have already
     written a new PC value to the child.)  */
  if (ptrace (request, pid, (caddr_t) 1,
	      gdb_signal_to_host (signal)) == -1)
    perror_with_name (("ptrace"));
}


/* Support for debugging kernel virtual memory images.  */

#include <machine/pcb.h>

#include "bsd-kvm.h"

static int
i386fbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  /* The following is true for FreeBSD 4.7:

     The pcb contains %eip, %ebx, %esp, %ebp, %esi, %edi and %gs.
     This accounts for all callee-saved registers specified by the
     psABI and then some.  Here %esp contains the stack pointer at the
     point just after the call to cpu_switch().  From this information
     we reconstruct the register state as it would look when we just
     returned from cpu_switch().  */

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_esp == 0)
    return 0;

  pcb->pcb_esp += 4;
  regcache->raw_supply (I386_EDI_REGNUM, &pcb->pcb_edi);
  regcache->raw_supply (I386_ESI_REGNUM, &pcb->pcb_esi);
  regcache->raw_supply (I386_EBP_REGNUM, &pcb->pcb_ebp);
  regcache->raw_supply (I386_ESP_REGNUM, &pcb->pcb_esp);
  regcache->raw_supply (I386_EBX_REGNUM, &pcb->pcb_ebx);
  regcache->raw_supply (I386_EIP_REGNUM, &pcb->pcb_eip);
  regcache->raw_supply (I386_GS_REGNUM, &pcb->pcb_gs);

  return 1;
}


/* Implement the read_description method.  */

const struct target_desc *
i386_fbsd_nat_target::read_description ()
{
  static int xmm_probed;

  if (inferior_ptid == null_ptid)
    return this->beneath ()->read_description ();

#ifdef PT_GETXSTATE_INFO
  probe_xsave_layout (inferior_ptid.pid ());
  if (m_xsave_info.xsave_len != 0)
    return i386_target_description (m_xsave_info.xsave_mask, true);
#endif

  if (!xmm_probed)
    {
      char xmmregs[I387_SIZEOF_FXSAVE];

      if (ptrace (PT_GETXMMREGS, inferior_ptid.pid (),
		  (PTRACE_TYPE_ARG3) xmmregs, 0) == 0)
	have_ptrace_xmmregs = 1;
      xmm_probed = 1;
    }

  if (have_ptrace_xmmregs)
    return i386_target_description (X86_XSTATE_SSE_MASK, true);

  return i386_target_description (X86_XSTATE_X87_MASK, true);
}

void _initialize_i386fbsd_nat ();
void
_initialize_i386fbsd_nat ()
{
  add_inf_child_target (&the_i386_fbsd_nat_target);

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (i386fbsd_supply_pcb);
}
