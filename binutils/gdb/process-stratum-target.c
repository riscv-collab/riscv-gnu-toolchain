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

#include "defs.h"
#include "process-stratum-target.h"
#include "inferior.h"
#include <algorithm>

process_stratum_target::~process_stratum_target ()
{
}

struct gdbarch *
process_stratum_target::thread_architecture (ptid_t ptid)
{
  inferior *inf = find_inferior_ptid (this, ptid);
  gdb_assert (inf != NULL);
  return inf->arch ();
}

bool
process_stratum_target::has_all_memory ()
{
  /* If no inferior selected, then we can't read memory here.  */
  return inferior_ptid != null_ptid;
}

bool
process_stratum_target::has_memory ()
{
  /* If no inferior selected, then we can't read memory here.  */
  return inferior_ptid != null_ptid;
}

bool
process_stratum_target::has_stack ()
{
  /* If no inferior selected, there's no stack.  */
  return inferior_ptid != null_ptid;
}

bool
process_stratum_target::has_registers ()
{
  /* Can't read registers from no inferior.  */
  return inferior_ptid != null_ptid;
}

bool
process_stratum_target::has_execution (inferior *inf)
{
  /* If there's a process running already, we can't make it run
     through hoops.  */
  return inf->pid != 0;
}

/* See process-stratum-target.h.  */

void
process_stratum_target::follow_exec (inferior *follow_inf, ptid_t ptid,
				     const char *execd_pathname)
{
  inferior *orig_inf = current_inferior ();

  if (orig_inf != follow_inf)
    {
      /* Execution continues in a new inferior, push the original inferior's
	 process target on the new inferior's target stack.  The process target
	 may decide to unpush itself from the original inferior's target stack
	 after that, at its discretion.  */
      follow_inf->push_target (orig_inf->process_target ());
      thread_info *t = add_thread (follow_inf->process_target (), ptid);

      /* Leave the new inferior / thread as the current inferior / thread.  */
      switch_to_thread (t);
    }
}

/* See process-stratum-target.h.  */

void
process_stratum_target::follow_fork (inferior *child_inf, ptid_t child_ptid,
				     target_waitkind fork_kind,
				     bool follow_child,
				     bool detach_on_fork)
{
  if (child_inf != nullptr)
    {
      child_inf->push_target (this);
      add_thread_silent (this, child_ptid);
    }
}

/* See process-stratum-target.h.  */

void
process_stratum_target::maybe_add_resumed_with_pending_wait_status
  (thread_info *thread)
{
  gdb_assert (!thread->resumed_with_pending_wait_status_node.is_linked ());

  if (thread->resumed () && thread->has_pending_waitstatus ())
    {
      infrun_debug_printf ("adding to resumed threads with event list: %s",
			   thread->ptid.to_string ().c_str ());
      m_resumed_with_pending_wait_status.push_back (*thread);
    }
}

/* See process-stratum-target.h.  */

void
process_stratum_target::maybe_remove_resumed_with_pending_wait_status
  (thread_info *thread)
{
  if (thread->resumed () && thread->has_pending_waitstatus ())
    {
      infrun_debug_printf ("removing from resumed threads with event list: %s",
			   thread->ptid.to_string ().c_str ());
      gdb_assert (thread->resumed_with_pending_wait_status_node.is_linked ());
      auto it = m_resumed_with_pending_wait_status.iterator_to (*thread);
      m_resumed_with_pending_wait_status.erase (it);
    }
  else
    gdb_assert (!thread->resumed_with_pending_wait_status_node.is_linked ());
}

/* See process-stratum-target.h.  */

thread_info *
process_stratum_target::random_resumed_with_pending_wait_status
  (inferior *inf, ptid_t filter_ptid)
{
  auto matches = [inf, filter_ptid] (const thread_info &thread)
    {
      return thread.inf == inf && thread.ptid.matches (filter_ptid);
    };

  /* First see how many matching events we have.  */
  const auto &l = m_resumed_with_pending_wait_status;
  unsigned int count = std::count_if (l.begin (), l.end (), matches);

  if (count == 0)
    return nullptr;

  /* Now randomly pick a thread out of those that match the criteria.  */
  int random_selector
    = (int) ((count * (double) rand ()) / (RAND_MAX + 1.0));

  if (count > 1)
    infrun_debug_printf ("Found %u events, selecting #%d",
			 count, random_selector);

  /* Select the Nth thread that matches.  */
  auto it = std::find_if (l.begin (), l.end (),
			  [&random_selector, &matches]
			  (const thread_info &thread)
    {
      if (!matches (thread))
	return false;

      return random_selector-- == 0;
    });

  gdb_assert (it != l.end ());

  return &*it;
}

/* See process-stratum-target.h.  */

thread_info *
process_stratum_target::find_thread (ptid_t ptid)
{
  inferior *inf = find_inferior_ptid (this, ptid);
  if (inf == NULL)
    return NULL;
  return inf->find_thread (ptid);
}

/* See process-stratum-target.h.  */

std::set<process_stratum_target *>
all_non_exited_process_targets ()
{
  /* Inferiors may share targets.  To eliminate duplicates, use a set.  */
  std::set<process_stratum_target *> targets;
  for (inferior *inf : all_non_exited_inferiors ())
    targets.insert (inf->process_target ());

  return targets;
}

/* See process-stratum-target.h.  */

void
switch_to_target_no_thread (process_stratum_target *target)
{
  for (inferior *inf : all_inferiors (target))
    {
      switch_to_inferior_no_thread (inf);
      break;
    }
}
