/* Native-dependent code for FreeBSD/aarch64.

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
#include "arch-utils.h"
#include "inferior.h"
#include "regcache.h"
#include "target.h"
#include "nat/aarch64-hw-point.h"

#include "elf/common.h"

#include <sys/param.h>
#include <sys/ptrace.h>
#include <machine/armreg.h>
#include <machine/reg.h>

#include "fbsd-nat.h"
#include "aarch64-tdep.h"
#include "aarch64-fbsd-tdep.h"
#include "aarch64-nat.h"
#include "inf-ptrace.h"

#if __FreeBSD_version >= 1400005
#define	HAVE_DBREG

#include <unordered_set>
#endif

#ifdef HAVE_DBREG
struct aarch64_fbsd_nat_target final
  : public aarch64_nat_target<fbsd_nat_target>
#else
struct aarch64_fbsd_nat_target final : public fbsd_nat_target
#endif
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  const struct target_desc *read_description () override;

#ifdef HAVE_DBREG
  /* Hardware breakpoints and watchpoints.  */
  bool stopped_by_watchpoint () override;
  bool stopped_data_address (CORE_ADDR *) override;
  bool stopped_by_hw_breakpoint () override;
  bool supports_stopped_by_hw_breakpoint () override;

  void post_startup_inferior (ptid_t) override;
  void post_attach (int pid) override;

  void low_new_fork (ptid_t parent, pid_t child) override;
  void low_delete_thread (thread_info *) override;
  void low_prepare_to_resume (thread_info *) override;

private:
  void probe_debug_regs (int pid);
  static bool debug_regs_probed;
#endif
};

static aarch64_fbsd_nat_target the_aarch64_fbsd_nat_target;

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

void
aarch64_fbsd_nat_target::fetch_registers (struct regcache *regcache,
					  int regnum)
{
  fetch_register_set<struct reg> (regcache, regnum, PT_GETREGS,
				  &aarch64_fbsd_gregset);
  fetch_register_set<struct fpreg> (regcache, regnum, PT_GETFPREGS,
				    &aarch64_fbsd_fpregset);

  gdbarch *gdbarch = regcache->arch ();
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);
  if (tdep->has_tls ())
    fetch_regset<uint64_t> (regcache, regnum, NT_ARM_TLS,
			    &aarch64_fbsd_tls_regset, tdep->tls_regnum_base);
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

void
aarch64_fbsd_nat_target::store_registers (struct regcache *regcache,
					  int regnum)
{
  store_register_set<struct reg> (regcache, regnum, PT_GETREGS, PT_SETREGS,
				  &aarch64_fbsd_gregset);
  store_register_set<struct fpreg> (regcache, regnum, PT_GETFPREGS,
				    PT_SETFPREGS, &aarch64_fbsd_fpregset);

  gdbarch *gdbarch = regcache->arch ();
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);
  if (tdep->has_tls ())
    store_regset<uint64_t> (regcache, regnum, NT_ARM_TLS,
			    &aarch64_fbsd_tls_regset, tdep->tls_regnum_base);
}

/* Implement the target read_description method.  */

const struct target_desc *
aarch64_fbsd_nat_target::read_description ()
{
  if (inferior_ptid == null_ptid)
    return this->beneath ()->read_description ();

  aarch64_features features;
  features.tls = have_regset (inferior_ptid, NT_ARM_TLS)? 1 : 0;
  return aarch64_read_description (features);
}

#ifdef HAVE_DBREG
bool aarch64_fbsd_nat_target::debug_regs_probed;

/* Set of threads which need to update debug registers on next resume.  */

static std::unordered_set<lwpid_t> aarch64_debug_pending_threads;

/* Implement the "stopped_data_address" target_ops method.  */

bool
aarch64_fbsd_nat_target::stopped_data_address (CORE_ADDR *addr_p)
{
  siginfo_t siginfo;
  struct aarch64_debug_reg_state *state;

  if (!fbsd_nat_get_siginfo (inferior_ptid, &siginfo))
    return false;

  /* This must be a hardware breakpoint.  */
  if (siginfo.si_signo != SIGTRAP
      || siginfo.si_code != TRAP_TRACE
      || siginfo.si_trapno != EXCP_WATCHPT_EL0)
    return false;

  const CORE_ADDR addr_trap = (CORE_ADDR) siginfo.si_addr;

  /* Check if the address matches any watched address.  */
  state = aarch64_get_debug_reg_state (inferior_ptid.pid ());
  return aarch64_stopped_data_address (state, addr_trap, addr_p);
}

/* Implement the "stopped_by_watchpoint" target_ops method.  */

bool
aarch64_fbsd_nat_target::stopped_by_watchpoint ()
{
  CORE_ADDR addr;

  return stopped_data_address (&addr);
}

/* Implement the "stopped_by_hw_breakpoint" target_ops method.  */

bool
aarch64_fbsd_nat_target::stopped_by_hw_breakpoint ()
{
  siginfo_t siginfo;
  struct aarch64_debug_reg_state *state;

  if (!fbsd_nat_get_siginfo (inferior_ptid, &siginfo))
    return false;

  /* This must be a hardware breakpoint.  */
  if (siginfo.si_signo != SIGTRAP
      || siginfo.si_code != TRAP_TRACE
      || siginfo.si_trapno != EXCP_WATCHPT_EL0)
    return false;

  return !stopped_by_watchpoint();
}

/* Implement the "supports_stopped_by_hw_breakpoint" target_ops method.  */

bool
aarch64_fbsd_nat_target::supports_stopped_by_hw_breakpoint ()
{
  return true;
}

/* Fetch the hardware debug register capability information.  */

