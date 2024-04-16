/* Top level stuff for GDB, the GNU debugger.

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

   Written by Elena Zannoni <ezannoni@cygnus.com> of Cygnus Solutions.

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
#include "top.h"
#include "ui.h"
#include "inferior.h"
#include "infrun.h"
#include "target.h"
#include "terminal.h"
#include "gdbsupport/event-loop.h"
#include "event-top.h"
#include "interps.h"
#include <signal.h>
#include "cli/cli-script.h"
#include "main.h"
#include "gdbthread.h"
#include "observable.h"
#include "gdbcmd.h"
#include "annotate.h"
#include "maint.h"
#include "ser-event.h"
#include "gdbsupport/gdb_select.h"
#include "gdbsupport/gdb-sigmask.h"
#include "async-event.h"
#include "bt-utils.h"
#include "pager.h"

/* readline include files.  */
#include "readline/readline.h"
#include "readline/history.h"

#ifdef TUI
#include "tui/tui.h"
#endif

/* readline defines this.  */
#undef savestring

static std::string top_level_prompt ();

/* Signal handlers.  */
#ifdef SIGQUIT
static void handle_sigquit (int sig);
#endif
#ifdef SIGHUP
static void handle_sighup (int sig);
#endif

/* Functions to be invoked by the event loop in response to
   signals.  */
#if defined (SIGQUIT) || defined (SIGHUP)
static void async_do_nothing (gdb_client_data);
#endif
#ifdef SIGHUP
static void async_disconnect (gdb_client_data);
#endif
#ifdef SIGTSTP
static void async_sigtstp_handler (gdb_client_data);
#endif
static void async_sigterm_handler (gdb_client_data arg);

/* Instead of invoking (and waiting for) readline to read the command
   line and pass it back for processing, we use readline's alternate
   interface, via callback functions, so that the event loop can react
   to other event sources while we wait for input.  */

/* Important variables for the event loop.  */

/* This is used to determine if GDB is using the readline library or
   its own simplified form of readline.  It is used by the asynchronous
   form of the set editing command.
   ezannoni: as of 1999-04-29 I expect that this
   variable will not be used after gdb is changed to use the event
   loop as default engine, and event-top.c is merged into top.c.  */
bool set_editing_cmd_var;

/* This is used to display the notification of the completion of an
   asynchronous execution command.  */
bool exec_done_display_p = false;

/* Used by the stdin event handler to compensate for missed stdin events.
   Setting this to a non-zero value inside an stdin callback makes the callback
   run again.  */
int call_stdin_event_handler_again_p;

/* When true GDB will produce a minimal backtrace when a fatal signal is
   reached (within GDB code).  */
static bool bt_on_fatal_signal = GDB_PRINT_INTERNAL_BACKTRACE_INIT_ON;

/* Implement 'maintenance show backtrace-on-fatal-signal'.  */

static void
show_bt_on_fatal_signal (struct ui_file *file, int from_tty,
			 struct cmd_list_element *cmd, const char *value)
{
  gdb_printf (file, _("Backtrace on a fatal signal is %s.\n"), value);
}

/* Signal handling variables.  */
/* Each of these is a pointer to a function that the event loop will
   invoke if the corresponding signal has received.  The real signal
   handlers mark these functions as ready to be executed and the event
   loop, in a later iteration, calls them.  See the function
   invoke_async_signal_handler.  */
static struct async_signal_handler *sigint_token;
#ifdef SIGHUP
static struct async_signal_handler *sighup_token;
#endif
#ifdef SIGQUIT
static struct async_signal_handler *sigquit_token;
#endif
#ifdef SIGTSTP
static struct async_signal_handler *sigtstp_token;
#endif
static struct async_signal_handler *async_sigterm_token;

/* This hook is called by gdb_rl_callback_read_char_wrapper after each
   character is processed.  */
void (*after_char_processing_hook) (void);

#if RL_VERSION_MAJOR == 7
extern "C" void _rl_signal_handler (int);
#endif

/* Wrapper function for calling into the readline library.  This takes
   care of a couple things:

   - The event loop expects the callback function to have a parameter,
     while readline expects none.

   - Propagation of GDB exceptions/errors thrown from INPUT_HANDLER
     across readline requires special handling.

   On the exceptions issue:

   DWARF-based unwinding cannot cross code built without -fexceptions.
   Any exception that tries to propagate through such code will fail
   and the result is a call to std::terminate.  While some ABIs, such
   as x86-64, require all code to be built with exception tables,
   others don't.

   This is a problem when GDB calls some non-EH-aware C library code,
   that calls into GDB again through a callback, and that GDB callback
   code throws a C++ exception.  Turns out this is exactly what
   happens with GDB's readline callback.

   In such cases, we must catch and save any C++ exception that might
   be thrown from the GDB callback before returning to the
   non-EH-aware code.  When the non-EH-aware function itself returns
   back to GDB, we then rethrow the original C++ exception.

   In the readline case however, the right thing to do is to longjmp
   out of the callback, rather than do a normal return -- there's no
   way for the callback to return to readline an indication that an
   error happened, so a normal return would have rl_callback_read_char
   potentially continue processing further input, redisplay the
   prompt, etc.  Instead of raw setjmp/longjmp however, we use our
   sjlj-based TRY/CATCH mechanism, which knows to handle multiple
   levels of active setjmp/longjmp frames, needed in order to handle
   the readline callback recursing, as happens with e.g., secondary
   prompts / queries, through gdb_readline_wrapper.  This must be
   noexcept in order to avoid problems with mixing sjlj and
   (sjlj-based) C++ exceptions.  */

