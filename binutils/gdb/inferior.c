/* Multi-process control for GDB, the GNU debugger.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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
#include "exec.h"
#include "inferior.h"
#include "target.h"
#include "command.h"
#include "completer.h"
#include "gdbcmd.h"
#include "gdbthread.h"
#include "ui-out.h"
#include "observable.h"
#include "gdbcore.h"
#include "symfile.h"
#include "gdbsupport/environ.h"
#include "cli/cli-utils.h"
#include "arch-utils.h"
#include "target-descriptions.h"
#include "target-connection.h"
#include "readline/tilde.h"
#include "progspace-and-thread.h"
#include "gdbsupport/buildargv.h"
#include "cli/cli-style.h"
#include "interps.h"

intrusive_list<inferior> inferior_list;
static int highest_inferior_num;

/* See inferior.h.  */
bool print_inferior_events = true;

/* The Current Inferior.  This is a strong reference.  I.e., whenever
   an inferior is the current inferior, its refcount is
   incremented.  */
static inferior_ref current_inferior_;

struct inferior*
current_inferior (void)
{
  return current_inferior_.get ();
}

void
set_current_inferior (struct inferior *inf)
{
  /* There's always an inferior.  */
  gdb_assert (inf != NULL);

  current_inferior_ = inferior_ref::new_reference (inf);
}

private_inferior::~private_inferior () = default;

inferior::~inferior ()
{
  /* Before the inferior is deleted, all target_ops should be popped from
     the target stack, this leaves just the dummy_target behind.  If this
     is not done, then any target left in the target stack will be left
     with an artificially high reference count.  As the dummy_target is
     still on the target stack then we are about to loose a reference to
     that target, leaving its reference count artificially high.  However,
     this is not critical as the dummy_target is a singleton.  */
  gdb_assert (m_target_stack.top ()->stratum () == dummy_stratum);

  m_continuations.clear ();
}

inferior::inferior (int pid_)
  : num (++highest_inferior_num),
    pid (pid_),
    environment (gdb_environ::from_host_environ ())
{
  m_target_stack.push (get_dummy_target ());
}

/* See inferior.h.  */

int
inferior::unpush_target (struct target_ops *t)
{
  /* If unpushing the process stratum target from the inferior while threads
     exist in the inferior, ensure that we don't leave any threads of the
     inferior in the target's "resumed with pending wait status" list.

     See also the comment in set_thread_exited.  */
  if (t->stratum () == process_stratum)
    {
      process_stratum_target *proc_target = as_process_stratum_target (t);

      for (thread_info *thread : this->non_exited_threads ())
	proc_target->maybe_remove_resumed_with_pending_wait_status (thread);
    }

  return m_target_stack.unpush (t);
}

/* See inferior.h.  */

void
inferior::unpush_target_and_assert (struct target_ops *target)
{
  gdb_assert (current_inferior () == this);

  if (!unpush_target (target))
    internal_error ("pop_all_targets couldn't find target %s\n",
		    target->shortname ());
}

/* See inferior.h.  */

void
inferior::pop_all_targets_above (enum strata stratum)
{
  /* Unpushing a target might cause it to close.  Some targets currently
     rely on the current_inferior being set for their ::close method, so we
     temporarily switch inferior now.  */
  scoped_restore_current_pspace_and_thread restore_pspace_and_thread;
  switch_to_inferior_no_thread (this);

  while (top_target ()->stratum () > stratum)
    unpush_target_and_assert (top_target ());
}

/* See inferior.h.  */

void
inferior::pop_all_targets_at_and_above (enum strata stratum)
{
  /* Unpushing a target might cause it to close.  Some targets currently
     rely on the current_inferior being set for their ::close method, so we
     temporarily switch inferior now.  */
  scoped_restore_current_pspace_and_thread restore_pspace_and_thread;
  switch_to_inferior_no_thread (this);

  while (top_target ()->stratum () >= stratum)
    unpush_target_and_assert (top_target ());
}

void
inferior::set_tty (std::string terminal_name)
{
  m_terminal = std::move (terminal_name);
}

const std::string &
inferior::tty ()
{
  return m_terminal;
}

/* See inferior.h.  */

void
inferior::set_args (gdb::array_view<char * const> args)
{
  set_args (construct_inferior_arguments (args));
}

