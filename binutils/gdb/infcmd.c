/* Memory-access and commands for "inferior" process, for GDB.

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
#include "arch-utils.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "frame.h"
#include "inferior.h"
#include "infrun.h"
#include "gdbsupport/environ.h"
#include "value.h"
#include "gdbcmd.h"
#include "symfile.h"
#include "gdbcore.h"
#include "target.h"
#include "language.h"
#include "objfiles.h"
#include "completer.h"
#include "ui-out.h"
#include "regcache.h"
#include "reggroups.h"
#include "block.h"
#include "solib.h"
#include <ctype.h>
#include "observable.h"
#include "target-descriptions.h"
#include "user-regs.h"
#include "gdbthread.h"
#include "valprint.h"
#include "inline-frame.h"
#include "tracepoint.h"
#include "inf-loop.h"
#include "linespec.h"
#include "thread-fsm.h"
#include "ui.h"
#include "interps.h"
#include "skip.h"
#include <optional>
#include "source.h"
#include "cli/cli-style.h"
#include "dwarf2/loc.h"

/* Local functions: */

static void until_next_command (int);

static void step_1 (int, int, const char *);

#define ERROR_NO_INFERIOR \
   if (!target_has_execution ()) error (_("The program is not being run."));

/* Pid of our debugged inferior, or 0 if no inferior now.
   Since various parts of infrun.c test this to see whether there is a program
   being debugged it should be nonzero (currently 3 is used) for remote
   debugging.  */

ptid_t inferior_ptid;

/* Nonzero if stopped due to completion of a stack dummy routine.  */

enum stop_stack_kind stop_stack_dummy;

/* Nonzero if stopped due to a random (unexpected) signal in inferior
   process.  */

int stopped_by_random_signal;


/* Whether "finish" should print the value.  */

static bool finish_print = true;



/* Store the new value passed to 'set inferior-tty'.  */

static void
set_tty_value (const std::string &tty)
{
  current_inferior ()->set_tty (tty);
}

/* Get the current 'inferior-tty' value.  */

static const std::string &
get_tty_value ()
{
  return current_inferior ()->tty ();
}

/* Implement 'show inferior-tty' command.  */

static void
show_inferior_tty_command (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c, const char *value)
{
  /* Note that we ignore the passed-in value in favor of computing it
     directly.  */
  const std::string &inferior_tty = current_inferior ()->tty ();

  gdb_printf (file,
	      _("Terminal for future runs of program being debugged "
		"is \"%s\".\n"), inferior_tty.c_str ());
}

/* Store the new value passed to 'set args'.  */

static void
set_args_value (const std::string &args)
{
  current_inferior ()->set_args (args);
}

/* Return the value for 'show args' to display.  */

static const std::string &
get_args_value ()
{
  return current_inferior ()->args ();
}

/* Callback to implement 'show args' command.  */

static void
show_args_command (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  /* Ignore the passed in value, pull the argument directly from the
     inferior.  However, these should always be the same.  */
  gdb_printf (file, _("\
Argument list to give program being debugged when it is started is \"%s\".\n"),
	      current_inferior ()->args ().c_str ());
}

/* See gdbsupport/common-inferior.h.  */

const std::string &
get_inferior_cwd ()
{
  return current_inferior ()->cwd ();
}

/* Store the new value passed to 'set cwd'.  */

static void
set_cwd_value (const std::string &args)
{
  current_inferior ()->set_cwd (args);
}

/* Handle the 'show cwd' command.  */

static void
show_cwd_command (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  const std::string &cwd = current_inferior ()->cwd ();

  if (cwd.empty ())
    gdb_printf (file,
		_("\
You have not set the inferior's current working directory.\n\
The inferior will inherit GDB's cwd if native debugging, or the remote\n\
server's cwd if remote debugging.\n"));
  else
    gdb_printf (file,
		_("Current working directory that will be used "
		  "when starting the inferior is \"%s\".\n"),
		cwd.c_str ());
}


/* This function strips the '&' character (indicating background
   execution) that is added as *the last* of the arguments ARGS of a
   command.  A copy of the incoming ARGS without the '&' is returned,
   unless the resulting string after stripping is empty, in which case
   NULL is returned.  *BG_CHAR_P is an output boolean that indicates
   whether the '&' character was found.  */

static gdb::unique_xmalloc_ptr<char>
strip_bg_char (const char *args, int *bg_char_p)
{
  const char *p;

  if (args == nullptr || *args == '\0')
    {
      *bg_char_p = 0;
      return nullptr;
    }

  p = args + strlen (args);
  if (p[-1] == '&')
    {
      p--;
      while (p > args && isspace (p[-1]))
	p--;

      *bg_char_p = 1;
      if (p != args)
	return gdb::unique_xmalloc_ptr<char>
	  (savestring (args, p - args));
      else
	return gdb::unique_xmalloc_ptr<char> (nullptr);
    }

  *bg_char_p = 0;
  return make_unique_xstrdup (args);
}

/* Common actions to take after creating any sort of inferior, by any
   means (running, attaching, connecting, et cetera).  The target
   should be stopped.  */

void
post_create_inferior (int from_tty)
{

  /* Be sure we own the terminal in case write operations are performed.  */ 
  target_terminal::ours_for_output ();

  infrun_debug_show_threads ("threads in the newly created inferior",
			     current_inferior ()->non_exited_threads ());

  /* If the target hasn't taken care of this already, do it now.
     Targets which need to access registers during to_open,
     to_create_inferior, or to_attach should do it earlier; but many
     don't need to.  */
  target_find_description ();

  /* Now that we know the register layout, retrieve current PC.  But
     if the PC is unavailable (e.g., we're opening a core file with
     missing registers info), ignore it.  */
  thread_info *thr = inferior_thread ();

  thr->clear_stop_pc ();
  try
    {
      regcache *rc = get_thread_regcache (thr);
      thr->set_stop_pc (regcache_read_pc (rc));
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw;
    }

  if (current_program_space->exec_bfd ())
    {
      const unsigned solib_add_generation
	= current_program_space->solib_add_generation;

      scoped_restore restore_in_initial_library_scan
	= make_scoped_restore (&current_inferior ()->in_initial_library_scan,
			       true);

      /* Create the hooks to handle shared library load and unload
	 events.  */
      solib_create_inferior_hook (from_tty);

      if (current_program_space->solib_add_generation == solib_add_generation)
	{
	  /* The platform-specific hook should load initial shared libraries,
	     but didn't.  FROM_TTY will be incorrectly 0 but such solib
	     targets should be fixed anyway.  Call it only after the solib
	     target has been initialized by solib_create_inferior_hook.  */

	  if (info_verbose)
	    warning (_("platform-specific solib_create_inferior_hook did "
		       "not load initial shared libraries."));

	  /* If the solist is global across processes, there's no need to
	     refetch it here.  */
	  if (!gdbarch_has_global_solist (current_inferior ()->arch ()))
	    solib_add (nullptr, 0, auto_solib_add);
	}
    }

  /* If the user sets watchpoints before execution having started,
     then she gets software watchpoints, because GDB can't know which
     target will end up being pushed, or if it supports hardware
     watchpoints or not.  breakpoint_re_set takes care of promoting
     watchpoints to hardware watchpoints if possible, however, if this
     new inferior doesn't load shared libraries or we don't pull in
     symbols from any other source on this target/arch,
     breakpoint_re_set is never called.  Call it now so that software
     watchpoints get a chance to be promoted to hardware watchpoints
     if the now pushed target supports hardware watchpoints.  */
  breakpoint_re_set ();

  gdb::observers::inferior_created.notify (current_inferior ());
}

/* Kill the inferior if already running.  This function is designed
   to be called when we are about to start the execution of the program
   from the beginning.  Ask the user to confirm that he wants to restart
   the program being debugged when FROM_TTY is non-null.  */

static void
kill_if_already_running (int from_tty)
{
  if (inferior_ptid != null_ptid && target_has_execution ())
    {
      /* Bail out before killing the program if we will not be able to
	 restart it.  */
      target_require_runnable ();

      if (from_tty
	  && !query (_("The program being debugged has been started already.\n\
Start it from the beginning? ")))
	error (_("Program not restarted."));
      target_kill ();
    }
}

/* See inferior.h.  */

void
prepare_execution_command (struct target_ops *target, int background)
{
  /* If we get a request for running in the bg but the target
     doesn't support it, error out.  */
  if (background && !target_can_async_p (target))
    error (_("Asynchronous execution not supported on this target."));

  if (!background)
    {
      /* If we get a request for running in the fg, then we need to
	 simulate synchronous (fg) execution.  Note no cleanup is
	 necessary for this.  stdin is re-enabled whenever an error
	 reaches the top level.  */
      all_uis_on_sync_execution_starting ();
    }
}

/* Determine how the new inferior will behave.  */

enum run_how
  {
    /* Run program without any explicit stop during startup.  */
    RUN_NORMAL,

    /* Stop at the beginning of the program's main function.  */
    RUN_STOP_AT_MAIN,

    /* Stop at the first instruction of the program.  */
    RUN_STOP_AT_FIRST_INSN
  };

/* Implement the "run" command.  Force a stop during program start if
   requested by RUN_HOW.  */

static void
run_command_1 (const char *args, int from_tty, enum run_how run_how)
{
  const char *exec_file;
  struct ui_out *uiout = current_uiout;
  struct target_ops *run_target;
  int async_exec;

  dont_repeat ();

  scoped_disable_commit_resumed disable_commit_resumed ("running");

  kill_if_already_running (from_tty);

  init_wait_for_inferior ();
  clear_breakpoint_hit_counts ();

  /* Clean up any leftovers from other runs.  Some other things from
     this function should probably be moved into target_pre_inferior.  */
  target_pre_inferior (from_tty);

  /* The comment here used to read, "The exec file is re-read every
     time we do a generic_mourn_inferior, so we just have to worry
     about the symbol file."  The `generic_mourn_inferior' function
     gets called whenever the program exits.  However, suppose the
     program exits, and *then* the executable file changes?  We need
     to check again here.  Since reopen_exec_file doesn't do anything
     if the timestamp hasn't changed, I don't see the harm.  */
  reopen_exec_file ();
  reread_symbols (from_tty);

  gdb::unique_xmalloc_ptr<char> stripped = strip_bg_char (args, &async_exec);
  args = stripped.get ();

  /* Do validation and preparation before possibly changing anything
     in the inferior.  */

  run_target = find_run_target ();

  prepare_execution_command (run_target, async_exec);

  if (non_stop && !run_target->supports_non_stop ())
    error (_("The target does not support running in non-stop mode."));

  /* Done.  Can now set breakpoints, change inferior args, etc.  */

  /* Insert temporary breakpoint in main function if requested.  */
  if (run_how == RUN_STOP_AT_MAIN)
    {
      /* To avoid other inferiors hitting this breakpoint, make it
	 inferior-specific.  */
      std::string arg = string_printf ("-qualified %s inferior %d",
				       main_name (),
				       current_inferior ()->num);
      tbreak_command (arg.c_str (), 0);
    }

  exec_file = get_exec_file (0);

  /* We keep symbols from add-symbol-file, on the grounds that the
     user might want to add some symbols before running the program
     (right?).  But sometimes (dynamic loading where the user manually
     introduces the new symbols with add-symbol-file), the code which
     the symbols describe does not persist between runs.  Currently
     the user has to manually nuke all symbols between runs if they
     want them to go away (PR 2207).  This is probably reasonable.  */

  /* If there were other args, beside '&', process them.  */
  if (args != nullptr)
    current_inferior ()->set_args (args);

  if (from_tty)
    {
      uiout->field_string (nullptr, "Starting program");
      uiout->text (": ");
      if (exec_file)
	uiout->field_string ("execfile", exec_file,
			     file_name_style.style ());
      uiout->spaces (1);
      uiout->field_string ("infargs", current_inferior ()->args ());
      uiout->text ("\n");
      uiout->flush ();
    }

  run_target->create_inferior (exec_file,
			       current_inferior ()->args (),
			       current_inferior ()->environment.envp (),
			       from_tty);
  /* to_create_inferior should push the target, so after this point we
     shouldn't refer to run_target again.  */
  run_target = nullptr;

  infrun_debug_show_threads ("immediately after create_process",
			     current_inferior ()->non_exited_threads ());

  /* We're starting off a new process.  When we get out of here, in
     non-stop mode, finish the state of all threads of that process,
     but leave other threads alone, as they may be stopped in internal
     events --- the frontend shouldn't see them as stopped.  In
     all-stop, always finish the state of all threads, as we may be
     resuming more than just the new process.  */
  process_stratum_target *finish_target;
  ptid_t finish_ptid;
  if (non_stop)
    {
      finish_target = current_inferior ()->process_target ();
      finish_ptid = ptid_t (current_inferior ()->pid);
    }
  else
    {
      finish_target = nullptr;
      finish_ptid = minus_one_ptid;
    }
  scoped_finish_thread_state finish_state (finish_target, finish_ptid);

  /* Pass zero for FROM_TTY, because at this point the "run" command
     has done its thing; now we are setting up the running program.  */
  post_create_inferior (0);

  /* Queue a pending event so that the program stops immediately.  */
  if (run_how == RUN_STOP_AT_FIRST_INSN)
    {
      thread_info *thr = inferior_thread ();
      target_waitstatus ws;
      ws.set_stopped (GDB_SIGNAL_0);
      thr->set_pending_waitstatus (ws);
    }

  /* Start the target running.  Do not use -1 continuation as it would skip
     breakpoint right at the entry point.  */
  proceed (regcache_read_pc (get_thread_regcache (inferior_thread ())),
	   GDB_SIGNAL_0);

  /* Since there was no error, there's no need to finish the thread
     states here.  */
  finish_state.release ();

  disable_commit_resumed.reset_and_commit ();
}

