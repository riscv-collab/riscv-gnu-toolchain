/* Multi-process/thread control for GDB, the GNU debugger.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

   Contributed by Lynx Real-Time Systems, Inc.  Los Gatos, CA.

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
#include "language.h"
#include "symtab.h"
#include "frame.h"
#include "inferior.h"
#include "gdbsupport/environ.h"
#include "value.h"
#include "target.h"
#include "gdbthread.h"
#include "command.h"
#include "gdbcmd.h"
#include "regcache.h"
#include "btrace.h"

#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#include "ui-out.h"
#include "observable.h"
#include "annotate.h"
#include "cli/cli-decode.h"
#include "cli/cli-option.h"
#include "gdbsupport/gdb_regex.h"
#include "cli/cli-utils.h"
#include "thread-fsm.h"
#include "tid-parse.h"
#include <algorithm>
#include <optional>
#include "inline-frame.h"
#include "stack.h"
#include "interps.h"

/* See gdbthread.h.  */

bool debug_threads = false;

/* Implement 'show debug threads'.  */

static void
show_debug_threads (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Thread debugging is \"%s\".\n"), value);
}

/* Definition of struct thread_info exported to gdbthread.h.  */

/* Prototypes for local functions.  */

static int highest_thread_num;

/* The current/selected thread.  */
static thread_info *current_thread_;

/* Returns true if THR is the current thread.  */

static bool
is_current_thread (const thread_info *thr)
{
  return thr == current_thread_;
}

struct thread_info*
inferior_thread (void)
{
  gdb_assert (current_thread_ != nullptr);
  return current_thread_;
}

/* Delete the breakpoint pointed at by BP_P, if there's one.  */

static void
delete_thread_breakpoint (struct breakpoint **bp_p)
{
  if (*bp_p != NULL)
    {
      delete_breakpoint (*bp_p);
      *bp_p = NULL;
    }
}

void
delete_step_resume_breakpoint (struct thread_info *tp)
{
  if (tp != NULL)
    delete_thread_breakpoint (&tp->control.step_resume_breakpoint);
}

void
delete_exception_resume_breakpoint (struct thread_info *tp)
{
  if (tp != NULL)
    delete_thread_breakpoint (&tp->control.exception_resume_breakpoint);
}

/* See gdbthread.h.  */

void
delete_single_step_breakpoints (struct thread_info *tp)
{
  if (tp != NULL)
    delete_thread_breakpoint (&tp->control.single_step_breakpoints);
}

/* Delete the breakpoint pointed at by BP_P at the next stop, if
   there's one.  */

static void
delete_at_next_stop (struct breakpoint **bp)
{
  if (*bp != NULL)
    {
      (*bp)->disposition = disp_del_at_next_stop;
      *bp = NULL;
    }
}

/* See gdbthread.h.  */

int
thread_has_single_step_breakpoints_set (struct thread_info *tp)
{
  return tp->control.single_step_breakpoints != NULL;
}

/* See gdbthread.h.  */

int
thread_has_single_step_breakpoint_here (struct thread_info *tp,
					const address_space *aspace,
					CORE_ADDR addr)
{
  struct breakpoint *ss_bps = tp->control.single_step_breakpoints;

  return (ss_bps != NULL
	  && breakpoint_has_location_inserted_here (ss_bps, aspace, addr));
}

/* See gdbthread.h.  */

void
thread_cancel_execution_command (struct thread_info *thr)
{
  if (thr->thread_fsm () != nullptr)
    {
      std::unique_ptr<thread_fsm> fsm = thr->release_thread_fsm ();
      fsm->clean_up (thr);
    }
}

static void
clear_thread_inferior_resources (struct thread_info *tp)
{
  /* NOTE: this will take care of any left-over step_resume breakpoints,
     but not any user-specified thread-specific breakpoints.  We can not
     delete the breakpoint straight-off, because the inferior might not
     be stopped at the moment.  */
  delete_at_next_stop (&tp->control.step_resume_breakpoint);
  delete_at_next_stop (&tp->control.exception_resume_breakpoint);
  delete_at_next_stop (&tp->control.single_step_breakpoints);

  delete_longjmp_breakpoint_at_next_stop (tp->global_num);

  bpstat_clear (&tp->control.stop_bpstat);

  btrace_teardown (tp);

  thread_cancel_execution_command (tp);

  clear_inline_frame_state (tp);
}

/* Notify interpreters and observers that thread T has exited.  */

static void
notify_thread_exited (thread_info *t, std::optional<ULONGEST> exit_code,
		      int silent)
{
  if (!silent && print_thread_events)
    {
      if (exit_code.has_value ())
	gdb_printf (_("[%s exited with code %s]\n"),
		    target_pid_to_str (t->ptid).c_str (),
		    pulongest (*exit_code));
      else
	gdb_printf (_("[%s exited]\n"),
		    target_pid_to_str (t->ptid).c_str ());
    }

  interps_notify_thread_exited (t, exit_code, silent);
  gdb::observers::thread_exit.notify (t, exit_code, silent);
}

/* See gdbthread.h.  */

void
set_thread_exited (thread_info *tp, std::optional<ULONGEST> exit_code,
		   bool silent)
{
  /* Dead threads don't need to step-over.  Remove from chain.  */
  if (thread_is_in_step_over_chain (tp))
    global_thread_step_over_chain_remove (tp);

  if (tp->state != THREAD_EXITED)
    {
      process_stratum_target *proc_target = tp->inf->process_target ();

      /* Some targets unpush themselves from the inferior's target stack before
	 clearing the inferior's thread list (which marks all threads as exited,
	 and therefore leads to this function).  In this case, the inferior's
	 process target will be nullptr when we arrive here.

	 See also the comment in inferior::unpush_target.  */
      if (proc_target != nullptr)
	proc_target->maybe_remove_resumed_with_pending_wait_status (tp);

      notify_thread_exited (tp, exit_code, silent);

      /* Tag it as exited.  */
      tp->state = THREAD_EXITED;

      /* Clear breakpoints, etc. associated with this thread.  */
      clear_thread_inferior_resources (tp);

      /* Remove from the ptid_t map.  We don't want for
	 inferior::find_thread to find exited threads.  Also, the target
	 may reuse the ptid for a new thread, and there can only be
	 one value per key; adding a new thread with the same ptid_t
	 would overwrite the exited thread's ptid entry.  */
      size_t nr_deleted = tp->inf->ptid_thread_map.erase (tp->ptid);
      gdb_assert (nr_deleted == 1);
    }
}

void
init_thread_list (void)
{
  highest_thread_num = 0;

  for (inferior *inf : all_inferiors ())
    inf->clear_thread_list ();
}

/* Allocate a new thread of inferior INF with target id PTID and add
   it to the thread list.  */

static struct thread_info *
new_thread (struct inferior *inf, ptid_t ptid)
{
  thread_info *tp = new thread_info (inf, ptid);

  threads_debug_printf ("creating a new thread object, inferior %d, ptid %s",
			inf->num, ptid.to_string ().c_str ());

  inf->thread_list.push_back (*tp);

  /* A thread with this ptid should not exist in the map yet.  */
  gdb_assert (inf->ptid_thread_map.find (ptid) == inf->ptid_thread_map.end ());

  inf->ptid_thread_map[ptid] = tp;

  return tp;
}

/* Notify interpreters and observers that thread T has been created.  */

static void
notify_new_thread (thread_info *t)
{
  interps_notify_new_thread (t);
  gdb::observers::new_thread.notify (t);
}

struct thread_info *
add_thread_silent (process_stratum_target *targ, ptid_t ptid)
{
  gdb_assert (targ != nullptr);

  inferior *inf = find_inferior_ptid (targ, ptid);

  threads_debug_printf ("add thread to inferior %d, ptid %s, target %s",
			inf->num, ptid.to_string ().c_str (),
			targ->shortname ());

  /* We may have an old thread with the same id in the thread list.
     If we do, it must be dead, otherwise we wouldn't be adding a new
     thread with the same id.  The OS is reusing this id --- delete
     the old thread, and create a new one.  */
  thread_info *tp = inf->find_thread (ptid);
  if (tp != nullptr)
    delete_thread (tp);

  tp = new_thread (inf, ptid);
  notify_new_thread (tp);

  return tp;
}