void
inferior::set_arch (gdbarch *arch)
{
  gdb_assert (arch != nullptr);
  gdb_assert (gdbarch_initialized_p (arch));
  m_gdbarch = arch;

  process_stratum_target *proc_target = this->process_target ();
  if (proc_target != nullptr)
    registers_changed_ptid (proc_target, ptid_t (this->pid));
}

void
inferior::add_continuation (std::function<void ()> &&cont)
{
  m_continuations.emplace_front (std::move (cont));
}

void
inferior::do_all_continuations ()
{
  while (!m_continuations.empty ())
    {
      auto iter = m_continuations.begin ();
      (*iter) ();
      m_continuations.erase (iter);
    }
}

/* Notify interpreters and observers that inferior INF was added.  */

static void
notify_inferior_added (inferior *inf)
{
  interps_notify_inferior_added (inf);
  gdb::observers::inferior_added.notify (inf);
}

struct inferior *
add_inferior_silent (int pid)
{
  inferior *inf = new inferior (pid);

  inferior_list.push_back (*inf);

  notify_inferior_added (inf);

  if (pid != 0)
    inferior_appeared (inf, pid);

  return inf;
}

struct inferior *
add_inferior (int pid)
{
  struct inferior *inf = add_inferior_silent (pid);

  if (print_inferior_events)
    {
      if (pid != 0)
	gdb_printf (_("[New inferior %d (%s)]\n"),
		    inf->num,
		    target_pid_to_str (ptid_t (pid)).c_str ());
      else
	gdb_printf (_("[New inferior %d]\n"), inf->num);
    }

  return inf;
}

/* See inferior.h.  */

thread_info *
inferior::find_thread (ptid_t ptid)
{
  auto it = this->ptid_thread_map.find (ptid);
  if (it != this->ptid_thread_map.end ())
    return it->second;
  else
    return nullptr;
}

/* See inferior.h.  */

void
inferior::clear_thread_list ()
{
  thread_list.clear_and_dispose ([=] (thread_info *thr)
    {
      threads_debug_printf ("deleting thread %s",
			    thr->ptid.to_string ().c_str ());
      set_thread_exited (thr, {}, true /* silent */);
      if (thr->deletable ())
	delete thr;
    });
  ptid_thread_map.clear ();
}

/* Notify interpreters and observers that inferior INF was removed.  */

static void
notify_inferior_removed (inferior *inf)
{
  interps_notify_inferior_removed (inf);
  gdb::observers::inferior_removed.notify (inf);
}

void
delete_inferior (struct inferior *inf)
{
  inf->clear_thread_list ();

  auto it = inferior_list.iterator_to (*inf);
  inferior_list.erase (it);

  notify_inferior_removed (inf);

  /* Pop all targets now, this ensures that inferior::unpush is called
     correctly.  As pop_all_targets ends up making a temporary switch to
     inferior INF then we need to make this call before we delete the
     program space, which we do below.  */
  inf->pop_all_targets ();

  /* If this program space is rendered useless, remove it. */
  if (inf->pspace->empty ())
    delete inf->pspace;

  delete inf;
}

/* Notify interpreters and observers that inferior INF disappeared.  */

static void
notify_inferior_disappeared (inferior *inf)
{
  interps_notify_inferior_disappeared (inf);
  gdb::observers::inferior_exit.notify (inf);
}

/* See inferior.h.  */

void
exit_inferior (struct inferior *inf)
{
  inf->clear_thread_list ();

  notify_inferior_disappeared (inf);

  inf->pid = 0;
  inf->fake_pid_p = false;
  inf->priv = NULL;

  if (inf->vfork_parent != NULL)
    {
      inf->vfork_parent->vfork_child = NULL;
      inf->vfork_parent = NULL;
    }
  if (inf->vfork_child != NULL)
    {
      inf->vfork_child->vfork_parent = NULL;
      inf->vfork_child = NULL;
    }

  inf->pending_detach = false;
  /* Reset it.  */
  inf->control = inferior_control_state (NO_STOP_QUIETLY);

  /* Clear the register cache and the frame cache.  */
  registers_changed ();
  reinit_frame_cache ();
}

/* See inferior.h.  */

void
detach_inferior (inferior *inf)
{
  /* Save the pid, since exit_inferior will reset it.  */
  int pid = inf->pid;

  exit_inferior (inf);

  if (print_inferior_events)
    gdb_printf (_("[Inferior %d (%s) detached]\n"),
		inf->num,
		target_pid_to_str (ptid_t (pid)).c_str ());
}