static struct gdb_exception
gdb_rl_callback_read_char_wrapper_noexcept () noexcept
{
  struct gdb_exception gdb_expt;

  /* C++ exceptions can't normally be thrown across readline (unless
     it is built with -fexceptions, but it won't by default on many
     ABIs).  So we instead wrap the readline call with a sjlj-based
     TRY/CATCH, and rethrow the GDB exception once back in GDB.  */
  TRY_SJLJ
    {
      rl_callback_read_char ();
#if RL_VERSION_MAJOR >= 8
      /* It can happen that readline (while in rl_callback_read_char)
	 received a signal, but didn't handle it yet.  Make sure it's handled
	 now.  If we don't do that we run into two related problems:
	 - we have to wait for another event triggering
	   rl_callback_read_char before the signal is handled
	 - there's no guarantee that the signal will be processed before the
	   event.  */
      while (rl_pending_signal () != 0)
	/* Do this in a while loop, in case rl_check_signals also leaves a
	   pending signal.  I'm not sure if that's possible, but it seems
	   better to handle the scenario than to assert.  */
	rl_check_signals ();
#elif RL_VERSION_MAJOR == 7
      /* Unfortunately, rl_check_signals is not available.  Use private
	 function _rl_signal_handler instead.  */

      while (rl_pending_signal () != 0)
	_rl_signal_handler (rl_pending_signal ());
#else
#error "Readline major version >= 7 expected"
#endif
      if (after_char_processing_hook)
	(*after_char_processing_hook) ();
    }
  CATCH_SJLJ (ex, RETURN_MASK_ALL)
    {
      gdb_expt = std::move (ex);
    }
  END_CATCH_SJLJ

  return gdb_expt;
}

static void
gdb_rl_callback_read_char_wrapper (gdb_client_data client_data)
{
  struct gdb_exception gdb_expt
    = gdb_rl_callback_read_char_wrapper_noexcept ();

  /* Rethrow using the normal EH mechanism.  */
  if (gdb_expt.reason < 0)
    throw_exception (std::move (gdb_expt));
}

/* GDB's readline callback handler.  Calls the current INPUT_HANDLER,
   and propagates GDB exceptions/errors thrown from INPUT_HANDLER back
   across readline.  See gdb_rl_callback_read_char_wrapper.  This must
   be noexcept in order to avoid problems with mixing sjlj and
   (sjlj-based) C++ exceptions.  */

static void
gdb_rl_callback_handler (char *rl) noexcept
{
  /* This is static to avoid undefined behavior when calling longjmp
     -- gdb_exception has a destructor with side effects.  */
  static struct gdb_exception gdb_rl_expt;
  struct ui *ui = current_ui;

  try
    {
      /* Ensure the exception is reset on each call.  */
      gdb_rl_expt = {};
      ui->input_handler (gdb::unique_xmalloc_ptr<char> (rl));
    }
  catch (gdb_exception &ex)
    {
      gdb_rl_expt = std::move (ex);
    }

  /* If we caught a GDB exception, longjmp out of the readline
     callback.  There's no other way for the callback to signal to
     readline that an error happened.  A normal return would have
     readline potentially continue processing further input, redisplay
     the prompt, etc.  (This is what GDB historically did when it was
     a C program.)  Note that since we're long jumping, local variable
     dtors are NOT run automatically.  */
  if (gdb_rl_expt.reason < 0)
    throw_exception_sjlj (gdb_rl_expt);
}

/* Change the function to be invoked every time there is a character
   ready on stdin.  This is used when the user sets the editing off,
   therefore bypassing readline, and letting gdb handle the input
   itself, via gdb_readline_no_editing_callback.  Also it is used in
   the opposite case in which the user sets editing on again, by
   restoring readline handling of the input.

   NOTE: this operates on input_fd, not instream.  If we are reading
   commands from a file, instream will point to the file.  However, we
   always read commands from a file with editing off.  This means that
   the 'set editing on/off' will have effect only on the interactive
   session.  */

void
change_line_handler (int editing)
{
  struct ui *ui = current_ui;

  /* We can only have one instance of readline, so we only allow
     editing on the main UI.  */
  if (ui != main_ui)
    return;

  /* Don't try enabling editing if the interpreter doesn't support it
     (e.g., MI).  */
  if (!top_level_interpreter ()->supports_command_editing ()
      || !command_interp ()->supports_command_editing ())
    return;

  if (editing)
    {
      gdb_assert (ui == main_ui);

      /* Turn on editing by using readline.  */
      ui->call_readline = gdb_rl_callback_read_char_wrapper;
    }
  else
    {
      /* Turn off editing by using gdb_readline_no_editing_callback.  */
      if (ui->command_editing)
	gdb_rl_callback_handler_remove ();
      ui->call_readline = gdb_readline_no_editing_callback;
    }
  ui->command_editing = editing;
}

