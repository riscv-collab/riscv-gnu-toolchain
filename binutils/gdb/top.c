/* Top level stuff for GDB, the GNU debugger.

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
#include "gdbcmd.h"
#include "cli/cli-cmds.h"
#include "cli/cli-script.h"
#include "cli/cli-setshow.h"
#include "cli/cli-decode.h"
#include "symtab.h"
#include "inferior.h"
#include "infrun.h"
#include <signal.h>
#include "target.h"
#include "target-dcache.h"
#include "breakpoint.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "language.h"
#include "terminal.h"
#include "gdbsupport/job-control.h"
#include "annotate.h"
#include "completer.h"
#include "top.h"
#include "ui.h"
#include "gdbsupport/version.h"
#include "serial.h"
#include "main.h"
#include "gdbsupport/event-loop.h"
#include "gdbthread.h"
#include "extension.h"
#include "interps.h"
#include "observable.h"
#include "maint.h"
#include "filenames.h"
#include "frame.h"
#include "gdbsupport/gdb_select.h"
#include "gdbsupport/scope-exit.h"
#include "gdbarch.h"
#include "gdbsupport/pathstuff.h"
#include "cli/cli-style.h"
#include "pager.h"

/* readline include files.  */
#include "readline/readline.h"
#include "readline/history.h"

/* readline defines this.  */
#undef savestring

#include <sys/types.h>

#include "event-top.h"
#include <sys/stat.h>
#include <ctype.h>
#include "ui-out.h"
#include "cli-out.h"
#include "tracepoint.h"
#include "inf-loop.h"

#if defined(TUI)
# include "tui/tui.h"
# include "tui/tui-io.h"
#endif

#ifndef O_NOCTTY
# define O_NOCTTY 0
#endif

extern void initialize_all_files (void);

#define PROMPT(X) the_prompts.prompt_stack[the_prompts.top + X].prompt
#define PREFIX(X) the_prompts.prompt_stack[the_prompts.top + X].prefix
#define SUFFIX(X) the_prompts.prompt_stack[the_prompts.top + X].suffix

/* Default command line prompt.  This is overridden in some configs.  */

#ifndef DEFAULT_PROMPT
#define DEFAULT_PROMPT	"(gdb) "
#endif

struct ui_file **
current_ui_gdb_stdout_ptr ()
{
  return &current_ui->m_gdb_stdout;
}

struct ui_file **
current_ui_gdb_stdin_ptr ()
{
  return &current_ui->m_gdb_stdin;
}

struct ui_file **
current_ui_gdb_stderr_ptr ()
{
  return &current_ui->m_gdb_stderr;
}

struct ui_file **
current_ui_gdb_stdlog_ptr ()
{
  return &current_ui->m_gdb_stdlog;
}

struct ui_out **
current_ui_current_uiout_ptr ()
{
  return &current_ui->m_current_uiout;
}

int inhibit_gdbinit = 0;

/* Flag for whether we want to confirm potentially dangerous
   operations.  Default is yes.  */

bool confirm = true;

static void
show_confirm (struct ui_file *file, int from_tty,
	      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Whether to confirm potentially "
		      "dangerous operations is %s.\n"),
	      value);
}

/* The last command line executed on the console.  Used for command
   repetitions when the user enters an empty line.  */

static char *saved_command_line;

/* If not NULL, the arguments that should be passed if
   saved_command_line is repeated.  */

static const char *repeat_arguments;

/* The previous last command line executed on the console.  Used for command
   repetitions when a command wants to relaunch the previously launched
   command.  We need this as when a command is running, saved_command_line
   already contains the line of the currently executing command.  */

static char *previous_saved_command_line;

/* If not NULL, the arguments that should be passed if the
   previous_saved_command_line is repeated.  */

static const char *previous_repeat_arguments;

/* Nonzero if the current command is modified by "server ".  This
   affects things like recording into the command history, commands
   repeating on RETURN, etc.  This is so a user interface (emacs, GUI,
   whatever) can issue its own commands and also send along commands
   from the user, and have the user not notice that the user interface
   is issuing commands too.  */
bool server_command;

/* Timeout limit for response from target.  */

/* The default value has been changed many times over the years.  It
   was originally 5 seconds.  But that was thought to be a long time
   to sit and wait, so it was changed to 2 seconds.  That was thought
   to be plenty unless the connection was going through some terminal
   server or multiplexer or other form of hairy serial connection.

   In mid-1996, remote_timeout was moved from remote.c to top.c and
   it began being used in other remote-* targets.  It appears that the
   default was changed to 20 seconds at that time, perhaps because the
   Renesas E7000 ICE didn't always respond in a timely manner.

   But if 5 seconds is a long time to sit and wait for retransmissions,
   20 seconds is far worse.  This demonstrates the difficulty of using
   a single variable for all protocol timeouts.

   As remote.c is used much more than remote-e7000.c, it was changed
   back to 2 seconds in 1999.  */

int remote_timeout = 2;

/* Sbrk location on entry to main.  Used for statistics only.  */
#ifdef HAVE_USEFUL_SBRK
char *lim_at_start;
#endif

/* Hooks for alternate command interfaces.  */

/* This hook is called from within gdb's many mini-event loops which
   could steal control from a real user interface's event loop.  It
   returns non-zero if the user is requesting a detach, zero
   otherwise.  */

int (*deprecated_ui_loop_hook) (int);


/* Called from print_frame_info to list the line we stopped in.  */

void (*deprecated_print_frame_info_listing_hook) (struct symtab * s,
						  int line,
						  int stopline,
						  int noerror);
/* Replaces most of query.  */

int (*deprecated_query_hook) (const char *, va_list);

/* Replaces most of warning.  */

thread_local void (*deprecated_warning_hook) (const char *, va_list);

/* These three functions support getting lines of text from the user.
   They are used in sequence.  First deprecated_readline_begin_hook is
   called with a text string that might be (for example) a message for
   the user to type in a sequence of commands to be executed at a
   breakpoint.  If this function calls back to a GUI, it might take
   this opportunity to pop up a text interaction window with this
   message.  Next, deprecated_readline_hook is called with a prompt
   that is emitted prior to collecting the user input.  It can be
   called multiple times.  Finally, deprecated_readline_end_hook is
   called to notify the GUI that we are done with the interaction
   window and it can close it.  */

void (*deprecated_readline_begin_hook) (const char *, ...);
char *(*deprecated_readline_hook) (const char *);
void (*deprecated_readline_end_hook) (void);

/* Called as appropriate to notify the interface that we have attached
   to or detached from an already running process.  */

void (*deprecated_attach_hook) (void);
void (*deprecated_detach_hook) (void);

/* Used by UI as a wrapper around command execution.  May do various
   things like enabling/disabling buttons, etc...  */

void (*deprecated_call_command_hook) (struct cmd_list_element * c,
				      const char *cmd, int from_tty);

/* Called when the current thread changes.  Argument is thread id.  */

void (*deprecated_context_hook) (int id);

/* See top.h.  */

void
unbuffer_stream (FILE *stream)
{
  /* Unbuffer the input stream so that in gdb_readline_no_editing_callback,
     the calls to fgetc fetch only one char at the time from STREAM.

     This is important because gdb_readline_no_editing_callback will read
     from STREAM up to the first '\n' character, after this GDB returns to
     the event loop and relies on a select on STREAM indicating that more
     input is pending.

     If STREAM is buffered then the fgetc calls may have moved all the
     pending input from the kernel into a local buffer, after which the
     select will not indicate that more input is pending, and input after
     the first '\n' will not be processed immediately.

     Please ensure that any changes in this area run the MI tests with the
     FORCE_SEPARATE_MI_TTY=1 flag being passed.  */

#ifdef __MINGW32__
  /* With MS-Windows runtime, making stdin unbuffered when it's
     connected to the terminal causes it to misbehave.  */
  if (!ISATTY (stream))
    setbuf (stream, nullptr);
#else
  /* On GNU/Linux the issues described above can impact GDB even when
     dealing with input from a terminal.  For now we unbuffer the input
     stream for everyone except MS-Windows.  */
  setbuf (stream, nullptr);
#endif
}

/* Handler for SIGHUP.  */

#ifdef SIGHUP
/* NOTE 1999-04-29: This function will be static again, once we modify
   gdb to use the event loop as the default command loop and we merge
   event-top.c into this file, top.c.  */
/* static */ void
quit_cover (void)
{
  /* Stop asking user for confirmation --- we're exiting.  This
     prevents asking the user dumb questions.  */
  confirm = 0;
  quit_command ((char *) 0, 0);
}
#endif /* defined SIGHUP */