struct thread_info *
add_thread_with_info (process_stratum_target *targ, ptid_t ptid,
		      private_thread_info_up priv)
{
  thread_info *result = add_thread_silent (targ, ptid);

  result->priv = std::move (priv);

  if (print_thread_events)
    gdb_printf (_("[New %s]\n"), target_pid_to_str (ptid).c_str ());

  annotate_new_thread ();
  return result;
}

struct thread_info *
add_thread (process_stratum_target *targ, ptid_t ptid)
{
  return add_thread_with_info (targ, ptid, NULL);
}

private_thread_info::~private_thread_info () = default;

thread_info::thread_info (struct inferior *inf_, ptid_t ptid_)
  : ptid (ptid_), inf (inf_)
{
  gdb_assert (inf_ != NULL);

  this->global_num = ++highest_thread_num;
  this->per_inf_num = ++inf_->highest_thread_num;

  /* Nothing to follow yet.  */
  this->pending_follow.set_spurious ();
}

/* See gdbthread.h.  */

thread_info::~thread_info ()
{
  threads_debug_printf ("thread %s", this->ptid.to_string ().c_str ());
}

/* See gdbthread.h.  */

bool
thread_info::deletable () const
{
  /* If this is the current thread, or there's code out there that
     relies on it existing (refcount > 0) we can't delete yet.  */
  return refcount () == 0 && !is_current_thread (this);
}

/* See gdbthread.h.  */

void
thread_info::set_executing (bool executing)
{
  m_executing = executing;
  if (executing)
    this->clear_stop_pc ();
}

/* See gdbthread.h.  */

void
thread_info::set_resumed (bool resumed)
{
  if (resumed == m_resumed)
    return;

  process_stratum_target *proc_target = this->inf->process_target ();

  /* If we transition from resumed to not resumed, we might need to remove
     the thread from the resumed threads with pending statuses list.  */
  if (!resumed)
    proc_target->maybe_remove_resumed_with_pending_wait_status (this);

  m_resumed = resumed;

  /* If we transition from not resumed to resumed, we might need to add
     the thread to the resumed threads with pending statuses list.  */
  if (resumed)
    proc_target->maybe_add_resumed_with_pending_wait_status (this);
}

/* See gdbthread.h.  */

void
thread_info::set_pending_waitstatus (const target_waitstatus &ws)
{
  gdb_assert (!this->has_pending_waitstatus ());

  m_suspend.waitstatus = ws;
  m_suspend.waitstatus_pending_p = 1;

  process_stratum_target *proc_target = this->inf->process_target ();
  proc_target->maybe_add_resumed_with_pending_wait_status (this);
}

/* See gdbthread.h.  */

void
thread_info::clear_pending_waitstatus ()
{
  gdb_assert (this->has_pending_waitstatus ());

  process_stratum_target *proc_target = this->inf->process_target ();
  proc_target->maybe_remove_resumed_with_pending_wait_status (this);

  m_suspend.waitstatus_pending_p = 0;
}

/* See gdbthread.h.  */

void
thread_info::set_thread_options (gdb_thread_options thread_options)
{
  gdb_assert (this->state != THREAD_EXITED);
  gdb_assert (!this->executing ());

  if (m_thread_options == thread_options)
    return;

  m_thread_options = thread_options;

  infrun_debug_printf ("[options for %s are now %s]",
		       this->ptid.to_string ().c_str (),
		       to_string (thread_options).c_str ());
}

/* See gdbthread.h.  */

int
thread_is_in_step_over_chain (struct thread_info *tp)
{
  return tp->step_over_list_node.is_linked ();
}

/* See gdbthread.h.  */

int
thread_step_over_chain_length (const thread_step_over_list &l)
{
  int num = 0;

  for (const thread_info &thread ATTRIBUTE_UNUSED : l)
    ++num;

  return num;
}

/* See gdbthread.h.  */

void
global_thread_step_over_chain_enqueue (struct thread_info *tp)
{
  infrun_debug_printf ("enqueueing thread %s in global step over chain",
		       tp->ptid.to_string ().c_str ());

  gdb_assert (!thread_is_in_step_over_chain (tp));
  global_thread_step_over_list.push_back (*tp);
}

/* See gdbthread.h.  */

void
global_thread_step_over_chain_enqueue_chain (thread_step_over_list &&list)
{
  global_thread_step_over_list.splice (std::move (list));
}

/* See gdbthread.h.  */

void
global_thread_step_over_chain_remove (struct thread_info *tp)
{
  infrun_debug_printf ("removing thread %s from global step over chain",
		       tp->ptid.to_string ().c_str ());

  gdb_assert (thread_is_in_step_over_chain (tp));
  auto it = global_thread_step_over_list.iterator_to (*tp);
  global_thread_step_over_list.erase (it);
}

/* Helper for the different delete_thread variants.  */

static void
delete_thread_1 (thread_info *thr, std::optional<ULONGEST> exit_code,
		 bool silent)
{
  gdb_assert (thr != nullptr);

  threads_debug_printf ("deleting thread %s, exit_code = %s, silent = %d",
			thr->ptid.to_string ().c_str (),
			(exit_code.has_value ()
			 ? pulongest (*exit_code)
			 : "<none>"),
			silent);

  set_thread_exited (thr, exit_code, silent);

  if (!thr->deletable ())
    {
       /* Will be really deleted some other time.  */
       return;
     }

  auto it = thr->inf->thread_list.iterator_to (*thr);
  thr->inf->thread_list.erase (it);

  gdb::observers::thread_deleted.notify (thr);

  delete thr;
}

/* See gdbthread.h.  */

void
delete_thread_with_exit_code (thread_info *thread, ULONGEST exit_code,
			      bool silent)
{
  delete_thread_1 (thread, exit_code, silent);
}

/* See gdbthread.h.  */

void
delete_thread (thread_info *thread)
{
  delete_thread_1 (thread, {}, false /* not silent */);
}

void
delete_thread_silent (thread_info *thread)
{
  delete_thread_1 (thread, {}, true /* not silent */);
}

struct thread_info *
find_thread_global_id (int global_id)
{
  for (thread_info *tp : all_threads ())
    if (tp->global_num == global_id)
      return tp;

  return NULL;
}

static struct thread_info *
find_thread_id (struct inferior *inf, int thr_num)
{
  for (thread_info *tp : inf->threads ())
    if (tp->per_inf_num == thr_num)
      return tp;

  return NULL;
}

/* See gdbthread.h.  */

struct thread_info *
find_thread_by_handle (gdb::array_view<const gdb_byte> handle,
		       struct inferior *inf)
{
  return target_thread_handle_to_thread_info (handle.data (),
					      handle.size (),
					      inf);
}

/*
 * Thread iterator function.
 *
 * Calls a callback function once for each thread, so long as
 * the callback function returns false.  If the callback function
 * returns true, the iteration will end and the current thread
 * will be returned.  This can be useful for implementing a
 * search for a thread with arbitrary attributes, or for applying
 * some operation to every thread.
 *
 * FIXME: some of the existing functionality, such as
 * "Thread apply all", might be rewritten using this functionality.
 */

struct thread_info *
iterate_over_threads (int (*callback) (struct thread_info *, void *),
		      void *data)
{
  for (thread_info *tp : all_threads_safe ())
    if ((*callback) (tp, data))
      return tp;

  return NULL;
}

/* See gdbthread.h.  */

bool
any_thread_p ()
{
  for (thread_info *tp ATTRIBUTE_UNUSED : all_threads ())
    return true;
  return false;
}

int
thread_count (process_stratum_target *proc_target)
{
  auto rng = all_threads (proc_target);
  return std::distance (rng.begin (), rng.end ());
}

/* Return the number of non-exited threads in the thread list.  */

static int
live_threads_count (void)
{
  auto rng = all_non_exited_threads ();
  return std::distance (rng.begin (), rng.end ());
}

int
valid_global_thread_id (int global_id)
{
  for (thread_info *tp : all_threads ())
    if (tp->global_num == global_id)
      return 1;

  return 0;
}