/* Notify interpreters and observers that inferior INF appeared.  */

static void
notify_inferior_appeared (inferior *inf)
{
  interps_notify_inferior_appeared (inf);
  gdb::observers::inferior_appeared.notify (inf);
}

void
inferior_appeared (struct inferior *inf, int pid)
{
  /* If this is the first inferior with threads, reset the global
     thread id.  */
  delete_exited_threads ();
  if (!any_thread_p ())
    init_thread_list ();

  inf->pid = pid;
  inf->has_exit_code = false;
  inf->exit_code = 0;

  notify_inferior_appeared (inf);
}

struct inferior *
find_inferior_id (int num)
{
  for (inferior *inf : all_inferiors ())
    if (inf->num == num)
      return inf;

  return NULL;
}

struct inferior *
find_inferior_pid (process_stratum_target *targ, int pid)
{
  /* Looking for inferior pid == 0 is always wrong, and indicative of
     a bug somewhere else.  There may be more than one with pid == 0,
     for instance.  */
  gdb_assert (pid != 0);

  for (inferior *inf : all_inferiors (targ))
    if (inf->pid == pid)
      return inf;

  return NULL;
}

/* See inferior.h */

struct inferior *
find_inferior_ptid (process_stratum_target *targ, ptid_t ptid)
{
  return find_inferior_pid (targ, ptid.pid ());
}

/* See inferior.h.  */

struct inferior *
find_inferior_for_program_space (struct program_space *pspace)
{
  struct inferior *cur_inf = current_inferior ();

  if (cur_inf->pspace == pspace)
    return cur_inf;

  for (inferior *inf : all_inferiors ())
    if (inf->pspace == pspace)
      return inf;

  return NULL;
}

int
have_inferiors (void)
{
  for (inferior *inf ATTRIBUTE_UNUSED : all_non_exited_inferiors ())
    return 1;

  return 0;
}

/* Return the number of live inferiors.  We account for the case
   where an inferior might have a non-zero pid but no threads, as
   in the middle of a 'mourn' operation.  */

int
number_of_live_inferiors (process_stratum_target *proc_target)
{
  int num_inf = 0;

  for (inferior *inf : all_non_exited_inferiors (proc_target))
    if (inf->has_execution ())
      for (thread_info *tp ATTRIBUTE_UNUSED : inf->non_exited_threads ())
	{
	  /* Found a live thread in this inferior, go to the next
	     inferior.  */
	  ++num_inf;
	  break;
	}

  return num_inf;
}

/* Return true if there is at least one live inferior.  */

int
have_live_inferiors (void)
{
  return number_of_live_inferiors (NULL) > 0;
}

/* Prune away any unused inferiors, and then prune away no longer used
   program spaces.  */

void
prune_inferiors (void)
{
  for (inferior *inf : all_inferiors_safe ())
    {
      if (!inf->deletable ()
	  || !inf->removable
	  || inf->pid != 0)
	continue;

      delete_inferior (inf);
    }
}

/* Simply returns the count of inferiors.  */

int
number_of_inferiors (void)
{
  auto rng = all_inferiors ();
  return std::distance (rng.begin (), rng.end ());
}

/* Converts an inferior process id to a string.  Like
   target_pid_to_str, but special cases the null process.  */

static std::string
inferior_pid_to_str (int pid)
{
  if (pid != 0)
    return target_pid_to_str (ptid_t (pid));
  else
    return _("<null>");
}

/* See inferior.h.  */

void
print_selected_inferior (struct ui_out *uiout)
{
  struct inferior *inf = current_inferior ();
  const char *filename = inf->pspace->exec_filename.get ();

  if (filename == NULL)
    filename = _("<noexec>");

  uiout->message (_("[Switching to inferior %d [%s] (%s)]\n"),
		  inf->num, inferior_pid_to_str (inf->pid).c_str (), filename);
}

/* Helper for print_inferior.  Returns the 'connection-id' string for
   PROC_TARGET.  */

static std::string
uiout_field_connection (process_stratum_target *proc_target)
{
  if (proc_target == NULL)
    return {};
  else
    {
      std::string conn_str = make_target_connection_string (proc_target);
      return string_printf ("%d (%s)", proc_target->connection_number,
			    conn_str.c_str ());
    }
}