/* Line number we are currently in, in a file which is being sourced.  */
/* NOTE 1999-04-29: This variable will be static again, once we modify
   gdb to use the event loop as the default command loop and we merge
   event-top.c into this file, top.c.  */
/* static */ int source_line_number;

/* Name of the file we are sourcing.  */
/* NOTE 1999-04-29: This variable will be static again, once we modify
   gdb to use the event loop as the default command loop and we merge
   event-top.c into this file, top.c.  */
/* static */ std::string source_file_name;

/* Read commands from STREAM.  */
void
read_command_file (FILE *stream)
{
  struct ui *ui = current_ui;

  unbuffer_stream (stream);

  scoped_restore save_instream
    = make_scoped_restore (&ui->instream, stream);

  /* Read commands from `instream' and execute them until end of file
     or error reading instream.  */

  while (ui->instream != NULL && !feof (ui->instream))
    {
      /* Get a command-line.  This calls the readline package.  */
      std::string command_buffer;
      const char *command
	= command_line_input (command_buffer, nullptr, nullptr);
      if (command == nullptr)
	break;
      command_handler (command);
    }
}

#ifdef __MSDOS__
static void
do_chdir_cleanup (void *old_dir)
{
  chdir ((const char *) old_dir);
  xfree (old_dir);
}
#endif

scoped_value_mark
prepare_execute_command ()
{
  /* With multiple threads running while the one we're examining is
     stopped, the dcache can get stale without us being able to detect
     it.  For the duration of the command, though, use the dcache to
     help things like backtrace.  */
  if (non_stop)
    target_dcache_invalidate (current_program_space->aspace);

  return scoped_value_mark ();
}

/* Tell the user if the language has changed (except first time) after
   executing a command.  */

void
check_frame_language_change (void)
{
  static int warned = 0;
  frame_info_ptr frame;

  /* First make sure that a new frame has been selected, in case the
     command or the hooks changed the program state.  */
  frame = deprecated_safe_get_selected_frame ();
  if (current_language != expected_language)
    {
      if (language_mode == language_mode_auto && info_verbose)
	{
	  /* Print what changed.  */
	  language_info ();
	}
      warned = 0;
    }

  /* Warn the user if the working language does not match the language
     of the current frame.  Only warn the user if we are actually
     running the program, i.e. there is a stack.  */
  /* FIXME: This should be cacheing the frame and only running when
     the frame changes.  */

  if (has_stack_frames ())
    {
      enum language flang;

      flang = get_frame_language (frame);
      if (!warned
	  && flang != language_unknown
	  && flang != current_language->la_language)
	{
	  gdb_printf ("%s\n", _(lang_frame_mismatch_warn));
	  warned = 1;
	}
    }
}

/* See top.h.  */

void
wait_sync_command_done (void)
{
  /* Processing events may change the current UI.  */
  scoped_restore save_ui = make_scoped_restore (&current_ui);
  struct ui *ui = current_ui;

  /* We're about to wait until the target stops after having resumed
     it so must force-commit resumptions, in case we're being called
     in some context where a scoped_disable_commit_resumed object is
     active.  I.e., this function is a commit-resumed sync/flush
     point.  */
  scoped_enable_commit_resumed enable ("sync wait");

  while (gdb_do_one_event () >= 0)
    if (ui->prompt_state != PROMPT_BLOCKED)
      break;
}

/* See top.h.  */

void
maybe_wait_sync_command_done (int was_sync)
{
  /* If the interpreter is in sync mode (we're running a user
     command's list, running command hooks or similars), and we
     just ran a synchronous command that started the target, wait
     for that command to end.  */
  if (!current_ui->async
      && !was_sync
      && current_ui->prompt_state == PROMPT_BLOCKED)
    wait_sync_command_done ();
}

/* See command.h.  */

void
set_repeat_arguments (const char *args)
{
  repeat_arguments = args;
}

/* Execute the line P as a command, in the current user context.
   Pass FROM_TTY as second argument to the defining function.  */

void
execute_command (const char *p, int from_tty)
{
  struct cmd_list_element *c;
  const char *line;
  const char *cmd_start = p;

  auto cleanup_if_error = make_scope_exit (bpstat_clear_actions);
  scoped_value_mark cleanup = prepare_execute_command ();

  /* This can happen when command_line_input hits end of file.  */
  if (p == NULL)
    {
      cleanup_if_error.release ();
      return;
    }

  std::string cmd_copy = p;

  target_log_command (p);

  while (*p == ' ' || *p == '\t')
    p++;
  if (*p)
    {
      const char *cmd = p;
      const char *arg;
      std::string default_args;
      std::string default_args_and_arg;
      int was_sync = current_ui->prompt_state == PROMPT_BLOCKED;

      line = p;

      /* If trace-commands is set then this will print this command.  */
      print_command_trace ("%s", p);

      c = lookup_cmd (&cmd, cmdlist, "", &default_args, 0, 1);
      p = cmd;

      scoped_restore save_repeat_args
	= make_scoped_restore (&repeat_arguments, nullptr);
      const char *args_pointer = p;

      if (!default_args.empty ())
	{
	  if (*p != '\0')
	    default_args_and_arg = default_args + ' ' + p;
	  else
	    default_args_and_arg = default_args;
	  arg = default_args_and_arg.c_str ();
	}
      else
	{
	  /* Pass null arg rather than an empty one.  */
	  arg = *p == '\0' ? nullptr : p;
	}

      /* FIXME: cagney/2002-02-02: The c->type test is pretty dodgy
	 while the is_complete_command(cfunc) test is just plain
	 bogus.  They should both be replaced by a test of the form
	 c->strip_trailing_white_space_p.  */
      /* NOTE: cagney/2002-02-02: The function.cfunc in the below
	 can't be replaced with func.  This is because it is the
	 cfunc, and not the func, that has the value that the
	 is_complete_command hack is testing for.  */
      /* Clear off trailing whitespace, except for set and complete
	 command.  */
      std::string without_whitespace;
      if (arg
	  && c->type != set_cmd
	  && !is_complete_command (c))
	{
	  const char *old_end = arg + strlen (arg) - 1;
	  p = old_end;
	  while (p >= arg && (*p == ' ' || *p == '\t'))
	    p--;
	  if (p != old_end)
	    {
	      without_whitespace = std::string (arg, p + 1);
	      arg = without_whitespace.c_str ();
	    }
	}

      /* If this command has been pre-hooked, run the hook first.  */
      execute_cmd_pre_hook (c);

      if (c->deprecated_warn_user)
	deprecated_cmd_warning (line, cmdlist);

      /* c->user_commands would be NULL in the case of a python command.  */
      if (c->theclass == class_user && c->user_commands)
	execute_user_command (c, arg);
      else if (c->theclass == class_user
	       && c->is_prefix () && !c->allow_unknown)
	/* If this is a user defined prefix that does not allow unknown
	   (in other words, C is a prefix command and not a command
	   that can be followed by its args), report the list of
	   subcommands.  */
	{
	  std::string prefixname = c->prefixname ();
	  std::string prefixname_no_space
	    = prefixname.substr (0, prefixname.length () - 1);
	  gdb_printf
	    ("\"%s\" must be followed by the name of a subcommand.\n",
	     prefixname_no_space.c_str ());
	  help_list (*c->subcommands, prefixname.c_str (), all_commands,
		     gdb_stdout);
	}
      else if (c->type == set_cmd)
	do_set_command (arg, from_tty, c);
      else if (c->type == show_cmd)
	do_show_command (arg, from_tty, c);
      else if (c->is_command_class_help ())
	error (_("That is not a command, just a help topic."));
      else if (deprecated_call_command_hook)
	deprecated_call_command_hook (c, arg, from_tty);
      else
	cmd_func (c, arg, from_tty);

      maybe_wait_sync_command_done (was_sync);

      /* If this command has been post-hooked, run the hook last.
	 We need to lookup the command again since during its execution,
	 a command may redefine itself.  In this case, C pointer
	 becomes invalid so we need to look it up again.  */
      const char *cmd2 = cmd_copy.c_str ();
      c = lookup_cmd (&cmd2, cmdlist, "", nullptr, 1, 1);
      if (c != nullptr)
	execute_cmd_post_hook (c);

      if (repeat_arguments != NULL && cmd_start == saved_command_line)
	{
	  gdb_assert (strlen (args_pointer) >= strlen (repeat_arguments));
	  strcpy (saved_command_line + (args_pointer - cmd_start),
		  repeat_arguments);
	}
    }

  /* Only perform the frame-language-change check if the command
     we just finished executing did not resume the inferior's execution.
     If it did resume the inferior, we will do that check after
     the inferior stopped.  */
  if (has_stack_frames () && inferior_thread ()->state != THREAD_RUNNING)
    check_frame_language_change ();

  cleanup_if_error.release ();
}