/* The functions below are wrappers for rl_callback_handler_remove and
   rl_callback_handler_install that keep track of whether the callback
   handler is installed in readline.  This is necessary because after
   handling a target event of a background execution command, we may
   need to reinstall the callback handler if it was removed due to a
   secondary prompt.  See gdb_readline_wrapper_line.  We don't
   unconditionally install the handler for every target event because
   that also clears the line buffer, thus installing it while the user
   is typing would lose input.  */

/* Whether we've registered a callback handler with readline.  */
static bool callback_handler_installed;

/* See event-top.h, and above.  */

void
gdb_rl_callback_handler_remove (void)
{
  gdb_assert (current_ui == main_ui);

  rl_callback_handler_remove ();
  callback_handler_installed = false;
}

/* See event-top.h, and above.  Note this wrapper doesn't have an
   actual callback parameter because we always install
   INPUT_HANDLER.  */

void
gdb_rl_callback_handler_install (const char *prompt)
{
  gdb_assert (current_ui == main_ui);

  /* Calling rl_callback_handler_install resets readline's input
     buffer.  Calling this when we were already processing input
     therefore loses input.  */
  gdb_assert (!callback_handler_installed);

  rl_callback_handler_install (prompt, gdb_rl_callback_handler);
  callback_handler_installed = true;
}

/* See event-top.h, and above.  */

void
gdb_rl_callback_handler_reinstall (void)
{
  gdb_assert (current_ui == main_ui);

  if (!callback_handler_installed)
    {
      /* Passing NULL as prompt argument tells readline to not display
	 a prompt.  */
      gdb_rl_callback_handler_install (NULL);
    }
}

/* Displays the prompt.  If the argument NEW_PROMPT is NULL, the
   prompt that is displayed is the current top level prompt.
   Otherwise, it displays whatever NEW_PROMPT is as a local/secondary
   prompt.

   This is used after each gdb command has completed, and in the
   following cases:

   1. When the user enters a command line which is ended by '\'
   indicating that the command will continue on the next line.  In
   that case the prompt that is displayed is the empty string.

   2. When the user is entering 'commands' for a breakpoint, or
   actions for a tracepoint.  In this case the prompt will be '>'

   3. On prompting for pagination.  */

void
display_gdb_prompt (const char *new_prompt)
{
  std::string actual_gdb_prompt;

  annotate_display_prompt ();

  /* Reset the nesting depth used when trace-commands is set.  */
  reset_command_nest_depth ();

  /* Do not call the python hook on an explicit prompt change as
     passed to this function, as this forms a secondary/local prompt,
     IE, displayed but not set.  */
  if (! new_prompt)
    {
      struct ui *ui = current_ui;

      if (ui->prompt_state == PROMPTED)
	internal_error (_("double prompt"));
      else if (ui->prompt_state == PROMPT_BLOCKED)
	{
	  /* This is to trick readline into not trying to display the
	     prompt.  Even though we display the prompt using this
	     function, readline still tries to do its own display if
	     we don't call rl_callback_handler_install and
	     rl_callback_handler_remove (which readline detects
	     because a global variable is not set).  If readline did
	     that, it could mess up gdb signal handlers for SIGINT.
	     Readline assumes that between calls to rl_set_signals and
	     rl_clear_signals gdb doesn't do anything with the signal
	     handlers.  Well, that's not the case, because when the
	     target executes we change the SIGINT signal handler.  If
	     we allowed readline to display the prompt, the signal
	     handler change would happen exactly between the calls to
	     the above two functions.  Calling
	     rl_callback_handler_remove(), does the job.  */

	  if (current_ui->command_editing)
	    gdb_rl_callback_handler_remove ();
	  return;
	}
      else if (ui->prompt_state == PROMPT_NEEDED)
	{
	  /* Display the top level prompt.  */
	  actual_gdb_prompt = top_level_prompt ();
	  ui->prompt_state = PROMPTED;
	}
    }
  else
    actual_gdb_prompt = new_prompt;

  if (current_ui->command_editing)
    {
      gdb_rl_callback_handler_remove ();
      gdb_rl_callback_handler_install (actual_gdb_prompt.c_str ());
    }
  /* new_prompt at this point can be the top of the stack or the one
     passed in.  It can't be NULL.  */
  else
    {
      /* Don't use a _filtered function here.  It causes the assumed
	 character position to be off, since the newline we read from
	 the user is not accounted for.  */
      printf_unfiltered ("%s", actual_gdb_prompt.c_str ());
      gdb_flush (gdb_stdout);
    }
}

/* Notify the 'before_prompt' observer, and run any additional actions
   that must be done before we display the prompt.  */
static void
notify_before_prompt (const char *prompt)
{
  /* Give observers a chance of changing the prompt.  E.g., the python
     `gdb.prompt_hook' is installed as an observer.  */
  gdb::observers::before_prompt.notify (prompt);

  /* As we are about to display the prompt, and so GDB might be sitting
     idle for some time, close all the cached BFDs.  This ensures that
     when we next start running a user command all BFDs will be reopened
     as needed, and as a result, we will see any on-disk changes.  */
  bfd_cache_close_all ();
}

/* Return the top level prompt, as specified by "set prompt", possibly
   overridden by the python gdb.prompt_hook hook, and then composed
   with the prompt prefix and suffix (annotations).  */