/* Prints the list of inferiors and their details on UIOUT.  This is a
   version of 'info_inferior_command' suitable for use from MI.

   If REQUESTED_INFERIORS is not NULL, it's a list of GDB ids of the
   inferiors that should be printed.  Otherwise, all inferiors are
   printed.  */

static void
print_inferior (struct ui_out *uiout, const char *requested_inferiors)
{
  int inf_count = 0;
  size_t connection_id_len = 20;

  /* Compute number of inferiors we will print.  */
  for (inferior *inf : all_inferiors ())
    {
      if (!number_is_in_list (requested_inferiors, inf->num))
	continue;

      std::string conn = uiout_field_connection (inf->process_target ());
      if (connection_id_len < conn.size ())
	connection_id_len = conn.size ();

      ++inf_count;
    }

  if (inf_count == 0)
    {
      uiout->message ("No inferiors.\n");
      return;
    }

  ui_out_emit_table table_emitter (uiout, 5, inf_count, "inferiors");
  uiout->table_header (1, ui_left, "current", "");
  uiout->table_header (4, ui_left, "number", "Num");
  uiout->table_header (17, ui_left, "target-id", "Description");
  uiout->table_header (connection_id_len, ui_left,
		       "connection-id", "Connection");
  uiout->table_header (17, ui_left, "exec", "Executable");

  uiout->table_body ();

  /* Restore the current thread after the loop because we switch the
     inferior in the loop.  */
  scoped_restore_current_pspace_and_thread restore_pspace_thread;
  inferior *current_inf = current_inferior ();
  for (inferior *inf : all_inferiors ())
    {
      if (!number_is_in_list (requested_inferiors, inf->num))
	continue;

      ui_out_emit_tuple tuple_emitter (uiout, NULL);

      if (inf == current_inf)
	uiout->field_string ("current", "*");
      else
	uiout->field_skip ("current");

      uiout->field_signed ("number", inf->num);

      /* Because target_pid_to_str uses the current inferior,
	 switch the inferior.  */
      switch_to_inferior_no_thread (inf);

      uiout->field_string ("target-id", inferior_pid_to_str (inf->pid));

      std::string conn = uiout_field_connection (inf->process_target ());
      uiout->field_string ("connection-id", conn);

      if (inf->pspace->exec_filename != nullptr)
	uiout->field_string ("exec", inf->pspace->exec_filename.get (),
			     file_name_style.style ());
      else
	uiout->field_skip ("exec");

      /* Print extra info that isn't really fit to always present in
	 tabular form.  Currently we print the vfork parent/child
	 relationships, if any.  */
      if (inf->vfork_parent)
	{
	  uiout->text (_("\n\tis vfork child of inferior "));
	  uiout->field_signed ("vfork-parent", inf->vfork_parent->num);
	}
      if (inf->vfork_child)
	{
	  uiout->text (_("\n\tis vfork parent of inferior "));
	  uiout->field_signed ("vfork-child", inf->vfork_child->num);
	}

      uiout->text ("\n");
    }
}

static void
detach_inferior_command (const char *args, int from_tty)
{
  if (!args || !*args)
    error (_("Requires argument (inferior id(s) to detach)"));

  scoped_restore_current_thread restore_thread;

  number_or_range_parser parser (args);
  while (!parser.finished ())
    {
      int num = parser.get_number ();

      inferior *inf = find_inferior_id (num);
      if (inf == NULL)
	{
	  warning (_("Inferior ID %d not known."), num);
	  continue;
	}

      if (inf->pid == 0)
	{
	  warning (_("Inferior ID %d is not running."), num);
	  continue;
	}

      thread_info *tp = any_thread_of_inferior (inf);
      if (tp == NULL)
	{
	  warning (_("Inferior ID %d has no threads."), num);
	  continue;
	}

      switch_to_thread (tp);

      detach_command (NULL, from_tty);
    }
}

static void
kill_inferior_command (const char *args, int from_tty)
{
  if (!args || !*args)
    error (_("Requires argument (inferior id(s) to kill)"));

  scoped_restore_current_thread restore_thread;

  number_or_range_parser parser (args);
  while (!parser.finished ())
    {
      int num = parser.get_number ();

      inferior *inf = find_inferior_id (num);
      if (inf == NULL)
	{
	  warning (_("Inferior ID %d not known."), num);
	  continue;
	}

      if (inf->pid == 0)
	{
	  warning (_("Inferior ID %d is not running."), num);
	  continue;
	}

      thread_info *tp = any_thread_of_inferior (inf);
      if (tp == NULL)
	{
	  warning (_("Inferior ID %d has no threads."), num);
	  continue;
	}

      switch_to_thread (tp);

      target_kill ();
    }
}