/* See gdbcmd.h.  */

void
execute_fn_to_ui_file (struct ui_file *file, std::function<void(void)> fn)
{
  /* GDB_STDOUT should be better already restored during these
     restoration callbacks.  */
  set_batch_flag_and_restore_page_info save_page_info;

  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  {
    ui_out_redirect_pop redirect_popper (current_uiout, file);

    scoped_restore save_stdout
      = make_scoped_restore (&gdb_stdout, file);
    scoped_restore save_stderr
      = make_scoped_restore (&gdb_stderr, file);
    scoped_restore save_stdlog
      = make_scoped_restore (&gdb_stdlog, file);
    scoped_restore save_stdtarg
      = make_scoped_restore (&gdb_stdtarg, file);
    scoped_restore save_stdtargerr
      = make_scoped_restore (&gdb_stdtargerr, file);

    fn ();
  }
}

/* See gdbcmd.h.  */

void
execute_fn_to_string (std::string &res, std::function<void(void)> fn,
		      bool term_out)
{
  string_file str_file (term_out);

  try
    {
      execute_fn_to_ui_file (&str_file, fn);
    }
  catch (...)
    {
      /* Finally.  */
      res = str_file.release ();
      throw;
    }

  /* And finally.  */
  res = str_file.release ();
}

/* See gdbcmd.h.  */

void
execute_command_to_ui_file (struct ui_file *file,
			    const char *p, int from_tty)
{
  execute_fn_to_ui_file (file, [=]() { execute_command (p, from_tty); });
}

/* See gdbcmd.h.  */

void
execute_command_to_string (std::string &res, const char *p, int from_tty,
			   bool term_out)
{
  execute_fn_to_string (res, [=]() { execute_command (p, from_tty); },
			term_out);
}

/* See gdbcmd.h.  */

void
execute_command_to_string (const char *p, int from_tty,
			   bool term_out)
{
  std::string dummy;
  execute_fn_to_string (dummy, [=]() { execute_command (p, from_tty); },
			term_out);
}

/* When nonzero, cause dont_repeat to do nothing.  This should only be
   set via prevent_dont_repeat.  */

static int suppress_dont_repeat = 0;

/* See command.h  */

void
dont_repeat (void)
{
  struct ui *ui = current_ui;

  if (suppress_dont_repeat || server_command)
    return;

  /* If we aren't reading from standard input, we are saving the last
     thing read from stdin in line and don't want to delete it.  Null
     lines won't repeat here in any case.  */
  if (ui->instream == ui->stdin_stream)
    {
      *saved_command_line = 0;
      repeat_arguments = NULL;
    }
}

/* See command.h  */

const char *
repeat_previous ()
{
  /* Do not repeat this command, as this command is a repeating command.  */
  dont_repeat ();

  /* We cannot free saved_command_line, as this line is being executed,
     so swap it with previous_saved_command_line.  */
  std::swap (previous_saved_command_line, saved_command_line);
  std::swap (previous_repeat_arguments, repeat_arguments);

  const char *prev = skip_spaces (get_saved_command_line ());
  if (*prev == '\0')
    error (_("No previous command to relaunch"));
  return prev;
}

/* See command.h.  */

scoped_restore_tmpl<int>
prevent_dont_repeat (void)
{
  return make_scoped_restore (&suppress_dont_repeat, 1);
}

/* See command.h.  */

char *
get_saved_command_line ()
{
  return saved_command_line;
}

/* See command.h.  */

void
save_command_line (const char *cmd)
{
  xfree (previous_saved_command_line);
  previous_saved_command_line = saved_command_line;
  previous_repeat_arguments = repeat_arguments;
  saved_command_line = xstrdup (cmd);
  repeat_arguments = NULL;
}


/* Read a line from the stream "instream" without command line editing.

   It prints PROMPT once at the start.
   Action is compatible with "readline", e.g. space for the result is
   malloc'd and should be freed by the caller.

   A NULL return means end of file.  */

static gdb::unique_xmalloc_ptr<char>
gdb_readline_no_editing (const char *prompt)
{
  std::string line_buffer;
  struct ui *ui = current_ui;
  /* Read from stdin if we are executing a user defined command.  This
     is the right thing for prompt_for_continue, at least.  */
  FILE *stream = ui->instream != NULL ? ui->instream : stdin;
  int fd = fileno (stream);

  if (prompt != NULL)
    {
      /* Don't use a _filtered function here.  It causes the assumed
	 character position to be off, since the newline we read from
	 the user is not accounted for.  */
      printf_unfiltered ("%s", prompt);
      gdb_flush (gdb_stdout);
    }

  while (1)
    {
      int c;
      fd_set readfds;

      QUIT;

      /* Wait until at least one byte of data is available.  Control-C
	 can interrupt interruptible_select, but not fgetc.  */
      FD_ZERO (&readfds);
      FD_SET (fd, &readfds);
      if (interruptible_select (fd + 1, &readfds, NULL, NULL, NULL) == -1)
	{
	  if (errno == EINTR)
	    {
	      /* If this was ctrl-c, the QUIT above handles it.  */
	      continue;
	    }
	  perror_with_name (("select"));
	}

      c = fgetc (stream);

      if (c == EOF)
	{
	  if (!line_buffer.empty ())
	    /* The last line does not end with a newline.  Return it, and
	       if we are called again fgetc will still return EOF and
	       we'll return NULL then.  */
	    break;
	  return NULL;
	}

      if (c == '\n')
	{
	  if (!line_buffer.empty () && line_buffer.back () == '\r')
	    line_buffer.pop_back ();
	  break;
	}

      line_buffer += c;
    }

  return make_unique_xstrdup (line_buffer.c_str ());
}

/* Variables which control command line editing and history
   substitution.  These variables are given default values at the end
   of this file.  */
static bool command_editing_p;

/* NOTE 1999-04-29: This variable will be static again, once we modify
   gdb to use the event loop as the default command loop and we merge
   event-top.c into this file, top.c.  */

/* static */ bool history_expansion_p;

/* Should we write out the command history on exit?  In order to write out
   the history both this flag must be true, and the history_filename
   variable must be set to something sensible.  */
static bool write_history_p;

/* The name of the file in which GDB history will be written.  If this is
   set to NULL, of the empty string then history will not be written.  */
static std::string history_filename;

/* Implement 'show history save'.  */
static void
show_write_history_p (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  if (!write_history_p || !history_filename.empty ())
    gdb_printf (file, _("Saving of the history record on exit is %s.\n"),
		value);
  else
    gdb_printf (file, _("Saving of the history is disabled due to "
			"the value of 'history filename'.\n"));
}

/* The variable associated with the "set/show history size"
   command.  The value -1 means unlimited, and -2 means undefined.  */
static int history_size_setshow_var = -2;

static void
show_history_size (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("The size of the command history is %s.\n"),
	      value);
}

/* Variable associated with the "history remove-duplicates" option.
   The value -1 means unlimited.  */
static int history_remove_duplicates = 0;

static void
show_history_remove_duplicates (struct ui_file *file, int from_tty,
				struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("The number of history entries to look back at for "
		"duplicates is %s.\n"),
	      value);
}

/* Implement 'show history filename'.  */
static void
show_history_filename (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  if (!history_filename.empty ())
    gdb_printf (file, _("The filename in which to record "
			"the command history is \"%ps\".\n"),
		styled_string (file_name_style.style (), value));
  else
    gdb_printf (file, _("There is no filename currently set for "
			"recording the command history in.\n"));
}

/* This is like readline(), but it has some gdb-specific behavior.
   gdb may want readline in both the synchronous and async modes during
   a single gdb invocation.  At the ordinary top-level prompt we might
   be using the async readline.  That means we can't use
   rl_pre_input_hook, since it doesn't work properly in async mode.
   However, for a secondary prompt (" >", such as occurs during a
   `define'), gdb wants a synchronous response.

   We used to call readline() directly, running it in synchronous
   mode.  But mixing modes this way is not supported, and as of
   readline 5.x it no longer works; the arrow keys come unbound during
   the synchronous call.  So we make a nested call into the event
   loop.  That's what gdb_readline_wrapper is for.  */

