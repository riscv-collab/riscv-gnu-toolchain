/* Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/break-common.h"
#include "nat/linux-nat.h"
#include "nat/aarch64-linux-hw-point.h"
#include "nat/aarch64-linux.h"

#include "elf/common.h"
#include "nat/gdb_ptrace.h"
#include <asm/ptrace.h>
#include <sys/uio.h>

/* Called when resuming a thread LWP.
   The hardware debug registers are updated when there is any change.  */

void
aarch64_linux_prepare_to_resume (struct lwp_info *lwp)
{
  struct arch_lwp_info *info = lwp_arch_private_info (lwp);

  /* NULL means this is the main thread still going through the shell,
     or, no watchpoint has been set yet.  In that case, there's
     nothing to do.  */
  if (info == NULL)
    return;

  if (DR_HAS_CHANGED (info->dr_changed_bp)
      || DR_HAS_CHANGED (info->dr_changed_wp))
    {
      ptid_t ptid = ptid_of_lwp (lwp);
      int tid = ptid.lwp ();
      struct aarch64_debug_reg_state *state
	= aarch64_get_debug_reg_state (ptid.pid ());

      if (show_debug_regs)
	debug_printf ("prepare_to_resume thread %d\n", tid);

      /* Watchpoints.  */
      if (DR_HAS_CHANGED (info->dr_changed_wp))
	{
	  aarch64_linux_set_debug_regs (state, tid, 1);
	  DR_CLEAR_CHANGED (info->dr_changed_wp);
	}

      /* Breakpoints.  */
      if (DR_HAS_CHANGED (info->dr_changed_bp))
	{
	  aarch64_linux_set_debug_regs (state, tid, 0);
	  DR_CLEAR_CHANGED (info->dr_changed_bp);
	}
    }
}

/* Function to call when a new thread is detected.  */

void
aarch64_linux_new_thread (struct lwp_info *lwp)
{
  ptid_t ptid = ptid_of_lwp (lwp);
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (ptid.pid ());
  struct arch_lwp_info *info = XCNEW (struct arch_lwp_info);

  /* If there are hardware breakpoints/watchpoints in the process then mark that
     all the hardware breakpoint/watchpoint register pairs for this thread need
     to be initialized (with data from aarch_process_info.debug_reg_state).  */
  if (aarch64_any_set_debug_regs_state (state, false))
    DR_MARK_ALL_CHANGED (info->dr_changed_bp, aarch64_num_bp_regs);
  if (aarch64_any_set_debug_regs_state (state, true))
    DR_MARK_ALL_CHANGED (info->dr_changed_wp, aarch64_num_wp_regs);

  lwp_set_arch_private_info (lwp, info);
}

/* See nat/aarch64-linux.h.  */

void
aarch64_linux_delete_thread (struct arch_lwp_info *arch_lwp)
{
  xfree (arch_lwp);
}

/* Convert native siginfo FROM to the siginfo in the layout of the
   inferior's architecture TO.  */

void
aarch64_compat_siginfo_from_siginfo (compat_siginfo_t *to, siginfo_t *from)
{
  memset (to, 0, sizeof (*to));

  to->si_signo = from->si_signo;
  to->si_errno = from->si_errno;
  to->si_code = from->si_code;

  if (to->si_code == SI_TIMER)
    {
      to->cpt_si_timerid = from->si_timerid;
      to->cpt_si_overrun = from->si_overrun;
      to->cpt_si_ptr = (intptr_t) from->si_ptr;
    }
  else if (to->si_code == SI_USER)
    {
      to->cpt_si_pid = from->si_pid;
      to->cpt_si_uid = from->si_uid;
    }
  else if (to->si_code < 0)
    {
      to->cpt_si_pid = from->si_pid;
      to->cpt_si_uid = from->si_uid;
      to->cpt_si_ptr = (intptr_t) from->si_ptr;
    }
  else
    {
      switch (to->si_signo)
	{
	case SIGCHLD:
	  to->cpt_si_pid = from->si_pid;
	  to->cpt_si_uid = from->si_uid;
	  to->cpt_si_status = from->si_status;
	  to->cpt_si_utime = from->si_utime;
	  to->cpt_si_stime = from->si_stime;
	  break;
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
	  to->cpt_si_addr = (intptr_t) from->si_addr;
	  break;
	case SIGPOLL:
	  to->cpt_si_band = from->si_band;
	  to->cpt_si_fd = from->si_fd;
	  break;
	default:
	  to->cpt_si_pid = from->si_pid;
	  to->cpt_si_uid = from->si_uid;
	  to->cpt_si_ptr = (intptr_t) from->si_ptr;
	  break;
	}
    }
}