/* See inferior.h.  */

void
switch_to_inferior_no_thread (inferior *inf)
{
  set_current_inferior (inf);
  switch_to_no_thread ();
  set_current_program_space (inf->pspace);
}

/* See regcache.h.  */

std::optional<scoped_restore_current_thread>
maybe_switch_inferior (inferior *inf)
{
  std::optional<scoped_restore_current_thread> maybe_restore_thread;
  if (inf != current_inferior ())
    {
      maybe_restore_thread.emplace ();
      switch_to_inferior_no_thread (inf);
    }

  return maybe_restore_thread;
}

static void
inferior_command (const char *args, int from_tty)
{
  struct inferior *inf;
  int num;

  if (args == nullptr)
    {
      inf = current_inferior ();
      gdb_assert (inf != nullptr);
      const char *filename = inf->pspace->exec_filename.get ();

      if (filename == nullptr)
	filename = _("<noexec>");

      gdb_printf (_("[Current inferior is %d [%s] (%s)]\n"),
		  inf->num, inferior_pid_to_str (inf->pid).c_str (),
		  filename);
    }
  else
    {
      num = parse_and_eval_long (args);

      inf = find_inferior_id (num);
      if (inf == NULL)
	error (_("Inferior ID %d not known."), num);

      if (inf->pid != 0)
	{
	  if (inf != current_inferior ())
	    {
	      thread_info *tp = any_thread_of_inferior (inf);
	      if (tp == NULL)
		error (_("Inferior has no threads."));

	      switch_to_thread (tp);
	    }

	  notify_user_selected_context_changed
	    (USER_SELECTED_INFERIOR
	     | USER_SELECTED_THREAD
	     | USER_SELECTED_FRAME);
	}
      else
	{
	  switch_to_inferior_no_thread (inf);

	  notify_user_selected_context_changed
	    (USER_SELECTED_INFERIOR);
	}
    }
}

/* Print information about currently known inferiors.  */

static void
info_inferiors_command (const char *args, int from_tty)
{
  print_inferior (current_uiout, args);
}

/* remove-inferior ID */

static void
remove_inferior_command (const char *args, int from_tty)
{
  if (args == NULL || *args == '\0')
    error (_("Requires an argument (inferior id(s) to remove)"));

  number_or_range_parser parser (args);
  while (!parser.finished ())
    {
      int num = parser.get_number ();
      struct inferior *inf = find_inferior_id (num);

      if (inf == NULL)
	{
	  warning (_("Inferior ID %d not known."), num);
	  continue;
	}

      if (!inf->deletable ())
	{
	  warning (_("Can not remove current inferior %d."), num);
	  continue;
	}
    
      if (inf->pid != 0)
	{
	  warning (_("Can not remove active inferior %d."), num);
	  continue;
	}

      delete_inferior (inf);
    }
}

struct inferior *
add_inferior_with_spaces (void)
{
  struct program_space *pspace;
  struct inferior *inf;

  /* If all inferiors share an address space on this system, this
     doesn't really return a new address space; otherwise, it
     really does.  */
  pspace = new program_space (maybe_new_address_space ());
  inf = add_inferior (0);
  inf->pspace = pspace;
  inf->aspace = pspace->aspace;

  /* Setup the inferior's initial arch, based on information obtained
     from the global "set ..." options.  */
  gdbarch_info info;
  inf->set_arch (gdbarch_find_by_info (info));
  /* The "set ..." options reject invalid settings, so we should
     always have a valid arch by now.  */
  gdb_assert (inf->arch () != nullptr);

  return inf;
}

/* See inferior.h.  */

void
switch_to_inferior_and_push_target (inferior *new_inf,
				    bool no_connection, inferior *org_inf)
{
  process_stratum_target *proc_target = org_inf->process_target ();

  /* Switch over temporarily, while reading executable and
     symbols.  */
  switch_to_inferior_no_thread (new_inf);

  /* Reuse the target for new inferior.  */
  if (!no_connection && proc_target != NULL)
    {
      new_inf->push_target (proc_target);
      gdb_printf (_("Added inferior %d on connection %d (%s)\n"),
		  new_inf->num,
		  proc_target->connection_number,
		  make_target_connection_string (proc_target).c_str ());
    }
  else
    gdb_printf (_("Added inferior %d\n"), new_inf->num);
}