/* A flag set as soon as gdb_readline_wrapper_line is called; we can't
   rely on gdb_readline_wrapper_result, which might still be NULL if
   the user types Control-D for EOF.  */
static int gdb_readline_wrapper_done;

/* The result of the current call to gdb_readline_wrapper, once a newline
   is seen.  */
static char *gdb_readline_wrapper_result;

/* Any intercepted hook.  Operate-and-get-next sets this, expecting it
   to be called after the newline is processed (which will redisplay
   the prompt).  But in gdb_readline_wrapper we will not get a new
   prompt until the next call, or until we return to the event loop.
   So we disable this hook around the newline and restore it before we
   return.  */
static void (*saved_after_char_processing_hook) (void);


/* See top.h.  */

int
gdb_in_secondary_prompt_p (struct ui *ui)
{
  return ui->secondary_prompt_depth > 0;
}


/* This function is called when readline has seen a complete line of
   text.  */

static void
gdb_readline_wrapper_line (gdb::unique_xmalloc_ptr<char> &&line)
{
  gdb_assert (!gdb_readline_wrapper_done);
  gdb_readline_wrapper_result = line.release ();
  gdb_readline_wrapper_done = 1;

  /* Prevent operate-and-get-next from acting too early.  */
  saved_after_char_processing_hook = after_char_processing_hook;
  after_char_processing_hook = NULL;

#if defined(TUI)
  if (tui_active)
    tui_inject_newline_into_command_window ();
#endif

  /* Prevent parts of the prompt from being redisplayed if annotations
     are enabled, and readline's state getting out of sync.  We'll
     reinstall the callback handler, which puts the terminal in raw
     mode (or in readline lingo, in prepped state), when we're next
     ready to process user input, either in display_gdb_prompt, or if
     we're handling an asynchronous target event and running in the
     background, just before returning to the event loop to process
     further input (or more target events).  */
  if (current_ui->command_editing)
    gdb_rl_callback_handler_remove ();
}

class gdb_readline_wrapper_cleanup
{
public:
  gdb_readline_wrapper_cleanup ()
    : m_handler_orig (current_ui->input_handler),
      m_already_prompted_orig (current_ui->command_editing
			       ? rl_already_prompted : 0),
      m_target_is_async_orig (target_is_async_p ()),
      m_save_ui (&current_ui)
  {
    current_ui->input_handler = gdb_readline_wrapper_line;
    current_ui->secondary_prompt_depth++;

    if (m_target_is_async_orig)
      target_async (false);
  }

  ~gdb_readline_wrapper_cleanup ()
  {
    struct ui *ui = current_ui;

    if (ui->command_editing)
      rl_already_prompted = m_already_prompted_orig;

    gdb_assert (ui->input_handler == gdb_readline_wrapper_line);
    ui->input_handler = m_handler_orig;

    /* Don't restore our input handler in readline yet.  That would make
       readline prep the terminal (putting it in raw mode), while the
       line we just read may trigger execution of a command that expects
       the terminal in the default cooked/canonical mode, such as e.g.,
       running Python's interactive online help utility.  See
       gdb_readline_wrapper_line for when we'll reinstall it.  */

    gdb_readline_wrapper_result = NULL;
    gdb_readline_wrapper_done = 0;
    ui->secondary_prompt_depth--;
    gdb_assert (ui->secondary_prompt_depth >= 0);

    after_char_processing_hook = saved_after_char_processing_hook;
    saved_after_char_processing_hook = NULL;

    if (m_target_is_async_orig)
      target_async (true);
  }

  DISABLE_COPY_AND_ASSIGN (gdb_readline_wrapper_cleanup);

private:

  void (*m_handler_orig) (gdb::unique_xmalloc_ptr<char> &&);
  int m_already_prompted_orig;

  /* Whether the target was async.  */
  int m_target_is_async_orig;

  /* Processing events may change the current UI.  */
  scoped_restore_tmpl<struct ui *> m_save_ui;
};

char *
gdb_readline_wrapper (const char *prompt)
{
  struct ui *ui = current_ui;

  gdb_readline_wrapper_cleanup cleanup;

  /* Display our prompt and prevent double prompt display.  Don't pass
     down a NULL prompt, since that has special meaning for
     display_gdb_prompt -- it indicates a request to print the primary
     prompt, while we want a secondary prompt here.  */
  display_gdb_prompt (prompt != NULL ? prompt : "");
  if (ui->command_editing)
    rl_already_prompted = 1;

  if (after_char_processing_hook)
    (*after_char_processing_hook) ();
  gdb_assert (after_char_processing_hook == NULL);

  while (gdb_do_one_event () >= 0)
    if (gdb_readline_wrapper_done)
      break;

  return gdb_readline_wrapper_result;
}


/* The current saved history number from operate-and-get-next.
   This is -1 if not valid.  */
static int operate_saved_history = -1;

/* This is put on the appropriate hook and helps operate-and-get-next
   do its work.  */
static void
gdb_rl_operate_and_get_next_completion (void)
{
  int delta = where_history () - operate_saved_history;

  /* The `key' argument to rl_get_previous_history is ignored.  */
  rl_get_previous_history (delta, 0);
  operate_saved_history = -1;

  /* readline doesn't automatically update the display for us.  */
  rl_redisplay ();

  after_char_processing_hook = NULL;
  rl_pre_input_hook = NULL;
}

/* This is a gdb-local readline command handler.  It accepts the
   current command line (like RET does) and, if this command was taken
   from the history, arranges for the next command in the history to
   appear on the command line when the prompt returns.
   We ignore the arguments.  */
static int
gdb_rl_operate_and_get_next (int count, int key)
{
  int where;

  /* Use the async hook.  */
  after_char_processing_hook = gdb_rl_operate_and_get_next_completion;

  /* Find the current line, and find the next line to use.  */
  where = where_history();

  if ((history_is_stifled () && (history_length >= history_max_entries))
      || (where >= history_length - 1))
    operate_saved_history = where;
  else
    operate_saved_history = where + 1;

  return rl_newline (1, key);
}

/* Number of user commands executed during this session.  */

static int command_count = 0;

/* Add the user command COMMAND to the input history list.  */

void
gdb_add_history (const char *command)
{
  command_count++;

  if (history_remove_duplicates != 0)
    {
      int lookbehind;
      int lookbehind_threshold;

      /* The lookbehind threshold for finding a duplicate history entry is
	 bounded by command_count because we can't meaningfully delete
	 history entries that are already stored in the history file since
	 the history file is appended to.  */
      if (history_remove_duplicates == -1
	  || history_remove_duplicates > command_count)
	lookbehind_threshold = command_count;
      else
	lookbehind_threshold = history_remove_duplicates;

      using_history ();
      for (lookbehind = 0; lookbehind < lookbehind_threshold; lookbehind++)
	{
	  HIST_ENTRY *temp = previous_history ();

	  if (temp == NULL)
	    break;

	  if (strcmp (temp->line, command) == 0)
	    {
	      HIST_ENTRY *prev = remove_history (where_history ());
	      command_count--;
	      free_history_entry (prev);
	      break;
	    }
	}
      using_history ();
    }

  add_history (command);
}

/* Safely append new history entries to the history file in a corruption-free
   way using an intermediate local history file.  */

static void
gdb_safe_append_history (void)
{
  int ret, saved_errno;

  std::string local_history_filename
    = string_printf ("%s-gdb%ld~", history_filename.c_str (), (long) getpid ());

  ret = rename (history_filename.c_str (), local_history_filename.c_str ());
  saved_errno = errno;
  if (ret < 0 && saved_errno != ENOENT)
    {
      warning (_("Could not rename %ps to %ps: %s"),
	       styled_string (file_name_style.style (),
			      history_filename.c_str ()),
	       styled_string (file_name_style.style (),
			      local_history_filename.c_str ()),
	       safe_strerror (saved_errno));
    }
  else
    {
      if (ret < 0)
	{
	  /* If the rename failed with ENOENT then either the global history
	     file never existed in the first place or another GDB process is
	     currently appending to it (and has thus temporarily renamed it).
	     Since we can't distinguish between these two cases, we have to
	     conservatively assume the first case and therefore must write out
	     (not append) our known history to our local history file and try
	     to move it back anyway.  Otherwise a global history file would
	     never get created!  */
	   gdb_assert (saved_errno == ENOENT);
	   write_history (local_history_filename.c_str ());
	}
      else
	{
	  append_history (command_count, local_history_filename.c_str ());
	  if (history_is_stifled ())
	    history_truncate_file (local_history_filename.c_str (),
				   history_max_entries);
	}

      ret = rename (local_history_filename.c_str (), history_filename.c_str ());
      saved_errno = errno;
      if (ret < 0 && saved_errno != EEXIST)
	warning (_("Could not rename %s to %s: %s"),
		 local_history_filename.c_str (), history_filename.c_str (),
		 safe_strerror (saved_errno));
    }
}