bool
in_thread_list (process_stratum_target *targ, ptid_t ptid)
{
  return targ->find_thread (ptid) != nullptr;
}

/* Finds the first thread of the inferior.  */

thread_info *
first_thread_of_inferior (inferior *inf)
{
  if (inf->thread_list.empty ())
    return nullptr;

  return &inf->thread_list.front ();
}

thread_info *
any_thread_of_inferior (inferior *inf)
{
  gdb_assert (inf->pid != 0);

  /* Prefer the current thread, if there's one.  */
  if (inf == current_inferior () && inferior_ptid != null_ptid)
    return inferior_thread ();

  for (thread_info *tp : inf->non_exited_threads ())
    return tp;

  return NULL;
}

thread_info *
any_live_thread_of_inferior (inferior *inf)
{
  struct thread_info *curr_tp = NULL;
  struct thread_info *tp_executing = NULL;

  gdb_assert (inf != NULL && inf->pid != 0);

  /* Prefer the current thread if it's not executing.  */
  if (inferior_ptid != null_ptid && current_inferior () == inf)
    {
      /* If the current thread is dead, forget it.  If it's not
	 executing, use it.  Otherwise, still choose it (below), but
	 only if no other non-executing thread is found.  */
      curr_tp = inferior_thread ();
      if (curr_tp->state == THREAD_EXITED)
	curr_tp = NULL;
      else if (!curr_tp->executing ())
	return curr_tp;
    }

  for (thread_info *tp : inf->non_exited_threads ())
    {
      if (!tp->executing ())
	return tp;

      tp_executing = tp;
    }

  /* If both the current thread and all live threads are executing,
     prefer the current thread.  */
  if (curr_tp != NULL)
    return curr_tp;

  /* Otherwise, just return an executing thread, if any.  */
  return tp_executing;
}

/* Return true if TP is an active thread.  */
static bool
thread_alive (thread_info *tp)
{
  if (tp->state == THREAD_EXITED)
    return false;

  /* Ensure we're looking at the right target stack.  */
  gdb_assert (tp->inf == current_inferior ());

  return target_thread_alive (tp->ptid);
}

/* See gdbthreads.h.  */

bool
switch_to_thread_if_alive (thread_info *thr)
{
  scoped_restore_current_thread restore_thread;

  /* Switch inferior first, so that we're looking at the right target
     stack.  */
  switch_to_inferior_no_thread (thr->inf);

  if (thread_alive (thr))
    {
      switch_to_thread (thr);
      restore_thread.dont_restore ();
      return true;
    }

  return false;
}

/* See gdbthreads.h.  */

void
prune_threads (void)
{
  scoped_restore_current_thread restore_thread;

  for (thread_info *tp : all_threads_safe ())
    {
      switch_to_inferior_no_thread (tp->inf);

      if (!thread_alive (tp))
	delete_thread (tp);
    }
}

/* See gdbthreads.h.  */

void
delete_exited_threads (void)
{
  for (thread_info *tp : all_threads_safe ())
    if (tp->state == THREAD_EXITED)
      delete_thread (tp);
}

/* Return true value if stack temporaries are enabled for the thread
   TP.  */

bool
thread_stack_temporaries_enabled_p (thread_info *tp)
{
  if (tp == NULL)
    return false;
  else
    return tp->stack_temporaries_enabled;
}

/* Push V on to the stack temporaries of the thread with id PTID.  */

void
push_thread_stack_temporary (thread_info *tp, struct value *v)
{
  gdb_assert (tp != NULL && tp->stack_temporaries_enabled);
  tp->stack_temporaries.push_back (v);
}

/* Return true if VAL is among the stack temporaries of the thread
   TP.  Return false otherwise.  */

bool
value_in_thread_stack_temporaries (struct value *val, thread_info *tp)
{
  gdb_assert (tp != NULL && tp->stack_temporaries_enabled);
  for (value *v : tp->stack_temporaries)
    if (v == val)
      return true;

  return false;
}

/* Return the last of the stack temporaries for thread with id PTID.
   Return NULL if there are no stack temporaries for the thread.  */

value *
get_last_thread_stack_temporary (thread_info *tp)
{
  struct value *lastval = NULL;

  gdb_assert (tp != NULL);
  if (!tp->stack_temporaries.empty ())
    lastval = tp->stack_temporaries.back ();

  return lastval;
}

void
thread_change_ptid (process_stratum_target *targ,
		    ptid_t old_ptid, ptid_t new_ptid)
{
  struct inferior *inf;
  struct thread_info *tp;

  /* It can happen that what we knew as the target inferior id
     changes.  E.g, target remote may only discover the remote process
     pid after adding the inferior to GDB's list.  */
  inf = find_inferior_ptid (targ, old_ptid);
  inf->pid = new_ptid.pid ();

  tp = inf->find_thread (old_ptid);
  gdb_assert (tp != nullptr);

  int num_erased = inf->ptid_thread_map.erase (old_ptid);
  gdb_assert (num_erased == 1);

  tp->ptid = new_ptid;
  inf->ptid_thread_map[new_ptid] = tp;

  gdb::observers::thread_ptid_changed.notify (targ, old_ptid, new_ptid);
}

/* See gdbthread.h.  */

void
set_resumed (process_stratum_target *targ, ptid_t ptid, bool resumed)
{
  for (thread_info *tp : all_non_exited_threads (targ, ptid))
    tp->set_resumed (resumed);
}

/* Helper for set_running, that marks one thread either running or
   stopped.  */

static bool
set_running_thread (struct thread_info *tp, bool running)
{
  bool started = false;

  if (running && tp->state == THREAD_STOPPED)
    started = true;
  tp->state = running ? THREAD_RUNNING : THREAD_STOPPED;

  threads_debug_printf ("thread: %s, running? %d%s",
			tp->ptid.to_string ().c_str (), running,
			(started ? " (started)" : ""));

  if (!running)
    {
      /* If the thread is now marked stopped, remove it from
	 the step-over queue, so that we don't try to resume
	 it until the user wants it to.  */
      if (thread_is_in_step_over_chain (tp))
	global_thread_step_over_chain_remove (tp);
    }

  return started;
}

/* Notify interpreters and observers that the target was resumed.  */

static void
notify_target_resumed (ptid_t ptid)
{
  interps_notify_target_resumed (ptid);
  gdb::observers::target_resumed.notify (ptid);

  /* We are about to resume the inferior.  Close all cached BFDs so that
     when the inferior next stops, and GDB regains control, we will spot
     any on-disk changes to the BFDs we are using.  */
  bfd_cache_close_all ();
}

/* See gdbthread.h.  */

void
thread_info::set_running (bool running)
{
  if (set_running_thread (this, running))
    notify_target_resumed (this->ptid);
}

void
set_running (process_stratum_target *targ, ptid_t ptid, bool running)
{
  /* We try not to notify the observer if no thread has actually
     changed the running state -- merely to reduce the number of
     messages to the MI frontend.  A frontend is supposed to handle
     multiple *running notifications just fine.  */
  bool any_started = false;

  for (thread_info *tp : all_non_exited_threads (targ, ptid))
    if (set_running_thread (tp, running))
      any_started = true;

  if (any_started)
    notify_target_resumed (ptid);
}

void
set_executing (process_stratum_target *targ, ptid_t ptid, bool executing)
{
  for (thread_info *tp : all_non_exited_threads (targ, ptid))
    tp->set_executing (executing);

  /* It only takes one running thread to spawn more threads.  */
  if (executing)
    targ->threads_executing = true;
  /* Only clear the flag if the caller is telling us everything is
     stopped.  */
  else if (minus_one_ptid == ptid)
    targ->threads_executing = false;
}

/* See gdbthread.h.  */

bool
threads_are_executing (process_stratum_target *target)
{
  return target->threads_executing;
}

void
set_stop_requested (process_stratum_target *targ, ptid_t ptid, bool stop)
{
  for (thread_info *tp : all_non_exited_threads (targ, ptid))
    tp->stop_requested = stop;

  /* Call the stop requested observer so other components of GDB can
     react to this request.  */
  if (stop)
    gdb::observers::thread_stop_requested.notify (ptid);
}

