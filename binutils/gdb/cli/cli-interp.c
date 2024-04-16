/* CLI Definitions for GDB, the GNU debugger.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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
#include "cli-interp.h"
#include "interps.h"
#include "event-top.h"
#include "ui-out.h"
#include "cli-out.h"
#include "top.h"
#include "ui.h"
#include "infrun.h"
#include "observable.h"
#include "gdbthread.h"
#include "thread-fsm.h"
#include "inferior.h"

cli_interp_base::cli_interp_base (const char *name)
  : interp (name)
{}

cli_interp_base::~cli_interp_base ()
{}

/* The console interpreter.  */

class cli_interp final : public cli_interp_base
{
 public:
  explicit cli_interp (const char *name);
  ~cli_interp () = default;

  void init (bool top_level) override;
  void resume () override;
  void suspend () override;
  void exec (const char *command_str) override;
  ui_out *interp_ui_out () override;

private:

  /* The ui_out for the console interpreter.  */
  std::unique_ptr<cli_ui_out> m_cli_uiout;
};

cli_interp::cli_interp (const char *name)
  : cli_interp_base (name),
    m_cli_uiout (new cli_ui_out (gdb_stdout))
{
}

/* Suppress notification struct.  */
struct cli_suppress_notification cli_suppress_notification;

/* See cli-interp.h.

   Breakpoint hits should always be mirrored to a console.  Deciding
   what to mirror to a console wrt to breakpoints and random stops
   gets messy real fast.  E.g., say "s" trips on a breakpoint.  We'd
   clearly want to mirror the event to the console in this case.  But
   what about more complicated cases like "s&; thread n; s&", and one
   of those steps spawning a new thread, and that thread hitting a
   breakpoint?  It's impossible in general to track whether the thread
   had any relation to the commands that had been executed.  So we
   just simplify and always mirror breakpoints and random events to
   all consoles.

   OTOH, we should print the source line to the console when stepping
   or other similar commands, iff the step was started by that console
   (or in MI's case, by a console command), but not if it was started
   with MI's -exec-step or similar.  */

int
should_print_stop_to_console (struct interp *console_interp,
			      struct thread_info *tp)
{
  if ((bpstat_what (tp->control.stop_bpstat).main_action
       == BPSTAT_WHAT_STOP_NOISY)
      || tp->thread_fsm () == nullptr
      || tp->thread_fsm ()->command_interp == console_interp
      || !tp->thread_fsm ()->finished_p ())
    return 1;
  return 0;
}

/* Observers for several run control events.  If the interpreter is
   quiet (i.e., another interpreter is being run with
   interpreter-exec), print nothing.  These are named "cli_base" as
   they print to both CLI interpreters and TUI interpreters.  */

void
cli_interp_base::on_normal_stop (struct bpstat *bs, int print_frame)
{
  if (!print_frame)
    return;

  /* This event is suppressed.  */
  if (cli_suppress_notification.normal_stop)
    return;

  thread_info *thread = inferior_thread ();
  if (should_print_stop_to_console (this, thread))
    print_stop_event (this->interp_ui_out ());

}

void
cli_interp_base::on_signal_received (enum gdb_signal siggnal)
{
  print_signal_received_reason (this->interp_ui_out (), siggnal);
}

void
cli_interp_base::on_signal_exited (gdb_signal sig)
{
  print_signal_exited_reason (this->interp_ui_out (), sig);
}

void
cli_interp_base::on_exited (int status)
{
  print_exited_reason (this->interp_ui_out (), status);
}

void
cli_interp_base::on_no_history ()
{
  print_no_history_reason (this->interp_ui_out ());
}

void
cli_interp_base::on_sync_execution_done ()
{
  display_gdb_prompt (NULL);
}

void
cli_interp_base::on_command_error ()
{
  display_gdb_prompt (NULL);
}

void
cli_interp_base::on_user_selected_context_changed (user_selected_what selection)
{
  /* This event is suppressed.  */
  if (cli_suppress_notification.user_selected_context)
    return;

  thread_info *tp = inferior_ptid != null_ptid ? inferior_thread () : nullptr;

  if (selection & USER_SELECTED_INFERIOR)
    print_selected_inferior (this->interp_ui_out ());

  if (tp != nullptr
      && ((selection & (USER_SELECTED_THREAD | USER_SELECTED_FRAME))))
    print_selected_thread_frame (this->interp_ui_out (), selection);
}

