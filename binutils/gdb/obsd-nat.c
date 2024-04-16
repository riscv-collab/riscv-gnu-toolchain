/* Native-dependent code for OpenBSD.

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
#include "gdbthread.h"
#include "inferior.h"
#include "target.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include "gdbsupport/gdb_wait.h"

#include "inf-ptrace.h"
#include "obsd-nat.h"

/* OpenBSD 5.2 and later include rthreads which uses a thread model
   that maps userland threads directly onto kernel threads in a 1:1
   fashion.  */

std::string
obsd_nat_target::pid_to_str (ptid_t ptid)
{
  if (ptid.lwp () != 0)
    return string_printf ("thread %ld of process %d", ptid.lwp (), ptid.pid ());

  return normal_pid_to_str (ptid);
}

void
obsd_nat_target::update_thread_list ()
{
  pid_t pid = inferior_ptid.pid ();
  struct ptrace_thread_state pts;

  prune_threads ();

  if (ptrace (PT_GET_THREAD_FIRST, pid, (caddr_t)&pts, sizeof pts) == -1)
    perror_with_name (("ptrace"));

  while (pts.pts_tid != -1)
    {
      ptid_t ptid = ptid_t (pid, pts.pts_tid, 0);

      if (!in_thread_list (this, ptid))
	{
	  if (inferior_ptid.lwp () == 0)
	    thread_change_ptid (this, inferior_ptid, ptid);
	  else
	    add_thread (this, ptid);
	}

      if (ptrace (PT_GET_THREAD_NEXT, pid, (caddr_t)&pts, sizeof pts) == -1)
	perror_with_name (("ptrace"));
    }
}

/* Enable additional event reporting on a new or existing process.  */

static void
obsd_enable_proc_events (pid_t pid)
{
  ptrace_event_t pe;

  /* Set the initial event mask.  */
  memset (&pe, 0, sizeof pe);
  pe.pe_set_event |= PTRACE_FORK;
  if (ptrace (PT_SET_EVENT_MASK, pid,
	      (PTRACE_TYPE_ARG3)&pe, sizeof pe) == -1)
    perror_with_name (("ptrace"));
}

ptid_t
obsd_nat_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
		       target_wait_flags options)
{
  ptid_t wptid = inf_ptrace_target::wait (ptid, ourstatus, options);
  if (ourstatus->kind () == TARGET_WAITKIND_STOPPED)
    {
      ptrace_state_t pe;

      pid_t pid = wptid.pid ();
      if (ptrace (PT_GET_PROCESS_STATE, pid, (caddr_t)&pe, sizeof pe) == -1)
	perror_with_name (("ptrace"));

      wptid = ptid_t (pid, pe.pe_tid, 0);

      switch (pe.pe_report_event)
	{
	case PTRACE_FORK:
	  ourstatus->set_forked (ptid_t (pe.pe_other_pid));

	  /* Make sure the other end of the fork is stopped too.  */
	  pid_t fpid = waitpid (pe.pe_other_pid, nullptr, 0);
	  if (fpid == -1)
	    perror_with_name (("waitpid"));

	  if (ptrace (PT_GET_PROCESS_STATE, fpid,
		      (caddr_t)&pe, sizeof pe) == -1)
	    perror_with_name (("ptrace"));

	  gdb_assert (pe.pe_report_event == PTRACE_FORK);
	  gdb_assert (pe.pe_other_pid == pid);
	  if (find_inferior_pid (this, fpid) != nullptr)
	    {
	      ourstatus->set_forked (ptid_t (pe.pe_other_pid));
	      wptid = ptid_t (fpid, pe.pe_tid, 0);
	    }

	  obsd_enable_proc_events (ourstatus->child_ptid ().pid ());
	  break;
	}

      /* Ensure the ptid is updated with an LWP id on the first stop
	 of a process.  */
      if (!in_thread_list (this, wptid))
	{
	  if (in_thread_list (this, ptid_t (pid)))
	    thread_change_ptid (this, ptid_t (pid), wptid);
	  else
	    add_thread (this, wptid);
	}
    }
  return wptid;
}

void
obsd_nat_target::post_attach (int pid)
{
  obsd_enable_proc_events (pid);
}

/* Implement the virtual inf_ptrace_target::post_startup_inferior method.  */

void
obsd_nat_target::post_startup_inferior (ptid_t pid)
{
  obsd_enable_proc_events (pid.pid ());
}

/* Target hook for follow_fork.  */

void
obsd_nat_target::follow_fork (inferior *child_inf, ptid_t child_ptid,
			      target_waitkind fork_kind,
			      bool follow_child, bool detach_fork)
{
  inf_ptrace_target::follow_fork (child_inf, child_ptid, fork_kind,
				  follow_child, detach_fork);

  if (!follow_child && detach_fork)
    {
      /* Breakpoints have already been detached from the child by
	 infrun.c.  */

      if (ptrace (PT_DETACH, child_ptid.pid (), (PTRACE_TYPE_ARG3)1, 0) == -1)
	perror_with_name (("ptrace"));
    }
}

int
obsd_nat_target::insert_fork_catchpoint (int pid)
{
  return 0;
}

int
obsd_nat_target::remove_fork_catchpoint (int pid)
{
  return 0;
}