static void
run_command (const char *args, int from_tty)
{
  run_command_1 (args, from_tty, RUN_NORMAL);
}

/* Start the execution of the program up until the beginning of the main
   program.  */

static void
start_command (const char *args, int from_tty)
{
  /* Some languages such as Ada need to search inside the program
     minimal symbols for the location where to put the temporary
     breakpoint before starting.  */
  if (!have_minimal_symbols ())
    error (_("No symbol table loaded.  Use the \"file\" command."));

  /* Run the program until reaching the main procedure...  */
  run_command_1 (args, from_tty, RUN_STOP_AT_MAIN);
}

/* Start the execution of the program stopping at the first
   instruction.  */

static void
starti_command (const char *args, int from_tty)
{
  run_command_1 (args, from_tty, RUN_STOP_AT_FIRST_INSN);
} 

static int
proceed_thread_callback (struct thread_info *thread, void *arg)
{
  /* We go through all threads individually instead of compressing
     into a single target `resume_all' request, because some threads
     may be stopped in internal breakpoints/events, or stopped waiting
     for its turn in the displaced stepping queue (that is, they are
     running && !executing).  The target side has no idea about why
     the thread is stopped, so a `resume_all' command would resume too
     much.  If/when GDB gains a way to tell the target `hold this
     thread stopped until I say otherwise', then we can optimize
     this.  */
  if (thread->state != THREAD_STOPPED)
    return 0;

  if (!thread->inf->has_execution ())
    return 0;

  switch_to_thread (thread);
  clear_proceed_status (0);
  proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
  return 0;
}

static void
ensure_valid_thread (void)
{
  if (inferior_ptid == null_ptid
      || inferior_thread ()->state == THREAD_EXITED)
    error (_("Cannot execute this command without a live selected thread."));
}

/* If the user is looking at trace frames, any resumption of execution
   is likely to mix up recorded and live target data.  So simply
   disallow those commands.  */

static void
ensure_not_tfind_mode (void)
{
  if (get_traceframe_number () >= 0)
    error (_("Cannot execute this command while looking at trace frames."));
}

/* Throw an error indicating the current thread is running.  */

static void
error_is_running (void)
{
  error (_("Cannot execute this command while "
	   "the selected thread is running."));
}

/* Calls error_is_running if the current thread is running.  */

static void
ensure_not_running (void)
{
  if (inferior_thread ()->state == THREAD_RUNNING)
    error_is_running ();
}

void
continue_1 (int all_threads)
{
  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();

  if (non_stop && all_threads)
    {
      /* Don't error out if the current thread is running, because
	 there may be other stopped threads.  */

      /* Backup current thread and selected frame and restore on scope
	 exit.  */
      scoped_restore_current_thread restore_thread;
      scoped_disable_commit_resumed disable_commit_resumed
	("continue all threads in non-stop");

      iterate_over_threads (proceed_thread_callback, nullptr);

      if (current_ui->prompt_state == PROMPT_BLOCKED)
	{
	  /* If all threads in the target were already running,
	     proceed_thread_callback ends up never calling proceed,
	     and so nothing calls this to put the inferior's terminal
	     settings in effect and remove stdin from the event loop,
	     which we must when running a foreground command.  E.g.:

	      (gdb) c -a&
	      Continuing.
	      <all threads are running now>
	      (gdb) c -a
	      Continuing.
	      <no thread was resumed, but the inferior now owns the terminal>
	  */
	  target_terminal::inferior ();
	}

      disable_commit_resumed.reset_and_commit ();
    }
  else
    {
      ensure_valid_thread ();
      ensure_not_running ();
      clear_proceed_status (0);
      proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
    }
}

/* continue [-a] [proceed-count] [&]  */

static void
continue_command (const char *args, int from_tty)
{
  int async_exec;
  bool all_threads_p = false;

  ERROR_NO_INFERIOR;

  /* Find out whether we must run in the background.  */
  gdb::unique_xmalloc_ptr<char> stripped = strip_bg_char (args, &async_exec);
  args = stripped.get ();

  if (args != nullptr)
    {
      if (startswith (args, "-a"))
	{
	  all_threads_p = true;
	  args += sizeof ("-a") - 1;
	  if (*args == '\0')
	    args = nullptr;
	}
    }

  if (!non_stop && all_threads_p)
    error (_("`-a' is meaningless in all-stop mode."));

  if (args != nullptr && all_threads_p)
    error (_("Can't resume all threads and specify "
	     "proceed count simultaneously."));

  /* If we have an argument left, set proceed count of breakpoint we
     stopped at.  */
  if (args != nullptr)
    {
      bpstat *bs = nullptr;
      int num, stat;
      int stopped = 0;
      struct thread_info *tp;

      if (non_stop)
	tp = inferior_thread ();
      else
	{
	  process_stratum_target *last_target;
	  ptid_t last_ptid;

	  get_last_target_status (&last_target, &last_ptid, nullptr);
	  tp = last_target->find_thread (last_ptid);
	}
      if (tp != nullptr)
	bs = tp->control.stop_bpstat;

      while ((stat = bpstat_num (&bs, &num)) != 0)
	if (stat > 0)
	  {
	    set_ignore_count (num,
			      parse_and_eval_long (args) - 1,
			      from_tty);
	    /* set_ignore_count prints a message ending with a period.
	       So print two spaces before "Continuing.".  */
	    if (from_tty)
	      gdb_printf ("  ");
	    stopped = 1;
	  }

      if (!stopped && from_tty)
	{
	  gdb_printf
	    ("Not stopped at any breakpoint; argument ignored.\n");
	}
    }

  ensure_not_tfind_mode ();

  if (!non_stop || !all_threads_p)
    {
      ensure_valid_thread ();
      ensure_not_running ();
    }

  prepare_execution_command (current_inferior ()->top_target (), async_exec);

  if (from_tty)
    gdb_printf (_("Continuing.\n"));

  continue_1 (all_threads_p);
}

/* Record in TP the starting point of a "step" or "next" command.  */

static void
set_step_frame (thread_info *tp)
{
  /* This can be removed once this function no longer implicitly relies on the
     inferior_ptid value.  */
  gdb_assert (inferior_ptid == tp->ptid);

  frame_info_ptr frame = get_current_frame ();

  symtab_and_line sal = find_frame_sal (frame);
  set_step_info (tp, frame, sal);

  CORE_ADDR pc = get_frame_pc (frame);
  tp->control.step_start_function = find_pc_function (pc);
}

/* Step until outside of current statement.  */

static void
step_command (const char *count_string, int from_tty)
{
  step_1 (0, 0, count_string);
}

/* Likewise, but skip over subroutine calls as if single instructions.  */

static void
next_command (const char *count_string, int from_tty)
{
  step_1 (1, 0, count_string);
}

/* Likewise, but step only one instruction.  */

static void
stepi_command (const char *count_string, int from_tty)
{
  step_1 (0, 1, count_string);
}

static void
nexti_command (const char *count_string, int from_tty)
{
  step_1 (1, 1, count_string);
}

/* Data for the FSM that manages the step/next/stepi/nexti
   commands.  */

struct step_command_fsm : public thread_fsm
{
  /* How many steps left in a "step N"-like command.  */
  int count;

  /* If true, this is a next/nexti, otherwise a step/stepi.  */
  int skip_subroutines;

  /* If true, this is a stepi/nexti, otherwise a step/step.  */
  int single_inst;

  explicit step_command_fsm (struct interp *cmd_interp)
    : thread_fsm (cmd_interp)
  {
  }

  void clean_up (struct thread_info *thread) override;
  bool should_stop (struct thread_info *thread) override;
  enum async_reply_reason do_async_reply_reason () override;
};

/* Prepare for a step/next/etc. command.  Any target resource
   allocated here is undone in the FSM's clean_up method.  */

static void
step_command_fsm_prepare (struct step_command_fsm *sm,
			  int skip_subroutines, int single_inst,
			  int count, struct thread_info *thread)
{
  sm->skip_subroutines = skip_subroutines;
  sm->single_inst = single_inst;
  sm->count = count;

  /* Leave the si command alone.  */
  if (!sm->single_inst || sm->skip_subroutines)
    set_longjmp_breakpoint (thread, get_frame_id (get_current_frame ()));

  thread->control.stepping_command = 1;
}

static int prepare_one_step (thread_info *, struct step_command_fsm *sm);

static void
step_1 (int skip_subroutines, int single_inst, const char *count_string)
{
  int count;
  int async_exec;
  struct thread_info *thr;
  struct step_command_fsm *step_sm;

  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  gdb::unique_xmalloc_ptr<char> stripped
    = strip_bg_char (count_string, &async_exec);
  count_string = stripped.get ();

  prepare_execution_command (current_inferior ()->top_target (), async_exec);

  count = count_string ? parse_and_eval_long (count_string) : 1;

  clear_proceed_status (1);

  /* Setup the execution command state machine to handle all the COUNT
     steps.  */
  thr = inferior_thread ();
  step_sm = new step_command_fsm (command_interp ());
  thr->set_thread_fsm (std::unique_ptr<thread_fsm> (step_sm));

  step_command_fsm_prepare (step_sm, skip_subroutines,
			    single_inst, count, thr);

  /* Do only one step for now, before returning control to the event
     loop.  Let the continuation figure out how many other steps we
     need to do, and handle them one at the time, through
     step_once.  */
  if (!prepare_one_step (thr, step_sm))
    proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
  else
    {
      /* Stepped into an inline frame.  Pretend that we've
	 stopped.  */
      thr->thread_fsm ()->clean_up (thr);
      bool proceeded = normal_stop ();
      if (!proceeded)
	inferior_event_handler (INF_EXEC_COMPLETE);
      all_uis_check_sync_execution_done ();
    }
}

/* Implementation of the 'should_stop' FSM method for stepping
   commands.  Called after we are done with one step operation, to
   check whether we need to step again, before we print the prompt and
   return control to the user.  If count is > 1, returns false, as we
   will need to keep going.  */

bool
step_command_fsm::should_stop (struct thread_info *tp)
{
  if (tp->control.stop_step)
    {
      /* There are more steps to make, and we did stop due to
	 ending a stepping range.  Do another step.  */
      if (--count > 0)
	return prepare_one_step (tp, this);

      set_finished ();
    }

  return true;
}

/* Implementation of the 'clean_up' FSM method for stepping commands.  */

void
step_command_fsm::clean_up (struct thread_info *thread)
{
  if (!single_inst || skip_subroutines)
    delete_longjmp_breakpoint (thread->global_num);
}

/* Implementation of the 'async_reply_reason' FSM method for stepping
   commands.  */

enum async_reply_reason
step_command_fsm::do_async_reply_reason ()
{
  return EXEC_ASYNC_END_STEPPING_RANGE;
}

/* Prepare for one step in "step N".  The actual target resumption is
   done by the caller.  Return true if we're done and should thus
   report a stop to the user.  Returns false if the target needs to be
   resumed.  */