void
finish_thread_state (process_stratum_target *targ, ptid_t ptid)
{
  bool any_started = false;

  for (thread_info *tp : all_non_exited_threads (targ, ptid))
    if (set_running_thread (tp, tp->executing ()))
      any_started = true;

  if (any_started)
    notify_target_resumed (ptid);
}

/* See gdbthread.h.  */

void
validate_registers_access (void)
{
  /* No selected thread, no registers.  */
  if (inferior_ptid == null_ptid)
    error (_("No thread selected."));

  thread_info *tp = inferior_thread ();

  /* Don't try to read from a dead thread.  */
  if (tp->state == THREAD_EXITED)
    error (_("The current thread has terminated"));

  /* ... or from a spinning thread.  FIXME: This isn't actually fully
     correct.  It'll allow an user-requested access (e.g., "print $pc"
     at the prompt) when a thread is not executing for some internal
     reason, but is marked running from the user's perspective.  E.g.,
     the thread is waiting for its turn in the step-over queue.  */
  if (tp->executing ())
    error (_("Selected thread is running."));
}

/* See gdbthread.h.  */

bool
can_access_registers_thread (thread_info *thread)
{
  /* No thread, no registers.  */
  if (thread == NULL)
    return false;

  /* Don't try to read from a dead thread.  */
  if (thread->state == THREAD_EXITED)
    return false;

  /* ... or from a spinning thread.  FIXME: see validate_registers_access.  */
  if (thread->executing ())
    return false;

  return true;
}

bool
pc_in_thread_step_range (CORE_ADDR pc, struct thread_info *thread)
{
  return (pc >= thread->control.step_range_start
	  && pc < thread->control.step_range_end);
}

/* Helper for print_thread_info.  Returns true if THR should be
   printed.  If REQUESTED_THREADS, a list of GDB ids/ranges, is not
   NULL, only print THR if its ID is included in the list.  GLOBAL_IDS
   is true if REQUESTED_THREADS is list of global IDs, false if a list
   of per-inferior thread ids.  If PID is not -1, only print THR if it
   is a thread from the process PID.  Otherwise, threads from all
   attached PIDs are printed.  If both REQUESTED_THREADS is not NULL
   and PID is not -1, then the thread is printed if it belongs to the
   specified process.  Otherwise, an error is raised.  */

static bool
should_print_thread (const char *requested_threads, int default_inf_num,
		     int global_ids, int pid, struct thread_info *thr)
{
  if (requested_threads != NULL && *requested_threads != '\0')
    {
      int in_list;

      if (global_ids)
	in_list = number_is_in_list (requested_threads, thr->global_num);
      else
	in_list = tid_is_in_list (requested_threads, default_inf_num,
				  thr->inf->num, thr->per_inf_num);
      if (!in_list)
	return false;
    }

  if (pid != -1 && thr->ptid.pid () != pid)
    {
      if (requested_threads != NULL && *requested_threads != '\0')
	error (_("Requested thread not found in requested process"));
      return false;
    }

  if (thr->state == THREAD_EXITED)
    return false;

  return true;
}

/* Return the string to display in "info threads"'s "Target Id"
   column, for TP.  */

static std::string
thread_target_id_str (thread_info *tp)
{
  std::string target_id = target_pid_to_str (tp->ptid);
  const char *extra_info = target_extra_thread_info (tp);
  const char *name = thread_name (tp);

  if (extra_info != nullptr && name != nullptr)
    return string_printf ("%s \"%s\" (%s)", target_id.c_str (), name,
			  extra_info);
  else if (extra_info != nullptr)
    return string_printf ("%s (%s)", target_id.c_str (), extra_info);
  else if (name != nullptr)
    return string_printf ("%s \"%s\"", target_id.c_str (), name);
  else
    return target_id;
}

/* Like print_thread_info, but in addition, GLOBAL_IDS indicates
   whether REQUESTED_THREADS is a list of global or per-inferior
   thread ids.  */

static void
print_thread_info_1 (struct ui_out *uiout, const char *requested_threads,
		     int global_ids, int pid,
		     int show_global_ids)
{
  int default_inf_num = current_inferior ()->num;

  update_thread_list ();

  /* Whether we saw any thread.  */
  bool any_thread = false;
  /* Whether the current thread is exited.  */
  bool current_exited = false;

  thread_info *current_thread = (inferior_ptid != null_ptid
				 ? inferior_thread () : NULL);

  {
    /* For backward compatibility, we make a list for MI.  A table is
       preferable for the CLI, though, because it shows table
       headers.  */
    std::optional<ui_out_emit_list> list_emitter;
    std::optional<ui_out_emit_table> table_emitter;

    /* We'll be switching threads temporarily below.  */
    scoped_restore_current_thread restore_thread;

    if (uiout->is_mi_like_p ())
      list_emitter.emplace (uiout, "threads");
    else
      {
	int n_threads = 0;
	/* The width of the "Target Id" column.  Grown below to
	   accommodate the largest entry.  */
	size_t target_id_col_width = 17;

	for (thread_info *tp : all_threads ())
	  {
	    /* In case REQUESTED_THREADS contains $_thread.  */
	    if (current_thread != nullptr)
	      switch_to_thread (current_thread);

	    if (!should_print_thread (requested_threads, default_inf_num,
				      global_ids, pid, tp))
	      continue;

	    /* Switch inferiors so we're looking at the right
	       target stack.  */
	    switch_to_inferior_no_thread (tp->inf);

	    target_id_col_width
	      = std::max (target_id_col_width,
			  thread_target_id_str (tp).size ());

	    ++n_threads;
	  }

	if (n_threads == 0)
	  {
	    if (requested_threads == NULL || *requested_threads == '\0')
	      uiout->message (_("No threads.\n"));
	    else
	      uiout->message (_("No threads match '%s'.\n"),
			      requested_threads);
	    return;
	  }

	table_emitter.emplace (uiout, show_global_ids ? 5 : 4,
			       n_threads, "threads");

	uiout->table_header (1, ui_left, "current", "");
	uiout->table_header (4, ui_left, "id-in-tg", "Id");
	if (show_global_ids)
	  uiout->table_header (4, ui_left, "id", "GId");
	uiout->table_header (target_id_col_width, ui_left,
			     "target-id", "Target Id");
	uiout->table_header (1, ui_left, "frame", "Frame");
	uiout->table_body ();
      }

    for (inferior *inf : all_inferiors ())
      for (thread_info *tp : inf->threads ())
	{
	  int core;

	  any_thread = true;
	  if (tp == current_thread && tp->state == THREAD_EXITED)
	    current_exited = true;

	  /* In case REQUESTED_THREADS contains $_thread.  */
	  if (current_thread != nullptr)
	    switch_to_thread (current_thread);

	  if (!should_print_thread (requested_threads, default_inf_num,
				    global_ids, pid, tp))
	    continue;

	  ui_out_emit_tuple tuple_emitter (uiout, NULL);

	  if (!uiout->is_mi_like_p ())
	    {
	      if (tp == current_thread)
		uiout->field_string ("current", "*");
	      else
		uiout->field_skip ("current");

	      uiout->field_string ("id-in-tg", print_thread_id (tp));
	    }

	  if (show_global_ids || uiout->is_mi_like_p ())
	    uiout->field_signed ("id", tp->global_num);

	  /* Switch to the thread (and inferior / target).  */
	  switch_to_thread (tp);

	  /* For the CLI, we stuff everything into the target-id field.
	     This is a gross hack to make the output come out looking
	     correct.  The underlying problem here is that ui-out has no
	     way to specify that a field's space allocation should be
	     shared by several fields.  For MI, we do the right thing
	     instead.  */

	  if (uiout->is_mi_like_p ())
	    {
	      uiout->field_string ("target-id", target_pid_to_str (tp->ptid));

	      const char *extra_info = target_extra_thread_info (tp);
	      if (extra_info != nullptr)
		uiout->field_string ("details", extra_info);

	      const char *name = thread_name (tp);
	      if (name != NULL)
		uiout->field_string ("name", name);
	    }
	  else
	    {
	      uiout->field_string ("target-id", thread_target_id_str (tp));
	    }

	  if (tp->state == THREAD_RUNNING)
	    uiout->text ("(running)\n");
	  else
	    {
	      /* The switch above put us at the top of the stack (leaf
		 frame).  */
	      print_stack_frame (get_selected_frame (NULL),
				 /* For MI output, print frame level.  */
				 uiout->is_mi_like_p (),
				 LOCATION, 0);
	    }

	  if (uiout->is_mi_like_p ())
	    {
	      const char *state = "stopped";

	      if (tp->state == THREAD_RUNNING)
		state = "running";
	      uiout->field_string ("state", state);
	    }

	  core = target_core_of_thread (tp->ptid);
	  if (uiout->is_mi_like_p () && core != -1)
	    uiout->field_signed ("core", core);
	}

    /* This end scope restores the current thread and the frame
       selected before the "info threads" command, and it finishes the
       ui-out list or table.  */
  }

  if (pid == -1 && requested_threads == NULL)
    {
      if (uiout->is_mi_like_p () && inferior_ptid != null_ptid)
	uiout->field_signed ("current-thread-id", current_thread->global_num);

      if (inferior_ptid != null_ptid && current_exited)
	uiout->message ("\n\
The current thread <Thread ID %s> has terminated.  See `help thread'.\n",
			print_thread_id (inferior_thread ()));
      else if (any_thread && inferior_ptid == null_ptid)
	uiout->message ("\n\
No selected thread.  See `help thread'.\n");
    }
}