static std::string
top_level_prompt (void)
{
  notify_before_prompt (get_prompt ().c_str ());

  const std::string &prompt = get_prompt ();

  if (annotation_level >= 2)
    {
      /* Prefix needs to have new line at end.  */
      const char prefix[] = "\n\032\032pre-prompt\n";

      /* Suffix needs to have a new line at end and \032 \032 at
	 beginning.  */
      const char suffix[] = "\n\032\032prompt\n";

      return std::string (prefix) + prompt.c_str () + suffix;
    }

  return prompt;
}

/* Get a reference to the current UI's line buffer.  This is used to
   construct a whole line of input from partial input.  */

static std::string &
get_command_line_buffer (void)
{
  return current_ui->line_buffer;
}

/* Re-enable stdin after the end of an execution command in
   synchronous mode, or after an error from the target, and we aborted
   the exec operation.  */

void
async_enable_stdin (void)
{
  struct ui *ui = current_ui;

  if (ui->prompt_state == PROMPT_BLOCKED)
    {
      target_terminal::ours ();
      ui->register_file_handler ();
      ui->prompt_state = PROMPT_NEEDED;
    }
}

/* Disable reads from stdin (the console) marking the command as
   synchronous.  */

void
async_disable_stdin (void)
{
  struct ui *ui = current_ui;

  ui->prompt_state = PROMPT_BLOCKED;
  ui->unregister_file_handler ();
}


/* Handle a gdb command line.  This function is called when
   handle_line_of_input has concatenated one or more input lines into
   a whole command.  */

void
command_handler (const char *command)
{
  struct ui *ui = current_ui;
  const char *c;

  if (ui->instream == ui->stdin_stream)
    reinitialize_more_filter ();

  scoped_command_stats stat_reporter (true);

  /* Do not execute commented lines.  */
  for (c = command; *c == ' ' || *c == '\t'; c++)
    ;
  if (c[0] != '#')
    {
      execute_command (command, ui->instream == ui->stdin_stream);

      /* Do any commands attached to breakpoint we stopped at.  */
      bpstat_do_actions ();
    }
}

/* Append RL, an input line returned by readline or one of its emulations, to
   CMD_LINE_BUFFER.  Return true if we have a whole command line ready to be
   processed by the command interpreter or false if the command line isn't
   complete yet (input line ends in a backslash).  */

static bool
command_line_append_input_line (std::string &cmd_line_buffer, const char *rl)
{
  size_t len = strlen (rl);

  if (len > 0 && rl[len - 1] == '\\')
    {
      /* Don't copy the backslash and wait for more.  */
      cmd_line_buffer.append (rl, len - 1);
      return false;
    }
  else
    {
      /* Copy whole line including terminating null, and we're
	 done.  */
      cmd_line_buffer.append (rl, len + 1);
      return true;
    }
}

/* Handle a line of input coming from readline.

   If the read line ends with a continuation character (backslash), return
   nullptr.  Otherwise, return a pointer to the command line, indicating a whole
   command line is ready to be executed.

   The returned pointer may or may not point to CMD_LINE_BUFFER's internal
   buffer.

   Return EOF on end of file.

   If REPEAT, handle command repetitions:

     - If the input command line is NOT empty, the command returned is
       saved using save_command_line () so that it can be repeated later.

     - OTOH, if the input command line IS empty, return the saved
       command instead of the empty input line.
*/

const char *
handle_line_of_input (std::string &cmd_line_buffer,
		      const char *rl, int repeat,
		      const char *annotation_suffix)
{
  struct ui *ui = current_ui;
  int from_tty = ui->instream == ui->stdin_stream;

  if (rl == NULL)
    return (char *) EOF;

  bool complete = command_line_append_input_line (cmd_line_buffer, rl);
  if (!complete)
    return NULL;

  if (from_tty && annotation_level > 1)
    printf_unfiltered (("\n\032\032post-%s\n"), annotation_suffix);

#define SERVER_COMMAND_PREFIX "server "
  server_command = startswith (cmd_line_buffer.c_str (), SERVER_COMMAND_PREFIX);
  if (server_command)
    {
      /* Note that we don't call `save_command_line'.  Between this
	 and the check in dont_repeat, this insures that repeating
	 will still do the right thing.  */
      return cmd_line_buffer.c_str () + strlen (SERVER_COMMAND_PREFIX);
    }

  /* Do history expansion if that is wished.  */
  if (history_expansion_p && from_tty && current_ui->input_interactive_p ())
    {
      char *cmd_expansion;
      int expanded;

      /* Note: here, we pass a pointer to the std::string's internal buffer as
	 a `char *`.  At the time of writing, readline's history_expand does
	 not modify the passed-in string.  Ideally, readline should be modified
	 to make that parameter `const char *`.  */
      expanded = history_expand (&cmd_line_buffer[0], &cmd_expansion);
      gdb::unique_xmalloc_ptr<char> history_value (cmd_expansion);
      if (expanded)
	{
	  /* Print the changes.  */
	  printf_unfiltered ("%s\n", history_value.get ());

	  /* If there was an error, call this function again.  */
	  if (expanded < 0)
	    return cmd_line_buffer.c_str ();

	  cmd_line_buffer = history_value.get ();
	}
    }

  /* If we just got an empty line, and that is supposed to repeat the
     previous command, return the previously saved command.  */
  const char *p1;
  for (p1 = cmd_line_buffer.c_str (); *p1 == ' ' || *p1 == '\t'; p1++)
    ;
  if (repeat && *p1 == '\0')
    return get_saved_command_line ();

  /* Add command to history if appropriate.  Note: lines consisting
     solely of comments are also added to the command history.  This
     is useful when you type a command, and then realize you don't
     want to execute it quite yet.  You can comment out the command
     and then later fetch it from the value history and remove the
     '#'.  The kill ring is probably better, but some people are in
     the habit of commenting things out.  */
  if (cmd_line_buffer[0] != '\0' && from_tty && current_ui->input_interactive_p ())
    gdb_add_history (cmd_line_buffer.c_str ());

  /* Save into global buffer if appropriate.  */
  if (repeat)
    {
      save_command_line (cmd_line_buffer.c_str ());

      /* It is important that we return a pointer to the saved command line
	 here, for the `cmd_start == saved_command_line` check in
	 execute_command to work.  */
      return get_saved_command_line ();
    }

  return cmd_line_buffer.c_str ();
}