static int
prepare_one_step (thread_info *tp, struct step_command_fsm *sm)
{
  /* This can be removed once this function no longer implicitly relies on the
     inferior_ptid value.  */
  gdb_assert (inferior_ptid == tp->ptid);

  if (sm->count > 0)
    {
      frame_info_ptr frame = get_current_frame ();

      set_step_frame (tp);

      if (!sm->single_inst)
	{
	  CORE_ADDR pc;

	  /* Step at an inlined function behaves like "down".  */
	  if (!sm->skip_subroutines
	      && inline_skipped_frames (tp))
	    {
	      ptid_t resume_ptid;
	      const char *fn = nullptr;
	      symtab_and_line sal;
	      struct symbol *sym;

	      /* Pretend that we've ran.  */
	      resume_ptid = user_visible_resume_ptid (1);
	      set_running (tp->inf->process_target (), resume_ptid, true);

	      step_into_inline_frame (tp);

	      frame = get_current_frame ();
	      sal = find_frame_sal (frame);
	      sym = get_frame_function (frame);

	      if (sym != nullptr)
		fn = sym->print_name ();

	      if (sal.line == 0
		  || !function_name_is_marked_for_skip (fn, sal))
		{
		  sm->count--;
		  return prepare_one_step (tp, sm);
		}
	    }

	  pc = get_frame_pc (frame);
	  find_pc_line_pc_range (pc,
				 &tp->control.step_range_start,
				 &tp->control.step_range_end);

	  if (execution_direction == EXEC_REVERSE)
	    {
	      symtab_and_line sal = find_pc_line (pc, 0);
	      symtab_and_line sal_start
		= find_pc_line (tp->control.step_range_start, 0);

	      if (sal.line == sal_start.line)
		/* Executing in reverse, the step_range_start address is in
		   the same line.  We want to stop in the previous line so
		   move step_range_start before the current line.  */
		tp->control.step_range_start--;
	    }

	  /* There's a problem in gcc (PR gcc/98780) that causes missing line
	     table entries, which results in a too large stepping range.
	     Use inlined_subroutine info to make the range more narrow.  */
	  if (inline_skipped_frames (tp) > 0)
	    {
	      symbol *sym = inline_skipped_symbol (tp);
	      if (sym->aclass () == LOC_BLOCK)
		{
		  const block *block = sym->value_block ();
		  if (block->end () < tp->control.step_range_end)
		    tp->control.step_range_end = block->end ();
		}
	    }

	  tp->control.may_range_step = 1;

	  /* If we have no line info, switch to stepi mode.  */
	  if (tp->control.step_range_end == 0 && step_stop_if_no_debug)
	    {
	      tp->control.step_range_start = tp->control.step_range_end = 1;
	      tp->control.may_range_step = 0;
	    }
	  else if (tp->control.step_range_end == 0)
	    {
	      const char *name;

	      if (find_pc_partial_function (pc, &name,
					    &tp->control.step_range_start,
					    &tp->control.step_range_end) == 0)
		error (_("Cannot find bounds of current function"));

	      target_terminal::ours_for_output ();
	      gdb_printf (_("Single stepping until exit from function %s,"
			    "\nwhich has no line number information.\n"),
			  name);
	    }
	}
      else
	{
	  /* Say we are stepping, but stop after one insn whatever it does.  */
	  tp->control.step_range_start = tp->control.step_range_end = 1;
	  if (!sm->skip_subroutines)
	    /* It is stepi.
	       Don't step over function calls, not even to functions lacking
	       line numbers.  */
	    tp->control.step_over_calls = STEP_OVER_NONE;
	}

      if (sm->skip_subroutines)
	tp->control.step_over_calls = STEP_OVER_ALL;

      return 0;
    }

  /* Done.  */
  sm->set_finished ();
  return 1;
}


/* Continue program at specified address.  */

static void
jump_command (const char *arg, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();
  CORE_ADDR addr;
  struct symbol *fn;
  struct symbol *sfn;
  int async_exec;

  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  /* Find out whether we must run in the background.  */
  gdb::unique_xmalloc_ptr<char> stripped = strip_bg_char (arg, &async_exec);
  arg = stripped.get ();

  prepare_execution_command (current_inferior ()->top_target (), async_exec);

  if (!arg)
    error_no_arg (_("starting address"));

  std::vector<symtab_and_line> sals
    = decode_line_with_current_source (arg, DECODE_LINE_FUNFIRSTLINE);
  if (sals.size () != 1)
    {
      /* If multiple sal-objects were found, try dropping those that aren't
	 from the current symtab.  */
      struct symtab_and_line cursal = get_current_source_symtab_and_line ();
      sals.erase (std::remove_if (sals.begin (), sals.end (),
		  [&] (const symtab_and_line &sal)
		    {
		      return sal.symtab != cursal.symtab;
		    }), sals.end ());
      if (sals.size () != 1)
	error (_("Jump request is ambiguous: "
		 "does not resolve to a single address"));
    }

  symtab_and_line &sal = sals[0];

  if (sal.symtab == 0 && sal.pc == 0)
    error (_("No source file has been specified."));

  resolve_sal_pc (&sal);	/* May error out.  */

  /* See if we are trying to jump to another function.  */
  fn = get_frame_function (get_current_frame ());
  sfn = find_pc_sect_containing_function (sal.pc,
					  find_pc_mapped_section (sal.pc));
  if (fn != nullptr && sfn != fn)
    {
      if (!query (_("Line %d is not in `%s'.  Jump anyway? "), sal.line,
		  fn->print_name ()))
	{
	  error (_("Not confirmed."));
	  /* NOTREACHED */
	}
    }

  if (sfn != nullptr)
    {
      struct obj_section *section;

      section = sfn->obj_section (sfn->objfile ());
      if (section_is_overlay (section)
	  && !section_is_mapped (section))
	{
	  if (!query (_("WARNING!!!  Destination is in "
			"unmapped overlay!  Jump anyway? ")))
	    {
	      error (_("Not confirmed."));
	      /* NOTREACHED */
	    }
	}
    }

  addr = sal.pc;

  if (from_tty)
    {
      gdb_printf (_("Continuing at "));
      gdb_puts (paddress (gdbarch, addr));
      gdb_printf (".\n");
    }

  clear_proceed_status (0);
  proceed (addr, GDB_SIGNAL_0);
}

/* Continue program giving it specified signal.  */

static void
signal_command (const char *signum_exp, int from_tty)
{
  enum gdb_signal oursig;
  int async_exec;

  dont_repeat ();		/* Too dangerous.  */
  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  /* Find out whether we must run in the background.  */
  gdb::unique_xmalloc_ptr<char> stripped
    = strip_bg_char (signum_exp, &async_exec);
  signum_exp = stripped.get ();

  prepare_execution_command (current_inferior ()->top_target (), async_exec);

  if (!signum_exp)
    error_no_arg (_("signal number"));

  /* It would be even slicker to make signal names be valid expressions,
     (the type could be "enum $signal" or some such), then the user could
     assign them to convenience variables.  */
  oursig = gdb_signal_from_name (signum_exp);

  if (oursig == GDB_SIGNAL_UNKNOWN)
    {
      /* No, try numeric.  */
      int num = parse_and_eval_long (signum_exp);

      if (num == 0)
	oursig = GDB_SIGNAL_0;
      else
	oursig = gdb_signal_from_command (num);
    }

  /* Look for threads other than the current that this command ends up
     resuming too (due to schedlock off), and warn if they'll get a
     signal delivered.  "signal 0" is used to suppress a previous
     signal, but if the current thread is no longer the one that got
     the signal, then the user is potentially suppressing the signal
     of the wrong thread.  */
  if (!non_stop)
    {
      int must_confirm = 0;

      /* This indicates what will be resumed.  Either a single thread,
	 a whole process, or all threads of all processes.  */
      ptid_t resume_ptid = user_visible_resume_ptid (0);
      process_stratum_target *resume_target
	= user_visible_resume_target (resume_ptid);

      thread_info *current = inferior_thread ();

      for (thread_info *tp : all_non_exited_threads (resume_target, resume_ptid))
	{
	  if (tp == current)
	    continue;

	  if (tp->stop_signal () != GDB_SIGNAL_0
	      && signal_pass_state (tp->stop_signal ()))
	    {
	      if (!must_confirm)
		gdb_printf (_("Note:\n"));
	      gdb_printf (_("  Thread %s previously stopped with signal %s, %s.\n"),
			  print_thread_id (tp),
			  gdb_signal_to_name (tp->stop_signal ()),
			  gdb_signal_to_string (tp->stop_signal ()));
	      must_confirm = 1;
	    }
	}

      if (must_confirm
	  && !query (_("Continuing thread %s (the current thread) with specified signal will\n"
		       "still deliver the signals noted above to their respective threads.\n"
		       "Continue anyway? "),
		     print_thread_id (inferior_thread ())))
	error (_("Not confirmed."));
    }

  if (from_tty)
    {
      if (oursig == GDB_SIGNAL_0)
	gdb_printf (_("Continuing with no signal.\n"));
      else
	gdb_printf (_("Continuing with signal %s.\n"),
		    gdb_signal_to_name (oursig));
    }

  clear_proceed_status (0);
  proceed ((CORE_ADDR) -1, oursig);
}

/* Queue a signal to be delivered to the current thread.  */

static void
queue_signal_command (const char *signum_exp, int from_tty)
{
  enum gdb_signal oursig;
  struct thread_info *tp;

  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  if (signum_exp == nullptr)
    error_no_arg (_("signal number"));

  /* It would be even slicker to make signal names be valid expressions,
     (the type could be "enum $signal" or some such), then the user could
     assign them to convenience variables.  */
  oursig = gdb_signal_from_name (signum_exp);

  if (oursig == GDB_SIGNAL_UNKNOWN)
    {
      /* No, try numeric.  */
      int num = parse_and_eval_long (signum_exp);

      if (num == 0)
	oursig = GDB_SIGNAL_0;
      else
	oursig = gdb_signal_from_command (num);
    }

  if (oursig != GDB_SIGNAL_0
      && !signal_pass_state (oursig))
    error (_("Signal handling set to not pass this signal to the program."));

  tp = inferior_thread ();
  tp->set_stop_signal (oursig);
}

/* Data for the FSM that manages the until (with no argument)
   command.  */

struct until_next_fsm : public thread_fsm
{
  /* The thread that as current when the command was executed.  */
  int thread;

  until_next_fsm (struct interp *cmd_interp, int thread)
    : thread_fsm (cmd_interp),
      thread (thread)
  {
  }

  bool should_stop (struct thread_info *thread) override;
  void clean_up (struct thread_info *thread) override;
  enum async_reply_reason do_async_reply_reason () override;
};

/* Implementation of the 'should_stop' FSM method for the until (with
   no arg) command.  */

bool
until_next_fsm::should_stop (struct thread_info *tp)
{
  if (tp->control.stop_step)
    set_finished ();

  return true;
}

/* Implementation of the 'clean_up' FSM method for the until (with no
   arg) command.  */

void
until_next_fsm::clean_up (struct thread_info *thread)
{
  delete_longjmp_breakpoint (thread->global_num);
}

/* Implementation of the 'async_reply_reason' FSM method for the until
   (with no arg) command.  */

enum async_reply_reason
until_next_fsm::do_async_reply_reason ()
{
  return EXEC_ASYNC_END_STEPPING_RANGE;
}

/* Proceed until we reach a different source line with pc greater than
   our current one or exit the function.  We skip calls in both cases.

   Note that eventually this command should probably be changed so
   that only source lines are printed out when we hit the breakpoint
   we set.  This may involve changes to wait_for_inferior and the
   proceed status code.  */