/* See gdbthread.h.  */

void
print_thread_info (struct ui_out *uiout, const char *requested_threads,
		   int pid)
{
  print_thread_info_1 (uiout, requested_threads, 1, pid, 0);
}

/* The options for the "info threads" command.  */

struct info_threads_opts
{
  /* For "-gid".  */
  bool show_global_ids = false;
};

static const gdb::option::option_def info_threads_option_defs[] = {

  gdb::option::flag_option_def<info_threads_opts> {
    "gid",
    [] (info_threads_opts *opts) { return &opts->show_global_ids; },
    N_("Show global thread IDs."),
  },

};

/* Create an option_def_group for the "info threads" options, with
   IT_OPTS as context.  */

static inline gdb::option::option_def_group
make_info_threads_options_def_group (info_threads_opts *it_opts)
{
  return {{info_threads_option_defs}, it_opts};
}

/* Implementation of the "info threads" command.

   Note: this has the drawback that it _really_ switches
	 threads, which frees the frame cache.  A no-side
	 effects info-threads command would be nicer.  */

static void
info_threads_command (const char *arg, int from_tty)
{
  info_threads_opts it_opts;

  auto grp = make_info_threads_options_def_group (&it_opts);
  gdb::option::process_options
    (&arg, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, grp);

  print_thread_info_1 (current_uiout, arg, 0, -1, it_opts.show_global_ids);
}

/* Completer for the "info threads" command.  */

static void
info_threads_command_completer (struct cmd_list_element *ignore,
				completion_tracker &tracker,
				const char *text, const char *word_ignored)
{
  const auto grp = make_info_threads_options_def_group (nullptr);

  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, grp))
    return;

  /* Convenience to let the user know what the option can accept.  */
  if (*text == '\0')
    {
      gdb::option::complete_on_all_options (tracker, grp);
      /* Keep this "ID" in sync with what "help info threads"
	 says.  */
      tracker.add_completion (make_unique_xstrdup ("ID"));
    }
}

/* See gdbthread.h.  */

void
switch_to_thread_no_regs (struct thread_info *thread)
{
  gdb_assert (thread != nullptr);
  threads_debug_printf ("thread = %s", thread->ptid.to_string ().c_str ());

  struct inferior *inf = thread->inf;

  set_current_program_space (inf->pspace);
  set_current_inferior (inf);

  current_thread_ = thread;
  inferior_ptid = current_thread_->ptid;
}

/* See gdbthread.h.  */

void
switch_to_no_thread ()
{
  if (current_thread_ == nullptr)
    return;

  threads_debug_printf ("thread = NONE");

  current_thread_ = nullptr;
  inferior_ptid = null_ptid;
  reinit_frame_cache ();
}

/* See gdbthread.h.  */

void
switch_to_thread (thread_info *thr)
{
  gdb_assert (thr != NULL);

  if (is_current_thread (thr))
    return;

  switch_to_thread_no_regs (thr);

  reinit_frame_cache ();
}

/* See gdbsupport/common-gdbthread.h.  */

void
switch_to_thread (process_stratum_target *proc_target, ptid_t ptid)
{
  thread_info *thr = proc_target->find_thread (ptid);
  switch_to_thread (thr);
}

/* See frame.h.  */

void
scoped_restore_current_thread::restore ()
{
  /* If an entry of thread_info was previously selected, it won't be
     deleted because we've increased its refcount.  The thread represented
     by this thread_info entry may have already exited (due to normal exit,
     detach, etc), so the thread_info.state is THREAD_EXITED.  */
  if (m_thread != NULL
      /* If the previously selected thread belonged to a process that has
	 in the mean time exited (or killed, detached, etc.), then don't revert
	 back to it, but instead simply drop back to no thread selected.  */
      && m_inf->pid != 0)
    switch_to_thread (m_thread.get ());
  else
    switch_to_inferior_no_thread (m_inf.get ());

  /* The running state of the originally selected thread may have
     changed, so we have to recheck it here.  */
  if (inferior_ptid != null_ptid
      && m_was_stopped
      && m_thread->state == THREAD_STOPPED
      && target_has_registers ()
      && target_has_stack ()
      && target_has_memory ())
    restore_selected_frame (m_selected_frame_id, m_selected_frame_level);
}

scoped_restore_current_thread::~scoped_restore_current_thread ()
{
  if (m_dont_restore)
    m_lang.dont_restore ();
  else
    restore ();
}

scoped_restore_current_thread::scoped_restore_current_thread ()
{
  m_inf = inferior_ref::new_reference (current_inferior ());

  if (inferior_ptid != null_ptid)
    {
      m_thread = thread_info_ref::new_reference (inferior_thread ());

      m_was_stopped = m_thread->state == THREAD_STOPPED;
      save_selected_frame (&m_selected_frame_id, &m_selected_frame_level);
    }
}

scoped_restore_current_thread::scoped_restore_current_thread
  (scoped_restore_current_thread &&rhs)
  : m_dont_restore (std::move (rhs.m_dont_restore)),
    m_thread (std::move (rhs.m_thread)),
    m_inf (std::move (rhs.m_inf)),
    m_selected_frame_id (std::move (rhs.m_selected_frame_id)),
    m_selected_frame_level (std::move (rhs.m_selected_frame_level)),
    m_was_stopped (std::move (rhs.m_was_stopped)),
    m_lang (std::move (rhs.m_lang))
{
  /* Deactivate the rhs.  */
  rhs.m_dont_restore = true;
}

/* See gdbthread.h.  */

int
show_thread_that_caused_stop (void)
{
  return highest_thread_num > 1;
}

/* See gdbthread.h.  */

int
show_inferior_qualified_tids (void)
{
  auto inf = inferior_list.begin ();
  if (inf->num != 1)
    return true;
  ++inf;
  return inf != inferior_list.end ();
}

/* See gdbthread.h.  */

const char *
print_thread_id (struct thread_info *thr)
{
  if (show_inferior_qualified_tids ())
    return print_full_thread_id (thr);

  char *s = get_print_cell ();

  gdb_assert (thr != nullptr);
  xsnprintf (s, PRINT_CELL_SIZE, "%d", thr->per_inf_num);
  return s;
}

/* See gdbthread.h.  */

const char *
print_full_thread_id (struct thread_info *thr)
{
  char *s = get_print_cell ();

  gdb_assert (thr != nullptr);
  xsnprintf (s, PRINT_CELL_SIZE, "%d.%d", thr->inf->num, thr->per_inf_num);
  return s;
}

/* Sort an array of struct thread_info pointers by thread ID (first by
   inferior number, and then by per-inferior thread number).  Sorts in
   ascending order.  */