void
aarch64_fbsd_nat_target::probe_debug_regs (int pid)
{
  if (!debug_regs_probed)
    {
      struct dbreg reg;

      debug_regs_probed = true;
      aarch64_num_bp_regs = 0;
      aarch64_num_wp_regs = 0;

      if (ptrace(PT_GETDBREGS, pid, (PTRACE_TYPE_ARG3) &reg, 0) == 0)
	{
	  switch (reg.db_debug_ver)
	    {
	    case AARCH64_DEBUG_ARCH_V8:
	    case AARCH64_DEBUG_ARCH_V8_1:
	    case AARCH64_DEBUG_ARCH_V8_2:
	    case AARCH64_DEBUG_ARCH_V8_4:
	    case AARCH64_DEBUG_ARCH_V8_8:
	    case AARCH64_DEBUG_ARCH_V8_9:
	      break;
	    default:
	      return;
	    }

	  aarch64_num_bp_regs = reg.db_nbkpts;
	  if (aarch64_num_bp_regs > AARCH64_HBP_MAX_NUM)
	    {
	      warning (_("Unexpected number of hardware breakpoint registers"
			 " reported by ptrace, got %d, expected %d."),
		       aarch64_num_bp_regs, AARCH64_HBP_MAX_NUM);
	      aarch64_num_bp_regs = AARCH64_HBP_MAX_NUM;
	    }
	  aarch64_num_wp_regs = reg.db_nwtpts;
	  if (aarch64_num_wp_regs > AARCH64_HWP_MAX_NUM)
	    {
	      warning (_("Unexpected number of hardware watchpoint registers"
			 " reported by ptrace, got %d, expected %d."),
		       aarch64_num_wp_regs, AARCH64_HWP_MAX_NUM);
	      aarch64_num_wp_regs = AARCH64_HWP_MAX_NUM;
	    }
	}
    }
}

/* Implement the virtual inf_ptrace_target::post_startup_inferior method.  */

void
aarch64_fbsd_nat_target::post_startup_inferior (ptid_t ptid)
{
  aarch64_remove_debug_reg_state (ptid.pid ());
  probe_debug_regs (ptid.pid ());
  fbsd_nat_target::post_startup_inferior (ptid);
}

/* Implement the "post_attach" target_ops method.  */

void
aarch64_fbsd_nat_target::post_attach (int pid)
{
  aarch64_remove_debug_reg_state (pid);
  probe_debug_regs (pid);
  fbsd_nat_target::post_attach (pid);
}

/* Implement the virtual fbsd_nat_target::low_new_fork method.  */

void
aarch64_fbsd_nat_target::low_new_fork (ptid_t parent, pid_t child)
{
  struct aarch64_debug_reg_state *parent_state, *child_state;

  /* If there is no parent state, no watchpoints nor breakpoints have
     been set, so there is nothing to do.  */
  parent_state = aarch64_lookup_debug_reg_state (parent.pid ());
  if (parent_state == nullptr)
    return;

  /* The kernel clears debug registers in the new child process after
     fork, but GDB core assumes the child inherits the watchpoints/hw
     breakpoints of the parent, and will remove them all from the
     forked off process.  Copy the debug registers mirrors into the
     new process so that all breakpoints and watchpoints can be
     removed together.  */

  child_state = aarch64_get_debug_reg_state (child);
  *child_state = *parent_state;
}

/* Mark debug register state "dirty" for all threads belonging to the
   current inferior.  */

void
aarch64_notify_debug_reg_change (ptid_t ptid,
				 int is_watchpoint, unsigned int idx)
{
  for (thread_info *tp : current_inferior ()->non_exited_threads ())
    {
      if (tp->ptid.lwp_p ())
	aarch64_debug_pending_threads.emplace (tp->ptid.lwp ());
    }
}

/* Implement the virtual fbsd_nat_target::low_delete_thread method.  */

void
aarch64_fbsd_nat_target::low_delete_thread (thread_info *tp)
{
  gdb_assert(tp->ptid.lwp_p ());
  aarch64_debug_pending_threads.erase (tp->ptid.lwp ());
}

/* Implement the virtual fbsd_nat_target::low_prepare_to_resume method.  */

void
aarch64_fbsd_nat_target::low_prepare_to_resume (thread_info *tp)
{
  gdb_assert(tp->ptid.lwp_p ());

  if (aarch64_debug_pending_threads.erase (tp->ptid.lwp ()) == 0)
    return;

  struct aarch64_debug_reg_state *state =
    aarch64_lookup_debug_reg_state (tp->ptid.pid ());
  gdb_assert(state != nullptr);

  struct dbreg reg;
  memset (&reg, 0, sizeof(reg));
  for (int i = 0; i < aarch64_num_bp_regs; i++)
    {
      reg.db_breakregs[i].dbr_addr = state->dr_addr_bp[i];
      reg.db_breakregs[i].dbr_ctrl = state->dr_ctrl_bp[i];
    }
  for (int i = 0; i < aarch64_num_wp_regs; i++)
    {
      reg.db_watchregs[i].dbw_addr = state->dr_addr_wp[i];
      reg.db_watchregs[i].dbw_ctrl = state->dr_ctrl_wp[i];
    }
  if (ptrace(PT_SETDBREGS, tp->ptid.lwp (), (PTRACE_TYPE_ARG3) &reg, 0) != 0)
    error (_("Failed to set hardware debug registers"));
}
#else
/* A stub that should never be called.  */
void
aarch64_notify_debug_reg_change (ptid_t ptid,
				 int is_watchpoint, unsigned int idx)
{
  gdb_assert (true);
}
#endif

void _initialize_aarch64_fbsd_nat ();
void
_initialize_aarch64_fbsd_nat ()
{
#ifdef HAVE_DBREG
  aarch64_initialize_hw_point ();
#endif
  add_inf_child_target (&the_aarch64_fbsd_nat_target);
}