/* Convert inferior's architecture siginfo FROM to native siginfo TO.  */

void
aarch64_siginfo_from_compat_siginfo (siginfo_t *to, compat_siginfo_t *from)
{
  memset (to, 0, sizeof (*to));

  to->si_signo = from->si_signo;
  to->si_errno = from->si_errno;
  to->si_code = from->si_code;

  if (to->si_code == SI_TIMER)
    {
      to->si_timerid = from->cpt_si_timerid;
      to->si_overrun = from->cpt_si_overrun;
      to->si_ptr = (void *) (intptr_t) from->cpt_si_ptr;
    }
  else if (to->si_code == SI_USER)
    {
      to->si_pid = from->cpt_si_pid;
      to->si_uid = from->cpt_si_uid;
    }
  if (to->si_code < 0)
    {
      to->si_pid = from->cpt_si_pid;
      to->si_uid = from->cpt_si_uid;
      to->si_ptr = (void *) (intptr_t) from->cpt_si_ptr;
    }
  else
    {
      switch (to->si_signo)
	{
	case SIGCHLD:
	  to->si_pid = from->cpt_si_pid;
	  to->si_uid = from->cpt_si_uid;
	  to->si_status = from->cpt_si_status;
	  to->si_utime = from->cpt_si_utime;
	  to->si_stime = from->cpt_si_stime;
	  break;
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
	  to->si_addr = (void *) (intptr_t) from->cpt_si_addr;
	  break;
	case SIGPOLL:
	  to->si_band = from->cpt_si_band;
	  to->si_fd = from->cpt_si_fd;
	  break;
	default:
	  to->si_pid = from->cpt_si_pid;
	  to->si_uid = from->cpt_si_uid;
	  to->si_ptr = (void* ) (intptr_t) from->cpt_si_ptr;
	  break;
	}
    }
}

/* Called by libthread_db.  Returns a pointer to the thread local
   storage (or its descriptor).  */

ps_err_e
aarch64_ps_get_thread_area (struct ps_prochandle *ph,
			    lwpid_t lwpid, int idx, void **base,
			    int is_64bit_p)
{
  struct iovec iovec;
  uint64_t reg64;
  uint32_t reg32;

  if (is_64bit_p)
    {
      iovec.iov_base = &reg64;
      iovec.iov_len = sizeof (reg64);
    }
  else
    {
      iovec.iov_base = &reg32;
      iovec.iov_len = sizeof (reg32);
    }

  if (ptrace (PTRACE_GETREGSET, lwpid, NT_ARM_TLS, &iovec) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  if (is_64bit_p)
    *base = (void *) (reg64 - idx);
  else
    *base = (void *) (uintptr_t) (reg32 - idx);

  return PS_OK;
}

/* See nat/aarch64-linux.h.  */

int
aarch64_tls_register_count (int tid)
{
  uint64_t tls_regs[2];
  struct iovec iovec;
  iovec.iov_base = tls_regs;
  iovec.iov_len = sizeof (tls_regs);

  /* Attempt to read both TPIDR and TPIDR2.  If ptrace returns less data than
     we are expecting, that means it doesn't support all the registers.  From
     the iovec length, figure out how many TPIDR registers the target actually
     supports.  */
  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_TLS, &iovec) != 0)
    return 0;

  /* Calculate how many TPIDR registers we have.  */
  return iovec.iov_len / sizeof (uint64_t);
}