static bool
tp_array_compar_ascending (const thread_info_ref &a, const thread_info_ref &b)
{
  if (a->inf->num != b->inf->num)
    return a->inf->num < b->inf->num;

  return (a->per_inf_num < b->per_inf_num);
}

/* Sort an array of struct thread_info pointers by thread ID (first by
   inferior number, and then by per-inferior thread number).  Sorts in
   descending order.  */

static bool
tp_array_compar_descending (const thread_info_ref &a, const thread_info_ref &b)
{
  if (a->inf->num != b->inf->num)
    return a->inf->num > b->inf->num;

  return (a->per_inf_num > b->per_inf_num);
}

/* See gdbthread.h.  */

void
thread_try_catch_cmd (thread_info *thr, std::optional<int> ada_task,
		      const char *cmd, int from_tty,
		      const qcs_flags &flags)
{
  gdb_assert (is_current_thread (thr));

  /* The thread header is computed before running the command since
     the command can change the inferior, which is not permitted
     by thread_target_id_str.  */
  std::string thr_header;
  if (ada_task.has_value ())
    thr_header = string_printf (_("\nTask ID %d:\n"), *ada_task);
  else
    thr_header = string_printf (_("\nThread %s (%s):\n"),
				print_thread_id (thr),
				thread_target_id_str (thr).c_str ());

  try
    {
      std::string cmd_result;
      execute_command_to_string
	(cmd_result, cmd, from_tty, gdb_stdout->term_out ());
      if (!flags.silent || cmd_result.length () > 0)
	{
	  if (!flags.quiet)
	    gdb_printf ("%s", thr_header.c_str ());
	  gdb_printf ("%s", cmd_result.c_str ());
	}
    }
  catch (const gdb_exception_error &ex)
    {
      if (!flags.silent)
	{
	  if (!flags.quiet)
	    gdb_printf ("%s", thr_header.c_str ());
	  if (flags.cont)
	    gdb_printf ("%s\n", ex.what ());
	  else
	    throw;
	}
    }
}

/* Option definition of "thread apply"'s "-ascending" option.  */

static const gdb::option::flag_option_def<> ascending_option_def = {
  "ascending",
  N_("\
Call COMMAND for all threads in ascending order.\n\
The default is descending order."),
};

/* The qcs command line flags for the "thread apply" commands.  Keep
   this in sync with the "frame apply" commands.  */

using qcs_flag_option_def
  = gdb::option::flag_option_def<qcs_flags>;

static const gdb::option::option_def thr_qcs_flags_option_defs[] = {
  qcs_flag_option_def {
    "q", [] (qcs_flags *opt) { return &opt->quiet; },
    N_("Disables printing the thread information."),
  },

  qcs_flag_option_def {
    "c", [] (qcs_flags *opt) { return &opt->cont; },
    N_("Print any error raised by COMMAND and continue."),
  },

  qcs_flag_option_def {
    "s", [] (qcs_flags *opt) { return &opt->silent; },
    N_("Silently ignore any errors or empty output produced by COMMAND."),
  },
};

/* Create an option_def_group for the "thread apply all" options, with
   ASCENDING and FLAGS as context.  */

static inline std::array<gdb::option::option_def_group, 2>
make_thread_apply_all_options_def_group (bool *ascending,
					 qcs_flags *flags)
{
  return {{
    { {ascending_option_def.def ()}, ascending},
    { {thr_qcs_flags_option_defs}, flags },
  }};
}

/* Create an option_def_group for the "thread apply" options, with
   FLAGS as context.  */

static inline gdb::option::option_def_group
make_thread_apply_options_def_group (qcs_flags *flags)
{
  return {{thr_qcs_flags_option_defs}, flags};
}

/* Apply a GDB command to a list of threads.  List syntax is a whitespace
   separated list of numbers, or ranges, or the keyword `all'.  Ranges consist
   of two numbers separated by a hyphen.  Examples:

   thread apply 1 2 7 4 backtrace       Apply backtrace cmd to threads 1,2,7,4
   thread apply 2-7 9 p foo(1)  Apply p foo(1) cmd to threads 2->7 & 9
   thread apply all x/i $pc   Apply x/i $pc cmd to all threads.  */

static void
thread_apply_all_command (const char *cmd, int from_tty)
{
  bool ascending = false;
  qcs_flags flags;

  auto group = make_thread_apply_all_options_def_group (&ascending,
							&flags);
  gdb::option::process_options
    (&cmd, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group);

  validate_flags_qcs ("thread apply all", &flags);

  if (cmd == NULL || *cmd == '\000')
    error (_("Please specify a command at the end of 'thread apply all'"));

  update_thread_list ();

  int tc = live_threads_count ();
  if (tc != 0)
    {
      /* Save a copy of the thread list and increment each thread's
	 refcount while executing the command in the context of each
	 thread, in case the command is one that wipes threads.  E.g.,
	 detach, kill, disconnect, etc., or even normally continuing
	 over an inferior or thread exit.  */
      std::vector<thread_info_ref> thr_list_cpy;
      thr_list_cpy.reserve (tc);

      for (thread_info *tp : all_non_exited_threads ())
	thr_list_cpy.push_back (thread_info_ref::new_reference (tp));
      gdb_assert (thr_list_cpy.size () == tc);

      auto *sorter = (ascending
		      ? tp_array_compar_ascending
		      : tp_array_compar_descending);
      std::sort (thr_list_cpy.begin (), thr_list_cpy.end (), sorter);

      scoped_restore_current_thread restore_thread;

      for (thread_info_ref &thr : thr_list_cpy)
	if (switch_to_thread_if_alive (thr.get ()))
	  thread_try_catch_cmd (thr.get (), {}, cmd, from_tty, flags);
    }
}

/* Completer for "thread apply [ID list]".  */

static void
thread_apply_command_completer (cmd_list_element *ignore,
				completion_tracker &tracker,
				const char *text, const char * /*word*/)
{
  /* Don't leave this to complete_options because there's an early
     return below.  */
  tracker.set_use_custom_word_point (true);

  tid_range_parser parser;
  parser.init (text, current_inferior ()->num);

  try
    {
      while (!parser.finished ())
	{
	  int inf_num, thr_start, thr_end;

	  if (!parser.get_tid_range (&inf_num, &thr_start, &thr_end))
	    break;

	  if (parser.in_star_range () || parser.in_thread_range ())
	    parser.skip_range ();
	}
    }
  catch (const gdb_exception_error &ex)
    {
      /* get_tid_range throws if it parses a negative number, for
	 example.  But a seemingly negative number may be the start of
	 an option instead.  */
    }

  const char *cmd = parser.cur_tok ();

  if (cmd == text)
    {
      /* No thread ID list yet.  */
      return;
    }

  /* Check if we're past a valid thread ID list already.  */
  if (parser.finished ()
      && cmd > text && !isspace (cmd[-1]))
    return;

  /* We're past the thread ID list, advance word point.  */
  tracker.advance_custom_word_point_by (cmd - text);
  text = cmd;

  const auto group = make_thread_apply_options_def_group (nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group))
    return;

  complete_nested_command_line (tracker, text);
}

/* Completer for "thread apply all".  */

static void
thread_apply_all_command_completer (cmd_list_element *ignore,
				    completion_tracker &tracker,
				    const char *text, const char *word)
{
  const auto group = make_thread_apply_all_options_def_group (nullptr,
							      nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group))
    return;

  complete_nested_command_line (tracker, text);
}

/* Implementation of the "thread apply" command.  */