/* add-inferior [-copies N] [-exec FILENAME] [-no-connection] */

static void
add_inferior_command (const char *args, int from_tty)
{
  int i, copies = 1;
  gdb::unique_xmalloc_ptr<char> exec;
  symfile_add_flags add_flags = 0;
  bool no_connection = false;

  if (from_tty)
    add_flags |= SYMFILE_VERBOSE;

  if (args)
    {
      gdb_argv built_argv (args);

      for (char **argv = built_argv.get (); *argv != NULL; argv++)
	{
	  if (**argv == '-')
	    {
	      if (strcmp (*argv, "-copies") == 0)
		{
		  ++argv;
		  if (!*argv)
		    error (_("No argument to -copies"));
		  copies = parse_and_eval_long (*argv);
		}
	      else if (strcmp (*argv, "-no-connection") == 0)
		no_connection = true;
	      else if (strcmp (*argv, "-exec") == 0)
		{
		  ++argv;
		  if (!*argv)
		    error (_("No argument to -exec"));
		  exec.reset (tilde_expand (*argv));
		}
	    }
	  else
	    error (_("Invalid argument"));
	}
    }

  inferior *orginf = current_inferior ();

  scoped_restore_current_pspace_and_thread restore_pspace_thread;

  for (i = 0; i < copies; ++i)
    {
      inferior *inf = add_inferior_with_spaces ();

      switch_to_inferior_and_push_target (inf, no_connection, orginf);

      if (exec != NULL)
	{
	  exec_file_attach (exec.get (), from_tty);
	  symbol_file_add_main (exec.get (), add_flags);
	}
    }
}

/* clone-inferior [-copies N] [ID] [-no-connection] */

static void
clone_inferior_command (const char *args, int from_tty)
{
  int i, copies = 1;
  struct inferior *orginf = NULL;
  bool no_connection = false;

  if (args)
    {
      gdb_argv built_argv (args);

      char **argv = built_argv.get ();
      for (; *argv != NULL; argv++)
	{
	  if (**argv == '-')
	    {
	      if (strcmp (*argv, "-copies") == 0)
		{
		  ++argv;
		  if (!*argv)
		    error (_("No argument to -copies"));
		  copies = parse_and_eval_long (*argv);

		  if (copies < 0)
		    error (_("Invalid copies number"));
		}
	      else if (strcmp (*argv, "-no-connection") == 0)
		no_connection = true;
	    }
	  else
	    {
	      if (orginf == NULL)
		{
		  int num;

		  /* The first non-option (-) argument specified the
		     program space ID.  */
		  num = parse_and_eval_long (*argv);
		  orginf = find_inferior_id (num);

		  if (orginf == NULL)
		    error (_("Inferior ID %d not known."), num);
		  continue;
		}
	      else
		error (_("Invalid argument"));
	    }
	}
    }

  /* If no inferior id was specified, then the user wants to clone the
     current inferior.  */
  if (orginf == NULL)
    orginf = current_inferior ();

  scoped_restore_current_pspace_and_thread restore_pspace_thread;

  for (i = 0; i < copies; ++i)
    {
      struct program_space *pspace;
      struct inferior *inf;

      /* If all inferiors share an address space on this system, this
	 doesn't really return a new address space; otherwise, it
	 really does.  */
      pspace = new program_space (maybe_new_address_space ());
      inf = add_inferior (0);
      inf->pspace = pspace;
      inf->aspace = pspace->aspace;
      inf->set_arch (orginf->arch ());

      switch_to_inferior_and_push_target (inf, no_connection, orginf);

      /* If the original inferior had a user specified target
	 description, make the clone use it too.  */
      if (inf->tdesc_info.from_user_p ())
	inf->tdesc_info = orginf->tdesc_info;

      clone_program_space (pspace, orginf->pspace);

      /* Copy properties from the original inferior to the new one.  */
      inf->set_args (orginf->args ());
      inf->set_cwd (orginf->cwd ());
      inf->set_tty (orginf->tty ());
      for (const std::string &set_var : orginf->environment.user_set_env ())
	{
	  /* set_var has the form NAME=value.  Split on the first '='.  */
	  const std::string::size_type pos = set_var.find ('=');
	  gdb_assert (pos != std::string::npos);
	  const std::string varname = set_var.substr (0, pos);
	  inf->environment.set
	    (varname.c_str (), orginf->environment.get (varname.c_str ()));
	}
      for (const std::string &unset_var
	   : orginf->environment.user_unset_env ())
	inf->environment.unset (unset_var.c_str ());

      gdb::observers::inferior_cloned.notify (orginf, inf);
    }
}