/* pre_command_loop implementation.  */

void
cli_interp_base::pre_command_loop ()
{
  display_gdb_prompt (0);
}

/* These implement the cli out interpreter: */

void
cli_interp::init (bool top_level)
{
}

void
cli_interp::resume ()
{
  struct ui *ui = current_ui;
  struct ui_file *stream;

  /*sync_execution = 1; */

  /* gdb_setup_readline will change gdb_stdout.  If the CLI was
     previously writing to gdb_stdout, then set it to the new
     gdb_stdout afterwards.  */

  stream = m_cli_uiout->set_stream (gdb_stdout);
  if (stream != gdb_stdout)
    {
      m_cli_uiout->set_stream (stream);
      stream = NULL;
    }

  gdb_setup_readline (1);

  ui->input_handler = command_line_handler;

  if (stream != NULL)
    m_cli_uiout->set_stream (gdb_stdout);
}

void
cli_interp::suspend ()
{
  gdb_disable_readline ();
}

void
cli_interp::exec (const char *command_str)
{
  /* gdb_stdout could change between the time m_cli_uiout was
     initialized and now.  Since we're probably using a different
     interpreter which has a new ui_file for gdb_stdout, use that one
     instead of the default.

     It is important that it gets reset everytime, since the user
     could set gdb to use a different interpreter.  */
  ui_file *old_stream = m_cli_uiout->set_stream (gdb_stdout);
  SCOPE_EXIT { m_cli_uiout->set_stream (old_stream); };

  /* Save and override the global ``struct ui_out'' builder.  */
  scoped_restore saved_uiout = make_scoped_restore (&current_uiout,
						    m_cli_uiout.get ());

  try
    {
      execute_command (command_str, 1);
    }
  catch (const gdb_exception_error &ex)
    {
      exception_print (gdb_stderr, ex);
      throw;
    }
}

bool
cli_interp_base::supports_command_editing ()
{
  return true;
}

ui_out *
cli_interp::interp_ui_out ()
{
  return m_cli_uiout.get ();
}

/* See cli-interp.h.  */

void
cli_interp_base::set_logging (ui_file_up logfile, bool logging_redirect,
			      bool debug_redirect)
{
  if (logfile != nullptr)
    {
      gdb_assert (m_saved_output == nullptr);
      m_saved_output.reset (new saved_output_files);
      m_saved_output->out = gdb_stdout;
      m_saved_output->err = gdb_stderr;
      m_saved_output->log = gdb_stdlog;
      m_saved_output->targ = gdb_stdtarg;
      m_saved_output->targerr = gdb_stdtargerr;

      ui_file *logfile_p = logfile.get ();
      m_saved_output->logfile_holder = std::move (logfile);

      /* The new stdout and stderr only depend on whether logging
	 redirection is being done.  */
      ui_file *new_stdout = logfile_p;
      ui_file *new_stderr = logfile_p;
      if (!logging_redirect)
	{
	  m_saved_output->stdout_holder.reset
	    (new tee_file (gdb_stdout, logfile_p));
	  new_stdout = m_saved_output->stdout_holder.get ();
	  m_saved_output->stderr_holder.reset
	    (new tee_file (gdb_stderr, logfile_p));
	  new_stderr = m_saved_output->stderr_holder.get ();
	}

      m_saved_output->stdlog_holder.reset
	(new timestamped_file (debug_redirect ? logfile_p : new_stderr));

      gdb_stdout = new_stdout;
      gdb_stdlog = m_saved_output->stdlog_holder.get ();
      gdb_stderr = new_stderr;
      gdb_stdtarg = new_stderr;
      gdb_stdtargerr = new_stderr;
    }
  else
    {
      gdb_stdout = m_saved_output->out;
      gdb_stderr = m_saved_output->err;
      gdb_stdlog = m_saved_output->log;
      gdb_stdtarg = m_saved_output->targ;
      gdb_stdtargerr = m_saved_output->targerr;

      m_saved_output.reset (nullptr);
    }
}

/* Factory for CLI interpreters.  */

static struct interp *
cli_interp_factory (const char *name)
{
  return new cli_interp (name);
}

/* Standard gdb initialization hook.  */

void _initialize_cli_interp ();
void
_initialize_cli_interp ()
{
  interp_factory_register (INTERP_CONSOLE, cli_interp_factory);
}