/* See event-top.h.  */

void
gdb_rl_deprep_term_function (void)
{
#ifdef RL_STATE_EOF
  std::optional<scoped_restore_tmpl<int>> restore_eof_found;

  if (RL_ISSTATE (RL_STATE_EOF))
    {
      printf_unfiltered ("quit\n");
      restore_eof_found.emplace (&rl_eof_found, 0);
    }

#endif /* RL_STATE_EOF */

  rl_deprep_terminal ();
}

/* Handle a complete line of input.  This is called by the callback
   mechanism within the readline library.  Deal with incomplete
   commands as well, by saving the partial input in a global
   buffer.

   NOTE: This is the asynchronous version of the command_line_input
   function.  */

void
command_line_handler (gdb::unique_xmalloc_ptr<char> &&rl)
{
  std::string &line_buffer = get_command_line_buffer ();
  struct ui *ui = current_ui;

  const char *cmd = handle_line_of_input (line_buffer, rl.get (), 1, "prompt");
  if (cmd == (char *) EOF)
    {
      /* stdin closed.  The connection with the terminal is gone.
	 This happens at the end of a testsuite run, after Expect has
	 hung up but GDB is still alive.  In such a case, we just quit
	 gdb killing the inferior program too.  This also happens if the
	 user sends EOF, which is usually bound to ctrl+d.  */

#ifndef RL_STATE_EOF
      /* When readline is using bracketed paste mode, then, when eof is
	 received, readline will emit the control sequence to leave
	 bracketed paste mode.

	 This control sequence ends with \r, which means that the "quit" we
	 are about to print will overwrite the prompt on this line.

	 The solution to this problem is to actually print the "quit"
	 message from gdb_rl_deprep_term_function (see above), however, we
	 can only do that if we can know, in that function, when eof was
	 received.

	 Unfortunately, with older versions of readline, it is not possible
	 in the gdb_rl_deprep_term_function to know if eof was received or
	 not, and, as GDB can be built against the system readline, which
	 could be older than the readline in GDB's repository, then we
	 can't be sure that we can work around this prompt corruption in
	 the gdb_rl_deprep_term_function function.

	 If we get here, RL_STATE_EOF is not defined.  This indicates that
	 we are using an older readline, and couldn't print the quit
	 message in gdb_rl_deprep_term_function.  So, what we do here is
	 check to see if bracketed paste mode is on or not.  If it's on
	 then we print a \n and then the quit, this means the user will
	 see:

	 (gdb)
	 quit

	 Rather than the usual:

	 (gdb) quit

	 Which we will get with a newer readline, but this really is the
	 best we can do with older versions of readline.  */
      const char *value = rl_variable_value ("enable-bracketed-paste");
      if (value != nullptr && strcmp (value, "on") == 0
	  && ((rl_readline_version >> 8) & 0xff) > 0x07)
	printf_unfiltered ("\n");
      printf_unfiltered ("quit\n");
#endif

      execute_command ("quit", 1);
    }
  else if (cmd == NULL)
    {
      /* We don't have a full line yet.  Print an empty prompt.  */
      display_gdb_prompt ("");
    }
  else
    {
      ui->prompt_state = PROMPT_NEEDED;

      /* Ensure the UI's line buffer is empty for the next command.  */
      SCOPE_EXIT { line_buffer.clear (); };

      command_handler (cmd);

      if (ui->prompt_state != PROMPTED)
	display_gdb_prompt (0);
    }
}

/* Does reading of input from terminal w/o the editing features
   provided by the readline library.  Calls the line input handler
   once we have a whole input line.  */

void
gdb_readline_no_editing_callback (gdb_client_data client_data)
{
  int c;
  std::string line_buffer;
  struct ui *ui = current_ui;

  FILE *stream = ui->instream != nullptr ? ui->instream : ui->stdin_stream;
  gdb_assert (stream != nullptr);

  /* We still need the while loop here, even though it would seem
     obvious to invoke gdb_readline_no_editing_callback at every
     character entered.  If not using the readline library, the
     terminal is in cooked mode, which sends the characters all at
     once.  Poll will notice that the input fd has changed state only
     after enter is pressed.  At this point we still need to fetch all
     the chars entered.  */

  while (1)
    {
      /* Read from stdin if we are executing a user defined command.
	 This is the right thing for prompt_for_continue, at least.  */
      c = fgetc (stream);

      if (c == EOF)
	{
	  if (!line_buffer.empty ())
	    {
	      /* The last line does not end with a newline.  Return it, and
		 if we are called again fgetc will still return EOF and
		 we'll return NULL then.  */
	      break;
	    }
	  ui->input_handler (NULL);
	  return;
	}

      if (c == '\n')
	{
	  if (!line_buffer.empty () && line_buffer.back () == '\r')
	    line_buffer.pop_back ();
	  break;
	}

      line_buffer += c;
    }

  ui->input_handler (make_unique_xstrdup (line_buffer.c_str ()));
}