/* Read one line from the command input stream `instream'.

   CMD_LINE_BUFFER is a buffer that the function may use to store the result, if
   it needs to be dynamically-allocated.  Otherwise, it is unused.string

   Return nullptr for end of file.

   This routine either uses fancy command line editing or simple input
   as the user has requested.  */

const char *
command_line_input (std::string &cmd_line_buffer, const char *prompt_arg,
		    const char *annotation_suffix)
{
  struct ui *ui = current_ui;
  const char *prompt = prompt_arg;
  const char *cmd;
  int from_tty = ui->instream == ui->stdin_stream;

  /* The annotation suffix must be non-NULL.  */
  if (annotation_suffix == NULL)
    annotation_suffix = "";

  if (from_tty && annotation_level > 1)
    {
      char *local_prompt;

      local_prompt
	= (char *) alloca ((prompt == NULL ? 0 : strlen (prompt))
			   + strlen (annotation_suffix) + 40);
      if (prompt == NULL)
	local_prompt[0] = '\0';
      else
	strcpy (local_prompt, prompt);
      strcat (local_prompt, "\n\032\032");
      strcat (local_prompt, annotation_suffix);
      strcat (local_prompt, "\n");

      prompt = local_prompt;
    }

#ifdef SIGTSTP
  if (job_control)
    signal (SIGTSTP, handle_sigtstp);
#endif

  while (1)
    {
      gdb::unique_xmalloc_ptr<char> rl;

      /* Make sure that all output has been output.  Some machines may
	 let you get away with leaving out some of the gdb_flush, but
	 not all.  */
      gdb_flush (gdb_stdout);
      gdb_flush (gdb_stderr);

      if (!source_file_name.empty ())
	++source_line_number;

      if (from_tty && annotation_level > 1)
	printf_unfiltered ("\n\032\032pre-%s\n", annotation_suffix);

      /* Don't use fancy stuff if not talking to stdin.  */
      if (deprecated_readline_hook
	  && from_tty
	  && current_ui->input_interactive_p ())
	{
	  rl.reset ((*deprecated_readline_hook) (prompt));
	}
      else if (command_editing_p
	       && from_tty
	       && current_ui->input_interactive_p ())
	{
	  rl.reset (gdb_readline_wrapper (prompt));
	}
      else
	{
	  rl = gdb_readline_no_editing (prompt);
	}

      cmd = handle_line_of_input (cmd_line_buffer, rl.get (),
				  0, annotation_suffix);
      if (cmd == (char *) EOF)
	{
	  cmd = NULL;
	  break;
	}
      if (cmd != NULL)
	break;

      /* Got partial input.  I.e., got a line that ends with a
	 continuation character (backslash).  Suppress printing the
	 prompt again.  */
      prompt = NULL;
    }

#ifdef SIGTSTP
  if (job_control)
    signal (SIGTSTP, SIG_DFL);
#endif

  return cmd;
}

/* See top.h.  */
void
print_gdb_version (struct ui_file *stream, bool interactive)
{
  /* From GNU coding standards, first line is meant to be easy for a
     program to parse, and is just canonical program name and version
     number, which starts after last space.  */

  std::string v_str = string_printf ("GNU gdb %s%s", PKGVERSION, version);
  gdb_printf (stream, "%ps\n",
	      styled_string (version_style.style (), v_str.c_str ()));

  /* Second line is a copyright notice.  */

  gdb_printf (stream,
	      "Copyright (C) 2024 Free Software Foundation, Inc.\n");

  /* Following the copyright is a brief statement that the program is
     free software, that users are free to copy and change it on
     certain conditions, that it is covered by the GNU GPL, and that
     there is no warranty.  */

  gdb_printf (stream, "\
License GPLv3+: GNU GPL version 3 or later <%ps>\
\nThis is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.",
		    styled_string (file_name_style.style (),
				   "http://gnu.org/licenses/gpl.html"));

  if (!interactive)
    return;

  gdb_printf (stream, ("\nType \"show copying\" and "
		       "\"show warranty\" for details.\n"));

  /* After the required info we print the configuration information.  */

  gdb_printf (stream, "This GDB was configured as \"");
  if (strcmp (host_name, target_name) != 0)
    {
      gdb_printf (stream, "--host=%s --target=%s",
		  host_name, target_name);
    }
  else
    {
      gdb_printf (stream, "%s", host_name);
    }
  gdb_printf (stream, "\".\n");

  gdb_printf (stream, _("Type \"show configuration\" "
			"for configuration details.\n"));

  if (REPORT_BUGS_TO[0])
    {
      gdb_printf (stream,
		  _("For bug reporting instructions, please see:\n"));
      gdb_printf (stream, "%ps.\n",
		  styled_string (file_name_style.style (),
				 REPORT_BUGS_TO));
    }
  gdb_printf (stream,
	      _("Find the GDB manual and other documentation \
resources online at:\n    <%ps>."),
	      styled_string (file_name_style.style (),
			     "http://www.gnu.org/software/gdb/documentation/"));
  gdb_printf (stream, "\n\n");
  gdb_printf (stream, _("For help, type \"help\".\n"));
  gdb_printf (stream,
	      _("Type \"apropos word\" to search for commands \
related to \"word\"."));
}