static void
until_next_command (int from_tty)
{
  frame_info_ptr frame;
  CORE_ADDR pc;
  struct symbol *func;
  struct symtab_and_line sal;
  struct thread_info *tp = inferior_thread ();
  int thread = tp->global_num;
  struct until_next_fsm *sm;

  clear_proceed_status (0);
  set_step_frame (tp);

  frame = get_current_frame ();

  /* Step until either exited from this function or greater
     than the current line (if in symbolic section) or pc (if
     not).  */

  pc = get_frame_pc (frame);
  func = find_pc_function (pc);

  if (!func)
    {
      struct bound_minimal_symbol msymbol = lookup_minimal_symbol_by_pc (pc);

      if (msymbol.minsym == nullptr)
	error (_("Execution is not within a known function."));

      tp->control.step_range_start = msymbol.value_address ();
      /* The upper-bound of step_range is exclusive.  In order to make PC
	 within the range, set the step_range_end with PC + 1.  */
      tp->control.step_range_end = pc + 1;
    }
  else
    {
      sal = find_pc_line (pc, 0);

      tp->control.step_range_start = func->value_block ()->entry_pc ();
      tp->control.step_range_end = sal.end;
    }
  tp->control.may_range_step = 1;

  tp->control.step_over_calls = STEP_OVER_ALL;

  set_longjmp_breakpoint (tp, get_frame_id (frame));
  delete_longjmp_breakpoint_cleanup lj_deleter (thread);

  sm = new until_next_fsm (command_interp (), tp->global_num);
  tp->set_thread_fsm (std::unique_ptr<thread_fsm> (sm));
  lj_deleter.release ();

  proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
}

static void
until_command (const char *arg, int from_tty)
{
  int async_exec;

  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  /* Find out whether we must run in the background.  */
  gdb::unique_xmalloc_ptr<char> stripped = strip_bg_char (arg, &async_exec);
  arg = stripped.get ();

  prepare_execution_command (current_inferior ()->top_target (), async_exec);

  if (arg)
    until_break_command (arg, from_tty, 0);
  else
    until_next_command (from_tty);
}

static void
advance_command (const char *arg, int from_tty)
{
  int async_exec;

  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  if (arg == nullptr)
    error_no_arg (_("a location"));

  /* Find out whether we must run in the background.  */
  gdb::unique_xmalloc_ptr<char> stripped = strip_bg_char (arg, &async_exec);
  arg = stripped.get ();

  prepare_execution_command (current_inferior ()->top_target (), async_exec);

  until_break_command (arg, from_tty, 1);
}

/* See inferior.h.  */

struct value *
get_return_value (struct symbol *func_symbol, struct value *function)
{
  regcache *stop_regs = get_thread_regcache (inferior_thread ());
  struct gdbarch *gdbarch = stop_regs->arch ();
  struct value *value;

  struct type *value_type
    = check_typedef (func_symbol->type ()->target_type ());
  gdb_assert (value_type->code () != TYPE_CODE_VOID);

  if (is_nocall_function (check_typedef (function->type ())))
    {
      warning (_("Function '%s' does not follow the target calling "
		 "convention, cannot determine its returned value."),
	       func_symbol->print_name ());

      return nullptr;
    }

  /* FIXME: 2003-09-27: When returning from a nested inferior function
     call, it's possible (with no help from the architecture vector)
     to locate and return/print a "struct return" value.  This is just
     a more complicated case of what is already being done in the
     inferior function call code.  In fact, when inferior function
     calls are made async, this will likely be made the norm.  */

  switch (gdbarch_return_value_as_value (gdbarch, function, value_type,
					 nullptr, nullptr, nullptr))
    {
    case RETURN_VALUE_REGISTER_CONVENTION:
    case RETURN_VALUE_ABI_RETURNS_ADDRESS:
    case RETURN_VALUE_ABI_PRESERVES_ADDRESS:
      gdbarch_return_value_as_value (gdbarch, function, value_type, stop_regs,
				     &value, nullptr);
      break;
    case RETURN_VALUE_STRUCT_CONVENTION:
      value = nullptr;
      break;
    default:
      internal_error (_("bad switch"));
    }

  return value;
}

/* The captured function return value/type and its position in the
   value history.  */

struct return_value_info
{
  /* The captured return value.  May be NULL if we weren't able to
     retrieve it.  See get_return_value.  */
  struct value *value;

  /* The return type.  In some cases, we'll not be able extract the
     return value, but we always know the type.  */
  struct type *type;

  /* If we captured a value, this is the value history index.  */
  int value_history_index;
};

/* Helper for print_return_value.  */

static void
print_return_value_1 (struct ui_out *uiout, struct return_value_info *rv)
{
  if (rv->value != nullptr)
    {
      /* Print it.  */
      uiout->text ("Value returned is ");
      uiout->field_fmt ("gdb-result-var", "$%d",
			 rv->value_history_index);
      uiout->text (" = ");

      if (finish_print)
	{
	  struct value_print_options opts;
	  get_user_print_options (&opts);

	  string_file stb;
	  value_print (rv->value, &stb, &opts);
	  uiout->field_stream ("return-value", stb);
	}
      else
	uiout->field_string ("return-value", _("<not displayed>"),
			     metadata_style.style ());
      uiout->text ("\n");
    }
  else
    {
      std::string type_name = type_to_string (rv->type);
      uiout->text ("Value returned has type: ");
      uiout->field_string ("return-type", type_name);
      uiout->text (".");
      uiout->text (" Cannot determine contents\n");
    }
}

/* Print the result of a function at the end of a 'finish' command.
   RV points at an object representing the captured return value/type
   and its position in the value history.  */

void
print_return_value (struct ui_out *uiout, struct return_value_info *rv)
{
  if (rv->type == nullptr
      || check_typedef (rv->type)->code () == TYPE_CODE_VOID)
    return;

  try
    {
      /* print_return_value_1 can throw an exception in some
	 circumstances.  We need to catch this so that we still
	 delete the breakpoint.  */
      print_return_value_1 (uiout, rv);
    }
  catch (const gdb_exception_error &ex)
    {
      exception_print (gdb_stdout, ex);
    }
}

/* Data for the FSM that manages the finish command.  */

struct finish_command_fsm : public thread_fsm
{
  /* The momentary breakpoint set at the function's return address in
     the caller.  */
  breakpoint_up breakpoint;

  /* The function that we're stepping out of.  */
  struct symbol *function = nullptr;

  /* If the FSM finishes successfully, this stores the function's
     return value.  */
  struct return_value_info return_value_info {};

  /* If the current function uses the "struct return convention",
     this holds the address at which the value being returned will
     be stored, or zero if that address could not be determined or
     the "struct return convention" is not being used.  */
  CORE_ADDR return_buf;

  explicit finish_command_fsm (struct interp *cmd_interp)
    : thread_fsm (cmd_interp)
  {
  }

  bool should_stop (struct thread_info *thread) override;
  void clean_up (struct thread_info *thread) override;
  struct return_value_info *return_value () override;
  enum async_reply_reason do_async_reply_reason () override;
};

/* Implementation of the 'should_stop' FSM method for the finish
   commands.  Detects whether the thread stepped out of the function
   successfully, and if so, captures the function's return value and
   marks the FSM finished.  */

bool
finish_command_fsm::should_stop (struct thread_info *tp)
{
  struct return_value_info *rv = &return_value_info;

  if (function != nullptr
      && bpstat_find_breakpoint (tp->control.stop_bpstat,
				 breakpoint.get ()) != nullptr)
    {
      /* We're done.  */
      set_finished ();

      rv->type = function->type ()->target_type ();
      if (rv->type == nullptr)
	internal_error (_("finish_command: function has no target type"));

      if (check_typedef (rv->type)->code () != TYPE_CODE_VOID)
	{
	  struct value *func;

	  func = read_var_value (function, nullptr, get_current_frame ());

	  if (return_buf != 0)
	    /* Retrieve return value from the buffer where it was saved.  */
	      rv->value = value_at (rv->type, return_buf);
	  else
	      rv->value = get_return_value (function, func);

	  if (rv->value != nullptr)
	    rv->value_history_index = rv->value->record_latest ();
	}
    }
  else if (tp->control.stop_step)
    {
      /* Finishing from an inline frame, or reverse finishing.  In
	 either case, there's no way to retrieve the return value.  */
      set_finished ();
    }

  return true;
}

/* Implementation of the 'clean_up' FSM method for the finish
   commands.  */

void
finish_command_fsm::clean_up (struct thread_info *thread)
{
  breakpoint.reset ();
  delete_longjmp_breakpoint (thread->global_num);
}

/* Implementation of the 'return_value' FSM method for the finish
   commands.  */

struct return_value_info *
finish_command_fsm::return_value ()
{
  return &return_value_info;
}

/* Implementation of the 'async_reply_reason' FSM method for the
   finish commands.  */

enum async_reply_reason
finish_command_fsm::do_async_reply_reason ()
{
  if (execution_direction == EXEC_REVERSE)
    return EXEC_ASYNC_END_STEPPING_RANGE;
  else
    return EXEC_ASYNC_FUNCTION_FINISHED;
}

/* finish_backward -- helper function for finish_command.  */

static void
finish_backward (struct finish_command_fsm *sm)
{
  struct symtab_and_line sal;
  struct thread_info *tp = inferior_thread ();
  CORE_ADDR pc;
  CORE_ADDR func_addr;
  CORE_ADDR alt_entry_point;
  CORE_ADDR entry_point;
  frame_info_ptr frame = get_selected_frame (nullptr);
  struct gdbarch *gdbarch = get_frame_arch (frame);

  pc = get_frame_pc (get_current_frame ());

  if (find_pc_partial_function (pc, nullptr, &func_addr, nullptr) == 0)
    error (_("Cannot find bounds of current function"));

  sal = find_pc_line (func_addr, 0);
  alt_entry_point = sal.pc;
  entry_point = alt_entry_point;

  if (gdbarch_skip_entrypoint_p (gdbarch))
    /* Some architectures, like PowerPC use local and global entry points.
       There is only one Entry Point (GEP = LEP) for other architectures.
       The GEP is an alternate entry point.  The LEP is the normal entry point.
       The value of entry_point was initialized to the alternate entry point
       (GEP).  It will be adjusted to the normal entry point if the function
       has two entry points.  */
    entry_point = gdbarch_skip_entrypoint (gdbarch, sal.pc);

  tp->control.proceed_to_finish = 1;
  /* Special case: if we're sitting at the function entry point,
     then all we need to do is take a reverse singlestep.  We
     don't need to set a breakpoint, and indeed it would do us
     no good to do so.

     Note that this can only happen at frame #0, since there's
     no way that a function up the stack can have a return address
     that's equal to its entry point.  */

  if ((pc < alt_entry_point) || (pc > entry_point))
    {
      /* We are in the body of the function.  Set a breakpoint to go back to
	 the normal entry point.  */
      symtab_and_line sr_sal;
      sr_sal.pc = entry_point;
      sr_sal.pspace = get_frame_program_space (frame);
      insert_step_resume_breakpoint_at_sal (gdbarch,
					    sr_sal, null_frame_id);

      proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
    }
  else
    {
      /* We are either at one of the entry points or between the entry points.
	 If we are not at the alt_entry point, go back to the alt_entry_point
	 If we at the normal entry point step back one instruction, when we
	 stop we will determine if we entered via the entry point or the
	 alternate entry point.  If we are at the alternate entry point,
	 single step back to the function call.  */
      tp->control.step_range_start = tp->control.step_range_end = 1;
      proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
    }
}

/* finish_forward -- helper function for finish_command.  FRAME is the
   frame that called the function we're about to step out of.  */

static void
finish_forward (struct finish_command_fsm *sm, frame_info_ptr frame)
{
  struct frame_id frame_id = get_frame_id (frame);
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct symtab_and_line sal;
  struct thread_info *tp = inferior_thread ();

  sal = find_pc_line (get_frame_pc (frame), 0);
  sal.pc = get_frame_pc (frame);

  sm->breakpoint = set_momentary_breakpoint (gdbarch, sal,
					     get_stack_frame_id (frame),
					     bp_finish);

  /* set_momentary_breakpoint invalidates FRAME.  */
  frame = nullptr;

  set_longjmp_breakpoint (tp, frame_id);

  /* We want to print return value, please...  */
  tp->control.proceed_to_finish = 1;

  proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
}

/* Skip frames for "finish".  */

static frame_info_ptr
skip_finish_frames (frame_info_ptr frame)
{
  frame_info_ptr start;

  do
    {
      start = frame;

      frame = skip_tailcall_frames (frame);
      if (frame == nullptr)
	break;

      frame = skip_unwritable_frames (frame);
      if (frame == nullptr)
	break;
    }
  while (start != frame);

  return frame;
}

/* "finish": Set a temporary breakpoint at the place the selected
   frame will return to, then continue.  */