static void
thread_apply_command (const char *tidlist, int from_tty)
{
  qcs_flags flags;
  const char *cmd = NULL;
  tid_range_parser parser;

  if (tidlist == NULL || *tidlist == '\000')
    error (_("Please specify a thread ID list"));

  parser.init (tidlist, current_inferior ()->num);
  while (!parser.finished ())
    {
      int inf_num, thr_start, thr_end;

      if (!parser.get_tid_range (&inf_num, &thr_start, &thr_end))
	break;
    }

  cmd = parser.cur_tok ();

  auto group = make_thread_apply_options_def_group (&flags);
  gdb::option::process_options
    (&cmd, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group);

  validate_flags_qcs ("thread apply", &flags);

  if (*cmd == '\0')
    error (_("Please specify a command following the thread ID list"));

  if (tidlist == cmd || isdigit (cmd[0]))
    invalid_thread_id_error (cmd);

  scoped_restore_current_thread restore_thread;

  parser.init (tidlist, current_inferior ()->num);
  while (!parser.finished ())
    {
      struct thread_info *tp = NULL;
      struct inferior *inf;
      int inf_num, thr_num;

      parser.get_tid (&inf_num, &thr_num);
      inf = find_inferior_id (inf_num);
      if (inf != NULL)
	tp = find_thread_id (inf, thr_num);

      if (parser.in_star_range ())
	{
	  if (inf == NULL)
	    {
	      warning (_("Unknown inferior %d"), inf_num);
	      parser.skip_range ();
	      continue;
	    }

	  /* No use looking for threads past the highest thread number
	     the inferior ever had.  */
	  if (thr_num >= inf->highest_thread_num)
	    parser.skip_range ();

	  /* Be quiet about unknown threads numbers.  */
	  if (tp == NULL)
	    continue;
	}

      if (tp == NULL)
	{
	  if (show_inferior_qualified_tids () || parser.tid_is_qualified ())
	    warning (_("Unknown thread %d.%d"), inf_num, thr_num);
	  else
	    warning (_("Unknown thread %d"), thr_num);
	  continue;
	}

      if (!switch_to_thread_if_alive (tp))
	{
	  warning (_("Thread %s has terminated."), print_thread_id (tp));
	  continue;
	}

      thread_try_catch_cmd (tp, {}, cmd, from_tty, flags);
    }
}


/* Implementation of the "taas" command.  */

static void
taas_command (const char *cmd, int from_tty)
{
  if (cmd == NULL || *cmd == '\0')
    error (_("Please specify a command to apply on all threads"));
  std::string expanded = std::string ("thread apply all -s ") + cmd;
  execute_command (expanded.c_str (), from_tty);
}

/* Implementation of the "tfaas" command.  */

static void
tfaas_command (const char *cmd, int from_tty)
{
  if (cmd == NULL || *cmd == '\0')
    error (_("Please specify a command to apply on all frames of all threads"));
  std::string expanded
    = std::string ("thread apply all -s -- frame apply all -s ") + cmd;
  execute_command (expanded.c_str (), from_tty);
}

/* Switch to the specified thread, or print the current thread.  */

void
thread_command (const char *tidstr, int from_tty)
{
  if (tidstr == NULL)
    {
      if (inferior_ptid == null_ptid)
	error (_("No thread selected"));

      if (target_has_stack ())
	{
	  struct thread_info *tp = inferior_thread ();

	  if (tp->state == THREAD_EXITED)
	    gdb_printf (_("[Current thread is %s (%s) (exited)]\n"),
			print_thread_id (tp),
			target_pid_to_str (inferior_ptid).c_str ());
	  else
	    gdb_printf (_("[Current thread is %s (%s)]\n"),
			print_thread_id (tp),
			target_pid_to_str (inferior_ptid).c_str ());
	}
      else
	error (_("No stack."));
    }
  else
    {
      ptid_t previous_ptid = inferior_ptid;

      thread_select (tidstr, parse_thread_id (tidstr, NULL));

      /* Print if the thread has not changed, otherwise an event will
	 be sent.  */
      if (inferior_ptid == previous_ptid)
	{
	  print_selected_thread_frame (current_uiout,
				       USER_SELECTED_THREAD
				       | USER_SELECTED_FRAME);
	}
      else
	notify_user_selected_context_changed
	  (USER_SELECTED_THREAD | USER_SELECTED_FRAME);
    }
}

/* Implementation of `thread name'.  */

static void
thread_name_command (const char *arg, int from_tty)
{
  struct thread_info *info;

  if (inferior_ptid == null_ptid)
    error (_("No thread selected"));

  arg = skip_spaces (arg);

  info = inferior_thread ();
  info->set_name (arg != nullptr ? make_unique_xstrdup (arg) : nullptr);
}

/* Find thread ids with a name, target pid, or extra info matching ARG.  */

static void
thread_find_command (const char *arg, int from_tty)
{
  const char *tmp;
  unsigned long match = 0;

  if (arg == NULL || *arg == '\0')
    error (_("Command requires an argument."));

  tmp = re_comp (arg);
  if (tmp != 0)
    error (_("Invalid regexp (%s): %s"), tmp, arg);

  /* We're going to be switching threads.  */
  scoped_restore_current_thread restore_thread;

  update_thread_list ();

  for (thread_info *tp : all_threads ())
    {
      switch_to_inferior_no_thread (tp->inf);

      if (tp->name () != nullptr && re_exec (tp->name ()))
	{
	  gdb_printf (_("Thread %s has name '%s'\n"),
		      print_thread_id (tp), tp->name ());
	  match++;
	}

      tmp = target_thread_name (tp);
      if (tmp != NULL && re_exec (tmp))
	{
	  gdb_printf (_("Thread %s has target name '%s'\n"),
		      print_thread_id (tp), tmp);
	  match++;
	}

      std::string name = target_pid_to_str (tp->ptid);
      if (!name.empty () && re_exec (name.c_str ()))
	{
	  gdb_printf (_("Thread %s has target id '%s'\n"),
		      print_thread_id (tp), name.c_str ());
	  match++;
	}

      tmp = target_extra_thread_info (tp);
      if (tmp != NULL && re_exec (tmp))
	{
	  gdb_printf (_("Thread %s has extra info '%s'\n"),
		      print_thread_id (tp), tmp);
	  match++;
	}
    }
  if (!match)
    gdb_printf (_("No threads match '%s'\n"), arg);
}

/* Print notices when new threads are attached and detached.  */
bool print_thread_events = true;
static void
show_print_thread_events (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Printing of thread events is %s.\n"),
	      value);
}

/* See gdbthread.h.  */

void
thread_select (const char *tidstr, thread_info *tp)
{
  if (!switch_to_thread_if_alive (tp))
    error (_("Thread ID %s has terminated."), tidstr);

  annotate_thread_changed ();

  /* Since the current thread may have changed, see if there is any
     exited thread we can now delete.  */
  delete_exited_threads ();
}

/* Print thread and frame switch command response.  */

void
print_selected_thread_frame (struct ui_out *uiout,
			     user_selected_what selection)
{
  struct thread_info *tp = inferior_thread ();

  if (selection & USER_SELECTED_THREAD)
    {
      if (uiout->is_mi_like_p ())
	{
	  uiout->field_signed ("new-thread-id",
			       inferior_thread ()->global_num);
	}
      else
	{
	  uiout->text ("[Switching to thread ");
	  uiout->field_string ("new-thread-id", print_thread_id (tp));
	  uiout->text (" (");
	  uiout->text (target_pid_to_str (inferior_ptid));
	  uiout->text (")]");
	}
    }

  if (tp->state == THREAD_RUNNING)
    {
      if (selection & USER_SELECTED_THREAD)
	uiout->text ("(running)\n");
    }
  else if (selection & USER_SELECTED_FRAME)
    {
      if (selection & USER_SELECTED_THREAD)
	uiout->text ("\n");

      if (has_stack_frames ())
	print_stack_frame_to_uiout (uiout, get_selected_frame (NULL),
				    1, SRC_AND_LOC, 1);
    }
}

/* Update the 'threads_executing' global based on the threads we know
   about right now.  This is used by infrun to tell whether we should
   pull events out of the current target.  */

static void
update_threads_executing (void)
{
  process_stratum_target *targ = current_inferior ()->process_target ();

  if (targ == NULL)
    return;

  targ->threads_executing = false;

  for (inferior *inf : all_non_exited_inferiors (targ))
    {
      if (!inf->has_execution ())
	continue;

      /* If the process has no threads, then it must be we have a
	 process-exit event pending.  */
      if (inf->thread_list.empty ())
	{
	  targ->threads_executing = true;
	  return;
	}

      for (thread_info *tp : inf->non_exited_threads ())
	{
	  if (tp->executing ())
	    {
	      targ->threads_executing = true;
	      return;
	    }
	}
    }
}