/* Print the details of GDB build-time configuration.  */
void
print_gdb_configuration (struct ui_file *stream)
{
  gdb_printf (stream, _("\
This GDB was configured as follows:\n\
   configure --host=%s --target=%s\n\
"), host_name, target_name);

  gdb_printf (stream, _("\
	     --with-auto-load-dir=%s\n\
	     --with-auto-load-safe-path=%s\n\
"), AUTO_LOAD_DIR, AUTO_LOAD_SAFE_PATH);

#if HAVE_LIBEXPAT
  gdb_printf (stream, _("\
	     --with-expat\n\
"));
#else
  gdb_printf (stream, _("\
	     --without-expat\n\
"));
#endif

  if (GDB_DATADIR[0])
    gdb_printf (stream, _("\
	     --with-gdb-datadir=%s%s\n\
"), GDB_DATADIR, GDB_DATADIR_RELOCATABLE ? " (relocatable)" : "");

#ifdef ICONV_BIN
  gdb_printf (stream, _("\
	     --with-iconv-bin=%s%s\n\
"), ICONV_BIN, ICONV_BIN_RELOCATABLE ? " (relocatable)" : "");
#endif

  if (JIT_READER_DIR[0])
    gdb_printf (stream, _("\
	     --with-jit-reader-dir=%s%s\n\
"), JIT_READER_DIR, JIT_READER_DIR_RELOCATABLE ? " (relocatable)" : "");

#if HAVE_LIBUNWIND_IA64_H
  gdb_printf (stream, _("\
	     --with-libunwind-ia64\n\
"));
#else
  gdb_printf (stream, _("\
	     --without-libunwind-ia64\n\
"));
#endif

#if HAVE_LIBLZMA
  gdb_printf (stream, _("\
	     --with-lzma\n\
"));
#else
  gdb_printf (stream, _("\
	     --without-lzma\n\
"));
#endif

#if HAVE_LIBBABELTRACE
  gdb_printf (stream, _("\
	     --with-babeltrace\n\
"));
#else
  gdb_printf (stream, _("\
	     --without-babeltrace\n\
"));
#endif

#if HAVE_LIBIPT
  gdb_printf (stream, _("\
	     --with-intel-pt\n\
"));
#else
  gdb_printf (stream, _("\
	     --without-intel-pt\n\
"));
#endif

#if HAVE_LIBXXHASH
  gdb_printf (stream, _("\
	     --with-xxhash\n\
"));
#else
  gdb_printf (stream, _("\
	     --without-xxhash\n\
"));
#endif
#ifdef WITH_PYTHON_PATH
  gdb_printf (stream, _("\
	     --with-python=%s%s\n\
"), WITH_PYTHON_PATH, PYTHON_PATH_RELOCATABLE ? " (relocatable)" : "");
#else
  gdb_printf (stream, _("\
	     --without-python\n\
"));
#endif
#ifdef WITH_PYTHON_LIBDIR
  gdb_printf (stream, _("\
	     --with-python-libdir=%s%s\n\
"), WITH_PYTHON_LIBDIR, PYTHON_LIBDIR_RELOCATABLE ? " (relocatable)" : "");
#else
  gdb_printf (stream, _("\
	     --without-python-libdir\n\
"));
#endif

#if HAVE_LIBDEBUGINFOD
  gdb_printf (stream, _("\
	     --with-debuginfod\n\
"));
#else
  gdb_printf (stream, _("\
	     --without-debuginfod\n\
"));
#endif

#if HAVE_LIBCURSES
  gdb_printf (stream, _("\
	     --with-curses\n\
"));
#else
  gdb_printf (stream, _("\
	     --without-curses\n\
"));
#endif

#if HAVE_GUILE
  gdb_printf (stream, _("\
	     --with-guile\n\
"));
#else
  gdb_printf (stream, _("\
	     --without-guile\n\
"));
#endif

#if HAVE_AMD_DBGAPI
  gdb_printf (stream, _("\
	     --with-amd-dbgapi\n\
"));
#else
  gdb_printf (stream, _("\
	     --without-amd-dbgapi\n\
"));
#endif

#if HAVE_SOURCE_HIGHLIGHT
  gdb_printf (stream, _("\
	     --enable-source-highlight\n\
"));
#else
  gdb_printf (stream, _("\
	     --disable-source-highlight\n\
"));
#endif

#if CXX_STD_THREAD
  gdb_printf (stream, _("\
	     --enable-threading\n\
"));
#else
  gdb_printf (stream, _("\
	     --disable-threading\n\
"));
#endif

#ifdef TUI
  gdb_printf (stream, _("\
	     --enable-tui\n\
"));
#else
  gdb_printf (stream, _("\
	     --disable-tui\n\
"));
#endif

#ifdef HAVE_READLINE_READLINE_H
  gdb_printf (stream, _("\
	     --with-system-readline\n\
"));
#else
  gdb_printf (stream, _("\
	     --without-system-readline\n\
"));
#endif

#ifdef RELOC_SRCDIR
  gdb_printf (stream, _("\
	     --with-relocated-sources=%s\n\
"), RELOC_SRCDIR);
#endif

  if (DEBUGDIR[0])
    gdb_printf (stream, _("\
	     --with-separate-debug-dir=%s%s\n\
"), DEBUGDIR, DEBUGDIR_RELOCATABLE ? " (relocatable)" : "");

#ifdef ADDITIONAL_DEBUG_DIRS
  gdb_printf (stream, _ ("\
	     --with-additional-debug-dirs=%s\n\
"), ADDITIONAL_DEBUG_DIRS);
#endif

  if (TARGET_SYSTEM_ROOT[0])
    gdb_printf (stream, _("\
	     --with-sysroot=%s%s\n\
"), TARGET_SYSTEM_ROOT, TARGET_SYSTEM_ROOT_RELOCATABLE ? " (relocatable)" : "");

  if (SYSTEM_GDBINIT[0])
    gdb_printf (stream, _("\
	     --with-system-gdbinit=%s%s\n\
"), SYSTEM_GDBINIT, SYSTEM_GDBINIT_RELOCATABLE ? " (relocatable)" : "");

  if (SYSTEM_GDBINIT_DIR[0])
    gdb_printf (stream, _("\
	     --with-system-gdbinit-dir=%s%s\n\
"), SYSTEM_GDBINIT_DIR, SYSTEM_GDBINIT_DIR_RELOCATABLE ? " (relocatable)" : "");

  /* We assume "relocatable" will be printed at least once, thus we always
     print this text.  It's a reasonably safe assumption for now.  */
  gdb_printf (stream, _("\n\
(\"Relocatable\" means the directory can be moved with the GDB installation\n\
tree, and GDB will still find it.)\n\
"));
}


/* The current top level prompt, settable with "set prompt", and/or
   with the python `gdb.prompt_hook' hook.  */
static std::string top_prompt;

/* Access method for the GDB prompt string.  */

const std::string &
get_prompt ()
{
  return top_prompt;
}

/* Set method for the GDB prompt string.  */

void
set_prompt (const char *s)
{
  top_prompt = s;
}


/* Kills or detaches the given inferior, depending on how we originally
   gained control of it.  */

static void
kill_or_detach (inferior *inf, int from_tty)
{
  if (inf->pid == 0)
    return;

  thread_info *thread = any_thread_of_inferior (inf);
  if (thread != NULL)
    {
      switch_to_thread (thread);

      /* Leave core files alone.  */
      if (target_has_execution ())
	{
	  if (inf->attach_flag)
	    target_detach (inf, from_tty);
	  else
	    target_kill ();
	}
    }
}

/* Prints info about what GDB will do to inferior INF on a "quit".  OUT is
   where to collect the output.  */

static void
print_inferior_quit_action (inferior *inf, ui_file *out)
{
  if (inf->pid == 0)
    return;

  if (inf->attach_flag)
    gdb_printf (out,
		_("\tInferior %d [%s] will be detached.\n"), inf->num,
		target_pid_to_str (ptid_t (inf->pid)).c_str ());
  else
    gdb_printf (out,
		_("\tInferior %d [%s] will be killed.\n"), inf->num,
		target_pid_to_str (ptid_t (inf->pid)).c_str ());
}

/* If necessary, make the user confirm that we should quit.  Return
   non-zero if we should quit, zero if we shouldn't.  */

int
quit_confirm (void)
{
  /* Don't even ask if we're only debugging a core file inferior.  */
  if (!have_live_inferiors ())
    return 1;

  /* Build the query string as a single string.  */
  string_file stb;

  stb.puts (_("A debugging session is active.\n\n"));

  for (inferior *inf : all_inferiors ())
    print_inferior_quit_action (inf, &stb);

  stb.puts (_("\nQuit anyway? "));

  return query ("%s", stb.c_str ());
}

/* Prepare to exit GDB cleanly by undoing any changes made to the
   terminal so that we leave the terminal in the state we acquired it.  */

static void
undo_terminal_modifications_before_exit (void)
{
  struct ui *saved_top_level = current_ui;

  target_terminal::ours ();

  current_ui = main_ui;

#if defined(TUI)
  tui_disable ();
#endif
  gdb_disable_readline ();

  current_ui = saved_top_level;
}


/* Quit without asking for confirmation.  */

void
quit_force (int *exit_arg, int from_tty)
{
  int exit_code = 0;

  /* Clear the quit flag and sync_quit_force_run so that a
     gdb_exception_forced_quit isn't inadvertently triggered by a QUIT
     check while running the various cleanup/exit code below.  Note
     that the call to 'check_quit_flag' clears the quit flag as a side
     effect.  */
  check_quit_flag ();
  sync_quit_force_run = false;

  /* An optional expression may be used to cause gdb to terminate with the
     value of that expression.  */
  if (exit_arg)
    exit_code = *exit_arg;
  else if (return_child_result)
    exit_code = return_child_result_value;

  gdb::observers::gdb_exiting.notify (exit_code);

  undo_terminal_modifications_before_exit ();

  /* We want to handle any quit errors and exit regardless.  */

  /* Get out of tfind mode, and kill or detach all inferiors.  */
  try
    {
      disconnect_tracing ();
      for (inferior *inf : all_inferiors ())
	kill_or_detach (inf, from_tty);
    }
  catch (const gdb_exception &ex)
    {
      exception_print (gdb_stderr, ex);
    }

  /* Give all pushed targets a chance to do minimal cleanup, and pop
     them all out.  */
  for (inferior *inf : all_inferiors ())
    {
      try
	{
	  inf->pop_all_targets ();
	}
      catch (const gdb_exception &ex)
	{
	  exception_print (gdb_stderr, ex);
	}
    }

  /* Save the history information if it is appropriate to do so.  */
  try
    {
      if (write_history_p && !history_filename.empty ())
	{
	  int save = 0;

	  /* History is currently shared between all UIs.  If there's
	     any UI with a terminal, save history.  */
	  for (ui *ui : all_uis ())
	    {
	      if (ui->input_interactive_p ())
		{
		  save = 1;
		  break;
		}
	    }

	  if (save)
	    gdb_safe_append_history ();
	}
    }
  catch (const gdb_exception &ex)
    {
      exception_print (gdb_stderr, ex);
    }

  /* Destroy any values currently allocated now instead of leaving it
     to global destructors, because that may be too late.  For
     example, the destructors of xmethod values call into the Python
     runtime, which is finalized via a final cleanup.  */
  finalize_values ();

  /* Do any final cleanups before exiting.  */
  try
    {
      do_final_cleanups ();
    }
  catch (const gdb_exception &ex)
    {
      exception_print (gdb_stderr, ex);
    }

  exit (exit_code);
}