static void
finish_command (const char *arg, int from_tty)
{
  frame_info_ptr frame;
  int async_exec;
  struct finish_command_fsm *sm;
  struct thread_info *tp;

  ERROR_NO_INFERIOR;
  ensure_not_tfind_mode ();
  ensure_valid_thread ();
  ensure_not_running ();

  /* Find out whether we must run in the background.  */
  gdb::unique_xmalloc_ptr<char> stripped = strip_bg_char (arg, &async_exec);
  arg = stripped.get ();

  prepare_execution_command (current_inferior ()->top_target (), async_exec);

  if (arg)
    error (_("The \"finish\" command does not take any arguments."));

  frame = get_prev_frame (get_selected_frame (_("No selected frame.")));
  if (frame == 0)
    error (_("\"finish\" not meaningful in the outermost frame."));

  clear_proceed_status (0);

  tp = inferior_thread ();

  sm = new finish_command_fsm (command_interp ());

  tp->set_thread_fsm (std::unique_ptr<thread_fsm> (sm));

  /* Finishing from an inline frame is completely different.  We don't
     try to show the "return value" - no way to locate it.  */
  if (get_frame_type (get_selected_frame (_("No selected frame.")))
      == INLINE_FRAME)
    {
      /* Claim we are stepping in the calling frame.  An empty step
	 range means that we will stop once we aren't in a function
	 called by that frame.  We don't use the magic "1" value for
	 step_range_end, because then infrun will think this is nexti,
	 and not step over the rest of this inlined function call.  */
      set_step_info (tp, frame, {});
      tp->control.step_range_start = get_frame_pc (frame);
      tp->control.step_range_end = tp->control.step_range_start;
      tp->control.step_over_calls = STEP_OVER_ALL;

      /* Print info on the selected frame, including level number but not
	 source.  */
      if (from_tty)
	{
	  gdb_printf (_("Run till exit from "));
	  print_stack_frame (get_selected_frame (nullptr), 1, LOCATION, 0);
	}

      proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
      return;
    }

  /* Find the function we will return from.  */
  frame_info_ptr callee_frame = get_selected_frame (nullptr);
  sm->function = find_pc_function (get_frame_pc (callee_frame));
  sm->return_buf = 0;    /* Initialize buffer address is not available.  */

  /* Determine the return convention.  If it is RETURN_VALUE_STRUCT_CONVENTION,
     attempt to determine the address of the return buffer.  */
  if (sm->function != nullptr)
    {
      enum return_value_convention return_value;
      struct gdbarch *gdbarch = get_frame_arch (callee_frame);

      struct type * val_type
	= check_typedef (sm->function->type ()->target_type ());

      return_value
	= gdbarch_return_value_as_value (gdbarch,
					 read_var_value (sm->function, nullptr,
							 callee_frame),
					 val_type, nullptr, nullptr, nullptr);

      if (return_value == RETURN_VALUE_STRUCT_CONVENTION
	  && val_type->code () != TYPE_CODE_VOID)
	sm->return_buf = gdbarch_get_return_buf_addr (gdbarch, val_type,
						      callee_frame);
    }

  /* Print info on the selected frame, including level number but not
     source.  */
  if (from_tty)
    {
      if (execution_direction == EXEC_REVERSE)
	gdb_printf (_("Run back to call of "));
      else
	{
	  if (sm->function != nullptr && TYPE_NO_RETURN (sm->function->type ())
	      && !query (_("warning: Function %s does not return normally.\n"
			   "Try to finish anyway? "),
			 sm->function->print_name ()))
	    error (_("Not confirmed."));
	  gdb_printf (_("Run till exit from "));
	}

      print_stack_frame (callee_frame, 1, LOCATION, 0);
    }

  if (execution_direction == EXEC_REVERSE)
    finish_backward (sm);
  else
    {
      frame = skip_finish_frames (frame);

      if (frame == nullptr)
	error (_("Cannot find the caller frame."));

      finish_forward (sm, frame);
    }
}


static void
info_program_command (const char *args, int from_tty)
{
  scoped_restore_current_thread restore_thread;

  thread_info *tp;

  /* In non-stop, since every thread is controlled individually, we'll
     show execution info about the current thread.  In all-stop, we'll
     show execution info about the last stop.  */

  if (non_stop)
    {
      if (!target_has_execution ())
	{
	  gdb_printf (_("The program being debugged is not being run.\n"));
	  return;
	}

      if (inferior_ptid == null_ptid)
	error (_("No selected thread."));

      tp = inferior_thread ();

      gdb_printf (_("Selected thread %s (%s).\n"),
		  print_thread_id (tp),
		  target_pid_to_str (tp->ptid).c_str ());

      if (tp->state == THREAD_EXITED)
	{
	  gdb_printf (_("Selected thread has exited.\n"));
	  return;
	}
      else if (tp->state == THREAD_RUNNING)
	{
	  gdb_printf (_("Selected thread is running.\n"));
	  return;
	}
    }
  else
    {
      tp = get_previous_thread ();

      if (tp == nullptr)
	{
	  gdb_printf (_("The program being debugged is not being run.\n"));
	  return;
	}

      switch_to_thread (tp);

      gdb_printf (_("Last stopped for thread %s (%s).\n"),
		  print_thread_id (tp),
		  target_pid_to_str (tp->ptid).c_str ());

      if (tp->state == THREAD_EXITED)
	{
	  gdb_printf (_("Thread has since exited.\n"));
	  return;
	}

      if (tp->state == THREAD_RUNNING)
	{
	  gdb_printf (_("Thread is now running.\n"));
	  return;
	}
    }

  int num;
  bpstat *bs = tp->control.stop_bpstat;
  int stat = bpstat_num (&bs, &num);

  target_files_info ();
  gdb_printf (_("Program stopped at %s.\n"),
	      paddress (current_inferior ()->arch (), tp->stop_pc ()));
  if (tp->control.stop_step)
    gdb_printf (_("It stopped after being stepped.\n"));
  else if (stat != 0)
    {
      /* There may be several breakpoints in the same place, so this
	 isn't as strange as it seems.  */
      while (stat != 0)
	{
	  if (stat < 0)
	    {
	      gdb_printf (_("It stopped at a breakpoint "
			    "that has since been deleted.\n"));
	    }
	  else
	    gdb_printf (_("It stopped at breakpoint %d.\n"), num);
	  stat = bpstat_num (&bs, &num);
	}
    }
  else if (tp->stop_signal () != GDB_SIGNAL_0)
    {
      gdb_printf (_("It stopped with signal %s, %s.\n"),
		  gdb_signal_to_name (tp->stop_signal ()),
		  gdb_signal_to_string (tp->stop_signal ()));
    }

  if (from_tty)
    {
      gdb_printf (_("Type \"info stack\" or \"info "
		    "registers\" for more information.\n"));
    }
}

static void
environment_info (const char *var, int from_tty)
{
  if (var)
    {
      const char *val = current_inferior ()->environment.get (var);

      if (val)
	{
	  gdb_puts (var);
	  gdb_puts (" = ");
	  gdb_puts (val);
	  gdb_puts ("\n");
	}
      else
	{
	  gdb_puts ("Environment variable \"");
	  gdb_puts (var);
	  gdb_puts ("\" not defined.\n");
	}
    }
  else
    {
      char **envp = current_inferior ()->environment.envp ();

      for (int idx = 0; envp[idx] != nullptr; ++idx)
	{
	  gdb_puts (envp[idx]);
	  gdb_puts ("\n");
	}
    }
}

static void
set_environment_command (const char *arg, int from_tty)
{
  const char *p, *val;
  int nullset = 0;

  if (arg == 0)
    error_no_arg (_("environment variable and value"));

  /* Find separation between variable name and value.  */
  p = (char *) strchr (arg, '=');
  val = (char *) strchr (arg, ' ');

  if (p != 0 && val != 0)
    {
      /* We have both a space and an equals.  If the space is before the
	 equals, walk forward over the spaces til we see a nonspace 
	 (possibly the equals).  */
      if (p > val)
	while (*val == ' ')
	  val++;

      /* Now if the = is after the char following the spaces,
	 take the char following the spaces.  */
      if (p > val)
	p = val - 1;
    }
  else if (val != 0 && p == 0)
    p = val;

  if (p == arg)
    error_no_arg (_("environment variable to set"));

  if (p == 0 || p[1] == 0)
    {
      nullset = 1;
      if (p == 0)
	p = arg + strlen (arg);	/* So that savestring below will work.  */
    }
  else
    {
      /* Not setting variable value to null.  */
      val = p + 1;
      while (*val == ' ' || *val == '\t')
	val++;
    }

  while (p != arg && (p[-1] == ' ' || p[-1] == '\t'))
    p--;

  std::string var (arg, p - arg);
  if (nullset)
    {
      gdb_printf (_("Setting environment variable "
		    "\"%s\" to null value.\n"),
		  var.c_str ());
      current_inferior ()->environment.set (var.c_str (), "");
    }
  else
    current_inferior ()->environment.set (var.c_str (), val);
}

static void
unset_environment_command (const char *var, int from_tty)
{
  if (var == 0)
    {
      /* If there is no argument, delete all environment variables.
	 Ask for confirmation if reading from the terminal.  */
      if (!from_tty || query (_("Delete all environment variables? ")))
	current_inferior ()->environment.clear ();
    }
  else
    current_inferior ()->environment.unset (var);
}

/* Handle the execution path (PATH variable).  */

static const char path_var_name[] = "PATH";

static void
path_info (const char *args, int from_tty)
{
  gdb_puts ("Executable and object file path: ");
  gdb_puts (current_inferior ()->environment.get (path_var_name));
  gdb_puts ("\n");
}

/* Add zero or more directories to the front of the execution path.  */

static void
path_command (const char *dirname, int from_tty)
{
  const char *env;

  dont_repeat ();
  env = current_inferior ()->environment.get (path_var_name);
  /* Can be null if path is not set.  */
  if (!env)
    env = "";
  std::string exec_path = env;
  mod_path (dirname, exec_path);
  current_inferior ()->environment.set (path_var_name, exec_path.c_str ());
  if (from_tty)
    path_info (nullptr, from_tty);
}


static void
pad_to_column (string_file &stream, int col)
{
  /* At least one space must be printed to separate columns.  */
  stream.putc (' ');
  const int size = stream.size ();
  if (size < col)
    stream.puts (n_spaces (col - size));
}

/* Print out the register NAME with value VAL, to FILE, in the default
   fashion.  */

static void
default_print_one_register_info (struct ui_file *file,
				 const char *name,
				 struct value *val)
{
  struct type *regtype = val->type ();
  int print_raw_format;
  string_file format_stream;
  enum tab_stops
    {
      value_column_1 = 15,
      /* Give enough room for "0x", 16 hex digits and two spaces in
	 preceding column.  */
      value_column_2 = value_column_1 + 2 + 16 + 2,
    };

  format_stream.puts (name);
  pad_to_column (format_stream, value_column_1);

  print_raw_format = (val->entirely_available ()
		      && !val->optimized_out ());

  /* If virtual format is floating, print it that way, and in raw
     hex.  */
  if (regtype->code () == TYPE_CODE_FLT
      || regtype->code () == TYPE_CODE_DECFLOAT)
    {
      struct value_print_options opts;
      const gdb_byte *valaddr = val->contents_for_printing ().data ();
      enum bfd_endian byte_order = type_byte_order (regtype);

      get_user_print_options (&opts);
      opts.deref_ref = true;

      common_val_print (val, &format_stream, 0, &opts, current_language);

      if (print_raw_format)
	{
	  pad_to_column (format_stream, value_column_2);
	  format_stream.puts ("(raw ");
	  print_hex_chars (&format_stream, valaddr, regtype->length (),
			   byte_order, true);
	  format_stream.putc (')');
	}
    }
  else
    {
      struct value_print_options opts;

      /* Print the register in hex.  */
      get_formatted_print_options (&opts, 'x');
      opts.deref_ref = true;
      common_val_print (val, &format_stream, 0, &opts, current_language);
      /* If not a vector register, print it also according to its
	 natural format.  */
      if (print_raw_format && regtype->is_vector () == 0)
	{
	  pad_to_column (format_stream, value_column_2);
	  get_user_print_options (&opts);
	  opts.deref_ref = true;
	  common_val_print (val, &format_stream, 0, &opts, current_language);
	}
    }