/* Attempt to unblock signal SIG, return true if the signal was unblocked,
   otherwise, return false.  */

static bool
unblock_signal (int sig)
{
#if HAVE_SIGPROCMASK
  sigset_t sigset;
  sigemptyset (&sigset);
  sigaddset (&sigset, sig);
  gdb_sigmask (SIG_UNBLOCK, &sigset, 0);
  return true;
#endif

  return false;
}

/* Called to handle fatal signals.  SIG is the signal number.  */

static void ATTRIBUTE_NORETURN
handle_fatal_signal (int sig)
{
#ifdef TUI
  tui_disable ();
#endif

#ifdef GDB_PRINT_INTERNAL_BACKTRACE
  const auto sig_write = [] (const char *msg) -> void
  {
    gdb_stderr->write_async_safe (msg, strlen (msg));
  };

  if (bt_on_fatal_signal)
    {
      sig_write ("\n\n");
      sig_write (_("Fatal signal: "));
      sig_write (strsignal (sig));
      sig_write ("\n");

      gdb_internal_backtrace ();

      sig_write (_("A fatal error internal to GDB has been detected, "
		   "further\ndebugging is not possible.  GDB will now "
		   "terminate.\n\n"));
      sig_write (_("This is a bug, please report it."));
      if (REPORT_BUGS_TO[0] != '\0')
	{
	  sig_write (_("  For instructions, see:\n"));
	  sig_write (REPORT_BUGS_TO);
	  sig_write (".");
	}
      sig_write ("\n\n");

      gdb_stderr->flush ();
    }
#endif

  /* If possible arrange for SIG to have its default behaviour (which
     should be to terminate the current process), unblock SIG, and reraise
     the signal.  This ensures GDB terminates with the expected signal.  */
  if (signal (sig, SIG_DFL) != SIG_ERR
      && unblock_signal (sig))
    raise (sig);

  /* The above failed, so try to use SIGABRT to terminate GDB.  */
#ifdef SIGABRT
  signal (SIGABRT, SIG_DFL);
#endif
  abort ();		/* ARI: abort */
}

/* The SIGSEGV handler for this thread, or NULL if there is none.  GDB
   always installs a global SIGSEGV handler, and then lets threads
   indicate their interest in handling the signal by setting this
   thread-local variable.

   This is a static variable instead of extern because on various platforms
   (notably Cygwin) extern thread_local variables cause link errors.  So
   instead, we have scoped_segv_handler_restore, which also makes it impossible
   to accidentally forget to restore it to the original value.  */

static thread_local void (*thread_local_segv_handler) (int);

static void handle_sigsegv (int sig);

/* Install the SIGSEGV handler.  */
static void
install_handle_sigsegv ()
{
#if defined (HAVE_SIGACTION)
  struct sigaction sa;
  sa.sa_handler = handle_sigsegv;
  sigemptyset (&sa.sa_mask);
#ifdef HAVE_SIGALTSTACK
  sa.sa_flags = SA_ONSTACK;
#else
  sa.sa_flags = 0;
#endif
  sigaction (SIGSEGV, &sa, nullptr);
#else
  signal (SIGSEGV, handle_sigsegv);
#endif
}

/* Handler for SIGSEGV.  */

static void
handle_sigsegv (int sig)
{
  install_handle_sigsegv ();

  if (thread_local_segv_handler == nullptr)
    handle_fatal_signal (sig);
  thread_local_segv_handler (sig);
}



/* The serial event associated with the QUIT flag.  set_quit_flag sets
   this, and check_quit_flag clears it.  Used by interruptible_select
   to be able to do interruptible I/O with no race with the SIGINT
   handler.  */
static struct serial_event *quit_serial_event;

/* Initialization of signal handlers and tokens.  There are a number of
   different strategies for handling different signals here.

   For SIGINT, SIGTERM, SIGQUIT, SIGHUP, SIGTSTP, there is a function
   handle_sig* for each of these signals.  These functions are the actual
   signal handlers associated to the signals via calls to signal().  The
   only job for these functions is to enqueue the appropriate
   event/procedure with the event loop.  The event loop will take care of
   invoking the queued procedures to perform the usual tasks associated
   with the reception of the signal.

   For SIGSEGV the handle_sig* function does all the work for handling this
   signal.

   For SIGFPE, SIGBUS, and SIGABRT, these signals will all cause GDB to
   terminate immediately.  */