/* See top.h.  */

auto_boolean interactive_mode = AUTO_BOOLEAN_AUTO;

/* Implement the "show interactive-mode" option.  */

static void
show_interactive_mode (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c,
		       const char *value)
{
  if (interactive_mode == AUTO_BOOLEAN_AUTO)
    gdb_printf (file, "Debugger's interactive mode "
		"is %s (currently %s).\n",
		value, current_ui->input_interactive_p () ? "on" : "off");
  else
    gdb_printf (file, "Debugger's interactive mode is %s.\n", value);
}

static void
dont_repeat_command (const char *ignored, int from_tty)
{
  /* Can't call dont_repeat here because we're not necessarily reading
     from stdin.  */
  *saved_command_line = 0;
}

/* Functions to manipulate command line editing control variables.  */

/* Number of commands to print in each call to show_commands.  */
#define Hist_print 10
void
show_commands (const char *args, int from_tty)
{
  /* Index for history commands.  Relative to history_base.  */
  int offset;

  /* Number of the history entry which we are planning to display next.
     Relative to history_base.  */
  static int num = 0;

  /* Print out some of the commands from the command history.  */

  if (args)
    {
      if (args[0] == '+' && args[1] == '\0')
	/* "info editing +" should print from the stored position.  */
	;
      else
	/* "info editing <exp>" should print around command number <exp>.  */
	num = (parse_and_eval_long (args) - history_base) - Hist_print / 2;
    }
  /* "show commands" means print the last Hist_print commands.  */
  else
    {
      num = history_length - Hist_print;
    }

  if (num < 0)
    num = 0;

  /* If there are at least Hist_print commands, we want to display the last
     Hist_print rather than, say, the last 6.  */
  if (history_length - num < Hist_print)
    {
      num = history_length - Hist_print;
      if (num < 0)
	num = 0;
    }

  for (offset = num;
       offset < num + Hist_print && offset < history_length;
       offset++)
    {
      gdb_printf ("%5d  %s\n", history_base + offset,
		  (history_get (history_base + offset))->line);
    }

  /* The next command we want to display is the next one that we haven't
     displayed yet.  */
  num += Hist_print;

  /* If the user repeats this command with return, it should do what
     "show commands +" does.  This is unnecessary if arg is null,
     because "show commands +" is not useful after "show commands".  */
  if (from_tty && args)
    set_repeat_arguments ("+");
}

/* Update the size of our command history file to HISTORY_SIZE.

   A HISTORY_SIZE of -1 stands for unlimited.  */

static void
set_readline_history_size (int history_size)
{
  gdb_assert (history_size >= -1);

  if (history_size == -1)
    unstifle_history ();
  else
    stifle_history (history_size);
}

/* Called by do_setshow_command.  */
static void
set_history_size_command (const char *args,
			  int from_tty, struct cmd_list_element *c)
{
  set_readline_history_size (history_size_setshow_var);
}

bool info_verbose = false;	/* Default verbose msgs off.  */

/* Called by do_set_command.  An elaborate joke.  */
void
set_verbose (const char *args, int from_tty, struct cmd_list_element *c)
{
  const char *cmdname = "verbose";
  struct cmd_list_element *showcmd;

  showcmd = lookup_cmd_1 (&cmdname, showlist, NULL, NULL, 1);
  gdb_assert (showcmd != NULL && showcmd != CMD_LIST_AMBIGUOUS);

  if (c->doc && c->doc_allocated)
    xfree ((char *) c->doc);
  if (showcmd->doc && showcmd->doc_allocated)
    xfree ((char *) showcmd->doc);
  if (info_verbose)
    {
      c->doc = _("Set verbose printing of informational messages.");
      showcmd->doc = _("Show verbose printing of informational messages.");
    }
  else
    {
      c->doc = _("Set verbosity.");
      showcmd->doc = _("Show verbosity.");
    }
  c->doc_allocated = 0;
  showcmd->doc_allocated = 0;
}

/* Init the history buffer.  Note that we are called after the init file(s)
   have been read so that the user can change the history file via his
   .gdbinit file (for instance).  The GDBHISTFILE environment variable
   overrides all of this.  */

void
init_history (void)
{
  const char *tmpenv;

  tmpenv = getenv ("GDBHISTSIZE");
  if (tmpenv)
    {
      long var;
      int saved_errno;
      char *endptr;

      tmpenv = skip_spaces (tmpenv);
      errno = 0;
      var = strtol (tmpenv, &endptr, 10);
      saved_errno = errno;
      endptr = skip_spaces (endptr);

      /* If GDBHISTSIZE is non-numeric then ignore it.  If GDBHISTSIZE is the
	 empty string, a negative number or a huge positive number (larger than
	 INT_MAX) then set the history size to unlimited.  Otherwise set our
	 history size to the number we have read.  This behavior is consistent
	 with how bash handles HISTSIZE.  */
      if (*endptr != '\0')
	;
      else if (*tmpenv == '\0'
	       || var < 0
	       || var > INT_MAX
	       /* On targets where INT_MAX == LONG_MAX, we have to look at
		  errno after calling strtol to distinguish between a value that
		  is exactly INT_MAX and an overflowing value that was clamped
		  to INT_MAX.  */
	       || (var == INT_MAX && saved_errno == ERANGE))
	history_size_setshow_var = -1;
      else
	history_size_setshow_var = var;
    }

  /* If neither the init file nor GDBHISTSIZE has set a size yet, pick the
     default.  */
  if (history_size_setshow_var == -2)
    history_size_setshow_var = 256;

  set_readline_history_size (history_size_setshow_var);

  if (!history_filename.empty ())
    read_history (history_filename.c_str ());
}

static void
show_prompt (struct ui_file *file, int from_tty,
	     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Gdb's prompt is \"%s\".\n"), value);
}

/* "set editing" command.  */

static void
set_editing (const char *args, int from_tty, struct cmd_list_element *c)
{
  change_line_handler (set_editing_cmd_var);
  /* Update the control variable so that MI's =cmd-param-changed event
     shows the correct value. */
  set_editing_cmd_var = current_ui->command_editing;
}

static void
show_editing (struct ui_file *file, int from_tty,
	      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Editing of command lines as "
		      "they are typed is %s.\n"),
	      current_ui->command_editing ? _("on") : _("off"));
}

static void
show_annotation_level (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Annotation_level is %s.\n"), value);
}

static void
show_exec_done_display_p (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Notification of completion for "
		      "asynchronous execution commands is %s.\n"),
	      value);
}

/* New values of the "data-directory" parameter are staged here.
   Extension languages, for example Python's gdb.parameter API, will read
   the value directory from this variable, so we must ensure that this
   always contains the correct value.  */
static std::string staged_gdb_datadir;

/* "set" command for the gdb_datadir configuration variable.  */

static void
set_gdb_datadir (const char *args, int from_tty, struct cmd_list_element *c)
{
  set_gdb_data_directory (staged_gdb_datadir.c_str ());

  /* SET_GDB_DATA_DIRECTORY will resolve relative paths in
     STAGED_GDB_DATADIR, so we now copy the value from GDB_DATADIR
     back into STAGED_GDB_DATADIR so the extension languages can read the
     correct value.  */
  staged_gdb_datadir = gdb_datadir;

  gdb::observers::gdb_datadir_changed.notify ();
}

/* "show" command for the gdb_datadir configuration variable.  */

static void
show_gdb_datadir (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("GDB's data directory is \"%ps\".\n"),
	      styled_string (file_name_style.style (),
			     gdb_datadir.c_str ()));
}

/* Implement 'set history filename'.  */

static void
set_history_filename (const char *args,
		      int from_tty, struct cmd_list_element *c)
{
  /* We include the current directory so that if the user changes
     directories the file written will be the same as the one
     that was read.  */
  if (!history_filename.empty ()
      && !IS_ABSOLUTE_PATH (history_filename.c_str ()))
    history_filename = gdb_abspath (history_filename.c_str ());
}

/* Whether we're in quiet startup mode.  */

static bool startup_quiet;

/* See top.h.  */

bool
check_quiet_mode ()
{
  return startup_quiet;
}

/* Show whether GDB should start up in quiet mode.  */

static void
show_startup_quiet (struct ui_file *file, int from_tty,
	      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Whether to start up quietly is %s.\n"),
	      value);
}