  gdb_puts (format_stream.c_str (), file);
  gdb_printf (file, "\n");
}

/* Print out the machine register regnum.  If regnum is -1, print all
   registers (print_all == 1) or all non-float and non-vector
   registers (print_all == 0).

   For most machines, having all_registers_info() print the
   register(s) one per line is good enough.  If a different format is
   required, (eg, for MIPS or Pyramid 90x, which both have lots of
   regs), or there is an existing convention for showing all the
   registers, define the architecture method PRINT_REGISTERS_INFO to
   provide that format.  */

void
default_print_registers_info (struct gdbarch *gdbarch,
			      struct ui_file *file,
			      frame_info_ptr frame,
			      int regnum, int print_all)
{
  int i;
  const int numregs = gdbarch_num_cooked_regs (gdbarch);

  for (i = 0; i < numregs; i++)
    {
      /* Decide between printing all regs, non-float / vector regs, or
	 specific reg.  */
      if (regnum == -1)
	{
	  if (print_all)
	    {
	      if (!gdbarch_register_reggroup_p (gdbarch, i, all_reggroup))
		continue;
	    }
	  else
	    {
	      if (!gdbarch_register_reggroup_p (gdbarch, i, general_reggroup))
		continue;
	    }
	}
      else
	{
	  if (i != regnum)
	    continue;
	}

      /* If the register name is empty, it is undefined for this
	 processor, so don't display anything.  */
      if (*(gdbarch_register_name (gdbarch, i)) == '\0')
	continue;

      default_print_one_register_info
	(file, gdbarch_register_name (gdbarch, i),
	 value_of_register (i, get_next_frame_sentinel_okay (frame)));
    }
}

void
registers_info (const char *addr_exp, int fpregs)
{
  frame_info_ptr frame;
  struct gdbarch *gdbarch;

  if (!target_has_registers ())
    error (_("The program has no registers now."));
  frame = get_selected_frame (nullptr);
  gdbarch = get_frame_arch (frame);

  if (!addr_exp)
    {
      gdbarch_print_registers_info (gdbarch, gdb_stdout,
				    frame, -1, fpregs);
      return;
    }

  while (*addr_exp != '\0')
    {
      const char *start;
      const char *end;

      /* Skip leading white space.  */
      addr_exp = skip_spaces (addr_exp);

      /* Discard any leading ``$''.  Check that there is something
	 resembling a register following it.  */
      if (addr_exp[0] == '$')
	addr_exp++;
      if (isspace ((*addr_exp)) || (*addr_exp) == '\0')
	error (_("Missing register name"));

      /* Find the start/end of this register name/num/group.  */
      start = addr_exp;
      while ((*addr_exp) != '\0' && !isspace ((*addr_exp)))
	addr_exp++;
      end = addr_exp;

      /* Figure out what we've found and display it.  */

      /* A register name?  */
      {
	int regnum = user_reg_map_name_to_regnum (gdbarch, start, end - start);

	if (regnum >= 0)
	  {
	    /* User registers lie completely outside of the range of
	       normal registers.  Catch them early so that the target
	       never sees them.  */
	    if (regnum >= gdbarch_num_cooked_regs (gdbarch))
	      {
		struct value *regval = value_of_user_reg (regnum, frame);
		const char *regname = user_reg_map_regnum_to_name (gdbarch,
								   regnum);

		/* Print in the same fashion
		   gdbarch_print_registers_info's default
		   implementation prints.  */
		default_print_one_register_info (gdb_stdout,
						 regname,
						 regval);
	      }
	    else
	      gdbarch_print_registers_info (gdbarch, gdb_stdout,
					    frame, regnum, fpregs);
	    continue;
	  }
      }

      /* A register group?  */
      {
	const struct reggroup *group = nullptr;
	for (const struct reggroup *g : gdbarch_reggroups (gdbarch))
	  {
	    /* Don't bother with a length check.  Should the user
	       enter a short register group name, go with the first
	       group that matches.  */
	    if (strncmp (start, g->name (), end - start) == 0)
	      {
		group = g;
		break;
	      }
	  }
	if (group != nullptr)
	  {
	    int regnum;

	    for (regnum = 0;
		 regnum < gdbarch_num_cooked_regs (gdbarch);
		 regnum++)
	      {
		if (gdbarch_register_reggroup_p (gdbarch, regnum, group))
		  gdbarch_print_registers_info (gdbarch,
						gdb_stdout, frame,
						regnum, fpregs);
	      }
	    continue;
	  }
      }

      /* Nothing matched.  */
      error (_("Invalid register `%.*s'"), (int) (end - start), start);
    }
}

static void
info_all_registers_command (const char *addr_exp, int from_tty)
{
  registers_info (addr_exp, 1);
}

static void
info_registers_command (const char *addr_exp, int from_tty)
{
  registers_info (addr_exp, 0);
}

static void
print_vector_info (struct ui_file *file,
		   frame_info_ptr frame, const char *args)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);

  if (gdbarch_print_vector_info_p (gdbarch))
    gdbarch_print_vector_info (gdbarch, file, frame, args);
  else
    {
      int regnum;
      int printed_something = 0;

      for (regnum = 0; regnum < gdbarch_num_cooked_regs (gdbarch); regnum++)
	{
	  if (gdbarch_register_reggroup_p (gdbarch, regnum, vector_reggroup))
	    {
	      printed_something = 1;
	      gdbarch_print_registers_info (gdbarch, file, frame, regnum, 1);
	    }
	}
      if (!printed_something)
	gdb_printf (file, "No vector information\n");
    }
}

static void
info_vector_command (const char *args, int from_tty)
{
  if (!target_has_registers ())
    error (_("The program has no registers now."));

  print_vector_info (gdb_stdout, get_selected_frame (nullptr), args);
}

/* Kill the inferior process.  Make us have no inferior.  */

static void
kill_command (const char *arg, int from_tty)
{
  /* FIXME:  This should not really be inferior_ptid (or target_has_execution).
     It should be a distinct flag that indicates that a target is active, cuz
     some targets don't have processes!  */

  if (inferior_ptid == null_ptid)
    error (_("The program is not being run."));
  if (!query (_("Kill the program being debugged? ")))
    error (_("Not confirmed."));

  int pid = current_inferior ()->pid;
  /* Save the pid as a string before killing the inferior, since that
     may unpush the current target, and we need the string after.  */
  std::string pid_str = target_pid_to_str (ptid_t (pid));
  int infnum = current_inferior ()->num;

  target_kill ();

  update_previous_thread ();

  if (print_inferior_events)
    gdb_printf (_("[Inferior %d (%s) killed]\n"),
		infnum, pid_str.c_str ());
}

/* Used in `attach&' command.  Proceed threads of inferior INF iff
   they stopped due to debugger request, and when they did, they
   reported a clean stop (GDB_SIGNAL_0).  Do not proceed threads that
   have been explicitly been told to stop.  */

static void
proceed_after_attach (inferior *inf)
{
  /* Don't error out if the current thread is running, because
     there may be other stopped threads.  */

  /* Backup current thread and selected frame.  */
  scoped_restore_current_thread restore_thread;

  for (thread_info *thread : inf->non_exited_threads ())
    if (!thread->executing ()
	&& !thread->stop_requested
	&& thread->stop_signal () == GDB_SIGNAL_0)
      {
	switch_to_thread (thread);
	clear_proceed_status (0);
	proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
      }
}

/* See inferior.h.  */

void
setup_inferior (int from_tty)
{
  struct inferior *inferior;

  inferior = current_inferior ();
  inferior->needs_setup = false;

  /* If no exec file is yet known, try to determine it from the
     process itself.  */
  if (get_exec_file (0) == nullptr)
    exec_file_locate_attach (inferior_ptid.pid (), 1, from_tty);
  else
    {
      reopen_exec_file ();
      reread_symbols (from_tty);
    }

  /* Take any necessary post-attaching actions for this platform.  */
  target_post_attach (inferior_ptid.pid ());

  post_create_inferior (from_tty);
}

/* What to do after the first program stops after attaching.  */
enum attach_post_wait_mode
{
  /* Do nothing.  Leaves threads as they are.  */
  ATTACH_POST_WAIT_NOTHING,

  /* Re-resume threads that are marked running.  */
  ATTACH_POST_WAIT_RESUME,

  /* Stop all threads.  */
  ATTACH_POST_WAIT_STOP,
};

/* Called after we've attached to a process and we've seen it stop for
   the first time.  Resume, stop, or don't touch the threads according
   to MODE.  */

static void
attach_post_wait (int from_tty, enum attach_post_wait_mode mode)
{
  struct inferior *inferior;

  inferior = current_inferior ();
  inferior->control.stop_soon = NO_STOP_QUIETLY;

  if (inferior->needs_setup)
    setup_inferior (from_tty);

  if (mode == ATTACH_POST_WAIT_RESUME)
    {
      /* The user requested an `attach&', so be sure to leave threads
	 that didn't get a signal running.  */

      /* Immediately resume all suspended threads of this inferior,
	 and this inferior only.  This should have no effect on
	 already running threads.  If a thread has been stopped with a
	 signal, leave it be.  */
      if (non_stop)
	proceed_after_attach (inferior);
      else
	{
	  if (inferior_thread ()->stop_signal () == GDB_SIGNAL_0)
	    {
	      clear_proceed_status (0);
	      proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
	    }
	}
    }
  else if (mode == ATTACH_POST_WAIT_STOP)
    {
      /* The user requested a plain `attach', so be sure to leave
	 the inferior stopped.  */

      /* At least the current thread is already stopped.  */

      /* In all-stop, by definition, all threads have to be already
	 stopped at this point.  In non-stop, however, although the
	 selected thread is stopped, others may still be executing.
	 Be sure to explicitly stop all threads of the process.  This
	 should have no effect on already stopped threads.  */
      if (non_stop)
	target_stop (ptid_t (inferior->pid));
      else if (target_is_non_stop_p ())
	{
	  struct thread_info *lowest = inferior_thread ();

	  stop_all_threads ("attaching");

	  /* It's not defined which thread will report the attach
	     stop.  For consistency, always select the thread with
	     lowest GDB number, which should be the main thread, if it
	     still exists.  */
	  for (thread_info *thread : current_inferior ()->non_exited_threads ())
	    if (thread->inf->num < lowest->inf->num
		|| thread->per_inf_num < lowest->per_inf_num)
	      lowest = thread;

	  switch_to_thread (lowest);
	}

      /* Tell the user/frontend where we're stopped.  */
      normal_stop ();
      if (deprecated_attach_hook)
	deprecated_attach_hook ();
    }
}

/* "attach" command entry point.  Takes a program started up outside
   of gdb and ``attaches'' to it.  This stops it cold in its tracks
   and allows us to start debugging it.  */