void
gdb_init_signals (void)
{
  initialize_async_signal_handlers ();

  quit_serial_event = make_serial_event ();

  sigint_token =
    create_async_signal_handler (async_request_quit, NULL, "sigint");
  install_sigint_handler (handle_sigint);

  async_sigterm_token
    = create_async_signal_handler (async_sigterm_handler, NULL, "sigterm");
  signal (SIGTERM, handle_sigterm);

#ifdef SIGQUIT
  sigquit_token =
    create_async_signal_handler (async_do_nothing, NULL, "sigquit");
  signal (SIGQUIT, handle_sigquit);
#endif

#ifdef SIGHUP
  if (signal (SIGHUP, handle_sighup) != SIG_IGN)
    sighup_token =
      create_async_signal_handler (async_disconnect, NULL, "sighup");
  else
    sighup_token =
      create_async_signal_handler (async_do_nothing, NULL, "sighup");
#endif

#ifdef SIGTSTP
  sigtstp_token =
    create_async_signal_handler (async_sigtstp_handler, NULL, "sigtstp");
#endif

#ifdef SIGFPE
  signal (SIGFPE, handle_fatal_signal);
#endif

#ifdef SIGBUS
  signal (SIGBUS, handle_fatal_signal);
#endif

#ifdef SIGABRT
  signal (SIGABRT, handle_fatal_signal);
#endif

  install_handle_sigsegv ();
}

/* See defs.h.  */

void
quit_serial_event_set (void)
{
  serial_event_set (quit_serial_event);
}

/* See defs.h.  */

void
quit_serial_event_clear (void)
{
  serial_event_clear (quit_serial_event);
}

/* Return the selectable file descriptor of the serial event
   associated with the quit flag.  */

static int
quit_serial_event_fd (void)
{
  return serial_event_fd (quit_serial_event);
}

/* See defs.h.  */

void
default_quit_handler (void)
{
  if (check_quit_flag ())
    {
      if (target_terminal::is_ours ())
	quit ();
      else
	target_pass_ctrlc ();
    }
}

/* See defs.h.  */
quit_handler_ftype *quit_handler = default_quit_handler;

/* Handle a SIGINT.  */

void
handle_sigint (int sig)
{
  signal (sig, handle_sigint);

  /* We could be running in a loop reading in symfiles or something so
     it may be quite a while before we get back to the event loop.  So
     set quit_flag to 1 here.  Then if QUIT is called before we get to
     the event loop, we will unwind as expected.  */
  set_quit_flag ();

  /* In case nothing calls QUIT before the event loop is reached, the
     event loop handles it.  */
  mark_async_signal_handler (sigint_token);
}

/* See gdb_select.h.  */

int
interruptible_select (int n,
		      fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		      struct timeval *timeout)
{
  fd_set my_readfds;
  int fd;
  int res;

  if (readfds == NULL)
    {
      readfds = &my_readfds;
      FD_ZERO (&my_readfds);
    }

  fd = quit_serial_event_fd ();
  FD_SET (fd, readfds);
  if (n <= fd)
    n = fd + 1;

  do
    {
      res = gdb_select (n, readfds, writefds, exceptfds, timeout);
    }
  while (res == -1 && errno == EINTR);

  if (res == 1 && FD_ISSET (fd, readfds))
    {
      errno = EINTR;
      return -1;
    }
  return res;
}

/* Handle GDB exit upon receiving SIGTERM if target_can_async_p ().  */

static void
async_sigterm_handler (gdb_client_data arg)
{
  quit_force (NULL, 0);
}

/* See defs.h.  */
volatile bool sync_quit_force_run;

/* See defs.h.  */
void
set_force_quit_flag ()
{
  sync_quit_force_run = true;
  set_quit_flag ();
}

/* Quit GDB if SIGTERM is received.
   GDB would quit anyway, but this way it will clean up properly.  */
void
handle_sigterm (int sig)
{
  signal (sig, handle_sigterm);

  set_force_quit_flag ();

  mark_async_signal_handler (async_sigterm_token);
}

/* Do the quit.  All the checks have been done by the caller.  */
void
async_request_quit (gdb_client_data arg)
{
  /* If the quit_flag has gotten reset back to 0 by the time we get
     back here, that means that an exception was thrown to unwind the
     current command before we got back to the event loop.  So there
     is no reason to call quit again here.  */
  QUIT;
}

#ifdef SIGQUIT
/* Tell the event loop what to do if SIGQUIT is received.
   See event-signal.c.  */
static void
handle_sigquit (int sig)
{
  mark_async_signal_handler (sigquit_token);
  signal (sig, handle_sigquit);
}
#endif

#if defined (SIGQUIT) || defined (SIGHUP)
/* Called by the event loop in response to a SIGQUIT or an
   ignored SIGHUP.  */
static void
async_do_nothing (gdb_client_data arg)
{
  /* Empty function body.  */
}
#endif

#ifdef SIGHUP
/* Tell the event loop what to do if SIGHUP is received.
   See event-signal.c.  */
static void
handle_sighup (int sig)
{
  mark_async_signal_handler (sighup_token);
  signal (sig, handle_sighup);
}

/* Called by the event loop to process a SIGHUP.  */
static void
async_disconnect (gdb_client_data arg)
{

  try
    {
      quit_cover ();
    }

  catch (const gdb_exception &exception)
    {
      gdb_puts ("Could not kill the program being debugged",
		gdb_stderr);
      exception_print (gdb_stderr, exception);
      if (exception.reason == RETURN_FORCED_QUIT)
	throw;
    }

  for (inferior *inf : all_inferiors ())
    {
      try
	{
	  inf->pop_all_targets ();
	}
      catch (const gdb_exception &exception)
	{
	}
    }

  signal (SIGHUP, SIG_DFL);	/*FIXME: ???????????  */
  raise (SIGHUP);
}
#endif