static void
init_main (void)
{
  /* Initialize the prompt to a simple "(gdb) " prompt or to whatever
     the DEFAULT_PROMPT is.  */
  set_prompt (DEFAULT_PROMPT);

  /* Set the important stuff up for command editing.  */
  command_editing_p = 1;
  history_expansion_p = 0;
  write_history_p = 0;

  /* Setup important stuff for command line editing.  */
  rl_completion_word_break_hook = gdb_completion_word_break_characters;
  rl_attempted_completion_function = gdb_rl_attempted_completion_function;
  set_rl_completer_word_break_characters (default_word_break_characters ());
  rl_completer_quote_characters = get_gdb_completer_quote_characters ();
  rl_completion_display_matches_hook = cli_display_match_list;
  rl_readline_name = "gdb";
  rl_terminal_name = getenv ("TERM");
  rl_deprep_term_function = gdb_rl_deprep_term_function;

  /* The name for this defun comes from Bash, where it originated.
     15 is Control-o, the same binding this function has in Bash.  */
  rl_add_defun ("operate-and-get-next", gdb_rl_operate_and_get_next, 15);

  add_setshow_string_cmd ("prompt", class_support,
			  &top_prompt,
			  _("Set gdb's prompt."),
			  _("Show gdb's prompt."),
			  NULL, NULL,
			  show_prompt,
			  &setlist, &showlist);

  add_com ("dont-repeat", class_support, dont_repeat_command, _("\
Don't repeat this command.\nPrimarily \
used inside of user-defined commands that should not be repeated when\n\
hitting return."));

  add_setshow_boolean_cmd ("editing", class_support,
			   &set_editing_cmd_var, _("\
Set editing of command lines as they are typed."), _("\
Show editing of command lines as they are typed."), _("\
Use \"on\" to enable the editing, and \"off\" to disable it.\n\
Without an argument, command line editing is enabled.  To edit, use\n\
EMACS-like or VI-like commands like control-P or ESC."),
			   set_editing,
			   show_editing,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("save", no_class, &write_history_p, _("\
Set saving of the history record on exit."), _("\
Show saving of the history record on exit."), _("\
Use \"on\" to enable the saving, and \"off\" to disable it.\n\
Without an argument, saving is enabled."),
			   NULL,
			   show_write_history_p,
			   &sethistlist, &showhistlist);

  add_setshow_zuinteger_unlimited_cmd ("size", no_class,
				       &history_size_setshow_var, _("\
Set the size of the command history."), _("\
Show the size of the command history."), _("\
This is the number of previous commands to keep a record of.\n\
If set to \"unlimited\", the number of commands kept in the history\n\
list is unlimited.  This defaults to the value of the environment\n\
variable \"GDBHISTSIZE\", or to 256 if this variable is not set."),
			    set_history_size_command,
			    show_history_size,
			    &sethistlist, &showhistlist);

  add_setshow_zuinteger_unlimited_cmd ("remove-duplicates", no_class,
				       &history_remove_duplicates, _("\
Set how far back in history to look for and remove duplicate entries."), _("\
Show how far back in history to look for and remove duplicate entries."), _("\
If set to a nonzero value N, GDB will look back at the last N history entries\n\
and remove the first history entry that is a duplicate of the most recent\n\
entry, each time a new history entry is added.\n\
If set to \"unlimited\", this lookbehind is unbounded.\n\
Only history entries added during this session are considered for removal.\n\
If set to 0, removal of duplicate history entries is disabled.\n\
By default this option is set to 0."),
			   NULL,
			   show_history_remove_duplicates,
			   &sethistlist, &showhistlist);

  add_setshow_optional_filename_cmd ("filename", no_class, &history_filename, _("\
Set the filename in which to record the command history."), _("\
Show the filename in which to record the command history."), _("\
(the list of previous commands of which a record is kept)."),
			    set_history_filename,
			    show_history_filename,
			    &sethistlist, &showhistlist);

  add_setshow_boolean_cmd ("confirm", class_support, &confirm, _("\
Set whether to confirm potentially dangerous operations."), _("\
Show whether to confirm potentially dangerous operations."), NULL,
			   NULL,
			   show_confirm,
			   &setlist, &showlist);

  add_setshow_zinteger_cmd ("annotate", class_obscure, &annotation_level, _("\
Set annotation_level."), _("\
Show annotation_level."), _("\
0 == normal;     1 == fullname (for use when running under emacs)\n\
2 == output annotated suitably for use by programs that control GDB."),
			    NULL,
			    show_annotation_level,
			    &setlist, &showlist);

  add_setshow_boolean_cmd ("exec-done-display", class_support,
			   &exec_done_display_p, _("\
Set notification of completion for asynchronous execution commands."), _("\
Show notification of completion for asynchronous execution commands."), _("\
Use \"on\" to enable the notification, and \"off\" to disable it."),
			   NULL,
			   show_exec_done_display_p,
			   &setlist, &showlist);

  add_setshow_filename_cmd ("data-directory", class_maintenance,
			   &staged_gdb_datadir, _("Set GDB's data directory."),
			   _("Show GDB's data directory."),
			   _("\
When set, GDB uses the specified path to search for data files."),
			   set_gdb_datadir, show_gdb_datadir,
			   &setlist,
			    &showlist);
  /* Prime the initial value for data-directory.  */
  staged_gdb_datadir = gdb_datadir;

  add_setshow_auto_boolean_cmd ("interactive-mode", class_support,
				&interactive_mode, _("\
Set whether GDB's standard input is a terminal."), _("\
Show whether GDB's standard input is a terminal."), _("\
If on, GDB assumes that standard input is a terminal.  In practice, it\n\
means that GDB should wait for the user to answer queries associated to\n\
commands entered at the command prompt.  If off, GDB assumes that standard\n\
input is not a terminal, and uses the default answer to all queries.\n\
If auto (the default), determine which mode to use based on the standard\n\
input settings."),
			NULL,
			show_interactive_mode,
			&setlist, &showlist);

  add_setshow_boolean_cmd ("startup-quietly", class_support,
			       &startup_quiet, _("\
Set whether GDB should start up quietly."), _("		\
Show whether GDB should start up quietly."), _("\
This setting will not affect the current session.  Instead this command\n\
should be added to the .gdbearlyinit file in the users home directory to\n\
affect future GDB sessions."),
			       NULL,
			       show_startup_quiet,
			       &setlist, &showlist);

  struct internalvar *major_version_var = create_internalvar ("_gdb_major");
  struct internalvar *minor_version_var = create_internalvar ("_gdb_minor");
  int vmajor = 0, vminor = 0, vrevision = 0;
  sscanf (version, "%d.%d.%d", &vmajor, &vminor, &vrevision);
  set_internalvar_integer (major_version_var, vmajor);
  set_internalvar_integer (minor_version_var, vminor + (vrevision > 0));
}

/* See top.h.  */

void
gdb_init ()
{
  saved_command_line = xstrdup ("");
  previous_saved_command_line = xstrdup ("");

  /* Run the init function of each source file.  */

#ifdef __MSDOS__
  /* Make sure we return to the original directory upon exit, come
     what may, since the OS doesn't do that for us.  */
  make_final_cleanup (do_chdir_cleanup, xstrdup (current_directory));
#endif

  init_page_info ();

  /* Here is where we call all the _initialize_foo routines.  */
  initialize_all_files ();

  /* This creates the current_program_space.  Do this after all the
     _initialize_foo routines have had a chance to install their
     per-sspace data keys.  Also do this before
     initialize_current_architecture is called, because it accesses
     exec_bfd of the current program space.  */
  initialize_progspace ();
  initialize_inferiors ();
  initialize_current_architecture ();
  init_main ();			/* But that omits this file!  Do it now.  */

  initialize_stdin_serial ();

  /* Take a snapshot of our tty state before readline/ncurses have had a chance
     to alter it.  */
  set_initial_gdb_ttystate ();

  gdb_init_signals ();

  /* We need a default language for parsing expressions, so simple
     things like "set width 0" won't fail if no language is explicitly
     set in a config file or implicitly set by reading an executable
     during startup.  */
  set_language (language_c);
  expected_language = current_language;	/* Don't warn about the change.  */
}

void _initialize_top ();
void
_initialize_top ()
{
  /* Determine a default value for the history filename.  */
  const char *tmpenv = getenv ("GDBHISTFILE");
  if (tmpenv != nullptr)
    history_filename = tmpenv;
  else
    {
      /* We include the current directory so that if the user changes
	 directories the file written will be the same as the one
	 that was read.  */
#ifdef __MSDOS__
      /* No leading dots in file names are allowed on MSDOS.  */
      const char *fname = "_gdb_history";
#else
      const char *fname = ".gdb_history";
#endif

      history_filename = gdb_abspath (fname);
    }
}