void
attach_command (const char *args, int from_tty)
{
  int async_exec;
  struct target_ops *attach_target;
  struct inferior *inferior = current_inferior ();
  enum attach_post_wait_mode mode;

  dont_repeat ();		/* Not for the faint of heart */

  scoped_disable_commit_resumed disable_commit_resumed ("attaching");

  if (gdbarch_has_global_solist (current_inferior ()->arch ()))
    /* Don't complain if all processes share the same symbol
       space.  */
    ;
  else if (target_has_execution ())
    {
      if (query (_("A program is being debugged already.  Kill it? ")))
	target_kill ();
      else
	error (_("Not killed."));
    }

  /* Clean up any leftovers from other runs.  Some other things from
     this function should probably be moved into target_pre_inferior.  */
  target_pre_inferior (from_tty);

  gdb::unique_xmalloc_ptr<char> stripped = strip_bg_char (args, &async_exec);
  args = stripped.get ();

  attach_target = find_attach_target ();

  prepare_execution_command (attach_target, async_exec);

  if (non_stop && !attach_target->supports_non_stop ())
    error (_("Cannot attach to this target in non-stop mode"));

  attach_target->attach (args, from_tty);
  /* to_attach should push the target, so after this point we
     shouldn't refer to attach_target again.  */
  attach_target = nullptr;

  infrun_debug_show_threads ("immediately after attach",
			     current_inferior ()->non_exited_threads ());

  /* Enable async mode if it is supported by the target.  */
  if (target_can_async_p ())
    target_async (true);

  /* Set up the "saved terminal modes" of the inferior
     based on what modes we are starting it with.  */
  target_terminal::init ();

  /* Install inferior's terminal modes.  This may look like a no-op,
     as we've just saved them above, however, this does more than
     restore terminal settings:

     - installs a SIGINT handler that forwards SIGINT to the inferior.
       Otherwise a Ctrl-C pressed just while waiting for the initial
       stop would end up as a spurious Quit.

     - removes stdin from the event loop, which we need if attaching
       in the foreground, otherwise on targets that report an initial
       stop on attach (which are most) we'd process input/commands
       while we're in the event loop waiting for that stop.  That is,
       before the attach continuation runs and the command is really
       finished.  */
  target_terminal::inferior ();

  /* Set up execution context to know that we should return from
     wait_for_inferior as soon as the target reports a stop.  */
  init_wait_for_inferior ();

  inferior->needs_setup = true;

  if (target_is_non_stop_p ())
    {
      /* If we find that the current thread isn't stopped, explicitly
	 do so now, because we're going to install breakpoints and
	 poke at memory.  */

      if (async_exec)
	/* The user requested an `attach&'; stop just one thread.  */
	target_stop (inferior_ptid);
      else
	/* The user requested an `attach', so stop all threads of this
	   inferior.  */
	target_stop (ptid_t (inferior_ptid.pid ()));
    }

  /* Check for exec file mismatch, and let the user solve it.  */
  validate_exec_file (from_tty);

  mode = async_exec ? ATTACH_POST_WAIT_RESUME : ATTACH_POST_WAIT_STOP;

  /* Some system don't generate traps when attaching to inferior.
     E.g. Mach 3 or GNU hurd.  */
  if (!target_attach_no_wait ())
    {
      /* Careful here.  See comments in inferior.h.  Basically some
	 OSes don't ignore SIGSTOPs on continue requests anymore.  We
	 need a way for handle_inferior_event to reset the stop_signal
	 variable after an attach, and this is what
	 STOP_QUIETLY_NO_SIGSTOP is for.  */
      inferior->control.stop_soon = STOP_QUIETLY_NO_SIGSTOP;

      /* Wait for stop.  */
      inferior->add_continuation ([=] ()
	{
	  attach_post_wait (from_tty, mode);
	});

      /* Let infrun consider waiting for events out of this
	 target.  */
      inferior->process_target ()->threads_executing = true;

      if (!target_is_async_p ())
	mark_infrun_async_event_handler ();
      return;
    }
  else
    attach_post_wait (from_tty, mode);

  disable_commit_resumed.reset_and_commit ();
}

/* We had just found out that the target was already attached to an
   inferior.  PTID points at a thread of this new inferior, that is
   the most likely to be stopped right now, but not necessarily so.
   The new inferior is assumed to be already added to the inferior
   list at this point.  If LEAVE_RUNNING, then leave the threads of
   this inferior running, except those we've explicitly seen reported
   as stopped.  */

void
notice_new_inferior (thread_info *thr, bool leave_running, int from_tty)
{
  enum attach_post_wait_mode mode
    = leave_running ? ATTACH_POST_WAIT_RESUME : ATTACH_POST_WAIT_NOTHING;

  std::optional<scoped_restore_current_thread> restore_thread;

  if (inferior_ptid != null_ptid)
    restore_thread.emplace ();

  /* Avoid reading registers -- we haven't fetched the target
     description yet.  */
  switch_to_thread_no_regs (thr);

  /* When we "notice" a new inferior we need to do all the things we
     would normally do if we had just attached to it.  */

  if (thr->executing ())
    {
      struct inferior *inferior = current_inferior ();

      /* We're going to install breakpoints, and poke at memory,
	 ensure that the inferior is stopped for a moment while we do
	 that.  */
      target_stop (inferior_ptid);

      inferior->control.stop_soon = STOP_QUIETLY_REMOTE;

      /* Wait for stop before proceeding.  */
      inferior->add_continuation ([=] ()
	{
	  attach_post_wait (from_tty, mode);
	});

      return;
    }

  attach_post_wait (from_tty, mode);
}

/*
 * detach_command --
 * takes a program previously attached to and detaches it.
 * The program resumes execution and will no longer stop
 * on signals, etc.  We better not have left any breakpoints
 * in the program or it'll die when it hits one.  For this
 * to work, it may be necessary for the process to have been
 * previously attached.  It *might* work if the program was
 * started via the normal ptrace (PTRACE_TRACEME).
 */

void
detach_command (const char *args, int from_tty)
{
  dont_repeat ();		/* Not for the faint of heart.  */

  if (inferior_ptid == null_ptid)
    error (_("The program is not being run."));

  scoped_disable_commit_resumed disable_commit_resumed ("detaching");

  query_if_trace_running (from_tty);

  disconnect_tracing ();

  /* Hold a strong reference to the target while (maybe)
     detaching the parent.  Otherwise detaching could close the
     target.  */
  auto target_ref
    = target_ops_ref::new_reference (current_inferior ()->process_target ());

  /* Save this before detaching, since detaching may unpush the
     process_stratum target.  */
  bool was_non_stop_p = target_is_non_stop_p ();

  target_detach (current_inferior (), from_tty);

  update_previous_thread ();

  /* The current inferior process was just detached successfully.  Get
     rid of breakpoints that no longer make sense.  Note we don't do
     this within target_detach because that is also used when
     following child forks, and in that case we will want to transfer
     breakpoints to the child, not delete them.  */
  breakpoint_init_inferior (inf_exited);

  /* If the solist is global across inferiors, don't clear it when we
     detach from a single inferior.  */
  if (!gdbarch_has_global_solist (current_inferior ()->arch ()))
    no_shared_libraries (nullptr, from_tty);

  if (deprecated_detach_hook)
    deprecated_detach_hook ();

  if (!was_non_stop_p)
    restart_after_all_stop_detach (as_process_stratum_target (target_ref.get ()));

  disable_commit_resumed.reset_and_commit ();
}

/* Disconnect from the current target without resuming it (leaving it
   waiting for a debugger).

   We'd better not have left any breakpoints in the program or the
   next debugger will get confused.  Currently only supported for some
   remote targets, since the normal attach mechanisms don't work on
   stopped processes on some native platforms (e.g. GNU/Linux).  */

static void
disconnect_command (const char *args, int from_tty)
{
  dont_repeat ();		/* Not for the faint of heart.  */
  query_if_trace_running (from_tty);
  disconnect_tracing ();
  target_disconnect (args, from_tty);
  no_shared_libraries (nullptr, from_tty);
  init_thread_list ();
  update_previous_thread ();
  if (deprecated_detach_hook)
    deprecated_detach_hook ();
}

/* Stop PTID in the current target, and tag the PTID threads as having
   been explicitly requested to stop.  PTID can be a thread, a
   process, or minus_one_ptid, meaning all threads of all inferiors of
   the current target.  */

static void
stop_current_target_threads_ns (ptid_t ptid)
{
  target_stop (ptid);

  /* Tag the thread as having been explicitly requested to stop, so
     other parts of gdb know not to resume this thread automatically,
     if it was stopped due to an internal event.  Limit this to
     non-stop mode, as when debugging a multi-threaded application in
     all-stop mode, we will only get one stop event --- it's undefined
     which thread will report the event.  */
  set_stop_requested (current_inferior ()->process_target (),
		      ptid, 1);
}

/* See inferior.h.  */

void
interrupt_target_1 (bool all_threads)
{
  scoped_disable_commit_resumed disable_commit_resumed ("interrupting");

  if (non_stop)
    {
      if (all_threads)
	{
	  scoped_restore_current_thread restore_thread;

	  for (inferior *inf : all_inferiors ())
	    {
	      switch_to_inferior_no_thread (inf);
	      stop_current_target_threads_ns (minus_one_ptid);
	    }
	}
      else
	stop_current_target_threads_ns (inferior_ptid);
    }
  else
    target_interrupt ();

  disable_commit_resumed.reset_and_commit ();
}

/* interrupt [-a]
   Stop the execution of the target while running in async mode, in
   the background.  In all-stop, stop the whole process.  In non-stop
   mode, stop the current thread only by default, or stop all threads
   if the `-a' switch is used.  */

static void
interrupt_command (const char *args, int from_tty)
{
  if (target_can_async_p ())
    {
      int all_threads = 0;

      dont_repeat ();		/* Not for the faint of heart.  */

      if (args != nullptr
	  && startswith (args, "-a"))
	all_threads = 1;

      interrupt_target_1 (all_threads);
    }
}

/* See inferior.h.  */

void
default_print_float_info (struct gdbarch *gdbarch, struct ui_file *file,
			  frame_info_ptr frame, const char *args)
{
  int regnum;
  int printed_something = 0;

  for (regnum = 0; regnum < gdbarch_num_cooked_regs (gdbarch); regnum++)
    {
      if (gdbarch_register_reggroup_p (gdbarch, regnum, float_reggroup))
	{
	  printed_something = 1;
	  gdbarch_print_registers_info (gdbarch, file, frame, regnum, 1);
	}
    }
  if (!printed_something)
    gdb_printf (file, "No floating-point info "
		"available for this processor.\n");
}

static void
info_float_command (const char *args, int from_tty)
{
  frame_info_ptr frame;

  if (!target_has_registers ())
    error (_("The program has no registers now."));

  frame = get_selected_frame (nullptr);
  gdbarch_print_float_info (get_frame_arch (frame), gdb_stdout, frame, args);
}

/* Implement `info proc' family of commands.  */

static void
info_proc_cmd_1 (const char *args, enum info_proc_what what, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();

  if (!target_info_proc (args, what))
    {
      if (gdbarch_info_proc_p (gdbarch))
	gdbarch_info_proc (gdbarch, args, what);
      else
	error (_("Not supported on this target."));
    }
}

/* Implement `info proc' when given without any further parameters.  */

static void
info_proc_cmd (const char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_MINIMAL, from_tty);
}

/* Implement `info proc mappings'.  */

static void
info_proc_cmd_mappings (const char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_MAPPINGS, from_tty);
}

/* Implement `info proc stat'.  */

static void
info_proc_cmd_stat (const char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_STAT, from_tty);
}

/* Implement `info proc status'.  */

static void
info_proc_cmd_status (const char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_STATUS, from_tty);
}

/* Implement `info proc cwd'.  */

static void
info_proc_cmd_cwd (const char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_CWD, from_tty);
}

/* Implement `info proc cmdline'.  */

static void
info_proc_cmd_cmdline (const char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_CMDLINE, from_tty);
}

/* Implement `info proc exe'.  */

static void
info_proc_cmd_exe (const char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_EXE, from_tty);
}

/* Implement `info proc files'.  */

static void
info_proc_cmd_files (const char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_FILES, from_tty);
}

/* Implement `info proc all'.  */

static void
info_proc_cmd_all (const char *args, int from_tty)
{
  info_proc_cmd_1 (args, IP_ALL, from_tty);
}

/* Implement `show print finish'.  */

static void
show_print_finish (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c,
		   const char *value)
{
  gdb_printf (file, _("\
Printing of return value after `finish' is %s.\n"),
	      value);
}


/* This help string is used for the run, start, and starti commands.
   It is defined as a macro to prevent duplication.  */

#define RUN_ARGS_HELP \
"You may specify arguments to give it.\n\
Args may include \"*\", or \"[...]\"; they are expanded using the\n\
shell that will start the program (specified by the \"$SHELL\" environment\n\
variable).  Input and output redirection with \">\", \"<\", or \">>\"\n\
are also allowed.\n\
\n\
With no arguments, uses arguments last specified (with \"run\" or \n\
\"set args\").  To cancel previous arguments and run with no arguments,\n\
use \"set args\" without arguments.\n\
\n\
To start the inferior without using a shell, use \"set startup-with-shell off\"."

