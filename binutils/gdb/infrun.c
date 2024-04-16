/* Target-struct-independent code to start (run) and stop an inferior
   process.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include "displaced-stepping.h"
#include "infrun.h"
#include <ctype.h>
#include "symtab.h"
#include "frame.h"
#include "inferior.h"
#include "breakpoint.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "target.h"
#include "target-connection.h"
#include "gdbthread.h"
#include "annotate.h"
#include "symfile.h"
#include "top.h"
#include "ui.h"
#include "inf-loop.h"
#include "regcache.h"
#include "value.h"
#include "observable.h"
#include "language.h"
#include "solib.h"
#include "main.h"
#include "block.h"
#include "mi/mi-common.h"
#include "event-top.h"
#include "record.h"
#include "record-full.h"
#include "inline-frame.h"
#include "jit.h"
#include "tracepoint.h"
#include "skip.h"
#include "probe.h"
#include "objfiles.h"
#include "completer.h"
#include "target-descriptions.h"
#include "target-dcache.h"
#include "terminal.h"
#include "solist.h"
#include "gdbsupport/event-loop.h"
#include "thread-fsm.h"
#include "gdbsupport/enum-flags.h"
#include "progspace-and-thread.h"
#include <optional>
#include "arch-utils.h"
#include "gdbsupport/scope-exit.h"
#include "gdbsupport/forward-scope-exit.h"
#include "gdbsupport/gdb_select.h"
#include <unordered_map>
#include "async-event.h"
#include "gdbsupport/selftest.h"
#include "scoped-mock-context.h"
#include "test-target.h"
#include "gdbsupport/common-debug.h"
#include "gdbsupport/buildargv.h"
#include "extension.h"
#include "disasm.h"
#include "interps.h"

/* Prototypes for local functions */

static void sig_print_info (enum gdb_signal);

static void sig_print_header (void);

static void follow_inferior_reset_breakpoints (void);

static bool currently_stepping (struct thread_info *tp);

static void insert_hp_step_resume_breakpoint_at_frame (frame_info_ptr);

static void insert_step_resume_breakpoint_at_caller (frame_info_ptr);

static void insert_longjmp_resume_breakpoint (struct gdbarch *, CORE_ADDR);

static bool maybe_software_singlestep (struct gdbarch *gdbarch);

static void resume (gdb_signal sig);

static void wait_for_inferior (inferior *inf);

static void restart_threads (struct thread_info *event_thread,
			     inferior *inf = nullptr);

static bool start_step_over (void);

static bool step_over_info_valid_p (void);

static bool schedlock_applies (struct thread_info *tp);

/* Asynchronous signal handler registered as event loop source for
   when we have pending events ready to be passed to the core.  */
static struct async_event_handler *infrun_async_inferior_event_token;

/* Stores whether infrun_async was previously enabled or disabled.
   Starts off as -1, indicating "never enabled/disabled".  */
static int infrun_is_async = -1;
static CORE_ADDR update_line_range_start (CORE_ADDR pc,
					  struct execution_control_state *ecs);

/* See infrun.h.  */

void
infrun_async (int enable)
{
  if (infrun_is_async != enable)
    {
      infrun_is_async = enable;

      infrun_debug_printf ("enable=%d", enable);

      if (enable)
	mark_async_event_handler (infrun_async_inferior_event_token);
      else
	clear_async_event_handler (infrun_async_inferior_event_token);
    }
}

/* See infrun.h.  */

void
mark_infrun_async_event_handler (void)
{
  mark_async_event_handler (infrun_async_inferior_event_token);
}

/* When set, stop the 'step' command if we enter a function which has
   no line number information.  The normal behavior is that we step
   over such function.  */
bool step_stop_if_no_debug = false;
static void
show_step_stop_if_no_debug (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Mode of the step operation is %s.\n"), value);
}

/* proceed and normal_stop use this to notify the user when the
   inferior stopped in a different thread than it had been running in.
   It can also be used to find for which thread normal_stop last
   reported a stop.  */
static thread_info_ref previous_thread;

/* See infrun.h.  */

void
update_previous_thread ()
{
  if (inferior_ptid == null_ptid)
    previous_thread = nullptr;
  else
    previous_thread = thread_info_ref::new_reference (inferior_thread ());
}

/* See infrun.h.  */

thread_info *
get_previous_thread ()
{
  return previous_thread.get ();
}

/* If set (default for legacy reasons), when following a fork, GDB
   will detach from one of the fork branches, child or parent.
   Exactly which branch is detached depends on 'set follow-fork-mode'
   setting.  */

static bool detach_fork = true;

bool debug_infrun = false;
static void
show_debug_infrun (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Inferior debugging is %s.\n"), value);
}

/* Support for disabling address space randomization.  */

bool disable_randomization = true;

static void
show_disable_randomization (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c, const char *value)
{
  if (target_supports_disable_randomization ())
    gdb_printf (file,
		_("Disabling randomization of debuggee's "
		  "virtual address space is %s.\n"),
		value);
  else
    gdb_puts (_("Disabling randomization of debuggee's "
		"virtual address space is unsupported on\n"
		"this platform.\n"), file);
}

static void
set_disable_randomization (const char *args, int from_tty,
			   struct cmd_list_element *c)
{
  if (!target_supports_disable_randomization ())
    error (_("Disabling randomization of debuggee's "
	     "virtual address space is unsupported on\n"
	     "this platform."));
}

/* User interface for non-stop mode.  */

bool non_stop = false;
static bool non_stop_1 = false;

static void
set_non_stop (const char *args, int from_tty,
	      struct cmd_list_element *c)
{
  if (target_has_execution ())
    {
      non_stop_1 = non_stop;
      error (_("Cannot change this setting while the inferior is running."));
    }

  non_stop = non_stop_1;
}

static void
show_non_stop (struct ui_file *file, int from_tty,
	       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Controlling the inferior in non-stop mode is %s.\n"),
	      value);
}

/* "Observer mode" is somewhat like a more extreme version of
   non-stop, in which all GDB operations that might affect the
   target's execution have been disabled.  */

static bool observer_mode = false;
static bool observer_mode_1 = false;

static void
set_observer_mode (const char *args, int from_tty,
		   struct cmd_list_element *c)
{
  if (target_has_execution ())
    {
      observer_mode_1 = observer_mode;
      error (_("Cannot change this setting while the inferior is running."));
    }

  observer_mode = observer_mode_1;

  may_write_registers = !observer_mode;
  may_write_memory = !observer_mode;
  may_insert_breakpoints = !observer_mode;
  may_insert_tracepoints = !observer_mode;
  /* We can insert fast tracepoints in or out of observer mode,
     but enable them if we're going into this mode.  */
  if (observer_mode)
    may_insert_fast_tracepoints = true;
  may_stop = !observer_mode;
  update_target_permissions ();

  /* Going *into* observer mode we must force non-stop, then
     going out we leave it that way.  */
  if (observer_mode)
    {
      pagination_enabled = false;
      non_stop = non_stop_1 = true;
    }

  if (from_tty)
    gdb_printf (_("Observer mode is now %s.\n"),
		(observer_mode ? "on" : "off"));
}

static void
show_observer_mode (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Observer mode is %s.\n"), value);
}

/* This updates the value of observer mode based on changes in
   permissions.  Note that we are deliberately ignoring the values of
   may-write-registers and may-write-memory, since the user may have
   reason to enable these during a session, for instance to turn on a
   debugging-related global.  */

void
update_observer_mode (void)
{
  bool newval = (!may_insert_breakpoints
		 && !may_insert_tracepoints
		 && may_insert_fast_tracepoints
		 && !may_stop
		 && non_stop);

  /* Let the user know if things change.  */
  if (newval != observer_mode)
    gdb_printf (_("Observer mode is now %s.\n"),
		(newval ? "on" : "off"));

  observer_mode = observer_mode_1 = newval;
}

/* Tables of how to react to signals; the user sets them.  */

static unsigned char signal_stop[GDB_SIGNAL_LAST];
static unsigned char signal_print[GDB_SIGNAL_LAST];
static unsigned char signal_program[GDB_SIGNAL_LAST];

/* Table of signals that are registered with "catch signal".  A
   non-zero entry indicates that the signal is caught by some "catch
   signal" command.  */
static unsigned char signal_catch[GDB_SIGNAL_LAST];

/* Table of signals that the target may silently handle.
   This is automatically determined from the flags above,
   and simply cached here.  */
static unsigned char signal_pass[GDB_SIGNAL_LAST];

#define SET_SIGS(nsigs,sigs,flags) \
  do { \
    int signum = (nsigs); \
    while (signum-- > 0) \
      if ((sigs)[signum]) \
	(flags)[signum] = 1; \
  } while (0)

#define UNSET_SIGS(nsigs,sigs,flags) \
  do { \
    int signum = (nsigs); \
    while (signum-- > 0) \
      if ((sigs)[signum]) \
	(flags)[signum] = 0; \
  } while (0)

/* Update the target's copy of SIGNAL_PROGRAM.  The sole purpose of
   this function is to avoid exporting `signal_program'.  */

void
update_signals_program_target (void)
{
  target_program_signals (signal_program);
}

/* Value to pass to target_resume() to cause all threads to resume.  */

#define RESUME_ALL minus_one_ptid

/* Command list pointer for the "stop" placeholder.  */

static struct cmd_list_element *stop_command;

/* Nonzero if we want to give control to the user when we're notified
   of shared library events by the dynamic linker.  */
int stop_on_solib_events;

/* Enable or disable optional shared library event breakpoints
   as appropriate when the above flag is changed.  */

static void
set_stop_on_solib_events (const char *args,
			  int from_tty, struct cmd_list_element *c)
{
  update_solib_breakpoints ();
}

static void
show_stop_on_solib_events (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Stopping for shared library events is %s.\n"),
	      value);
}

/* True after stop if current stack frame should be printed.  */

static bool stop_print_frame;

/* This is a cached copy of the target/ptid/waitstatus of the last
   event returned by target_wait().
   This information is returned by get_last_target_status().  */
static process_stratum_target *target_last_proc_target;
static ptid_t target_last_wait_ptid;
static struct target_waitstatus target_last_waitstatus;

void init_thread_stepping_state (struct thread_info *tss);

static const char follow_fork_mode_child[] = "child";
static const char follow_fork_mode_parent[] = "parent";

static const char *const follow_fork_mode_kind_names[] = {
  follow_fork_mode_child,
  follow_fork_mode_parent,
  nullptr
};

static const char *follow_fork_mode_string = follow_fork_mode_parent;
static void
show_follow_fork_mode_string (struct ui_file *file, int from_tty,
			      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Debugger response to a program "
		"call of fork or vfork is \"%s\".\n"),
	      value);
}


/* Handle changes to the inferior list based on the type of fork,
   which process is being followed, and whether the other process
   should be detached.  On entry inferior_ptid must be the ptid of
   the fork parent.  At return inferior_ptid is the ptid of the
   followed inferior.  */

static bool
follow_fork_inferior (bool follow_child, bool detach_fork)
{
  INFRUN_SCOPED_DEBUG_ENTER_EXIT;

  infrun_debug_printf ("follow_child = %d, detach_fork = %d",
		       follow_child, detach_fork);

  target_waitkind fork_kind = inferior_thread ()->pending_follow.kind ();
  gdb_assert (fork_kind == TARGET_WAITKIND_FORKED
	      || fork_kind == TARGET_WAITKIND_VFORKED);
  bool has_vforked = fork_kind == TARGET_WAITKIND_VFORKED;
  ptid_t parent_ptid = inferior_ptid;
  ptid_t child_ptid = inferior_thread ()->pending_follow.child_ptid ();

  if (has_vforked
      && !non_stop /* Non-stop always resumes both branches.  */
      && current_ui->prompt_state == PROMPT_BLOCKED
      && !(follow_child || detach_fork || sched_multi))
    {
      /* The parent stays blocked inside the vfork syscall until the
	 child execs or exits.  If we don't let the child run, then
	 the parent stays blocked.  If we're telling the parent to run
	 in the foreground, the user will not be able to ctrl-c to get
	 back the terminal, effectively hanging the debug session.  */
      gdb_printf (gdb_stderr, _("\
Can not resume the parent process over vfork in the foreground while\n\
holding the child stopped.  Try \"set detach-on-fork\" or \
\"set schedule-multiple\".\n"));
      return true;
    }

  inferior *parent_inf = current_inferior ();
  inferior *child_inf = nullptr;

  gdb_assert (parent_inf->thread_waiting_for_vfork_done == nullptr);

  if (!follow_child)
    {
      /* Detach new forked process?  */
      if (detach_fork)
	{
	  /* Before detaching from the child, remove all breakpoints
	     from it.  If we forked, then this has already been taken
	     care of by infrun.c.  If we vforked however, any
	     breakpoint inserted in the parent is visible in the
	     child, even those added while stopped in a vfork
	     catchpoint.  This will remove the breakpoints from the
	     parent also, but they'll be reinserted below.  */
	  if (has_vforked)
	    {
	      /* Keep breakpoints list in sync.  */
	      remove_breakpoints_inf (current_inferior ());
	    }

	  if (print_inferior_events)
	    {
	      /* Ensure that we have a process ptid.  */
	      ptid_t process_ptid = ptid_t (child_ptid.pid ());

	      target_terminal::ours_for_output ();
	      gdb_printf (_("[Detaching after %s from child %s]\n"),
			  has_vforked ? "vfork" : "fork",
			  target_pid_to_str (process_ptid).c_str ());
	    }
	}
      else
	{
	  /* Add process to GDB's tables.  */
	  child_inf = add_inferior (child_ptid.pid ());

	  child_inf->attach_flag = parent_inf->attach_flag;
	  copy_terminal_info (child_inf, parent_inf);
	  child_inf->set_arch (parent_inf->arch ());
	  child_inf->tdesc_info = parent_inf->tdesc_info;

	  child_inf->symfile_flags = SYMFILE_NO_READ;

	  /* If this is a vfork child, then the address-space is
	     shared with the parent.  */
	  if (has_vforked)
	    {
	      child_inf->pspace = parent_inf->pspace;
	      child_inf->aspace = parent_inf->aspace;

	      exec_on_vfork (child_inf);

	      /* The parent will be frozen until the child is done
		 with the shared region.  Keep track of the
		 parent.  */
	      child_inf->vfork_parent = parent_inf;
	      child_inf->pending_detach = false;
	      parent_inf->vfork_child = child_inf;
	      parent_inf->pending_detach = false;
	    }
	  else
	    {
	      child_inf->pspace = new program_space (new_address_space ());
	      child_inf->aspace = child_inf->pspace->aspace;
	      child_inf->removable = true;
	      clone_program_space (child_inf->pspace, parent_inf->pspace);
	    }
	}

      if (has_vforked)
	{
	  /* If we detached from the child, then we have to be careful
	     to not insert breakpoints in the parent until the child
	     is done with the shared memory region.  However, if we're
	     staying attached to the child, then we can and should
	     insert breakpoints, so that we can debug it.  A
	     subsequent child exec or exit is enough to know when does
	     the child stops using the parent's address space.  */
	  parent_inf->thread_waiting_for_vfork_done
	    = detach_fork ? inferior_thread () : nullptr;
	  parent_inf->pspace->breakpoints_not_allowed = detach_fork;

	  infrun_debug_printf
	    ("parent_inf->thread_waiting_for_vfork_done == %s",
	     (parent_inf->thread_waiting_for_vfork_done == nullptr
	      ? "nullptr"
	      : (parent_inf->thread_waiting_for_vfork_done
		 ->ptid.to_string ().c_str ())));
	}
    }
  else
    {
      /* Follow the child.  */

      if (print_inferior_events)
	{
	  std::string parent_pid = target_pid_to_str (parent_ptid);
	  std::string child_pid = target_pid_to_str (child_ptid);

	  target_terminal::ours_for_output ();
	  gdb_printf (_("[Attaching after %s %s to child %s]\n"),
		      parent_pid.c_str (),
		      has_vforked ? "vfork" : "fork",
		      child_pid.c_str ());
	}

      /* Add the new inferior first, so that the target_detach below
	 doesn't unpush the target.  */

      child_inf = add_inferior (child_ptid.pid ());

      child_inf->attach_flag = parent_inf->attach_flag;
      copy_terminal_info (child_inf, parent_inf);
      child_inf->set_arch (parent_inf->arch ());
      child_inf->tdesc_info = parent_inf->tdesc_info;

      if (has_vforked)
	{
	  /* If this is a vfork child, then the address-space is shared
	     with the parent.  */
	  child_inf->aspace = parent_inf->aspace;
	  child_inf->pspace = parent_inf->pspace;

	  exec_on_vfork (child_inf);
	}
      else if (detach_fork)
	{
	  /* We follow the child and detach from the parent: move the parent's
	     program space to the child.  This simplifies some things, like
	     doing "next" over fork() and landing on the expected line in the
	     child (note, that is broken with "set detach-on-fork off").

	     Before assigning brand new spaces for the parent, remove
	     breakpoints from it: because the new pspace won't match
	     currently inserted locations, the normal detach procedure
	     wouldn't remove them, and we would leave them inserted when
	     detaching.  */
	  remove_breakpoints_inf (parent_inf);

	  child_inf->aspace = parent_inf->aspace;
	  child_inf->pspace = parent_inf->pspace;
	  parent_inf->pspace = new program_space (new_address_space ());
	  parent_inf->aspace = parent_inf->pspace->aspace;
	  clone_program_space (parent_inf->pspace, child_inf->pspace);

	  /* The parent inferior is still the current one, so keep things
	     in sync.  */
	  set_current_program_space (parent_inf->pspace);
	}
      else
	{
	  child_inf->pspace = new program_space (new_address_space ());
	  child_inf->aspace = child_inf->pspace->aspace;
	  child_inf->removable = true;
	  child_inf->symfile_flags = SYMFILE_NO_READ;
	  clone_program_space (child_inf->pspace, parent_inf->pspace);
	}
    }

  gdb_assert (current_inferior () == parent_inf);

  /* If we are setting up an inferior for the child, target_follow_fork is
     responsible for pushing the appropriate targets on the new inferior's
     target stack and adding the initial thread (with ptid CHILD_PTID).

     If we are not setting up an inferior for the child (because following
     the parent and detach_fork is true), it is responsible for detaching
     from CHILD_PTID.  */
  target_follow_fork (child_inf, child_ptid, fork_kind, follow_child,
		      detach_fork);

  gdb::observers::inferior_forked.notify (parent_inf, child_inf, fork_kind);

  /* target_follow_fork must leave the parent as the current inferior.  If we
     want to follow the child, we make it the current one below.  */
  gdb_assert (current_inferior () == parent_inf);

  /* If there is a child inferior, target_follow_fork must have created a thread
     for it.  */
  if (child_inf != nullptr)
    gdb_assert (!child_inf->thread_list.empty ());

  /* Clear the parent thread's pending follow field.  Do this before calling
     target_detach, so that the target can differentiate the two following
     cases:

      - We continue past a fork with "follow-fork-mode == child" &&
	"detach-on-fork on", and therefore detach the parent.  In that
	case the target should not detach the fork child.
      - We run to a fork catchpoint and the user types "detach".  In that
	case, the target should detach the fork child in addition to the
	parent.

     The former case will have pending_follow cleared, the later will have
     pending_follow set.  */
  thread_info *parent_thread = parent_inf->find_thread (parent_ptid);
  gdb_assert (parent_thread != nullptr);
  parent_thread->pending_follow.set_spurious ();

  /* Detach the parent if needed.  */
  if (follow_child)
    {
      /* If we're vforking, we want to hold on to the parent until
	 the child exits or execs.  At child exec or exit time we
	 can remove the old breakpoints from the parent and detach
	 or resume debugging it.  Otherwise, detach the parent now;
	 we'll want to reuse it's program/address spaces, but we
	 can't set them to the child before removing breakpoints
	 from the parent, otherwise, the breakpoints module could
	 decide to remove breakpoints from the wrong process (since
	 they'd be assigned to the same address space).  */

      if (has_vforked)
	{
	  gdb_assert (child_inf->vfork_parent == nullptr);
	  gdb_assert (parent_inf->vfork_child == nullptr);
	  child_inf->vfork_parent = parent_inf;
	  child_inf->pending_detach = false;
	  parent_inf->vfork_child = child_inf;
	  parent_inf->pending_detach = detach_fork;
	}
      else if (detach_fork)
	{
	  if (print_inferior_events)
	    {
	      /* Ensure that we have a process ptid.  */
	      ptid_t process_ptid = ptid_t (parent_ptid.pid ());

	      target_terminal::ours_for_output ();
	      gdb_printf (_("[Detaching after fork from "
			    "parent %s]\n"),
			  target_pid_to_str (process_ptid).c_str ());
	    }

	  target_detach (parent_inf, 0);
	}
    }

  /* If we ended up creating a new inferior, call post_create_inferior to inform
     the various subcomponents.  */
  if (child_inf != nullptr)
    {
      /* If FOLLOW_CHILD, we leave CHILD_INF as the current inferior
	 (do not restore the parent as the current inferior).  */
      std::optional<scoped_restore_current_thread> maybe_restore;

      if (!follow_child && !sched_multi)
	maybe_restore.emplace ();

      switch_to_thread (*child_inf->threads ().begin ());
      post_create_inferior (0);
    }

  return false;
}

/* Set the last target status as TP having stopped.  */

static void
set_last_target_status_stopped (thread_info *tp)
{
  set_last_target_status (tp->inf->process_target (), tp->ptid,
			  target_waitstatus {}.set_stopped (GDB_SIGNAL_0));
}

/* Tell the target to follow the fork we're stopped at.  Returns true
   if the inferior should be resumed; false, if the target for some
   reason decided it's best not to resume.  */

static bool
follow_fork ()
{
  INFRUN_SCOPED_DEBUG_ENTER_EXIT;

  bool follow_child = (follow_fork_mode_string == follow_fork_mode_child);
  bool should_resume = true;

  /* Copy user stepping state to the new inferior thread.  FIXME: the
     followed fork child thread should have a copy of most of the
     parent thread structure's run control related fields, not just these.
     Initialized to avoid "may be used uninitialized" warnings from gcc.  */
  struct breakpoint *step_resume_breakpoint = nullptr;
  struct breakpoint *exception_resume_breakpoint = nullptr;
  CORE_ADDR step_range_start = 0;
  CORE_ADDR step_range_end = 0;
  int current_line = 0;
  symtab *current_symtab = nullptr;
  struct frame_id step_frame_id = { 0 };

  if (!non_stop)
    {
      thread_info *cur_thr = inferior_thread ();

      ptid_t resume_ptid
	= user_visible_resume_ptid (cur_thr->control.stepping_command);
      process_stratum_target *resume_target
	= user_visible_resume_target (resume_ptid);

      /* Check if there's a thread that we're about to resume, other
	 than the current, with an unfollowed fork/vfork.  If so,
	 switch back to it, to tell the target to follow it (in either
	 direction).  We'll afterwards refuse to resume, and inform
	 the user what happened.  */
      for (thread_info *tp : all_non_exited_threads (resume_target,
						     resume_ptid))
	{
	  if (tp == cur_thr)
	    continue;

	  /* follow_fork_inferior clears tp->pending_follow, and below
	     we'll need the value after the follow_fork_inferior
	     call.  */
	  target_waitkind kind = tp->pending_follow.kind ();

	  if (kind != TARGET_WAITKIND_SPURIOUS)
	    {
	      infrun_debug_printf ("need to follow-fork [%s] first",
				   tp->ptid.to_string ().c_str ());

	      switch_to_thread (tp);

	      /* Set up inferior(s) as specified by the caller, and
		 tell the target to do whatever is necessary to follow
		 either parent or child.  */
	      if (follow_child)
		{
		  /* The thread that started the execution command
		     won't exist in the child.  Abort the command and
		     immediately stop in this thread, in the child,
		     inside fork.  */
		  should_resume = false;
		}
	      else
		{
		  /* Following the parent, so let the thread fork its
		     child freely, it won't influence the current
		     execution command.  */
		  if (follow_fork_inferior (follow_child, detach_fork))
		    {
		      /* Target refused to follow, or there's some
			 other reason we shouldn't resume.  */
		      switch_to_thread (cur_thr);
		      set_last_target_status_stopped (cur_thr);
		      return false;
		    }

		  /* If we're following a vfork, when we need to leave
		     the just-forked thread as selected, as we need to
		     solo-resume it to collect the VFORK_DONE event.
		     If we're following a fork, however, switch back
		     to the original thread that we continue stepping
		     it, etc.  */
		  if (kind != TARGET_WAITKIND_VFORKED)
		    {
		      gdb_assert (kind == TARGET_WAITKIND_FORKED);
		      switch_to_thread (cur_thr);
		    }
		}

	      break;
	    }
	}
    }

  thread_info *tp = inferior_thread ();

  /* If there were any forks/vforks that were caught and are now to be
     followed, then do so now.  */
  switch (tp->pending_follow.kind ())
    {
    case TARGET_WAITKIND_FORKED:
    case TARGET_WAITKIND_VFORKED:
      {
	ptid_t parent, child;
	std::unique_ptr<struct thread_fsm> thread_fsm;

	/* If the user did a next/step, etc, over a fork call,
	   preserve the stepping state in the fork child.  */
	if (follow_child && should_resume)
	  {
	    step_resume_breakpoint = clone_momentary_breakpoint
					 (tp->control.step_resume_breakpoint);
	    step_range_start = tp->control.step_range_start;
	    step_range_end = tp->control.step_range_end;
	    current_line = tp->current_line;
	    current_symtab = tp->current_symtab;
	    step_frame_id = tp->control.step_frame_id;
	    exception_resume_breakpoint
	      = clone_momentary_breakpoint (tp->control.exception_resume_breakpoint);
	    thread_fsm = tp->release_thread_fsm ();

	    /* For now, delete the parent's sr breakpoint, otherwise,
	       parent/child sr breakpoints are considered duplicates,
	       and the child version will not be installed.  Remove
	       this when the breakpoints module becomes aware of
	       inferiors and address spaces.  */
	    delete_step_resume_breakpoint (tp);
	    tp->control.step_range_start = 0;
	    tp->control.step_range_end = 0;
	    tp->control.step_frame_id = null_frame_id;
	    delete_exception_resume_breakpoint (tp);
	  }

	parent = inferior_ptid;
	child = tp->pending_follow.child_ptid ();

	/* If handling a vfork, stop all the inferior's threads, they will be
	   restarted when the vfork shared region is complete.  */
	if (tp->pending_follow.kind () == TARGET_WAITKIND_VFORKED
	    && target_is_non_stop_p ())
	  stop_all_threads ("handling vfork", tp->inf);

	process_stratum_target *parent_targ = tp->inf->process_target ();
	/* Set up inferior(s) as specified by the caller, and tell the
	   target to do whatever is necessary to follow either parent
	   or child.  */
	if (follow_fork_inferior (follow_child, detach_fork))
	  {
	    /* Target refused to follow, or there's some other reason
	       we shouldn't resume.  */
	    should_resume = 0;
	  }
	else
	  {
	    /* If we followed the child, switch to it...  */
	    if (follow_child)
	      {
		tp = parent_targ->find_thread (child);
		switch_to_thread (tp);

		/* ... and preserve the stepping state, in case the
		   user was stepping over the fork call.  */
		if (should_resume)
		  {
		    tp->control.step_resume_breakpoint
		      = step_resume_breakpoint;
		    tp->control.step_range_start = step_range_start;
		    tp->control.step_range_end = step_range_end;
		    tp->current_line = current_line;
		    tp->current_symtab = current_symtab;
		    tp->control.step_frame_id = step_frame_id;
		    tp->control.exception_resume_breakpoint
		      = exception_resume_breakpoint;
		    tp->set_thread_fsm (std::move (thread_fsm));
		  }
		else
		  {
		    /* If we get here, it was because we're trying to
		       resume from a fork catchpoint, but, the user
		       has switched threads away from the thread that
		       forked.  In that case, the resume command
		       issued is most likely not applicable to the
		       child, so just warn, and refuse to resume.  */
		    warning (_("Not resuming: switched threads "
			       "before following fork child."));
		  }

		/* Reset breakpoints in the child as appropriate.  */
		follow_inferior_reset_breakpoints ();
	      }
	  }
      }
      break;
    case TARGET_WAITKIND_SPURIOUS:
      /* Nothing to follow.  */
      break;
    default:
      internal_error ("Unexpected pending_follow.kind %d\n",
		      tp->pending_follow.kind ());
      break;
    }

  if (!should_resume)
    set_last_target_status_stopped (tp);
  return should_resume;
}

static void
follow_inferior_reset_breakpoints (void)
{
  struct thread_info *tp = inferior_thread ();

  /* Was there a step_resume breakpoint?  (There was if the user
     did a "next" at the fork() call.)  If so, explicitly reset its
     thread number.  Cloned step_resume breakpoints are disabled on
     creation, so enable it here now that it is associated with the
     correct thread.

     step_resumes are a form of bp that are made to be per-thread.
     Since we created the step_resume bp when the parent process
     was being debugged, and now are switching to the child process,
     from the breakpoint package's viewpoint, that's a switch of
     "threads".  We must update the bp's notion of which thread
     it is for, or it'll be ignored when it triggers.  */

  if (tp->control.step_resume_breakpoint)
    {
      breakpoint_re_set_thread (tp->control.step_resume_breakpoint);
      tp->control.step_resume_breakpoint->first_loc ().enabled = 1;
    }

  /* Treat exception_resume breakpoints like step_resume breakpoints.  */
  if (tp->control.exception_resume_breakpoint)
    {
      breakpoint_re_set_thread (tp->control.exception_resume_breakpoint);
      tp->control.exception_resume_breakpoint->first_loc ().enabled = 1;
    }

  /* Reinsert all breakpoints in the child.  The user may have set
     breakpoints after catching the fork, in which case those
     were never set in the child, but only in the parent.  This makes
     sure the inserted breakpoints match the breakpoint list.  */

  breakpoint_re_set ();
  insert_breakpoints ();
}

/* The child has exited or execed: resume THREAD, a thread of the parent,
   if it was meant to be executing.  */

static void
proceed_after_vfork_done (thread_info *thread)
{
  if (thread->state == THREAD_RUNNING
      && !thread->executing ()
      && !thread->stop_requested
      && thread->stop_signal () == GDB_SIGNAL_0)
    {
      infrun_debug_printf ("resuming vfork parent thread %s",
			   thread->ptid.to_string ().c_str ());

      switch_to_thread (thread);
      clear_proceed_status (0);
      proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
    }
}

/* Called whenever we notice an exec or exit event, to handle
   detaching or resuming a vfork parent.  */

static void
handle_vfork_child_exec_or_exit (int exec)
{
  INFRUN_SCOPED_DEBUG_ENTER_EXIT;

  struct inferior *inf = current_inferior ();

  if (inf->vfork_parent)
    {
      inferior *resume_parent = nullptr;

      /* This exec or exit marks the end of the shared memory region
	 between the parent and the child.  Break the bonds.  */
      inferior *vfork_parent = inf->vfork_parent;
      inf->vfork_parent->vfork_child = nullptr;
      inf->vfork_parent = nullptr;

      /* If the user wanted to detach from the parent, now is the
	 time.  */
      if (vfork_parent->pending_detach)
	{
	  struct program_space *pspace;

	  /* follow-fork child, detach-on-fork on.  */

	  vfork_parent->pending_detach = false;

	  scoped_restore_current_pspace_and_thread restore_thread;

	  /* We're letting loose of the parent.  */
	  thread_info *tp = any_live_thread_of_inferior (vfork_parent);
	  switch_to_thread (tp);

	  /* We're about to detach from the parent, which implicitly
	     removes breakpoints from its address space.  There's a
	     catch here: we want to reuse the spaces for the child,
	     but, parent/child are still sharing the pspace at this
	     point, although the exec in reality makes the kernel give
	     the child a fresh set of new pages.  The problem here is
	     that the breakpoints module being unaware of this, would
	     likely chose the child process to write to the parent
	     address space.  Swapping the child temporarily away from
	     the spaces has the desired effect.  Yes, this is "sort
	     of" a hack.  */

	  pspace = inf->pspace;
	  inf->pspace = nullptr;
	  address_space_ref_ptr aspace = std::move (inf->aspace);

	  if (print_inferior_events)
	    {
	      std::string pidstr
		= target_pid_to_str (ptid_t (vfork_parent->pid));

	      target_terminal::ours_for_output ();

	      if (exec)
		{
		  gdb_printf (_("[Detaching vfork parent %s "
				"after child exec]\n"), pidstr.c_str ());
		}
	      else
		{
		  gdb_printf (_("[Detaching vfork parent %s "
				"after child exit]\n"), pidstr.c_str ());
		}
	    }

	  target_detach (vfork_parent, 0);

	  /* Put it back.  */
	  inf->pspace = pspace;
	  inf->aspace = aspace;
	}
      else if (exec)
	{
	  /* We're staying attached to the parent, so, really give the
	     child a new address space.  */
	  inf->pspace = new program_space (maybe_new_address_space ());
	  inf->aspace = inf->pspace->aspace;
	  inf->removable = true;
	  set_current_program_space (inf->pspace);

	  resume_parent = vfork_parent;
	}
      else
	{
	  /* If this is a vfork child exiting, then the pspace and
	     aspaces were shared with the parent.  Since we're
	     reporting the process exit, we'll be mourning all that is
	     found in the address space, and switching to null_ptid,
	     preparing to start a new inferior.  But, since we don't
	     want to clobber the parent's address/program spaces, we
	     go ahead and create a new one for this exiting
	     inferior.  */

	  scoped_restore_current_thread restore_thread;

	  /* Temporarily switch to the vfork parent, to facilitate ptrace
	     calls done during maybe_new_address_space.  */
	  switch_to_thread (any_live_thread_of_inferior (vfork_parent));
	  address_space_ref_ptr aspace = maybe_new_address_space ();

	  /* Switch back to the vfork child inferior.  Switch to no-thread
	     while running clone_program_space, so that clone_program_space
	     doesn't want to read the selected frame of a dead process.  */
	  switch_to_inferior_no_thread (inf);

	  inf->pspace = new program_space (std::move (aspace));
	  inf->aspace = inf->pspace->aspace;
	  set_current_program_space (inf->pspace);
	  inf->removable = true;
	  inf->symfile_flags = SYMFILE_NO_READ;
	  clone_program_space (inf->pspace, vfork_parent->pspace);

	  resume_parent = vfork_parent;
	}

      gdb_assert (current_program_space == inf->pspace);

      if (non_stop && resume_parent != nullptr)
	{
	  /* If the user wanted the parent to be running, let it go
	     free now.  */
	  scoped_restore_current_thread restore_thread;

	  infrun_debug_printf ("resuming vfork parent process %d",
			       resume_parent->pid);

	  for (thread_info *thread : resume_parent->threads ())
	    proceed_after_vfork_done (thread);
	}
    }
}

/* Handle TARGET_WAITKIND_VFORK_DONE.  */

static void
handle_vfork_done (thread_info *event_thread)
{
  INFRUN_SCOPED_DEBUG_ENTER_EXIT;

  /* We only care about this event if inferior::thread_waiting_for_vfork_done is
     set, that is if we are waiting for a vfork child not under our control
     (because we detached it) to exec or exit.

     If an inferior has vforked and we are debugging the child, we don't use
     the vfork-done event to get notified about the end of the shared address
     space window.  We rely instead on the child's exec or exit event, and the
     inferior::vfork_{parent,child} fields are used instead.  See
     handle_vfork_child_exec_or_exit for that.  */
  if (event_thread->inf->thread_waiting_for_vfork_done == nullptr)
    {
      infrun_debug_printf ("not waiting for a vfork-done event");
      return;
    }

  /* We stopped all threads (other than the vforking thread) of the inferior in
     follow_fork and kept them stopped until now.  It should therefore not be
     possible for another thread to have reported a vfork during that window.
     If THREAD_WAITING_FOR_VFORK_DONE is set, it has to be the same thread whose
     vfork-done we are handling right now.  */
  gdb_assert (event_thread->inf->thread_waiting_for_vfork_done == event_thread);

  event_thread->inf->thread_waiting_for_vfork_done = nullptr;
  event_thread->inf->pspace->breakpoints_not_allowed = 0;

  /* On non-stop targets, we stopped all the inferior's threads in follow_fork,
     resume them now.  On all-stop targets, everything that needs to be resumed
     will be when we resume the event thread.  */
  if (target_is_non_stop_p ())
    {
      /* restart_threads and start_step_over may change the current thread, make
	 sure we leave the event thread as the current thread.  */
      scoped_restore_current_thread restore_thread;

      insert_breakpoints ();
      start_step_over ();

      if (!step_over_info_valid_p ())
	restart_threads (event_thread, event_thread->inf);
    }
}

/* Enum strings for "set|show follow-exec-mode".  */

static const char follow_exec_mode_new[] = "new";
static const char follow_exec_mode_same[] = "same";
static const char *const follow_exec_mode_names[] =
{
  follow_exec_mode_new,
  follow_exec_mode_same,
  nullptr,
};

static const char *follow_exec_mode_string = follow_exec_mode_same;
static void
show_follow_exec_mode_string (struct ui_file *file, int from_tty,
			      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Follow exec mode is \"%s\".\n"),  value);
}

/* EXEC_FILE_TARGET is assumed to be non-NULL.  */

static void
follow_exec (ptid_t ptid, const char *exec_file_target)
{
  int pid = ptid.pid ();
  ptid_t process_ptid;

  /* Switch terminal for any messages produced e.g. by
     breakpoint_re_set.  */
  target_terminal::ours_for_output ();

  /* This is an exec event that we actually wish to pay attention to.
     Refresh our symbol table to the newly exec'd program, remove any
     momentary bp's, etc.

     If there are breakpoints, they aren't really inserted now,
     since the exec() transformed our inferior into a fresh set
     of instructions.

     We want to preserve symbolic breakpoints on the list, since
     we have hopes that they can be reset after the new a.out's
     symbol table is read.

     However, any "raw" breakpoints must be removed from the list
     (e.g., the solib bp's), since their address is probably invalid
     now.

     And, we DON'T want to call delete_breakpoints() here, since
     that may write the bp's "shadow contents" (the instruction
     value that was overwritten with a TRAP instruction).  Since
     we now have a new a.out, those shadow contents aren't valid.  */

  mark_breakpoints_out ();

  /* The target reports the exec event to the main thread, even if
     some other thread does the exec, and even if the main thread was
     stopped or already gone.  We may still have non-leader threads of
     the process on our list.  E.g., on targets that don't have thread
     exit events (like remote) and nothing forces an update of the
     thread list up to here.  When debugging remotely, it's best to
     avoid extra traffic, when possible, so avoid syncing the thread
     list with the target, and instead go ahead and delete all threads
     of the process but the one that reported the event.  Note this must
     be done before calling update_breakpoints_after_exec, as
     otherwise clearing the threads' resources would reference stale
     thread breakpoints -- it may have been one of these threads that
     stepped across the exec.  We could just clear their stepping
     states, but as long as we're iterating, might as well delete
     them.  Deleting them now rather than at the next user-visible
     stop provides a nicer sequence of events for user and MI
     notifications.  */
  for (thread_info *th : all_threads_safe ())
    if (th->ptid.pid () == pid && th->ptid != ptid)
      delete_thread (th);

  /* We also need to clear any left over stale state for the
     leader/event thread.  E.g., if there was any step-resume
     breakpoint or similar, it's gone now.  We cannot truly
     step-to-next statement through an exec().  */
  thread_info *th = inferior_thread ();
  th->control.step_resume_breakpoint = nullptr;
  th->control.exception_resume_breakpoint = nullptr;
  th->control.single_step_breakpoints = nullptr;
  th->control.step_range_start = 0;
  th->control.step_range_end = 0;

  /* The user may have had the main thread held stopped in the
     previous image (e.g., schedlock on, or non-stop).  Release
     it now.  */
  th->stop_requested = 0;

  update_breakpoints_after_exec ();

  /* What is this a.out's name?  */
  process_ptid = ptid_t (pid);
  gdb_printf (_("%s is executing new program: %s\n"),
	      target_pid_to_str (process_ptid).c_str (),
	      exec_file_target);

  /* We've followed the inferior through an exec.  Therefore, the
     inferior has essentially been killed & reborn.  */

  breakpoint_init_inferior (inf_execd);

  gdb::unique_xmalloc_ptr<char> exec_file_host
    = exec_file_find (exec_file_target, nullptr);

  /* If we were unable to map the executable target pathname onto a host
     pathname, tell the user that.  Otherwise GDB's subsequent behavior
     is confusing.  Maybe it would even be better to stop at this point
     so that the user can specify a file manually before continuing.  */
  if (exec_file_host == nullptr)
    warning (_("Could not load symbols for executable %s.\n"
	       "Do you need \"set sysroot\"?"),
	     exec_file_target);

  /* Reset the shared library package.  This ensures that we get a
     shlib event when the child reaches "_start", at which point the
     dld will have had a chance to initialize the child.  */
  /* Also, loading a symbol file below may trigger symbol lookups, and
     we don't want those to be satisfied by the libraries of the
     previous incarnation of this process.  */
  no_shared_libraries (nullptr, 0);

  inferior *execing_inferior = current_inferior ();
  inferior *following_inferior;

  if (follow_exec_mode_string == follow_exec_mode_new)
    {
      /* The user wants to keep the old inferior and program spaces
	 around.  Create a new fresh one, and switch to it.  */

      /* Do exit processing for the original inferior before setting the new
	 inferior's pid.  Having two inferiors with the same pid would confuse
	 find_inferior_p(t)id.  Transfer the terminal state and info from the
	  old to the new inferior.  */
      following_inferior = add_inferior_with_spaces ();

      swap_terminal_info (following_inferior, execing_inferior);
      exit_inferior (execing_inferior);

      following_inferior->pid = pid;
    }
  else
    {
      /* follow-exec-mode is "same", we continue execution in the execing
	 inferior.  */
      following_inferior = execing_inferior;

      /* The old description may no longer be fit for the new image.
	 E.g, a 64-bit process exec'ed a 32-bit process.  Clear the
	 old description; we'll read a new one below.  No need to do
	 this on "follow-exec-mode new", as the old inferior stays
	 around (its description is later cleared/refetched on
	 restart).  */
      target_clear_description ();
    }

  target_follow_exec (following_inferior, ptid, exec_file_target);

  gdb_assert (current_inferior () == following_inferior);
  gdb_assert (current_program_space == following_inferior->pspace);

  /* Attempt to open the exec file.  SYMFILE_DEFER_BP_RESET is used
     because the proper displacement for a PIE (Position Independent
     Executable) main symbol file will only be computed by
     solib_create_inferior_hook below.  breakpoint_re_set would fail
     to insert the breakpoints with the zero displacement.  */
  try_open_exec_file (exec_file_host.get (), following_inferior,
		      SYMFILE_DEFER_BP_RESET);

  /* If the target can specify a description, read it.  Must do this
     after flipping to the new executable (because the target supplied
     description must be compatible with the executable's
     architecture, and the old executable may e.g., be 32-bit, while
     the new one 64-bit), and before anything involving memory or
     registers.  */
  target_find_description ();

  gdb::observers::inferior_execd.notify (execing_inferior, following_inferior);

  breakpoint_re_set ();

  /* Reinsert all breakpoints.  (Those which were symbolic have
     been reset to the proper address in the new a.out, thanks
     to symbol_file_command...).  */
  insert_breakpoints ();

  /* The next resume of this inferior should bring it to the shlib
     startup breakpoints.  (If the user had also set bp's on
     "main" from the old (parent) process, then they'll auto-
     matically get reset there in the new process.).  */
}

/* The chain of threads that need to do a step-over operation to get
   past e.g., a breakpoint.  What technique is used to step over the
   breakpoint/watchpoint does not matter -- all threads end up in the
   same queue, to maintain rough temporal order of execution, in order
   to avoid starvation, otherwise, we could e.g., find ourselves
   constantly stepping the same couple threads past their breakpoints
   over and over, if the single-step finish fast enough.  */
thread_step_over_list global_thread_step_over_list;

/* Bit flags indicating what the thread needs to step over.  */

enum step_over_what_flag
  {
    /* Step over a breakpoint.  */
    STEP_OVER_BREAKPOINT = 1,

    /* Step past a non-continuable watchpoint, in order to let the
       instruction execute so we can evaluate the watchpoint
       expression.  */
    STEP_OVER_WATCHPOINT = 2
  };
DEF_ENUM_FLAGS_TYPE (enum step_over_what_flag, step_over_what);

/* Info about an instruction that is being stepped over.  */

struct step_over_info
{
  /* If we're stepping past a breakpoint, this is the address space
     and address of the instruction the breakpoint is set at.  We'll
     skip inserting all breakpoints here.  Valid iff ASPACE is
     non-NULL.  */
  const address_space *aspace = nullptr;
  CORE_ADDR address = 0;

  /* The instruction being stepped over triggers a nonsteppable
     watchpoint.  If true, we'll skip inserting watchpoints.  */
  int nonsteppable_watchpoint_p = 0;

  /* The thread's global number.  */
  int thread = -1;
};

/* The step-over info of the location that is being stepped over.

   Note that with async/breakpoint always-inserted mode, a user might
   set a new breakpoint/watchpoint/etc. exactly while a breakpoint is
   being stepped over.  As setting a new breakpoint inserts all
   breakpoints, we need to make sure the breakpoint being stepped over
   isn't inserted then.  We do that by only clearing the step-over
   info when the step-over is actually finished (or aborted).

   Presently GDB can only step over one breakpoint at any given time.
   Given threads that can't run code in the same address space as the
   breakpoint's can't really miss the breakpoint, GDB could be taught
   to step-over at most one breakpoint per address space (so this info
   could move to the address space object if/when GDB is extended).
   The set of breakpoints being stepped over will normally be much
   smaller than the set of all breakpoints, so a flag in the
   breakpoint location structure would be wasteful.  A separate list
   also saves complexity and run-time, as otherwise we'd have to go
   through all breakpoint locations clearing their flag whenever we
   start a new sequence.  Similar considerations weigh against storing
   this info in the thread object.  Plus, not all step overs actually
   have breakpoint locations -- e.g., stepping past a single-step
   breakpoint, or stepping to complete a non-continuable
   watchpoint.  */
static struct step_over_info step_over_info;

/* Record the address of the breakpoint/instruction we're currently
   stepping over.
   N.B. We record the aspace and address now, instead of say just the thread,
   because when we need the info later the thread may be running.  */

static void
set_step_over_info (const address_space *aspace, CORE_ADDR address,
		    int nonsteppable_watchpoint_p,
		    int thread)
{
  step_over_info.aspace = aspace;
  step_over_info.address = address;
  step_over_info.nonsteppable_watchpoint_p = nonsteppable_watchpoint_p;
  step_over_info.thread = thread;
}

/* Called when we're not longer stepping over a breakpoint / an
   instruction, so all breakpoints are free to be (re)inserted.  */

static void
clear_step_over_info (void)
{
  infrun_debug_printf ("clearing step over info");
  step_over_info.aspace = nullptr;
  step_over_info.address = 0;
  step_over_info.nonsteppable_watchpoint_p = 0;
  step_over_info.thread = -1;
}

/* See infrun.h.  */

int
stepping_past_instruction_at (struct address_space *aspace,
			      CORE_ADDR address)
{
  return (step_over_info.aspace != nullptr
	  && breakpoint_address_match (aspace, address,
				       step_over_info.aspace,
				       step_over_info.address));
}

/* See infrun.h.  */

int
thread_is_stepping_over_breakpoint (int thread)
{
  return (step_over_info.thread != -1
	  && thread == step_over_info.thread);
}

/* See infrun.h.  */

int
stepping_past_nonsteppable_watchpoint (void)
{
  return step_over_info.nonsteppable_watchpoint_p;
}

/* Returns true if step-over info is valid.  */

static bool
step_over_info_valid_p (void)
{
  return (step_over_info.aspace != nullptr
	  || stepping_past_nonsteppable_watchpoint ());
}


/* Displaced stepping.  */

/* In non-stop debugging mode, we must take special care to manage
   breakpoints properly; in particular, the traditional strategy for
   stepping a thread past a breakpoint it has hit is unsuitable.
   'Displaced stepping' is a tactic for stepping one thread past a
   breakpoint it has hit while ensuring that other threads running
   concurrently will hit the breakpoint as they should.

   The traditional way to step a thread T off a breakpoint in a
   multi-threaded program in all-stop mode is as follows:

   a0) Initially, all threads are stopped, and breakpoints are not
       inserted.
   a1) We single-step T, leaving breakpoints uninserted.
   a2) We insert breakpoints, and resume all threads.

   In non-stop debugging, however, this strategy is unsuitable: we
   don't want to have to stop all threads in the system in order to
   continue or step T past a breakpoint.  Instead, we use displaced
   stepping:

   n0) Initially, T is stopped, other threads are running, and
       breakpoints are inserted.
   n1) We copy the instruction "under" the breakpoint to a separate
       location, outside the main code stream, making any adjustments
       to the instruction, register, and memory state as directed by
       T's architecture.
   n2) We single-step T over the instruction at its new location.
   n3) We adjust the resulting register and memory state as directed
       by T's architecture.  This includes resetting T's PC to point
       back into the main instruction stream.
   n4) We resume T.

   This approach depends on the following gdbarch methods:

   - gdbarch_max_insn_length and gdbarch_displaced_step_location
     indicate where to copy the instruction, and how much space must
     be reserved there.  We use these in step n1.

   - gdbarch_displaced_step_copy_insn copies a instruction to a new
     address, and makes any necessary adjustments to the instruction,
     register contents, and memory.  We use this in step n1.

   - gdbarch_displaced_step_fixup adjusts registers and memory after
     we have successfully single-stepped the instruction, to yield the
     same effect the instruction would have had if we had executed it
     at its original address.  We use this in step n3.

   The gdbarch_displaced_step_copy_insn and
   gdbarch_displaced_step_fixup functions must be written so that
   copying an instruction with gdbarch_displaced_step_copy_insn,
   single-stepping across the copied instruction, and then applying
   gdbarch_displaced_insn_fixup should have the same effects on the
   thread's memory and registers as stepping the instruction in place
   would have.  Exactly which responsibilities fall to the copy and
   which fall to the fixup is up to the author of those functions.

   See the comments in gdbarch.sh for details.

   Note that displaced stepping and software single-step cannot
   currently be used in combination, although with some care I think
   they could be made to.  Software single-step works by placing
   breakpoints on all possible subsequent instructions; if the
   displaced instruction is a PC-relative jump, those breakpoints
   could fall in very strange places --- on pages that aren't
   executable, or at addresses that are not proper instruction
   boundaries.  (We do generally let other threads run while we wait
   to hit the software single-step breakpoint, and they might
   encounter such a corrupted instruction.)  One way to work around
   this would be to have gdbarch_displaced_step_copy_insn fully
   simulate the effect of PC-relative instructions (and return NULL)
   on architectures that use software single-stepping.

   In non-stop mode, we can have independent and simultaneous step
   requests, so more than one thread may need to simultaneously step
   over a breakpoint.  The current implementation assumes there is
   only one scratch space per process.  In this case, we have to
   serialize access to the scratch space.  If thread A wants to step
   over a breakpoint, but we are currently waiting for some other
   thread to complete a displaced step, we leave thread A stopped and
   place it in the displaced_step_request_queue.  Whenever a displaced
   step finishes, we pick the next thread in the queue and start a new
   displaced step operation on it.  See displaced_step_prepare and
   displaced_step_finish for details.  */

/* Return true if THREAD is doing a displaced step.  */

static bool
displaced_step_in_progress_thread (thread_info *thread)
{
  gdb_assert (thread != nullptr);

  return thread->displaced_step_state.in_progress ();
}

/* Return true if INF has a thread doing a displaced step.  */

static bool
displaced_step_in_progress (inferior *inf)
{
  return inf->displaced_step_state.in_progress_count > 0;
}

/* Return true if any thread is doing a displaced step.  */

static bool
displaced_step_in_progress_any_thread ()
{
  for (inferior *inf : all_non_exited_inferiors ())
    {
      if (displaced_step_in_progress (inf))
	return true;
    }

  return false;
}

static void
infrun_inferior_exit (struct inferior *inf)
{
  inf->displaced_step_state.reset ();
  inf->thread_waiting_for_vfork_done = nullptr;
}

static void
infrun_inferior_execd (inferior *exec_inf, inferior *follow_inf)
{
  /* If some threads where was doing a displaced step in this inferior at the
     moment of the exec, they no longer exist.  Even if the exec'ing thread
     doing a displaced step, we don't want to to any fixup nor restore displaced
     stepping buffer bytes.  */
  follow_inf->displaced_step_state.reset ();

  for (thread_info *thread : follow_inf->threads ())
    thread->displaced_step_state.reset ();

  /* Since an in-line step is done with everything else stopped, if there was
     one in progress at the time of the exec, it must have been the exec'ing
     thread.  */
  clear_step_over_info ();

  follow_inf->thread_waiting_for_vfork_done = nullptr;
}

/* If ON, and the architecture supports it, GDB will use displaced
   stepping to step over breakpoints.  If OFF, or if the architecture
   doesn't support it, GDB will instead use the traditional
   hold-and-step approach.  If AUTO (which is the default), GDB will
   decide which technique to use to step over breakpoints depending on
   whether the target works in a non-stop way (see use_displaced_stepping).  */

static enum auto_boolean can_use_displaced_stepping = AUTO_BOOLEAN_AUTO;

static void
show_can_use_displaced_stepping (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c,
				 const char *value)
{
  if (can_use_displaced_stepping == AUTO_BOOLEAN_AUTO)
    gdb_printf (file,
		_("Debugger's willingness to use displaced stepping "
		  "to step over breakpoints is %s (currently %s).\n"),
		value, target_is_non_stop_p () ? "on" : "off");
  else
    gdb_printf (file,
		_("Debugger's willingness to use displaced stepping "
		  "to step over breakpoints is %s.\n"), value);
}

/* Return true if the gdbarch implements the required methods to use
   displaced stepping.  */

static bool
gdbarch_supports_displaced_stepping (gdbarch *arch)
{
  /* Only check for the presence of `prepare`.  The gdbarch verification ensures
     that if `prepare` is provided, so is `finish`.  */
  return gdbarch_displaced_step_prepare_p (arch);
}

/* Return non-zero if displaced stepping can/should be used to step
   over breakpoints of thread TP.  */

static bool
use_displaced_stepping (thread_info *tp)
{
  /* If the user disabled it explicitly, don't use displaced stepping.  */
  if (can_use_displaced_stepping == AUTO_BOOLEAN_FALSE)
    return false;

  /* If "auto", only use displaced stepping if the target operates in a non-stop
     way.  */
  if (can_use_displaced_stepping == AUTO_BOOLEAN_AUTO
      && !target_is_non_stop_p ())
    return false;

  gdbarch *gdbarch = get_thread_regcache (tp)->arch ();

  /* If the architecture doesn't implement displaced stepping, don't use
     it.  */
  if (!gdbarch_supports_displaced_stepping (gdbarch))
    return false;

  /* If recording, don't use displaced stepping.  */
  if (find_record_target () != nullptr)
    return false;

  /* If displaced stepping failed before for this inferior, don't bother trying
     again.  */
  if (tp->inf->displaced_step_state.failed_before)
    return false;

  return true;
}

/* Simple function wrapper around displaced_step_thread_state::reset.  */

static void
displaced_step_reset (displaced_step_thread_state *displaced)
{
  displaced->reset ();
}

/* A cleanup that wraps displaced_step_reset.  We use this instead of, say,
   SCOPE_EXIT, because it needs to be discardable with "cleanup.release ()".  */

using displaced_step_reset_cleanup = FORWARD_SCOPE_EXIT (displaced_step_reset);

/* Prepare to single-step, using displaced stepping.

   Note that we cannot use displaced stepping when we have a signal to
   deliver.  If we have a signal to deliver and an instruction to step
   over, then after the step, there will be no indication from the
   target whether the thread entered a signal handler or ignored the
   signal and stepped over the instruction successfully --- both cases
   result in a simple SIGTRAP.  In the first case we mustn't do a
   fixup, and in the second case we must --- but we can't tell which.
   Comments in the code for 'random signals' in handle_inferior_event
   explain how we handle this case instead.

   Returns DISPLACED_STEP_PREPARE_STATUS_OK if preparing was successful -- this
   thread is going to be stepped now; DISPLACED_STEP_PREPARE_STATUS_UNAVAILABLE
   if displaced stepping this thread got queued; or
   DISPLACED_STEP_PREPARE_STATUS_CANT if this instruction can't be displaced
   stepped.  */

static displaced_step_prepare_status
displaced_step_prepare_throw (thread_info *tp)
{
  regcache *regcache = get_thread_regcache (tp);
  struct gdbarch *gdbarch = regcache->arch ();
  displaced_step_thread_state &disp_step_thread_state
    = tp->displaced_step_state;

  /* We should never reach this function if the architecture does not
     support displaced stepping.  */
  gdb_assert (gdbarch_supports_displaced_stepping (gdbarch));

  /* Nor if the thread isn't meant to step over a breakpoint.  */
  gdb_assert (tp->control.trap_expected);

  /* Disable range stepping while executing in the scratch pad.  We
     want a single-step even if executing the displaced instruction in
     the scratch buffer lands within the stepping range (e.g., a
     jump/branch).  */
  tp->control.may_range_step = 0;

  /* We are about to start a displaced step for this thread.  If one is already
     in progress, something's wrong.  */
  gdb_assert (!disp_step_thread_state.in_progress ());

  if (tp->inf->displaced_step_state.unavailable)
    {
      /* The gdbarch tells us it's not worth asking to try a prepare because
	 it is likely that it will return unavailable, so don't bother asking.  */

      displaced_debug_printf ("deferring step of %s",
			      tp->ptid.to_string ().c_str ());

      global_thread_step_over_chain_enqueue (tp);
      return DISPLACED_STEP_PREPARE_STATUS_UNAVAILABLE;
    }

  displaced_debug_printf ("displaced-stepping %s now",
			  tp->ptid.to_string ().c_str ());

  scoped_restore_current_thread restore_thread;

  switch_to_thread (tp);

  CORE_ADDR original_pc = regcache_read_pc (regcache);
  CORE_ADDR displaced_pc;

  /* Display the instruction we are going to displaced step.  */
  if (debug_displaced)
    {
      string_file tmp_stream;
      int dislen = gdb_print_insn (gdbarch, original_pc, &tmp_stream,
				   nullptr);

      if (dislen > 0)
	{
	  gdb::byte_vector insn_buf (dislen);
	  read_memory (original_pc, insn_buf.data (), insn_buf.size ());

	  std::string insn_bytes = bytes_to_string (insn_buf);

	  displaced_debug_printf ("original insn %s: %s \t %s",
				  paddress (gdbarch, original_pc),
				  insn_bytes.c_str (),
				  tmp_stream.string ().c_str ());
	}
      else
	displaced_debug_printf ("original insn %s: invalid length: %d",
				paddress (gdbarch, original_pc), dislen);
    }

  displaced_step_prepare_status status
    = gdbarch_displaced_step_prepare (gdbarch, tp, displaced_pc);

  if (status == DISPLACED_STEP_PREPARE_STATUS_CANT)
    {
      displaced_debug_printf ("failed to prepare (%s)",
			      tp->ptid.to_string ().c_str ());

      return DISPLACED_STEP_PREPARE_STATUS_CANT;
    }
  else if (status == DISPLACED_STEP_PREPARE_STATUS_UNAVAILABLE)
    {
      /* Not enough displaced stepping resources available, defer this
	 request by placing it the queue.  */

      displaced_debug_printf ("not enough resources available, "
			      "deferring step of %s",
			      tp->ptid.to_string ().c_str ());

      global_thread_step_over_chain_enqueue (tp);

      return DISPLACED_STEP_PREPARE_STATUS_UNAVAILABLE;
    }

  gdb_assert (status == DISPLACED_STEP_PREPARE_STATUS_OK);

  /* Save the information we need to fix things up if the step
     succeeds.  */
  disp_step_thread_state.set (gdbarch);

  tp->inf->displaced_step_state.in_progress_count++;

  displaced_debug_printf ("prepared successfully thread=%s, "
			  "original_pc=%s, displaced_pc=%s",
			  tp->ptid.to_string ().c_str (),
			  paddress (gdbarch, original_pc),
			  paddress (gdbarch, displaced_pc));

  /* Display the new displaced instruction(s).  */
  if (debug_displaced)
    {
      string_file tmp_stream;
      CORE_ADDR addr = displaced_pc;

      /* If displaced stepping is going to use h/w single step then we know
	 that the replacement instruction can only be a single instruction,
	 in that case set the end address at the next byte.

	 Otherwise the displaced stepping copy instruction routine could
	 have generated multiple instructions, and all we know is that they
	 must fit within the LEN bytes of the buffer.  */
      CORE_ADDR end
	= addr + (gdbarch_displaced_step_hw_singlestep (gdbarch)
		  ? 1 : gdbarch_displaced_step_buffer_length (gdbarch));

      while (addr < end)
	{
	  int dislen = gdb_print_insn (gdbarch, addr, &tmp_stream, nullptr);
	  if (dislen <= 0)
	    {
	      displaced_debug_printf
		("replacement insn %s: invalid length: %d",
		 paddress (gdbarch, addr), dislen);
	      break;
	    }

	  gdb::byte_vector insn_buf (dislen);
	  read_memory (addr, insn_buf.data (), insn_buf.size ());

	  std::string insn_bytes = bytes_to_string (insn_buf);
	  std::string insn_str = tmp_stream.release ();
	  displaced_debug_printf ("replacement insn %s: %s \t %s",
				  paddress (gdbarch, addr),
				  insn_bytes.c_str (),
				  insn_str.c_str ());
	  addr += dislen;
	}
    }

  return DISPLACED_STEP_PREPARE_STATUS_OK;
}

/* Wrapper for displaced_step_prepare_throw that disabled further
   attempts at displaced stepping if we get a memory error.  */

static displaced_step_prepare_status
displaced_step_prepare (thread_info *thread)
{
  displaced_step_prepare_status status
    = DISPLACED_STEP_PREPARE_STATUS_CANT;

  try
    {
      status = displaced_step_prepare_throw (thread);
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error != MEMORY_ERROR
	  && ex.error != NOT_SUPPORTED_ERROR)
	throw;

      infrun_debug_printf ("caught exception, disabling displaced stepping: %s",
			   ex.what ());

      /* Be verbose if "set displaced-stepping" is "on", silent if
	 "auto".  */
      if (can_use_displaced_stepping == AUTO_BOOLEAN_TRUE)
	{
	  warning (_("disabling displaced stepping: %s"),
		   ex.what ());
	}

      /* Disable further displaced stepping attempts.  */
      thread->inf->displaced_step_state.failed_before = 1;
    }

  return status;
}

/* True if any thread of TARGET that matches RESUME_PTID requires
   target_thread_events enabled.  This assumes TARGET does not support
   target thread options.  */

static bool
any_thread_needs_target_thread_events (process_stratum_target *target,
				       ptid_t resume_ptid)
{
  for (thread_info *tp : all_non_exited_threads (target, resume_ptid))
    if (displaced_step_in_progress_thread (tp)
	|| schedlock_applies (tp)
	|| tp->thread_fsm () != nullptr)
      return true;
  return false;
}

/* Maybe disable thread-{cloned,created,exited} event reporting after
   a step-over (either in-line or displaced) finishes.  */

static void
update_thread_events_after_step_over (thread_info *event_thread,
				      const target_waitstatus &event_status)
{
  if (schedlock_applies (event_thread))
    {
      /* If scheduler-locking applies, continue reporting
	 thread-created/thread-cloned events.  */
      return;
    }
  else if (target_supports_set_thread_options (0))
    {
      /* We can control per-thread options.  Disable events for the
	 event thread, unless the thread is gone.  */
      if (event_status.kind () != TARGET_WAITKIND_THREAD_EXITED)
	event_thread->set_thread_options (0);
    }
  else
    {
      /* We can only control the target-wide target_thread_events
	 setting.  Disable it, but only if other threads in the target
	 don't need it enabled.  */
      process_stratum_target *target = event_thread->inf->process_target ();
      if (!any_thread_needs_target_thread_events (target, minus_one_ptid))
	target_thread_events (false);
    }
}

/* If we displaced stepped an instruction successfully, adjust registers and
   memory to yield the same effect the instruction would have had if we had
   executed it at its original address, and return
   DISPLACED_STEP_FINISH_STATUS_OK.  If the instruction didn't complete,
   relocate the PC and return DISPLACED_STEP_FINISH_STATUS_NOT_EXECUTED.

   If the thread wasn't displaced stepping, return
   DISPLACED_STEP_FINISH_STATUS_OK as well.  */

static displaced_step_finish_status
displaced_step_finish (thread_info *event_thread,
		       const target_waitstatus &event_status)
{
  /* Check whether the parent is displaced stepping.  */
  inferior *parent_inf = event_thread->inf;

  /* If this was a fork/vfork/clone, this event indicates that the
     displaced stepping of the syscall instruction has been done, so
     we perform cleanup for parent here.  Also note that this
     operation also cleans up the child for vfork, because their pages
     are shared.  */

  /* If this is a fork (child gets its own address space copy) and
     some displaced step buffers were in use at the time of the fork,
     restore the displaced step buffer bytes in the child process.

     Architectures which support displaced stepping and fork events
     must supply an implementation of
     gdbarch_displaced_step_restore_all_in_ptid.  This is not enforced
     during gdbarch validation to support architectures which support
     displaced stepping but not forks.  */
  if (event_status.kind () == TARGET_WAITKIND_FORKED)
    {
      struct regcache *parent_regcache = get_thread_regcache (event_thread);
      struct gdbarch *gdbarch = parent_regcache->arch ();

      if (gdbarch_supports_displaced_stepping (gdbarch))
	gdbarch_displaced_step_restore_all_in_ptid
	  (gdbarch, parent_inf, event_status.child_ptid ());
    }

  displaced_step_thread_state *displaced = &event_thread->displaced_step_state;

  /* Was this thread performing a displaced step?  */
  if (!displaced->in_progress ())
    return DISPLACED_STEP_FINISH_STATUS_OK;

  update_thread_events_after_step_over (event_thread, event_status);

  gdb_assert (event_thread->inf->displaced_step_state.in_progress_count > 0);
  event_thread->inf->displaced_step_state.in_progress_count--;

  /* Fixup may need to read memory/registers.  Switch to the thread
     that we're fixing up.  Also, target_stopped_by_watchpoint checks
     the current thread, and displaced_step_restore performs ptid-dependent
     memory accesses using current_inferior().  */
  switch_to_thread (event_thread);

  displaced_step_reset_cleanup cleanup (displaced);

  /* Do the fixup, and release the resources acquired to do the displaced
     step. */
  displaced_step_finish_status status
    = gdbarch_displaced_step_finish (displaced->get_original_gdbarch (),
				     event_thread, event_status);

  if (event_status.kind () == TARGET_WAITKIND_FORKED
      || event_status.kind () == TARGET_WAITKIND_VFORKED
      || event_status.kind () == TARGET_WAITKIND_THREAD_CLONED)
    {
      /* Since the vfork/fork/clone syscall instruction was executed
	 in the scratchpad, the child's PC is also within the
	 scratchpad.  Set the child's PC to the parent's PC value,
	 which has already been fixed up.  Note: we use the parent's
	 aspace here, although we're touching the child, because the
	 child hasn't been added to the inferior list yet at this
	 point.  */

      struct regcache *parent_regcache = get_thread_regcache (event_thread);
      struct gdbarch *gdbarch = parent_regcache->arch ();
      struct regcache *child_regcache
	= get_thread_arch_regcache (parent_inf, event_status.child_ptid (),
				    gdbarch);
      /* Read PC value of parent.  */
      CORE_ADDR parent_pc = regcache_read_pc (parent_regcache);

      displaced_debug_printf ("write child pc from %s to %s",
			      paddress (gdbarch,
					regcache_read_pc (child_regcache)),
			      paddress (gdbarch, parent_pc));

      regcache_write_pc (child_regcache, parent_pc);
    }

  return status;
}

/* Data to be passed around while handling an event.  This data is
   discarded between events.  */
struct execution_control_state
{
  explicit execution_control_state (thread_info *thr = nullptr)
    : ptid (thr == nullptr ? null_ptid : thr->ptid),
      event_thread (thr)
  {
  }

  process_stratum_target *target = nullptr;
  ptid_t ptid;
  /* The thread that got the event, if this was a thread event; NULL
     otherwise.  */
  struct thread_info *event_thread;

  struct target_waitstatus ws;
  int stop_func_filled_in = 0;
  CORE_ADDR stop_func_alt_start = 0;
  CORE_ADDR stop_func_start = 0;
  CORE_ADDR stop_func_end = 0;
  const char *stop_func_name = nullptr;
  int wait_some_more = 0;

  /* True if the event thread hit the single-step breakpoint of
     another thread.  Thus the event doesn't cause a stop, the thread
     needs to be single-stepped past the single-step breakpoint before
     we can switch back to the original stepping thread.  */
  int hit_singlestep_breakpoint = 0;
};

static void keep_going_pass_signal (struct execution_control_state *ecs);
static void prepare_to_wait (struct execution_control_state *ecs);
static bool keep_going_stepped_thread (struct thread_info *tp);
static step_over_what thread_still_needs_step_over (struct thread_info *tp);

/* Are there any pending step-over requests?  If so, run all we can
   now and return true.  Otherwise, return false.  */

static bool
start_step_over (void)
{
  INFRUN_SCOPED_DEBUG_ENTER_EXIT;

  /* Don't start a new step-over if we already have an in-line
     step-over operation ongoing.  */
  if (step_over_info_valid_p ())
    return false;

  /* Steal the global thread step over chain.  As we try to initiate displaced
     steps, threads will be enqueued in the global chain if no buffers are
     available.  If we iterated on the global chain directly, we might iterate
     indefinitely.  */
  thread_step_over_list threads_to_step
    = std::move (global_thread_step_over_list);

  infrun_debug_printf ("stealing global queue of threads to step, length = %d",
		       thread_step_over_chain_length (threads_to_step));

  bool started = false;

  /* On scope exit (whatever the reason, return or exception), if there are
     threads left in the THREADS_TO_STEP chain, put back these threads in the
     global list.  */
  SCOPE_EXIT
    {
      if (threads_to_step.empty ())
	infrun_debug_printf ("step-over queue now empty");
      else
	{
	  infrun_debug_printf ("putting back %d threads to step in global queue",
			       thread_step_over_chain_length (threads_to_step));

	  global_thread_step_over_chain_enqueue_chain
	    (std::move (threads_to_step));
	}
    };

  thread_step_over_list_safe_range range
    = make_thread_step_over_list_safe_range (threads_to_step);

  for (thread_info *tp : range)
    {
      step_over_what step_what;
      int must_be_in_line;

      gdb_assert (!tp->stop_requested);

      if (tp->inf->displaced_step_state.unavailable)
	{
	  /* The arch told us to not even try preparing another displaced step
	     for this inferior.  Just leave the thread in THREADS_TO_STEP, it
	     will get moved to the global chain on scope exit.  */
	  continue;
	}

      if (tp->inf->thread_waiting_for_vfork_done != nullptr)
	{
	  /* When we stop all threads, handling a vfork, any thread in the step
	     over chain remains there.  A user could also try to continue a
	     thread stopped at a breakpoint while another thread is waiting for
	     a vfork-done event.  In any case, we don't want to start a step
	     over right now.  */
	  continue;
	}

      /* Remove thread from the THREADS_TO_STEP chain.  If anything goes wrong
	 while we try to prepare the displaced step, we don't add it back to
	 the global step over chain.  This is to avoid a thread staying in the
	 step over chain indefinitely if something goes wrong when resuming it
	 If the error is intermittent and it still needs a step over, it will
	 get enqueued again when we try to resume it normally.  */
      threads_to_step.erase (threads_to_step.iterator_to (*tp));

      step_what = thread_still_needs_step_over (tp);
      must_be_in_line = ((step_what & STEP_OVER_WATCHPOINT)
			 || ((step_what & STEP_OVER_BREAKPOINT)
			     && !use_displaced_stepping (tp)));

      /* We currently stop all threads of all processes to step-over
	 in-line.  If we need to start a new in-line step-over, let
	 any pending displaced steps finish first.  */
      if (must_be_in_line && displaced_step_in_progress_any_thread ())
	{
	  global_thread_step_over_chain_enqueue (tp);
	  continue;
	}

      if (tp->control.trap_expected
	  || tp->resumed ()
	  || tp->executing ())
	{
	  internal_error ("[%s] has inconsistent state: "
			  "trap_expected=%d, resumed=%d, executing=%d\n",
			  tp->ptid.to_string ().c_str (),
			  tp->control.trap_expected,
			  tp->resumed (),
			  tp->executing ());
	}

      infrun_debug_printf ("resuming [%s] for step-over",
			   tp->ptid.to_string ().c_str ());

      /* keep_going_pass_signal skips the step-over if the breakpoint
	 is no longer inserted.  In all-stop, we want to keep looking
	 for a thread that needs a step-over instead of resuming TP,
	 because we wouldn't be able to resume anything else until the
	 target stops again.  In non-stop, the resume always resumes
	 only TP, so it's OK to let the thread resume freely.  */
      if (!target_is_non_stop_p () && !step_what)
	continue;

      switch_to_thread (tp);
      execution_control_state ecs (tp);
      keep_going_pass_signal (&ecs);

      if (!ecs.wait_some_more)
	error (_("Command aborted."));

      /* If the thread's step over could not be initiated because no buffers
	 were available, it was re-added to the global step over chain.  */
      if (tp->resumed  ())
	{
	  infrun_debug_printf ("[%s] was resumed.",
			       tp->ptid.to_string ().c_str ());
	  gdb_assert (!thread_is_in_step_over_chain (tp));
	}
      else
	{
	  infrun_debug_printf ("[%s] was NOT resumed.",
			       tp->ptid.to_string ().c_str ());
	  gdb_assert (thread_is_in_step_over_chain (tp));
	}

      /* If we started a new in-line step-over, we're done.  */
      if (step_over_info_valid_p ())
	{
	  gdb_assert (tp->control.trap_expected);
	  started = true;
	  break;
	}

      if (!target_is_non_stop_p ())
	{
	  /* On all-stop, shouldn't have resumed unless we needed a
	     step over.  */
	  gdb_assert (tp->control.trap_expected
		      || tp->step_after_step_resume_breakpoint);

	  /* With remote targets (at least), in all-stop, we can't
	     issue any further remote commands until the program stops
	     again.  */
	  started = true;
	  break;
	}

      /* Either the thread no longer needed a step-over, or a new
	 displaced stepping sequence started.  Even in the latter
	 case, continue looking.  Maybe we can also start another
	 displaced step on a thread of other process. */
    }

  return started;
}

/* Update global variables holding ptids to hold NEW_PTID if they were
   holding OLD_PTID.  */
static void
infrun_thread_ptid_changed (process_stratum_target *target,
			    ptid_t old_ptid, ptid_t new_ptid)
{
  if (inferior_ptid == old_ptid
      && current_inferior ()->process_target () == target)
    inferior_ptid = new_ptid;
}



static const char schedlock_off[] = "off";
static const char schedlock_on[] = "on";
static const char schedlock_step[] = "step";
static const char schedlock_replay[] = "replay";
static const char *const scheduler_enums[] = {
  schedlock_off,
  schedlock_on,
  schedlock_step,
  schedlock_replay,
  nullptr
};
static const char *scheduler_mode = schedlock_replay;
static void
show_scheduler_mode (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Mode for locking scheduler "
		"during execution is \"%s\".\n"),
	      value);
}

static void
set_schedlock_func (const char *args, int from_tty, struct cmd_list_element *c)
{
  if (!target_can_lock_scheduler ())
    {
      scheduler_mode = schedlock_off;
      error (_("Target '%s' cannot support this command."),
	     target_shortname ());
    }
}

/* True if execution commands resume all threads of all processes by
   default; otherwise, resume only threads of the current inferior
   process.  */
bool sched_multi = false;

/* Try to setup for software single stepping.  Return true if target_resume()
   should use hardware single step.

   GDBARCH the current gdbarch.  */

static bool
maybe_software_singlestep (struct gdbarch *gdbarch)
{
  bool hw_step = true;

  if (execution_direction == EXEC_FORWARD
      && gdbarch_software_single_step_p (gdbarch))
    hw_step = !insert_single_step_breakpoints (gdbarch);

  return hw_step;
}

/* See infrun.h.  */

ptid_t
user_visible_resume_ptid (int step)
{
  ptid_t resume_ptid;

  if (non_stop)
    {
      /* With non-stop mode on, threads are always handled
	 individually.  */
      resume_ptid = inferior_ptid;
    }
  else if ((scheduler_mode == schedlock_on)
	   || (scheduler_mode == schedlock_step && step))
    {
      /* User-settable 'scheduler' mode requires solo thread
	 resume.  */
      resume_ptid = inferior_ptid;
    }
  else if ((scheduler_mode == schedlock_replay)
	   && target_record_will_replay (minus_one_ptid, execution_direction))
    {
      /* User-settable 'scheduler' mode requires solo thread resume in replay
	 mode.  */
      resume_ptid = inferior_ptid;
    }
  else if (!sched_multi && target_supports_multi_process ())
    {
      /* Resume all threads of the current process (and none of other
	 processes).  */
      resume_ptid = ptid_t (inferior_ptid.pid ());
    }
  else
    {
      /* Resume all threads of all processes.  */
      resume_ptid = RESUME_ALL;
    }

  return resume_ptid;
}

/* See infrun.h.  */

process_stratum_target *
user_visible_resume_target (ptid_t resume_ptid)
{
  return (resume_ptid == minus_one_ptid && sched_multi
	  ? nullptr
	  : current_inferior ()->process_target ());
}

/* Find a thread from the inferiors that we'll resume that is waiting
   for a vfork-done event.  */

static thread_info *
find_thread_waiting_for_vfork_done ()
{
  gdb_assert (!target_is_non_stop_p ());

  if (sched_multi)
    {
      for (inferior *inf : all_non_exited_inferiors ())
	if (inf->thread_waiting_for_vfork_done != nullptr)
	  return inf->thread_waiting_for_vfork_done;
    }
  else
    {
      inferior *cur_inf = current_inferior ();
      if (cur_inf->thread_waiting_for_vfork_done != nullptr)
	return cur_inf->thread_waiting_for_vfork_done;
    }
  return nullptr;
}

/* Return a ptid representing the set of threads that we will resume,
   in the perspective of the target, assuming run control handling
   does not require leaving some threads stopped (e.g., stepping past
   breakpoint).  USER_STEP indicates whether we're about to start the
   target for a stepping command.  */

static ptid_t
internal_resume_ptid (int user_step)
{
  /* In non-stop, we always control threads individually.  Note that
     the target may always work in non-stop mode even with "set
     non-stop off", in which case user_visible_resume_ptid could
     return a wildcard ptid.  */
  if (target_is_non_stop_p ())
    return inferior_ptid;

  /* The rest of the function assumes non-stop==off and
     target-non-stop==off.

     If a thread is waiting for a vfork-done event, it means breakpoints are out
     for this inferior (well, program space in fact).  We don't want to resume
     any thread other than the one waiting for vfork done, otherwise these other
     threads could miss breakpoints.  So if a thread in the resumption set is
     waiting for a vfork-done event, resume only that thread.

     The resumption set width depends on whether schedule-multiple is on or off.

     Note that if the target_resume interface was more flexible, we could be
     smarter here when schedule-multiple is on.  For example, imagine 3
     inferiors with 2 threads each (1.1, 1.2, 2.1, 2.2, 3.1 and 3.2).  Threads
     2.1 and 3.2 are both waiting for a vfork-done event.  Then we could ask the
     target(s) to resume:

      - All threads of inferior 1
      - Thread 2.1
      - Thread 3.2

     Since we don't have that flexibility (we can only pass one ptid), just
     resume the first thread waiting for a vfork-done event we find (e.g. thread
     2.1).  */
  thread_info *thr = find_thread_waiting_for_vfork_done ();
  if (thr != nullptr)
    {
      /* If we have a thread that is waiting for a vfork-done event,
	 then we should have switched to it earlier.  Calling
	 target_resume with thread scope is only possible when the
	 current thread matches the thread scope.  */
      gdb_assert (thr->ptid == inferior_ptid);
      gdb_assert (thr->inf->process_target ()
		  == inferior_thread ()->inf->process_target ());
      return thr->ptid;
    }

  return user_visible_resume_ptid (user_step);
}

/* Wrapper for target_resume, that handles infrun-specific
   bookkeeping.  */

static void
do_target_resume (ptid_t resume_ptid, bool step, enum gdb_signal sig)
{
  struct thread_info *tp = inferior_thread ();

  gdb_assert (!tp->stop_requested);

  /* Install inferior's terminal modes.  */
  target_terminal::inferior ();

  /* Avoid confusing the next resume, if the next stop/resume
     happens to apply to another thread.  */
  tp->set_stop_signal (GDB_SIGNAL_0);

  /* Advise target which signals may be handled silently.

     If we have removed breakpoints because we are stepping over one
     in-line (in any thread), we need to receive all signals to avoid
     accidentally skipping a breakpoint during execution of a signal
     handler.

     Likewise if we're displaced stepping, otherwise a trap for a
     breakpoint in a signal handler might be confused with the
     displaced step finishing.  We don't make the displaced_step_finish
     step distinguish the cases instead, because:

     - a backtrace while stopped in the signal handler would show the
       scratch pad as frame older than the signal handler, instead of
       the real mainline code.

     - when the thread is later resumed, the signal handler would
       return to the scratch pad area, which would no longer be
       valid.  */
  if (step_over_info_valid_p ()
      || displaced_step_in_progress (tp->inf))
    target_pass_signals ({});
  else
    target_pass_signals (signal_pass);

  /* Request that the target report thread-{created,cloned,exited}
     events in the following situations:

     - If we are performing an in-line step-over-breakpoint, then we
       will remove a breakpoint from the target and only run the
       current thread.  We don't want any new thread (spawned by the
       step) to start running, as it might miss the breakpoint.  We
       need to clear the step-over state if the stepped thread exits,
       so we also enable thread-exit events.

     - If we are stepping over a breakpoint out of line (displaced
       stepping) then we won't remove a breakpoint from the target,
       but, if the step spawns a new clone thread, then we will need
       to fixup the $pc address in the clone child too, so we need it
       to start stopped.  We need to release the displaced stepping
       buffer if the stepped thread exits, so we also enable
       thread-exit events.

     - If scheduler-locking applies, threads that the current thread
       spawns should remain halted.  It's not strictly necessary to
       enable thread-exit events in this case, but it doesn't hurt.
  */
  if (step_over_info_valid_p ()
      || displaced_step_in_progress_thread (tp)
      || schedlock_applies (tp))
    {
      gdb_thread_options options
	= GDB_THREAD_OPTION_CLONE | GDB_THREAD_OPTION_EXIT;
      if (target_supports_set_thread_options (options))
	tp->set_thread_options (options);
      else
	target_thread_events (true);
    }
  else if (tp->thread_fsm () != nullptr)
    {
      gdb_thread_options options = GDB_THREAD_OPTION_EXIT;
      if (target_supports_set_thread_options (options))
	tp->set_thread_options (options);
      else
	target_thread_events (true);
    }
  else
    {
      if (target_supports_set_thread_options (0))
	tp->set_thread_options (0);
      else
	{
	  process_stratum_target *resume_target = tp->inf->process_target ();
	  if (!any_thread_needs_target_thread_events (resume_target,
						      resume_ptid))
	    target_thread_events (false);
	}
    }

  /* If we're resuming more than one thread simultaneously, then any
     thread other than the leader is being set to run free.  Clear any
     previous thread option for those threads.  */
  if (resume_ptid != inferior_ptid && target_supports_set_thread_options (0))
    {
      process_stratum_target *resume_target = tp->inf->process_target ();
      for (thread_info *thr_iter : all_non_exited_threads (resume_target,
							   resume_ptid))
	if (thr_iter != tp)
	  thr_iter->set_thread_options (0);
    }

  infrun_debug_printf ("resume_ptid=%s, step=%d, sig=%s",
		       resume_ptid.to_string ().c_str (),
		       step, gdb_signal_to_symbol_string (sig));

  target_resume (resume_ptid, step, sig);
}

/* Resume the inferior.  SIG is the signal to give the inferior
   (GDB_SIGNAL_0 for none).  Note: don't call this directly; instead
   call 'resume', which handles exceptions.  */

static void
resume_1 (enum gdb_signal sig)
{
  struct thread_info *tp = inferior_thread ();
  regcache *regcache = get_thread_regcache (tp);
  struct gdbarch *gdbarch = regcache->arch ();
  ptid_t resume_ptid;
  /* This represents the user's step vs continue request.  When
     deciding whether "set scheduler-locking step" applies, it's the
     user's intention that counts.  */
  const int user_step = tp->control.stepping_command;
  /* This represents what we'll actually request the target to do.
     This can decay from a step to a continue, if e.g., we need to
     implement single-stepping with breakpoints (software
     single-step).  */
  bool step;

  gdb_assert (!tp->stop_requested);
  gdb_assert (!thread_is_in_step_over_chain (tp));

  if (tp->has_pending_waitstatus ())
    {
      infrun_debug_printf
	("thread %s has pending wait "
	 "status %s (currently_stepping=%d).",
	 tp->ptid.to_string ().c_str (),
	 tp->pending_waitstatus ().to_string ().c_str (),
	 currently_stepping (tp));

      tp->inf->process_target ()->threads_executing = true;
      tp->set_resumed (true);

      /* FIXME: What should we do if we are supposed to resume this
	 thread with a signal?  Maybe we should maintain a queue of
	 pending signals to deliver.  */
      if (sig != GDB_SIGNAL_0)
	{
	  warning (_("Couldn't deliver signal %s to %s."),
		   gdb_signal_to_name (sig),
		   tp->ptid.to_string ().c_str ());
	}

      tp->set_stop_signal (GDB_SIGNAL_0);

      if (target_can_async_p ())
	{
	  target_async (true);
	  /* Tell the event loop we have an event to process. */
	  mark_async_event_handler (infrun_async_inferior_event_token);
	}
      return;
    }

  tp->stepped_breakpoint = 0;

  /* Depends on stepped_breakpoint.  */
  step = currently_stepping (tp);

  if (current_inferior ()->thread_waiting_for_vfork_done != nullptr)
    {
      /* Don't try to single-step a vfork parent that is waiting for
	 the child to get out of the shared memory region (by exec'ing
	 or exiting).  This is particularly important on software
	 single-step archs, as the child process would trip on the
	 software single step breakpoint inserted for the parent
	 process.  Since the parent will not actually execute any
	 instruction until the child is out of the shared region (such
	 are vfork's semantics), it is safe to simply continue it.
	 Eventually, we'll see a TARGET_WAITKIND_VFORK_DONE event for
	 the parent, and tell it to `keep_going', which automatically
	 re-sets it stepping.  */
      infrun_debug_printf ("resume : clear step");
      step = false;
    }

  CORE_ADDR pc = regcache_read_pc (regcache);

  infrun_debug_printf ("step=%d, signal=%s, trap_expected=%d, "
		       "current thread [%s] at %s",
		       step, gdb_signal_to_symbol_string (sig),
		       tp->control.trap_expected,
		       inferior_ptid.to_string ().c_str (),
		       paddress (gdbarch, pc));

  const address_space *aspace = tp->inf->aspace.get ();

  /* Normally, by the time we reach `resume', the breakpoints are either
     removed or inserted, as appropriate.  The exception is if we're sitting
     at a permanent breakpoint; we need to step over it, but permanent
     breakpoints can't be removed.  So we have to test for it here.  */
  if (breakpoint_here_p (aspace, pc) == permanent_breakpoint_here)
    {
      if (sig != GDB_SIGNAL_0)
	{
	  /* We have a signal to pass to the inferior.  The resume
	     may, or may not take us to the signal handler.  If this
	     is a step, we'll need to stop in the signal handler, if
	     there's one, (if the target supports stepping into
	     handlers), or in the next mainline instruction, if
	     there's no handler.  If this is a continue, we need to be
	     sure to run the handler with all breakpoints inserted.
	     In all cases, set a breakpoint at the current address
	     (where the handler returns to), and once that breakpoint
	     is hit, resume skipping the permanent breakpoint.  If
	     that breakpoint isn't hit, then we've stepped into the
	     signal handler (or hit some other event).  We'll delete
	     the step-resume breakpoint then.  */

	  infrun_debug_printf ("resume: skipping permanent breakpoint, "
			       "deliver signal first");

	  clear_step_over_info ();
	  tp->control.trap_expected = 0;

	  if (tp->control.step_resume_breakpoint == nullptr)
	    {
	      /* Set a "high-priority" step-resume, as we don't want
		 user breakpoints at PC to trigger (again) when this
		 hits.  */
	      insert_hp_step_resume_breakpoint_at_frame (get_current_frame ());
	      gdb_assert (tp->control.step_resume_breakpoint->first_loc ()
			  .permanent);

	      tp->step_after_step_resume_breakpoint = step;
	    }

	  insert_breakpoints ();
	}
      else
	{
	  /* There's no signal to pass, we can go ahead and skip the
	     permanent breakpoint manually.  */
	  infrun_debug_printf ("skipping permanent breakpoint");
	  gdbarch_skip_permanent_breakpoint (gdbarch, regcache);
	  /* Update pc to reflect the new address from which we will
	     execute instructions.  */
	  pc = regcache_read_pc (regcache);

	  if (step)
	    {
	      /* We've already advanced the PC, so the stepping part
		 is done.  Now we need to arrange for a trap to be
		 reported to handle_inferior_event.  Set a breakpoint
		 at the current PC, and run to it.  Don't update
		 prev_pc, because if we end in
		 switch_back_to_stepped_thread, we want the "expected
		 thread advanced also" branch to be taken.  IOW, we
		 don't want this thread to step further from PC
		 (overstep).  */
	      gdb_assert (!step_over_info_valid_p ());
	      insert_single_step_breakpoint (gdbarch, aspace, pc);
	      insert_breakpoints ();

	      resume_ptid = internal_resume_ptid (user_step);
	      do_target_resume (resume_ptid, false, GDB_SIGNAL_0);
	      tp->set_resumed (true);
	      return;
	    }
	}
    }

  /* If we have a breakpoint to step over, make sure to do a single
     step only.  Same if we have software watchpoints.  */
  if (tp->control.trap_expected || bpstat_should_step ())
    tp->control.may_range_step = 0;

  /* If displaced stepping is enabled, step over breakpoints by executing a
     copy of the instruction at a different address.

     We can't use displaced stepping when we have a signal to deliver;
     the comments for displaced_step_prepare explain why.  The
     comments in the handle_inferior event for dealing with 'random
     signals' explain what we do instead.

     We can't use displaced stepping when we are waiting for vfork_done
     event, displaced stepping breaks the vfork child similarly as single
     step software breakpoint.  */
  if (tp->control.trap_expected
      && use_displaced_stepping (tp)
      && !step_over_info_valid_p ()
      && sig == GDB_SIGNAL_0
      && current_inferior ()->thread_waiting_for_vfork_done == nullptr)
    {
      displaced_step_prepare_status prepare_status
	= displaced_step_prepare (tp);

      if (prepare_status == DISPLACED_STEP_PREPARE_STATUS_UNAVAILABLE)
	{
	  infrun_debug_printf ("Got placed in step-over queue");

	  tp->control.trap_expected = 0;
	  return;
	}
      else if (prepare_status == DISPLACED_STEP_PREPARE_STATUS_CANT)
	{
	  /* Fallback to stepping over the breakpoint in-line.  */

	  if (target_is_non_stop_p ())
	    stop_all_threads ("displaced stepping falling back on inline stepping");

	  set_step_over_info (aspace, regcache_read_pc (regcache), 0,
			      tp->global_num);

	  step = maybe_software_singlestep (gdbarch);

	  insert_breakpoints ();
	}
      else if (prepare_status == DISPLACED_STEP_PREPARE_STATUS_OK)
	{
	  /* Update pc to reflect the new address from which we will
	     execute instructions due to displaced stepping.  */
	  pc = regcache_read_pc (get_thread_regcache (tp));

	  step = gdbarch_displaced_step_hw_singlestep (gdbarch);
	}
      else
	gdb_assert_not_reached ("Invalid displaced_step_prepare_status "
				"value.");
    }

  /* Do we need to do it the hard way, w/temp breakpoints?  */
  else if (step)
    step = maybe_software_singlestep (gdbarch);

  /* Currently, our software single-step implementation leads to different
     results than hardware single-stepping in one situation: when stepping
     into delivering a signal which has an associated signal handler,
     hardware single-step will stop at the first instruction of the handler,
     while software single-step will simply skip execution of the handler.

     For now, this difference in behavior is accepted since there is no
     easy way to actually implement single-stepping into a signal handler
     without kernel support.

     However, there is one scenario where this difference leads to follow-on
     problems: if we're stepping off a breakpoint by removing all breakpoints
     and then single-stepping.  In this case, the software single-step
     behavior means that even if there is a *breakpoint* in the signal
     handler, GDB still would not stop.

     Fortunately, we can at least fix this particular issue.  We detect
     here the case where we are about to deliver a signal while software
     single-stepping with breakpoints removed.  In this situation, we
     revert the decisions to remove all breakpoints and insert single-
     step breakpoints, and instead we install a step-resume breakpoint
     at the current address, deliver the signal without stepping, and
     once we arrive back at the step-resume breakpoint, actually step
     over the breakpoint we originally wanted to step over.  */
  if (thread_has_single_step_breakpoints_set (tp)
      && sig != GDB_SIGNAL_0
      && step_over_info_valid_p ())
    {
      /* If we have nested signals or a pending signal is delivered
	 immediately after a handler returns, might already have
	 a step-resume breakpoint set on the earlier handler.  We cannot
	 set another step-resume breakpoint; just continue on until the
	 original breakpoint is hit.  */
      if (tp->control.step_resume_breakpoint == nullptr)
	{
	  insert_hp_step_resume_breakpoint_at_frame (get_current_frame ());
	  tp->step_after_step_resume_breakpoint = 1;
	}

      delete_single_step_breakpoints (tp);

      clear_step_over_info ();
      tp->control.trap_expected = 0;

      insert_breakpoints ();
    }

  /* If STEP is set, it's a request to use hardware stepping
     facilities.  But in that case, we should never
     use singlestep breakpoint.  */
  gdb_assert (!(thread_has_single_step_breakpoints_set (tp) && step));

  /* Decide the set of threads to ask the target to resume.  */
  if (tp->control.trap_expected)
    {
      /* We're allowing a thread to run past a breakpoint it has
	 hit, either by single-stepping the thread with the breakpoint
	 removed, or by displaced stepping, with the breakpoint inserted.
	 In the former case, we need to single-step only this thread,
	 and keep others stopped, as they can miss this breakpoint if
	 allowed to run.  That's not really a problem for displaced
	 stepping, but, we still keep other threads stopped, in case
	 another thread is also stopped for a breakpoint waiting for
	 its turn in the displaced stepping queue.  */
      resume_ptid = inferior_ptid;
    }
  else
    resume_ptid = internal_resume_ptid (user_step);

  if (execution_direction != EXEC_REVERSE
      && step && breakpoint_inserted_here_p (aspace, pc))
    {
      /* There are two cases where we currently need to step a
	 breakpoint instruction when we have a signal to deliver:

	 - See handle_signal_stop where we handle random signals that
	 could take out us out of the stepping range.  Normally, in
	 that case we end up continuing (instead of stepping) over the
	 signal handler with a breakpoint at PC, but there are cases
	 where we should _always_ single-step, even if we have a
	 step-resume breakpoint, like when a software watchpoint is
	 set.  Assuming single-stepping and delivering a signal at the
	 same time would takes us to the signal handler, then we could
	 have removed the breakpoint at PC to step over it.  However,
	 some hardware step targets (like e.g., Mac OS) can't step
	 into signal handlers, and for those, we need to leave the
	 breakpoint at PC inserted, as otherwise if the handler
	 recurses and executes PC again, it'll miss the breakpoint.
	 So we leave the breakpoint inserted anyway, but we need to
	 record that we tried to step a breakpoint instruction, so
	 that adjust_pc_after_break doesn't end up confused.

	 - In non-stop if we insert a breakpoint (e.g., a step-resume)
	 in one thread after another thread that was stepping had been
	 momentarily paused for a step-over.  When we re-resume the
	 stepping thread, it may be resumed from that address with a
	 breakpoint that hasn't trapped yet.  Seen with
	 gdb.threads/non-stop-fair-events.exp, on targets that don't
	 do displaced stepping.  */

      infrun_debug_printf ("resume: [%s] stepped breakpoint",
			   tp->ptid.to_string ().c_str ());

      tp->stepped_breakpoint = 1;

      /* Most targets can step a breakpoint instruction, thus
	 executing it normally.  But if this one cannot, just
	 continue and we will hit it anyway.  */
      if (gdbarch_cannot_step_breakpoint (gdbarch))
	step = false;
    }

  if (tp->control.may_range_step)
    {
      /* If we're resuming a thread with the PC out of the step
	 range, then we're doing some nested/finer run control
	 operation, like stepping the thread out of the dynamic
	 linker or the displaced stepping scratch pad.  We
	 shouldn't have allowed a range step then.  */
      gdb_assert (pc_in_thread_step_range (pc, tp));
    }

  do_target_resume (resume_ptid, step, sig);
  tp->set_resumed (true);
}

/* Resume the inferior.  SIG is the signal to give the inferior
   (GDB_SIGNAL_0 for none).  This is a wrapper around 'resume_1' that
   rolls back state on error.  */

static void
resume (gdb_signal sig)
{
  try
    {
      resume_1 (sig);
    }
  catch (const gdb_exception &ex)
    {
      /* If resuming is being aborted for any reason, delete any
	 single-step breakpoint resume_1 may have created, to avoid
	 confusing the following resumption, and to avoid leaving
	 single-step breakpoints perturbing other threads, in case
	 we're running in non-stop mode.  */
      if (inferior_ptid != null_ptid)
	delete_single_step_breakpoints (inferior_thread ());
      throw;
    }
}


/* Proceeding.  */

/* See infrun.h.  */

/* Counter that tracks number of user visible stops.  This can be used
   to tell whether a command has proceeded the inferior past the
   current location.  This allows e.g., inferior function calls in
   breakpoint commands to not interrupt the command list.  When the
   call finishes successfully, the inferior is standing at the same
   breakpoint as if nothing happened (and so we don't call
   normal_stop).  */
static ULONGEST current_stop_id;

/* See infrun.h.  */

ULONGEST
get_stop_id (void)
{
  return current_stop_id;
}

/* Called when we report a user visible stop.  */

static void
new_stop_id (void)
{
  current_stop_id++;
}

/* Clear out all variables saying what to do when inferior is continued.
   First do this, then set the ones you want, then call `proceed'.  */

static void
clear_proceed_status_thread (struct thread_info *tp)
{
  infrun_debug_printf ("%s", tp->ptid.to_string ().c_str ());

  /* If we're starting a new sequence, then the previous finished
     single-step is no longer relevant.  */
  if (tp->has_pending_waitstatus ())
    {
      if (tp->stop_reason () == TARGET_STOPPED_BY_SINGLE_STEP)
	{
	  infrun_debug_printf ("pending event of %s was a finished step. "
			       "Discarding.",
			       tp->ptid.to_string ().c_str ());

	  tp->clear_pending_waitstatus ();
	  tp->set_stop_reason (TARGET_STOPPED_BY_NO_REASON);
	}
      else
	{
	  infrun_debug_printf
	    ("thread %s has pending wait status %s (currently_stepping=%d).",
	     tp->ptid.to_string ().c_str (),
	     tp->pending_waitstatus ().to_string ().c_str (),
	     currently_stepping (tp));
	}
    }

  /* If this signal should not be seen by program, give it zero.
     Used for debugging signals.  */
  if (!signal_pass_state (tp->stop_signal ()))
    tp->set_stop_signal (GDB_SIGNAL_0);

  tp->release_thread_fsm ();

  tp->control.trap_expected = 0;
  tp->control.step_range_start = 0;
  tp->control.step_range_end = 0;
  tp->control.may_range_step = 0;
  tp->control.step_frame_id = null_frame_id;
  tp->control.step_stack_frame_id = null_frame_id;
  tp->control.step_over_calls = STEP_OVER_UNDEBUGGABLE;
  tp->control.step_start_function = nullptr;
  tp->stop_requested = 0;

  tp->control.stop_step = 0;

  tp->control.proceed_to_finish = 0;

  tp->control.stepping_command = 0;

  /* Discard any remaining commands or status from previous stop.  */
  bpstat_clear (&tp->control.stop_bpstat);
}

/* Notify the current interpreter and observers that the target is about to
   proceed.  */

static void
notify_about_to_proceed ()
{
  top_level_interpreter ()->on_about_to_proceed ();
  gdb::observers::about_to_proceed.notify ();
}

void
clear_proceed_status (int step)
{
  /* With scheduler-locking replay, stop replaying other threads if we're
     not replaying the user-visible resume ptid.

     This is a convenience feature to not require the user to explicitly
     stop replaying the other threads.  We're assuming that the user's
     intent is to resume tracing the recorded process.  */
  if (!non_stop && scheduler_mode == schedlock_replay
      && target_record_is_replaying (minus_one_ptid)
      && !target_record_will_replay (user_visible_resume_ptid (step),
				     execution_direction))
    target_record_stop_replaying ();

  if (!non_stop && inferior_ptid != null_ptid)
    {
      ptid_t resume_ptid = user_visible_resume_ptid (step);
      process_stratum_target *resume_target
	= user_visible_resume_target (resume_ptid);

      /* In all-stop mode, delete the per-thread status of all threads
	 we're about to resume, implicitly and explicitly.  */
      for (thread_info *tp : all_non_exited_threads (resume_target, resume_ptid))
	clear_proceed_status_thread (tp);
    }

  if (inferior_ptid != null_ptid)
    {
      struct inferior *inferior;

      if (non_stop)
	{
	  /* If in non-stop mode, only delete the per-thread status of
	     the current thread.  */
	  clear_proceed_status_thread (inferior_thread ());
	}

      inferior = current_inferior ();
      inferior->control.stop_soon = NO_STOP_QUIETLY;
    }

  notify_about_to_proceed ();
}

/* Returns true if TP is still stopped at a breakpoint that needs
   stepping-over in order to make progress.  If the breakpoint is gone
   meanwhile, we can skip the whole step-over dance.  */

static bool
thread_still_needs_step_over_bp (struct thread_info *tp)
{
  if (tp->stepping_over_breakpoint)
    {
      struct regcache *regcache = get_thread_regcache (tp);

      if (breakpoint_here_p (tp->inf->aspace.get (),
			     regcache_read_pc (regcache))
	  == ordinary_breakpoint_here)
	return true;

      tp->stepping_over_breakpoint = 0;
    }

  return false;
}

/* Check whether thread TP still needs to start a step-over in order
   to make progress when resumed.  Returns an bitwise or of enum
   step_over_what bits, indicating what needs to be stepped over.  */

static step_over_what
thread_still_needs_step_over (struct thread_info *tp)
{
  step_over_what what = 0;

  if (thread_still_needs_step_over_bp (tp))
    what |= STEP_OVER_BREAKPOINT;

  if (tp->stepping_over_watchpoint
      && !target_have_steppable_watchpoint ())
    what |= STEP_OVER_WATCHPOINT;

  return what;
}

/* Returns true if scheduler locking applies.  STEP indicates whether
   we're about to do a step/next-like command to a thread.  */

static bool
schedlock_applies (struct thread_info *tp)
{
  return (scheduler_mode == schedlock_on
	  || (scheduler_mode == schedlock_step
	      && tp->control.stepping_command)
	  || (scheduler_mode == schedlock_replay
	      && target_record_will_replay (minus_one_ptid,
					    execution_direction)));
}

/* Set process_stratum_target::COMMIT_RESUMED_STATE in all target
   stacks that have threads executing and don't have threads with
   pending events.  */

static void
maybe_set_commit_resumed_all_targets ()
{
  scoped_restore_current_thread restore_thread;

  for (inferior *inf : all_non_exited_inferiors ())
    {
      process_stratum_target *proc_target = inf->process_target ();

      if (proc_target->commit_resumed_state)
	{
	  /* We already set this in a previous iteration, via another
	     inferior sharing the process_stratum target.  */
	  continue;
	}

      /* If the target has no resumed threads, it would be useless to
	 ask it to commit the resumed threads.  */
      if (!proc_target->threads_executing)
	{
	  infrun_debug_printf ("not requesting commit-resumed for target "
			       "%s, no resumed threads",
			       proc_target->shortname ());
	  continue;
	}

      /* As an optimization, if a thread from this target has some
	 status to report, handle it before requiring the target to
	 commit its resumed threads: handling the status might lead to
	 resuming more threads.  */
      if (proc_target->has_resumed_with_pending_wait_status ())
	{
	  infrun_debug_printf ("not requesting commit-resumed for target %s, a"
			       " thread has a pending waitstatus",
			       proc_target->shortname ());
	  continue;
	}

      switch_to_inferior_no_thread (inf);

      if (target_has_pending_events ())
	{
	  infrun_debug_printf ("not requesting commit-resumed for target %s, "
			       "target has pending events",
			       proc_target->shortname ());
	  continue;
	}

      infrun_debug_printf ("enabling commit-resumed for target %s",
			   proc_target->shortname ());

      proc_target->commit_resumed_state = true;
    }
}

/* See infrun.h.  */

void
maybe_call_commit_resumed_all_targets ()
{
  scoped_restore_current_thread restore_thread;

  for (inferior *inf : all_non_exited_inferiors ())
    {
      process_stratum_target *proc_target = inf->process_target ();

      if (!proc_target->commit_resumed_state)
	continue;

      switch_to_inferior_no_thread (inf);

      infrun_debug_printf ("calling commit_resumed for target %s",
			   proc_target->shortname());

      target_commit_resumed ();
    }
}

/* To track nesting of scoped_disable_commit_resumed objects, ensuring
   that only the outermost one attempts to re-enable
   commit-resumed.  */
static bool enable_commit_resumed = true;

/* See infrun.h.  */

scoped_disable_commit_resumed::scoped_disable_commit_resumed
  (const char *reason)
  : m_reason (reason),
    m_prev_enable_commit_resumed (enable_commit_resumed)
{
  infrun_debug_printf ("reason=%s", m_reason);

  enable_commit_resumed = false;

  for (inferior *inf : all_non_exited_inferiors ())
    {
      process_stratum_target *proc_target = inf->process_target ();

      if (m_prev_enable_commit_resumed)
	{
	  /* This is the outermost instance: force all
	     COMMIT_RESUMED_STATE to false.  */
	  proc_target->commit_resumed_state = false;
	}
      else
	{
	  /* This is not the outermost instance, we expect
	     COMMIT_RESUMED_STATE to have been cleared by the
	     outermost instance.  */
	  gdb_assert (!proc_target->commit_resumed_state);
	}
    }
}

/* See infrun.h.  */

void
scoped_disable_commit_resumed::reset ()
{
  if (m_reset)
    return;
  m_reset = true;

  infrun_debug_printf ("reason=%s", m_reason);

  gdb_assert (!enable_commit_resumed);

  enable_commit_resumed = m_prev_enable_commit_resumed;

  if (m_prev_enable_commit_resumed)
    {
      /* This is the outermost instance, re-enable
	 COMMIT_RESUMED_STATE on the targets where it's possible.  */
      maybe_set_commit_resumed_all_targets ();
    }
  else
    {
      /* This is not the outermost instance, we expect
	 COMMIT_RESUMED_STATE to still be false.  */
      for (inferior *inf : all_non_exited_inferiors ())
	{
	  process_stratum_target *proc_target = inf->process_target ();
	  gdb_assert (!proc_target->commit_resumed_state);
	}
    }
}

/* See infrun.h.  */

scoped_disable_commit_resumed::~scoped_disable_commit_resumed ()
{
  reset ();
}

/* See infrun.h.  */

void
scoped_disable_commit_resumed::reset_and_commit ()
{
  reset ();
  maybe_call_commit_resumed_all_targets ();
}

/* See infrun.h.  */

scoped_enable_commit_resumed::scoped_enable_commit_resumed
  (const char *reason)
  : m_reason (reason),
    m_prev_enable_commit_resumed (enable_commit_resumed)
{
  infrun_debug_printf ("reason=%s", m_reason);

  if (!enable_commit_resumed)
    {
      enable_commit_resumed = true;

      /* Re-enable COMMIT_RESUMED_STATE on the targets where it's
	 possible.  */
      maybe_set_commit_resumed_all_targets ();

      maybe_call_commit_resumed_all_targets ();
    }
}

/* See infrun.h.  */

scoped_enable_commit_resumed::~scoped_enable_commit_resumed ()
{
  infrun_debug_printf ("reason=%s", m_reason);

  gdb_assert (enable_commit_resumed);

  enable_commit_resumed = m_prev_enable_commit_resumed;

  if (!enable_commit_resumed)
    {
      /* Force all COMMIT_RESUMED_STATE back to false.  */
      for (inferior *inf : all_non_exited_inferiors ())
	{
	  process_stratum_target *proc_target = inf->process_target ();
	  proc_target->commit_resumed_state = false;
	}
    }
}

/* Check that all the targets we're about to resume are in non-stop
   mode.  Ideally, we'd only care whether all targets support
   target-async, but we're not there yet.  E.g., stop_all_threads
   doesn't know how to handle all-stop targets.  Also, the remote
   protocol in all-stop mode is synchronous, irrespective of
   target-async, which means that things like a breakpoint re-set
   triggered by one target would try to read memory from all targets
   and fail.  */

static void
check_multi_target_resumption (process_stratum_target *resume_target)
{
  if (!non_stop && resume_target == nullptr)
    {
      scoped_restore_current_thread restore_thread;

      /* This is used to track whether we're resuming more than one
	 target.  */
      process_stratum_target *first_connection = nullptr;

      /* The first inferior we see with a target that does not work in
	 always-non-stop mode.  */
      inferior *first_not_non_stop = nullptr;

      for (inferior *inf : all_non_exited_inferiors ())
	{
	  switch_to_inferior_no_thread (inf);

	  if (!target_has_execution ())
	    continue;

	  process_stratum_target *proc_target
	    = current_inferior ()->process_target();

	  if (!target_is_non_stop_p ())
	    first_not_non_stop = inf;

	  if (first_connection == nullptr)
	    first_connection = proc_target;
	  else if (first_connection != proc_target
		   && first_not_non_stop != nullptr)
	    {
	      switch_to_inferior_no_thread (first_not_non_stop);

	      proc_target = current_inferior ()->process_target();

	      error (_("Connection %d (%s) does not support "
		       "multi-target resumption."),
		     proc_target->connection_number,
		     make_target_connection_string (proc_target).c_str ());
	    }
	}
    }
}

/* Helper function for `proceed`.  Check if thread TP is suitable for
   resuming, and, if it is, switch to the thread and call
   `keep_going_pass_signal`.  If TP is not suitable for resuming then this
   function will just return without switching threads.  */

static void
proceed_resume_thread_checked (thread_info *tp)
{
  if (!tp->inf->has_execution ())
    {
      infrun_debug_printf ("[%s] target has no execution",
			   tp->ptid.to_string ().c_str ());
      return;
    }

  if (tp->resumed ())
    {
      infrun_debug_printf ("[%s] resumed",
			   tp->ptid.to_string ().c_str ());
      gdb_assert (tp->executing () || tp->has_pending_waitstatus ());
      return;
    }

  if (thread_is_in_step_over_chain (tp))
    {
      infrun_debug_printf ("[%s] needs step-over",
			   tp->ptid.to_string ().c_str ());
      return;
    }

  /* When handling a vfork GDB removes all breakpoints from the program
     space in which the vfork is being handled.  If we are following the
     parent then GDB will set the thread_waiting_for_vfork_done member of
     the parent inferior.  In this case we should take care to only resume
     the vfork parent thread, the kernel will hold this thread suspended
     until the vfork child has exited or execd, at which point the parent
     will be resumed and a VFORK_DONE event sent to GDB.  */
  if (tp->inf->thread_waiting_for_vfork_done != nullptr)
    {
      if (target_is_non_stop_p ())
	{
	  /* For non-stop targets, regardless of whether GDB is using
	     all-stop or non-stop mode, threads are controlled
	     individually.

	     When a thread is handling a vfork, breakpoints are removed
	     from the inferior (well, program space in fact), so it is
	     critical that we don't try to resume any thread other than the
	     vfork parent.  */
	  if (tp != tp->inf->thread_waiting_for_vfork_done)
	    {
	      infrun_debug_printf ("[%s] thread %s of this inferior is "
				   "waiting for vfork-done",
				   tp->ptid.to_string ().c_str (),
				   tp->inf->thread_waiting_for_vfork_done
				     ->ptid.to_string ().c_str ());
	      return;
	    }
	}
      else
	{
	  /* For all-stop targets, when we attempt to resume the inferior,
	     we will only resume the vfork parent thread, this is handled
	     in internal_resume_ptid.

	     Additionally, we will always be called with the vfork parent
	     thread as the current thread (TP) thanks to follow_fork, as
	     such the following assertion should hold.

	     Beyond this there is nothing more that needs to be done
	     here.  */
	  gdb_assert (tp == tp->inf->thread_waiting_for_vfork_done);
	}
    }

  /* When handling a vfork GDB removes all breakpoints from the program
     space in which the vfork is being handled.  If we are following the
     child then GDB will set vfork_child member of the vfork parent
     inferior.  Once the child has either exited or execd then GDB will
     detach from the parent process.  Until that point GDB should not
     resume any thread in the parent process.  */
  if (tp->inf->vfork_child != nullptr)
    {
      infrun_debug_printf ("[%s] thread is part of a vfork parent, child is %d",
			   tp->ptid.to_string ().c_str (),
			   tp->inf->vfork_child->pid);
      return;
    }

  infrun_debug_printf ("resuming %s",
		       tp->ptid.to_string ().c_str ());

  execution_control_state ecs (tp);
  switch_to_thread (tp);
  keep_going_pass_signal (&ecs);
  if (!ecs.wait_some_more)
    error (_("Command aborted."));
}

/* Basic routine for continuing the program in various fashions.

   ADDR is the address to resume at, or -1 for resume where stopped.
   SIGGNAL is the signal to give it, or GDB_SIGNAL_0 for none,
   or GDB_SIGNAL_DEFAULT for act according to how it stopped.

   You should call clear_proceed_status before calling proceed.  */

void
proceed (CORE_ADDR addr, enum gdb_signal siggnal)
{
  INFRUN_SCOPED_DEBUG_ENTER_EXIT;

  struct gdbarch *gdbarch;
  CORE_ADDR pc;

  /* If we're stopped at a fork/vfork, switch to either the parent or child
     thread as defined by the "set follow-fork-mode" command, or, if both
     the parent and child are controlled by GDB, and schedule-multiple is
     on, follow the child.  If none of the above apply then we just proceed
     resuming the current thread.  */
  if (!follow_fork ())
    {
      /* The target for some reason decided not to resume.  */
      normal_stop ();
      if (target_can_async_p ())
	inferior_event_handler (INF_EXEC_COMPLETE);
      return;
    }

  /* We'll update this if & when we switch to a new thread.  */
  update_previous_thread ();

  thread_info *cur_thr = inferior_thread ();
  infrun_debug_printf ("cur_thr = %s", cur_thr->ptid.to_string ().c_str ());

  regcache *regcache = get_thread_regcache (cur_thr);
  gdbarch = regcache->arch ();
  pc = regcache_read_pc_protected (regcache);

  /* Fill in with reasonable starting values.  */
  init_thread_stepping_state (cur_thr);

  gdb_assert (!thread_is_in_step_over_chain (cur_thr));

  ptid_t resume_ptid
    = user_visible_resume_ptid (cur_thr->control.stepping_command);
  process_stratum_target *resume_target
    = user_visible_resume_target (resume_ptid);

  check_multi_target_resumption (resume_target);

  if (addr == (CORE_ADDR) -1)
    {
      const address_space *aspace = cur_thr->inf->aspace.get ();

      if (cur_thr->stop_pc_p ()
	  && pc == cur_thr->stop_pc ()
	  && breakpoint_here_p (aspace, pc) == ordinary_breakpoint_here
	  && execution_direction != EXEC_REVERSE)
	/* There is a breakpoint at the address we will resume at,
	   step one instruction before inserting breakpoints so that
	   we do not stop right away (and report a second hit at this
	   breakpoint).

	   Note, we don't do this in reverse, because we won't
	   actually be executing the breakpoint insn anyway.
	   We'll be (un-)executing the previous instruction.  */
	cur_thr->stepping_over_breakpoint = 1;
      else if (gdbarch_single_step_through_delay_p (gdbarch)
	       && gdbarch_single_step_through_delay (gdbarch,
						     get_current_frame ()))
	/* We stepped onto an instruction that needs to be stepped
	   again before re-inserting the breakpoint, do so.  */
	cur_thr->stepping_over_breakpoint = 1;
    }
  else
    {
      regcache_write_pc (regcache, addr);
    }

  if (siggnal != GDB_SIGNAL_DEFAULT)
    cur_thr->set_stop_signal (siggnal);

  /* If an exception is thrown from this point on, make sure to
     propagate GDB's knowledge of the executing state to the
     frontend/user running state.  */
  scoped_finish_thread_state finish_state (resume_target, resume_ptid);

  /* Even if RESUME_PTID is a wildcard, and we end up resuming fewer
     threads (e.g., we might need to set threads stepping over
     breakpoints first), from the user/frontend's point of view, all
     threads in RESUME_PTID are now running.  Unless we're calling an
     inferior function, as in that case we pretend the inferior
     doesn't run at all.  */
  if (!cur_thr->control.in_infcall)
    set_running (resume_target, resume_ptid, true);

  infrun_debug_printf ("addr=%s, signal=%s, resume_ptid=%s",
		       paddress (gdbarch, addr),
		       gdb_signal_to_symbol_string (siggnal),
		       resume_ptid.to_string ().c_str ());

  annotate_starting ();

  /* Make sure that output from GDB appears before output from the
     inferior.  */
  gdb_flush (gdb_stdout);

  /* Since we've marked the inferior running, give it the terminal.  A
     QUIT/Ctrl-C from here on is forwarded to the target (which can
     still detect attempts to unblock a stuck connection with repeated
     Ctrl-C from within target_pass_ctrlc).  */
  target_terminal::inferior ();

  /* In a multi-threaded task we may select another thread and
     then continue or step.

     But if a thread that we're resuming had stopped at a breakpoint,
     it will immediately cause another breakpoint stop without any
     execution (i.e. it will report a breakpoint hit incorrectly).  So
     we must step over it first.

     Look for threads other than the current (TP) that reported a
     breakpoint hit and haven't been resumed yet since.  */

  /* If scheduler locking applies, we can avoid iterating over all
     threads.  */
  if (!non_stop && !schedlock_applies (cur_thr))
    {
      for (thread_info *tp : all_non_exited_threads (resume_target,
						     resume_ptid))
	{
	  switch_to_thread_no_regs (tp);

	  /* Ignore the current thread here.  It's handled
	     afterwards.  */
	  if (tp == cur_thr)
	    continue;

	  if (!thread_still_needs_step_over (tp))
	    continue;

	  gdb_assert (!thread_is_in_step_over_chain (tp));

	  infrun_debug_printf ("need to step-over [%s] first",
			       tp->ptid.to_string ().c_str ());

	  global_thread_step_over_chain_enqueue (tp);
	}

      switch_to_thread (cur_thr);
    }

  /* Enqueue the current thread last, so that we move all other
     threads over their breakpoints first.  */
  if (cur_thr->stepping_over_breakpoint)
    global_thread_step_over_chain_enqueue (cur_thr);

  /* If the thread isn't started, we'll still need to set its prev_pc,
     so that switch_back_to_stepped_thread knows the thread hasn't
     advanced.  Must do this before resuming any thread, as in
     all-stop/remote, once we resume we can't send any other packet
     until the target stops again.  */
  cur_thr->prev_pc = regcache_read_pc_protected (regcache);

  {
    scoped_disable_commit_resumed disable_commit_resumed ("proceeding");
    bool step_over_started = start_step_over ();

    if (step_over_info_valid_p ())
      {
	/* Either this thread started a new in-line step over, or some
	   other thread was already doing one.  In either case, don't
	   resume anything else until the step-over is finished.  */
      }
    else if (step_over_started && !target_is_non_stop_p ())
      {
	/* A new displaced stepping sequence was started.  In all-stop,
	   we can't talk to the target anymore until it next stops.  */
      }
    else if (!non_stop && target_is_non_stop_p ())
      {
	INFRUN_SCOPED_DEBUG_START_END
	  ("resuming threads, all-stop-on-top-of-non-stop");

	/* In all-stop, but the target is always in non-stop mode.
	   Start all other threads that are implicitly resumed too.  */
	for (thread_info *tp : all_non_exited_threads (resume_target,
						       resume_ptid))
	  {
	    switch_to_thread_no_regs (tp);
	    proceed_resume_thread_checked (tp);
	  }
      }
    else
      proceed_resume_thread_checked (cur_thr);

    disable_commit_resumed.reset_and_commit ();
  }

  finish_state.release ();

  /* If we've switched threads above, switch back to the previously
     current thread.  We don't want the user to see a different
     selected thread.  */
  switch_to_thread (cur_thr);

  /* Tell the event loop to wait for it to stop.  If the target
     supports asynchronous execution, it'll do this from within
     target_resume.  */
  if (!target_can_async_p ())
    mark_async_event_handler (infrun_async_inferior_event_token);
}


/* Start remote-debugging of a machine over a serial link.  */

void
start_remote (int from_tty)
{
  inferior *inf = current_inferior ();
  inf->control.stop_soon = STOP_QUIETLY_REMOTE;

  /* Always go on waiting for the target, regardless of the mode.  */
  /* FIXME: cagney/1999-09-23: At present it isn't possible to
     indicate to wait_for_inferior that a target should timeout if
     nothing is returned (instead of just blocking).  Because of this,
     targets expecting an immediate response need to, internally, set
     things up so that the target_wait() is forced to eventually
     timeout.  */
  /* FIXME: cagney/1999-09-24: It isn't possible for target_open() to
     differentiate to its caller what the state of the target is after
     the initial open has been performed.  Here we're assuming that
     the target has stopped.  It should be possible to eventually have
     target_open() return to the caller an indication that the target
     is currently running and GDB state should be set to the same as
     for an async run.  */
  wait_for_inferior (inf);

  /* Now that the inferior has stopped, do any bookkeeping like
     loading shared libraries.  We want to do this before normal_stop,
     so that the displayed frame is up to date.  */
  post_create_inferior (from_tty);

  normal_stop ();
}

/* Initialize static vars when a new inferior begins.  */

void
init_wait_for_inferior (void)
{
  /* These are meaningless until the first time through wait_for_inferior.  */

  breakpoint_init_inferior (inf_starting);

  clear_proceed_status (0);

  nullify_last_target_wait_ptid ();

  update_previous_thread ();
}



static void handle_inferior_event (struct execution_control_state *ecs);

static void handle_step_into_function (struct gdbarch *gdbarch,
				       struct execution_control_state *ecs);
static void handle_step_into_function_backward (struct gdbarch *gdbarch,
						struct execution_control_state *ecs);
static void handle_signal_stop (struct execution_control_state *ecs);
static void check_exception_resume (struct execution_control_state *,
				    frame_info_ptr);

static void end_stepping_range (struct execution_control_state *ecs);
static void stop_waiting (struct execution_control_state *ecs);
static void keep_going (struct execution_control_state *ecs);
static void process_event_stop_test (struct execution_control_state *ecs);
static bool switch_back_to_stepped_thread (struct execution_control_state *ecs);

/* This function is attached as a "thread_stop_requested" observer.
   Cleanup local state that assumed the PTID was to be resumed, and
   report the stop to the frontend.  */

static void
infrun_thread_stop_requested (ptid_t ptid)
{
  process_stratum_target *curr_target = current_inferior ()->process_target ();

  /* PTID was requested to stop.  If the thread was already stopped,
     but the user/frontend doesn't know about that yet (e.g., the
     thread had been temporarily paused for some step-over), set up
     for reporting the stop now.  */
  for (thread_info *tp : all_threads (curr_target, ptid))
    {
      if (tp->state != THREAD_RUNNING)
	continue;
      if (tp->executing ())
	continue;

      /* Remove matching threads from the step-over queue, so
	 start_step_over doesn't try to resume them
	 automatically.  */
      if (thread_is_in_step_over_chain (tp))
	global_thread_step_over_chain_remove (tp);

      /* If the thread is stopped, but the user/frontend doesn't
	 know about that yet, queue a pending event, as if the
	 thread had just stopped now.  Unless the thread already had
	 a pending event.  */
      if (!tp->has_pending_waitstatus ())
	{
	  target_waitstatus ws;
	  ws.set_stopped (GDB_SIGNAL_0);
	  tp->set_pending_waitstatus (ws);
	}

      /* Clear the inline-frame state, since we're re-processing the
	 stop.  */
      clear_inline_frame_state (tp);

      /* If this thread was paused because some other thread was
	 doing an inline-step over, let that finish first.  Once
	 that happens, we'll restart all threads and consume pending
	 stop events then.  */
      if (step_over_info_valid_p ())
	continue;

      /* Otherwise we can process the (new) pending event now.  Set
	 it so this pending event is considered by
	 do_target_wait.  */
      tp->set_resumed (true);
    }
}

/* Delete the step resume, single-step and longjmp/exception resume
   breakpoints of TP.  */

static void
delete_thread_infrun_breakpoints (struct thread_info *tp)
{
  delete_step_resume_breakpoint (tp);
  delete_exception_resume_breakpoint (tp);
  delete_single_step_breakpoints (tp);
}

/* If the target still has execution, call FUNC for each thread that
   just stopped.  In all-stop, that's all the non-exited threads; in
   non-stop, that's the current thread, only.  */

typedef void (*for_each_just_stopped_thread_callback_func)
  (struct thread_info *tp);

static void
for_each_just_stopped_thread (for_each_just_stopped_thread_callback_func func)
{
  if (!target_has_execution () || inferior_ptid == null_ptid)
    return;

  if (target_is_non_stop_p ())
    {
      /* If in non-stop mode, only the current thread stopped.  */
      func (inferior_thread ());
    }
  else
    {
      /* In all-stop mode, all threads have stopped.  */
      for (thread_info *tp : all_non_exited_threads ())
	func (tp);
    }
}

/* Delete the step resume and longjmp/exception resume breakpoints of
   the threads that just stopped.  */

static void
delete_just_stopped_threads_infrun_breakpoints (void)
{
  for_each_just_stopped_thread (delete_thread_infrun_breakpoints);
}

/* Delete the single-step breakpoints of the threads that just
   stopped.  */

static void
delete_just_stopped_threads_single_step_breakpoints (void)
{
  for_each_just_stopped_thread (delete_single_step_breakpoints);
}

/* See infrun.h.  */

void
print_target_wait_results (ptid_t waiton_ptid, ptid_t result_ptid,
			   const struct target_waitstatus &ws)
{
  infrun_debug_printf ("target_wait (%s [%s], status) =",
		       waiton_ptid.to_string ().c_str (),
		       target_pid_to_str (waiton_ptid).c_str ());
  infrun_debug_printf ("  %s [%s],",
		       result_ptid.to_string ().c_str (),
		       target_pid_to_str (result_ptid).c_str ());
  infrun_debug_printf ("  %s", ws.to_string ().c_str ());
}

/* Select a thread at random, out of those which are resumed and have
   had events.  */

static struct thread_info *
random_pending_event_thread (inferior *inf, ptid_t waiton_ptid)
{
  process_stratum_target *proc_target = inf->process_target ();
  thread_info *thread
    = proc_target->random_resumed_with_pending_wait_status (inf, waiton_ptid);

  if (thread == nullptr)
    {
      infrun_debug_printf ("None found.");
      return nullptr;
    }

  infrun_debug_printf ("Found %s.", thread->ptid.to_string ().c_str ());
  gdb_assert (thread->resumed ());
  gdb_assert (thread->has_pending_waitstatus ());

  return thread;
}

/* Wrapper for target_wait that first checks whether threads have
   pending statuses to report before actually asking the target for
   more events.  INF is the inferior we're using to call target_wait
   on.  */

static ptid_t
do_target_wait_1 (inferior *inf, ptid_t ptid,
		  target_waitstatus *status, target_wait_flags options)
{
  struct thread_info *tp;

  /* We know that we are looking for an event in the target of inferior
     INF, but we don't know which thread the event might come from.  As
     such we want to make sure that INFERIOR_PTID is reset so that none of
     the wait code relies on it - doing so is always a mistake.  */
  switch_to_inferior_no_thread (inf);

  /* First check if there is a resumed thread with a wait status
     pending.  */
  if (ptid == minus_one_ptid || ptid.is_pid ())
    {
      tp = random_pending_event_thread (inf, ptid);
    }
  else
    {
      infrun_debug_printf ("Waiting for specific thread %s.",
			   ptid.to_string ().c_str ());

      /* We have a specific thread to check.  */
      tp = inf->find_thread (ptid);
      gdb_assert (tp != nullptr);
      if (!tp->has_pending_waitstatus ())
	tp = nullptr;
    }

  if (tp != nullptr
      && (tp->stop_reason () == TARGET_STOPPED_BY_SW_BREAKPOINT
	  || tp->stop_reason () == TARGET_STOPPED_BY_HW_BREAKPOINT))
    {
      struct regcache *regcache = get_thread_regcache (tp);
      struct gdbarch *gdbarch = regcache->arch ();
      CORE_ADDR pc;
      int discard = 0;

      pc = regcache_read_pc (regcache);

      if (pc != tp->stop_pc ())
	{
	  infrun_debug_printf ("PC of %s changed.  was=%s, now=%s",
			       tp->ptid.to_string ().c_str (),
			       paddress (gdbarch, tp->stop_pc ()),
			       paddress (gdbarch, pc));
	  discard = 1;
	}
      else if (!breakpoint_inserted_here_p (tp->inf->aspace.get (), pc))
	{
	  infrun_debug_printf ("previous breakpoint of %s, at %s gone",
			       tp->ptid.to_string ().c_str (),
			       paddress (gdbarch, pc));

	  discard = 1;
	}

      if (discard)
	{
	  infrun_debug_printf ("pending event of %s cancelled.",
			       tp->ptid.to_string ().c_str ());

	  tp->clear_pending_waitstatus ();
	  target_waitstatus ws;
	  ws.set_spurious ();
	  tp->set_pending_waitstatus (ws);
	  tp->set_stop_reason (TARGET_STOPPED_BY_NO_REASON);
	}
    }

  if (tp != nullptr)
    {
      infrun_debug_printf ("Using pending wait status %s for %s.",
			   tp->pending_waitstatus ().to_string ().c_str (),
			   tp->ptid.to_string ().c_str ());

      /* Now that we've selected our final event LWP, un-adjust its PC
	 if it was a software breakpoint (and the target doesn't
	 always adjust the PC itself).  */
      if (tp->stop_reason () == TARGET_STOPPED_BY_SW_BREAKPOINT
	  && !target_supports_stopped_by_sw_breakpoint ())
	{
	  struct regcache *regcache;
	  struct gdbarch *gdbarch;
	  int decr_pc;

	  regcache = get_thread_regcache (tp);
	  gdbarch = regcache->arch ();

	  decr_pc = gdbarch_decr_pc_after_break (gdbarch);
	  if (decr_pc != 0)
	    {
	      CORE_ADDR pc;

	      pc = regcache_read_pc (regcache);
	      regcache_write_pc (regcache, pc + decr_pc);
	    }
	}

      tp->set_stop_reason (TARGET_STOPPED_BY_NO_REASON);
      *status = tp->pending_waitstatus ();
      tp->clear_pending_waitstatus ();

      /* Wake up the event loop again, until all pending events are
	 processed.  */
      if (target_is_async_p ())
	mark_async_event_handler (infrun_async_inferior_event_token);
      return tp->ptid;
    }

  /* But if we don't find one, we'll have to wait.  */

  /* We can't ask a non-async target to do a non-blocking wait, so this will be
     a blocking wait.  */
  if (!target_can_async_p ())
    options &= ~TARGET_WNOHANG;

  return target_wait (ptid, status, options);
}

/* Wrapper for target_wait that first checks whether threads have
   pending statuses to report before actually asking the target for
   more events.  Polls for events from all inferiors/targets.  */

static bool
do_target_wait (execution_control_state *ecs, target_wait_flags options)
{
  int num_inferiors = 0;
  int random_selector;

  /* For fairness, we pick the first inferior/target to poll at random
     out of all inferiors that may report events, and then continue
     polling the rest of the inferior list starting from that one in a
     circular fashion until the whole list is polled once.  */

  auto inferior_matches = [] (inferior *inf)
    {
      return inf->process_target () != nullptr;
    };

  /* First see how many matching inferiors we have.  */
  for (inferior *inf : all_inferiors ())
    if (inferior_matches (inf))
      num_inferiors++;

  if (num_inferiors == 0)
    {
      ecs->ws.set_ignore ();
      return false;
    }

  /* Now randomly pick an inferior out of those that matched.  */
  random_selector = (int)
    ((num_inferiors * (double) rand ()) / (RAND_MAX + 1.0));

  if (num_inferiors > 1)
    infrun_debug_printf ("Found %d inferiors, starting at #%d",
			 num_inferiors, random_selector);

  /* Select the Nth inferior that matched.  */

  inferior *selected = nullptr;

  for (inferior *inf : all_inferiors ())
    if (inferior_matches (inf))
      if (random_selector-- == 0)
	{
	  selected = inf;
	  break;
	}

  /* Now poll for events out of each of the matching inferior's
     targets, starting from the selected one.  */

  auto do_wait = [&] (inferior *inf)
  {
    ecs->ptid = do_target_wait_1 (inf, minus_one_ptid, &ecs->ws, options);
    ecs->target = inf->process_target ();
    return (ecs->ws.kind () != TARGET_WAITKIND_IGNORE);
  };

  /* Needed in 'all-stop + target-non-stop' mode, because we end up
     here spuriously after the target is all stopped and we've already
     reported the stop to the user, polling for events.  */
  scoped_restore_current_thread restore_thread;

  intrusive_list_iterator<inferior> start
    = inferior_list.iterator_to (*selected);

  for (intrusive_list_iterator<inferior> it = start;
       it != inferior_list.end ();
       ++it)
    {
      inferior *inf = &*it;

      if (inferior_matches (inf) && do_wait (inf))
	return true;
    }

  for (intrusive_list_iterator<inferior> it = inferior_list.begin ();
       it != start;
       ++it)
    {
      inferior *inf = &*it;

      if (inferior_matches (inf) && do_wait (inf))
	return true;
    }

  ecs->ws.set_ignore ();
  return false;
}

/* An event reported by wait_one.  */

struct wait_one_event
{
  /* The target the event came out of.  */
  process_stratum_target *target;

  /* The PTID the event was for.  */
  ptid_t ptid;

  /* The waitstatus.  */
  target_waitstatus ws;
};

static bool handle_one (const wait_one_event &event);
static int finish_step_over (struct execution_control_state *ecs);

/* Prepare and stabilize the inferior for detaching it.  E.g.,
   detaching while a thread is displaced stepping is a recipe for
   crashing it, as nothing would readjust the PC out of the scratch
   pad.  */

void
prepare_for_detach (void)
{
  struct inferior *inf = current_inferior ();
  ptid_t pid_ptid = ptid_t (inf->pid);
  scoped_restore_current_thread restore_thread;

  scoped_restore restore_detaching = make_scoped_restore (&inf->detaching, true);

  /* Remove all threads of INF from the global step-over chain.  We
     want to stop any ongoing step-over, not start any new one.  */
  thread_step_over_list_safe_range range
    = make_thread_step_over_list_safe_range (global_thread_step_over_list);

  for (thread_info *tp : range)
    if (tp->inf == inf)
      {
	infrun_debug_printf ("removing thread %s from global step over chain",
			     tp->ptid.to_string ().c_str ());
	global_thread_step_over_chain_remove (tp);
      }

  /* If we were already in the middle of an inline step-over, and the
     thread stepping belongs to the inferior we're detaching, we need
     to restart the threads of other inferiors.  */
  if (step_over_info.thread != -1)
    {
      infrun_debug_printf ("inline step-over in-process while detaching");

      thread_info *thr = find_thread_global_id (step_over_info.thread);
      if (thr->inf == inf)
	{
	  /* Since we removed threads of INF from the step-over chain,
	     we know this won't start a step-over for INF.  */
	  clear_step_over_info ();

	  if (target_is_non_stop_p ())
	    {
	      /* Start a new step-over in another thread if there's
		 one that needs it.  */
	      start_step_over ();

	      /* Restart all other threads (except the
		 previously-stepping thread, since that one is still
		 running).  */
	      if (!step_over_info_valid_p ())
		restart_threads (thr);
	    }
	}
    }

  if (displaced_step_in_progress (inf))
    {
      infrun_debug_printf ("displaced-stepping in-process while detaching");

      /* Stop threads currently displaced stepping, aborting it.  */

      for (thread_info *thr : inf->non_exited_threads ())
	{
	  if (thr->displaced_step_state.in_progress ())
	    {
	      if (thr->executing ())
		{
		  if (!thr->stop_requested)
		    {
		      target_stop (thr->ptid);
		      thr->stop_requested = true;
		    }
		}
	      else
		thr->set_resumed (false);
	    }
	}

      while (displaced_step_in_progress (inf))
	{
	  wait_one_event event;

	  event.target = inf->process_target ();
	  event.ptid = do_target_wait_1 (inf, pid_ptid, &event.ws, 0);

	  if (debug_infrun)
	    print_target_wait_results (pid_ptid, event.ptid, event.ws);

	  handle_one (event);
	}

      /* It's OK to leave some of the threads of INF stopped, since
	 they'll be detached shortly.  */
    }
}

/* If all-stop, but there exists a non-stop target, stop all threads
   now that we're presenting the stop to the user.  */

static void
stop_all_threads_if_all_stop_mode ()
{
  if (!non_stop && exists_non_stop_target ())
    stop_all_threads ("presenting stop to user in all-stop");
}

/* Wait for control to return from inferior to debugger.

   If inferior gets a signal, we may decide to start it up again
   instead of returning.  That is why there is a loop in this function.
   When this function actually returns it means the inferior
   should be left stopped and GDB should read more commands.  */

static void
wait_for_inferior (inferior *inf)
{
  infrun_debug_printf ("wait_for_inferior ()");

  SCOPE_EXIT { delete_just_stopped_threads_infrun_breakpoints (); };

  /* If an error happens while handling the event, propagate GDB's
     knowledge of the executing state to the frontend/user running
     state.  */
  scoped_finish_thread_state finish_state
    (inf->process_target (), minus_one_ptid);

  while (1)
    {
      execution_control_state ecs;

      overlay_cache_invalid = 1;

      /* Flush target cache before starting to handle each event.
	 Target was running and cache could be stale.  This is just a
	 heuristic.  Running threads may modify target memory, but we
	 don't get any event.  */
      target_dcache_invalidate (current_program_space->aspace);

      ecs.ptid = do_target_wait_1 (inf, minus_one_ptid, &ecs.ws, 0);
      ecs.target = inf->process_target ();

      if (debug_infrun)
	print_target_wait_results (minus_one_ptid, ecs.ptid, ecs.ws);

      /* Now figure out what to do with the result of the result.  */
      handle_inferior_event (&ecs);

      if (!ecs.wait_some_more)
	break;
    }

  stop_all_threads_if_all_stop_mode ();

  /* No error, don't finish the state yet.  */
  finish_state.release ();
}

/* Cleanup that reinstalls the readline callback handler, if the
   target is running in the background.  If while handling the target
   event something triggered a secondary prompt, like e.g., a
   pagination prompt, we'll have removed the callback handler (see
   gdb_readline_wrapper_line).  Need to do this as we go back to the
   event loop, ready to process further input.  Note this has no
   effect if the handler hasn't actually been removed, because calling
   rl_callback_handler_install resets the line buffer, thus losing
   input.  */

static void
reinstall_readline_callback_handler_cleanup ()
{
  struct ui *ui = current_ui;

  if (!ui->async)
    {
      /* We're not going back to the top level event loop yet.  Don't
	 install the readline callback, as it'd prep the terminal,
	 readline-style (raw, noecho) (e.g., --batch).  We'll install
	 it the next time the prompt is displayed, when we're ready
	 for input.  */
      return;
    }

  if (ui->command_editing && ui->prompt_state != PROMPT_BLOCKED)
    gdb_rl_callback_handler_reinstall ();
}

/* Clean up the FSMs of threads that are now stopped.  In non-stop,
   that's just the event thread.  In all-stop, that's all threads.  In
   all-stop, threads that had a pending exit no longer have a reason
   to be around, as their FSMs/commands are canceled, so we delete
   them.  This avoids "info threads" listing such threads as if they
   were alive (and failing to read their registers), the user being
   able to select and resume them (and that failing), etc.  */

static void
clean_up_just_stopped_threads_fsms (struct execution_control_state *ecs)
{
  /* The first clean_up call below assumes the event thread is the current
     one.  */
  if (ecs->event_thread != nullptr)
    gdb_assert (ecs->event_thread == inferior_thread ());

  if (ecs->event_thread != nullptr
      && ecs->event_thread->thread_fsm () != nullptr)
    ecs->event_thread->thread_fsm ()->clean_up (ecs->event_thread);

  if (!non_stop)
    {
      scoped_restore_current_thread restore_thread;

      for (thread_info *thr : all_threads_safe ())
	{
	  if (thr->state == THREAD_EXITED)
	    continue;

	  if (thr == ecs->event_thread)
	    continue;

	  if (thr->thread_fsm () != nullptr)
	    {
	      switch_to_thread (thr);
	      thr->thread_fsm ()->clean_up (thr);
	    }

	  /* As we are cancelling the command/FSM of this thread,
	     whatever was the reason we needed to report a thread
	     exited event to the user, that reason is gone.  Delete
	     the thread, so that the user doesn't see it in the thread
	     list, the next proceed doesn't try to resume it, etc.  */
	  if (thr->has_pending_waitstatus ()
	      && (thr->pending_waitstatus ().kind ()
		  == TARGET_WAITKIND_THREAD_EXITED))
	    delete_thread (thr);
	}
    }
}

/* Helper for all_uis_check_sync_execution_done that works on the
   current UI.  */

static void
check_curr_ui_sync_execution_done (void)
{
  struct ui *ui = current_ui;

  if (ui->prompt_state == PROMPT_NEEDED
      && ui->async
      && !gdb_in_secondary_prompt_p (ui))
    {
      target_terminal::ours ();
      top_level_interpreter ()->on_sync_execution_done ();
      ui->register_file_handler ();
    }
}

/* See infrun.h.  */

void
all_uis_check_sync_execution_done (void)
{
  SWITCH_THRU_ALL_UIS ()
    {
      check_curr_ui_sync_execution_done ();
    }
}

/* See infrun.h.  */

void
all_uis_on_sync_execution_starting (void)
{
  SWITCH_THRU_ALL_UIS ()
    {
      if (current_ui->prompt_state == PROMPT_NEEDED)
	async_disable_stdin ();
    }
}

/* A quit_handler callback installed while we're handling inferior
   events.  */

static void
infrun_quit_handler ()
{
  if (target_terminal::is_ours ())
    {
      /* Do nothing.

	 default_quit_handler would throw a quit in this case, but if
	 we're handling an event while we have the terminal, it means
	 the target is running a background execution command, and
	 thus when users press Ctrl-C, they're wanting to interrupt
	 whatever command they were executing in the command line.
	 E.g.:

	  (gdb) c&
	  (gdb) foo bar whatever<ctrl-c>

	 That Ctrl-C should clear the input line, not interrupt event
	 handling if it happens that the user types Ctrl-C at just the
	 "wrong" time!

	 It's as-if background event handling was handled by a
	 separate background thread.

	 To be clear, the Ctrl-C is not lost -- it will be processed
	 by the next QUIT call once we're out of fetch_inferior_event
	 again.  */
    }
  else
    {
      if (check_quit_flag ())
	target_pass_ctrlc ();
    }
}

/* Asynchronous version of wait_for_inferior.  It is called by the
   event loop whenever a change of state is detected on the file
   descriptor corresponding to the target.  It can be called more than
   once to complete a single execution command.  In such cases we need
   to keep the state in a global variable ECSS.  If it is the last time
   that this function is called for a single execution command, then
   report to the user that the inferior has stopped, and do the
   necessary cleanups.  */

void
fetch_inferior_event ()
{
  INFRUN_SCOPED_DEBUG_ENTER_EXIT;

  execution_control_state ecs;
  int cmd_done = 0;

  /* Events are always processed with the main UI as current UI.  This
     way, warnings, debug output, etc. are always consistently sent to
     the main console.  */
  scoped_restore save_ui = make_scoped_restore (&current_ui, main_ui);

  /* Temporarily disable pagination.  Otherwise, the user would be
     given an option to press 'q' to quit, which would cause an early
     exit and could leave GDB in a half-baked state.  */
  scoped_restore save_pagination
    = make_scoped_restore (&pagination_enabled, false);

  /* Install a quit handler that does nothing if we have the terminal
     (meaning the target is running a background execution command),
     so that Ctrl-C never interrupts GDB before the event is fully
     handled.  */
  scoped_restore restore_quit_handler
    = make_scoped_restore (&quit_handler, infrun_quit_handler);

  /* Make sure a SIGINT does not interrupt an extension language while
     we're handling an event.  That could interrupt a Python unwinder
     or a Python observer or some such.  A Ctrl-C should either be
     forwarded to the inferior if the inferior has the terminal, or,
     if GDB has the terminal, should interrupt the command the user is
     typing in the CLI.  */
  scoped_disable_cooperative_sigint_handling restore_coop_sigint;

  /* End up with readline processing input, if necessary.  */
  {
    SCOPE_EXIT { reinstall_readline_callback_handler_cleanup (); };

    /* We're handling a live event, so make sure we're doing live
       debugging.  If we're looking at traceframes while the target is
       running, we're going to need to get back to that mode after
       handling the event.  */
    std::optional<scoped_restore_current_traceframe> maybe_restore_traceframe;
    if (non_stop)
      {
	maybe_restore_traceframe.emplace ();
	set_current_traceframe (-1);
      }

    /* The user/frontend should not notice a thread switch due to
       internal events.  Make sure we revert to the user selected
       thread and frame after handling the event and running any
       breakpoint commands.  */
    scoped_restore_current_thread restore_thread;

    overlay_cache_invalid = 1;
    /* Flush target cache before starting to handle each event.  Target
       was running and cache could be stale.  This is just a heuristic.
       Running threads may modify target memory, but we don't get any
       event.  */
    target_dcache_invalidate (current_program_space->aspace);

    scoped_restore save_exec_dir
      = make_scoped_restore (&execution_direction,
			     target_execution_direction ());

    /* Allow targets to pause their resumed threads while we handle
       the event.  */
    scoped_disable_commit_resumed disable_commit_resumed ("handling event");

    if (!do_target_wait (&ecs, TARGET_WNOHANG))
      {
	infrun_debug_printf ("do_target_wait returned no event");
	disable_commit_resumed.reset_and_commit ();
	return;
      }

    gdb_assert (ecs.ws.kind () != TARGET_WAITKIND_IGNORE);

    /* Switch to the inferior that generated the event, so we can do
       target calls.  If the event was not associated to a ptid,  */
    if (ecs.ptid != null_ptid
	&& ecs.ptid != minus_one_ptid)
      switch_to_inferior_no_thread (find_inferior_ptid (ecs.target, ecs.ptid));
    else
      switch_to_target_no_thread (ecs.target);

    if (debug_infrun)
      print_target_wait_results (minus_one_ptid, ecs.ptid, ecs.ws);

    /* If an error happens while handling the event, propagate GDB's
       knowledge of the executing state to the frontend/user running
       state.  */
    ptid_t finish_ptid = !target_is_non_stop_p () ? minus_one_ptid : ecs.ptid;
    scoped_finish_thread_state finish_state (ecs.target, finish_ptid);

    /* Get executed before scoped_restore_current_thread above to apply
       still for the thread which has thrown the exception.  */
    auto defer_bpstat_clear
      = make_scope_exit (bpstat_clear_actions);
    auto defer_delete_threads
      = make_scope_exit (delete_just_stopped_threads_infrun_breakpoints);

    int stop_id = get_stop_id ();

    /* Now figure out what to do with the result of the result.  */
    handle_inferior_event (&ecs);

    if (!ecs.wait_some_more)
      {
	struct inferior *inf = find_inferior_ptid (ecs.target, ecs.ptid);
	bool should_stop = true;
	struct thread_info *thr = ecs.event_thread;

	delete_just_stopped_threads_infrun_breakpoints ();

	if (thr != nullptr && thr->thread_fsm () != nullptr)
	  should_stop = thr->thread_fsm ()->should_stop (thr);

	if (!should_stop)
	  {
	    keep_going (&ecs);
	  }
	else
	  {
	    bool should_notify_stop = true;
	    bool proceeded = false;

	    stop_all_threads_if_all_stop_mode ();

	    clean_up_just_stopped_threads_fsms (&ecs);

	    if (stop_id != get_stop_id ())
	      {
		/* If the stop-id has changed then a stop has already been
		   presented to the user in handle_inferior_event, this is
		   likely a failed inferior call.  As the stop has already
		   been announced then we should not notify again.

		   Also, if the prompt state is not PROMPT_NEEDED then GDB
		   will not be ready for user input after this function.  */
		should_notify_stop = false;
		gdb_assert (current_ui->prompt_state == PROMPT_NEEDED);
	      }
	    else if (thr != nullptr && thr->thread_fsm () != nullptr)
	      should_notify_stop
	       = thr->thread_fsm ()->should_notify_stop ();

	    if (should_notify_stop)
	      {
		/* We may not find an inferior if this was a process exit.  */
		if (inf == nullptr || inf->control.stop_soon == NO_STOP_QUIETLY)
		  proceeded = normal_stop ();
	      }

	    if (!proceeded)
	      {
		inferior_event_handler (INF_EXEC_COMPLETE);
		cmd_done = 1;
	      }

	    /* If we got a TARGET_WAITKIND_NO_RESUMED event, then the
	       previously selected thread is gone.  We have two
	       choices - switch to no thread selected, or restore the
	       previously selected thread (now exited).  We chose the
	       later, just because that's what GDB used to do.  After
	       this, "info threads" says "The current thread <Thread
	       ID 2> has terminated." instead of "No thread
	       selected.".  */
	    if (!non_stop
		&& cmd_done
		&& ecs.ws.kind () != TARGET_WAITKIND_NO_RESUMED)
	      restore_thread.dont_restore ();
	  }
      }

    defer_delete_threads.release ();
    defer_bpstat_clear.release ();

    /* No error, don't finish the thread states yet.  */
    finish_state.release ();

    disable_commit_resumed.reset_and_commit ();

    /* This scope is used to ensure that readline callbacks are
       reinstalled here.  */
  }

  /* Handling this event might have caused some inferiors to become prunable.
     For example, the exit of an inferior that was automatically added.  Try
     to get rid of them.  Keeping those around slows down things linearly.

     Note that this never removes the current inferior.  Therefore, call this
     after RESTORE_THREAD went out of scope, in case the event inferior (which was
     temporarily made the current inferior) is meant to be deleted.

     Call this before all_uis_check_sync_execution_done, so that notifications about
     removed inferiors appear before the prompt.  */
  prune_inferiors ();

  /* If a UI was in sync execution mode, and now isn't, restore its
     prompt (a synchronous execution command has finished, and we're
     ready for input).  */
  all_uis_check_sync_execution_done ();

  if (cmd_done
      && exec_done_display_p
      && (inferior_ptid == null_ptid
	  || inferior_thread ()->state != THREAD_RUNNING))
    gdb_printf (_("completed.\n"));
}

/* See infrun.h.  */

void
set_step_info (thread_info *tp, frame_info_ptr frame,
	       struct symtab_and_line sal)
{
  /* This can be removed once this function no longer implicitly relies on the
     inferior_ptid value.  */
  gdb_assert (inferior_ptid == tp->ptid);

  tp->control.step_frame_id = get_frame_id (frame);
  tp->control.step_stack_frame_id = get_stack_frame_id (frame);

  tp->current_symtab = sal.symtab;
  tp->current_line = sal.line;

  infrun_debug_printf
    ("symtab = %s, line = %d, step_frame_id = %s, step_stack_frame_id = %s",
     tp->current_symtab != nullptr ? tp->current_symtab->filename : "<null>",
     tp->current_line,
     tp->control.step_frame_id.to_string ().c_str (),
     tp->control.step_stack_frame_id.to_string ().c_str ());
}

/* Clear context switchable stepping state.  */

void
init_thread_stepping_state (struct thread_info *tss)
{
  tss->stepped_breakpoint = 0;
  tss->stepping_over_breakpoint = 0;
  tss->stepping_over_watchpoint = 0;
  tss->step_after_step_resume_breakpoint = 0;
}

/* See infrun.h.  */

void
set_last_target_status (process_stratum_target *target, ptid_t ptid,
			const target_waitstatus &status)
{
  target_last_proc_target = target;
  target_last_wait_ptid = ptid;
  target_last_waitstatus = status;
}

/* See infrun.h.  */

void
get_last_target_status (process_stratum_target **target, ptid_t *ptid,
			target_waitstatus *status)
{
  if (target != nullptr)
    *target = target_last_proc_target;
  if (ptid != nullptr)
    *ptid = target_last_wait_ptid;
  if (status != nullptr)
    *status = target_last_waitstatus;
}

/* See infrun.h.  */

void
nullify_last_target_wait_ptid (void)
{
  target_last_proc_target = nullptr;
  target_last_wait_ptid = minus_one_ptid;
  target_last_waitstatus = {};
}

/* Switch thread contexts.  */

static void
context_switch (execution_control_state *ecs)
{
  if (ecs->ptid != inferior_ptid
      && (inferior_ptid == null_ptid
	  || ecs->event_thread != inferior_thread ()))
    {
      infrun_debug_printf ("Switching context from %s to %s",
			   inferior_ptid.to_string ().c_str (),
			   ecs->ptid.to_string ().c_str ());
    }

  switch_to_thread (ecs->event_thread);
}

/* If the target can't tell whether we've hit breakpoints
   (target_supports_stopped_by_sw_breakpoint), and we got a SIGTRAP,
   check whether that could have been caused by a breakpoint.  If so,
   adjust the PC, per gdbarch_decr_pc_after_break.  */

static void
adjust_pc_after_break (struct thread_info *thread,
		       const target_waitstatus &ws)
{
  struct regcache *regcache;
  struct gdbarch *gdbarch;
  CORE_ADDR breakpoint_pc, decr_pc;

  /* If we've hit a breakpoint, we'll normally be stopped with SIGTRAP.  If
     we aren't, just return.

     We assume that waitkinds other than TARGET_WAITKIND_STOPPED are not
     affected by gdbarch_decr_pc_after_break.  Other waitkinds which are
     implemented by software breakpoints should be handled through the normal
     breakpoint layer.

     NOTE drow/2004-01-31: On some targets, breakpoints may generate
     different signals (SIGILL or SIGEMT for instance), but it is less
     clear where the PC is pointing afterwards.  It may not match
     gdbarch_decr_pc_after_break.  I don't know any specific target that
     generates these signals at breakpoints (the code has been in GDB since at
     least 1992) so I can not guess how to handle them here.

     In earlier versions of GDB, a target with 
     gdbarch_have_nonsteppable_watchpoint would have the PC after hitting a
     watchpoint affected by gdbarch_decr_pc_after_break.  I haven't found any
     target with both of these set in GDB history, and it seems unlikely to be
     correct, so gdbarch_have_nonsteppable_watchpoint is not checked here.  */

  if (ws.kind () != TARGET_WAITKIND_STOPPED)
    return;

  if (ws.sig () != GDB_SIGNAL_TRAP)
    return;

  /* In reverse execution, when a breakpoint is hit, the instruction
     under it has already been de-executed.  The reported PC always
     points at the breakpoint address, so adjusting it further would
     be wrong.  E.g., consider this case on a decr_pc_after_break == 1
     architecture:

       B1         0x08000000 :   INSN1
       B2         0x08000001 :   INSN2
		  0x08000002 :   INSN3
	    PC -> 0x08000003 :   INSN4

     Say you're stopped at 0x08000003 as above.  Reverse continuing
     from that point should hit B2 as below.  Reading the PC when the
     SIGTRAP is reported should read 0x08000001 and INSN2 should have
     been de-executed already.

       B1         0x08000000 :   INSN1
       B2   PC -> 0x08000001 :   INSN2
		  0x08000002 :   INSN3
		  0x08000003 :   INSN4

     We can't apply the same logic as for forward execution, because
     we would wrongly adjust the PC to 0x08000000, since there's a
     breakpoint at PC - 1.  We'd then report a hit on B1, although
     INSN1 hadn't been de-executed yet.  Doing nothing is the correct
     behaviour.  */
  if (execution_direction == EXEC_REVERSE)
    return;

  /* If the target can tell whether the thread hit a SW breakpoint,
     trust it.  Targets that can tell also adjust the PC
     themselves.  */
  if (target_supports_stopped_by_sw_breakpoint ())
    return;

  /* Note that relying on whether a breakpoint is planted in memory to
     determine this can fail.  E.g,. the breakpoint could have been
     removed since.  Or the thread could have been told to step an
     instruction the size of a breakpoint instruction, and only
     _after_ was a breakpoint inserted at its address.  */

  /* If this target does not decrement the PC after breakpoints, then
     we have nothing to do.  */
  regcache = get_thread_regcache (thread);
  gdbarch = regcache->arch ();

  decr_pc = gdbarch_decr_pc_after_break (gdbarch);
  if (decr_pc == 0)
    return;

  const address_space *aspace = thread->inf->aspace.get ();

  /* Find the location where (if we've hit a breakpoint) the
     breakpoint would be.  */
  breakpoint_pc = regcache_read_pc (regcache) - decr_pc;

  /* If the target can't tell whether a software breakpoint triggered,
     fallback to figuring it out based on breakpoints we think were
     inserted in the target, and on whether the thread was stepped or
     continued.  */

  /* Check whether there actually is a software breakpoint inserted at
     that location.

     If in non-stop mode, a race condition is possible where we've
     removed a breakpoint, but stop events for that breakpoint were
     already queued and arrive later.  To suppress those spurious
     SIGTRAPs, we keep a list of such breakpoint locations for a bit,
     and retire them after a number of stop events are reported.  Note
     this is an heuristic and can thus get confused.  The real fix is
     to get the "stopped by SW BP and needs adjustment" info out of
     the target/kernel (and thus never reach here; see above).  */
  if (software_breakpoint_inserted_here_p (aspace, breakpoint_pc)
      || (target_is_non_stop_p ()
	  && moribund_breakpoint_here_p (aspace, breakpoint_pc)))
    {
      std::optional<scoped_restore_tmpl<int>> restore_operation_disable;

      if (record_full_is_used ())
	restore_operation_disable.emplace
	  (record_full_gdb_operation_disable_set ());

      /* When using hardware single-step, a SIGTRAP is reported for both
	 a completed single-step and a software breakpoint.  Need to
	 differentiate between the two, as the latter needs adjusting
	 but the former does not.

	 The SIGTRAP can be due to a completed hardware single-step only if 
	  - we didn't insert software single-step breakpoints
	  - this thread is currently being stepped

	 If any of these events did not occur, we must have stopped due
	 to hitting a software breakpoint, and have to back up to the
	 breakpoint address.

	 As a special case, we could have hardware single-stepped a
	 software breakpoint.  In this case (prev_pc == breakpoint_pc),
	 we also need to back up to the breakpoint address.  */

      if (thread_has_single_step_breakpoints_set (thread)
	  || !currently_stepping (thread)
	  || (thread->stepped_breakpoint
	      && thread->prev_pc == breakpoint_pc))
	regcache_write_pc (regcache, breakpoint_pc);
    }
}

static bool
stepped_in_from (frame_info_ptr frame, struct frame_id step_frame_id)
{
  for (frame = get_prev_frame (frame);
       frame != nullptr;
       frame = get_prev_frame (frame))
    {
      if (get_frame_id (frame) == step_frame_id)
	return true;

      if (get_frame_type (frame) != INLINE_FRAME)
	break;
    }

  return false;
}

/* Look for an inline frame that is marked for skip.
   If PREV_FRAME is TRUE start at the previous frame,
   otherwise start at the current frame.  Stop at the
   first non-inline frame, or at the frame where the
   step started.  */

static bool
inline_frame_is_marked_for_skip (bool prev_frame, struct thread_info *tp)
{
  frame_info_ptr frame = get_current_frame ();

  if (prev_frame)
    frame = get_prev_frame (frame);

  for (; frame != nullptr; frame = get_prev_frame (frame))
    {
      const char *fn = nullptr;
      symtab_and_line sal;
      struct symbol *sym;

      if (get_frame_id (frame) == tp->control.step_frame_id)
	break;
      if (get_frame_type (frame) != INLINE_FRAME)
	break;

      sal = find_frame_sal (frame);
      sym = get_frame_function (frame);

      if (sym != nullptr)
	fn = sym->print_name ();

      if (sal.line != 0
	  && function_name_is_marked_for_skip (fn, sal))
	return true;
    }

  return false;
}

/* If the event thread has the stop requested flag set, pretend it
   stopped for a GDB_SIGNAL_0 (i.e., as if it stopped due to
   target_stop).  */

static bool
handle_stop_requested (struct execution_control_state *ecs)
{
  if (ecs->event_thread->stop_requested)
    {
      ecs->ws.set_stopped (GDB_SIGNAL_0);
      handle_signal_stop (ecs);
      return true;
    }
  return false;
}

/* Auxiliary function that handles syscall entry/return events.
   It returns true if the inferior should keep going (and GDB
   should ignore the event), or false if the event deserves to be
   processed.  */

static bool
handle_syscall_event (struct execution_control_state *ecs)
{
  struct regcache *regcache;
  int syscall_number;

  context_switch (ecs);

  regcache = get_thread_regcache (ecs->event_thread);
  syscall_number = ecs->ws.syscall_number ();
  ecs->event_thread->set_stop_pc (regcache_read_pc (regcache));

  if (catch_syscall_enabled ()
      && catching_syscall_number (syscall_number))
    {
      infrun_debug_printf ("syscall number=%d", syscall_number);

      ecs->event_thread->control.stop_bpstat
	= bpstat_stop_status_nowatch (ecs->event_thread->inf->aspace.get (),
				      ecs->event_thread->stop_pc (),
				      ecs->event_thread, ecs->ws);

      if (handle_stop_requested (ecs))
	return false;

      if (bpstat_causes_stop (ecs->event_thread->control.stop_bpstat))
	{
	  /* Catchpoint hit.  */
	  return false;
	}
    }

  if (handle_stop_requested (ecs))
    return false;

  /* If no catchpoint triggered for this, then keep going.  */
  keep_going (ecs);

  return true;
}

/* Lazily fill in the execution_control_state's stop_func_* fields.  */

static void
fill_in_stop_func (struct gdbarch *gdbarch,
		   struct execution_control_state *ecs)
{
  if (!ecs->stop_func_filled_in)
    {
      const block *block;
      const general_symbol_info *gsi;

      /* Don't care about return value; stop_func_start and stop_func_name
	 will both be 0 if it doesn't work.  */
      find_pc_partial_function_sym (ecs->event_thread->stop_pc (),
				    &gsi,
				    &ecs->stop_func_start,
				    &ecs->stop_func_end,
				    &block);
      ecs->stop_func_name = gsi == nullptr ? nullptr : gsi->print_name ();

      /* The call to find_pc_partial_function, above, will set
	 stop_func_start and stop_func_end to the start and end
	 of the range containing the stop pc.  If this range
	 contains the entry pc for the block (which is always the
	 case for contiguous blocks), advance stop_func_start past
	 the function's start offset and entrypoint.  Note that
	 stop_func_start is NOT advanced when in a range of a
	 non-contiguous block that does not contain the entry pc.  */
      if (block != nullptr
	  && ecs->stop_func_start <= block->entry_pc ()
	  && block->entry_pc () < ecs->stop_func_end)
	{
	  ecs->stop_func_start
	    += gdbarch_deprecated_function_start_offset (gdbarch);

	  /* PowerPC functions have a Local Entry Point (LEP) and a Global
	     Entry Point (GEP).  There is only one Entry Point (GEP = LEP) for
	     other architectures.  */
	  ecs->stop_func_alt_start = ecs->stop_func_start;

	  if (gdbarch_skip_entrypoint_p (gdbarch))
	    ecs->stop_func_start
	      = gdbarch_skip_entrypoint (gdbarch, ecs->stop_func_start);
	}

      ecs->stop_func_filled_in = 1;
    }
}


/* Return the STOP_SOON field of the inferior pointed at by ECS.  */

static enum stop_kind
get_inferior_stop_soon (execution_control_state *ecs)
{
  struct inferior *inf = find_inferior_ptid (ecs->target, ecs->ptid);

  gdb_assert (inf != nullptr);
  return inf->control.stop_soon;
}

/* Poll for one event out of the current target.  Store the resulting
   waitstatus in WS, and return the event ptid.  Does not block.  */

static ptid_t
poll_one_curr_target (struct target_waitstatus *ws)
{
  ptid_t event_ptid;

  overlay_cache_invalid = 1;

  /* Flush target cache before starting to handle each event.
     Target was running and cache could be stale.  This is just a
     heuristic.  Running threads may modify target memory, but we
     don't get any event.  */
  target_dcache_invalidate (current_program_space->aspace);

  event_ptid = target_wait (minus_one_ptid, ws, TARGET_WNOHANG);

  if (debug_infrun)
    print_target_wait_results (minus_one_ptid, event_ptid, *ws);

  return event_ptid;
}

/* Wait for one event out of any target.  */

static wait_one_event
wait_one ()
{
  while (1)
    {
      for (inferior *inf : all_inferiors ())
	{
	  process_stratum_target *target = inf->process_target ();
	  if (target == nullptr
	      || !target->is_async_p ()
	      || !target->threads_executing)
	    continue;

	  switch_to_inferior_no_thread (inf);

	  wait_one_event event;
	  event.target = target;
	  event.ptid = poll_one_curr_target (&event.ws);

	  if (event.ws.kind () == TARGET_WAITKIND_NO_RESUMED)
	    {
	      /* If nothing is resumed, remove the target from the
		 event loop.  */
	      target_async (false);
	    }
	  else if (event.ws.kind () != TARGET_WAITKIND_IGNORE)
	    return event;
	}

      /* Block waiting for some event.  */

      fd_set readfds;
      int nfds = 0;

      FD_ZERO (&readfds);

      for (inferior *inf : all_inferiors ())
	{
	  process_stratum_target *target = inf->process_target ();
	  if (target == nullptr
	      || !target->is_async_p ()
	      || !target->threads_executing)
	    continue;

	  int fd = target->async_wait_fd ();
	  FD_SET (fd, &readfds);
	  if (nfds <= fd)
	    nfds = fd + 1;
	}

      if (nfds == 0)
	{
	  /* No waitable targets left.  All must be stopped.  */
	  infrun_debug_printf ("no waitable targets left");

	  target_waitstatus ws;
	  ws.set_no_resumed ();
	  return {nullptr, minus_one_ptid, std::move (ws)};
	}

      QUIT;

      int numfds = interruptible_select (nfds, &readfds, 0, nullptr, 0);
      if (numfds < 0)
	{
	  if (errno == EINTR)
	    continue;
	  else
	    perror_with_name ("interruptible_select");
	}
    }
}

/* Save the thread's event and stop reason to process it later.  */

static void
save_waitstatus (struct thread_info *tp, const target_waitstatus &ws)
{
  infrun_debug_printf ("saving status %s for %s",
		       ws.to_string ().c_str (),
		       tp->ptid.to_string ().c_str ());

  /* Record for later.  */
  tp->set_pending_waitstatus (ws);

  if (ws.kind () == TARGET_WAITKIND_STOPPED
      && ws.sig () == GDB_SIGNAL_TRAP)
    {
      struct regcache *regcache = get_thread_regcache (tp);
      const address_space *aspace = tp->inf->aspace.get ();
      CORE_ADDR pc = regcache_read_pc (regcache);

      adjust_pc_after_break (tp, tp->pending_waitstatus ());

      scoped_restore_current_thread restore_thread;
      switch_to_thread (tp);

      if (target_stopped_by_watchpoint ())
	tp->set_stop_reason (TARGET_STOPPED_BY_WATCHPOINT);
      else if (target_supports_stopped_by_sw_breakpoint ()
	       && target_stopped_by_sw_breakpoint ())
	tp->set_stop_reason (TARGET_STOPPED_BY_SW_BREAKPOINT);
      else if (target_supports_stopped_by_hw_breakpoint ()
	       && target_stopped_by_hw_breakpoint ())
	tp->set_stop_reason (TARGET_STOPPED_BY_HW_BREAKPOINT);
      else if (!target_supports_stopped_by_hw_breakpoint ()
	       && hardware_breakpoint_inserted_here_p (aspace, pc))
	tp->set_stop_reason (TARGET_STOPPED_BY_HW_BREAKPOINT);
      else if (!target_supports_stopped_by_sw_breakpoint ()
	       && software_breakpoint_inserted_here_p (aspace, pc))
	tp->set_stop_reason (TARGET_STOPPED_BY_SW_BREAKPOINT);
      else if (!thread_has_single_step_breakpoints_set (tp)
	       && currently_stepping (tp))
	tp->set_stop_reason (TARGET_STOPPED_BY_SINGLE_STEP);
    }
}

/* Mark the non-executing threads accordingly.  In all-stop, all
   threads of all processes are stopped when we get any event
   reported.  In non-stop mode, only the event thread stops.  */

static void
mark_non_executing_threads (process_stratum_target *target,
			    ptid_t event_ptid,
			    const target_waitstatus &ws)
{
  ptid_t mark_ptid;

  if (!target_is_non_stop_p ())
    mark_ptid = minus_one_ptid;
  else if (ws.kind () == TARGET_WAITKIND_SIGNALLED
	   || ws.kind () == TARGET_WAITKIND_EXITED)
    {
      /* If we're handling a process exit in non-stop mode, even
	 though threads haven't been deleted yet, one would think
	 that there is nothing to do, as threads of the dead process
	 will be soon deleted, and threads of any other process were
	 left running.  However, on some targets, threads survive a
	 process exit event.  E.g., for the "checkpoint" command,
	 when the current checkpoint/fork exits, linux-fork.c
	 automatically switches to another fork from within
	 target_mourn_inferior, by associating the same
	 inferior/thread to another fork.  We haven't mourned yet at
	 this point, but we must mark any threads left in the
	 process as not-executing so that finish_thread_state marks
	 them stopped (in the user's perspective) if/when we present
	 the stop to the user.  */
      mark_ptid = ptid_t (event_ptid.pid ());
    }
  else
    mark_ptid = event_ptid;

  set_executing (target, mark_ptid, false);

  /* Likewise the resumed flag.  */
  set_resumed (target, mark_ptid, false);
}

/* Handle one event after stopping threads.  If the eventing thread
   reports back any interesting event, we leave it pending.  If the
   eventing thread was in the middle of a displaced step, we
   cancel/finish it, and unless the thread's inferior is being
   detached, put the thread back in the step-over chain.  Returns true
   if there are no resumed threads left in the target (thus there's no
   point in waiting further), false otherwise.  */

static bool
handle_one (const wait_one_event &event)
{
  infrun_debug_printf
    ("%s %s", event.ws.to_string ().c_str (),
     event.ptid.to_string ().c_str ());

  if (event.ws.kind () == TARGET_WAITKIND_NO_RESUMED)
    {
      /* All resumed threads exited.  */
      return true;
    }
  else if (event.ws.kind () == TARGET_WAITKIND_THREAD_EXITED
	   || event.ws.kind () == TARGET_WAITKIND_EXITED
	   || event.ws.kind () == TARGET_WAITKIND_SIGNALLED)
    {
      /* One thread/process exited/signalled.  */

      thread_info *t = nullptr;

      /* The target may have reported just a pid.  If so, try
	 the first non-exited thread.  */
      if (event.ptid.is_pid ())
	{
	  int pid  = event.ptid.pid ();
	  inferior *inf = find_inferior_pid (event.target, pid);
	  for (thread_info *tp : inf->non_exited_threads ())
	    {
	      t = tp;
	      break;
	    }

	  /* If there is no available thread, the event would
	     have to be appended to a per-inferior event list,
	     which does not exist (and if it did, we'd have
	     to adjust run control command to be able to
	     resume such an inferior).  We assert here instead
	     of going into an infinite loop.  */
	  gdb_assert (t != nullptr);

	  infrun_debug_printf
	    ("using %s", t->ptid.to_string ().c_str ());
	}
      else
	{
	  t = event.target->find_thread (event.ptid);
	  /* Check if this is the first time we see this thread.
	     Don't bother adding if it individually exited.  */
	  if (t == nullptr
	      && event.ws.kind () != TARGET_WAITKIND_THREAD_EXITED)
	    t = add_thread (event.target, event.ptid);
	}

      if (t != nullptr)
	{
	  /* Set the threads as non-executing to avoid
	     another stop attempt on them.  */
	  switch_to_thread_no_regs (t);
	  mark_non_executing_threads (event.target, event.ptid,
				      event.ws);
	  save_waitstatus (t, event.ws);
	  t->stop_requested = false;

	  if (event.ws.kind () == TARGET_WAITKIND_THREAD_EXITED)
	    {
	      if (displaced_step_finish (t, event.ws)
		  != DISPLACED_STEP_FINISH_STATUS_OK)
		{
		  gdb_assert_not_reached ("displaced_step_finish on "
					  "exited thread failed");
		}
	    }
	}
    }
  else
    {
      thread_info *t = event.target->find_thread (event.ptid);
      if (t == nullptr)
	t = add_thread (event.target, event.ptid);

      t->stop_requested = 0;
      t->set_executing (false);
      t->set_resumed (false);
      t->control.may_range_step = 0;

      /* This may be the first time we see the inferior report
	 a stop.  */
      if (t->inf->needs_setup)
	{
	  switch_to_thread_no_regs (t);
	  setup_inferior (0);
	}

      if (event.ws.kind () == TARGET_WAITKIND_STOPPED
	  && event.ws.sig () == GDB_SIGNAL_0)
	{
	  /* We caught the event that we intended to catch, so
	     there's no event to save as pending.  */

	  if (displaced_step_finish (t, event.ws)
	      == DISPLACED_STEP_FINISH_STATUS_NOT_EXECUTED)
	    {
	      /* Add it back to the step-over queue.  */
	      infrun_debug_printf
		("displaced-step of %s canceled",
		 t->ptid.to_string ().c_str ());

	      t->control.trap_expected = 0;
	      if (!t->inf->detaching)
		global_thread_step_over_chain_enqueue (t);
	    }
	}
      else
	{
	  struct regcache *regcache;

	  infrun_debug_printf
	    ("target_wait %s, saving status for %s",
	     event.ws.to_string ().c_str (),
	     t->ptid.to_string ().c_str ());

	  /* Record for later.  */
	  save_waitstatus (t, event.ws);

	  if (displaced_step_finish (t, event.ws)
	      == DISPLACED_STEP_FINISH_STATUS_NOT_EXECUTED)
	    {
	      /* Add it back to the step-over queue.  */
	      t->control.trap_expected = 0;
	      if (!t->inf->detaching)
		global_thread_step_over_chain_enqueue (t);
	    }

	  regcache = get_thread_regcache (t);
	  t->set_stop_pc (regcache_read_pc (regcache));

	  infrun_debug_printf ("saved stop_pc=%s for %s "
			       "(currently_stepping=%d)",
			       paddress (current_inferior ()->arch (),
					 t->stop_pc ()),
			       t->ptid.to_string ().c_str (),
			       currently_stepping (t));
	}
    }

  return false;
}

/* Helper for stop_all_threads.  wait_one waits for events until it
   sees a TARGET_WAITKIND_NO_RESUMED event.  When it sees one, it
   disables target_async for the target to stop waiting for events
   from it.  TARGET_WAITKIND_NO_RESUMED can be delayed though,
   consider, debugging against gdbserver:

    #1 - Threads 1-5 are running, and thread 1 hits a breakpoint.

    #2 - gdb processes the breakpoint hit for thread 1, stops all
	 threads, and steps thread 1 over the breakpoint.  while
	 stopping threads, some other threads reported interesting
	 events, which were left pending in the thread's objects
	 (infrun's queue).

    #2 - Thread 1 exits (it stepped an exit syscall), and gdbserver
	 reports the thread exit for thread 1.	The event ends up in
	 remote's stop reply queue.

    #3 - That was the last resumed thread, so gdbserver reports
	 no-resumed, and that event also ends up in remote's stop
	 reply queue, queued after the thread exit from #2.

    #4 - gdb processes the thread exit event, which finishes the
	 step-over, and so gdb restarts all threads (threads with
	 pending events are left marked resumed, but aren't set
	 executing).  The no-resumed event is still left pending in
	 the remote stop reply queue.

    #5 - Since there are now resumed threads with pending breakpoint
	 hits, gdb picks one at random to process next.

    #5 - gdb picks the breakpoint hit for thread 2 this time, and that
	 breakpoint also needs to be stepped over, so gdb stops all
	 threads again.

    #6 - stop_all_threads counts number of expected stops and calls
	 wait_one once for each.

    #7 - The first wait_one call collects the no-resumed event from #3
	 above.

    #9 - Seeing the no-resumed event, wait_one disables target async
	 for the remote target, to stop waiting for events from it.
	 wait_one from here on always return no-resumed directly
	 without reaching the target.

    #10 - stop_all_threads still hasn't seen all the stops it expects,
	  so it does another pass.

    #11 - Since the remote target is not async (disabled in #9),
	  wait_one doesn't wait on it, so it won't see the expected
	  stops, and instead returns no-resumed directly.

    #12 - stop_all_threads still haven't seen all the stops, so it
	  does another pass.  goto #11, looping forever.

   To handle this, we explicitly (re-)enable target async on all
   targets that can async every time stop_all_threads goes wait for
   the expected stops.  */

static void
reenable_target_async ()
{
  for (inferior *inf : all_inferiors ())
    {
      process_stratum_target *target = inf->process_target ();
      if (target != nullptr
	  && target->threads_executing
	  && target->can_async_p ()
	  && !target->is_async_p ())
	{
	  switch_to_inferior_no_thread (inf);
	  target_async (1);
	}
    }
}

/* See infrun.h.  */

void
stop_all_threads (const char *reason, inferior *inf)
{
  /* We may need multiple passes to discover all threads.  */
  int pass;
  int iterations = 0;

  gdb_assert (exists_non_stop_target ());

  INFRUN_SCOPED_DEBUG_START_END ("reason=%s, inf=%d", reason,
				 inf != nullptr ? inf->num : -1);

  infrun_debug_show_threads ("non-exited threads",
			     all_non_exited_threads ());

  scoped_restore_current_thread restore_thread;

  /* Enable thread events on relevant targets.  */
  for (auto *target : all_non_exited_process_targets ())
    {
      if (inf != nullptr && inf->process_target () != target)
	continue;

      switch_to_target_no_thread (target);
      target_thread_events (true);
    }

  SCOPE_EXIT
    {
      /* Disable thread events on relevant targets.  */
      for (auto *target : all_non_exited_process_targets ())
	{
	  if (inf != nullptr && inf->process_target () != target)
	    continue;

	  switch_to_target_no_thread (target);
	  target_thread_events (false);
	}

      /* Use debug_prefixed_printf directly to get a meaningful function
	 name.  */
      if (debug_infrun)
	debug_prefixed_printf ("infrun", "stop_all_threads", "done");
    };

  /* Request threads to stop, and then wait for the stops.  Because
     threads we already know about can spawn more threads while we're
     trying to stop them, and we only learn about new threads when we
     update the thread list, do this in a loop, and keep iterating
     until two passes find no threads that need to be stopped.  */
  for (pass = 0; pass < 2; pass++, iterations++)
    {
      infrun_debug_printf ("pass=%d, iterations=%d", pass, iterations);
      while (1)
	{
	  int waits_needed = 0;

	  for (auto *target : all_non_exited_process_targets ())
	    {
	      if (inf != nullptr && inf->process_target () != target)
		continue;

	      switch_to_target_no_thread (target);
	      update_thread_list ();
	    }

	  /* Go through all threads looking for threads that we need
	     to tell the target to stop.  */
	  for (thread_info *t : all_non_exited_threads ())
	    {
	      if (inf != nullptr && t->inf != inf)
		continue;

	      /* For a single-target setting with an all-stop target,
		 we would not even arrive here.  For a multi-target
		 setting, until GDB is able to handle a mixture of
		 all-stop and non-stop targets, simply skip all-stop
		 targets' threads.  This should be fine due to the
		 protection of 'check_multi_target_resumption'.  */

	      switch_to_thread_no_regs (t);
	      if (!target_is_non_stop_p ())
		continue;

	      if (t->executing ())
		{
		  /* If already stopping, don't request a stop again.
		     We just haven't seen the notification yet.  */
		  if (!t->stop_requested)
		    {
		      infrun_debug_printf ("  %s executing, need stop",
					   t->ptid.to_string ().c_str ());
		      target_stop (t->ptid);
		      t->stop_requested = 1;
		    }
		  else
		    {
		      infrun_debug_printf ("  %s executing, already stopping",
					   t->ptid.to_string ().c_str ());
		    }

		  if (t->stop_requested)
		    waits_needed++;
		}
	      else
		{
		  infrun_debug_printf ("  %s not executing",
				       t->ptid.to_string ().c_str ());

		  /* The thread may be not executing, but still be
		     resumed with a pending status to process.  */
		  t->set_resumed (false);
		}
	    }

	  if (waits_needed == 0)
	    break;

	  /* If we find new threads on the second iteration, restart
	     over.  We want to see two iterations in a row with all
	     threads stopped.  */
	  if (pass > 0)
	    pass = -1;

	  reenable_target_async ();

	  for (int i = 0; i < waits_needed; i++)
	    {
	      wait_one_event event = wait_one ();
	      if (handle_one (event))
		break;
	    }
	}
    }
}

/* Handle a TARGET_WAITKIND_NO_RESUMED event.  Return true if we
   handled the event and should continue waiting.  Return false if we
   should stop and report the event to the user.  */

static bool
handle_no_resumed (struct execution_control_state *ecs)
{
  if (target_can_async_p ())
    {
      bool any_sync = false;

      for (ui *ui : all_uis ())
	{
	  if (ui->prompt_state == PROMPT_BLOCKED)
	    {
	      any_sync = true;
	      break;
	    }
	}
      if (!any_sync)
	{
	  /* There were no unwaited-for children left in the target, but,
	     we're not synchronously waiting for events either.  Just
	     ignore.  */

	  infrun_debug_printf ("TARGET_WAITKIND_NO_RESUMED (ignoring: bg)");
	  prepare_to_wait (ecs);
	  return true;
	}
    }

  /* Otherwise, if we were running a synchronous execution command, we
     may need to cancel it and give the user back the terminal.

     In non-stop mode, the target can't tell whether we've already
     consumed previous stop events, so it can end up sending us a
     no-resumed event like so:

       #0 - thread 1 is left stopped

       #1 - thread 2 is resumed and hits breakpoint
	       -> TARGET_WAITKIND_STOPPED

       #2 - thread 3 is resumed and exits
	    this is the last resumed thread, so
	       -> TARGET_WAITKIND_NO_RESUMED

       #3 - gdb processes stop for thread 2 and decides to re-resume
	    it.

       #4 - gdb processes the TARGET_WAITKIND_NO_RESUMED event.
	    thread 2 is now resumed, so the event should be ignored.

     IOW, if the stop for thread 2 doesn't end a foreground command,
     then we need to ignore the following TARGET_WAITKIND_NO_RESUMED
     event.  But it could be that the event meant that thread 2 itself
     (or whatever other thread was the last resumed thread) exited.

     To address this we refresh the thread list and check whether we
     have resumed threads _now_.  In the example above, this removes
     thread 3 from the thread list.  If thread 2 was re-resumed, we
     ignore this event.  If we find no thread resumed, then we cancel
     the synchronous command and show "no unwaited-for " to the
     user.  */

  inferior *curr_inf = current_inferior ();

  scoped_restore_current_thread restore_thread;
  update_thread_list ();

  /* If:

       - the current target has no thread executing, and
       - the current inferior is native, and
       - the current inferior is the one which has the terminal, and
       - we did nothing,

     then a Ctrl-C from this point on would remain stuck in the
     kernel, until a thread resumes and dequeues it.  That would
     result in the GDB CLI not reacting to Ctrl-C, not able to
     interrupt the program.  To address this, if the current inferior
     no longer has any thread executing, we give the terminal to some
     other inferior that has at least one thread executing.  */
  bool swap_terminal = true;

  /* Whether to ignore this TARGET_WAITKIND_NO_RESUMED event, or
     whether to report it to the user.  */
  bool ignore_event = false;

  for (thread_info *thread : all_non_exited_threads ())
    {
      if (swap_terminal && thread->executing ())
	{
	  if (thread->inf != curr_inf)
	    {
	      target_terminal::ours ();

	      switch_to_thread (thread);
	      target_terminal::inferior ();
	    }
	  swap_terminal = false;
	}

      if (!ignore_event && thread->resumed ())
	{
	  /* Either there were no unwaited-for children left in the
	     target at some point, but there are now, or some target
	     other than the eventing one has unwaited-for children
	     left.  Just ignore.  */
	  infrun_debug_printf ("TARGET_WAITKIND_NO_RESUMED "
			       "(ignoring: found resumed)");

	  ignore_event = true;
	}

      if (ignore_event && !swap_terminal)
	break;
    }

  if (ignore_event)
    {
      switch_to_inferior_no_thread (curr_inf);
      prepare_to_wait (ecs);
      return true;
    }

  /* Go ahead and report the event.  */
  return false;
}

/* Handle a TARGET_WAITKIND_THREAD_EXITED event.  Return true if we
   handled the event and should continue waiting.  Return false if we
   should stop and report the event to the user.  */

static bool
handle_thread_exited (execution_control_state *ecs)
{
  context_switch (ecs);

  /* Clear these so we don't re-start the thread stepping over a
     breakpoint/watchpoint.  */
  ecs->event_thread->stepping_over_breakpoint = 0;
  ecs->event_thread->stepping_over_watchpoint = 0;

  /* If the thread had an FSM, then abort the command.  But only after
     finishing the step over, as in non-stop mode, aborting this
     thread's command should not interfere with other threads.  We
     must check this before finish_step over, however, which may
     update the thread list and delete the event thread.  */
  bool abort_cmd = (ecs->event_thread->thread_fsm () != nullptr);

  /* Mark the thread exited right now, because finish_step_over may
     update the thread list and that may delete the thread silently
     (depending on target), while we always want to emit the "[Thread
     ... exited]" notification.  Don't actually delete the thread yet,
     because we need to pass its pointer down to finish_step_over.  */
  set_thread_exited (ecs->event_thread);

  /* Maybe the thread was doing a step-over, if so release
     resources and start any further pending step-overs.

     If we are on a non-stop target and the thread was doing an
     in-line step, this also restarts the other threads.  */
  int ret = finish_step_over (ecs);

  /* finish_step_over returns true if it moves ecs' wait status
     back into the thread, so that we go handle another pending
     event before this one.  But we know it never does that if
     the event thread has exited.  */
  gdb_assert (ret == 0);

  if (abort_cmd)
    {
      /* We're stopping for the thread exit event.  Switch to the
	 event thread again, as finish_step_over may have switched
	 threads.  */
      switch_to_thread (ecs->event_thread);
      ecs->event_thread = nullptr;
      return false;
    }

  /* If finish_step_over started a new in-line step-over, don't
     try to restart anything else.  */
  if (step_over_info_valid_p ())
    {
      delete_thread (ecs->event_thread);
      return true;
    }

  /* Maybe we are on an all-stop target and we got this event
     while doing a step-like command on another thread.  If so,
     go back to doing that.  If this thread was stepping,
     switch_back_to_stepped_thread will consider that the thread
     was interrupted mid-step and will try keep stepping it.  We
     don't want that, the thread is gone.  So clear the proceed
     status so it doesn't do that.  */
  clear_proceed_status_thread (ecs->event_thread);
  if (switch_back_to_stepped_thread (ecs))
    {
      delete_thread (ecs->event_thread);
      return true;
    }

  inferior *inf = ecs->event_thread->inf;
  bool slock_applies = schedlock_applies (ecs->event_thread);

  delete_thread (ecs->event_thread);
  ecs->event_thread = nullptr;

  /* Continue handling the event as if we had gotten a
     TARGET_WAITKIND_NO_RESUMED.  */
  auto handle_as_no_resumed = [ecs] ()
  {
    /* handle_no_resumed doesn't really look at the event kind, but
       normal_stop does.  */
    ecs->ws.set_no_resumed ();
    ecs->event_thread = nullptr;
    ecs->ptid = minus_one_ptid;

    /* Re-record the last target status.  */
    set_last_target_status (ecs->target, ecs->ptid, ecs->ws);

    return handle_no_resumed (ecs);
  };

  /* If we are on an all-stop target, the target has stopped all
     threads to report the event.  We don't actually want to
     stop, so restart the threads.  */
  if (!target_is_non_stop_p ())
    {
      if (slock_applies)
	{
	  /* Since the target is !non-stop, then everything is stopped
	     at this point, and we can't assume we'll get further
	     events until we resume the target again.  Handle this
	     event like if it were a TARGET_WAITKIND_NO_RESUMED.  Note
	     this refreshes the thread list and checks whether there
	     are other resumed threads before deciding whether to
	     print "no-unwaited-for left".  This is important because
	     the user could have done:

	      (gdb) set scheduler-locking on
	      (gdb) thread 1
	      (gdb) c&
	      (gdb) thread 2
	      (gdb) c

	     ... and only one of the threads exited.  */
	  return handle_as_no_resumed ();
	}
      else
	{
	  /* Switch to the first non-exited thread we can find, and
	     resume.  */
	  auto range = inf->non_exited_threads ();
	  if (range.begin () == range.end ())
	    {
	      /* Looks like the target reported a
		 TARGET_WAITKIND_THREAD_EXITED for its last known
		 thread.  */
	      return handle_as_no_resumed ();
	    }
	  thread_info *non_exited_thread = *range.begin ();
	  switch_to_thread (non_exited_thread);
	  insert_breakpoints ();
	  resume (GDB_SIGNAL_0);
	}
    }

  prepare_to_wait (ecs);
  return true;
}

/* Given an execution control state that has been freshly filled in by
   an event from the inferior, figure out what it means and take
   appropriate action.

   The alternatives are:

   1) stop_waiting and return; to really stop and return to the
   debugger.

   2) keep_going and return; to wait for the next event (set
   ecs->event_thread->stepping_over_breakpoint to 1 to single step
   once).  */

static void
handle_inferior_event (struct execution_control_state *ecs)
{
  /* Make sure that all temporary struct value objects that were
     created during the handling of the event get deleted at the
     end.  */
  scoped_value_mark free_values;

  infrun_debug_printf ("%s", ecs->ws.to_string ().c_str ());

  if (ecs->ws.kind () == TARGET_WAITKIND_IGNORE)
    {
      /* We had an event in the inferior, but we are not interested in
	 handling it at this level.  The lower layers have already
	 done what needs to be done, if anything.

	 One of the possible circumstances for this is when the
	 inferior produces output for the console.  The inferior has
	 not stopped, and we are ignoring the event.  Another possible
	 circumstance is any event which the lower level knows will be
	 reported multiple times without an intervening resume.  */
      prepare_to_wait (ecs);
      return;
    }

  if (ecs->ws.kind () == TARGET_WAITKIND_NO_RESUMED
      && handle_no_resumed (ecs))
    return;

  /* Cache the last target/ptid/waitstatus.  */
  set_last_target_status (ecs->target, ecs->ptid, ecs->ws);

  /* Always clear state belonging to the previous time we stopped.  */
  stop_stack_dummy = STOP_NONE;

  if (ecs->ws.kind () == TARGET_WAITKIND_NO_RESUMED)
    {
      /* No unwaited-for children left.  IOW, all resumed children
	 have exited.  */
      stop_waiting (ecs);
      return;
    }

  if (ecs->ws.kind () != TARGET_WAITKIND_EXITED
      && ecs->ws.kind () != TARGET_WAITKIND_SIGNALLED)
    {
      ecs->event_thread = ecs->target->find_thread (ecs->ptid);
      /* If it's a new thread, add it to the thread database.  */
      if (ecs->event_thread == nullptr)
	ecs->event_thread = add_thread (ecs->target, ecs->ptid);

      /* Disable range stepping.  If the next step request could use a
	 range, this will be end up re-enabled then.  */
      ecs->event_thread->control.may_range_step = 0;
    }

  /* Dependent on valid ECS->EVENT_THREAD.  */
  adjust_pc_after_break (ecs->event_thread, ecs->ws);

  /* Dependent on the current PC value modified by adjust_pc_after_break.  */
  reinit_frame_cache ();

  breakpoint_retire_moribund ();

  /* First, distinguish signals caused by the debugger from signals
     that have to do with the program's own actions.  Note that
     breakpoint insns may cause SIGTRAP or SIGILL or SIGEMT, depending
     on the operating system version.  Here we detect when a SIGILL or
     SIGEMT is really a breakpoint and change it to SIGTRAP.  We do
     something similar for SIGSEGV, since a SIGSEGV will be generated
     when we're trying to execute a breakpoint instruction on a
     non-executable stack.  This happens for call dummy breakpoints
     for architectures like SPARC that place call dummies on the
     stack.  */
  if (ecs->ws.kind () == TARGET_WAITKIND_STOPPED
      && (ecs->ws.sig () == GDB_SIGNAL_ILL
	  || ecs->ws.sig () == GDB_SIGNAL_SEGV
	  || ecs->ws.sig () == GDB_SIGNAL_EMT))
    {
      struct regcache *regcache = get_thread_regcache (ecs->event_thread);

      if (breakpoint_inserted_here_p (ecs->event_thread->inf->aspace.get (),
				      regcache_read_pc (regcache)))
	{
	  infrun_debug_printf ("Treating signal as SIGTRAP");
	  ecs->ws.set_stopped (GDB_SIGNAL_TRAP);
	}
    }

  mark_non_executing_threads (ecs->target, ecs->ptid, ecs->ws);

  switch (ecs->ws.kind ())
    {
    case TARGET_WAITKIND_LOADED:
      {
	context_switch (ecs);
	/* Ignore gracefully during startup of the inferior, as it might
	   be the shell which has just loaded some objects, otherwise
	   add the symbols for the newly loaded objects.  Also ignore at
	   the beginning of an attach or remote session; we will query
	   the full list of libraries once the connection is
	   established.  */

	stop_kind stop_soon = get_inferior_stop_soon (ecs);
	if (stop_soon == NO_STOP_QUIETLY)
	  {
	    struct regcache *regcache;

	    regcache = get_thread_regcache (ecs->event_thread);

	    handle_solib_event ();

	    ecs->event_thread->set_stop_pc (regcache_read_pc (regcache));
	    address_space *aspace = ecs->event_thread->inf->aspace.get ();
	    ecs->event_thread->control.stop_bpstat
	      = bpstat_stop_status_nowatch (aspace,
					    ecs->event_thread->stop_pc (),
					    ecs->event_thread, ecs->ws);

	    if (handle_stop_requested (ecs))
	      return;

	    if (bpstat_causes_stop (ecs->event_thread->control.stop_bpstat))
	      {
		/* A catchpoint triggered.  */
		process_event_stop_test (ecs);
		return;
	      }

	    /* If requested, stop when the dynamic linker notifies
	       gdb of events.  This allows the user to get control
	       and place breakpoints in initializer routines for
	       dynamically loaded objects (among other things).  */
	    ecs->event_thread->set_stop_signal (GDB_SIGNAL_0);
	    if (stop_on_solib_events)
	      {
		/* Make sure we print "Stopped due to solib-event" in
		   normal_stop.  */
		stop_print_frame = true;

		stop_waiting (ecs);
		return;
	      }
	  }

	/* If we are skipping through a shell, or through shared library
	   loading that we aren't interested in, resume the program.  If
	   we're running the program normally, also resume.  */
	if (stop_soon == STOP_QUIETLY || stop_soon == NO_STOP_QUIETLY)
	  {
	    /* Loading of shared libraries might have changed breakpoint
	       addresses.  Make sure new breakpoints are inserted.  */
	    if (stop_soon == NO_STOP_QUIETLY)
	      insert_breakpoints ();
	    resume (GDB_SIGNAL_0);
	    prepare_to_wait (ecs);
	    return;
	  }

	/* But stop if we're attaching or setting up a remote
	   connection.  */
	if (stop_soon == STOP_QUIETLY_NO_SIGSTOP
	    || stop_soon == STOP_QUIETLY_REMOTE)
	  {
	    infrun_debug_printf ("quietly stopped");
	    stop_waiting (ecs);
	    return;
	  }

	internal_error (_("unhandled stop_soon: %d"), (int) stop_soon);
      }

    case TARGET_WAITKIND_SPURIOUS:
      if (handle_stop_requested (ecs))
	return;
      context_switch (ecs);
      resume (GDB_SIGNAL_0);
      prepare_to_wait (ecs);
      return;

    case TARGET_WAITKIND_THREAD_CREATED:
      if (handle_stop_requested (ecs))
	return;
      context_switch (ecs);
      if (!switch_back_to_stepped_thread (ecs))
	keep_going (ecs);
      return;

    case TARGET_WAITKIND_THREAD_EXITED:
      if (handle_thread_exited (ecs))
	return;
      stop_waiting (ecs);
      break;

    case TARGET_WAITKIND_EXITED:
    case TARGET_WAITKIND_SIGNALLED:
      {
	/* Depending on the system, ecs->ptid may point to a thread or
	   to a process.  On some targets, target_mourn_inferior may
	   need to have access to the just-exited thread.  That is the
	   case of GNU/Linux's "checkpoint" support, for example.
	   Call the switch_to_xxx routine as appropriate.  */
	thread_info *thr = ecs->target->find_thread (ecs->ptid);
	if (thr != nullptr)
	  switch_to_thread (thr);
	else
	  {
	    inferior *inf = find_inferior_ptid (ecs->target, ecs->ptid);
	    switch_to_inferior_no_thread (inf);
	  }
      }
      handle_vfork_child_exec_or_exit (0);
      target_terminal::ours ();	/* Must do this before mourn anyway.  */

      /* Clearing any previous state of convenience variables.  */
      clear_exit_convenience_vars ();

      if (ecs->ws.kind () == TARGET_WAITKIND_EXITED)
	{
	  /* Record the exit code in the convenience variable $_exitcode, so
	     that the user can inspect this again later.  */
	  set_internalvar_integer (lookup_internalvar ("_exitcode"),
				   (LONGEST) ecs->ws.exit_status ());

	  /* Also record this in the inferior itself.  */
	  current_inferior ()->has_exit_code = true;
	  current_inferior ()->exit_code = (LONGEST) ecs->ws.exit_status ();

	  /* Support the --return-child-result option.  */
	  return_child_result_value = ecs->ws.exit_status ();

	  interps_notify_exited (ecs->ws.exit_status ());
	}
      else
	{
	  struct gdbarch *gdbarch = current_inferior ()->arch ();

	  if (gdbarch_gdb_signal_to_target_p (gdbarch))
	    {
	      /* Set the value of the internal variable $_exitsignal,
		 which holds the signal uncaught by the inferior.  */
	      set_internalvar_integer (lookup_internalvar ("_exitsignal"),
				       gdbarch_gdb_signal_to_target (gdbarch,
							  ecs->ws.sig ()));
	    }
	  else
	    {
	      /* We don't have access to the target's method used for
		 converting between signal numbers (GDB's internal
		 representation <-> target's representation).
		 Therefore, we cannot do a good job at displaying this
		 information to the user.  It's better to just warn
		 her about it (if infrun debugging is enabled), and
		 give up.  */
	      infrun_debug_printf ("Cannot fill $_exitsignal with the correct "
				   "signal number.");
	    }

	  interps_notify_signal_exited (ecs->ws.sig ());
	}

      gdb_flush (gdb_stdout);
      target_mourn_inferior (inferior_ptid);
      stop_print_frame = false;
      stop_waiting (ecs);
      return;

    case TARGET_WAITKIND_FORKED:
    case TARGET_WAITKIND_VFORKED:
    case TARGET_WAITKIND_THREAD_CLONED:

      displaced_step_finish (ecs->event_thread, ecs->ws);

      /* Start a new step-over in another thread if there's one that
	 needs it.  */
      start_step_over ();

      context_switch (ecs);

      /* Immediately detach breakpoints from the child before there's
	 any chance of letting the user delete breakpoints from the
	 breakpoint lists.  If we don't do this early, it's easy to
	 leave left over traps in the child, vis: "break foo; catch
	 fork; c; <fork>; del; c; <child calls foo>".  We only follow
	 the fork on the last `continue', and by that time the
	 breakpoint at "foo" is long gone from the breakpoint table.
	 If we vforked, then we don't need to unpatch here, since both
	 parent and child are sharing the same memory pages; we'll
	 need to unpatch at follow/detach time instead to be certain
	 that new breakpoints added between catchpoint hit time and
	 vfork follow are detached.  */
      if (ecs->ws.kind () == TARGET_WAITKIND_FORKED)
	{
	  /* This won't actually modify the breakpoint list, but will
	     physically remove the breakpoints from the child.  */
	  detach_breakpoints (ecs->ws.child_ptid ());
	}

      delete_just_stopped_threads_single_step_breakpoints ();

      /* In case the event is caught by a catchpoint, remember that
	 the event is to be followed at the next resume of the thread,
	 and not immediately.  */
      ecs->event_thread->pending_follow = ecs->ws;

      ecs->event_thread->set_stop_pc
	(regcache_read_pc (get_thread_regcache (ecs->event_thread)));

      ecs->event_thread->control.stop_bpstat
	= bpstat_stop_status_nowatch (ecs->event_thread->inf->aspace.get (),
				      ecs->event_thread->stop_pc (),
				      ecs->event_thread, ecs->ws);

      if (handle_stop_requested (ecs))
	return;

      /* If no catchpoint triggered for this, then keep going.  Note
	 that we're interested in knowing the bpstat actually causes a
	 stop, not just if it may explain the signal.  Software
	 watchpoints, for example, always appear in the bpstat.  */
      if (!bpstat_causes_stop (ecs->event_thread->control.stop_bpstat))
	{
	  bool follow_child
	    = (ecs->ws.kind () != TARGET_WAITKIND_THREAD_CLONED
	       && follow_fork_mode_string == follow_fork_mode_child);

	  ecs->event_thread->set_stop_signal (GDB_SIGNAL_0);

	  process_stratum_target *targ
	    = ecs->event_thread->inf->process_target ();

	  bool should_resume;
	  if (ecs->ws.kind () != TARGET_WAITKIND_THREAD_CLONED)
	    should_resume = follow_fork ();
	  else
	    {
	      should_resume = true;
	      inferior *inf = ecs->event_thread->inf;
	      inf->top_target ()->follow_clone (ecs->ws.child_ptid ());
	      ecs->event_thread->pending_follow.set_spurious ();
	    }

	  /* Note that one of these may be an invalid pointer,
	     depending on detach_fork.  */
	  thread_info *parent = ecs->event_thread;
	  thread_info *child = targ->find_thread (ecs->ws.child_ptid ());

	  /* At this point, the parent is marked running, and the
	     child is marked stopped.  */

	  /* If not resuming the parent, mark it stopped.  */
	  if (ecs->ws.kind () != TARGET_WAITKIND_THREAD_CLONED
	      && follow_child && !detach_fork && !non_stop && !sched_multi)
	    parent->set_running (false);

	  /* If resuming the child, mark it running.  */
	  if ((ecs->ws.kind () == TARGET_WAITKIND_THREAD_CLONED
	       && !schedlock_applies (ecs->event_thread))
	      || (ecs->ws.kind () != TARGET_WAITKIND_THREAD_CLONED
		  && (follow_child
		      || (!detach_fork && (non_stop || sched_multi)))))
	    child->set_running (true);

	  /* In non-stop mode, also resume the other branch.  */
	  if ((ecs->ws.kind () == TARGET_WAITKIND_THREAD_CLONED
	       && target_is_non_stop_p ()
	       && !schedlock_applies (ecs->event_thread))
	      || (ecs->ws.kind () != TARGET_WAITKIND_THREAD_CLONED
		  && (!detach_fork && (non_stop
				       || (sched_multi
					   && target_is_non_stop_p ())))))
	    {
	      if (follow_child)
		switch_to_thread (parent);
	      else
		switch_to_thread (child);

	      ecs->event_thread = inferior_thread ();
	      ecs->ptid = inferior_ptid;
	      keep_going (ecs);
	    }

	  if (follow_child)
	    switch_to_thread (child);
	  else
	    switch_to_thread (parent);

	  ecs->event_thread = inferior_thread ();
	  ecs->ptid = inferior_ptid;

	  if (should_resume)
	    {
	      /* Never call switch_back_to_stepped_thread if we are waiting for
		 vfork-done (waiting for an external vfork child to exec or
		 exit).  We will resume only the vforking thread for the purpose
		 of collecting the vfork-done event, and we will restart any
		 step once the critical shared address space window is done.  */
	      if ((!follow_child
		   && detach_fork
		   && parent->inf->thread_waiting_for_vfork_done != nullptr)
		  || !switch_back_to_stepped_thread (ecs))
		keep_going (ecs);
	    }
	  else
	    stop_waiting (ecs);
	  return;
	}
      process_event_stop_test (ecs);
      return;

    case TARGET_WAITKIND_VFORK_DONE:
      /* Done with the shared memory region.  Re-insert breakpoints in
	 the parent, and keep going.  */

      context_switch (ecs);

      handle_vfork_done (ecs->event_thread);
      gdb_assert (inferior_thread () == ecs->event_thread);

      if (handle_stop_requested (ecs))
	return;

      if (!switch_back_to_stepped_thread (ecs))
	{
	  gdb_assert (inferior_thread () == ecs->event_thread);
	  /* This also takes care of reinserting breakpoints in the
	     previously locked inferior.  */
	  keep_going (ecs);
	}
      return;

    case TARGET_WAITKIND_EXECD:

      /* Note we can't read registers yet (the stop_pc), because we
	 don't yet know the inferior's post-exec architecture.
	 'stop_pc' is explicitly read below instead.  */
      switch_to_thread_no_regs (ecs->event_thread);

      /* Do whatever is necessary to the parent branch of the vfork.  */
      handle_vfork_child_exec_or_exit (1);

      /* This causes the eventpoints and symbol table to be reset.
	 Must do this now, before trying to determine whether to
	 stop.  */
      follow_exec (inferior_ptid, ecs->ws.execd_pathname ());

      /* In follow_exec we may have deleted the original thread and
	 created a new one.  Make sure that the event thread is the
	 execd thread for that case (this is a nop otherwise).  */
      ecs->event_thread = inferior_thread ();

      ecs->event_thread->set_stop_pc
	(regcache_read_pc (get_thread_regcache (ecs->event_thread)));

      ecs->event_thread->control.stop_bpstat
	= bpstat_stop_status_nowatch (ecs->event_thread->inf->aspace.get (),
				      ecs->event_thread->stop_pc (),
				      ecs->event_thread, ecs->ws);

      if (handle_stop_requested (ecs))
	return;

      /* If no catchpoint triggered for this, then keep going.  */
      if (!bpstat_causes_stop (ecs->event_thread->control.stop_bpstat))
	{
	  ecs->event_thread->set_stop_signal (GDB_SIGNAL_0);
	  keep_going (ecs);
	  return;
	}
      process_event_stop_test (ecs);
      return;

      /* Be careful not to try to gather much state about a thread
	 that's in a syscall.  It's frequently a losing proposition.  */
    case TARGET_WAITKIND_SYSCALL_ENTRY:
      /* Getting the current syscall number.  */
      if (handle_syscall_event (ecs) == 0)
	process_event_stop_test (ecs);
      return;

      /* Before examining the threads further, step this thread to
	 get it entirely out of the syscall.  (We get notice of the
	 event when the thread is just on the verge of exiting a
	 syscall.  Stepping one instruction seems to get it back
	 into user code.)  */
    case TARGET_WAITKIND_SYSCALL_RETURN:
      if (handle_syscall_event (ecs) == 0)
	process_event_stop_test (ecs);
      return;

    case TARGET_WAITKIND_STOPPED:
      handle_signal_stop (ecs);
      return;

    case TARGET_WAITKIND_NO_HISTORY:
      /* Reverse execution: target ran out of history info.  */

      /* Switch to the stopped thread.  */
      context_switch (ecs);
      infrun_debug_printf ("stopped");

      delete_just_stopped_threads_single_step_breakpoints ();
      ecs->event_thread->set_stop_pc
	(regcache_read_pc (get_thread_regcache (inferior_thread ())));

      if (handle_stop_requested (ecs))
	return;

      interps_notify_no_history ();
      stop_waiting (ecs);
      return;
    }
}

/* Restart threads back to what they were trying to do back when we
   paused them (because of an in-line step-over or vfork, for example).
   The EVENT_THREAD thread is ignored (not restarted).

   If INF is non-nullptr, only resume threads from INF.  */

static void
restart_threads (struct thread_info *event_thread, inferior *inf)
{
  INFRUN_SCOPED_DEBUG_START_END ("event_thread=%s, inf=%d",
				 event_thread->ptid.to_string ().c_str (),
				 inf != nullptr ? inf->num : -1);

  gdb_assert (!step_over_info_valid_p ());

  /* In case the instruction just stepped spawned a new thread.  */
  update_thread_list ();

  for (thread_info *tp : all_non_exited_threads ())
    {
      if (inf != nullptr && tp->inf != inf)
	continue;

      if (tp->inf->detaching)
	{
	  infrun_debug_printf ("restart threads: [%s] inferior detaching",
			       tp->ptid.to_string ().c_str ());
	  continue;
	}

      switch_to_thread_no_regs (tp);

      if (tp == event_thread)
	{
	  infrun_debug_printf ("restart threads: [%s] is event thread",
			       tp->ptid.to_string ().c_str ());
	  continue;
	}

      if (!(tp->state == THREAD_RUNNING || tp->control.in_infcall))
	{
	  infrun_debug_printf ("restart threads: [%s] not meant to be running",
			       tp->ptid.to_string ().c_str ());
	  continue;
	}

      if (tp->resumed ())
	{
	  infrun_debug_printf ("restart threads: [%s] resumed",
			      tp->ptid.to_string ().c_str ());
	  gdb_assert (tp->executing () || tp->has_pending_waitstatus ());
	  continue;
	}

      if (thread_is_in_step_over_chain (tp))
	{
	  infrun_debug_printf ("restart threads: [%s] needs step-over",
			       tp->ptid.to_string ().c_str ());
	  gdb_assert (!tp->resumed ());
	  continue;
	}


      if (tp->has_pending_waitstatus ())
	{
	  infrun_debug_printf ("restart threads: [%s] has pending status",
			       tp->ptid.to_string ().c_str ());
	  tp->set_resumed (true);
	  continue;
	}

      gdb_assert (!tp->stop_requested);

      /* If some thread needs to start a step-over at this point, it
	 should still be in the step-over queue, and thus skipped
	 above.  */
      if (thread_still_needs_step_over (tp))
	{
	  internal_error ("thread [%s] needs a step-over, but not in "
			  "step-over queue\n",
			  tp->ptid.to_string ().c_str ());
	}

      if (currently_stepping (tp))
	{
	  infrun_debug_printf ("restart threads: [%s] was stepping",
			       tp->ptid.to_string ().c_str ());
	  keep_going_stepped_thread (tp);
	}
      else
	{
	  infrun_debug_printf ("restart threads: [%s] continuing",
			       tp->ptid.to_string ().c_str ());
	  execution_control_state ecs (tp);
	  switch_to_thread (tp);
	  keep_going_pass_signal (&ecs);
	}
    }
}

/* Callback for iterate_over_threads.  Find a resumed thread that has
   a pending waitstatus.  */

static int
resumed_thread_with_pending_status (struct thread_info *tp,
				    void *arg)
{
  return tp->resumed () && tp->has_pending_waitstatus ();
}

/* Called when we get an event that may finish an in-line or
   out-of-line (displaced stepping) step-over started previously.
   Return true if the event is processed and we should go back to the
   event loop; false if the caller should continue processing the
   event.  */

static int
finish_step_over (struct execution_control_state *ecs)
{
  displaced_step_finish (ecs->event_thread, ecs->ws);

  bool had_step_over_info = step_over_info_valid_p ();

  if (had_step_over_info)
    {
      /* If we're stepping over a breakpoint with all threads locked,
	 then only the thread that was stepped should be reporting
	 back an event.  */
      gdb_assert (ecs->event_thread->control.trap_expected);

      update_thread_events_after_step_over (ecs->event_thread, ecs->ws);

      clear_step_over_info ();
    }

  if (!target_is_non_stop_p ())
    return 0;

  /* Start a new step-over in another thread if there's one that
     needs it.  */
  start_step_over ();

  /* If we were stepping over a breakpoint before, and haven't started
     a new in-line step-over sequence, then restart all other threads
     (except the event thread).  We can't do this in all-stop, as then
     e.g., we wouldn't be able to issue any other remote packet until
     these other threads stop.  */
  if (had_step_over_info && !step_over_info_valid_p ())
    {
      struct thread_info *pending;

      /* If we only have threads with pending statuses, the restart
	 below won't restart any thread and so nothing re-inserts the
	 breakpoint we just stepped over.  But we need it inserted
	 when we later process the pending events, otherwise if
	 another thread has a pending event for this breakpoint too,
	 we'd discard its event (because the breakpoint that
	 originally caused the event was no longer inserted).  */
      context_switch (ecs);
      insert_breakpoints ();

      restart_threads (ecs->event_thread);

      /* If we have events pending, go through handle_inferior_event
	 again, picking up a pending event at random.  This avoids
	 thread starvation.  */

      /* But not if we just stepped over a watchpoint in order to let
	 the instruction execute so we can evaluate its expression.
	 The set of watchpoints that triggered is recorded in the
	 breakpoint objects themselves (see bp->watchpoint_triggered).
	 If we processed another event first, that other event could
	 clobber this info.  */
      if (ecs->event_thread->stepping_over_watchpoint)
	return 0;

      /* The code below is meant to avoid one thread hogging the event
	 loop by doing constant in-line step overs.  If the stepping
	 thread exited, there's no risk for this to happen, so we can
	 safely let our caller process the event immediately.  */
      if (ecs->ws.kind () == TARGET_WAITKIND_THREAD_EXITED)
       return 0;

      pending = iterate_over_threads (resumed_thread_with_pending_status,
				      nullptr);
      if (pending != nullptr)
	{
	  struct thread_info *tp = ecs->event_thread;
	  struct regcache *regcache;

	  infrun_debug_printf ("found resumed threads with "
			       "pending events, saving status");

	  gdb_assert (pending != tp);

	  /* Record the event thread's event for later.  */
	  save_waitstatus (tp, ecs->ws);
	  /* This was cleared early, by handle_inferior_event.  Set it
	     so this pending event is considered by
	     do_target_wait.  */
	  tp->set_resumed (true);

	  gdb_assert (!tp->executing ());

	  regcache = get_thread_regcache (tp);
	  tp->set_stop_pc (regcache_read_pc (regcache));

	  infrun_debug_printf ("saved stop_pc=%s for %s "
			       "(currently_stepping=%d)",
			       paddress (current_inferior ()->arch (),
					 tp->stop_pc ()),
			       tp->ptid.to_string ().c_str (),
			       currently_stepping (tp));

	  /* This in-line step-over finished; clear this so we won't
	     start a new one.  This is what handle_signal_stop would
	     do, if we returned false.  */
	  tp->stepping_over_breakpoint = 0;

	  /* Wake up the event loop again.  */
	  mark_async_event_handler (infrun_async_inferior_event_token);

	  prepare_to_wait (ecs);
	  return 1;
	}
    }

  return 0;
}

/* See infrun.h.  */

void
notify_signal_received (gdb_signal sig)
{
  interps_notify_signal_received (sig);
  gdb::observers::signal_received.notify (sig);
}

/* See infrun.h.  */

void
notify_normal_stop (bpstat *bs, int print_frame)
{
  interps_notify_normal_stop (bs, print_frame);
  gdb::observers::normal_stop.notify (bs, print_frame);
}

/* See infrun.h.  */

void notify_user_selected_context_changed (user_selected_what selection)
{
  interps_notify_user_selected_context_changed (selection);
  gdb::observers::user_selected_context_changed.notify (selection);
}

/* Come here when the program has stopped with a signal.  */

static void
handle_signal_stop (struct execution_control_state *ecs)
{
  frame_info_ptr frame;
  struct gdbarch *gdbarch;
  int stopped_by_watchpoint;
  enum stop_kind stop_soon;
  int random_signal;

  gdb_assert (ecs->ws.kind () == TARGET_WAITKIND_STOPPED);

  ecs->event_thread->set_stop_signal (ecs->ws.sig ());

  /* Do we need to clean up the state of a thread that has
     completed a displaced single-step?  (Doing so usually affects
     the PC, so do it here, before we set stop_pc.)  */
  if (finish_step_over (ecs))
    return;

  /* If we either finished a single-step or hit a breakpoint, but
     the user wanted this thread to be stopped, pretend we got a
     SIG0 (generic unsignaled stop).  */
  if (ecs->event_thread->stop_requested
      && ecs->event_thread->stop_signal () == GDB_SIGNAL_TRAP)
    ecs->event_thread->set_stop_signal (GDB_SIGNAL_0);

  ecs->event_thread->set_stop_pc
    (regcache_read_pc (get_thread_regcache (ecs->event_thread)));

  context_switch (ecs);

  if (deprecated_context_hook)
    deprecated_context_hook (ecs->event_thread->global_num);

  if (debug_infrun)
    {
      struct regcache *regcache = get_thread_regcache (ecs->event_thread);
      struct gdbarch *reg_gdbarch = regcache->arch ();

      infrun_debug_printf
	("stop_pc=%s", paddress (reg_gdbarch, ecs->event_thread->stop_pc ()));
      if (target_stopped_by_watchpoint ())
	{
	  CORE_ADDR addr;

	  infrun_debug_printf ("stopped by watchpoint");

	  if (target_stopped_data_address (current_inferior ()->top_target (),
					   &addr))
	    infrun_debug_printf ("stopped data address=%s",
				 paddress (reg_gdbarch, addr));
	  else
	    infrun_debug_printf ("(no data address available)");
	}
    }

  /* This is originated from start_remote(), start_inferior() and
     shared libraries hook functions.  */
  stop_soon = get_inferior_stop_soon (ecs);
  if (stop_soon == STOP_QUIETLY || stop_soon == STOP_QUIETLY_REMOTE)
    {
      infrun_debug_printf ("quietly stopped");
      stop_print_frame = true;
      stop_waiting (ecs);
      return;
    }

  /* This originates from attach_command().  We need to overwrite
     the stop_signal here, because some kernels don't ignore a
     SIGSTOP in a subsequent ptrace(PTRACE_CONT,SIGSTOP) call.
     See more comments in inferior.h.  On the other hand, if we
     get a non-SIGSTOP, report it to the user - assume the backend
     will handle the SIGSTOP if it should show up later.

     Also consider that the attach is complete when we see a
     SIGTRAP.  Some systems (e.g. Windows), and stubs supporting
     target extended-remote report it instead of a SIGSTOP
     (e.g. gdbserver).  We already rely on SIGTRAP being our
     signal, so this is no exception.

     Also consider that the attach is complete when we see a
     GDB_SIGNAL_0.  In non-stop mode, GDB will explicitly tell
     the target to stop all threads of the inferior, in case the
     low level attach operation doesn't stop them implicitly.  If
     they weren't stopped implicitly, then the stub will report a
     GDB_SIGNAL_0, meaning: stopped for no particular reason
     other than GDB's request.  */
  if (stop_soon == STOP_QUIETLY_NO_SIGSTOP
      && (ecs->event_thread->stop_signal () == GDB_SIGNAL_STOP
	  || ecs->event_thread->stop_signal () == GDB_SIGNAL_TRAP
	  || ecs->event_thread->stop_signal () == GDB_SIGNAL_0))
    {
      stop_print_frame = true;
      stop_waiting (ecs);
      ecs->event_thread->set_stop_signal (GDB_SIGNAL_0);
      return;
    }

  /* At this point, get hold of the now-current thread's frame.  */
  frame = get_current_frame ();
  gdbarch = get_frame_arch (frame);

  /* Pull the single step breakpoints out of the target.  */
  if (ecs->event_thread->stop_signal () == GDB_SIGNAL_TRAP)
    {
      struct regcache *regcache;
      CORE_ADDR pc;

      regcache = get_thread_regcache (ecs->event_thread);
      const address_space *aspace = ecs->event_thread->inf->aspace.get ();

      pc = regcache_read_pc (regcache);

      /* However, before doing so, if this single-step breakpoint was
	 actually for another thread, set this thread up for moving
	 past it.  */
      if (!thread_has_single_step_breakpoint_here (ecs->event_thread,
						   aspace, pc))
	{
	  if (single_step_breakpoint_inserted_here_p (aspace, pc))
	    {
	      infrun_debug_printf ("[%s] hit another thread's single-step "
				   "breakpoint",
				   ecs->ptid.to_string ().c_str ());
	      ecs->hit_singlestep_breakpoint = 1;
	    }
	}
      else
	{
	  infrun_debug_printf ("[%s] hit its single-step breakpoint",
			       ecs->ptid.to_string ().c_str ());
	}
    }
  delete_just_stopped_threads_single_step_breakpoints ();

  if (ecs->event_thread->stop_signal () == GDB_SIGNAL_TRAP
      && ecs->event_thread->control.trap_expected
      && ecs->event_thread->stepping_over_watchpoint)
    stopped_by_watchpoint = 0;
  else
    stopped_by_watchpoint = watchpoints_triggered (ecs->ws);

  /* If necessary, step over this watchpoint.  We'll be back to display
     it in a moment.  */
  if (stopped_by_watchpoint
      && (target_have_steppable_watchpoint ()
	  || gdbarch_have_nonsteppable_watchpoint (gdbarch)))
    {
      /* At this point, we are stopped at an instruction which has
	 attempted to write to a piece of memory under control of
	 a watchpoint.  The instruction hasn't actually executed
	 yet.  If we were to evaluate the watchpoint expression
	 now, we would get the old value, and therefore no change
	 would seem to have occurred.

	 In order to make watchpoints work `right', we really need
	 to complete the memory write, and then evaluate the
	 watchpoint expression.  We do this by single-stepping the
	 target.

	 It may not be necessary to disable the watchpoint to step over
	 it.  For example, the PA can (with some kernel cooperation)
	 single step over a watchpoint without disabling the watchpoint.

	 It is far more common to need to disable a watchpoint to step
	 the inferior over it.  If we have non-steppable watchpoints,
	 we must disable the current watchpoint; it's simplest to
	 disable all watchpoints.

	 Any breakpoint at PC must also be stepped over -- if there's
	 one, it will have already triggered before the watchpoint
	 triggered, and we either already reported it to the user, or
	 it didn't cause a stop and we called keep_going.  In either
	 case, if there was a breakpoint at PC, we must be trying to
	 step past it.  */
      ecs->event_thread->stepping_over_watchpoint = 1;
      keep_going (ecs);
      return;
    }

  ecs->event_thread->stepping_over_breakpoint = 0;
  ecs->event_thread->stepping_over_watchpoint = 0;
  bpstat_clear (&ecs->event_thread->control.stop_bpstat);
  ecs->event_thread->control.stop_step = 0;
  stop_print_frame = true;
  stopped_by_random_signal = 0;
  bpstat *stop_chain = nullptr;

  /* Hide inlined functions starting here, unless we just performed stepi or
     nexti.  After stepi and nexti, always show the innermost frame (not any
     inline function call sites).  */
  if (ecs->event_thread->control.step_range_end != 1)
    {
      const address_space *aspace = ecs->event_thread->inf->aspace.get ();

      /* skip_inline_frames is expensive, so we avoid it if we can
	 determine that the address is one where functions cannot have
	 been inlined.  This improves performance with inferiors that
	 load a lot of shared libraries, because the solib event
	 breakpoint is defined as the address of a function (i.e. not
	 inline).  Note that we have to check the previous PC as well
	 as the current one to catch cases when we have just
	 single-stepped off a breakpoint prior to reinstating it.
	 Note that we're assuming that the code we single-step to is
	 not inline, but that's not definitive: there's nothing
	 preventing the event breakpoint function from containing
	 inlined code, and the single-step ending up there.  If the
	 user had set a breakpoint on that inlined code, the missing
	 skip_inline_frames call would break things.  Fortunately
	 that's an extremely unlikely scenario.  */
      if (!pc_at_non_inline_function (aspace,
				      ecs->event_thread->stop_pc (),
				      ecs->ws)
	  && !(ecs->event_thread->stop_signal () == GDB_SIGNAL_TRAP
	       && ecs->event_thread->control.trap_expected
	       && pc_at_non_inline_function (aspace,
					     ecs->event_thread->prev_pc,
					     ecs->ws)))
	{
	  stop_chain = build_bpstat_chain (aspace,
					   ecs->event_thread->stop_pc (),
					   ecs->ws);
	  skip_inline_frames (ecs->event_thread, stop_chain);

	  /* Re-fetch current thread's frame in case that invalidated
	     the frame cache.  */
	  frame = get_current_frame ();
	  gdbarch = get_frame_arch (frame);
	}
    }

  if (ecs->event_thread->stop_signal () == GDB_SIGNAL_TRAP
      && ecs->event_thread->control.trap_expected
      && gdbarch_single_step_through_delay_p (gdbarch)
      && currently_stepping (ecs->event_thread))
    {
      /* We're trying to step off a breakpoint.  Turns out that we're
	 also on an instruction that needs to be stepped multiple
	 times before it's been fully executing.  E.g., architectures
	 with a delay slot.  It needs to be stepped twice, once for
	 the instruction and once for the delay slot.  */
      int step_through_delay
	= gdbarch_single_step_through_delay (gdbarch, frame);

      if (step_through_delay)
	infrun_debug_printf ("step through delay");

      if (ecs->event_thread->control.step_range_end == 0
	  && step_through_delay)
	{
	  /* The user issued a continue when stopped at a breakpoint.
	     Set up for another trap and get out of here.  */
	 ecs->event_thread->stepping_over_breakpoint = 1;
	 keep_going (ecs);
	 return;
	}
      else if (step_through_delay)
	{
	  /* The user issued a step when stopped at a breakpoint.
	     Maybe we should stop, maybe we should not - the delay
	     slot *might* correspond to a line of source.  In any
	     case, don't decide that here, just set 
	     ecs->stepping_over_breakpoint, making sure we 
	     single-step again before breakpoints are re-inserted.  */
	  ecs->event_thread->stepping_over_breakpoint = 1;
	}
    }

  /* See if there is a breakpoint/watchpoint/catchpoint/etc. that
     handles this event.  */
  ecs->event_thread->control.stop_bpstat
    = bpstat_stop_status (ecs->event_thread->inf->aspace.get (),
			  ecs->event_thread->stop_pc (),
			  ecs->event_thread, ecs->ws, stop_chain);

  /* Following in case break condition called a
     function.  */
  stop_print_frame = true;

  /* This is where we handle "moribund" watchpoints.  Unlike
     software breakpoints traps, hardware watchpoint traps are
     always distinguishable from random traps.  If no high-level
     watchpoint is associated with the reported stop data address
     anymore, then the bpstat does not explain the signal ---
     simply make sure to ignore it if `stopped_by_watchpoint' is
     set.  */

  if (ecs->event_thread->stop_signal () == GDB_SIGNAL_TRAP
      && !bpstat_explains_signal (ecs->event_thread->control.stop_bpstat,
				  GDB_SIGNAL_TRAP)
      && stopped_by_watchpoint)
    {
      infrun_debug_printf ("no user watchpoint explains watchpoint SIGTRAP, "
			   "ignoring");
    }

  /* NOTE: cagney/2003-03-29: These checks for a random signal
     at one stage in the past included checks for an inferior
     function call's call dummy's return breakpoint.  The original
     comment, that went with the test, read:

     ``End of a stack dummy.  Some systems (e.g. Sony news) give
     another signal besides SIGTRAP, so check here as well as
     above.''

     If someone ever tries to get call dummys on a
     non-executable stack to work (where the target would stop
     with something like a SIGSEGV), then those tests might need
     to be re-instated.  Given, however, that the tests were only
     enabled when momentary breakpoints were not being used, I
     suspect that it won't be the case.

     NOTE: kettenis/2004-02-05: Indeed such checks don't seem to
     be necessary for call dummies on a non-executable stack on
     SPARC.  */

  /* See if the breakpoints module can explain the signal.  */
  random_signal
    = !bpstat_explains_signal (ecs->event_thread->control.stop_bpstat,
			       ecs->event_thread->stop_signal ());

  /* Maybe this was a trap for a software breakpoint that has since
     been removed.  */
  if (random_signal && target_stopped_by_sw_breakpoint ())
    {
      if (gdbarch_program_breakpoint_here_p (gdbarch,
					     ecs->event_thread->stop_pc ()))
	{
	  struct regcache *regcache;
	  int decr_pc;

	  /* Re-adjust PC to what the program would see if GDB was not
	     debugging it.  */
	  regcache = get_thread_regcache (ecs->event_thread);
	  decr_pc = gdbarch_decr_pc_after_break (gdbarch);
	  if (decr_pc != 0)
	    {
	      std::optional<scoped_restore_tmpl<int>>
		restore_operation_disable;

	      if (record_full_is_used ())
		restore_operation_disable.emplace
		  (record_full_gdb_operation_disable_set ());

	      regcache_write_pc (regcache,
				 ecs->event_thread->stop_pc () + decr_pc);
	    }
	}
      else
	{
	  /* A delayed software breakpoint event.  Ignore the trap.  */
	  infrun_debug_printf ("delayed software breakpoint trap, ignoring");
	  random_signal = 0;
	}
    }

  /* Maybe this was a trap for a hardware breakpoint/watchpoint that
     has since been removed.  */
  if (random_signal && target_stopped_by_hw_breakpoint ())
    {
      /* A delayed hardware breakpoint event.  Ignore the trap.  */
      infrun_debug_printf ("delayed hardware breakpoint/watchpoint "
			   "trap, ignoring");
      random_signal = 0;
    }

  /* If not, perhaps stepping/nexting can.  */
  if (random_signal)
    random_signal = !(ecs->event_thread->stop_signal () == GDB_SIGNAL_TRAP
		      && currently_stepping (ecs->event_thread));

  /* Perhaps the thread hit a single-step breakpoint of _another_
     thread.  Single-step breakpoints are transparent to the
     breakpoints module.  */
  if (random_signal)
    random_signal = !ecs->hit_singlestep_breakpoint;

  /* No?  Perhaps we got a moribund watchpoint.  */
  if (random_signal)
    random_signal = !stopped_by_watchpoint;

  /* Always stop if the user explicitly requested this thread to
     remain stopped.  */
  if (ecs->event_thread->stop_requested)
    {
      random_signal = 1;
      infrun_debug_printf ("user-requested stop");
    }

  /* For the program's own signals, act according to
     the signal handling tables.  */

  if (random_signal)
    {
      /* Signal not for debugging purposes.  */
      enum gdb_signal stop_signal = ecs->event_thread->stop_signal ();

      infrun_debug_printf ("random signal (%s)",
			   gdb_signal_to_symbol_string (stop_signal));

      stopped_by_random_signal = 1;

      /* Always stop on signals if we're either just gaining control
	 of the program, or the user explicitly requested this thread
	 to remain stopped.  */
      if (stop_soon != NO_STOP_QUIETLY
	  || ecs->event_thread->stop_requested
	  || signal_stop_state (ecs->event_thread->stop_signal ()))
	{
	  stop_waiting (ecs);
	  return;
	}

      /* Notify observers the signal has "handle print" set.  Note we
	 returned early above if stopping; normal_stop handles the
	 printing in that case.  */
      if (signal_print[ecs->event_thread->stop_signal ()])
	{
	  /* The signal table tells us to print about this signal.  */
	  target_terminal::ours_for_output ();
	  notify_signal_received (ecs->event_thread->stop_signal ());
	  target_terminal::inferior ();
	}

      /* Clear the signal if it should not be passed.  */
      if (signal_program[ecs->event_thread->stop_signal ()] == 0)
	ecs->event_thread->set_stop_signal (GDB_SIGNAL_0);

      if (ecs->event_thread->prev_pc == ecs->event_thread->stop_pc ()
	  && ecs->event_thread->control.trap_expected
	  && ecs->event_thread->control.step_resume_breakpoint == nullptr)
	{
	  /* We were just starting a new sequence, attempting to
	     single-step off of a breakpoint and expecting a SIGTRAP.
	     Instead this signal arrives.  This signal will take us out
	     of the stepping range so GDB needs to remember to, when
	     the signal handler returns, resume stepping off that
	     breakpoint.  */
	  /* To simplify things, "continue" is forced to use the same
	     code paths as single-step - set a breakpoint at the
	     signal return address and then, once hit, step off that
	     breakpoint.  */
	  infrun_debug_printf ("signal arrived while stepping over breakpoint");

	  insert_hp_step_resume_breakpoint_at_frame (frame);
	  ecs->event_thread->step_after_step_resume_breakpoint = 1;
	  /* Reset trap_expected to ensure breakpoints are re-inserted.  */
	  ecs->event_thread->control.trap_expected = 0;

	  /* If we were nexting/stepping some other thread, switch to
	     it, so that we don't continue it, losing control.  */
	  if (!switch_back_to_stepped_thread (ecs))
	    keep_going (ecs);
	  return;
	}

      if (ecs->event_thread->stop_signal () != GDB_SIGNAL_0
	  && (pc_in_thread_step_range (ecs->event_thread->stop_pc (),
				       ecs->event_thread)
	      || ecs->event_thread->control.step_range_end == 1)
	  && (get_stack_frame_id (frame)
	      == ecs->event_thread->control.step_stack_frame_id)
	  && ecs->event_thread->control.step_resume_breakpoint == nullptr)
	{
	  /* The inferior is about to take a signal that will take it
	     out of the single step range.  Set a breakpoint at the
	     current PC (which is presumably where the signal handler
	     will eventually return) and then allow the inferior to
	     run free.

	     Note that this is only needed for a signal delivered
	     while in the single-step range.  Nested signals aren't a
	     problem as they eventually all return.  */
	  infrun_debug_printf ("signal may take us out of single-step range");

	  clear_step_over_info ();
	  insert_hp_step_resume_breakpoint_at_frame (frame);
	  ecs->event_thread->step_after_step_resume_breakpoint = 1;
	  /* Reset trap_expected to ensure breakpoints are re-inserted.  */
	  ecs->event_thread->control.trap_expected = 0;
	  keep_going (ecs);
	  return;
	}

      /* Note: step_resume_breakpoint may be non-NULL.  This occurs
	 when either there's a nested signal, or when there's a
	 pending signal enabled just as the signal handler returns
	 (leaving the inferior at the step-resume-breakpoint without
	 actually executing it).  Either way continue until the
	 breakpoint is really hit.  */

      if (!switch_back_to_stepped_thread (ecs))
	{
	  infrun_debug_printf ("random signal, keep going");

	  keep_going (ecs);
	}
      return;
    }

  process_event_stop_test (ecs);
}

/* Return the address for the beginning of the line.  */

CORE_ADDR
update_line_range_start (CORE_ADDR pc, struct execution_control_state *ecs)
{
  /* The line table may have multiple entries for the same source code line.
     Given the PC, check the line table and return the PC that corresponds
     to the line table entry for the source line that PC is in.  */
  CORE_ADDR start_line_pc = ecs->event_thread->control.step_range_start;
  std::optional<CORE_ADDR> real_range_start;

  /* Call find_line_range_start to get the smallest address in the
     linetable for multiple Line X entries in the line table.  */
  real_range_start = find_line_range_start (pc);

  if (real_range_start.has_value ())
    start_line_pc = *real_range_start;

  return start_line_pc;
}

/* Come here when we've got some debug event / signal we can explain
   (IOW, not a random signal), and test whether it should cause a
   stop, or whether we should resume the inferior (transparently).
   E.g., could be a breakpoint whose condition evaluates false; we
   could be still stepping within the line; etc.  */

static void
process_event_stop_test (struct execution_control_state *ecs)
{
  struct symtab_and_line stop_pc_sal;
  frame_info_ptr frame;
  struct gdbarch *gdbarch;
  CORE_ADDR jmp_buf_pc;
  struct bpstat_what what;

  /* Handle cases caused by hitting a breakpoint.  */

  frame = get_current_frame ();
  gdbarch = get_frame_arch (frame);

  what = bpstat_what (ecs->event_thread->control.stop_bpstat);

  if (what.call_dummy)
    {
      stop_stack_dummy = what.call_dummy;
    }

  /* A few breakpoint types have callbacks associated (e.g.,
     bp_jit_event).  Run them now.  */
  bpstat_run_callbacks (ecs->event_thread->control.stop_bpstat);

  /* If we hit an internal event that triggers symbol changes, the
     current frame will be invalidated within bpstat_what (e.g., if we
     hit an internal solib event).  Re-fetch it.  */
  frame = get_current_frame ();
  gdbarch = get_frame_arch (frame);

  /* Shorthand to make if statements smaller.  */
  struct frame_id original_frame_id
    = ecs->event_thread->control.step_frame_id;
  struct frame_id curr_frame_id = get_frame_id (get_current_frame ());

  switch (what.main_action)
    {
    case BPSTAT_WHAT_SET_LONGJMP_RESUME:
      /* If we hit the breakpoint at longjmp while stepping, we
	 install a momentary breakpoint at the target of the
	 jmp_buf.  */

      infrun_debug_printf ("BPSTAT_WHAT_SET_LONGJMP_RESUME");

      ecs->event_thread->stepping_over_breakpoint = 1;

      if (what.is_longjmp)
	{
	  struct value *arg_value;

	  /* If we set the longjmp breakpoint via a SystemTap probe,
	     then use it to extract the arguments.  The destination PC
	     is the third argument to the probe.  */
	  arg_value = probe_safe_evaluate_at_pc (frame, 2);
	  if (arg_value)
	    {
	      jmp_buf_pc = value_as_address (arg_value);
	      jmp_buf_pc = gdbarch_addr_bits_remove (gdbarch, jmp_buf_pc);
	    }
	  else if (!gdbarch_get_longjmp_target_p (gdbarch)
		   || !gdbarch_get_longjmp_target (gdbarch,
						   frame, &jmp_buf_pc))
	    {
	      infrun_debug_printf ("BPSTAT_WHAT_SET_LONGJMP_RESUME "
				   "(!gdbarch_get_longjmp_target)");
	      keep_going (ecs);
	      return;
	    }

	  /* Insert a breakpoint at resume address.  */
	  insert_longjmp_resume_breakpoint (gdbarch, jmp_buf_pc);
	}
      else
	check_exception_resume (ecs, frame);
      keep_going (ecs);
      return;

    case BPSTAT_WHAT_CLEAR_LONGJMP_RESUME:
      {
	frame_info_ptr init_frame;

	/* There are several cases to consider.

	   1. The initiating frame no longer exists.  In this case we
	   must stop, because the exception or longjmp has gone too
	   far.

	   2. The initiating frame exists, and is the same as the
	   current frame.  We stop, because the exception or longjmp
	   has been caught.

	   3. The initiating frame exists and is different from the
	   current frame.  This means the exception or longjmp has
	   been caught beneath the initiating frame, so keep going.

	   4. longjmp breakpoint has been placed just to protect
	   against stale dummy frames and user is not interested in
	   stopping around longjmps.  */

	infrun_debug_printf ("BPSTAT_WHAT_CLEAR_LONGJMP_RESUME");

	gdb_assert (ecs->event_thread->control.exception_resume_breakpoint
		    != nullptr);
	delete_exception_resume_breakpoint (ecs->event_thread);

	if (what.is_longjmp)
	  {
	    check_longjmp_breakpoint_for_call_dummy (ecs->event_thread);

	    if (!frame_id_p (ecs->event_thread->initiating_frame))
	      {
		/* Case 4.  */
		keep_going (ecs);
		return;
	      }
	  }

	init_frame = frame_find_by_id (ecs->event_thread->initiating_frame);

	if (init_frame)
	  {
	    if (curr_frame_id == ecs->event_thread->initiating_frame)
	      {
		/* Case 2.  Fall through.  */
	      }
	    else
	      {
		/* Case 3.  */
		keep_going (ecs);
		return;
	      }
	  }

	/* For Cases 1 and 2, remove the step-resume breakpoint, if it
	   exists.  */
	delete_step_resume_breakpoint (ecs->event_thread);

	end_stepping_range (ecs);
      }
      return;

    case BPSTAT_WHAT_SINGLE:
      infrun_debug_printf ("BPSTAT_WHAT_SINGLE");
      ecs->event_thread->stepping_over_breakpoint = 1;
      /* Still need to check other stuff, at least the case where we
	 are stepping and step out of the right range.  */
      break;

    case BPSTAT_WHAT_STEP_RESUME:
      infrun_debug_printf ("BPSTAT_WHAT_STEP_RESUME");

      delete_step_resume_breakpoint (ecs->event_thread);
      if (ecs->event_thread->control.proceed_to_finish
	  && execution_direction == EXEC_REVERSE)
	{
	  struct thread_info *tp = ecs->event_thread;

	  /* We are finishing a function in reverse, and just hit the
	     step-resume breakpoint at the start address of the
	     function, and we're almost there -- just need to back up
	     by one more single-step, which should take us back to the
	     function call.  */
	  tp->control.step_range_start = tp->control.step_range_end = 1;
	  keep_going (ecs);
	  return;
	}
      fill_in_stop_func (gdbarch, ecs);
      if (ecs->event_thread->stop_pc () == ecs->stop_func_start
	  && execution_direction == EXEC_REVERSE)
	{
	  /* We are stepping over a function call in reverse, and just
	     hit the step-resume breakpoint at the start address of
	     the function.  Go back to single-stepping, which should
	     take us back to the function call.  */
	  ecs->event_thread->stepping_over_breakpoint = 1;
	  keep_going (ecs);
	  return;
	}
      break;

    case BPSTAT_WHAT_STOP_NOISY:
      infrun_debug_printf ("BPSTAT_WHAT_STOP_NOISY");
      stop_print_frame = true;

      /* Assume the thread stopped for a breakpoint.  We'll still check
	 whether a/the breakpoint is there when the thread is next
	 resumed.  */
      ecs->event_thread->stepping_over_breakpoint = 1;

      stop_waiting (ecs);
      return;

    case BPSTAT_WHAT_STOP_SILENT:
      infrun_debug_printf ("BPSTAT_WHAT_STOP_SILENT");
      stop_print_frame = false;

      /* Assume the thread stopped for a breakpoint.  We'll still check
	 whether a/the breakpoint is there when the thread is next
	 resumed.  */
      ecs->event_thread->stepping_over_breakpoint = 1;
      stop_waiting (ecs);
      return;

    case BPSTAT_WHAT_HP_STEP_RESUME:
      infrun_debug_printf ("BPSTAT_WHAT_HP_STEP_RESUME");

      delete_step_resume_breakpoint (ecs->event_thread);
      if (ecs->event_thread->step_after_step_resume_breakpoint)
	{
	  /* Back when the step-resume breakpoint was inserted, we
	     were trying to single-step off a breakpoint.  Go back to
	     doing that.  */
	  ecs->event_thread->step_after_step_resume_breakpoint = 0;
	  ecs->event_thread->stepping_over_breakpoint = 1;
	  keep_going (ecs);
	  return;
	}
      break;

    case BPSTAT_WHAT_KEEP_CHECKING:
      break;
    }

  /* If we stepped a permanent breakpoint and we had a high priority
     step-resume breakpoint for the address we stepped, but we didn't
     hit it, then we must have stepped into the signal handler.  The
     step-resume was only necessary to catch the case of _not_
     stepping into the handler, so delete it, and fall through to
     checking whether the step finished.  */
  if (ecs->event_thread->stepped_breakpoint)
    {
      struct breakpoint *sr_bp
	= ecs->event_thread->control.step_resume_breakpoint;

      if (sr_bp != nullptr
	  && sr_bp->first_loc ().permanent
	  && sr_bp->type == bp_hp_step_resume
	  && sr_bp->first_loc ().address == ecs->event_thread->prev_pc)
	{
	  infrun_debug_printf ("stepped permanent breakpoint, stopped in handler");
	  delete_step_resume_breakpoint (ecs->event_thread);
	  ecs->event_thread->step_after_step_resume_breakpoint = 0;
	}
    }

  /* We come here if we hit a breakpoint but should not stop for it.
     Possibly we also were stepping and should stop for that.  So fall
     through and test for stepping.  But, if not stepping, do not
     stop.  */

  /* In all-stop mode, if we're currently stepping but have stopped in
     some other thread, we need to switch back to the stepped thread.  */
  if (switch_back_to_stepped_thread (ecs))
    return;

  if (ecs->event_thread->control.step_resume_breakpoint)
    {
      infrun_debug_printf ("step-resume breakpoint is inserted");

      /* Having a step-resume breakpoint overrides anything
	 else having to do with stepping commands until
	 that breakpoint is reached.  */
      keep_going (ecs);
      return;
    }

  if (ecs->event_thread->control.step_range_end == 0)
    {
      infrun_debug_printf ("no stepping, continue");
      /* Likewise if we aren't even stepping.  */
      keep_going (ecs);
      return;
    }

  /* Re-fetch current thread's frame in case the code above caused
     the frame cache to be re-initialized, making our FRAME variable
     a dangling pointer.  */
  frame = get_current_frame ();
  gdbarch = get_frame_arch (frame);
  fill_in_stop_func (gdbarch, ecs);

  /* If stepping through a line, keep going if still within it.

     Note that step_range_end is the address of the first instruction
     beyond the step range, and NOT the address of the last instruction
     within it!

     Note also that during reverse execution, we may be stepping
     through a function epilogue and therefore must detect when
     the current-frame changes in the middle of a line.  */

  if (pc_in_thread_step_range (ecs->event_thread->stop_pc (),
			       ecs->event_thread)
      && (execution_direction != EXEC_REVERSE
	  || curr_frame_id == original_frame_id))
    {
      infrun_debug_printf
	("stepping inside range [%s-%s]",
	 paddress (gdbarch, ecs->event_thread->control.step_range_start),
	 paddress (gdbarch, ecs->event_thread->control.step_range_end));

      /* Tentatively re-enable range stepping; `resume' disables it if
	 necessary (e.g., if we're stepping over a breakpoint or we
	 have software watchpoints).  */
      ecs->event_thread->control.may_range_step = 1;

      /* When stepping backward, stop at beginning of line range
	 (unless it's the function entry point, in which case
	 keep going back to the call point).  */
      CORE_ADDR stop_pc = ecs->event_thread->stop_pc ();
      if (stop_pc == ecs->event_thread->control.step_range_start
	  && stop_pc != ecs->stop_func_start
	  && execution_direction == EXEC_REVERSE)
	end_stepping_range (ecs);
      else
	keep_going (ecs);

      return;
    }

  /* We stepped out of the stepping range.  */

  /* If we are stepping at the source level and entered the runtime
     loader dynamic symbol resolution code...

     EXEC_FORWARD: we keep on single stepping until we exit the run
     time loader code and reach the callee's address.

     EXEC_REVERSE: we've already executed the callee (backward), and
     the runtime loader code is handled just like any other
     undebuggable function call.  Now we need only keep stepping
     backward through the trampoline code, and that's handled further
     down, so there is nothing for us to do here.  */

  if (execution_direction != EXEC_REVERSE
      && ecs->event_thread->control.step_over_calls == STEP_OVER_UNDEBUGGABLE
      && in_solib_dynsym_resolve_code (ecs->event_thread->stop_pc ())
      && (ecs->event_thread->control.step_start_function == nullptr
	  || !in_solib_dynsym_resolve_code (
	       ecs->event_thread->control.step_start_function->value_block ()
		->entry_pc ())))
    {
      CORE_ADDR pc_after_resolver =
	gdbarch_skip_solib_resolver (gdbarch, ecs->event_thread->stop_pc ());

      infrun_debug_printf ("stepped into dynsym resolve code");

      if (pc_after_resolver)
	{
	  /* Set up a step-resume breakpoint at the address
	     indicated by SKIP_SOLIB_RESOLVER.  */
	  symtab_and_line sr_sal;
	  sr_sal.pc = pc_after_resolver;
	  sr_sal.pspace = get_frame_program_space (frame);

	  insert_step_resume_breakpoint_at_sal (gdbarch,
						sr_sal, null_frame_id);
	}

      keep_going (ecs);
      return;
    }

  /* Step through an indirect branch thunk.  */
  if (ecs->event_thread->control.step_over_calls != STEP_OVER_NONE
      && gdbarch_in_indirect_branch_thunk (gdbarch,
					   ecs->event_thread->stop_pc ()))
    {
      infrun_debug_printf ("stepped into indirect branch thunk");
      keep_going (ecs);
      return;
    }

  if (ecs->event_thread->control.step_range_end != 1
      && (ecs->event_thread->control.step_over_calls == STEP_OVER_UNDEBUGGABLE
	  || ecs->event_thread->control.step_over_calls == STEP_OVER_ALL)
      && get_frame_type (frame) == SIGTRAMP_FRAME)
    {
      infrun_debug_printf ("stepped into signal trampoline");
      /* The inferior, while doing a "step" or "next", has ended up in
	 a signal trampoline (either by a signal being delivered or by
	 the signal handler returning).  Just single-step until the
	 inferior leaves the trampoline (either by calling the handler
	 or returning).  */
      keep_going (ecs);
      return;
    }

  /* If we're in the return path from a shared library trampoline,
     we want to proceed through the trampoline when stepping.  */
  /* macro/2012-04-25: This needs to come before the subroutine
     call check below as on some targets return trampolines look
     like subroutine calls (MIPS16 return thunks).  */
  if (gdbarch_in_solib_return_trampoline (gdbarch,
					  ecs->event_thread->stop_pc (),
					  ecs->stop_func_name)
      && ecs->event_thread->control.step_over_calls != STEP_OVER_NONE)
    {
      /* Determine where this trampoline returns.  */
      CORE_ADDR stop_pc = ecs->event_thread->stop_pc ();
      CORE_ADDR real_stop_pc
	= gdbarch_skip_trampoline_code (gdbarch, frame, stop_pc);

      infrun_debug_printf ("stepped into solib return tramp");

      /* Only proceed through if we know where it's going.  */
      if (real_stop_pc)
	{
	  /* And put the step-breakpoint there and go until there.  */
	  symtab_and_line sr_sal;
	  sr_sal.pc = real_stop_pc;
	  sr_sal.section = find_pc_overlay (sr_sal.pc);
	  sr_sal.pspace = get_frame_program_space (frame);

	  /* Do not specify what the fp should be when we stop since
	     on some machines the prologue is where the new fp value
	     is established.  */
	  insert_step_resume_breakpoint_at_sal (gdbarch,
						sr_sal, null_frame_id);

	  /* Restart without fiddling with the step ranges or
	     other state.  */
	  keep_going (ecs);
	  return;
	}
    }

  /* Check for subroutine calls.  The check for the current frame
     equalling the step ID is not necessary - the check of the
     previous frame's ID is sufficient - but it is a common case and
     cheaper than checking the previous frame's ID.

     NOTE: frame_id::operator== will never report two invalid frame IDs as
     being equal, so to get into this block, both the current and
     previous frame must have valid frame IDs.  */
  /* The outer_frame_id check is a heuristic to detect stepping
     through startup code.  If we step over an instruction which
     sets the stack pointer from an invalid value to a valid value,
     we may detect that as a subroutine call from the mythical
     "outermost" function.  This could be fixed by marking
     outermost frames as !stack_p,code_p,special_p.  Then the
     initial outermost frame, before sp was valid, would
     have code_addr == &_start.  See the comment in frame_id::operator==
     for more.  */

  /* We want "nexti" to step into, not over, signal handlers invoked
     by the kernel, therefore this subroutine check should not trigger
     for a signal handler invocation.  On most platforms, this is already
     not the case, as the kernel puts a signal trampoline frame onto the
     stack to handle proper return after the handler, and therefore at this
     point, the current frame is a grandchild of the step frame, not a
     child.  However, on some platforms, the kernel actually uses a
     trampoline to handle *invocation* of the handler.  In that case,
     when executing the first instruction of the trampoline, this check
     would erroneously detect the trampoline invocation as a subroutine
     call.  Fix this by checking for SIGTRAMP_FRAME.  */
  if ((get_stack_frame_id (frame)
       != ecs->event_thread->control.step_stack_frame_id)
      && get_frame_type (frame) != SIGTRAMP_FRAME
      && ((frame_unwind_caller_id (get_current_frame ())
	   == ecs->event_thread->control.step_stack_frame_id)
	  && ((ecs->event_thread->control.step_stack_frame_id
	       != outer_frame_id)
	      || (ecs->event_thread->control.step_start_function
		  != find_pc_function (ecs->event_thread->stop_pc ())))))
    {
      CORE_ADDR stop_pc = ecs->event_thread->stop_pc ();
      CORE_ADDR real_stop_pc;

      infrun_debug_printf ("stepped into subroutine");

      if (ecs->event_thread->control.step_over_calls == STEP_OVER_NONE)
	{
	  /* I presume that step_over_calls is only 0 when we're
	     supposed to be stepping at the assembly language level
	     ("stepi").  Just stop.  */
	  /* And this works the same backward as frontward.  MVS */
	  end_stepping_range (ecs);
	  return;
	}

      /* Reverse stepping through solib trampolines.  */

      if (execution_direction == EXEC_REVERSE
	  && ecs->event_thread->control.step_over_calls != STEP_OVER_NONE
	  && (gdbarch_skip_trampoline_code (gdbarch, frame, stop_pc)
	      || (ecs->stop_func_start == 0
		  && in_solib_dynsym_resolve_code (stop_pc))))
	{
	  /* Any solib trampoline code can be handled in reverse
	     by simply continuing to single-step.  We have already
	     executed the solib function (backwards), and a few 
	     steps will take us back through the trampoline to the
	     caller.  */
	  keep_going (ecs);
	  return;
	}

      if (ecs->event_thread->control.step_over_calls == STEP_OVER_ALL)
	{
	  /* We're doing a "next".

	     Normal (forward) execution: set a breakpoint at the
	     callee's return address (the address at which the caller
	     will resume).

	     Reverse (backward) execution.  set the step-resume
	     breakpoint at the start of the function that we just
	     stepped into (backwards), and continue to there.  When we
	     get there, we'll need to single-step back to the caller.  */

	  if (execution_direction == EXEC_REVERSE)
	    {
	      /* If we're already at the start of the function, we've either
		 just stepped backward into a single instruction function,
		 or stepped back out of a signal handler to the first instruction
		 of the function.  Just keep going, which will single-step back
		 to the caller.  */
	      if (ecs->stop_func_start != stop_pc && ecs->stop_func_start != 0)
		{
		  /* Normal function call return (static or dynamic).  */
		  symtab_and_line sr_sal;
		  sr_sal.pc = ecs->stop_func_start;
		  sr_sal.pspace = get_frame_program_space (frame);
		  insert_step_resume_breakpoint_at_sal (gdbarch,
							sr_sal, get_stack_frame_id (frame));
		}
	    }
	  else
	    insert_step_resume_breakpoint_at_caller (frame);

	  keep_going (ecs);
	  return;
	}

      /* If we are in a function call trampoline (a stub between the
	 calling routine and the real function), locate the real
	 function.  That's what tells us (a) whether we want to step
	 into it at all, and (b) what prologue we want to run to the
	 end of, if we do step into it.  */
      real_stop_pc = skip_language_trampoline (frame, stop_pc);
      if (real_stop_pc == 0)
	real_stop_pc = gdbarch_skip_trampoline_code (gdbarch, frame, stop_pc);
      if (real_stop_pc != 0)
	ecs->stop_func_start = real_stop_pc;

      if (real_stop_pc != 0 && in_solib_dynsym_resolve_code (real_stop_pc))
	{
	  symtab_and_line sr_sal;
	  sr_sal.pc = ecs->stop_func_start;
	  sr_sal.pspace = get_frame_program_space (frame);

	  insert_step_resume_breakpoint_at_sal (gdbarch,
						sr_sal, null_frame_id);
	  keep_going (ecs);
	  return;
	}

      /* If we have line number information for the function we are
	 thinking of stepping into and the function isn't on the skip
	 list, step into it.

	 If there are several symtabs at that PC (e.g. with include
	 files), just want to know whether *any* of them have line
	 numbers.  find_pc_line handles this.  */
      {
	struct symtab_and_line tmp_sal;

	tmp_sal = find_pc_line (ecs->stop_func_start, 0);
	if (tmp_sal.line != 0
	    && !function_name_is_marked_for_skip (ecs->stop_func_name,
						  tmp_sal)
	    && !inline_frame_is_marked_for_skip (true, ecs->event_thread))
	  {
	    if (execution_direction == EXEC_REVERSE)
	      handle_step_into_function_backward (gdbarch, ecs);
	    else
	      handle_step_into_function (gdbarch, ecs);
	    return;
	  }
      }

      /* If we have no line number and the step-stop-if-no-debug is
	 set, we stop the step so that the user has a chance to switch
	 in assembly mode.  */
      if (ecs->event_thread->control.step_over_calls == STEP_OVER_UNDEBUGGABLE
	  && step_stop_if_no_debug)
	{
	  end_stepping_range (ecs);
	  return;
	}

      if (execution_direction == EXEC_REVERSE)
	{
	  /* If we're already at the start of the function, we've either just
	     stepped backward into a single instruction function without line
	     number info, or stepped back out of a signal handler to the first
	     instruction of the function without line number info.  Just keep
	     going, which will single-step back to the caller.  */
	  if (ecs->stop_func_start != stop_pc)
	    {
	      /* Set a breakpoint at callee's start address.
		 From there we can step once and be back in the caller.  */
	      symtab_and_line sr_sal;
	      sr_sal.pc = ecs->stop_func_start;
	      sr_sal.pspace = get_frame_program_space (frame);
	      insert_step_resume_breakpoint_at_sal (gdbarch,
						    sr_sal, null_frame_id);
	    }
	}
      else
	/* Set a breakpoint at callee's return address (the address
	   at which the caller will resume).  */
	insert_step_resume_breakpoint_at_caller (frame);

      keep_going (ecs);
      return;
    }

  /* Reverse stepping through solib trampolines.  */

  if (execution_direction == EXEC_REVERSE
      && ecs->event_thread->control.step_over_calls != STEP_OVER_NONE)
    {
      CORE_ADDR stop_pc = ecs->event_thread->stop_pc ();

      if (gdbarch_skip_trampoline_code (gdbarch, frame, stop_pc)
	  || (ecs->stop_func_start == 0
	      && in_solib_dynsym_resolve_code (stop_pc)))
	{
	  /* Any solib trampoline code can be handled in reverse
	     by simply continuing to single-step.  We have already
	     executed the solib function (backwards), and a few 
	     steps will take us back through the trampoline to the
	     caller.  */
	  keep_going (ecs);
	  return;
	}
      else if (in_solib_dynsym_resolve_code (stop_pc))
	{
	  /* Stepped backward into the solib dynsym resolver.
	     Set a breakpoint at its start and continue, then
	     one more step will take us out.  */
	  symtab_and_line sr_sal;
	  sr_sal.pc = ecs->stop_func_start;
	  sr_sal.pspace = get_frame_program_space (frame);
	  insert_step_resume_breakpoint_at_sal (gdbarch, 
						sr_sal, null_frame_id);
	  keep_going (ecs);
	  return;
	}
    }

  /* This always returns the sal for the inner-most frame when we are in a
     stack of inlined frames, even if GDB actually believes that it is in a
     more outer frame.  This is checked for below by calls to
     inline_skipped_frames.  */
  stop_pc_sal = find_pc_line (ecs->event_thread->stop_pc (), 0);

  /* NOTE: tausq/2004-05-24: This if block used to be done before all
     the trampoline processing logic, however, there are some trampolines 
     that have no names, so we should do trampoline handling first.  */
  if (ecs->event_thread->control.step_over_calls == STEP_OVER_UNDEBUGGABLE
      && ecs->stop_func_name == nullptr
      && stop_pc_sal.line == 0)
    {
      infrun_debug_printf ("stepped into undebuggable function");

      /* The inferior just stepped into, or returned to, an
	 undebuggable function (where there is no debugging information
	 and no line number corresponding to the address where the
	 inferior stopped).  Since we want to skip this kind of code,
	 we keep going until the inferior returns from this
	 function - unless the user has asked us not to (via
	 set step-mode) or we no longer know how to get back
	 to the call site.  */
      if (step_stop_if_no_debug
	  || !frame_id_p (frame_unwind_caller_id (frame)))
	{
	  /* If we have no line number and the step-stop-if-no-debug
	     is set, we stop the step so that the user has a chance to
	     switch in assembly mode.  */
	  end_stepping_range (ecs);
	  return;
	}
      else
	{
	  /* Set a breakpoint at callee's return address (the address
	     at which the caller will resume).  */
	  insert_step_resume_breakpoint_at_caller (frame);
	  keep_going (ecs);
	  return;
	}
    }

  if (execution_direction == EXEC_REVERSE
      && ecs->event_thread->control.proceed_to_finish
      && ecs->event_thread->stop_pc () >= ecs->stop_func_alt_start
      && ecs->event_thread->stop_pc () < ecs->stop_func_start)
    {
      /* We are executing the reverse-finish command.
	 If the system supports multiple entry points and we are finishing a
	 function in reverse.   If we are between the entry points single-step
	 back to the alternate entry point.  If we are at the alternate entry
	 point -- just   need to back up by one more single-step, which
	 should take us back to the function call.  */
      ecs->event_thread->control.step_range_start
	= ecs->event_thread->control.step_range_end = 1;
      keep_going (ecs);
      return;

    }

  if (ecs->event_thread->control.step_range_end == 1)
    {
      /* It is stepi or nexti.  We always want to stop stepping after
	 one instruction.  */
      infrun_debug_printf ("stepi/nexti");
      end_stepping_range (ecs);
      return;
    }

  if (stop_pc_sal.line == 0)
    {
      /* We have no line number information.  That means to stop
	 stepping (does this always happen right after one instruction,
	 when we do "s" in a function with no line numbers,
	 or can this happen as a result of a return or longjmp?).  */
      infrun_debug_printf ("line number info");
      end_stepping_range (ecs);
      return;
    }

  /* Look for "calls" to inlined functions, part one.  If the inline
     frame machinery detected some skipped call sites, we have entered
     a new inline function.  */

  if ((curr_frame_id == original_frame_id)
      && inline_skipped_frames (ecs->event_thread))
    {
      infrun_debug_printf ("stepped into inlined function");

      symtab_and_line call_sal = find_frame_sal (get_current_frame ());

      if (ecs->event_thread->control.step_over_calls != STEP_OVER_ALL)
	{
	  /* For "step", we're going to stop.  But if the call site
	     for this inlined function is on the same source line as
	     we were previously stepping, go down into the function
	     first.  Otherwise stop at the call site.  */

	  if (call_sal.line == ecs->event_thread->current_line
	      && call_sal.symtab == ecs->event_thread->current_symtab)
	    {
	      step_into_inline_frame (ecs->event_thread);
	      if (inline_frame_is_marked_for_skip (false, ecs->event_thread))
		{
		  keep_going (ecs);
		  return;
		}
	    }

	  end_stepping_range (ecs);
	  return;
	}
      else
	{
	  /* For "next", we should stop at the call site if it is on a
	     different source line.  Otherwise continue through the
	     inlined function.  */
	  if (call_sal.line == ecs->event_thread->current_line
	      && call_sal.symtab == ecs->event_thread->current_symtab)
	    keep_going (ecs);
	  else
	    end_stepping_range (ecs);
	  return;
	}
    }

  /* Look for "calls" to inlined functions, part two.  If we are still
     in the same real function we were stepping through, but we have
     to go further up to find the exact frame ID, we are stepping
     through a more inlined call beyond its call site.  */

  if (get_frame_type (get_current_frame ()) == INLINE_FRAME
      && (curr_frame_id != original_frame_id)
      && stepped_in_from (get_current_frame (), original_frame_id))
    {
      infrun_debug_printf ("stepping through inlined function");

      if (ecs->event_thread->control.step_over_calls == STEP_OVER_ALL
	  || inline_frame_is_marked_for_skip (false, ecs->event_thread))
	keep_going (ecs);
      else
	end_stepping_range (ecs);
      return;
    }

  bool refresh_step_info = true;
  if ((ecs->event_thread->stop_pc () == stop_pc_sal.pc)
      && (ecs->event_thread->current_line != stop_pc_sal.line
	  || ecs->event_thread->current_symtab != stop_pc_sal.symtab))
    {
      /* We are at a different line.  */

      if (stop_pc_sal.is_stmt)
	{
	  if (execution_direction == EXEC_REVERSE)
	    {
	      /* We are stepping backwards make sure we have reached the
		 beginning of the line.  */
	      CORE_ADDR stop_pc = ecs->event_thread->stop_pc ();
	      CORE_ADDR start_line_pc
		= update_line_range_start (stop_pc, ecs);

	      if (stop_pc != start_line_pc)
		{
		  /* Have not reached the beginning of the source code line.
		     Set a step range.  Execution should stop in any function
		     calls we execute back into before reaching the beginning
		     of the line.  */
		  ecs->event_thread->control.step_range_start
		    = start_line_pc;
		  ecs->event_thread->control.step_range_end = stop_pc;
		  set_step_info (ecs->event_thread, frame, stop_pc_sal);
		  keep_going (ecs);
		  return;
		}
	    }

	  /* We are at the start of a statement.

	     So stop.  Note that we don't stop if we step into the middle of a
	     statement.  That is said to make things like for (;;) statements
	     work better.  */
	  infrun_debug_printf ("stepped to a different line");
	  end_stepping_range (ecs);
	  return;
	}
      else if (curr_frame_id == original_frame_id)
	{
	  /* We are not at the start of a statement, and we have not changed
	     frame.

	     We ignore this line table entry, and continue stepping forward,
	     looking for a better place to stop.  */
	  refresh_step_info = false;
	  infrun_debug_printf ("stepped to a different line, but "
			       "it's not the start of a statement");
	}
      else
	{
	  /* We are not the start of a statement, and we have changed frame.

	     We ignore this line table entry, and continue stepping forward,
	     looking for a better place to stop.  Keep refresh_step_info at
	     true to note that the frame has changed, but ignore the line
	     number to make sure we don't ignore a subsequent entry with the
	     same line number.  */
	  stop_pc_sal.line = 0;
	  infrun_debug_printf ("stepped to a different frame, but "
			       "it's not the start of a statement");
	}
    }
  else if (execution_direction == EXEC_REVERSE
	  && curr_frame_id != original_frame_id
	  && original_frame_id.code_addr_p && curr_frame_id.code_addr_p
	  && original_frame_id.code_addr == curr_frame_id.code_addr)
    {
      /* If we enter here, we're leaving a recursive function call.  In this
	 situation, we shouldn't refresh the step information, because if we
	 do, we'll lose the frame_id of when we started stepping, and this
	 will make GDB not know we need to print frame information.  */
      refresh_step_info = false;
      infrun_debug_printf ("reverse stepping, left a recursive call, don't "
			   "update step info so we remember we left a frame");
    }

  /* We aren't done stepping.

     Optimize by setting the stepping range to the line.
     (We might not be in the original line, but if we entered a
     new line in mid-statement, we continue stepping.  This makes
     things like for(;;) statements work better.)

     If we entered a SAL that indicates a non-statement line table entry,
     then we update the stepping range, but we don't update the step info,
     which includes things like the line number we are stepping away from.
     This means we will stop when we find a line table entry that is marked
     as is-statement, even if it matches the non-statement one we just
     stepped into.   */

  ecs->event_thread->control.step_range_start = stop_pc_sal.pc;
  ecs->event_thread->control.step_range_end = stop_pc_sal.end;
  ecs->event_thread->control.may_range_step = 1;
  infrun_debug_printf
    ("updated step range, start = %s, end = %s, may_range_step = %d",
     paddress (gdbarch, ecs->event_thread->control.step_range_start),
     paddress (gdbarch, ecs->event_thread->control.step_range_end),
     ecs->event_thread->control.may_range_step);
  if (refresh_step_info)
    set_step_info (ecs->event_thread, frame, stop_pc_sal);

  infrun_debug_printf ("keep going");

  if (execution_direction == EXEC_REVERSE)
    {
      CORE_ADDR stop_pc = ecs->event_thread->stop_pc ();

      /* Make sure the stop_pc is set to the beginning of the line.  */
      if (stop_pc != ecs->event_thread->control.step_range_start)
	ecs->event_thread->control.step_range_start
	  = update_line_range_start (stop_pc, ecs);
    }

  keep_going (ecs);
}

static bool restart_stepped_thread (process_stratum_target *resume_target,
				    ptid_t resume_ptid);

/* In all-stop mode, if we're currently stepping but have stopped in
   some other thread, we may need to switch back to the stepped
   thread.  Returns true we set the inferior running, false if we left
   it stopped (and the event needs further processing).  */

static bool
switch_back_to_stepped_thread (struct execution_control_state *ecs)
{
  if (!target_is_non_stop_p ())
    {
      /* If any thread is blocked on some internal breakpoint, and we
	 simply need to step over that breakpoint to get it going
	 again, do that first.  */

      /* However, if we see an event for the stepping thread, then we
	 know all other threads have been moved past their breakpoints
	 already.  Let the caller check whether the step is finished,
	 etc., before deciding to move it past a breakpoint.  */
      if (ecs->event_thread->control.step_range_end != 0)
	return false;

      /* Check if the current thread is blocked on an incomplete
	 step-over, interrupted by a random signal.  */
      if (ecs->event_thread->control.trap_expected
	  && ecs->event_thread->stop_signal () != GDB_SIGNAL_TRAP)
	{
	  infrun_debug_printf
	    ("need to finish step-over of [%s]",
	     ecs->event_thread->ptid.to_string ().c_str ());
	  keep_going (ecs);
	  return true;
	}

      /* Check if the current thread is blocked by a single-step
	 breakpoint of another thread.  */
      if (ecs->hit_singlestep_breakpoint)
       {
	 infrun_debug_printf ("need to step [%s] over single-step breakpoint",
			      ecs->ptid.to_string ().c_str ());
	 keep_going (ecs);
	 return true;
       }

      /* If this thread needs yet another step-over (e.g., stepping
	 through a delay slot), do it first before moving on to
	 another thread.  */
      if (thread_still_needs_step_over (ecs->event_thread))
	{
	  infrun_debug_printf
	    ("thread [%s] still needs step-over",
	     ecs->event_thread->ptid.to_string ().c_str ());
	  keep_going (ecs);
	  return true;
	}

      /* If scheduler locking applies even if not stepping, there's no
	 need to walk over threads.  Above we've checked whether the
	 current thread is stepping.  If some other thread not the
	 event thread is stepping, then it must be that scheduler
	 locking is not in effect.  */
      if (schedlock_applies (ecs->event_thread))
	return false;

      /* Otherwise, we no longer expect a trap in the current thread.
	 Clear the trap_expected flag before switching back -- this is
	 what keep_going does as well, if we call it.  */
      ecs->event_thread->control.trap_expected = 0;

      /* Likewise, clear the signal if it should not be passed.  */
      if (!signal_program[ecs->event_thread->stop_signal ()])
	ecs->event_thread->set_stop_signal (GDB_SIGNAL_0);

      if (restart_stepped_thread (ecs->target, ecs->ptid))
	{
	  prepare_to_wait (ecs);
	  return true;
	}

      switch_to_thread (ecs->event_thread);
    }

  return false;
}

/* Look for the thread that was stepping, and resume it.
   RESUME_TARGET / RESUME_PTID indicate the set of threads the caller
   is resuming.  Return true if a thread was started, false
   otherwise.  */

static bool
restart_stepped_thread (process_stratum_target *resume_target,
			ptid_t resume_ptid)
{
  /* Do all pending step-overs before actually proceeding with
     step/next/etc.  */
  if (start_step_over ())
    return true;

  for (thread_info *tp : all_threads_safe ())
    {
      if (tp->state == THREAD_EXITED)
	continue;

      if (tp->has_pending_waitstatus ())
	continue;

      /* Ignore threads of processes the caller is not
	 resuming.  */
      if (!sched_multi
	  && (tp->inf->process_target () != resume_target
	      || tp->inf->pid != resume_ptid.pid ()))
	continue;

      if (tp->control.trap_expected)
	{
	  infrun_debug_printf ("switching back to stepped thread (step-over)");

	  if (keep_going_stepped_thread (tp))
	    return true;
	}
    }

  for (thread_info *tp : all_threads_safe ())
    {
      if (tp->state == THREAD_EXITED)
	continue;

      if (tp->has_pending_waitstatus ())
	continue;

      /* Ignore threads of processes the caller is not
	 resuming.  */
      if (!sched_multi
	  && (tp->inf->process_target () != resume_target
	      || tp->inf->pid != resume_ptid.pid ()))
	continue;

      /* Did we find the stepping thread?  */
      if (tp->control.step_range_end)
	{
	  infrun_debug_printf ("switching back to stepped thread (stepping)");

	  if (keep_going_stepped_thread (tp))
	    return true;
	}
    }

  return false;
}

/* See infrun.h.  */

void
restart_after_all_stop_detach (process_stratum_target *proc_target)
{
  /* Note we don't check target_is_non_stop_p() here, because the
     current inferior may no longer have a process_stratum target
     pushed, as we just detached.  */

  /* See if we have a THREAD_RUNNING thread that need to be
     re-resumed.  If we have any thread that is already executing,
     then we don't need to resume the target -- it is already been
     resumed.  With the remote target (in all-stop), it's even
     impossible to issue another resumption if the target is already
     resumed, until the target reports a stop.  */
  for (thread_info *thr : all_threads (proc_target))
    {
      if (thr->state != THREAD_RUNNING)
	continue;

      /* If we have any thread that is already executing, then we
	 don't need to resume the target -- it is already been
	 resumed.  */
      if (thr->executing ())
	return;

      /* If we have a pending event to process, skip resuming the
	 target and go straight to processing it.  */
      if (thr->resumed () && thr->has_pending_waitstatus ())
	return;
    }

  /* Alright, we need to re-resume the target.  If a thread was
     stepping, we need to restart it stepping.  */
  if (restart_stepped_thread (proc_target, minus_one_ptid))
    return;

  /* Otherwise, find the first THREAD_RUNNING thread and resume
     it.  */
  for (thread_info *thr : all_threads (proc_target))
    {
      if (thr->state != THREAD_RUNNING)
	continue;

      execution_control_state ecs (thr);
      switch_to_thread (thr);
      keep_going (&ecs);
      return;
    }
}

/* Set a previously stepped thread back to stepping.  Returns true on
   success, false if the resume is not possible (e.g., the thread
   vanished).  */

static bool
keep_going_stepped_thread (struct thread_info *tp)
{
  frame_info_ptr frame;

  /* If the stepping thread exited, then don't try to switch back and
     resume it, which could fail in several different ways depending
     on the target.  Instead, just keep going.

     We can find a stepping dead thread in the thread list in two
     cases:

     - The target supports thread exit events, and when the target
       tries to delete the thread from the thread list, inferior_ptid
       pointed at the exiting thread.  In such case, calling
       delete_thread does not really remove the thread from the list;
       instead, the thread is left listed, with 'exited' state.

     - The target's debug interface does not support thread exit
       events, and so we have no idea whatsoever if the previously
       stepping thread is still alive.  For that reason, we need to
       synchronously query the target now.  */

  if (tp->state == THREAD_EXITED || !target_thread_alive (tp->ptid))
    {
      infrun_debug_printf ("not resuming previously stepped thread, it has "
			   "vanished");

      delete_thread (tp);
      return false;
    }

  infrun_debug_printf ("resuming previously stepped thread");

  execution_control_state ecs (tp);
  switch_to_thread (tp);

  tp->set_stop_pc (regcache_read_pc (get_thread_regcache (tp)));
  frame = get_current_frame ();

  /* If the PC of the thread we were trying to single-step has
     changed, then that thread has trapped or been signaled, but the
     event has not been reported to GDB yet.  Re-poll the target
     looking for this particular thread's event (i.e. temporarily
     enable schedlock) by:

     - setting a break at the current PC
     - resuming that particular thread, only (by setting trap
     expected)

     This prevents us continuously moving the single-step breakpoint
     forward, one instruction at a time, overstepping.  */

  if (tp->stop_pc () != tp->prev_pc)
    {
      ptid_t resume_ptid;

      infrun_debug_printf ("expected thread advanced also (%s -> %s)",
			   paddress (current_inferior ()->arch (), tp->prev_pc),
			   paddress (current_inferior ()->arch (),
				     tp->stop_pc ()));

      /* Clear the info of the previous step-over, as it's no longer
	 valid (if the thread was trying to step over a breakpoint, it
	 has already succeeded).  It's what keep_going would do too,
	 if we called it.  Do this before trying to insert the sss
	 breakpoint, otherwise if we were previously trying to step
	 over this exact address in another thread, the breakpoint is
	 skipped.  */
      clear_step_over_info ();
      tp->control.trap_expected = 0;

      insert_single_step_breakpoint (get_frame_arch (frame),
				     get_frame_address_space (frame),
				     tp->stop_pc ());

      tp->set_resumed (true);
      resume_ptid = internal_resume_ptid (tp->control.stepping_command);
      do_target_resume (resume_ptid, false, GDB_SIGNAL_0);
    }
  else
    {
      infrun_debug_printf ("expected thread still hasn't advanced");

      keep_going_pass_signal (&ecs);
    }

  return true;
}

/* Is thread TP in the middle of (software or hardware)
   single-stepping?  (Note the result of this function must never be
   passed directly as target_resume's STEP parameter.)  */

static bool
currently_stepping (struct thread_info *tp)
{
  return ((tp->control.step_range_end
	   && tp->control.step_resume_breakpoint == nullptr)
	  || tp->control.trap_expected
	  || tp->stepped_breakpoint
	  || bpstat_should_step ());
}

/* Inferior has stepped into a subroutine call with source code that
   we should not step over.  Do step to the first line of code in
   it.  */

static void
handle_step_into_function (struct gdbarch *gdbarch,
			   struct execution_control_state *ecs)
{
  fill_in_stop_func (gdbarch, ecs);

  compunit_symtab *cust
    = find_pc_compunit_symtab (ecs->event_thread->stop_pc ());
  if (cust != nullptr && cust->language () != language_asm)
    ecs->stop_func_start
      = gdbarch_skip_prologue_noexcept (gdbarch, ecs->stop_func_start);

  symtab_and_line stop_func_sal = find_pc_line (ecs->stop_func_start, 0);
  /* Use the step_resume_break to step until the end of the prologue,
     even if that involves jumps (as it seems to on the vax under
     4.2).  */
  /* If the prologue ends in the middle of a source line, continue to
     the end of that source line (if it is still within the function).
     Otherwise, just go to end of prologue.  */
  if (stop_func_sal.end
      && stop_func_sal.pc != ecs->stop_func_start
      && stop_func_sal.end < ecs->stop_func_end)
    ecs->stop_func_start = stop_func_sal.end;

  /* Architectures which require breakpoint adjustment might not be able
     to place a breakpoint at the computed address.  If so, the test
     ``ecs->stop_func_start == stop_pc'' will never succeed.  Adjust
     ecs->stop_func_start to an address at which a breakpoint may be
     legitimately placed.

     Note:  kevinb/2004-01-19:  On FR-V, if this adjustment is not
     made, GDB will enter an infinite loop when stepping through
     optimized code consisting of VLIW instructions which contain
     subinstructions corresponding to different source lines.  On
     FR-V, it's not permitted to place a breakpoint on any but the
     first subinstruction of a VLIW instruction.  When a breakpoint is
     set, GDB will adjust the breakpoint address to the beginning of
     the VLIW instruction.  Thus, we need to make the corresponding
     adjustment here when computing the stop address.  */

  if (gdbarch_adjust_breakpoint_address_p (gdbarch))
    {
      ecs->stop_func_start
	= gdbarch_adjust_breakpoint_address (gdbarch,
					     ecs->stop_func_start);
    }

  if (ecs->stop_func_start == ecs->event_thread->stop_pc ())
    {
      /* We are already there: stop now.  */
      end_stepping_range (ecs);
      return;
    }
  else
    {
      /* Put the step-breakpoint there and go until there.  */
      symtab_and_line sr_sal;
      sr_sal.pc = ecs->stop_func_start;
      sr_sal.section = find_pc_overlay (ecs->stop_func_start);
      sr_sal.pspace = get_frame_program_space (get_current_frame ());

      /* Do not specify what the fp should be when we stop since on
	 some machines the prologue is where the new fp value is
	 established.  */
      insert_step_resume_breakpoint_at_sal (gdbarch, sr_sal, null_frame_id);

      /* And make sure stepping stops right away then.  */
      ecs->event_thread->control.step_range_end
	= ecs->event_thread->control.step_range_start;
    }
  keep_going (ecs);
}

/* Inferior has stepped backward into a subroutine call with source
   code that we should not step over.  Do step to the beginning of the
   last line of code in it.  */

static void
handle_step_into_function_backward (struct gdbarch *gdbarch,
				    struct execution_control_state *ecs)
{
  struct compunit_symtab *cust;
  struct symtab_and_line stop_func_sal;

  fill_in_stop_func (gdbarch, ecs);

  cust = find_pc_compunit_symtab (ecs->event_thread->stop_pc ());
  if (cust != nullptr && cust->language () != language_asm)
    ecs->stop_func_start
      = gdbarch_skip_prologue_noexcept (gdbarch, ecs->stop_func_start);

  stop_func_sal = find_pc_line (ecs->event_thread->stop_pc (), 0);

  /* OK, we're just going to keep stepping here.  */
  if (stop_func_sal.pc == ecs->event_thread->stop_pc ())
    {
      /* We're there already.  Just stop stepping now.  */
      end_stepping_range (ecs);
    }
  else
    {
      /* Else just reset the step range and keep going.
	 No step-resume breakpoint, they don't work for
	 epilogues, which can have multiple entry paths.  */
      ecs->event_thread->control.step_range_start = stop_func_sal.pc;
      ecs->event_thread->control.step_range_end = stop_func_sal.end;
      keep_going (ecs);
    }
  return;
}

/* Insert a "step-resume breakpoint" at SR_SAL with frame ID SR_ID.
   This is used to both functions and to skip over code.  */

static void
insert_step_resume_breakpoint_at_sal_1 (struct gdbarch *gdbarch,
					struct symtab_and_line sr_sal,
					struct frame_id sr_id,
					enum bptype sr_type)
{
  /* There should never be more than one step-resume or longjmp-resume
     breakpoint per thread, so we should never be setting a new
     step_resume_breakpoint when one is already active.  */
  gdb_assert (inferior_thread ()->control.step_resume_breakpoint == nullptr);
  gdb_assert (sr_type == bp_step_resume || sr_type == bp_hp_step_resume);

  infrun_debug_printf ("inserting step-resume breakpoint at %s",
		       paddress (gdbarch, sr_sal.pc));

  inferior_thread ()->control.step_resume_breakpoint
    = set_momentary_breakpoint (gdbarch, sr_sal, sr_id, sr_type).release ();
}

void
insert_step_resume_breakpoint_at_sal (struct gdbarch *gdbarch,
				      struct symtab_and_line sr_sal,
				      struct frame_id sr_id)
{
  insert_step_resume_breakpoint_at_sal_1 (gdbarch,
					  sr_sal, sr_id,
					  bp_step_resume);
}

/* Insert a "high-priority step-resume breakpoint" at RETURN_FRAME.pc.
   This is used to skip a potential signal handler.

   This is called with the interrupted function's frame.  The signal
   handler, when it returns, will resume the interrupted function at
   RETURN_FRAME.pc.  */

static void
insert_hp_step_resume_breakpoint_at_frame (frame_info_ptr return_frame)
{
  gdb_assert (return_frame != nullptr);

  struct gdbarch *gdbarch = get_frame_arch (return_frame);

  symtab_and_line sr_sal;
  sr_sal.pc = gdbarch_addr_bits_remove (gdbarch, get_frame_pc (return_frame));
  sr_sal.section = find_pc_overlay (sr_sal.pc);
  sr_sal.pspace = get_frame_program_space (return_frame);

  insert_step_resume_breakpoint_at_sal_1 (gdbarch, sr_sal,
					  get_stack_frame_id (return_frame),
					  bp_hp_step_resume);
}

/* Insert a "step-resume breakpoint" at the previous frame's PC.  This
   is used to skip a function after stepping into it (for "next" or if
   the called function has no debugging information).

   The current function has almost always been reached by single
   stepping a call or return instruction.  NEXT_FRAME belongs to the
   current function, and the breakpoint will be set at the caller's
   resume address.

   This is a separate function rather than reusing
   insert_hp_step_resume_breakpoint_at_frame in order to avoid
   get_prev_frame, which may stop prematurely (see the implementation
   of frame_unwind_caller_id for an example).  */

static void
insert_step_resume_breakpoint_at_caller (frame_info_ptr next_frame)
{
  /* We shouldn't have gotten here if we don't know where the call site
     is.  */
  gdb_assert (frame_id_p (frame_unwind_caller_id (next_frame)));

  struct gdbarch *gdbarch = frame_unwind_caller_arch (next_frame);

  symtab_and_line sr_sal;
  sr_sal.pc = gdbarch_addr_bits_remove (gdbarch,
					frame_unwind_caller_pc (next_frame));
  sr_sal.section = find_pc_overlay (sr_sal.pc);
  sr_sal.pspace = frame_unwind_program_space (next_frame);

  insert_step_resume_breakpoint_at_sal (gdbarch, sr_sal,
					frame_unwind_caller_id (next_frame));
}

/* Insert a "longjmp-resume" breakpoint at PC.  This is used to set a
   new breakpoint at the target of a jmp_buf.  The handling of
   longjmp-resume uses the same mechanisms used for handling
   "step-resume" breakpoints.  */

static void
insert_longjmp_resume_breakpoint (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  /* There should never be more than one longjmp-resume breakpoint per
     thread, so we should never be setting a new
     longjmp_resume_breakpoint when one is already active.  */
  gdb_assert (inferior_thread ()->control.exception_resume_breakpoint == nullptr);

  infrun_debug_printf ("inserting longjmp-resume breakpoint at %s",
		       paddress (gdbarch, pc));

  inferior_thread ()->control.exception_resume_breakpoint =
    set_momentary_breakpoint_at_pc (gdbarch, pc, bp_longjmp_resume).release ();
}

/* Insert an exception resume breakpoint.  TP is the thread throwing
   the exception.  The block B is the block of the unwinder debug hook
   function.  FRAME is the frame corresponding to the call to this
   function.  SYM is the symbol of the function argument holding the
   target PC of the exception.  */

static void
insert_exception_resume_breakpoint (struct thread_info *tp,
				    const struct block *b,
				    frame_info_ptr frame,
				    struct symbol *sym)
{
  try
    {
      struct block_symbol vsym;
      struct value *value;
      CORE_ADDR handler;
      struct breakpoint *bp;

      vsym = lookup_symbol_search_name (sym->search_name (),
					b, VAR_DOMAIN);
      value = read_var_value (vsym.symbol, vsym.block, frame);
      /* If the value was optimized out, revert to the old behavior.  */
      if (! value->optimized_out ())
	{
	  handler = value_as_address (value);

	  infrun_debug_printf ("exception resume at %lx",
			       (unsigned long) handler);

	  /* set_momentary_breakpoint_at_pc creates a thread-specific
	     breakpoint for the current inferior thread.  */
	  gdb_assert (tp == inferior_thread ());
	  bp = set_momentary_breakpoint_at_pc (get_frame_arch (frame),
					       handler,
					       bp_exception_resume).release ();

	  /* set_momentary_breakpoint_at_pc invalidates FRAME.  */
	  frame = nullptr;

	  tp->control.exception_resume_breakpoint = bp;
	}
    }
  catch (const gdb_exception_error &e)
    {
      /* We want to ignore errors here.  */
    }
}

/* A helper for check_exception_resume that sets an
   exception-breakpoint based on a SystemTap probe.  */

static void
insert_exception_resume_from_probe (struct thread_info *tp,
				    const struct bound_probe *probe,
				    frame_info_ptr frame)
{
  struct value *arg_value;
  CORE_ADDR handler;
  struct breakpoint *bp;

  arg_value = probe_safe_evaluate_at_pc (frame, 1);
  if (!arg_value)
    return;

  handler = value_as_address (arg_value);

  infrun_debug_printf ("exception resume at %s",
		       paddress (probe->objfile->arch (), handler));

  /* set_momentary_breakpoint_at_pc creates a thread-specific breakpoint
     for the current inferior thread.  */
  gdb_assert (tp == inferior_thread ());
  bp = set_momentary_breakpoint_at_pc (get_frame_arch (frame),
				       handler, bp_exception_resume).release ();
  tp->control.exception_resume_breakpoint = bp;
}

/* This is called when an exception has been intercepted.  Check to
   see whether the exception's destination is of interest, and if so,
   set an exception resume breakpoint there.  */

static void
check_exception_resume (struct execution_control_state *ecs,
			frame_info_ptr frame)
{
  struct bound_probe probe;
  struct symbol *func;

  /* First see if this exception unwinding breakpoint was set via a
     SystemTap probe point.  If so, the probe has two arguments: the
     CFA and the HANDLER.  We ignore the CFA, extract the handler, and
     set a breakpoint there.  */
  probe = find_probe_by_pc (get_frame_pc (frame));
  if (probe.prob)
    {
      insert_exception_resume_from_probe (ecs->event_thread, &probe, frame);
      return;
    }

  func = get_frame_function (frame);
  if (!func)
    return;

  try
    {
      const struct block *b;
      int argno = 0;

      /* The exception breakpoint is a thread-specific breakpoint on
	 the unwinder's debug hook, declared as:
	 
	 void _Unwind_DebugHook (void *cfa, void *handler);
	 
	 The CFA argument indicates the frame to which control is
	 about to be transferred.  HANDLER is the destination PC.
	 
	 We ignore the CFA and set a temporary breakpoint at HANDLER.
	 This is not extremely efficient but it avoids issues in gdb
	 with computing the DWARF CFA, and it also works even in weird
	 cases such as throwing an exception from inside a signal
	 handler.  */

      b = func->value_block ();
      for (struct symbol *sym : block_iterator_range (b))
	{
	  if (!sym->is_argument ())
	    continue;

	  if (argno == 0)
	    ++argno;
	  else
	    {
	      insert_exception_resume_breakpoint (ecs->event_thread,
						  b, frame, sym);
	      break;
	    }
	}
    }
  catch (const gdb_exception_error &e)
    {
    }
}

static void
stop_waiting (struct execution_control_state *ecs)
{
  infrun_debug_printf ("stop_waiting");

  /* Let callers know we don't want to wait for the inferior anymore.  */
  ecs->wait_some_more = 0;
}

/* Like keep_going, but passes the signal to the inferior, even if the
   signal is set to nopass.  */

static void
keep_going_pass_signal (struct execution_control_state *ecs)
{
  gdb_assert (ecs->event_thread->ptid == inferior_ptid);
  gdb_assert (!ecs->event_thread->resumed ());

  /* Save the pc before execution, to compare with pc after stop.  */
  ecs->event_thread->prev_pc
    = regcache_read_pc_protected (get_thread_regcache (ecs->event_thread));

  if (ecs->event_thread->control.trap_expected)
    {
      struct thread_info *tp = ecs->event_thread;

      infrun_debug_printf ("%s has trap_expected set, "
			   "resuming to collect trap",
			   tp->ptid.to_string ().c_str ());

      /* We haven't yet gotten our trap, and either: intercepted a
	 non-signal event (e.g., a fork); or took a signal which we
	 are supposed to pass through to the inferior.  Simply
	 continue.  */
      resume (ecs->event_thread->stop_signal ());
    }
  else if (step_over_info_valid_p ())
    {
      /* Another thread is stepping over a breakpoint in-line.  If
	 this thread needs a step-over too, queue the request.  In
	 either case, this resume must be deferred for later.  */
      struct thread_info *tp = ecs->event_thread;

      if (ecs->hit_singlestep_breakpoint
	  || thread_still_needs_step_over (tp))
	{
	  infrun_debug_printf ("step-over already in progress: "
			       "step-over for %s deferred",
			       tp->ptid.to_string ().c_str ());
	  global_thread_step_over_chain_enqueue (tp);
	}
      else
	infrun_debug_printf ("step-over in progress: resume of %s deferred",
			     tp->ptid.to_string ().c_str ());
    }
  else
    {
      regcache *regcache = get_thread_regcache (ecs->event_thread);
      int remove_bp;
      int remove_wps;
      step_over_what step_what;

      /* Either the trap was not expected, but we are continuing
	 anyway (if we got a signal, the user asked it be passed to
	 the child)
	 -- or --
	 We got our expected trap, but decided we should resume from
	 it.

	 We're going to run this baby now!

	 Note that insert_breakpoints won't try to re-insert
	 already inserted breakpoints.  Therefore, we don't
	 care if breakpoints were already inserted, or not.  */

      /* If we need to step over a breakpoint, and we're not using
	 displaced stepping to do so, insert all breakpoints
	 (watchpoints, etc.) but the one we're stepping over, step one
	 instruction, and then re-insert the breakpoint when that step
	 is finished.  */

      step_what = thread_still_needs_step_over (ecs->event_thread);

      remove_bp = (ecs->hit_singlestep_breakpoint
		   || (step_what & STEP_OVER_BREAKPOINT));
      remove_wps = (step_what & STEP_OVER_WATCHPOINT);

      /* We can't use displaced stepping if we need to step past a
	 watchpoint.  The instruction copied to the scratch pad would
	 still trigger the watchpoint.  */
      if (remove_bp
	  && (remove_wps || !use_displaced_stepping (ecs->event_thread)))
	{
	  set_step_over_info (ecs->event_thread->inf->aspace.get (),
			      regcache_read_pc (regcache), remove_wps,
			      ecs->event_thread->global_num);
	}
      else if (remove_wps)
	set_step_over_info (nullptr, 0, remove_wps, -1);

      /* If we now need to do an in-line step-over, we need to stop
	 all other threads.  Note this must be done before
	 insert_breakpoints below, because that removes the breakpoint
	 we're about to step over, otherwise other threads could miss
	 it.  */
      if (step_over_info_valid_p () && target_is_non_stop_p ())
	stop_all_threads ("starting in-line step-over");

      /* Stop stepping if inserting breakpoints fails.  */
      try
	{
	  insert_breakpoints ();
	}
      catch (const gdb_exception_error &e)
	{
	  exception_print (gdb_stderr, e);
	  stop_waiting (ecs);
	  clear_step_over_info ();
	  return;
	}

      ecs->event_thread->control.trap_expected = (remove_bp || remove_wps);

      resume (ecs->event_thread->stop_signal ());
    }

  prepare_to_wait (ecs);
}

/* Called when we should continue running the inferior, because the
   current event doesn't cause a user visible stop.  This does the
   resuming part; waiting for the next event is done elsewhere.  */

static void
keep_going (struct execution_control_state *ecs)
{
  if (ecs->event_thread->control.trap_expected
      && ecs->event_thread->stop_signal () == GDB_SIGNAL_TRAP)
    ecs->event_thread->control.trap_expected = 0;

  if (!signal_program[ecs->event_thread->stop_signal ()])
    ecs->event_thread->set_stop_signal (GDB_SIGNAL_0);
  keep_going_pass_signal (ecs);
}

/* This function normally comes after a resume, before
   handle_inferior_event exits.  It takes care of any last bits of
   housekeeping, and sets the all-important wait_some_more flag.  */

static void
prepare_to_wait (struct execution_control_state *ecs)
{
  infrun_debug_printf ("prepare_to_wait");

  ecs->wait_some_more = 1;

  /* If the target can't async, emulate it by marking the infrun event
     handler such that as soon as we get back to the event-loop, we
     immediately end up in fetch_inferior_event again calling
     target_wait.  */
  if (!target_can_async_p ())
    mark_infrun_async_event_handler ();
}

/* We are done with the step range of a step/next/si/ni command.
   Called once for each n of a "step n" operation.  */

static void
end_stepping_range (struct execution_control_state *ecs)
{
  ecs->event_thread->control.stop_step = 1;
  stop_waiting (ecs);
}

/* Several print_*_reason functions to print why the inferior has stopped.
   We always print something when the inferior exits, or receives a signal.
   The rest of the cases are dealt with later on in normal_stop and
   print_it_typical.  Ideally there should be a call to one of these
   print_*_reason functions functions from handle_inferior_event each time
   stop_waiting is called.

   Note that we don't call these directly, instead we delegate that to
   the interpreters, through observers.  Interpreters then call these
   with whatever uiout is right.  */

void
print_signal_exited_reason (struct ui_out *uiout, enum gdb_signal siggnal)
{
  annotate_signalled ();
  if (uiout->is_mi_like_p ())
    uiout->field_string
      ("reason", async_reason_lookup (EXEC_ASYNC_EXITED_SIGNALLED));
  uiout->text ("\nProgram terminated with signal ");
  annotate_signal_name ();
  uiout->field_string ("signal-name",
		       gdb_signal_to_name (siggnal));
  annotate_signal_name_end ();
  uiout->text (", ");
  annotate_signal_string ();
  uiout->field_string ("signal-meaning",
		       gdb_signal_to_string (siggnal));
  annotate_signal_string_end ();
  uiout->text (".\n");
  uiout->text ("The program no longer exists.\n");
}

void
print_exited_reason (struct ui_out *uiout, int exitstatus)
{
  struct inferior *inf = current_inferior ();
  std::string pidstr = target_pid_to_str (ptid_t (inf->pid));

  annotate_exited (exitstatus);
  if (exitstatus)
    {
      if (uiout->is_mi_like_p ())
	uiout->field_string ("reason", async_reason_lookup (EXEC_ASYNC_EXITED));
      std::string exit_code_str
	= string_printf ("0%o", (unsigned int) exitstatus);
      uiout->message ("[Inferior %s (%s) exited with code %pF]\n",
		      plongest (inf->num), pidstr.c_str (),
		      string_field ("exit-code", exit_code_str.c_str ()));
    }
  else
    {
      if (uiout->is_mi_like_p ())
	uiout->field_string
	  ("reason", async_reason_lookup (EXEC_ASYNC_EXITED_NORMALLY));
      uiout->message ("[Inferior %s (%s) exited normally]\n",
		      plongest (inf->num), pidstr.c_str ());
    }
}

void
print_signal_received_reason (struct ui_out *uiout, enum gdb_signal siggnal)
{
  struct thread_info *thr = inferior_thread ();

  infrun_debug_printf ("signal = %s", gdb_signal_to_string (siggnal));

  annotate_signal ();

  if (uiout->is_mi_like_p ())
    ;
  else if (show_thread_that_caused_stop ())
    {
      uiout->text ("\nThread ");
      uiout->field_string ("thread-id", print_thread_id (thr));

      const char *name = thread_name (thr);
      if (name != nullptr)
	{
	  uiout->text (" \"");
	  uiout->field_string ("name", name);
	  uiout->text ("\"");
	}
    }
  else
    uiout->text ("\nProgram");

  if (siggnal == GDB_SIGNAL_0 && !uiout->is_mi_like_p ())
    uiout->text (" stopped");
  else
    {
      uiout->text (" received signal ");
      annotate_signal_name ();
      if (uiout->is_mi_like_p ())
	uiout->field_string
	  ("reason", async_reason_lookup (EXEC_ASYNC_SIGNAL_RECEIVED));
      uiout->field_string ("signal-name", gdb_signal_to_name (siggnal));
      annotate_signal_name_end ();
      uiout->text (", ");
      annotate_signal_string ();
      uiout->field_string ("signal-meaning", gdb_signal_to_string (siggnal));

      regcache *regcache = get_thread_regcache (thr);
      struct gdbarch *gdbarch = regcache->arch ();
      if (gdbarch_report_signal_info_p (gdbarch))
	gdbarch_report_signal_info (gdbarch, uiout, siggnal);

      annotate_signal_string_end ();
    }
  uiout->text (".\n");
}

void
print_no_history_reason (struct ui_out *uiout)
{
  if (uiout->is_mi_like_p ())
    uiout->field_string ("reason", async_reason_lookup (EXEC_ASYNC_NO_HISTORY));
  else
    uiout->text ("\nNo more reverse-execution history.\n");
}

/* Print current location without a level number, if we have changed
   functions or hit a breakpoint.  Print source line if we have one.
   bpstat_print contains the logic deciding in detail what to print,
   based on the event(s) that just occurred.  */

static void
print_stop_location (const target_waitstatus &ws)
{
  int bpstat_ret;
  enum print_what source_flag;
  int do_frame_printing = 1;
  struct thread_info *tp = inferior_thread ();

  bpstat_ret = bpstat_print (tp->control.stop_bpstat, ws.kind ());
  switch (bpstat_ret)
    {
    case PRINT_UNKNOWN:
      /* FIXME: cagney/2002-12-01: Given that a frame ID does (or
	 should) carry around the function and does (or should) use
	 that when doing a frame comparison.  */
      if (tp->control.stop_step
	  && (tp->control.step_frame_id
	      == get_frame_id (get_current_frame ()))
	  && (tp->control.step_start_function
	      == find_pc_function (tp->stop_pc ())))
	{
	  /* Finished step, just print source line.  */
	  source_flag = SRC_LINE;
	}
      else
	{
	  /* Print location and source line.  */
	  source_flag = SRC_AND_LOC;
	}
      break;
    case PRINT_SRC_AND_LOC:
      /* Print location and source line.  */
      source_flag = SRC_AND_LOC;
      break;
    case PRINT_SRC_ONLY:
      source_flag = SRC_LINE;
      break;
    case PRINT_NOTHING:
      /* Something bogus.  */
      source_flag = SRC_LINE;
      do_frame_printing = 0;
      break;
    default:
      internal_error (_("Unknown value."));
    }

  /* The behavior of this routine with respect to the source
     flag is:
     SRC_LINE: Print only source line
     LOCATION: Print only location
     SRC_AND_LOC: Print location and source line.  */
  if (do_frame_printing)
    print_stack_frame (get_selected_frame (nullptr), 0, source_flag, 1);
}

/* See infrun.h.  */

void
print_stop_event (struct ui_out *uiout, bool displays)
{
  struct target_waitstatus last;
  struct thread_info *tp;

  get_last_target_status (nullptr, nullptr, &last);

  {
    scoped_restore save_uiout = make_scoped_restore (&current_uiout, uiout);

    print_stop_location (last);

    /* Display the auto-display expressions.  */
    if (displays)
      do_displays ();
  }

  tp = inferior_thread ();
  if (tp->thread_fsm () != nullptr
      && tp->thread_fsm ()->finished_p ())
    {
      struct return_value_info *rv;

      rv = tp->thread_fsm ()->return_value ();
      if (rv != nullptr)
	print_return_value (uiout, rv);
    }
}

/* See infrun.h.  */

void
maybe_remove_breakpoints (void)
{
  if (!breakpoints_should_be_inserted_now () && target_has_execution ())
    {
      if (remove_breakpoints ())
	{
	  target_terminal::ours_for_output ();
	  gdb_printf (_("Cannot remove breakpoints because "
			"program is no longer writable.\nFurther "
			"execution is probably impossible.\n"));
	}
    }
}

/* The execution context that just caused a normal stop.  */

struct stop_context
{
  stop_context ();

  DISABLE_COPY_AND_ASSIGN (stop_context);

  bool changed () const;

  /* The stop ID.  */
  ULONGEST stop_id;

  /* The event PTID.  */

  ptid_t ptid;

  /* If stopp for a thread event, this is the thread that caused the
     stop.  */
  thread_info_ref thread;

  /* The inferior that caused the stop.  */
  int inf_num;
};

/* Initializes a new stop context.  If stopped for a thread event, this
   takes a strong reference to the thread.  */

stop_context::stop_context ()
{
  stop_id = get_stop_id ();
  ptid = inferior_ptid;
  inf_num = current_inferior ()->num;

  if (inferior_ptid != null_ptid)
    {
      /* Take a strong reference so that the thread can't be deleted
	 yet.  */
      thread = thread_info_ref::new_reference (inferior_thread ());
    }
}

/* Return true if the current context no longer matches the saved stop
   context.  */

bool
stop_context::changed () const
{
  if (ptid != inferior_ptid)
    return true;
  if (inf_num != current_inferior ()->num)
    return true;
  if (thread != nullptr && thread->state != THREAD_STOPPED)
    return true;
  if (get_stop_id () != stop_id)
    return true;
  return false;
}

/* See infrun.h.  */

bool
normal_stop ()
{
  struct target_waitstatus last;

  get_last_target_status (nullptr, nullptr, &last);

  new_stop_id ();

  /* If an exception is thrown from this point on, make sure to
     propagate GDB's knowledge of the executing state to the
     frontend/user running state.  A QUIT is an easy exception to see
     here, so do this before any filtered output.  */

  ptid_t finish_ptid = null_ptid;

  if (!non_stop)
    finish_ptid = minus_one_ptid;
  else if (last.kind () == TARGET_WAITKIND_SIGNALLED
	   || last.kind () == TARGET_WAITKIND_EXITED)
    {
      /* On some targets, we may still have live threads in the
	 inferior when we get a process exit event.  E.g., for
	 "checkpoint", when the current checkpoint/fork exits,
	 linux-fork.c automatically switches to another fork from
	 within target_mourn_inferior.  */
      if (inferior_ptid != null_ptid)
	finish_ptid = ptid_t (inferior_ptid.pid ());
    }
  else if (last.kind () != TARGET_WAITKIND_NO_RESUMED
	   && last.kind () != TARGET_WAITKIND_THREAD_EXITED)
    finish_ptid = inferior_ptid;

  std::optional<scoped_finish_thread_state> maybe_finish_thread_state;
  if (finish_ptid != null_ptid)
    {
      maybe_finish_thread_state.emplace
	(user_visible_resume_target (finish_ptid), finish_ptid);
    }

  /* As we're presenting a stop, and potentially removing breakpoints,
     update the thread list so we can tell whether there are threads
     running on the target.  With target remote, for example, we can
     only learn about new threads when we explicitly update the thread
     list.  Do this before notifying the interpreters about signal
     stops, end of stepping ranges, etc., so that the "new thread"
     output is emitted before e.g., "Program received signal FOO",
     instead of after.  */
  update_thread_list ();

  if (last.kind () == TARGET_WAITKIND_STOPPED && stopped_by_random_signal)
    notify_signal_received (inferior_thread ()->stop_signal ());

  /* As with the notification of thread events, we want to delay
     notifying the user that we've switched thread context until
     the inferior actually stops.

     There's no point in saying anything if the inferior has exited.
     Note that SIGNALLED here means "exited with a signal", not
     "received a signal".

     Also skip saying anything in non-stop mode.  In that mode, as we
     don't want GDB to switch threads behind the user's back, to avoid
     races where the user is typing a command to apply to thread x,
     but GDB switches to thread y before the user finishes entering
     the command, fetch_inferior_event installs a cleanup to restore
     the current thread back to the thread the user had selected right
     after this event is handled, so we're not really switching, only
     informing of a stop.  */
  if (!non_stop)
    {
      if ((last.kind () != TARGET_WAITKIND_SIGNALLED
	   && last.kind () != TARGET_WAITKIND_EXITED
	   && last.kind () != TARGET_WAITKIND_NO_RESUMED
	   && last.kind () != TARGET_WAITKIND_THREAD_EXITED)
	  && target_has_execution ()
	  && previous_thread != inferior_thread ())
	{
	  SWITCH_THRU_ALL_UIS ()
	    {
	      target_terminal::ours_for_output ();
	      gdb_printf (_("[Switching to %s]\n"),
			  target_pid_to_str (inferior_ptid).c_str ());
	      annotate_thread_changed ();
	    }
	}

      update_previous_thread ();
    }

  if (last.kind () == TARGET_WAITKIND_NO_RESUMED
      || last.kind () == TARGET_WAITKIND_THREAD_EXITED)
    {
      stop_print_frame = false;

      SWITCH_THRU_ALL_UIS ()
	if (current_ui->prompt_state == PROMPT_BLOCKED)
	  {
	    target_terminal::ours_for_output ();
	    if (last.kind () == TARGET_WAITKIND_NO_RESUMED)
	      gdb_printf (_("No unwaited-for children left.\n"));
	    else if (last.kind () == TARGET_WAITKIND_THREAD_EXITED)
	      gdb_printf (_("Command aborted, thread exited.\n"));
	    else
	      gdb_assert_not_reached ("unhandled");
	  }
    }

  /* Note: this depends on the update_thread_list call above.  */
  maybe_remove_breakpoints ();

  /* If an auto-display called a function and that got a signal,
     delete that auto-display to avoid an infinite recursion.  */

  if (stopped_by_random_signal)
    disable_current_display ();

  SWITCH_THRU_ALL_UIS ()
    {
      async_enable_stdin ();
    }

  /* Let the user/frontend see the threads as stopped.  */
  maybe_finish_thread_state.reset ();

  /* Select innermost stack frame - i.e., current frame is frame 0,
     and current location is based on that.  Handle the case where the
     dummy call is returning after being stopped.  E.g. the dummy call
     previously hit a breakpoint.  (If the dummy call returns
     normally, we won't reach here.)  Do this before the stop hook is
     run, so that it doesn't get to see the temporary dummy frame,
     which is not where we'll present the stop.  */
  if (has_stack_frames ())
    {
      if (stop_stack_dummy == STOP_STACK_DUMMY)
	{
	  /* Pop the empty frame that contains the stack dummy.  This
	     also restores inferior state prior to the call (struct
	     infcall_suspend_state).  */
	  frame_info_ptr frame = get_current_frame ();

	  gdb_assert (get_frame_type (frame) == DUMMY_FRAME);
	  frame_pop (frame);
	  /* frame_pop calls reinit_frame_cache as the last thing it
	     does which means there's now no selected frame.  */
	}

      select_frame (get_current_frame ());

      /* Set the current source location.  */
      set_current_sal_from_frame (get_current_frame ());
    }

  /* Look up the hook_stop and run it (CLI internally handles problem
     of stop_command's pre-hook not existing).  */
  stop_context saved_context;

  try
    {
      execute_cmd_pre_hook (stop_command);
    }
  catch (const gdb_exception_error &ex)
    {
      exception_fprintf (gdb_stderr, ex,
			 "Error while running hook_stop:\n");
    }

  /* If the stop hook resumes the target, then there's no point in
     trying to notify about the previous stop; its context is
     gone.  Likewise if the command switches thread or inferior --
     the observers would print a stop for the wrong
     thread/inferior.  */
  if (saved_context.changed ())
    return true;

  /* Notify observers about the stop.  This is where the interpreters
     print the stop event.  */
  notify_normal_stop ((inferior_ptid != null_ptid
		       ? inferior_thread ()->control.stop_bpstat
		       : nullptr),
		      stop_print_frame);
  annotate_stopped ();

  if (target_has_execution ())
    {
      if (last.kind () != TARGET_WAITKIND_SIGNALLED
	  && last.kind () != TARGET_WAITKIND_EXITED
	  && last.kind () != TARGET_WAITKIND_NO_RESUMED
	  && last.kind () != TARGET_WAITKIND_THREAD_EXITED)
	/* Delete the breakpoint we stopped at, if it wants to be deleted.
	   Delete any breakpoint that is to be deleted at the next stop.  */
	breakpoint_auto_delete (inferior_thread ()->control.stop_bpstat);
    }

  return false;
}

int
signal_stop_state (int signo)
{
  return signal_stop[signo];
}

int
signal_print_state (int signo)
{
  return signal_print[signo];
}

int
signal_pass_state (int signo)
{
  return signal_program[signo];
}

static void
signal_cache_update (int signo)
{
  if (signo == -1)
    {
      for (signo = 0; signo < (int) GDB_SIGNAL_LAST; signo++)
	signal_cache_update (signo);

      return;
    }

  signal_pass[signo] = (signal_stop[signo] == 0
			&& signal_print[signo] == 0
			&& signal_program[signo] == 1
			&& signal_catch[signo] == 0);
}

int
signal_stop_update (int signo, int state)
{
  int ret = signal_stop[signo];

  signal_stop[signo] = state;
  signal_cache_update (signo);
  return ret;
}

int
signal_print_update (int signo, int state)
{
  int ret = signal_print[signo];

  signal_print[signo] = state;
  signal_cache_update (signo);
  return ret;
}

int
signal_pass_update (int signo, int state)
{
  int ret = signal_program[signo];

  signal_program[signo] = state;
  signal_cache_update (signo);
  return ret;
}

/* Update the global 'signal_catch' from INFO and notify the
   target.  */

void
signal_catch_update (const unsigned int *info)
{
  int i;

  for (i = 0; i < GDB_SIGNAL_LAST; ++i)
    signal_catch[i] = info[i] > 0;
  signal_cache_update (-1);
  target_pass_signals (signal_pass);
}

static void
sig_print_header (void)
{
  gdb_printf (_("Signal        Stop\tPrint\tPass "
		"to program\tDescription\n"));
}

static void
sig_print_info (enum gdb_signal oursig)
{
  const char *name = gdb_signal_to_name (oursig);
  int name_padding = 13 - strlen (name);

  if (name_padding <= 0)
    name_padding = 0;

  gdb_printf ("%s", name);
  gdb_printf ("%*.*s ", name_padding, name_padding, "                 ");
  gdb_printf ("%s\t", signal_stop[oursig] ? "Yes" : "No");
  gdb_printf ("%s\t", signal_print[oursig] ? "Yes" : "No");
  gdb_printf ("%s\t\t", signal_program[oursig] ? "Yes" : "No");
  gdb_printf ("%s\n", gdb_signal_to_string (oursig));
}

/* Specify how various signals in the inferior should be handled.  */

static void
handle_command (const char *args, int from_tty)
{
  int digits, wordlen;
  int sigfirst, siglast;
  enum gdb_signal oursig;
  int allsigs;

  if (args == nullptr)
    {
      error_no_arg (_("signal to handle"));
    }

  /* Allocate and zero an array of flags for which signals to handle.  */

  const size_t nsigs = GDB_SIGNAL_LAST;
  unsigned char sigs[nsigs] {};

  /* Break the command line up into args.  */

  gdb_argv built_argv (args);

  /* Walk through the args, looking for signal oursigs, signal names, and
     actions.  Signal numbers and signal names may be interspersed with
     actions, with the actions being performed for all signals cumulatively
     specified.  Signal ranges can be specified as <LOW>-<HIGH>.  */

  for (char *arg : built_argv)
    {
      wordlen = strlen (arg);
      for (digits = 0; isdigit (arg[digits]); digits++)
	{;
	}
      allsigs = 0;
      sigfirst = siglast = -1;

      if (wordlen >= 1 && !strncmp (arg, "all", wordlen))
	{
	  /* Apply action to all signals except those used by the
	     debugger.  Silently skip those.  */
	  allsigs = 1;
	  sigfirst = 0;
	  siglast = nsigs - 1;
	}
      else if (wordlen >= 1 && !strncmp (arg, "stop", wordlen))
	{
	  SET_SIGS (nsigs, sigs, signal_stop);
	  SET_SIGS (nsigs, sigs, signal_print);
	}
      else if (wordlen >= 1 && !strncmp (arg, "ignore", wordlen))
	{
	  UNSET_SIGS (nsigs, sigs, signal_program);
	}
      else if (wordlen >= 2 && !strncmp (arg, "print", wordlen))
	{
	  SET_SIGS (nsigs, sigs, signal_print);
	}
      else if (wordlen >= 2 && !strncmp (arg, "pass", wordlen))
	{
	  SET_SIGS (nsigs, sigs, signal_program);
	}
      else if (wordlen >= 3 && !strncmp (arg, "nostop", wordlen))
	{
	  UNSET_SIGS (nsigs, sigs, signal_stop);
	}
      else if (wordlen >= 3 && !strncmp (arg, "noignore", wordlen))
	{
	  SET_SIGS (nsigs, sigs, signal_program);
	}
      else if (wordlen >= 4 && !strncmp (arg, "noprint", wordlen))
	{
	  UNSET_SIGS (nsigs, sigs, signal_print);
	  UNSET_SIGS (nsigs, sigs, signal_stop);
	}
      else if (wordlen >= 4 && !strncmp (arg, "nopass", wordlen))
	{
	  UNSET_SIGS (nsigs, sigs, signal_program);
	}
      else if (digits > 0)
	{
	  /* It is numeric.  The numeric signal refers to our own
	     internal signal numbering from target.h, not to host/target
	     signal  number.  This is a feature; users really should be
	     using symbolic names anyway, and the common ones like
	     SIGHUP, SIGINT, SIGALRM, etc. will work right anyway.  */

	  sigfirst = siglast = (int)
	    gdb_signal_from_command (atoi (arg));
	  if (arg[digits] == '-')
	    {
	      siglast = (int)
		gdb_signal_from_command (atoi (arg + digits + 1));
	    }
	  if (sigfirst > siglast)
	    {
	      /* Bet he didn't figure we'd think of this case...  */
	      std::swap (sigfirst, siglast);
	    }
	}
      else
	{
	  oursig = gdb_signal_from_name (arg);
	  if (oursig != GDB_SIGNAL_UNKNOWN)
	    {
	      sigfirst = siglast = (int) oursig;
	    }
	  else
	    {
	      /* Not a number and not a recognized flag word => complain.  */
	      error (_("Unrecognized or ambiguous flag word: \"%s\"."), arg);
	    }
	}

      /* If any signal numbers or symbol names were found, set flags for
	 which signals to apply actions to.  */

      for (int signum = sigfirst; signum >= 0 && signum <= siglast; signum++)
	{
	  switch ((enum gdb_signal) signum)
	    {
	    case GDB_SIGNAL_TRAP:
	    case GDB_SIGNAL_INT:
	      if (!allsigs && !sigs[signum])
		{
		  if (query (_("%s is used by the debugger.\n\
Are you sure you want to change it? "),
			     gdb_signal_to_name ((enum gdb_signal) signum)))
		    {
		      sigs[signum] = 1;
		    }
		  else
		    gdb_printf (_("Not confirmed, unchanged.\n"));
		}
	      break;
	    case GDB_SIGNAL_0:
	    case GDB_SIGNAL_DEFAULT:
	    case GDB_SIGNAL_UNKNOWN:
	      /* Make sure that "all" doesn't print these.  */
	      break;
	    default:
	      sigs[signum] = 1;
	      break;
	    }
	}
    }

  for (int signum = 0; signum < nsigs; signum++)
    if (sigs[signum])
      {
	signal_cache_update (-1);
	target_pass_signals (signal_pass);
	target_program_signals (signal_program);

	if (from_tty)
	  {
	    /* Show the results.  */
	    sig_print_header ();
	    for (; signum < nsigs; signum++)
	      if (sigs[signum])
		sig_print_info ((enum gdb_signal) signum);
	  }

	break;
      }
}

/* Complete the "handle" command.  */

static void
handle_completer (struct cmd_list_element *ignore,
		  completion_tracker &tracker,
		  const char *text, const char *word)
{
  static const char * const keywords[] =
    {
      "all",
      "stop",
      "ignore",
      "print",
      "pass",
      "nostop",
      "noignore",
      "noprint",
      "nopass",
      nullptr,
    };

  signal_completer (ignore, tracker, text, word);
  complete_on_enum (tracker, keywords, word, word);
}

enum gdb_signal
gdb_signal_from_command (int num)
{
  if (num >= 1 && num <= 15)
    return (enum gdb_signal) num;
  error (_("Only signals 1-15 are valid as numeric signals.\n\
Use \"info signals\" for a list of symbolic signals."));
}

/* Print current contents of the tables set by the handle command.
   It is possible we should just be printing signals actually used
   by the current target (but for things to work right when switching
   targets, all signals should be in the signal tables).  */

static void
info_signals_command (const char *signum_exp, int from_tty)
{
  enum gdb_signal oursig;

  sig_print_header ();

  if (signum_exp)
    {
      /* First see if this is a symbol name.  */
      oursig = gdb_signal_from_name (signum_exp);
      if (oursig == GDB_SIGNAL_UNKNOWN)
	{
	  /* No, try numeric.  */
	  oursig =
	    gdb_signal_from_command (parse_and_eval_long (signum_exp));
	}
      sig_print_info (oursig);
      return;
    }

  gdb_printf ("\n");
  /* These ugly casts brought to you by the native VAX compiler.  */
  for (oursig = GDB_SIGNAL_FIRST;
       (int) oursig < (int) GDB_SIGNAL_LAST;
       oursig = (enum gdb_signal) ((int) oursig + 1))
    {
      QUIT;

      if (oursig != GDB_SIGNAL_UNKNOWN
	  && oursig != GDB_SIGNAL_DEFAULT && oursig != GDB_SIGNAL_0)
	sig_print_info (oursig);
    }

  gdb_printf (_("\nUse the \"handle\" command "
		"to change these tables.\n"));
}

/* The $_siginfo convenience variable is a bit special.  We don't know
   for sure the type of the value until we actually have a chance to
   fetch the data.  The type can change depending on gdbarch, so it is
   also dependent on which thread you have selected.

     1. making $_siginfo be an internalvar that creates a new value on
     access.

     2. making the value of $_siginfo be an lval_computed value.  */

/* This function implements the lval_computed support for reading a
   $_siginfo value.  */

static void
siginfo_value_read (struct value *v)
{
  LONGEST transferred;

  /* If we can access registers, so can we access $_siginfo.  Likewise
     vice versa.  */
  validate_registers_access ();

  transferred =
    target_read (current_inferior ()->top_target (),
		 TARGET_OBJECT_SIGNAL_INFO,
		 nullptr,
		 v->contents_all_raw ().data (),
		 v->offset (),
		 v->type ()->length ());

  if (transferred != v->type ()->length ())
    error (_("Unable to read siginfo"));
}

/* This function implements the lval_computed support for writing a
   $_siginfo value.  */

static void
siginfo_value_write (struct value *v, struct value *fromval)
{
  LONGEST transferred;

  /* If we can access registers, so can we access $_siginfo.  Likewise
     vice versa.  */
  validate_registers_access ();

  transferred = target_write (current_inferior ()->top_target (),
			      TARGET_OBJECT_SIGNAL_INFO,
			      nullptr,
			      fromval->contents_all_raw ().data (),
			      v->offset (),
			      fromval->type ()->length ());

  if (transferred != fromval->type ()->length ())
    error (_("Unable to write siginfo"));
}

static const struct lval_funcs siginfo_value_funcs =
  {
    siginfo_value_read,
    siginfo_value_write
  };

/* Return a new value with the correct type for the siginfo object of
   the current thread using architecture GDBARCH.  Return a void value
   if there's no object available.  */

static struct value *
siginfo_make_value (struct gdbarch *gdbarch, struct internalvar *var,
		    void *ignore)
{
  if (target_has_stack ()
      && inferior_ptid != null_ptid
      && gdbarch_get_siginfo_type_p (gdbarch))
    {
      struct type *type = gdbarch_get_siginfo_type (gdbarch);

      return value::allocate_computed (type, &siginfo_value_funcs, nullptr);
    }

  return value::allocate (builtin_type (gdbarch)->builtin_void);
}


/* infcall_suspend_state contains state about the program itself like its
   registers and any signal it received when it last stopped.
   This state must be restored regardless of how the inferior function call
   ends (either successfully, or after it hits a breakpoint or signal)
   if the program is to properly continue where it left off.  */

class infcall_suspend_state
{
public:
  /* Capture state from GDBARCH, TP, and REGCACHE that must be restored
     once the inferior function call has finished.  */
  infcall_suspend_state (struct gdbarch *gdbarch,
			 const struct thread_info *tp,
			 struct regcache *regcache)
    : m_registers (new readonly_detached_regcache (*regcache))
  {
    tp->save_suspend_to (m_thread_suspend);

    gdb::unique_xmalloc_ptr<gdb_byte> siginfo_data;

    if (gdbarch_get_siginfo_type_p (gdbarch))
      {
	struct type *type = gdbarch_get_siginfo_type (gdbarch);
	size_t len = type->length ();

	siginfo_data.reset ((gdb_byte *) xmalloc (len));

	if (target_read (current_inferior ()->top_target (),
			 TARGET_OBJECT_SIGNAL_INFO, nullptr,
			 siginfo_data.get (), 0, len) != len)
	  {
	    /* Errors ignored.  */
	    siginfo_data.reset (nullptr);
	  }
      }

    if (siginfo_data)
      {
	m_siginfo_gdbarch = gdbarch;
	m_siginfo_data = std::move (siginfo_data);
      }
  }

  /* Return a pointer to the stored register state.  */

  readonly_detached_regcache *registers () const
  {
    return m_registers.get ();
  }

  /* Restores the stored state into GDBARCH, TP, and REGCACHE.  */

  void restore (struct gdbarch *gdbarch,
		struct thread_info *tp,
		struct regcache *regcache) const
  {
    tp->restore_suspend_from (m_thread_suspend);

    if (m_siginfo_gdbarch == gdbarch)
      {
	struct type *type = gdbarch_get_siginfo_type (gdbarch);

	/* Errors ignored.  */
	target_write (current_inferior ()->top_target (),
		      TARGET_OBJECT_SIGNAL_INFO, nullptr,
		      m_siginfo_data.get (), 0, type->length ());
      }

    /* The inferior can be gone if the user types "print exit(0)"
       (and perhaps other times).  */
    if (target_has_execution ())
      /* NB: The register write goes through to the target.  */
      regcache->restore (registers ());
  }

private:
  /* How the current thread stopped before the inferior function call was
     executed.  */
  struct thread_suspend_state m_thread_suspend;

  /* The registers before the inferior function call was executed.  */
  std::unique_ptr<readonly_detached_regcache> m_registers;

  /* Format of SIGINFO_DATA or NULL if it is not present.  */
  struct gdbarch *m_siginfo_gdbarch = nullptr;

  /* The inferior format depends on SIGINFO_GDBARCH and it has a length of
     gdbarch_get_siginfo_type ()->length ().  For different gdbarch the
     content would be invalid.  */
  gdb::unique_xmalloc_ptr<gdb_byte> m_siginfo_data;
};

infcall_suspend_state_up
save_infcall_suspend_state ()
{
  struct thread_info *tp = inferior_thread ();
  regcache *regcache = get_thread_regcache (tp);
  struct gdbarch *gdbarch = regcache->arch ();

  infcall_suspend_state_up inf_state
    (new struct infcall_suspend_state (gdbarch, tp, regcache));

  /* Having saved the current state, adjust the thread state, discarding
     any stop signal information.  The stop signal is not useful when
     starting an inferior function call, and run_inferior_call will not use
     the signal due to its `proceed' call with GDB_SIGNAL_0.  */
  tp->set_stop_signal (GDB_SIGNAL_0);

  return inf_state;
}

/* Restore inferior session state to INF_STATE.  */

void
restore_infcall_suspend_state (struct infcall_suspend_state *inf_state)
{
  struct thread_info *tp = inferior_thread ();
  regcache *regcache = get_thread_regcache (inferior_thread ());
  struct gdbarch *gdbarch = regcache->arch ();

  inf_state->restore (gdbarch, tp, regcache);
  discard_infcall_suspend_state (inf_state);
}

void
discard_infcall_suspend_state (struct infcall_suspend_state *inf_state)
{
  delete inf_state;
}

readonly_detached_regcache *
get_infcall_suspend_state_regcache (struct infcall_suspend_state *inf_state)
{
  return inf_state->registers ();
}

/* infcall_control_state contains state regarding gdb's control of the
   inferior itself like stepping control.  It also contains session state like
   the user's currently selected frame.  */

struct infcall_control_state
{
  struct thread_control_state thread_control;
  struct inferior_control_state inferior_control;

  /* Other fields:  */
  enum stop_stack_kind stop_stack_dummy = STOP_NONE;
  int stopped_by_random_signal = 0;

  /* ID and level of the selected frame when the inferior function
     call was made.  */
  struct frame_id selected_frame_id {};
  int selected_frame_level = -1;
};

/* Save all of the information associated with the inferior<==>gdb
   connection.  */

infcall_control_state_up
save_infcall_control_state ()
{
  infcall_control_state_up inf_status (new struct infcall_control_state);
  struct thread_info *tp = inferior_thread ();
  struct inferior *inf = current_inferior ();

  inf_status->thread_control = tp->control;
  inf_status->inferior_control = inf->control;

  tp->control.step_resume_breakpoint = nullptr;
  tp->control.exception_resume_breakpoint = nullptr;

  /* Save original bpstat chain to INF_STATUS; replace it in TP with copy of
     chain.  If caller's caller is walking the chain, they'll be happier if we
     hand them back the original chain when restore_infcall_control_state is
     called.  */
  tp->control.stop_bpstat = bpstat_copy (tp->control.stop_bpstat);

  /* Other fields:  */
  inf_status->stop_stack_dummy = stop_stack_dummy;
  inf_status->stopped_by_random_signal = stopped_by_random_signal;

  save_selected_frame (&inf_status->selected_frame_id,
		       &inf_status->selected_frame_level);

  return inf_status;
}

/* Restore inferior session state to INF_STATUS.  */

void
restore_infcall_control_state (struct infcall_control_state *inf_status)
{
  struct thread_info *tp = inferior_thread ();
  struct inferior *inf = current_inferior ();

  if (tp->control.step_resume_breakpoint)
    tp->control.step_resume_breakpoint->disposition = disp_del_at_next_stop;

  if (tp->control.exception_resume_breakpoint)
    tp->control.exception_resume_breakpoint->disposition
      = disp_del_at_next_stop;

  /* Handle the bpstat_copy of the chain.  */
  bpstat_clear (&tp->control.stop_bpstat);

  tp->control = inf_status->thread_control;
  inf->control = inf_status->inferior_control;

  /* Other fields:  */
  stop_stack_dummy = inf_status->stop_stack_dummy;
  stopped_by_random_signal = inf_status->stopped_by_random_signal;

  if (target_has_stack ())
    {
      restore_selected_frame (inf_status->selected_frame_id,
			      inf_status->selected_frame_level);
    }

  delete inf_status;
}

void
discard_infcall_control_state (struct infcall_control_state *inf_status)
{
  if (inf_status->thread_control.step_resume_breakpoint)
    inf_status->thread_control.step_resume_breakpoint->disposition
      = disp_del_at_next_stop;

  if (inf_status->thread_control.exception_resume_breakpoint)
    inf_status->thread_control.exception_resume_breakpoint->disposition
      = disp_del_at_next_stop;

  /* See save_infcall_control_state for info on stop_bpstat.  */
  bpstat_clear (&inf_status->thread_control.stop_bpstat);

  delete inf_status;
}

/* See infrun.h.  */

void
clear_exit_convenience_vars (void)
{
  clear_internalvar (lookup_internalvar ("_exitsignal"));
  clear_internalvar (lookup_internalvar ("_exitcode"));
}


/* User interface for reverse debugging:
   Set exec-direction / show exec-direction commands
   (returns error unless target implements to_set_exec_direction method).  */

enum exec_direction_kind execution_direction = EXEC_FORWARD;
static const char exec_forward[] = "forward";
static const char exec_reverse[] = "reverse";
static const char *exec_direction = exec_forward;
static const char *const exec_direction_names[] = {
  exec_forward,
  exec_reverse,
  nullptr
};

static void
set_exec_direction_func (const char *args, int from_tty,
			 struct cmd_list_element *cmd)
{
  if (target_can_execute_reverse ())
    {
      if (!strcmp (exec_direction, exec_forward))
	execution_direction = EXEC_FORWARD;
      else if (!strcmp (exec_direction, exec_reverse))
	execution_direction = EXEC_REVERSE;
    }
  else
    {
      exec_direction = exec_forward;
      error (_("Target does not support this operation."));
    }
}

static void
show_exec_direction_func (struct ui_file *out, int from_tty,
			  struct cmd_list_element *cmd, const char *value)
{
  switch (execution_direction) {
  case EXEC_FORWARD:
    gdb_printf (out, _("Forward.\n"));
    break;
  case EXEC_REVERSE:
    gdb_printf (out, _("Reverse.\n"));
    break;
  default:
    internal_error (_("bogus execution_direction value: %d"),
		    (int) execution_direction);
  }
}

static void
show_schedule_multiple (struct ui_file *file, int from_tty,
			struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Resuming the execution of threads "
		      "of all processes is %s.\n"), value);
}

/* Implementation of `siginfo' variable.  */

static const struct internalvar_funcs siginfo_funcs =
{
  siginfo_make_value,
  nullptr,
};

/* Callback for infrun's target events source.  This is marked when a
   thread has a pending status to process.  */

static void
infrun_async_inferior_event_handler (gdb_client_data data)
{
  clear_async_event_handler (infrun_async_inferior_event_token);
  inferior_event_handler (INF_REG_EVENT);
}

#if GDB_SELF_TEST
namespace selftests
{

/* Verify that when two threads with the same ptid exist (from two different
   targets) and one of them changes ptid, we only update inferior_ptid if
   it is appropriate.  */

static void
infrun_thread_ptid_changed ()
{
  gdbarch *arch = current_inferior ()->arch ();

  /* The thread which inferior_ptid represents changes ptid.  */
  {
    scoped_restore_current_pspace_and_thread restore;

    scoped_mock_context<test_target_ops> target1 (arch);
    scoped_mock_context<test_target_ops> target2 (arch);

    ptid_t old_ptid (111, 222);
    ptid_t new_ptid (111, 333);

    target1.mock_inferior.pid = old_ptid.pid ();
    target1.mock_thread.ptid = old_ptid;
    target1.mock_inferior.ptid_thread_map.clear ();
    target1.mock_inferior.ptid_thread_map[old_ptid] = &target1.mock_thread;

    target2.mock_inferior.pid = old_ptid.pid ();
    target2.mock_thread.ptid = old_ptid;
    target2.mock_inferior.ptid_thread_map.clear ();
    target2.mock_inferior.ptid_thread_map[old_ptid] = &target2.mock_thread;

    auto restore_inferior_ptid = make_scoped_restore (&inferior_ptid, old_ptid);
    set_current_inferior (&target1.mock_inferior);

    thread_change_ptid (&target1.mock_target, old_ptid, new_ptid);

    gdb_assert (inferior_ptid == new_ptid);
  }

  /* A thread with the same ptid as inferior_ptid, but from another target,
     changes ptid.  */
  {
    scoped_restore_current_pspace_and_thread restore;

    scoped_mock_context<test_target_ops> target1 (arch);
    scoped_mock_context<test_target_ops> target2 (arch);

    ptid_t old_ptid (111, 222);
    ptid_t new_ptid (111, 333);

    target1.mock_inferior.pid = old_ptid.pid ();
    target1.mock_thread.ptid = old_ptid;
    target1.mock_inferior.ptid_thread_map.clear ();
    target1.mock_inferior.ptid_thread_map[old_ptid] = &target1.mock_thread;

    target2.mock_inferior.pid = old_ptid.pid ();
    target2.mock_thread.ptid = old_ptid;
    target2.mock_inferior.ptid_thread_map.clear ();
    target2.mock_inferior.ptid_thread_map[old_ptid] = &target2.mock_thread;

    auto restore_inferior_ptid = make_scoped_restore (&inferior_ptid, old_ptid);
    set_current_inferior (&target2.mock_inferior);

    thread_change_ptid (&target1.mock_target, old_ptid, new_ptid);

    gdb_assert (inferior_ptid == old_ptid);
  }
}

} /* namespace selftests */

#endif /* GDB_SELF_TEST */

void _initialize_infrun ();
void
_initialize_infrun ()
{
  struct cmd_list_element *c;

  /* Register extra event sources in the event loop.  */
  infrun_async_inferior_event_token
    = create_async_event_handler (infrun_async_inferior_event_handler, nullptr,
				  "infrun");

  cmd_list_element *info_signals_cmd
    = add_info ("signals", info_signals_command, _("\
What debugger does when program gets various signals.\n\
Specify a signal as argument to print info on that signal only."));
  add_info_alias ("handle", info_signals_cmd, 0);

  c = add_com ("handle", class_run, handle_command, _("\
Specify how to handle signals.\n\
Usage: handle SIGNAL [ACTIONS]\n\
Args are signals and actions to apply to those signals.\n\
If no actions are specified, the current settings for the specified signals\n\
will be displayed instead.\n\
\n\
Symbolic signals (e.g. SIGSEGV) are recommended but numeric signals\n\
from 1-15 are allowed for compatibility with old versions of GDB.\n\
Numeric ranges may be specified with the form LOW-HIGH (e.g. 1-5).\n\
The special arg \"all\" is recognized to mean all signals except those\n\
used by the debugger, typically SIGTRAP and SIGINT.\n\
\n\
Recognized actions include \"stop\", \"nostop\", \"print\", \"noprint\",\n\
\"pass\", \"nopass\", \"ignore\", or \"noignore\".\n\
Stop means reenter debugger if this signal happens (implies print).\n\
Print means print a message if this signal happens.\n\
Pass means let program see this signal; otherwise program doesn't know.\n\
Ignore is a synonym for nopass and noignore is a synonym for pass.\n\
Pass and Stop may be combined.\n\
\n\
Multiple signals may be specified.  Signal numbers and signal names\n\
may be interspersed with actions, with the actions being performed for\n\
all signals cumulatively specified."));
  set_cmd_completer (c, handle_completer);

  stop_command = add_cmd ("stop", class_obscure,
			  not_just_help_class_command, _("\
There is no `stop' command, but you can set a hook on `stop'.\n\
This allows you to set a list of commands to be run each time execution\n\
of the program stops."), &cmdlist);

  add_setshow_boolean_cmd
    ("infrun", class_maintenance, &debug_infrun,
     _("Set inferior debugging."),
     _("Show inferior debugging."),
     _("When non-zero, inferior specific debugging is enabled."),
     nullptr, show_debug_infrun, &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("non-stop", no_class,
			   &non_stop_1, _("\
Set whether gdb controls the inferior in non-stop mode."), _("\
Show whether gdb controls the inferior in non-stop mode."), _("\
When debugging a multi-threaded program and this setting is\n\
off (the default, also called all-stop mode), when one thread stops\n\
(for a breakpoint, watchpoint, exception, or similar events), GDB stops\n\
all other threads in the program while you interact with the thread of\n\
interest.  When you continue or step a thread, you can allow the other\n\
threads to run, or have them remain stopped, but while you inspect any\n\
thread's state, all threads stop.\n\
\n\
In non-stop mode, when one thread stops, other threads can continue\n\
to run freely.  You'll be able to step each thread independently,\n\
leave it stopped or free to run as needed."),
			   set_non_stop,
			   show_non_stop,
			   &setlist,
			   &showlist);

  for (size_t i = 0; i < GDB_SIGNAL_LAST; i++)
    {
      signal_stop[i] = 1;
      signal_print[i] = 1;
      signal_program[i] = 1;
      signal_catch[i] = 0;
    }

  /* Signals caused by debugger's own actions should not be given to
     the program afterwards.

     Do not deliver GDB_SIGNAL_TRAP by default, except when the user
     explicitly specifies that it should be delivered to the target
     program.  Typically, that would occur when a user is debugging a
     target monitor on a simulator: the target monitor sets a
     breakpoint; the simulator encounters this breakpoint and halts
     the simulation handing control to GDB; GDB, noting that the stop
     address doesn't map to any known breakpoint, returns control back
     to the simulator; the simulator then delivers the hardware
     equivalent of a GDB_SIGNAL_TRAP to the program being
     debugged.  */
  signal_program[GDB_SIGNAL_TRAP] = 0;
  signal_program[GDB_SIGNAL_INT] = 0;

  /* Signals that are not errors should not normally enter the debugger.  */
  signal_stop[GDB_SIGNAL_ALRM] = 0;
  signal_print[GDB_SIGNAL_ALRM] = 0;
  signal_stop[GDB_SIGNAL_VTALRM] = 0;
  signal_print[GDB_SIGNAL_VTALRM] = 0;
  signal_stop[GDB_SIGNAL_PROF] = 0;
  signal_print[GDB_SIGNAL_PROF] = 0;
  signal_stop[GDB_SIGNAL_CHLD] = 0;
  signal_print[GDB_SIGNAL_CHLD] = 0;
  signal_stop[GDB_SIGNAL_IO] = 0;
  signal_print[GDB_SIGNAL_IO] = 0;
  signal_stop[GDB_SIGNAL_POLL] = 0;
  signal_print[GDB_SIGNAL_POLL] = 0;
  signal_stop[GDB_SIGNAL_URG] = 0;
  signal_print[GDB_SIGNAL_URG] = 0;
  signal_stop[GDB_SIGNAL_WINCH] = 0;
  signal_print[GDB_SIGNAL_WINCH] = 0;
  signal_stop[GDB_SIGNAL_PRIO] = 0;
  signal_print[GDB_SIGNAL_PRIO] = 0;

  /* These signals are used internally by user-level thread
     implementations.  (See signal(5) on Solaris.)  Like the above
     signals, a healthy program receives and handles them as part of
     its normal operation.  */
  signal_stop[GDB_SIGNAL_LWP] = 0;
  signal_print[GDB_SIGNAL_LWP] = 0;
  signal_stop[GDB_SIGNAL_WAITING] = 0;
  signal_print[GDB_SIGNAL_WAITING] = 0;
  signal_stop[GDB_SIGNAL_CANCEL] = 0;
  signal_print[GDB_SIGNAL_CANCEL] = 0;
  signal_stop[GDB_SIGNAL_LIBRT] = 0;
  signal_print[GDB_SIGNAL_LIBRT] = 0;

  /* Update cached state.  */
  signal_cache_update (-1);

  add_setshow_zinteger_cmd ("stop-on-solib-events", class_support,
			    &stop_on_solib_events, _("\
Set stopping for shared library events."), _("\
Show stopping for shared library events."), _("\
If nonzero, gdb will give control to the user when the dynamic linker\n\
notifies gdb of shared library events.  The most common event of interest\n\
to the user would be loading/unloading of a new library."),
			    set_stop_on_solib_events,
			    show_stop_on_solib_events,
			    &setlist, &showlist);

  add_setshow_enum_cmd ("follow-fork-mode", class_run,
			follow_fork_mode_kind_names,
			&follow_fork_mode_string, _("\
Set debugger response to a program call of fork or vfork."), _("\
Show debugger response to a program call of fork or vfork."), _("\
A fork or vfork creates a new process.  follow-fork-mode can be:\n\
  parent  - the original process is debugged after a fork\n\
  child   - the new process is debugged after a fork\n\
The unfollowed process will continue to run.\n\
By default, the debugger will follow the parent process."),
			nullptr,
			show_follow_fork_mode_string,
			&setlist, &showlist);

  add_setshow_enum_cmd ("follow-exec-mode", class_run,
			follow_exec_mode_names,
			&follow_exec_mode_string, _("\
Set debugger response to a program call of exec."), _("\
Show debugger response to a program call of exec."), _("\
An exec call replaces the program image of a process.\n\
\n\
follow-exec-mode can be:\n\
\n\
  new - the debugger creates a new inferior and rebinds the process\n\
to this new inferior.  The program the process was running before\n\
the exec call can be restarted afterwards by restarting the original\n\
inferior.\n\
\n\
  same - the debugger keeps the process bound to the same inferior.\n\
The new executable image replaces the previous executable loaded in\n\
the inferior.  Restarting the inferior after the exec call restarts\n\
the executable the process was running after the exec call.\n\
\n\
By default, the debugger will use the same inferior."),
			nullptr,
			show_follow_exec_mode_string,
			&setlist, &showlist);

  add_setshow_enum_cmd ("scheduler-locking", class_run, 
			scheduler_enums, &scheduler_mode, _("\
Set mode for locking scheduler during execution."), _("\
Show mode for locking scheduler during execution."), _("\
off    == no locking (threads may preempt at any time)\n\
on     == full locking (no thread except the current thread may run)\n\
	  This applies to both normal execution and replay mode.\n\
step   == scheduler locked during stepping commands (step, next, stepi, nexti).\n\
	  In this mode, other threads may run during other commands.\n\
	  This applies to both normal execution and replay mode.\n\
replay == scheduler locked in replay mode and unlocked during normal execution."),
			set_schedlock_func,	/* traps on target vector */
			show_scheduler_mode,
			&setlist, &showlist);

  add_setshow_boolean_cmd ("schedule-multiple", class_run, &sched_multi, _("\
Set mode for resuming threads of all processes."), _("\
Show mode for resuming threads of all processes."), _("\
When on, execution commands (such as 'continue' or 'next') resume all\n\
threads of all processes.  When off (which is the default), execution\n\
commands only resume the threads of the current process.  The set of\n\
threads that are resumed is further refined by the scheduler-locking\n\
mode (see help set scheduler-locking)."),
			   nullptr,
			   show_schedule_multiple,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("step-mode", class_run, &step_stop_if_no_debug, _("\
Set mode of the step operation."), _("\
Show mode of the step operation."), _("\
When set, doing a step over a function without debug line information\n\
will stop at the first instruction of that function. Otherwise, the\n\
function is skipped and the step command stops at a different source line."),
			   nullptr,
			   show_step_stop_if_no_debug,
			   &setlist, &showlist);

  add_setshow_auto_boolean_cmd ("displaced-stepping", class_run,
				&can_use_displaced_stepping, _("\
Set debugger's willingness to use displaced stepping."), _("\
Show debugger's willingness to use displaced stepping."), _("\
If on, gdb will use displaced stepping to step over breakpoints if it is\n\
supported by the target architecture.  If off, gdb will not use displaced\n\
stepping to step over breakpoints, even if such is supported by the target\n\
architecture.  If auto (which is the default), gdb will use displaced stepping\n\
if the target architecture supports it and non-stop mode is active, but will not\n\
use it in all-stop mode (see help set non-stop)."),
				nullptr,
				show_can_use_displaced_stepping,
				&setlist, &showlist);

  add_setshow_enum_cmd ("exec-direction", class_run, exec_direction_names,
			&exec_direction, _("Set direction of execution.\n\
Options are 'forward' or 'reverse'."),
			_("Show direction of execution (forward/reverse)."),
			_("Tells gdb whether to execute forward or backward."),
			set_exec_direction_func, show_exec_direction_func,
			&setlist, &showlist);

  /* Set/show detach-on-fork: user-settable mode.  */

  add_setshow_boolean_cmd ("detach-on-fork", class_run, &detach_fork, _("\
Set whether gdb will detach the child of a fork."), _("\
Show whether gdb will detach the child of a fork."), _("\
Tells gdb whether to detach the child of a fork."),
			   nullptr, nullptr, &setlist, &showlist);

  /* Set/show disable address space randomization mode.  */

  add_setshow_boolean_cmd ("disable-randomization", class_support,
			   &disable_randomization, _("\
Set disabling of debuggee's virtual address space randomization."), _("\
Show disabling of debuggee's virtual address space randomization."), _("\
When this mode is on (which is the default), randomization of the virtual\n\
address space is disabled.  Standalone programs run with the randomization\n\
enabled by default on some platforms."),
			   &set_disable_randomization,
			   &show_disable_randomization,
			   &setlist, &showlist);

  /* ptid initializations */
  inferior_ptid = null_ptid;
  target_last_wait_ptid = minus_one_ptid;

  gdb::observers::thread_ptid_changed.attach (infrun_thread_ptid_changed,
					      "infrun");
  gdb::observers::thread_stop_requested.attach (infrun_thread_stop_requested,
						"infrun");
  gdb::observers::inferior_exit.attach (infrun_inferior_exit, "infrun");
  gdb::observers::inferior_execd.attach (infrun_inferior_execd, "infrun");

  /* Explicitly create without lookup, since that tries to create a
     value with a void typed value, and when we get here, gdbarch
     isn't initialized yet.  At this point, we're quite sure there
     isn't another convenience variable of the same name.  */
  create_internalvar_type_lazy ("_siginfo", &siginfo_funcs, nullptr);

  add_setshow_boolean_cmd ("observer", no_class,
			   &observer_mode_1, _("\
Set whether gdb controls the inferior in observer mode."), _("\
Show whether gdb controls the inferior in observer mode."), _("\
In observer mode, GDB can get data from the inferior, but not\n\
affect its execution.  Registers and memory may not be changed,\n\
breakpoints may not be set, and the program cannot be interrupted\n\
or signalled."),
			   set_observer_mode,
			   show_observer_mode,
			   &setlist,
			   &showlist);

#if GDB_SELF_TEST
  selftests::register_test ("infrun_thread_ptid_changed",
			    selftests::infrun_thread_ptid_changed);
#endif
}