/* Print notices when new inferiors are created and die.  */
static void
show_print_inferior_events (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Printing of inferior events is %s.\n"), value);
}

/* Return a new value for the selected inferior's id.  */

static struct value *
inferior_id_make_value (struct gdbarch *gdbarch, struct internalvar *var,
			void *ignore)
{
  struct inferior *inf = current_inferior ();

  return value_from_longest (builtin_type (gdbarch)->builtin_int, inf->num);
}

/* Implementation of `$_inferior' variable.  */

static const struct internalvar_funcs inferior_funcs =
{
  inferior_id_make_value,
  NULL,
};



void
initialize_inferiors (void)
{
  struct cmd_list_element *c = NULL;

  /* There's always one inferior.  Note that this function isn't an
     automatic _initialize_foo function, since other _initialize_foo
     routines may need to install their per-inferior data keys.  We
     can only allocate an inferior when all those modules have done
     that.  Do this after initialize_progspace, due to the
     current_program_space reference.  */
  set_current_inferior (add_inferior_silent (0));
  current_inferior_->pspace = current_program_space;
  current_inferior_->aspace = current_program_space->aspace;
  /* The architecture will be initialized shortly, by
     initialize_current_architecture.  */

  add_info ("inferiors", info_inferiors_command,
	    _("Print a list of inferiors being managed.\n\
Usage: info inferiors [ID]...\n\
If IDs are specified, the list is limited to just those inferiors.\n\
By default all inferiors are displayed."));

  c = add_com ("add-inferior", no_class, add_inferior_command, _("\
Add a new inferior.\n\
Usage: add-inferior [-copies N] [-exec FILENAME] [-no-connection]\n\
N is the optional number of inferiors to add, default is 1.\n\
FILENAME is the file name of the executable to use\n\
as main program.\n\
By default, the new inferior inherits the current inferior's connection.\n\
If -no-connection is specified, the new inferior begins with\n\
no target connection yet."));
  set_cmd_completer (c, filename_completer);

  add_com ("remove-inferiors", no_class, remove_inferior_command, _("\
Remove inferior ID (or list of IDs).\n\
Usage: remove-inferiors ID..."));

  add_com ("clone-inferior", no_class, clone_inferior_command, _("\
Clone inferior ID.\n\
Usage: clone-inferior [-copies N] [-no-connection] [ID]\n\
Add N copies of inferior ID.  The new inferiors have the same\n\
executable loaded as the copied inferior.  If -copies is not specified,\n\
adds 1 copy.  If ID is not specified, it is the current inferior\n\
that is cloned.\n\
By default, the new inferiors inherit the copied inferior's connection.\n\
If -no-connection is specified, the new inferiors begin with\n\
no target connection yet."));

  add_cmd ("inferiors", class_run, detach_inferior_command, _("\
Detach from inferior ID (or list of IDS).\n\
Usage; detach inferiors ID..."),
	   &detachlist);

  add_cmd ("inferiors", class_run, kill_inferior_command, _("\
Kill inferior ID (or list of IDs).\n\
Usage: kill inferiors ID..."),
	   &killlist);

  add_cmd ("inferior", class_run, inferior_command, _("\
Use this command to switch between inferiors.\n\
Usage: inferior ID\n\
The new inferior ID must be currently known."),
	   &cmdlist);

  add_setshow_boolean_cmd ("inferior-events", no_class,
	 &print_inferior_events, _("\
Set printing of inferior events (such as inferior start and exit)."), _("\
Show printing of inferior events (such as inferior start and exit)."), NULL,
	 NULL,
	 show_print_inferior_events,
	 &setprintlist, &showprintlist);

  create_internalvar_type_lazy ("_inferior", &inferior_funcs, NULL);
}