void _initialize_infcmd ();
void
_initialize_infcmd ()
{
  static struct cmd_list_element *info_proc_cmdlist;
  struct cmd_list_element *c = nullptr;

  /* Add the filename of the terminal connected to inferior I/O.  */
  auto tty_set_show
    = add_setshow_optional_filename_cmd ("inferior-tty", class_run, _("\
Set terminal for future runs of program being debugged."), _("		\
Show terminal for future runs of program being debugged."), _("		\
Usage: set inferior-tty [TTY]\n\n					\
If TTY is omitted, the default behavior of using the same terminal as GDB\n \
is restored."),
					 set_tty_value,
					 get_tty_value,
					 show_inferior_tty_command,
					 &setlist, &showlist);
  add_alias_cmd ("tty", tty_set_show.set, class_run, 0, &cmdlist);

  auto args_set_show
    = add_setshow_string_noescape_cmd ("args", class_run, _("\
Set argument list to give program being debugged when it is started."), _("\
Show argument list to give program being debugged when it is started."), _("\
Follow this command with any number of args, to be passed to the program."),
				       set_args_value,
				       get_args_value,
				       show_args_command,
				       &setlist, &showlist);
  set_cmd_completer (args_set_show.set, filename_completer);

  auto cwd_set_show
    = add_setshow_string_noescape_cmd ("cwd", class_run, _("\
Set the current working directory to be used when the inferior is started.\n \
Changing this setting does not have any effect on inferiors that are\n	\
already running."),
				       _("\
Show the current working directory that is used when the inferior is started."),
				       _("\
Use this command to change the current working directory that will be used\n\
when the inferior is started.  This setting does not affect GDB's current\n\
working directory."),
				       set_cwd_value, get_inferior_cwd,
				       show_cwd_command,
				       &setlist, &showlist);
  set_cmd_completer (cwd_set_show.set, filename_completer);

  c = add_cmd ("environment", no_class, environment_info, _("\
The environment to give the program, or one variable's value.\n\
With an argument VAR, prints the value of environment variable VAR to\n\
give the program being debugged.  With no arguments, prints the entire\n\
environment to be given to the program."), &showlist);
  set_cmd_completer (c, noop_completer);

  add_basic_prefix_cmd ("unset", no_class,
			_("Complement to certain \"set\" commands."),
			&unsetlist, 0, &cmdlist);

  c = add_cmd ("environment", class_run, unset_environment_command, _("\
Cancel environment variable VAR for the program.\n\
This does not affect the program until the next \"run\" command."),
	       &unsetlist);
  set_cmd_completer (c, noop_completer);

  c = add_cmd ("environment", class_run, set_environment_command, _("\
Set environment variable value to give the program.\n\
Arguments are VAR VALUE where VAR is variable name and VALUE is value.\n\
VALUES of environment variables are uninterpreted strings.\n\
This does not affect the program until the next \"run\" command."),
	       &setlist);
  set_cmd_completer (c, noop_completer);

  c = add_com ("path", class_files, path_command, _("\
Add directory DIR(s) to beginning of search path for object files.\n\
$cwd in the path means the current working directory.\n\
This path is equivalent to the $PATH shell variable.  It is a list of\n\
directories, separated by colons.  These directories are searched to find\n\
fully linked executable files and separately compiled object files as \
needed."));
  set_cmd_completer (c, filename_completer);

  c = add_cmd ("paths", no_class, path_info, _("\
Current search path for finding object files.\n\
$cwd in the path means the current working directory.\n\
This path is equivalent to the $PATH shell variable.  It is a list of\n\
directories, separated by colons.  These directories are searched to find\n\
fully linked executable files and separately compiled object files as \
needed."),
	       &showlist);
  set_cmd_completer (c, noop_completer);

  add_prefix_cmd ("kill", class_run, kill_command,
		  _("Kill execution of program being debugged."),
		  &killlist, 0, &cmdlist);

  add_com ("attach", class_run, attach_command, _("\
Attach to a process or file outside of GDB.\n\
This command attaches to another target, of the same type as your last\n\
\"target\" command (\"info files\" will show your target stack).\n\
The command may take as argument a process id or a device file.\n\
For a process id, you must have permission to send the process a signal,\n\
and it must have the same effective uid as the debugger.\n\
When using \"attach\" with a process id, the debugger finds the\n\
program running in the process, looking first in the current working\n\
directory, or (if not found there) using the source file search path\n\
(see the \"directory\" command).  You can also use the \"file\" command\n\
to specify the program, and to load its symbol table."));

  add_prefix_cmd ("detach", class_run, detach_command, _("\
Detach a process or file previously attached.\n\
If a process, it is no longer traced, and it continues its execution.  If\n\
you were debugging a file, the file is closed and gdb no longer accesses it."),
		  &detachlist, 0, &cmdlist);

  add_com ("disconnect", class_run, disconnect_command, _("\
Disconnect from a target.\n\
The target will wait for another debugger to connect.  Not available for\n\
all targets."));

  c = add_com ("signal", class_run, signal_command, _("\
Continue program with the specified signal.\n\
Usage: signal SIGNAL\n\
The SIGNAL argument is processed the same as the handle command.\n\
\n\
An argument of \"0\" means continue the program without sending it a signal.\n\
This is useful in cases where the program stopped because of a signal,\n\
and you want to resume the program while discarding the signal.\n\
\n\
In a multi-threaded program the signal is delivered to, or discarded from,\n\
the current thread only."));
  set_cmd_completer (c, signal_completer);

  c = add_com ("queue-signal", class_run, queue_signal_command, _("\
Queue a signal to be delivered to the current thread when it is resumed.\n\
Usage: queue-signal SIGNAL\n\
The SIGNAL argument is processed the same as the handle command.\n\
It is an error if the handling state of SIGNAL is \"nopass\".\n\
\n\
An argument of \"0\" means remove any currently queued signal from\n\
the current thread.  This is useful in cases where the program stopped\n\
because of a signal, and you want to resume it while discarding the signal.\n\
\n\
In a multi-threaded program the signal is queued with, or discarded from,\n\
the current thread only."));
  set_cmd_completer (c, signal_completer);

  cmd_list_element *stepi_cmd
    = add_com ("stepi", class_run, stepi_command, _("\
Step one instruction exactly.\n\
Usage: stepi [N]\n\
Argument N means step N times (or till program stops for another \
reason)."));
  add_com_alias ("si", stepi_cmd, class_run, 0);

  cmd_list_element *nexti_cmd
   = add_com ("nexti", class_run, nexti_command, _("\
Step one instruction, but proceed through subroutine calls.\n\
Usage: nexti [N]\n\
Argument N means step N times (or till program stops for another \
reason)."));
  add_com_alias ("ni", nexti_cmd, class_run, 0);

  cmd_list_element *finish_cmd
    = add_com ("finish", class_run, finish_command, _("\
Execute until selected stack frame returns.\n\
Usage: finish\n\
Upon return, the value returned is printed and put in the value history."));
  add_com_alias ("fin", finish_cmd, class_run, 1);

  cmd_list_element *next_cmd
    = add_com ("next", class_run, next_command, _("\
Step program, proceeding through subroutine calls.\n\
Usage: next [N]\n\
Unlike \"step\", if the current source line calls a subroutine,\n\
this command does not enter the subroutine, but instead steps over\n\
the call, in effect treating it as a single source line."));
  add_com_alias ("n", next_cmd, class_run, 1);

  cmd_list_element *step_cmd
    = add_com ("step", class_run, step_command, _("\
Step program until it reaches a different source line.\n\
Usage: step [N]\n\
Argument N means step N times (or till program stops for another \
reason)."));
  add_com_alias ("s", step_cmd, class_run, 1);

  cmd_list_element *until_cmd
    = add_com ("until", class_run, until_command, _("\
Execute until past the current line or past a LOCATION.\n\
Execute until the program reaches a source line greater than the current\n\
or a specified location (same args as break command) within the current \
frame."));
  set_cmd_completer (until_cmd, location_completer);
  add_com_alias ("u", until_cmd, class_run, 1);

  c = add_com ("advance", class_run, advance_command, _("\
Continue the program up to the given location (same form as args for break \
command).\n\
Execution will also stop upon exit from the current stack frame."));
  set_cmd_completer (c, location_completer);

  cmd_list_element *jump_cmd
    = add_com ("jump", class_run, jump_command, _("\
Continue program being debugged at specified line or address.\n\
Usage: jump LOCATION\n\
Give as argument either LINENUM or *ADDR, where ADDR is an expression\n\
for an address to start at."));
  set_cmd_completer (jump_cmd, location_completer);
  add_com_alias ("j", jump_cmd, class_run, 1);

  cmd_list_element *continue_cmd
    = add_com ("continue", class_run, continue_command, _("\
Continue program being debugged, after signal or breakpoint.\n\
Usage: continue [N]\n\
If proceeding from breakpoint, a number N may be used as an argument,\n\
which means to set the ignore count of that breakpoint to N - 1 (so that\n\
the breakpoint won't break until the Nth time it is reached).\n\
\n\
If non-stop mode is enabled, continue only the current thread,\n\
otherwise all the threads in the program are continued.  To \n\
continue all stopped threads in non-stop mode, use the -a option.\n\
Specifying -a and an ignore count simultaneously is an error."));
  add_com_alias ("c", continue_cmd, class_run, 1);
  add_com_alias ("fg", continue_cmd, class_run, 1);

  cmd_list_element *run_cmd
    = add_com ("run", class_run, run_command, _("\
Start debugged program.\n"
RUN_ARGS_HELP));
  set_cmd_completer (run_cmd, filename_completer);
  add_com_alias ("r", run_cmd, class_run, 1);

  c = add_com ("start", class_run, start_command, _("\
Start the debugged program stopping at the beginning of the main procedure.\n"
RUN_ARGS_HELP));
  set_cmd_completer (c, filename_completer);

  c = add_com ("starti", class_run, starti_command, _("\
Start the debugged program stopping at the first instruction.\n"
RUN_ARGS_HELP));
  set_cmd_completer (c, filename_completer);

  add_com ("interrupt", class_run, interrupt_command,
	   _("Interrupt the execution of the debugged program.\n\
If non-stop mode is enabled, interrupt only the current thread,\n\
otherwise all the threads in the program are stopped.  To \n\
interrupt all running threads in non-stop mode, use the -a option."));

  cmd_list_element *info_registers_cmd
    = add_info ("registers", info_registers_command, _("\
List of integer registers and their contents, for selected stack frame.\n\
One or more register names as argument means describe the given registers.\n\
One or more register group names as argument means describe the registers\n\
in the named register groups."));
  add_info_alias ("r", info_registers_cmd, 1);
  set_cmd_completer (info_registers_cmd, reg_or_group_completer);

  c = add_info ("all-registers", info_all_registers_command, _("\
List of all registers and their contents, for selected stack frame.\n\
One or more register names as argument means describe the given registers.\n\
One or more register group names as argument means describe the registers\n\
in the named register groups."));
  set_cmd_completer (c, reg_or_group_completer);

  add_info ("program", info_program_command,
	    _("Execution status of the program."));

  add_info ("float", info_float_command,
	    _("Print the status of the floating point unit."));

  add_info ("vector", info_vector_command,
	    _("Print the status of the vector unit."));

  add_prefix_cmd ("proc", class_info, info_proc_cmd,
		  _("\
Show additional information about a process.\n\
Specify any process id, or use the program being debugged by default."),
		  &info_proc_cmdlist,
		  1/*allow-unknown*/, &infolist);

  add_cmd ("mappings", class_info, info_proc_cmd_mappings, _("\
List memory regions mapped by the specified process."),
	   &info_proc_cmdlist);

  add_cmd ("stat", class_info, info_proc_cmd_stat, _("\
List process info from /proc/PID/stat."),
	   &info_proc_cmdlist);

  add_cmd ("status", class_info, info_proc_cmd_status, _("\
List process info from /proc/PID/status."),
	   &info_proc_cmdlist);

  add_cmd ("cwd", class_info, info_proc_cmd_cwd, _("\
List current working directory of the specified process."),
	   &info_proc_cmdlist);

  add_cmd ("cmdline", class_info, info_proc_cmd_cmdline, _("\
List command line arguments of the specified process."),
	   &info_proc_cmdlist);

  add_cmd ("exe", class_info, info_proc_cmd_exe, _("\
List absolute filename for executable of the specified process."),
	   &info_proc_cmdlist);

  add_cmd ("files", class_info, info_proc_cmd_files, _("\
List files opened by the specified process."),
	   &info_proc_cmdlist);

  add_cmd ("all", class_info, info_proc_cmd_all, _("\
List all available info about the specified process."),
	   &info_proc_cmdlist);

  add_setshow_boolean_cmd ("finish", class_support,
			   &finish_print, _("\
Set whether `finish' prints the return value."), _("\
Show whether `finish' prints the return value."), nullptr,
			   nullptr,
			   show_print_finish,
			   &setprintlist, &showprintlist);
}
