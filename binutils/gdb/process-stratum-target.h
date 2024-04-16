/* Abstract base class inherited by all process_stratum targets

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#ifndef PROCESS_STRATUM_TARGET_H
#define PROCESS_STRATUM_TARGET_H

#include "target.h"
#include <set>
#include "gdbsupport/intrusive_list.h"
#include "gdbsupport/gdb-checked-static-cast.h"
#include "gdbthread.h"

/* Abstract base class inherited by all process_stratum targets.  */

class process_stratum_target : public target_ops
{
public:
  ~process_stratum_target () override = 0;

  strata stratum () const final override { return process_stratum; }

  /* Return a string representation of this target's open connection.
     This string is used to distinguish different instances of a given
     target type.  For example, when remote debugging, the target is
     called "remote", but since we may have more than one remote
     target open, connection_string() returns the connection serial
     connection name, e.g., "localhost:10001", "192.168.0.1:20000",
     etc.  This string is shown in several places, e.g., in "info
     connections" and "info inferiors".  */
  virtual const char *connection_string () { return nullptr; }

  /* We must default these because they must be implemented by any
     target that can run.  */
  bool can_async_p () override { return false; }
  bool supports_non_stop () override { return false; }
  bool supports_disable_randomization () override { return false; }

  /* This default implementation always returns the current inferior's
     gdbarch.  */
  struct gdbarch *thread_architecture (ptid_t ptid) override;

  /* Default implementations for process_stratum targets.  Return true
     if there's a selected inferior, false otherwise.  */
  bool has_all_memory () override;
  bool has_memory () override;
  bool has_stack () override;
  bool has_registers () override;
  bool has_execution (inferior *inf) override;

  /* Default implementation of follow_exec.

     If the current inferior and FOLLOW_INF are different (execution continues
     in a new inferior), push this process target to FOLLOW_INF's target stack
     and add an initial thread to FOLLOW_INF.  */
  void follow_exec (inferior *follow_inf, ptid_t ptid,
		    const char *execd_pathname) override;

  /* Default implementation of follow_fork.

     If a child inferior was created by infrun while following the fork
     (CHILD_INF is non-nullptr), push this target on CHILD_INF's target stack
     and add an initial thread with ptid CHILD_PTID.  */
  void follow_fork (inferior *child_inf, ptid_t child_ptid,
		    target_waitkind fork_kind, bool follow_child,
		    bool detach_on_fork) override;

  /* True if any thread is, or may be executing.  We need to track
     this separately because until we fully sync the thread list, we
     won't know whether the target is fully stopped, even if we see
     stop events for all known threads, because any of those threads
     may have spawned new threads we haven't heard of yet.  */
  bool threads_executing = false;

  /* If THREAD is resumed and has a pending wait status, add it to the
     target's "resumed with pending wait status" list.  */
  void maybe_add_resumed_with_pending_wait_status (thread_info *thread);

  /* If THREAD is resumed and has a pending wait status, remove it from the
     target's "resumed with pending wait status" list.  */
  void maybe_remove_resumed_with_pending_wait_status (thread_info *thread);

  /* Return true if this target has at least one resumed thread with a pending
     wait status.  */
  bool has_resumed_with_pending_wait_status () const
  { return !m_resumed_with_pending_wait_status.empty (); }

  /* Return a random resumed thread with pending wait status belonging to INF
     and matching FILTER_PTID.  */
  thread_info *random_resumed_with_pending_wait_status
    (inferior *inf, ptid_t filter_ptid);

  /* Search function to lookup a (non-exited) thread by 'ptid'.  */
  thread_info *find_thread (ptid_t ptid);

  /* The connection number.  Visible in "info connections".  */
  int connection_number = 0;

  /* Whether resumed threads must be committed to the target.

     When true, resumed threads must be committed to the execution
     target.

     When false, the target may leave resumed threads stopped when
     it's convenient or efficient to do so.  When the core requires
     resumed threads to be committed again, this is set back to true
     and calls the `commit_resumed` method to allow the target to do
     so.

     To simplify the implementation of targets, the following methods
     are guaranteed to be called with COMMIT_RESUMED_STATE set to
     false:

       - resume
       - stop
       - wait

     Knowing this, the target doesn't need to implement different
     behaviors depending on the COMMIT_RESUMED_STATE, and can simply
     assume that it is false.

     Targets can take advantage of this to batch resumption requests,
     for example.  In that case, the target doesn't actually resume in
     its `resume` implementation.  Instead, it takes note of the
     resumption intent in `resume` and defers the actual resumption to
     `commit_resumed`.  For example, the remote target uses this to
     coalesce multiple resumption requests in a single vCont
     packet.  */
  bool commit_resumed_state = false;

private:
  /* List of threads managed by this target which simultaneously are resumed
     and have a pending wait status.

     This is done for optimization reasons, it would be possible to walk the
     inferior thread lists to find these threads.  But since this is something
     we need to do quite frequently in the hot path, maintaining this list
     avoids walking the thread lists repeatedly.  */
  thread_info_resumed_with_pending_wait_status_list
    m_resumed_with_pending_wait_status;
};

/* Downcast TARGET to process_stratum_target.  */

static inline process_stratum_target *
as_process_stratum_target (target_ops *target)
{
  gdb_assert (target->stratum () == process_stratum);
  return gdb::checked_static_cast<process_stratum_target *> (target);
}

/* Return a collection of targets that have non-exited inferiors.  */

extern std::set<process_stratum_target *> all_non_exited_process_targets ();

/* Switch to the first inferior (and program space) of TARGET, and
   switch to no thread selected.  */

extern void switch_to_target_no_thread (process_stratum_target *target);

#endif /* !defined (PROCESS_STRATUM_TARGET_H) */