#ifdef SIGTSTP
void
handle_sigtstp (int sig)
{
  mark_async_signal_handler (sigtstp_token);
  signal (sig, handle_sigtstp);
}

static void
async_sigtstp_handler (gdb_client_data arg)
{
  const std::string &prompt = get_prompt ();

  signal (SIGTSTP, SIG_DFL);
  unblock_signal (SIGTSTP);
  raise (SIGTSTP);
  signal (SIGTSTP, handle_sigtstp);
  printf_unfiltered ("%s", prompt.c_str ());
  gdb_flush (gdb_stdout);

  /* Forget about any previous command -- null line now will do
     nothing.  */
  dont_repeat ();
}
#endif /* SIGTSTP */



/* Set things up for readline to be invoked via the alternate
   interface, i.e. via a callback function
   (gdb_rl_callback_read_char), and hook up instream to the event
   loop.  */

void
gdb_setup_readline (int editing)
{
  struct ui *ui = current_ui;

  /* If the input stream is connected to a terminal, turn on editing.
     However, that is only allowed on the main UI, as we can only have
     one instance of readline.  Also, INSTREAM might be nullptr when
     executing a user-defined command.  */
  if (ui->instream != nullptr && ISATTY (ui->instream)
      && editing && ui == main_ui)
    {
      /* Tell gdb that we will be using the readline library.  This
	 could be overwritten by a command in .gdbinit like 'set
	 editing on' or 'off'.  */
      ui->command_editing = 1;

      /* When a character is detected on instream by select or poll,
	 readline will be invoked via this callback function.  */
      ui->call_readline = gdb_rl_callback_read_char_wrapper;

      /* Tell readline to use the same input stream that gdb uses.  */
      rl_instream = ui->instream;
    }
  else
    {
      ui->command_editing = 0;
      ui->call_readline = gdb_readline_no_editing_callback;
    }

  /* Now create the event source for this UI's input file descriptor.
     Another source is going to be the target program (inferior), but
     that must be registered only when it actually exists (I.e. after
     we say 'run' or after we connect to a remote target.  */
  ui->register_file_handler ();
}

/* Disable command input through the standard CLI channels.  Used in
   the suspend proc for interpreters that use the standard gdb readline
   interface, like the cli & the mi.  */

void
gdb_disable_readline (void)
{
  struct ui *ui = current_ui;

  if (ui->command_editing)
    gdb_rl_callback_handler_remove ();
  ui->unregister_file_handler ();
}

scoped_segv_handler_restore::scoped_segv_handler_restore (segv_handler_t new_handler)
{
  m_old_handler = thread_local_segv_handler;
  thread_local_segv_handler = new_handler;
}

scoped_segv_handler_restore::~scoped_segv_handler_restore()
{
  thread_local_segv_handler = m_old_handler;
}

static const char debug_event_loop_off[] = "off";
static const char debug_event_loop_all_except_ui[] = "all-except-ui";
static const char debug_event_loop_all[] = "all";

static const char *debug_event_loop_enum[] = {
  debug_event_loop_off,
  debug_event_loop_all_except_ui,
  debug_event_loop_all,
  nullptr
};

static const char *debug_event_loop_value = debug_event_loop_off;

static void
set_debug_event_loop_command (const char *args, int from_tty,
			      cmd_list_element *c)
{
  if (debug_event_loop_value == debug_event_loop_off)
    debug_event_loop = debug_event_loop_kind::OFF;
  else if (debug_event_loop_value == debug_event_loop_all_except_ui)
    debug_event_loop = debug_event_loop_kind::ALL_EXCEPT_UI;
  else if (debug_event_loop_value == debug_event_loop_all)
    debug_event_loop = debug_event_loop_kind::ALL;
  else
    gdb_assert_not_reached ("Invalid debug event look kind value.");
}

static void
show_debug_event_loop_command (struct ui_file *file, int from_tty,
			       struct cmd_list_element *cmd, const char *value)
{
  gdb_printf (file, _("Event loop debugging is %s.\n"), value);
}

void _initialize_event_top ();
void
_initialize_event_top ()
{
  add_setshow_enum_cmd ("event-loop", class_maintenance,
			debug_event_loop_enum,
			&debug_event_loop_value,
			_("Set event-loop debugging."),
			_("Show event-loop debugging."),
			_("\
Control whether to show event loop-related debug messages."),
			set_debug_event_loop_command,
			show_debug_event_loop_command,
			&setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("backtrace-on-fatal-signal", class_maintenance,
			   &bt_on_fatal_signal, _("\
Set whether to produce a backtrace if GDB receives a fatal signal."), _("\
Show whether GDB will produce a backtrace if it receives a fatal signal."), _("\
Use \"on\" to enable, \"off\" to disable.\n\
If enabled, GDB will produce a minimal backtrace if it encounters a fatal\n\
signal from within GDB itself.  This is a mechanism to help diagnose\n\
crashes within GDB, not a mechanism for debugging inferiors."),
			   gdb_internal_backtrace_set_cmd,
			   show_bt_on_fatal_signal,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);
}