void
update_thread_list (void)
{
  target_update_thread_list ();
  update_threads_executing ();
}

/* See gdbthread.h.  */

const char *
thread_name (thread_info *thread)
{
  /* Use the manually set name if there is one.  */
  const char *name = thread->name ();
  if (name != nullptr)
    return name;

  /* Otherwise, ask the target.  Ensure we query the right target stack.  */
  scoped_restore_current_thread restore_thread;
  if (thread->inf != current_inferior ())
    switch_to_inferior_no_thread (thread->inf);

  return target_thread_name (thread);
}

/* See gdbthread.h.  */

const char *
thread_state_string (enum thread_state state)
{
  switch (state)
    {
    case THREAD_STOPPED:
      return "STOPPED";

    case THREAD_RUNNING:
      return "RUNNING";

    case THREAD_EXITED:
      return "EXITED";
    }

  gdb_assert_not_reached ("unknown thread state");
}

/* Return a new value for the selected thread's id.  Return a value of
   0 if no thread is selected.  If GLOBAL is true, return the thread's
   global number.  Otherwise return the per-inferior number.  */

static struct value *
thread_num_make_value_helper (struct gdbarch *gdbarch, int global)
{
  int int_val;

  if (inferior_ptid == null_ptid)
    int_val = 0;
  else
    {
      thread_info *tp = inferior_thread ();
      if (global)
	int_val = tp->global_num;
      else
	int_val = tp->per_inf_num;
    }

  return value_from_longest (builtin_type (gdbarch)->builtin_int, int_val);
}

/* Return a new value for the selected thread's per-inferior thread
   number.  Return a value of 0 if no thread is selected, or no
   threads exist.  */

static struct value *
thread_id_per_inf_num_make_value (struct gdbarch *gdbarch,
				  struct internalvar *var,
				  void *ignore)
{
  return thread_num_make_value_helper (gdbarch, 0);
}

/* Return a new value for the selected thread's global id.  Return a
   value of 0 if no thread is selected, or no threads exist.  */

static struct value *
global_thread_id_make_value (struct gdbarch *gdbarch, struct internalvar *var,
			     void *ignore)
{
  return thread_num_make_value_helper (gdbarch, 1);
}

/* Return a new value for the number of non-exited threads in the current
   inferior.  If there are no threads in the current inferior return a
   value of 0.  */

static struct value *
inferior_thread_count_make_value (struct gdbarch *gdbarch,
				  struct internalvar *var, void *ignore)
{
  int int_val = 0;

  update_thread_list ();

  if (inferior_ptid != null_ptid)
    int_val = current_inferior ()->non_exited_threads ().size ();

  return value_from_longest (builtin_type (gdbarch)->builtin_int, int_val);
}

/* Commands with a prefix of `thread'.  */
struct cmd_list_element *thread_cmd_list = NULL;

/* Implementation of `thread' variable.  */

static const struct internalvar_funcs thread_funcs =
{
  thread_id_per_inf_num_make_value,
  NULL,
};

/* Implementation of `gthread' variable.  */

static const struct internalvar_funcs gthread_funcs =
{
  global_thread_id_make_value,
  NULL,
};

/* Implementation of `_inferior_thread_count` convenience variable.  */

static const struct internalvar_funcs inferior_thread_count_funcs =
{
  inferior_thread_count_make_value,
  NULL,
};

void _initialize_thread ();
void
_initialize_thread ()
{
  static struct cmd_list_element *thread_apply_list = NULL;
  cmd_list_element *c;

  const auto info_threads_opts = make_info_threads_options_def_group (nullptr);

  /* Note: keep this "ID" in sync with what "info threads [TAB]"
     suggests.  */
  static std::string info_threads_help
    = gdb::option::build_help (_("\
Display currently known threads.\n\
Usage: info threads [OPTION]... [ID]...\n\
If ID is given, it is a space-separated list of IDs of threads to display.\n\
Otherwise, all threads are displayed.\n\
\n\
Options:\n\
%OPTIONS%"),
			       info_threads_opts);

  c = add_info ("threads", info_threads_command, info_threads_help.c_str ());
  set_cmd_completer_handle_brkchars (c, info_threads_command_completer);

  cmd_list_element *thread_cmd
    = add_prefix_cmd ("thread", class_run, thread_command, _("\
Use this command to switch between threads.\n\
The new thread ID must be currently known."),
		      &thread_cmd_list, 1, &cmdlist);

  add_com_alias ("t", thread_cmd, class_run, 1);

#define THREAD_APPLY_OPTION_HELP "\
Prints per-inferior thread number and target system's thread id\n\
followed by COMMAND output.\n\
\n\
By default, an error raised during the execution of COMMAND\n\
aborts \"thread apply\".\n\
\n\
Options:\n\
%OPTIONS%"

  const auto thread_apply_opts = make_thread_apply_options_def_group (nullptr);

  static std::string thread_apply_help = gdb::option::build_help (_("\
Apply a command to a list of threads.\n\
Usage: thread apply ID... [OPTION]... COMMAND\n\
ID is a space-separated list of IDs of threads to apply COMMAND on.\n"
THREAD_APPLY_OPTION_HELP),
			       thread_apply_opts);

  c = add_prefix_cmd ("apply", class_run, thread_apply_command,
		      thread_apply_help.c_str (),
		      &thread_apply_list, 1,
		      &thread_cmd_list);
  set_cmd_completer_handle_brkchars (c, thread_apply_command_completer);

  const auto thread_apply_all_opts
    = make_thread_apply_all_options_def_group (nullptr, nullptr);

  static std::string thread_apply_all_help = gdb::option::build_help (_("\
Apply a command to all threads.\n\
\n\
Usage: thread apply all [OPTION]... COMMAND\n"
THREAD_APPLY_OPTION_HELP),
			       thread_apply_all_opts);

  c = add_cmd ("all", class_run, thread_apply_all_command,
	       thread_apply_all_help.c_str (),
	       &thread_apply_list);
  set_cmd_completer_handle_brkchars (c, thread_apply_all_command_completer);

  c = add_com ("taas", class_run, taas_command, _("\
Apply a command to all threads (ignoring errors and empty output).\n\
Usage: taas [OPTION]... COMMAND\n\
shortcut for 'thread apply all -s [OPTION]... COMMAND'\n\
See \"help thread apply all\" for available options."));
  set_cmd_completer_handle_brkchars (c, thread_apply_all_command_completer);

  c = add_com ("tfaas", class_run, tfaas_command, _("\
Apply a command to all frames of all threads (ignoring errors and empty output).\n\
Usage: tfaas [OPTION]... COMMAND\n\
shortcut for 'thread apply all -s -- frame apply all -s [OPTION]... COMMAND'\n\
See \"help frame apply all\" for available options."));
  set_cmd_completer_handle_brkchars (c, frame_apply_all_cmd_completer);

  add_cmd ("name", class_run, thread_name_command,
	   _("Set the current thread's name.\n\
Usage: thread name [NAME]\n\
If NAME is not given, then any existing name is removed."), &thread_cmd_list);

  add_cmd ("find", class_run, thread_find_command, _("\
Find threads that match a regular expression.\n\
Usage: thread find REGEXP\n\
Will display thread ids whose name, target ID, or extra info matches REGEXP."),
	   &thread_cmd_list);

  add_setshow_boolean_cmd ("thread-events", no_class,
			   &print_thread_events, _("\
Set printing of thread events (such as thread start and exit)."), _("\
Show printing of thread events (such as thread start and exit)."), NULL,
			   NULL,
			   show_print_thread_events,
			   &setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("threads", class_maintenance, &debug_threads, _("\
Set thread debugging."), _("\
Show thread debugging."), _("\
When on messages about thread creation and deletion are printed."),
			   nullptr,
			   show_debug_threads,
			   &setdebuglist, &showdebuglist);

  create_internalvar_type_lazy ("_thread", &thread_funcs, NULL);
  create_internalvar_type_lazy ("_gthread", &gthread_funcs, NULL);
  create_internalvar_type_lazy ("_inferior_thread_count",
				&inferior_thread_count_funcs, NULL);
}
