/* General utility routines for GDB, the GNU debugger.

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
#include <ctype.h>
#include "gdbsupport/gdb_wait.h"
#include "event-top.h"
#include "gdbthread.h"
#include "fnmatch.h"
#include "gdb_bfd.h"
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif /* HAVE_SYS_RESOURCE_H */

#ifdef TUI
/* For tui_get_command_dimension and tui_disable.   */
#include "tui/tui.h"
#endif

#ifdef __GO32__
#include <pc.h>
#endif

#include <signal.h>
#include "gdbcmd.h"
#include "serial.h"
#include "bfd.h"
#include "target.h"
#include "gdb-demangle.h"
#include "expression.h"
#include "language.h"
#include "charset.h"
#include "annotate.h"
#include "filenames.h"
#include "symfile.h"
#include "gdbsupport/gdb_obstack.h"
#include "gdbcore.h"
#include "top.h"
#include "ui.h"
#include "main.h"
#include "solist.h"

#include "inferior.h"

#include "gdb_curses.h"

#include "readline/readline.h"

#include <chrono>

#include "interps.h"
#include "gdbsupport/gdb_regex.h"
#include "gdbsupport/job-control.h"
#include "gdbsupport/selftest.h"
#include <optional>
#include "cp-support.h"
#include <algorithm>
#include "gdbsupport/pathstuff.h"
#include "cli/cli-style.h"
#include "gdbsupport/scope-exit.h"
#include "gdbarch.h"
#include "cli-out.h"
#include "gdbsupport/gdb-safe-ctype.h"
#include "bt-utils.h"
#include "gdbsupport/buildargv.h"
#include "pager.h"
#include "run-on-main-thread.h"

void (*deprecated_error_begin_hook) (void);

/* Prototypes for local functions */

static void set_screen_size (void);
static void set_width (void);

/* Time spent in prompt_for_continue in the currently executing command
   waiting for user to respond.
   Initialized in make_command_stats_cleanup.
   Modified in prompt_for_continue and defaulted_query.
   Used in report_command_stats.  */

static std::chrono::steady_clock::duration prompt_for_continue_wait_time;

/* A flag indicating whether to timestamp debugging messages.  */

bool debug_timestamp = false;

/* True means that strings with character values >0x7F should be printed
   as octal escapes.  False means just print the value (e.g. it's an
   international character, and the terminal or window can cope.)  */

bool sevenbit_strings = false;
static void
show_sevenbit_strings (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Printing of 8-bit characters "
		      "in strings as \\nnn is %s.\n"),
	      value);
}

/* String to be printed before warning messages, if any.  */

const char *warning_pre_print = "\nwarning: ";

bool pagination_enabled = true;
static void
show_pagination_enabled (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("State of pagination is %s.\n"), value);
}




/* Print a warning message.  The first argument STRING is the warning
   message, used as an fprintf format string, the second is the
   va_list of arguments for that string.  A warning is unfiltered (not
   paginated) so that the user does not need to page through each
   screen full of warnings when there are lots of them.  */

void
vwarning (const char *string, va_list args)
{
  if (deprecated_warning_hook)
    (*deprecated_warning_hook) (string, args);
  else
    {
      std::optional<target_terminal::scoped_restore_terminal_state> term_state;
      if (target_supports_terminal_ours ())
	{
	  term_state.emplace ();
	  target_terminal::ours_for_output ();
	}
      if (warning_pre_print)
	gdb_puts (warning_pre_print, gdb_stderr);
      gdb_vprintf (gdb_stderr, string, args);
      gdb_printf (gdb_stderr, "\n");
    }
}

/* Print an error message and return to command level.
   The first argument STRING is the error message, used as a fprintf string,
   and the remaining args are passed as arguments to it.  */

void
verror (const char *string, va_list args)
{
  throw_verror (GENERIC_ERROR, string, args);
}

/* Emit a message and abort.  */

static void ATTRIBUTE_NORETURN
abort_with_message (const char *msg)
{
  if (current_ui == NULL)
    fputs (msg, stderr);
  else
    gdb_puts (msg, gdb_stderr);

  abort ();		/* ARI: abort */
}

/* Dump core trying to increase the core soft limit to hard limit first.  */

void
dump_core (void)
{
#ifdef HAVE_SETRLIMIT
  struct rlimit rlim = { (rlim_t) RLIM_INFINITY, (rlim_t) RLIM_INFINITY };

  setrlimit (RLIMIT_CORE, &rlim);
#endif /* HAVE_SETRLIMIT */

  /* Ensure that the SIGABRT we're about to raise will immediately cause
     GDB to exit and dump core, we don't want to trigger GDB's printing of
     a backtrace to the console here.  */
  signal (SIGABRT, SIG_DFL);

  abort ();		/* ARI: abort */
}

/* Check whether GDB will be able to dump core using the dump_core
   function.  Returns zero if GDB cannot or should not dump core.
   If LIMIT_KIND is LIMIT_CUR the user's soft limit will be respected.
   If LIMIT_KIND is LIMIT_MAX only the hard limit will be respected.  */

int
can_dump_core (enum resource_limit_kind limit_kind)
{
#ifdef HAVE_GETRLIMIT
  struct rlimit rlim;

  /* Be quiet and assume we can dump if an error is returned.  */
  if (getrlimit (RLIMIT_CORE, &rlim) != 0)
    return 1;

  switch (limit_kind)
    {
    case LIMIT_CUR:
      if (rlim.rlim_cur == 0)
	return 0;
      [[fallthrough]];

    case LIMIT_MAX:
      if (rlim.rlim_max == 0)
	return 0;
    }
#endif /* HAVE_GETRLIMIT */

  return 1;
}

/* Print a warning that we cannot dump core.  */

void
warn_cant_dump_core (const char *reason)
{
  gdb_printf (gdb_stderr,
	      _("%s\nUnable to dump core, use `ulimit -c"
		" unlimited' before executing GDB next time.\n"),
	      reason);
}

/* Check whether GDB will be able to dump core using the dump_core
   function, and print a warning if we cannot.  */

static int
can_dump_core_warn (enum resource_limit_kind limit_kind,
		    const char *reason)
{
  int core_dump_allowed = can_dump_core (limit_kind);

  if (!core_dump_allowed)
    warn_cant_dump_core (reason);

  return core_dump_allowed;
}

/* Allow the user to configure the debugger behavior with respect to
   what to do when an internal problem is detected.  */

const char internal_problem_ask[] = "ask";
const char internal_problem_yes[] = "yes";
const char internal_problem_no[] = "no";
static const char *const internal_problem_modes[] =
{
  internal_problem_ask,
  internal_problem_yes,
  internal_problem_no,
  NULL
};

/* Data structure used to control how the internal_vproblem function
   should behave.  An instance of this structure is created for each
   problem type that GDB supports.  */

struct internal_problem
{
  /* The name of this problem type.  This must not contain white space as
     this string is used to build command names.  */
  const char *name;

  /* When this is true then a user command is created (based on NAME) that
     allows the SHOULD_QUIT field to be modified, otherwise, SHOULD_QUIT
     can't be changed from its default value by the user.  */
  bool user_settable_should_quit;

  /* Reference a value from internal_problem_modes to indicate if GDB
     should quit when it hits a problem of this type.  */
  const char *should_quit;

  /* Like USER_SETTABLE_SHOULD_QUIT but for SHOULD_DUMP_CORE.  */
  bool user_settable_should_dump_core;

  /* Like SHOULD_QUIT, but whether GDB should dump core.  */
  const char *should_dump_core;

  /* Like USER_SETTABLE_SHOULD_QUIT but for SHOULD_PRINT_BACKTRACE.  */
  bool user_settable_should_print_backtrace;

  /* When this is true GDB will print a backtrace when a problem of this
     type is encountered.  */
  bool should_print_backtrace;
};

/* Return true if the readline callbacks have been initialized for UI.
   This is always true once GDB is fully initialized, but during the early
   startup phase this is initially false.  */

static bool
readline_initialized (struct ui *ui)
{
  return ui->call_readline != nullptr;
}

/* Report a problem, internal to GDB, to the user.  Once the problem
   has been reported, and assuming GDB didn't quit, the caller can
   either allow execution to resume or throw an error.  */

static void ATTRIBUTE_PRINTF (4, 0)
internal_vproblem (struct internal_problem *problem,
		   const char *file, int line, const char *fmt, va_list ap)
{
  static int dejavu;
  int quit_p;
  int dump_core_p;
  std::string reason;

  /* Don't allow infinite error/warning recursion.  */
  {
    static const char msg[] = "Recursive internal problem.\n";

    switch (dejavu)
      {
      case 0:
	dejavu = 1;
	break;
      case 1:
	dejavu = 2;
	abort_with_message (msg);
      default:
	dejavu = 3;
	/* Newer GLIBC versions put the warn_unused_result attribute
	   on write, but this is one of those rare cases where
	   ignoring the return value is correct.  Casting to (void)
	   does not fix this problem.  This is the solution suggested
	   at http://gcc.gnu.org/bugzilla/show_bug.cgi?id=25509.  */
	if (write (STDERR_FILENO, msg, sizeof (msg)) != sizeof (msg))
	  abort (); /* ARI: abort */
	exit (1);
      }
  }

#ifdef TUI
  tui_disable ();
#endif

  /* Create a string containing the full error/warning message.  Need
     to call query with this full string, as otherwize the reason
     (error/warning) and question become separated.  Format using a
     style similar to a compiler error message.  Include extra detail
     so that the user knows that they are living on the edge.  */
  {
    std::string msg = string_vprintf (fmt, ap);
    reason = string_printf ("%s:%d: %s: %s\n"
			    "A problem internal to GDB has been detected,\n"
			    "further debugging may prove unreliable.",
			    file, line, problem->name, msg.c_str ());
  }

  /* Fall back to abort_with_message if gdb_stderr is not set up.  */
  if (current_ui == NULL)
    {
      fputs (reason.c_str (), stderr);
      abort_with_message ("\n");
    }

  /* Try to get the message out and at the start of a new line.  */
  std::optional<target_terminal::scoped_restore_terminal_state> term_state;
  if (target_supports_terminal_ours ())
    {
      term_state.emplace ();
      target_terminal::ours_for_output ();
    }
  if (filtered_printing_initialized ())
    begin_line ();

  /* Emit the message unless query will emit it below.  */
  if (problem->should_quit != internal_problem_ask
      || !confirm
      || !filtered_printing_initialized ()
      || !readline_initialized (current_ui)
      || problem->should_print_backtrace)
    gdb_printf (gdb_stderr, "%s\n", reason.c_str ());

  if (problem->should_print_backtrace)
    gdb_internal_backtrace ();

  if (problem->should_quit == internal_problem_ask)
    {
      /* Default (yes/batch case) is to quit GDB.  When in batch mode
	 this lessens the likelihood of GDB going into an infinite
	 loop.  */
      if (!confirm || !filtered_printing_initialized ()
	  || !readline_initialized (current_ui))
	quit_p = 1;
      else
	quit_p = query (_("%s\nQuit this debugging session? "),
			reason.c_str ());
    }
  else if (problem->should_quit == internal_problem_yes)
    quit_p = 1;
  else if (problem->should_quit == internal_problem_no)
    quit_p = 0;
  else
    internal_error (_("bad switch"));

  gdb_puts (_("\nThis is a bug, please report it."), gdb_stderr);
  if (REPORT_BUGS_TO[0])
    gdb_printf (gdb_stderr, _("  For instructions, see:\n%ps."),
		styled_string (file_name_style.style (),
			       REPORT_BUGS_TO));
  gdb_puts ("\n\n", gdb_stderr);

  if (problem->should_dump_core == internal_problem_ask)
    {
      if (!can_dump_core_warn (LIMIT_MAX, reason.c_str ()))
	dump_core_p = 0;
      else if (!filtered_printing_initialized ()
	       || !readline_initialized (current_ui))
	dump_core_p = 1;
      else
	{
	  /* Default (yes/batch case) is to dump core.  This leaves a GDB
	     `dropping' so that it is easier to see that something went
	     wrong in GDB.  */
	  dump_core_p = query (_("%s\nCreate a core file of GDB? "),
			       reason.c_str ());
	}
    }
  else if (problem->should_dump_core == internal_problem_yes)
    dump_core_p = can_dump_core_warn (LIMIT_MAX, reason.c_str ());
  else if (problem->should_dump_core == internal_problem_no)
    dump_core_p = 0;
  else
    internal_error (_("bad switch"));

  if (quit_p)
    {
      if (dump_core_p)
	dump_core ();
      else
	exit (1);
    }
  else
    {
      if (dump_core_p)
	{
#ifdef HAVE_WORKING_FORK
	  if (fork () == 0)
	    dump_core ();
#endif
	}
    }

  dejavu = 0;
}

static struct internal_problem internal_error_problem = {
  "internal-error", true, internal_problem_ask, true, internal_problem_ask,
  true, GDB_PRINT_INTERNAL_BACKTRACE_INIT_ON
};

void
internal_verror (const char *file, int line, const char *fmt, va_list ap)
{
  internal_vproblem (&internal_error_problem, file, line, fmt, ap);
  throw_quit (_("Command aborted."));
}

static struct internal_problem internal_warning_problem = {
  "internal-warning", true, internal_problem_ask, true, internal_problem_ask,
  true, false
};

void
internal_vwarning (const char *file, int line, const char *fmt, va_list ap)
{
  internal_vproblem (&internal_warning_problem, file, line, fmt, ap);
}

static struct internal_problem demangler_warning_problem = {
  "demangler-warning", true, internal_problem_ask, false, internal_problem_no,
  false, false
};

void
demangler_vwarning (const char *file, int line, const char *fmt, va_list ap)
{
  internal_vproblem (&demangler_warning_problem, file, line, fmt, ap);
}

void
demangler_warning (const char *file, int line, const char *string, ...)
{
  va_list ap;

  va_start (ap, string);
  demangler_vwarning (file, line, string, ap);
  va_end (ap);
}

/* When GDB reports an internal problem (error or warning) it gives
   the user the opportunity to quit GDB and/or create a core file of
   the current debug session.  This function registers a few commands
   that make it possible to specify that GDB should always or never
   quit or create a core file, without asking.  The commands look
   like:

   maint set PROBLEM-NAME quit ask|yes|no
   maint show PROBLEM-NAME quit
   maint set PROBLEM-NAME corefile ask|yes|no
   maint show PROBLEM-NAME corefile

   Where PROBLEM-NAME is currently "internal-error" or
   "internal-warning".  */

static void
add_internal_problem_command (struct internal_problem *problem)
{
  struct cmd_list_element **set_cmd_list;
  struct cmd_list_element **show_cmd_list;

  set_cmd_list = XNEW (struct cmd_list_element *);
  show_cmd_list = XNEW (struct cmd_list_element *);
  *set_cmd_list = NULL;
  *show_cmd_list = NULL;

  /* The add_basic_prefix_cmd and add_show_prefix_cmd functions take
     ownership of the string passed in, which is why we don't need to free
     set_doc and show_doc in this function.  */
  const char *set_doc
    = xstrprintf (_("Configure what GDB does when %s is detected."),
		  problem->name).release ();
  const char *show_doc
    = xstrprintf (_("Show what GDB does when %s is detected."),
		  problem->name).release ();

  add_setshow_prefix_cmd (problem->name, class_maintenance,
			  set_doc, show_doc, set_cmd_list, show_cmd_list,
			  &maintenance_set_cmdlist, &maintenance_show_cmdlist);

  if (problem->user_settable_should_quit)
    {
      std::string set_quit_doc
	= string_printf (_("Set whether GDB should quit when an %s is "
			   "detected."), problem->name);
      std::string show_quit_doc
	= string_printf (_("Show whether GDB will quit when an %s is "
			   "detected."), problem->name);
      add_setshow_enum_cmd ("quit", class_maintenance,
			    internal_problem_modes,
			    &problem->should_quit,
			    set_quit_doc.c_str (),
			    show_quit_doc.c_str (),
			    NULL, /* help_doc */
			    NULL, /* setfunc */
			    NULL, /* showfunc */
			    set_cmd_list,
			    show_cmd_list);
    }

  if (problem->user_settable_should_dump_core)
    {
      std::string set_core_doc
	= string_printf (_("Set whether GDB should create a core file of "
			   "GDB when %s is detected."), problem->name);
      std::string show_core_doc
	= string_printf (_("Show whether GDB will create a core file of "
			   "GDB when %s is detected."), problem->name);
      add_setshow_enum_cmd ("corefile", class_maintenance,
			    internal_problem_modes,
			    &problem->should_dump_core,
			    set_core_doc.c_str (),
			    show_core_doc.c_str (),
			    NULL, /* help_doc */
			    NULL, /* setfunc */
			    NULL, /* showfunc */
			    set_cmd_list,
			    show_cmd_list);
    }

  if (problem->user_settable_should_print_backtrace)
    {
      std::string set_bt_doc
	= string_printf (_("Set whether GDB should print a backtrace of "
			   "GDB when %s is detected."), problem->name);
      std::string show_bt_doc
	= string_printf (_("Show whether GDB will print a backtrace of "
			   "GDB when %s is detected."), problem->name);
      add_setshow_boolean_cmd ("backtrace", class_maintenance,
			       &problem->should_print_backtrace,
			       set_bt_doc.c_str (),
			       show_bt_doc.c_str (),
			       NULL, /* help_doc */
			       gdb_internal_backtrace_set_cmd,
			       NULL, /* showfunc */
			       set_cmd_list,
			       show_cmd_list);
    }
}

/* Same as perror_with_name except that it prints a warning instead
   of throwing an error.  */

void
perror_warning_with_name (const char *string)
{
  std::string combined = perror_string (string);
  warning (_("%s"), combined.c_str ());
}

/* See utils.h.  */

void
warning_filename_and_errno (const char *filename, int saved_errno)
{
  warning (_("%ps: %s"), styled_string (file_name_style.style (), filename),
	   safe_strerror (saved_errno));
}

/* Control C eventually causes this to be called, at a convenient time.  */

void
quit (void)
{
  if (sync_quit_force_run)
    {
      sync_quit_force_run = false;
      throw_forced_quit ("SIGTERM");
    }

#ifdef __MSDOS__
  /* No steenking SIGINT will ever be coming our way when the
     program is resumed.  Don't lie.  */
  throw_quit ("Quit");
#else
  if (job_control
      /* If there is no terminal switching for this target, then we can't
	 possibly get screwed by the lack of job control.  */
      || !target_supports_terminal_ours ())
    throw_quit ("Quit");
  else
    throw_quit ("Quit (expect signal SIGINT when the program is resumed)");
#endif
}

/* See defs.h.  */

void
maybe_quit (void)
{
  if (!is_main_thread ())
    return;

  if (sync_quit_force_run)
    quit ();

  quit_handler ();
}


/* Called when a memory allocation fails, with the number of bytes of
   memory requested in SIZE.  */

void
malloc_failure (long size)
{
  if (size > 0)
    {
      internal_error (_("virtual memory exhausted: can't allocate %ld bytes."),
		      size);
    }
  else
    {
      internal_error (_("virtual memory exhausted."));
    }
}

/* See common/errors.h.  */

void
flush_streams ()
{
  gdb_stdout->flush ();
  gdb_stderr->flush ();
}

/* My replacement for the read system call.
   Used like `read' but keeps going if `read' returns too soon.  */

int
myread (int desc, char *addr, int len)
{
  int val;
  int orglen = len;

  while (len > 0)
    {
      val = read (desc, addr, len);
      if (val < 0)
	return val;
      if (val == 0)
	return orglen - len;
      len -= val;
      addr += val;
    }
  return orglen;
}



/* An RAII class that sets up to handle input and then tears down
   during destruction.  */

class scoped_input_handler
{
public:

  scoped_input_handler ()
    : m_quit_handler (&quit_handler, default_quit_handler),
      m_ui (NULL)
  {
    target_terminal::ours ();
    current_ui->register_file_handler ();
    if (current_ui->prompt_state == PROMPT_BLOCKED)
      m_ui = current_ui;
  }

  ~scoped_input_handler ()
  {
    if (m_ui != NULL)
      m_ui->unregister_file_handler ();
  }

  DISABLE_COPY_AND_ASSIGN (scoped_input_handler);

private:

  /* Save and restore the terminal state.  */
  target_terminal::scoped_restore_terminal_state m_term_state;

  /* Save and restore the quit handler.  */
  scoped_restore_tmpl<quit_handler_ftype *> m_quit_handler;

  /* The saved UI, if non-NULL.  */
  struct ui *m_ui;
};



/* This function supports the query, nquery, and yquery functions.
   Ask user a y-or-n question and return 0 if answer is no, 1 if
   answer is yes, or default the answer to the specified default
   (for yquery or nquery).  DEFCHAR may be 'y' or 'n' to provide a
   default answer, or '\0' for no default.
   CTLSTR is the control string and should end in "? ".  It should
   not say how to answer, because we do that.
   ARGS are the arguments passed along with the CTLSTR argument to
   printf.  */

static int ATTRIBUTE_PRINTF (1, 0)
defaulted_query (const char *ctlstr, const char defchar, va_list args)
{
  int retval;
  int def_value;
  char def_answer, not_def_answer;
  const char *y_string, *n_string;

  /* Set up according to which answer is the default.  */
  if (defchar == '\0')
    {
      def_value = 1;
      def_answer = 'Y';
      not_def_answer = 'N';
      y_string = "y";
      n_string = "n";
    }
  else if (defchar == 'y')
    {
      def_value = 1;
      def_answer = 'Y';
      not_def_answer = 'N';
      y_string = "[y]";
      n_string = "n";
    }
  else
    {
      def_value = 0;
      def_answer = 'N';
      not_def_answer = 'Y';
      y_string = "y";
      n_string = "[n]";
    }

  /* Automatically answer the default value if the user did not want
     prompts or the command was issued with the server prefix.  */
  if (!confirm || server_command)
    return def_value;

  /* If input isn't coming from the user directly, just say what
     question we're asking, and then answer the default automatically.  This
     way, important error messages don't get lost when talking to GDB
     over a pipe.  */
  if (current_ui->instream != current_ui->stdin_stream
      || !current_ui->input_interactive_p ()
      /* Restrict queries to the main UI.  */
      || current_ui != main_ui)
    {
      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();
      gdb_stdout->wrap_here (0);
      gdb_vprintf (gdb_stdout, ctlstr, args);

      gdb_printf (_("(%s or %s) [answered %c; "
		    "input not from terminal]\n"),
		  y_string, n_string, def_answer);

      return def_value;
    }

  if (deprecated_query_hook)
    {
      target_terminal::scoped_restore_terminal_state term_state;
      return deprecated_query_hook (ctlstr, args);
    }

  /* Format the question outside of the loop, to avoid reusing args.  */
  std::string question = string_vprintf (ctlstr, args);
  std::string prompt
    = string_printf (_("%s%s(%s or %s) %s"),
		     annotation_level > 1 ? "\n\032\032pre-query\n" : "",
		     question.c_str (), y_string, n_string,
		     annotation_level > 1 ? "\n\032\032query\n" : "");

  /* Used to add duration we waited for user to respond to
     prompt_for_continue_wait_time.  */
  using namespace std::chrono;
  steady_clock::time_point prompt_started = steady_clock::now ();

  scoped_input_handler prepare_input;

  while (1)
    {
      char *response, answer;

      gdb_flush (gdb_stdout);
      response = gdb_readline_wrapper (prompt.c_str ());

      if (response == NULL)	/* C-d  */
	{
	  gdb_printf ("EOF [assumed %c]\n", def_answer);
	  retval = def_value;
	  break;
	}

      answer = response[0];
      xfree (response);

      if (answer >= 'a')
	answer -= 040;
      /* Check answer.  For the non-default, the user must specify
	 the non-default explicitly.  */
      if (answer == not_def_answer)
	{
	  retval = !def_value;
	  break;
	}
      /* Otherwise, if a default was specified, the user may either
	 specify the required input or have it default by entering
	 nothing.  */
      if (answer == def_answer
	  || (defchar != '\0' && answer == '\0'))
	{
	  retval = def_value;
	  break;
	}
      /* Invalid entries are not defaulted and require another selection.  */
      gdb_printf (_("Please answer %s or %s.\n"),
		  y_string, n_string);
    }

  /* Add time spend in this routine to prompt_for_continue_wait_time.  */
  prompt_for_continue_wait_time += steady_clock::now () - prompt_started;

  if (annotation_level > 1)
    gdb_printf (("\n\032\032post-query\n"));
  return retval;
}


/* Ask user a y-or-n question and return 0 if answer is no, 1 if
   answer is yes, or 0 if answer is defaulted.
   Takes three args which are given to printf to print the question.
   The first, a control string, should end in "? ".
   It should not say how to answer, because we do that.  */

int
nquery (const char *ctlstr, ...)
{
  va_list args;
  int ret;

  va_start (args, ctlstr);
  ret = defaulted_query (ctlstr, 'n', args);
  va_end (args);
  return ret;
}

/* Ask user a y-or-n question and return 0 if answer is no, 1 if
   answer is yes, or 1 if answer is defaulted.
   Takes three args which are given to printf to print the question.
   The first, a control string, should end in "? ".
   It should not say how to answer, because we do that.  */

int
yquery (const char *ctlstr, ...)
{
  va_list args;
  int ret;

  va_start (args, ctlstr);
  ret = defaulted_query (ctlstr, 'y', args);
  va_end (args);
  return ret;
}

/* Ask user a y-or-n question and return 1 iff answer is yes.
   Takes three args which are given to printf to print the question.
   The first, a control string, should end in "? ".
   It should not say how to answer, because we do that.  */

int
query (const char *ctlstr, ...)
{
  va_list args;
  int ret;

  va_start (args, ctlstr);
  ret = defaulted_query (ctlstr, '\0', args);
  va_end (args);
  return ret;
}

/* A helper for parse_escape that converts a host character to a
   target character.  C is the host character.  If conversion is
   possible, then the target character is stored in *TARGET_C and the
   function returns 1.  Otherwise, the function returns 0.  */

static int
host_char_to_target (struct gdbarch *gdbarch, int c, int *target_c)
{
  char the_char = c;
  int result = 0;

  auto_obstack host_data;

  convert_between_encodings (target_charset (gdbarch), host_charset (),
			     (gdb_byte *) &the_char, 1, 1,
			     &host_data, translit_none);

  if (obstack_object_size (&host_data) == 1)
    {
      result = 1;
      *target_c = *(char *) obstack_base (&host_data);
    }

  return result;
}

/* Parse a C escape sequence.  STRING_PTR points to a variable
   containing a pointer to the string to parse.  That pointer
   should point to the character after the \.  That pointer
   is updated past the characters we use.  The value of the
   escape sequence is returned.

   A negative value means the sequence \ newline was seen,
   which is supposed to be equivalent to nothing at all.

   If \ is followed by a null character, we return a negative
   value and leave the string pointer pointing at the null character.

   If \ is followed by 000, we return 0 and leave the string pointer
   after the zeros.  A value of 0 does not mean end of string.  */

int
parse_escape (struct gdbarch *gdbarch, const char **string_ptr)
{
  int target_char = -2;	/* Initialize to avoid GCC warnings.  */
  int c = *(*string_ptr)++;

  switch (c)
    {
      case '\n':
	return -2;
      case 0:
	(*string_ptr)--;
	return 0;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
	{
	  int i = fromhex (c);
	  int count = 0;
	  while (++count < 3)
	    {
	      c = (**string_ptr);
	      if (ISDIGIT (c) && c != '8' && c != '9')
		{
		  (*string_ptr)++;
		  i *= 8;
		  i += fromhex (c);
		}
	      else
		{
		  break;
		}
	    }
	  return i;
	}

    case 'a':
      c = '\a';
      break;
    case 'b':
      c = '\b';
      break;
    case 'f':
      c = '\f';
      break;
    case 'n':
      c = '\n';
      break;
    case 'r':
      c = '\r';
      break;
    case 't':
      c = '\t';
      break;
    case 'v':
      c = '\v';
      break;

    default:
      break;
    }

  if (!host_char_to_target (gdbarch, c, &target_char))
    error (_("The escape sequence `\\%c' is equivalent to plain `%c',"
	     " which has no equivalent\nin the `%s' character set."),
	   c, c, target_charset (gdbarch));
  return target_char;
}


/* Number of lines per page or UINT_MAX if paging is disabled.  */
static unsigned int lines_per_page;
static void
show_lines_per_page (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Number of lines gdb thinks are in a page is %s.\n"),
	      value);
}

/* Number of chars per line or UINT_MAX if line folding is disabled.  */
static unsigned int chars_per_line;
static void
show_chars_per_line (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Number of characters gdb thinks "
		"are in a line is %s.\n"),
	      value);
}

/* Current count of lines printed on this page, chars on this line.  */
static unsigned int lines_printed, chars_printed;

/* True if pagination is disabled for just one command.  */

static bool pagination_disabled_for_command;

/* Buffer and start column of buffered text, for doing smarter word-
   wrapping.  When someone calls wrap_here(), we start buffering output
   that comes through gdb_puts().  If we see a newline, we just
   spit it out and forget about the wrap_here().  If we see another
   wrap_here(), we spit it out and remember the newer one.  If we see
   the end of the line, we spit out a newline, the indent, and then
   the buffered output.  */

static bool filter_initialized = false;



/* See utils.h.  */

int readline_hidden_cols = 0;

/* Initialize the number of lines per page and chars per line.  */

void
init_page_info (void)
{
  if (batch_flag)
    {
      lines_per_page = UINT_MAX;
      chars_per_line = UINT_MAX;
    }
  else
#if defined(TUI)
  if (!tui_get_command_dimension (&chars_per_line, &lines_per_page))
#endif
    {
      int rows, cols;

#if defined(__GO32__)
      rows = ScreenRows ();
      cols = ScreenCols ();
      lines_per_page = rows;
      chars_per_line = cols;
#else
      /* Make sure Readline has initialized its terminal settings.  */
      rl_reset_terminal (NULL);

      /* Get the screen size from Readline.  */
      rl_get_screen_size (&rows, &cols);

      /* Readline:
	 - ignores the COLUMNS variable when detecting screen width
	   (because rl_prefer_env_winsize defaults to 0)
	 - puts the detected screen width in the COLUMNS variable
	   (because rl_change_environment defaults to 1)
	 - may report one less than the detected screen width in
	   rl_get_screen_size (when _rl_term_autowrap == 0).
	 We could use _rl_term_autowrap, but we want to avoid introducing
	 another dependency on readline private variables, so set
	 readline_hidden_cols by comparing COLUMNS to cols as returned by
	 rl_get_screen_size.  */
      const char *columns_env_str = getenv ("COLUMNS");
      gdb_assert (columns_env_str != nullptr);
      int columns_env_val = atoi (columns_env_str);
      gdb_assert (columns_env_val != 0);
      readline_hidden_cols = columns_env_val - cols;
      gdb_assert (readline_hidden_cols >= 0);
      gdb_assert (readline_hidden_cols <= 1);

      lines_per_page = rows;
      chars_per_line = cols + readline_hidden_cols;

      /* Readline should have fetched the termcap entry for us.
	 Only try to use tgetnum function if rl_get_screen_size
	 did not return a useful value. */
      if (((rows <= 0) && (tgetnum ((char *) "li") < 0))
	/* Also disable paging if inside Emacs.  $EMACS was used
	   before Emacs v25.1, $INSIDE_EMACS is used since then.  */
	  || getenv ("EMACS") || getenv ("INSIDE_EMACS"))
	{
	  /* The number of lines per page is not mentioned in the terminal
	     description or EMACS environment variable is set.  This probably
	     means that paging is not useful, so disable paging.  */
	  lines_per_page = UINT_MAX;
	}

      /* If the output is not a terminal, don't paginate it.  */
      if (!gdb_stdout->isatty ())
	lines_per_page = UINT_MAX;
#endif
    }

  /* We handle SIGWINCH ourselves.  */
  rl_catch_sigwinch = 0;

  set_screen_size ();
  set_width ();
}

/* Return nonzero if filtered printing is initialized.  */
int
filtered_printing_initialized (void)
{
  return filter_initialized;
}

set_batch_flag_and_restore_page_info::set_batch_flag_and_restore_page_info ()
  : m_save_lines_per_page (lines_per_page),
    m_save_chars_per_line (chars_per_line),
    m_save_batch_flag (batch_flag)
{
  batch_flag = 1;
  init_page_info ();
}

set_batch_flag_and_restore_page_info::~set_batch_flag_and_restore_page_info ()
{
  batch_flag = m_save_batch_flag;
  chars_per_line = m_save_chars_per_line;
  lines_per_page = m_save_lines_per_page;

  set_screen_size ();
  set_width ();
}

/* An approximation of SQRT(INT_MAX) that is:
   - cheap to calculate,
   - guaranteed to be smaller than SQRT(INT_MAX), such that
     sqrt_int_max * sqrt_int_max doesn't overflow, and
   - "close enough" to SQRT(INT_MAX), for instance for INT_MAX == 2147483647,
     SQRT(INT_MAX) is ~46341 and sqrt_int_max == 32767.  */

static const int sqrt_int_max = INT_MAX >> (sizeof (int) * 8 / 2);

/* Set the screen size based on LINES_PER_PAGE and CHARS_PER_LINE.  */

static void
set_screen_size (void)
{
  int rows = lines_per_page;
  int cols = chars_per_line;

  /* If we get 0 or negative ROWS or COLS, treat as "infinite" size.
     A negative number can be seen here with the "set width/height"
     commands and either:

     - the user specified "unlimited", which maps to UINT_MAX, or
     - the user specified some number between INT_MAX and UINT_MAX.

     Cap "infinity" to approximately sqrt(INT_MAX) so that we don't
     overflow in rl_set_screen_size, which multiplies rows and columns
     to compute the number of characters on the screen.  */

  if (rows <= 0 || rows > sqrt_int_max)
    {
      rows = sqrt_int_max;
      lines_per_page = UINT_MAX;
    }

  if (cols <= 0 || cols > sqrt_int_max)
    {
      cols = sqrt_int_max;
      chars_per_line = UINT_MAX;
    }

  /* Update Readline's idea of the terminal size.  */
  rl_set_screen_size (rows, cols);
}

/* Reinitialize WRAP_BUFFER.  */

static void
set_width (void)
{
  if (chars_per_line == 0)
    init_page_info ();

  filter_initialized = true;
}

static void
set_width_command (const char *args, int from_tty, struct cmd_list_element *c)
{
  set_screen_size ();
  set_width ();
}

static void
set_height_command (const char *args, int from_tty, struct cmd_list_element *c)
{
  set_screen_size ();
}

/* See utils.h.  */

void
set_screen_width_and_height (int width, int height)
{
  lines_per_page = height;
  chars_per_line = width;

  set_screen_size ();
  set_width ();
}

/* Implement "maint info screen".  */

static void
maintenance_info_screen (const char *args, int from_tty)
{
  int rows, cols;
  rl_get_screen_size (&rows, &cols);

  gdb_printf (gdb_stdout,
	      _("Number of characters gdb thinks "
		"are in a line is %u%s.\n"),
	      chars_per_line,
	      chars_per_line == UINT_MAX ? " (unlimited)" : "");

  gdb_printf (gdb_stdout,
	      _("Number of characters readline reports "
		"are in a line is %d%s.\n"),
	      cols,
	      (cols == sqrt_int_max
	       ? " (unlimited)"
	       : (cols == sqrt_int_max - 1
		  ? " (unlimited - 1)"
		  : "")));

#ifdef HAVE_LIBCURSES
  gdb_printf (gdb_stdout,
	     _("Number of characters curses thinks "
	       "are in a line is %d.\n"),
	     COLS);
#endif

  gdb_printf (gdb_stdout,
	      _("Number of characters environment thinks "
		"are in a line is %s (COLUMNS).\n"),
	      getenv ("COLUMNS"));

  gdb_printf (gdb_stdout,
	      _("Number of lines gdb thinks are in a page is %u%s.\n"),
	      lines_per_page,
	      lines_per_page == UINT_MAX ? " (unlimited)" : "");

  gdb_printf (gdb_stdout,
	      _("Number of lines readline reports "
		"are in a page is %d%s.\n"),
	      rows,
	      rows == sqrt_int_max ? " (unlimited)" : "");

#ifdef HAVE_LIBCURSES
  gdb_printf (gdb_stdout,
	     _("Number of lines curses thinks "
	       "are in a page is %d.\n"),
	      LINES);
#endif

  gdb_printf (gdb_stdout,
	      _("Number of lines environment thinks "
		"are in a page is %s (LINES).\n"),
	      getenv ("LINES"));
}

void
pager_file::emit_style_escape (const ui_file_style &style)
{
  if (can_emit_style_escape () && style != m_applied_style)
    {
      m_applied_style = style;
      if (m_paging)
	m_stream->emit_style_escape (style);
      else
	m_wrap_buffer.append (style.to_ansi ());
    }
}

/* See pager.h.  */

void
pager_file::reset_style ()
{
  if (can_emit_style_escape ())
    {
      m_applied_style = ui_file_style ();
      m_wrap_buffer.append (m_applied_style.to_ansi ());
    }
}

/* Wait, so the user can read what's on the screen.  Prompt the user
   to continue by pressing RETURN.  'q' is also provided because
   telling users what to do in the prompt is more user-friendly than
   expecting them to think of Ctrl-C/SIGINT.  */

void
pager_file::prompt_for_continue ()
{
  char cont_prompt[120];
  /* Used to add duration we waited for user to respond to
     prompt_for_continue_wait_time.  */
  using namespace std::chrono;
  steady_clock::time_point prompt_started = steady_clock::now ();
  bool disable_pagination = pagination_disabled_for_command;

  scoped_restore save_paging = make_scoped_restore (&m_paging, true);

  /* Clear the current styling.  */
  m_stream->emit_style_escape (ui_file_style ());

  if (annotation_level > 1)
    m_stream->puts (("\n\032\032pre-prompt-for-continue\n"));

  strcpy (cont_prompt,
	  "--Type <RET> for more, q to quit, "
	  "c to continue without paging--");
  if (annotation_level > 1)
    strcat (cont_prompt, "\n\032\032prompt-for-continue\n");

  /* We must do this *before* we call gdb_readline_wrapper, else it
     will eventually call us -- thinking that we're trying to print
     beyond the end of the screen.  */
  reinitialize_more_filter ();

  scoped_input_handler prepare_input;

  /* Call gdb_readline_wrapper, not readline, in order to keep an
     event loop running.  */
  gdb::unique_xmalloc_ptr<char> ignore (gdb_readline_wrapper (cont_prompt));

  /* Add time spend in this routine to prompt_for_continue_wait_time.  */
  prompt_for_continue_wait_time += steady_clock::now () - prompt_started;

  if (annotation_level > 1)
    m_stream->puts (("\n\032\032post-prompt-for-continue\n"));

  if (ignore != NULL)
    {
      char *p = ignore.get ();

      while (*p == ' ' || *p == '\t')
	++p;
      if (p[0] == 'q')
	/* Do not call quit here; there is no possibility of SIGINT.  */
	throw_quit ("Quit");
      if (p[0] == 'c')
	disable_pagination = true;
    }

  /* Now we have to do this again, so that GDB will know that it doesn't
     need to save the ---Type <return>--- line at the top of the screen.  */
  reinitialize_more_filter ();
  pagination_disabled_for_command = disable_pagination;

  dont_repeat ();		/* Forget prev cmd -- CR won't repeat it.  */
}

/* Initialize timer to keep track of how long we waited for the user.  */

void
reset_prompt_for_continue_wait_time (void)
{
  using namespace std::chrono;

  prompt_for_continue_wait_time = steady_clock::duration::zero ();
}

/* Fetch the cumulative time spent in prompt_for_continue.  */

std::chrono::steady_clock::duration
get_prompt_for_continue_wait_time ()
{
  return prompt_for_continue_wait_time;
}

/* Reinitialize filter; ie. tell it to reset to original values.  */

void
reinitialize_more_filter (void)
{
  lines_printed = 0;
  chars_printed = 0;
  pagination_disabled_for_command = false;
}

void
pager_file::flush_wrap_buffer ()
{
  if (!m_paging && !m_wrap_buffer.empty ())
    {
      m_stream->puts (m_wrap_buffer.c_str ());
      m_wrap_buffer.clear ();
    }
}

void
pager_file::flush ()
{
  flush_wrap_buffer ();
  m_stream->flush ();
}

/* See utils.h.  */

void
gdb_flush (struct ui_file *stream)
{
  stream->flush ();
}

/* See utils.h.  */

int
get_chars_per_line ()
{
  return chars_per_line;
}

/* See ui-file.h.  */

void
pager_file::wrap_here (int indent)
{
  /* This should have been allocated, but be paranoid anyway.  */
  gdb_assert (filter_initialized);

  flush_wrap_buffer ();
  if (chars_per_line == UINT_MAX)	/* No line overflow checking.  */
    {
      m_wrap_column = 0;
    }
  else if (chars_printed >= chars_per_line)
    {
      this->puts ("\n");
      if (indent != 0)
	this->puts (n_spaces (indent));
      m_wrap_column = 0;
    }
  else
    {
      m_wrap_column = chars_printed;
      m_wrap_indent = indent;
      m_wrap_style = m_applied_style;
    }
}

/* Print input string to gdb_stdout arranging strings in columns of n
   chars.  String can be right or left justified in the column.  Never
   prints trailing spaces.  String should never be longer than width.
   FIXME: this could be useful for the EXAMINE command, which
   currently doesn't tabulate very well.  */

void
puts_tabular (char *string, int width, int right)
{
  int spaces = 0;
  int stringlen;
  char *spacebuf;

  gdb_assert (chars_per_line > 0);
  if (chars_per_line == UINT_MAX)
    {
      gdb_puts (string);
      gdb_puts ("\n");
      return;
    }

  if (((chars_printed - 1) / width + 2) * width >= chars_per_line)
    gdb_puts ("\n");

  if (width >= chars_per_line)
    width = chars_per_line - 1;

  stringlen = strlen (string);

  if (chars_printed > 0)
    spaces = width - (chars_printed - 1) % width - 1;
  if (right)
    spaces += width - stringlen;

  spacebuf = (char *) alloca (spaces + 1);
  spacebuf[spaces] = '\0';
  while (spaces--)
    spacebuf[spaces] = ' ';

  gdb_puts (spacebuf);
  gdb_puts (string);
}


/* Ensure that whatever gets printed next, using the filtered output
   commands, starts at the beginning of the line.  I.e. if there is
   any pending output for the current line, flush it and start a new
   line.  Otherwise do nothing.  */

void
begin_line (void)
{
  if (chars_printed > 0)
    {
      gdb_puts ("\n");
    }
}

void
pager_file::puts (const char *linebuffer)
{
  const char *lineptr;

  if (linebuffer == 0)
    return;

  /* Don't do any filtering or wrapping if both are disabled.  */
  if (batch_flag
      || (lines_per_page == UINT_MAX && chars_per_line == UINT_MAX)
      || top_level_interpreter () == NULL
      || top_level_interpreter ()->interp_ui_out ()->is_mi_like_p ())
    {
      flush_wrap_buffer ();
      m_stream->puts (linebuffer);
      return;
    }

  auto buffer_clearer
    = make_scope_exit ([&] ()
		       {
			 m_wrap_buffer.clear ();
			 m_wrap_column = 0;
			 m_wrap_indent = 0;
		       });

  /* If the user does "set height 1" then the pager will exhibit weird
     behavior.  This is pathological, though, so don't allow it.  */
  const unsigned int lines_allowed = (lines_per_page > 1
				      ? lines_per_page - 1
				      : 1);

  /* Go through and output each character.  Show line extension
     when this is necessary; prompt user for new page when this is
     necessary.  */

  lineptr = linebuffer;
  while (*lineptr)
    {
      /* Possible new page.  Note that PAGINATION_DISABLED_FOR_COMMAND
	 might be set during this loop, so we must continue to check
	 it here.  */
      if (pagination_enabled
	  && !pagination_disabled_for_command
	  && lines_printed >= lines_allowed)
	prompt_for_continue ();

      while (*lineptr && *lineptr != '\n')
	{
	  int skip_bytes;

	  /* Print a single line.  */
	  if (*lineptr == '\t')
	    {
	      m_wrap_buffer.push_back ('\t');
	      /* Shifting right by 3 produces the number of tab stops
		 we have already passed, and then adding one and
		 shifting left 3 advances to the next tab stop.  */
	      chars_printed = ((chars_printed >> 3) + 1) << 3;
	      lineptr++;
	    }
	  else if (*lineptr == '\033'
		   && skip_ansi_escape (lineptr, &skip_bytes))
	    {
	      m_wrap_buffer.append (lineptr, skip_bytes);
	      /* Note that we don't consider this a character, so we
		 don't increment chars_printed here.  */
	      lineptr += skip_bytes;
	    }
	  else if (*lineptr == '\r')
	    {
	      m_wrap_buffer.push_back (*lineptr);
	      chars_printed = 0;
	      lineptr++;
	    }
	  else
	    {
	      m_wrap_buffer.push_back (*lineptr);
	      chars_printed++;
	      lineptr++;
	    }

	  if (chars_printed >= chars_per_line)
	    {
	      unsigned int save_chars = chars_printed;

	      /* If we change the style, below, we'll want to reset it
		 before continuing to print.  If there is no wrap
		 column, then we'll only reset the style if the pager
		 prompt is given; and to avoid emitting style
		 sequences in the middle of a run of text, we track
		 this as well.  */
	      ui_file_style save_style = m_applied_style;
	      bool did_paginate = false;

	      chars_printed = 0;
	      lines_printed++;
	      if (m_wrap_column)
		{
		  /* We are about to insert a newline at an historic
		     location in the WRAP_BUFFER.  Before we do we want to
		     restore the default style.  To know if we actually
		     need to insert an escape sequence we must restore the
		     current applied style to how it was at the WRAP_COLUMN
		     location.  */
		  m_applied_style = m_wrap_style;
		  m_stream->emit_style_escape (ui_file_style ());
		  /* If we aren't actually wrapping, don't output
		     newline -- if chars_per_line is right, we
		     probably just overflowed anyway; if it's wrong,
		     let us keep going.  */
		  m_stream->puts ("\n");
		}
	      else
		this->flush_wrap_buffer ();

	      /* Possible new page.  Note that
		 PAGINATION_DISABLED_FOR_COMMAND might be set during
		 this loop, so we must continue to check it here.  */
	      if (pagination_enabled
		  && !pagination_disabled_for_command
		  && lines_printed >= lines_allowed)
		{
		  prompt_for_continue ();
		  did_paginate = true;
		}

	      /* Now output indentation and wrapped string.  */
	      if (m_wrap_column)
		{
		  m_stream->puts (n_spaces (m_wrap_indent));

		  /* Having finished inserting the wrapping we should
		     restore the style as it was at the WRAP_COLUMN.  */
		  m_stream->emit_style_escape (m_wrap_style);

		  /* The WRAP_BUFFER will still contain content, and that
		     content might set some alternative style.  Restore
		     APPLIED_STYLE as it was before we started wrapping,
		     this reflects the current style for the last character
		     in WRAP_BUFFER.  */
		  m_applied_style = save_style;

		  /* Note that this can set chars_printed > chars_per_line
		     if we are printing a long string.  */
		  chars_printed = m_wrap_indent + (save_chars - m_wrap_column);
		  m_wrap_column = 0;	/* And disable fancy wrap */
		}
	      else if (did_paginate)
		m_stream->emit_style_escape (save_style);
	    }
	}

      if (*lineptr == '\n')
	{
	  chars_printed = 0;
	  wrap_here (0); /* Spit out chars, cancel further wraps.  */
	  lines_printed++;
	  m_stream->puts ("\n");
	  lineptr++;
	}
    }

  buffer_clearer.release ();
}

void
pager_file::write (const char *buf, long length_buf)
{
  /* We have to make a string here because the pager uses
     skip_ansi_escape, which requires NUL-termination.  */
  std::string str (buf, length_buf);
  this->puts (str.c_str ());
}

#if GDB_SELF_TEST

/* Test that disabling the pager does not also disable word
   wrapping.  */

static void
test_pager ()
{
  string_file *strfile = new string_file ();
  pager_file pager (strfile);

  /* Make sure the pager is disabled.  */
  scoped_restore save_enabled
    = make_scoped_restore (&pagination_enabled, false);
  scoped_restore save_disabled
    = make_scoped_restore (&pagination_disabled_for_command, false);
  scoped_restore save_batch
    = make_scoped_restore (&batch_flag, false);
  scoped_restore save_lines
    = make_scoped_restore (&lines_per_page, 50);
  /* Make it easy to word wrap.  */
  scoped_restore save_chars
    = make_scoped_restore (&chars_per_line, 15);
  scoped_restore save_printed
    = make_scoped_restore (&chars_printed, 0);

  pager.puts ("aaaaaaaaaaaa");
  pager.wrap_here (2);
  pager.puts ("bbbbbbbbbbbb\n");

  SELF_CHECK (strfile->string () == "aaaaaaaaaaaa\n  bbbbbbbbbbbb\n");
}

#endif /* GDB_SELF_TEST */

void
gdb_puts (const char *linebuffer, struct ui_file *stream)
{
  stream->puts (linebuffer);
}

/* See utils.h.  */

void
fputs_styled (const char *linebuffer, const ui_file_style &style,
	      struct ui_file *stream)
{
  stream->emit_style_escape (style);
  gdb_puts (linebuffer, stream);
  stream->emit_style_escape (ui_file_style ());
}

/* See utils.h.  */

void
fputs_highlighted (const char *str, const compiled_regex &highlight,
		   struct ui_file *stream)
{
  regmatch_t pmatch;

  while (*str && highlight.exec (str, 1, &pmatch, 0) == 0)
    {
      size_t n_highlight = pmatch.rm_eo - pmatch.rm_so;

      /* Output the part before pmatch with current style.  */
      while (pmatch.rm_so > 0)
	{
	  gdb_putc (*str, stream);
	  pmatch.rm_so--;
	  str++;
	}

      /* Output pmatch with the highlight style.  */
      stream->emit_style_escape (highlight_style.style ());
      while (n_highlight > 0)
	{
	  gdb_putc (*str, stream);
	  n_highlight--;
	  str++;
	}
      stream->emit_style_escape (ui_file_style ());
    }

  /* Output the trailing part of STR not matching HIGHLIGHT.  */
  if (*str)
    gdb_puts (str, stream);
}

void
gdb_putc (int c)
{
  return gdb_stdout->putc (c);
}

void
gdb_putc (int c, struct ui_file *stream)
{
  return stream->putc (c);
}

void
gdb_vprintf (struct ui_file *stream, const char *format, va_list args)
{
  stream->vprintf (format, args);
}

void
gdb_vprintf (const char *format, va_list args)
{
  gdb_stdout->vprintf (format, args);
}

void
gdb_printf (struct ui_file *stream, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  gdb_vprintf (stream, format, args);
  va_end (args);
}

/* See utils.h.  */

void
fprintf_styled (struct ui_file *stream, const ui_file_style &style,
		const char *format, ...)
{
  va_list args;

  stream->emit_style_escape (style);
  va_start (args, format);
  gdb_vprintf (stream, format, args);
  va_end (args);
  stream->emit_style_escape (ui_file_style ());
}

void
gdb_printf (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  gdb_vprintf (gdb_stdout, format, args);
  va_end (args);
}


void
printf_unfiltered (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  string_file file (gdb_stdout->can_emit_style_escape ());
  file.vprintf (format, args);
  gdb_stdout->puts_unfiltered (file.string ().c_str ());
  va_end (args);
}

/* Easy -- but watch out!

   This routine is *not* a replacement for puts()!  puts() appends a newline.
   This one doesn't, and had better not!  */

void
gdb_puts (const char *string)
{
  gdb_stdout->puts (string);
}

/* Return a pointer to N spaces and a null.  The pointer is good
   until the next call to here.  */
const char *
n_spaces (int n)
{
  char *t;
  static char *spaces = 0;
  static int max_spaces = -1;

  if (n > max_spaces)
    {
      xfree (spaces);
      spaces = (char *) xmalloc (n + 1);
      for (t = spaces + n; t != spaces;)
	*--t = ' ';
      spaces[n] = '\0';
      max_spaces = n;
    }

  return spaces + max_spaces - n;
}

/* Print N spaces.  */
void
print_spaces (int n, struct ui_file *stream)
{
  gdb_puts (n_spaces (n), stream);
}

/* C++/ObjC demangler stuff.  */

/* fprintf_symbol attempts to demangle NAME, a symbol in language
   LANG, using demangling args ARG_MODE, and print it filtered to STREAM.
   If the name is not mangled, or the language for the name is unknown, or
   demangling is off, the name is printed in its "raw" form.  */

void
fprintf_symbol (struct ui_file *stream, const char *name,
		enum language lang, int arg_mode)
{
  if (name != NULL)
    {
      /* If user wants to see raw output, no problem.  */
      if (!demangle)
	{
	  gdb_puts (name, stream);
	}
      else
	{
	  gdb::unique_xmalloc_ptr<char> demangled
	    = language_def (lang)->demangle_symbol (name, arg_mode);
	  gdb_puts (demangled ? demangled.get () : name, stream);
	}
    }
}

/* True if CH is a character that can be part of a symbol name.  I.e.,
   either a number, a letter, or a '_'.  */

static bool
valid_identifier_name_char (int ch)
{
  return (ISALNUM (ch) || ch == '_');
}

/* Skip to end of token, or to END, whatever comes first.  Input is
   assumed to be a C++ operator name.  */

static const char *
cp_skip_operator_token (const char *token, const char *end)
{
  const char *p = token;
  while (p != end && !ISSPACE (*p) && *p != '(')
    {
      if (valid_identifier_name_char (*p))
	{
	  while (p != end && valid_identifier_name_char (*p))
	    p++;
	  return p;
	}
      else
	{
	  /* Note, ordered such that among ops that share a prefix,
	     longer comes first.  This is so that the loop below can
	     bail on first match.  */
	  static const char *ops[] =
	    {
	      "[",
	      "]",
	      "~",
	      ",",
	      "-=", "--", "->", "-",
	      "+=", "++", "+",
	      "*=", "*",
	      "/=", "/",
	      "%=", "%",
	      "|=", "||", "|",
	      "&=", "&&", "&",
	      "^=", "^",
	      "!=", "!",
	      "<<=", "<=", "<<", "<",
	      ">>=", ">=", ">>", ">",
	      "==", "=",
	    };

	  for (const char *op : ops)
	    {
	      size_t oplen = strlen (op);
	      size_t lencmp = std::min<size_t> (oplen, end - p);

	      if (strncmp (p, op, lencmp) == 0)
		return p + lencmp;
	    }
	  /* Some unidentified character.  Return it.  */
	  return p + 1;
	}
    }

  return p;
}

/* Advance STRING1/STRING2 past whitespace.  */

static void
skip_ws (const char *&string1, const char *&string2, const char *end_str2)
{
  while (ISSPACE (*string1))
    string1++;
  while (string2 < end_str2 && ISSPACE (*string2))
    string2++;
}

/* True if STRING points at the start of a C++ operator name.  START
   is the start of the string that STRING points to, hence when
   reading backwards, we must not read any character before START.  */

static bool
cp_is_operator (const char *string, const char *start)
{
  return ((string == start
	   || !valid_identifier_name_char (string[-1]))
	  && strncmp (string, CP_OPERATOR_STR, CP_OPERATOR_LEN) == 0
	  && !valid_identifier_name_char (string[CP_OPERATOR_LEN]));
}

/* If *NAME points at an ABI tag, skip it and return true.  Otherwise
   leave *NAME unmodified and return false.  (see GCC's abi_tag
   attribute), such names are demangled as e.g.,
   "function[abi:cxx11]()".  */

static bool
skip_abi_tag (const char **name)
{
  const char *p = *name;

  if (startswith (p, "[abi:"))
    {
      p += 5;

      while (valid_identifier_name_char (*p))
	p++;

      if (*p == ']')
	{
	  p++;
	  *name = p;
	  return true;
	}
    }
  return false;
}

/* If *NAME points at a template parameter list, skip it and return true.
   Otherwise do nothing and return false.  */

static bool
skip_template_parameter_list (const char **name)
{
  const char *p = *name;

  if (*p == '<')
    {
      const char *template_param_list_end = find_toplevel_char (p + 1, '>');

      if (template_param_list_end == NULL)
	return false;

      p = template_param_list_end + 1;

      /* Skip any whitespace that might occur after the closing of the
	 parameter list, but only if it is the end of parameter list.  */
      const char *q = p;
      while (ISSPACE (*q))
	++q;
      if (*q == '>')
	p = q;
      *name = p;
      return true;
    }

  return false;
}

/* See utils.h.  */

int
strncmp_iw_with_mode (const char *string1, const char *string2,
		      size_t string2_len, strncmp_iw_mode mode,
		      enum language language,
		      completion_match_for_lcd *match_for_lcd,
		      bool ignore_template_params)
{
  const char *string1_start = string1;
  const char *end_str2 = string2 + string2_len;
  bool skip_spaces = true;
  bool have_colon_op = (language == language_cplus
			|| language == language_rust
			|| language == language_fortran);

  gdb_assert (match_for_lcd == nullptr || match_for_lcd->empty ());

  while (1)
    {
      if (skip_spaces
	  || ((ISSPACE (*string1) && !valid_identifier_name_char (*string2))
	      || (ISSPACE (*string2) && !valid_identifier_name_char (*string1))))
	{
	  skip_ws (string1, string2, end_str2);
	  skip_spaces = false;
	}

      /* Skip [abi:cxx11] tags in the symbol name if the lookup name
	 doesn't include them.  E.g.:

	 string1: function[abi:cxx1](int)
	 string2: function

	 string1: function[abi:cxx1](int)
	 string2: function(int)

	 string1: Struct[abi:cxx1]::function()
	 string2: Struct::function()

	 string1: function(Struct[abi:cxx1], int)
	 string2: function(Struct, int)
      */
      if (string2 == end_str2
	  || (*string2 != '[' && !valid_identifier_name_char (*string2)))
	{
	  const char *abi_start = string1;

	  /* There can be more than one tag.  */
	  while (*string1 == '[' && skip_abi_tag (&string1))
	    ;

	  if (match_for_lcd != NULL && abi_start != string1)
	    match_for_lcd->mark_ignored_range (abi_start, string1);

	  while (ISSPACE (*string1))
	    string1++;
	}

      /* Skip template parameters in STRING1 if STRING2 does not contain
	 any.  E.g.:

	 Case 1: User is looking for all functions named "foo".
	 string1: foo <...> (...)
	 string2: foo

	 Case 2: User is looking for all methods named "foo" in all template
	 class instantiations.
	 string1: Foo<...>::foo <...> (...)
	 string2: Foo::foo (...)

	 Case 3: User is looking for a specific overload of a template
	 function or method.
	 string1: foo<...>
	 string2: foo(...)

	 Case 4: User is looking for a specific overload of a specific
	 template instantiation.
	 string1: foo<A> (...)
	 string2: foo<B> (...)

	 Case 5: User is looking wild parameter match.
	 string1: foo<A<a<b<...> > > > (...)
	 string2: foo<A
      */
      if (language == language_cplus && ignore_template_params
	  && *string1 == '<' && *string2 != '<')
	{
	  /* Skip any parameter list in STRING1.  */
	  const char *template_start = string1;

	  if (skip_template_parameter_list (&string1))
	    {
	      /* Don't mark the parameter list ignored if the user didn't
		 try to ignore it.  [Case #5 above]  */
	      if (*string2 != '\0'
		  && match_for_lcd != NULL && template_start != string1)
		match_for_lcd->mark_ignored_range (template_start, string1);
	    }
	}

      if (*string1 == '\0' || string2 == end_str2)
	break;

      /* Handle the :: operator.  */
      if (have_colon_op && string1[0] == ':' && string1[1] == ':')
	{
	  if (*string2 != ':')
	    return 1;

	  string1++;
	  string2++;

	  if (string2 == end_str2)
	    break;

	  if (*string2 != ':')
	    return 1;

	  string1++;
	  string2++;

	  while (ISSPACE (*string1))
	    string1++;
	  while (string2 < end_str2 && ISSPACE (*string2))
	    string2++;
	  continue;
	}

      /* Handle C++ user-defined operators.  */
      else if (language == language_cplus
	       && *string1 == 'o')
	{
	  if (cp_is_operator (string1, string1_start))
	    {
	      /* An operator name in STRING1.  Check STRING2.  */
	      size_t cmplen
		= std::min<size_t> (CP_OPERATOR_LEN, end_str2 - string2);
	      if (strncmp (string1, string2, cmplen) != 0)
		return 1;

	      string1 += cmplen;
	      string2 += cmplen;

	      if (string2 != end_str2)
		{
		  /* Check for "operatorX" in STRING2.  */
		  if (valid_identifier_name_char (*string2))
		    return 1;

		  skip_ws (string1, string2, end_str2);
		}

	      /* Handle operator().  */
	      if (*string1 == '(')
		{
		  if (string2 == end_str2)
		    {
		      if (mode == strncmp_iw_mode::NORMAL)
			return 0;
		      else
			{
			  /* Don't break for the regular return at the
			     bottom, because "operator" should not
			     match "operator()", since this open
			     parentheses is not the parameter list
			     start.  */
			  return *string1 != '\0';
			}
		    }

		  if (*string1 != *string2)
		    return 1;

		  string1++;
		  string2++;
		}

	      while (1)
		{
		  skip_ws (string1, string2, end_str2);

		  /* Skip to end of token, or to END, whatever comes
		     first.  */
		  const char *end_str1 = string1 + strlen (string1);
		  const char *p1 = cp_skip_operator_token (string1, end_str1);
		  const char *p2 = cp_skip_operator_token (string2, end_str2);

		  cmplen = std::min (p1 - string1, p2 - string2);
		  if (p2 == end_str2)
		    {
		      if (strncmp (string1, string2, cmplen) != 0)
			return 1;
		    }
		  else
		    {
		      if (p1 - string1 != p2 - string2)
			return 1;
		      if (strncmp (string1, string2, cmplen) != 0)
			return 1;
		    }

		  string1 += cmplen;
		  string2 += cmplen;

		  if (*string1 == '\0' || string2 == end_str2)
		    break;
		  if (*string1 == '(' || *string2 == '(')
		    break;

		  /* If STRING1 or STRING2 starts with a template
		     parameter list, break out of operator processing.  */
		  skip_ws (string1, string2, end_str2);
		  if (*string1 == '<' || *string2 == '<')
		    break;
		}

	      continue;
	    }
	}

      if (case_sensitivity == case_sensitive_on && *string1 != *string2)
	break;
      if (case_sensitivity == case_sensitive_off
	  && (TOLOWER ((unsigned char) *string1)
	      != TOLOWER ((unsigned char) *string2)))
	break;

      /* If we see any non-whitespace, non-identifier-name character
	 (any of "()<>*&" etc.), then skip spaces the next time
	 around.  */
      if (!ISSPACE (*string1) && !valid_identifier_name_char (*string1))
	skip_spaces = true;

      string1++;
      string2++;
    }

  if (string2 == end_str2)
    {
      if (mode == strncmp_iw_mode::NORMAL)
	{
	  /* Strip abi tag markers from the matched symbol name.
	     Usually the ABI marker will be found on function name
	     (automatically added because the function returns an
	     object marked with an ABI tag).  However, it's also
	     possible to see a marker in one of the function
	     parameters, for example.

	     string2 (lookup name):
	       func
	     symbol name:
	       function(some_struct[abi:cxx11], int)

	     and for completion LCD computation we want to say that
	     the match was for:
	       function(some_struct, int)
	  */
	  if (match_for_lcd != NULL)
	    {
	      while ((string1 = strstr (string1, "[abi:")) != NULL)
		{
		  const char *abi_start = string1;

		  /* There can be more than one tag.  */
		  while (skip_abi_tag (&string1) && *string1 == '[')
		    ;

		  if (abi_start != string1)
		    match_for_lcd->mark_ignored_range (abi_start, string1);
		}
	    }

	  return 0;
	}
      else
	{
	  if (*string1 == '(')
	    {
	      int p_count = 0;

	      do
		{
		  if (*string1 == '(')
		    ++p_count;
		  else if (*string1 == ')')
		    --p_count;
		  ++string1;
		}
	      while (*string1 != '\0' && p_count > 0);

	      /* There maybe things like 'const' after the parameters,
		 which we do want to ignore.  However, if there's an '@'
		 then this likely indicates something like '@plt' which we
		 should not ignore.  */
	      return *string1 == '@';
	    }

	  return *string1 == '\0' ? 0 : 1;
	}

    }
  else
    return 1;
}

#if GDB_SELF_TEST

/* Unit tests for strncmp_iw_with_mode.  */

#define CHECK_MATCH_LM(S1, S2, MODE, LANG, LCD)			\
  SELF_CHECK (strncmp_iw_with_mode ((S1), (S2), strlen ((S2)),	\
				    strncmp_iw_mode::MODE,				\
				    (LANG), (LCD)) == 0)

#define CHECK_MATCH_LANG(S1, S2, MODE, LANG)			\
  CHECK_MATCH_LM ((S1), (S2), MODE, (LANG), nullptr)

#define CHECK_MATCH(S1, S2, MODE)						\
  CHECK_MATCH_LANG ((S1), (S2), MODE, language_minimal)

#define CHECK_NO_MATCH_LM(S1, S2, MODE, LANG, LCD)		\
  SELF_CHECK (strncmp_iw_with_mode ((S1), (S2), strlen ((S2)),	\
				    strncmp_iw_mode::MODE,				\
				    (LANG)) != 0)

#define CHECK_NO_MATCH_LANG(S1, S2, MODE, LANG)		\
  CHECK_NO_MATCH_LM ((S1), (S2), MODE, (LANG), nullptr)

#define CHECK_NO_MATCH(S1, S2, MODE)				       \
  CHECK_NO_MATCH_LANG ((S1), (S2), MODE, language_minimal)

static void
check_scope_operator (enum language lang)
{
  CHECK_MATCH_LANG ("::", "::", NORMAL, lang);
  CHECK_MATCH_LANG ("::foo", "::", NORMAL, lang);
  CHECK_MATCH_LANG ("::foo", "::foo", NORMAL, lang);
  CHECK_MATCH_LANG (" :: foo ", "::foo", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b", "a ::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b", "a\t::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b", "a \t::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b", "a\t ::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b", "a:: b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b", "a::\tb", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b", "a:: \tb", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b", "a::\t b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b", "a :: b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b", "a ::\tb", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b", "a\t:: b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b", "a \t::\t b", NORMAL, lang);
  CHECK_MATCH_LANG ("a ::b", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a\t::b", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a \t::b", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a\t ::b", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a:: b", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::\tb", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a:: \tb", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::\t b", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a :: b", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a ::\tb", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a\t:: b", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a \t::\t b", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b::c", "a::b::c", NORMAL, lang);
  CHECK_MATCH_LANG (" a:: b:: c", "a::b::c", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b::c", " a:: b:: c", NORMAL, lang);
  CHECK_MATCH_LANG ("a ::b ::c", "a::b::c", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b::c", "a :: b:: c", NORMAL, lang);
  CHECK_MATCH_LANG ("\ta::\tb::\tc", "\ta::\tb::\tc", NORMAL, lang);
  CHECK_MATCH_LANG ("a\t::b\t::c\t", "a\t::b\t::c\t", NORMAL, lang);
  CHECK_MATCH_LANG (" \ta:: \tb:: \tc", " \ta:: \tb:: \tc", NORMAL, lang);
  CHECK_MATCH_LANG ("\t a::\t b::\t c", "\t a::\t b::\t c", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b::c", "\ta::\tb::\tc", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b::c", "a\t::b\t::c\t", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b::c", " \ta:: \tb:: \tc", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b::c", "\t a::\t b::\t c", NORMAL, lang);
  CHECK_MATCH_LANG ("\ta::\tb::\tc", "a::b::c", NORMAL, lang);
  CHECK_MATCH_LANG ("a\t::b\t::c\t", "a::b::c", NORMAL, lang);
  CHECK_MATCH_LANG (" \ta:: \tb:: \tc", "a::b::c", NORMAL, lang);
  CHECK_MATCH_LANG ("\t a::\t b::\t c", "a::b::c", NORMAL, lang);
  CHECK_MATCH_LANG ("a :: b:: c\t", "\ta :: b\t::  c\t\t", NORMAL, lang);
  CHECK_MATCH_LANG ("  a::\t  \t    b::     c\t", "\ta ::b::  c\t\t",
	      NORMAL, lang);
  CHECK_MATCH_LANG ("a      :: b               :: \t\t\tc\t",
	      "\t\t\t\ta        ::   \t\t\t        b             \t\t::c",
	      NORMAL, lang);
  CHECK_MATCH_LANG ("a::b()", "a", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b()", "a::", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b()", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b(a)", "a", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b(a)", "a::", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b(a)", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b(a,b)", "a", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b(a,b)", "a::", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b(a,b)", "a::b", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b(a,b,c)", "a", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b(a,b,c)", "a::", NORMAL, lang);
  CHECK_MATCH_LANG ("a::b(a,b,c)", "a::b", NORMAL, lang);

  CHECK_NO_MATCH_LANG ("a::", "::a", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("::a", "::a()", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("::", "::a", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a:::b", "a::b", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b()", "a::b(a)", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b(a)", "a::b()", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b(a,b)", "a::b(a,a)", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b", "a()", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b", "a::()", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b", "a::b()", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b", "a(a)", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b", "a::(a)", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b", "a::b()", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b", "a(a,b)", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b", "a::(a,b)", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b", "a::b(a,b)", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b", "a(a,b,c)", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b", "a::(a,b,c)", NORMAL, lang);
  CHECK_NO_MATCH_LANG ("a::b", "a::b(a,b,c)", NORMAL, lang);
}

/* Callback for strncmp_iw_with_mode unit tests.  */

static void
strncmp_iw_with_mode_tests ()
{
  /* Some of the following tests are nonsensical, but could be input by a
     deranged script (or user).  */

  /* strncmp_iw_mode::NORMAL: strcmp()-like but ignore any whitespace...  */

  CHECK_MATCH ("", "", NORMAL);
  CHECK_MATCH ("foo", "foo", NORMAL);
  CHECK_MATCH (" foo", "foo", NORMAL);
  CHECK_MATCH ("foo ", "foo", NORMAL);
  CHECK_MATCH (" foo ", "foo", NORMAL);
  CHECK_MATCH ("  foo", "foo", NORMAL);
  CHECK_MATCH ("foo  ", "foo", NORMAL);
  CHECK_MATCH ("  foo  ", "foo", NORMAL);
  CHECK_MATCH ("\tfoo", "foo", NORMAL);
  CHECK_MATCH ("foo\t", "foo", NORMAL);
  CHECK_MATCH ("\tfoo\t", "foo", NORMAL);
  CHECK_MATCH (" \tfoo \t", "foo", NORMAL);
  CHECK_MATCH ("\t foo\t ", "foo", NORMAL);
  CHECK_MATCH ("\t \t     \t\t\t\t   foo\t\t\t  \t\t   \t   \t    \t  \t ",
	       "foo", NORMAL);
  CHECK_MATCH ("foo",
	       "\t \t     \t\t\t\t   foo\t\t\t  \t\t   \t   \t    \t  \t ",
	       NORMAL);
  CHECK_MATCH ("foo bar", "foo", NORMAL);
  CHECK_NO_MATCH ("foo", "bar", NORMAL);
  CHECK_NO_MATCH ("foo bar", "foobar", NORMAL);
  CHECK_NO_MATCH (" foo ", "bar", NORMAL);
  CHECK_NO_MATCH ("foo", " bar ", NORMAL);
  CHECK_NO_MATCH (" \t\t    foo\t\t ", "\t    \t    \tbar\t", NORMAL);
  CHECK_NO_MATCH ("@!%&", "@!%&foo", NORMAL);

  /* ... and function parameters in STRING1.  */
  CHECK_MATCH ("foo()", "foo()", NORMAL);
  CHECK_MATCH ("foo ()", "foo()", NORMAL);
  CHECK_MATCH ("foo  ()", "foo()", NORMAL);
  CHECK_MATCH ("foo\t()", "foo()", NORMAL);
  CHECK_MATCH ("foo\t  ()", "foo()", NORMAL);
  CHECK_MATCH ("foo  \t()", "foo()", NORMAL);
  CHECK_MATCH ("foo()", "foo ()", NORMAL);
  CHECK_MATCH ("foo()", "foo  ()", NORMAL);
  CHECK_MATCH ("foo()", "foo\t()", NORMAL);
  CHECK_MATCH ("foo()", "foo\t ()", NORMAL);
  CHECK_MATCH ("foo()", "foo \t()", NORMAL);
  CHECK_MATCH ("foo()", "foo()", NORMAL);
  CHECK_MATCH ("foo ()", "foo ()", NORMAL);
  CHECK_MATCH ("foo  ()", "foo  ()", NORMAL);
  CHECK_MATCH ("foo\t()", "foo\t()", NORMAL);
  CHECK_MATCH ("foo\t  ()", "foo\t ()", NORMAL);
  CHECK_MATCH ("foo  \t()", "foo \t()", NORMAL);
  CHECK_MATCH ("foo(a)", "foo(a)", NORMAL);
  CHECK_MATCH ("foo( a)", "foo(a)", NORMAL);
  CHECK_MATCH ("foo(a )", "foo(a)", NORMAL);
  CHECK_MATCH ("foo(\ta)", "foo(a)", NORMAL);
  CHECK_MATCH ("foo(a\t)", "foo(a)", NORMAL);
  CHECK_MATCH ("foo(\t a)", "foo(a)", NORMAL);
  CHECK_MATCH ("foo( \ta)", "foo(a)", NORMAL);
  CHECK_MATCH ("foo(a\t )", "foo(a)", NORMAL);
  CHECK_MATCH ("foo(a \t)", "foo(a)", NORMAL);
  CHECK_MATCH ("foo( a )", "foo(a)", NORMAL);
  CHECK_MATCH ("foo(\ta\t)", "foo(a)", NORMAL);
  CHECK_MATCH ("foo(\t a\t )", "foo(a)", NORMAL);
  CHECK_MATCH ("foo( \ta \t)", "foo(a)", NORMAL);
  CHECK_MATCH ("foo(a)", "foo( a)", NORMAL);
  CHECK_MATCH ("foo(a)", "foo(a )", NORMAL);
  CHECK_MATCH ("foo(a)", "foo(\ta)", NORMAL);
  CHECK_MATCH ("foo(a)", "foo(a\t)", NORMAL);
  CHECK_MATCH ("foo(a)", "foo(\t a)", NORMAL);
  CHECK_MATCH ("foo(a)", "foo( \ta)", NORMAL);
  CHECK_MATCH ("foo(a)", "foo(a\t )", NORMAL);
  CHECK_MATCH ("foo(a)", "foo(a \t)", NORMAL);
  CHECK_MATCH ("foo(a)", "foo( a )", NORMAL);
  CHECK_MATCH ("foo(a)", "foo(\ta\t)", NORMAL);
  CHECK_MATCH ("foo(a)", "foo(\t a\t )", NORMAL);
  CHECK_MATCH ("foo(a)", "foo( \ta \t)", NORMAL);
  CHECK_MATCH ("foo(a,b)", "foo(a,b)", NORMAL);
  CHECK_MATCH ("foo(a ,b)", "foo(a,b)", NORMAL);
  CHECK_MATCH ("foo(a\t,b)", "foo(a,b)", NORMAL);
  CHECK_MATCH ("foo(a,\tb)", "foo(a,b)", NORMAL);
  CHECK_MATCH ("foo(a\t,\tb)", "foo(a,b)", NORMAL);
  CHECK_MATCH ("foo(a \t,b)", "foo(a,b)", NORMAL);
  CHECK_MATCH ("foo(a\t ,b)", "foo(a,b)", NORMAL);
  CHECK_MATCH ("foo(a,\tb)", "foo(a,b)", NORMAL);
  CHECK_MATCH ("foo(a, \tb)", "foo(a,b)", NORMAL);
  CHECK_MATCH ("foo(a,\t b)", "foo(a,b)", NORMAL);
  CHECK_MATCH ("foo(a,b)", "foo(a ,b)", NORMAL);
  CHECK_MATCH ("foo(a,b)", "foo(a\t,b)", NORMAL);
  CHECK_MATCH ("foo(a,b)", "foo(a,\tb)", NORMAL);
  CHECK_MATCH ("foo(a,b)", "foo(a\t,\tb)", NORMAL);
  CHECK_MATCH ("foo(a,b)", "foo(a \t,b)", NORMAL);
  CHECK_MATCH ("foo(a,b)", "foo(a\t ,b)", NORMAL);
  CHECK_MATCH ("foo(a,b)", "foo(a,\tb)", NORMAL);
  CHECK_MATCH ("foo(a,b)", "foo(a, \tb)", NORMAL);
  CHECK_MATCH ("foo(a,b)", "foo(a,\t b)", NORMAL);
  CHECK_MATCH ("foo(a,b,c,d)", "foo(a,b,c,d)", NORMAL);
  CHECK_MATCH (" foo ( a , b , c , d ) ", "foo(a,b,c,d)", NORMAL);
  CHECK_MATCH (" foo ( a , b , c , d ) ", "foo( a , b , c , d )", NORMAL);
  CHECK_MATCH ("foo &\t*(\ta b    *\t\t&)", "foo", NORMAL);
  CHECK_MATCH ("foo &\t*(\ta b    *\t\t&)", "foo&*(a b * &)", NORMAL);
  CHECK_MATCH ("foo(a) b", "foo(a)", NORMAL);
  CHECK_MATCH ("*foo(*a&)", "*foo", NORMAL);
  CHECK_MATCH ("*foo(*a&)", "*foo(*a&)", NORMAL);
  CHECK_MATCH ("*a&b#c/^d$foo(*a&)", "*a&b#c/^d$foo", NORMAL);
  CHECK_MATCH ("* foo", "*foo", NORMAL);
  CHECK_MATCH ("foo&", "foo", NORMAL);
  CHECK_MATCH ("foo*", "foo", NORMAL);
  CHECK_MATCH ("foo.", "foo", NORMAL);
  CHECK_MATCH ("foo->", "foo", NORMAL);

  CHECK_NO_MATCH ("foo", "foo(", NORMAL);
  CHECK_NO_MATCH ("foo", "foo()", NORMAL);
  CHECK_NO_MATCH ("foo", "foo(a)", NORMAL);
  CHECK_NO_MATCH ("foo", "foo(a)", NORMAL);
  CHECK_NO_MATCH ("foo", "foo*", NORMAL);
  CHECK_NO_MATCH ("foo", "foo (*", NORMAL);
  CHECK_NO_MATCH ("foo*", "foo (*", NORMAL);
  CHECK_NO_MATCH ("foo *", "foo (*", NORMAL);
  CHECK_NO_MATCH ("foo&", "foo (*", NORMAL);
  CHECK_NO_MATCH ("foo &", "foo (*", NORMAL);
  CHECK_NO_MATCH ("foo &*", "foo (&)", NORMAL);
  CHECK_NO_MATCH ("foo & \t    *\t", "foo (*", NORMAL);
  CHECK_NO_MATCH ("foo & \t    *\t", "foo (*", NORMAL);
  CHECK_NO_MATCH ("foo(a*) b", "foo(a) b", NORMAL);
  CHECK_NO_MATCH ("foo[aqi:A](a)", "foo(b)", NORMAL);
  CHECK_NO_MATCH ("*foo", "foo", NORMAL);
  CHECK_NO_MATCH ("*foo", "foo*", NORMAL);
  CHECK_NO_MATCH ("*foo*", "*foo&", NORMAL);
  CHECK_NO_MATCH ("*foo*", "foo *", NORMAL);
  CHECK_NO_MATCH ("&foo", "foo", NORMAL);
  CHECK_NO_MATCH ("&foo", "foo&", NORMAL);
  CHECK_NO_MATCH ("foo&", "&foo", NORMAL);
  CHECK_NO_MATCH ("foo", "foo&", NORMAL);
  CHECK_NO_MATCH ("foo", "foo*", NORMAL);
  CHECK_NO_MATCH ("foo", "foo.", NORMAL);
  CHECK_NO_MATCH ("foo", "foo->", NORMAL);
  CHECK_NO_MATCH ("foo bar", "foo()", NORMAL);
  CHECK_NO_MATCH ("foo bar", "foo bar()", NORMAL);
  CHECK_NO_MATCH ("foo()", "foo(a)", NORMAL);
  CHECK_NO_MATCH ("*(*)&", "*(*)*", NORMAL);
  CHECK_NO_MATCH ("foo(a)", "foo()", NORMAL);
  CHECK_NO_MATCH ("foo(a)", "foo(b)", NORMAL);
  CHECK_NO_MATCH ("foo(a,b)", "foo(a,b,c)", NORMAL);
  CHECK_NO_MATCH ("foo(a\\b)", "foo()", NORMAL);
  CHECK_NO_MATCH ("foo bar(a b c d)", "foobar", NORMAL);
  CHECK_NO_MATCH ("foo bar(a b c d)", "foobar ( a b   c \td\t)\t", NORMAL);

  /* Test scope operator.  */
  check_scope_operator (language_minimal);
  check_scope_operator (language_cplus);
  check_scope_operator (language_fortran);
  check_scope_operator (language_rust);

  /* Test C++ user-defined operators.  */
  CHECK_MATCH_LANG ("operator foo(int&)", "operator foo(int &)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("operator foo(int &)", "operator foo(int &)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("operator foo(int\t&)", "operator foo(int\t&)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("operator foo (int)", "operator foo(int)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("operator foo\t(int)", "operator foo(int)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("operator foo \t(int)", "operator foo(int)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("operator foo (int)", "operator foo \t(int)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("operator foo\t(int)", "operator foo \t(int)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("operator foo \t(int)", "operator foo \t(int)", NORMAL,
		    language_cplus);

  CHECK_MATCH_LANG ("a::operator foo(int&)", "a::operator foo(int &)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("a :: operator foo(int &)", "a::operator foo(int &)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("a \t:: \toperator foo(int\t&)", "a::operator foo(int\t&)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("a::operator foo (int)", "a::operator foo(int)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("a::operator foo\t(int)", "a::operator foo(int)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("a::operator foo \t(int)", "a::operator foo(int)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("a::operator foo (int)", "a::operator foo \t(int)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("a::operator foo\t(int)", "a::operator foo \t(int)", NORMAL,
		    language_cplus);
  CHECK_MATCH_LANG ("a::operator foo \t(int)", "a::operator foo \t(int)", NORMAL,
		    language_cplus);

  CHECK_NO_MATCH_LANG ("operator foo(int)", "operator foo(char)", NORMAL,
		       language_cplus);
  CHECK_NO_MATCH_LANG ("operator foo(int)", "operator foo(int *)", NORMAL,
		       language_cplus);
  CHECK_NO_MATCH_LANG ("operator foo(int)", "operator foo(int &)", NORMAL,
		       language_cplus);
  CHECK_NO_MATCH_LANG ("operator foo(int)", "operator foo(int, char *)", NORMAL,
		       language_cplus);
  CHECK_NO_MATCH_LANG ("operator foo(int)", "operator bar(int)", NORMAL,
		       language_cplus);

  CHECK_NO_MATCH_LANG ("a::operator b::foo(int)", "a::operator a::foo(char)", NORMAL,
		       language_cplus);
  CHECK_NO_MATCH_LANG ("a::operator foo(int)", "a::operator foo(int *)", NORMAL,
		       language_cplus);
  CHECK_NO_MATCH_LANG ("a::operator foo(int)", "a::operator foo(int &)", NORMAL,
		       language_cplus);
  CHECK_NO_MATCH_LANG ("a::operator foo(int)", "a::operator foo(int, char *)", NORMAL,
		       language_cplus);
  CHECK_NO_MATCH_LANG ("a::operator foo(int)", "a::operator bar(int)", NORMAL,
		       language_cplus);

  /* Skip "[abi:cxx11]" tags in the symbol name if the lookup name
     doesn't include them.  These are not language-specific in
     strncmp_iw_with_mode.  */

  CHECK_MATCH ("foo[abi:a]", "foo", NORMAL);
  CHECK_MATCH ("foo[abi:a]()", "foo", NORMAL);
  CHECK_MATCH ("foo[abi:a](a)", "foo", NORMAL);
  CHECK_MATCH ("foo[abi:a](a&,b*)", "foo", NORMAL);
  CHECK_MATCH ("foo[abi:a](a,b)", "foo(a,b)", NORMAL);
  CHECK_MATCH ("foo[abi:a](a,b) c", "foo(a,b) c", NORMAL);
  CHECK_MATCH ("foo[abi:a](a)", "foo(a)", NORMAL);
  CHECK_MATCH ("foo[abi:a](a,b)", "foo(a,b)", NORMAL);
  CHECK_MATCH ("foo[abi:a]", "foo[abi:a]", NORMAL);
  CHECK_MATCH ("foo[ abi:a]", "foo[abi:a]", NORMAL);
  CHECK_MATCH ("foo[\tabi:a]", "foo[abi:a]", NORMAL);
  CHECK_MATCH ("foo[ \tabi:a]", "foo[abi:a]", NORMAL);
  CHECK_MATCH ("foo[\t abi:a]", "foo[abi:a]", NORMAL);
  CHECK_MATCH ("foo[abi :a]", "foo[abi:a]", NORMAL);
  CHECK_MATCH ("foo[abi\t:a]", "foo[abi:a]", NORMAL);
  CHECK_MATCH ("foo[abi \t:a]", "foo[abi:a]", NORMAL);
  CHECK_MATCH ("foo[abi\t :a]", "foo[abi:a]", NORMAL);
  CHECK_MATCH ("foo[abi:a]", "foo[ abi:a]", NORMAL);
  CHECK_MATCH ("foo[abi:a]", "foo[\tabi:a]", NORMAL);
  CHECK_MATCH ("foo[abi:a]", "foo[ \tabi:a]", NORMAL);
  CHECK_MATCH ("foo[abi:a]", "foo[\t abi:a]", NORMAL);
  CHECK_MATCH ("foo[abi:a]", "foo[abi :a]", NORMAL);
  CHECK_MATCH ("foo[abi:a]", "foo[abi\t:a]", NORMAL);
  CHECK_MATCH ("foo[abi:a]", "foo[abi \t:a]", NORMAL);
  CHECK_MATCH ("foo[abi:a]", "foo[abi\t :a]", NORMAL);
  CHECK_MATCH ("foo[abi:a]", "foo[abi:a ]", NORMAL);
  CHECK_MATCH ("foo[abi:a]", "foo[abi:a\t]", NORMAL);
  CHECK_MATCH ("foo[abi:a]", "foo[abi:a \t]", NORMAL);
  CHECK_MATCH ("foo[abi:a]", "foo[abi:a\t ]", NORMAL);
  CHECK_MATCH ("foo[abi:a,b]", "foo[abi:a,b]", NORMAL);
  CHECK_MATCH ("foo[abi:::]", "foo[abi:::]", NORMAL);
  CHECK_MATCH ("foo[abi : : : ]", "foo[abi:::]", NORMAL);
  CHECK_MATCH ("foo[abi:::]", "foo[abi : : : ]", NORMAL);
  CHECK_MATCH ("foo[ \t abi  \t:\t:   :   \t]",
	       "foo[   abi :                \t    ::]",
	       NORMAL);
  CHECK_MATCH ("foo< bar< baz< quxi > > >(int)", "foo<bar<baz<quxi>>>(int)",
	       NORMAL);
  CHECK_MATCH ("\tfoo<\tbar<\tbaz\t<\tquxi\t>\t>\t>(int)",
	       "foo<bar<baz<quxi>>>(int)", NORMAL);
  CHECK_MATCH (" \tfoo \t< \tbar \t< \tbaz \t< \tquxi \t> \t> \t> \t( \tint \t)",
	       "foo<bar<baz<quxi>>>(int)", NORMAL);
  CHECK_MATCH ("foo<bar<baz<quxi>>>(int)",
	       "foo < bar < baz < quxi > > > (int)", NORMAL);
  CHECK_MATCH ("foo<bar<baz<quxi>>>(int)",
	       "\tfoo\t<\tbar\t<\tbaz\t<\tquxi\t>\t>\t>\t(int)", NORMAL);
  CHECK_MATCH ("foo<bar<baz<quxi>>>(int)",
	       " \tfoo \t< \tbar \t< \tbaz \t< \tquxi \t> \t> \t> \t( \tint \t)", NORMAL);
  CHECK_MATCH ("foo<bar<baz>>::foo(quxi &)", "fo", NORMAL);
  CHECK_MATCH ("foo<bar<baz>>::foo(quxi &)", "foo", NORMAL);
  CHECK_MATCH ("foo<bar<baz>>::foo(quxi &)", "foo<bar<baz>>::", NORMAL);
  CHECK_MATCH ("foo<bar<baz>>::foo(quxi &)", "foo<bar<baz> >::foo", NORMAL);
  CHECK_MATCH ("foo[abi:a][abi:b](bar[abi:c][abi:d])", "foo[abi:a][abi:b](bar[abi:c][abi:d])",
	       NORMAL);
  CHECK_MATCH ("foo[abi:a][abi:b](bar[abi:c][abi:d])", "foo", NORMAL);
  CHECK_MATCH ("foo[abi:a][abi:b](bar[abi:c][abi:d])", "foo(bar)", NORMAL);
  CHECK_MATCH ("foo[abi:a][abi:b](bar[abi:c][abi:d])", "foo[abi:a](bar)", NORMAL);
  CHECK_MATCH ("foo[abi:a][abi:b](bar[abi:c][abi:d])", "foo(bar[abi:c])", NORMAL);
  CHECK_MATCH ("foo[abi:a][abi:b](bar[abi:c][abi:d])", "foo[abi:a](bar[abi:c])", NORMAL);
  CHECK_MATCH ("foo[abi:a][abi:b](bar[abi:c][abi:d])", "foo[abi:a][abi:b](bar)", NORMAL);
  CHECK_MATCH ("foo[abi:a][abi:b](bar[abi:c][abi:d])", "foo[abi:a][abi:b](bar[abi:c])",
	       NORMAL);
  CHECK_MATCH("foo<bar[abi:a]>(char *, baz[abi:b])", "foo", NORMAL);
  CHECK_NO_MATCH("foo<bar[abi:a]>(char *, baz[abi:b])", "foo()", NORMAL);
  CHECK_MATCH("foo<bar[abi:a]>(char *, baz[abi:b])", "foo<bar>", NORMAL);
  CHECK_MATCH("foo<bar[abi:a]>(char *, baz[abi:b])", "foo<bar>(char*, baz)", NORMAL);
  CHECK_MATCH("foo<bar[abi:a]>(char *, baz[abi:b])", "foo<bar>(char*, baz[abi:b])",
	      NORMAL);
  CHECK_NO_MATCH("foo<bar[abi:a]>(char *, baz[abi:b])", "foo<bar>(char*, baz[abi:A])",
	      NORMAL);
  CHECK_MATCH("foo<bar[abi:a]>(char *, baz[abi:b])", "foo<bar[abi:a]>(char*, baz)",
	      NORMAL);
  CHECK_NO_MATCH("foo<bar[abi:a]>(char *, baz[abi:b])", "foo<bar[abi:A]>(char*, baz)",
	      NORMAL);
  CHECK_MATCH("foo<bar[abi:a]>(char *, baz[abi:b])", "foo<bar[abi:a]>(char*, baz[abi:b])",
	      NORMAL);
  CHECK_NO_MATCH("foo<bar[abi:a]>(char *, baz[abi:b])",
		 "foo<bar[abi:a]>(char*, baz[abi:B])", NORMAL);

  CHECK_NO_MATCH ("foo", "foo[", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[ a]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[a ]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[ a ]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[\ta]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[a \t]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[a\t ]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[ \ta]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[\t a]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[ \ta \t]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[\t a\t ]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[ abi]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi ]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[\tabi]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi\t]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[ \tabi]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[\t abi]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi \t]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi\t ]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi :]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi\t:]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi \t:]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi\t :]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi: ]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi:\t]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi: \t]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi:\t ]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi: a]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi:\ta]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi: \ta]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi:\t a]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi:a ]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi:a\t]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi:a \t]", NORMAL);
  CHECK_NO_MATCH ("foo", "foo[abi:a\t ]", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a]()", "foo(a)", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a]()", "foo(a)", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a]()", "foo(a)", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a]()", "foo(a)", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a]()", "foo(a) c", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a]()", "foo(a) .", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a]()", "foo(a) *", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a]()", "foo(a) &", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a](a,b)", "foo(a,b) c", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a](a,b)", "foo(a,b) .", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a](a,b)", "foo(a,b) *", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a](a,b)", "foo(a,b) &", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a](a,b)", "foo(a,b)c", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a](a,b)", "foo(a,b).", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a](a,b)", "foo(a,b)*", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a](a,b)", "foo(a,b)&", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a](a,b) d", "foo(a,b) c", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a](a)", "foo()", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a](a)", "foo(b)", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a](a)", "foo[abi:b](a)", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a](a)", "foo[abi:a](b)", NORMAL);
  CHECK_NO_MATCH ("foo[abi:]", "foo[abi:a]", NORMAL);
  CHECK_NO_MATCH ("foo[abi:", "foo[abi:a]", NORMAL);
  CHECK_NO_MATCH ("foo[abi:]", "foo[abi:a", NORMAL);
  CHECK_NO_MATCH ("foo[abi:,]", "foo[abi:a]", NORMAL);
  CHECK_NO_MATCH ("foo[abi:a,b]", "foo[abi:a]", NORMAL);
  CHECK_NO_MATCH ("foo[abi::a]", "foo[abi:a]", NORMAL);
  CHECK_NO_MATCH ("foo[abi:,([a]", "foo[abi:a]", NORMAL);

  CHECK_MATCH ("foo <a, b [, c (",  "foo", NORMAL);
  CHECK_MATCH ("foo >a, b ], c )",  "foo", NORMAL);
  CHECK_MATCH ("@!%&\\*", "@!%&\\*", NORMAL);
  CHECK_MATCH ("()", "()", NORMAL);
  CHECK_MATCH ("*(*)*", "*(*)*", NORMAL);
  CHECK_MATCH ("[]", "[]", NORMAL);
  CHECK_MATCH ("<>", "<>", NORMAL);

  /* strncmp_iw_with_mode::MATCH_PARAMS: the "strcmp_iw hack."  */
  CHECK_MATCH ("foo2", "foo", NORMAL);
  CHECK_NO_MATCH ("foo2", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2", "foo ", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2", "foo\t", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2", "foo \t", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2", "foo\t ", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2", "foo \t", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2", " foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2", "\tfoo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2", " \tfoo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2", "\t foo", MATCH_PARAMS);
  CHECK_NO_MATCH (" foo2", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("\tfoo2", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH (" \tfoo2", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("\t foo2", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH (" foo2 ", " foo ", MATCH_PARAMS);
  CHECK_NO_MATCH ("\tfoo2\t", "\tfoo\t", MATCH_PARAMS);
  CHECK_NO_MATCH (" \tfoo2 \t", " \tfoo \t", MATCH_PARAMS);
  CHECK_NO_MATCH ("\t foo2\t ", "\t foo\t ", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2 ", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2\t", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2 ", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2 \t", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2\t ", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2 (args)", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2 (args)", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2\t(args)", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2 \t(args)", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2\t (args)", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2 ( args)", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2(args )", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2(args\t)", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2 (args \t)", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo2 (args\t )", "foo", MATCH_PARAMS);
  CHECK_MATCH ("foo[abi:a][abi:b](bar[abi:c][abi:d])", "foo[abi:a][abi:b](bar[abi:c][abi:d])",
	       MATCH_PARAMS);
  CHECK_MATCH ("foo[abi:a][abi:b](bar[abi:c][abi:d])", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo(args)@plt", "foo", MATCH_PARAMS);
  CHECK_NO_MATCH ("foo((())args(()))@plt", "foo", MATCH_PARAMS);
  CHECK_MATCH ("foo((())args(()))", "foo", MATCH_PARAMS);
  CHECK_MATCH ("foo(args) const", "foo", MATCH_PARAMS);
  CHECK_MATCH ("foo(args)const", "foo", MATCH_PARAMS);

  /* strncmp_iw_with_mode also supports case insensitivity.  */
  {
    CHECK_NO_MATCH ("FoO", "foo", NORMAL);
    CHECK_NO_MATCH ("FoO", "foo", MATCH_PARAMS);

    scoped_restore restore_case = make_scoped_restore (&case_sensitivity);
    case_sensitivity = case_sensitive_off;

    CHECK_MATCH ("FoO", "foo", NORMAL);
    CHECK_MATCH ("FoO", "foo", MATCH_PARAMS);
    CHECK_MATCH ("foo", "FoO", NORMAL);
    CHECK_MATCH ("foo", "FoO", MATCH_PARAMS);

    CHECK_MATCH ("FoO[AbI:abC]()", "foo", NORMAL);
    CHECK_NO_MATCH ("FoO[AbI:abC]()", "foo", MATCH_PARAMS);
    CHECK_MATCH ("FoO2[AbI:abC]()", "foo", NORMAL);
    CHECK_NO_MATCH ("FoO2[AbI:abC]()", "foo", MATCH_PARAMS);

    CHECK_MATCH ("foo[abi:abc]()", "FoO[AbI:abC]()", NORMAL);
    CHECK_MATCH ("foo[abi:abc]()", "FoO[AbI:AbC]()", MATCH_PARAMS);
    CHECK_MATCH ("foo[abi:abc](xyz)", "FoO[AbI:abC](XyZ)", NORMAL);
    CHECK_MATCH ("foo[abi:abc](xyz)", "FoO[AbI:abC](XyZ)", MATCH_PARAMS);
    CHECK_MATCH ("foo[abi:abc][abi:def](xyz)", "FoO[AbI:abC](XyZ)", NORMAL);
    CHECK_MATCH ("foo[abi:abc][abi:def](xyz)", "FoO[AbI:abC](XyZ)",
		 MATCH_PARAMS);
    CHECK_MATCH ("foo<bar<baz>>(bar<baz>)", "FoO<bAr<BaZ>>(bAr<BaZ>)",
		 NORMAL);
    CHECK_MATCH ("foo<bar<baz>>(bar<baz>)", "FoO<bAr<BaZ>>(bAr<BaZ>)",
		 MATCH_PARAMS);
  }
}

#undef MATCH
#undef NO_MATCH
#endif

/* See utils.h.  */

int
strncmp_iw (const char *string1, const char *string2, size_t string2_len)
{
  return strncmp_iw_with_mode (string1, string2, string2_len,
			       strncmp_iw_mode::NORMAL, language_minimal);
}

/* See utils.h.  */

int
strcmp_iw (const char *string1, const char *string2)
{
  return strncmp_iw_with_mode (string1, string2, strlen (string2),
			       strncmp_iw_mode::MATCH_PARAMS, language_minimal);
}

/* This is like strcmp except that it ignores whitespace and treats
   '(' as the first non-NULL character in terms of ordering.  Like
   strcmp (and unlike strcmp_iw), it returns negative if STRING1 <
   STRING2, 0 if STRING2 = STRING2, and positive if STRING1 > STRING2
   according to that ordering.

   If a list is sorted according to this function and if you want to
   find names in the list that match some fixed NAME according to
   strcmp_iw(LIST_ELT, NAME), then the place to start looking is right
   where this function would put NAME.

   This function must be neutral to the CASE_SENSITIVITY setting as the user
   may choose it during later lookup.  Therefore this function always sorts
   primarily case-insensitively and secondarily case-sensitively.

   Here are some examples of why using strcmp to sort is a bad idea:

   Whitespace example:

   Say your partial symtab contains: "foo<char *>", "goo".  Then, if
   we try to do a search for "foo<char*>", strcmp will locate this
   after "foo<char *>" and before "goo".  Then lookup_partial_symbol
   will start looking at strings beginning with "goo", and will never
   see the correct match of "foo<char *>".

   Parenthesis example:

   In practice, this is less like to be an issue, but I'll give it a
   shot.  Let's assume that '$' is a legitimate character to occur in
   symbols.  (Which may well even be the case on some systems.)  Then
   say that the partial symbol table contains "foo$" and "foo(int)".
   strcmp will put them in this order, since '$' < '('.  Now, if the
   user searches for "foo", then strcmp will sort "foo" before "foo$".
   Then lookup_partial_symbol will notice that strcmp_iw("foo$",
   "foo") is false, so it won't proceed to the actual match of
   "foo(int)" with "foo".  */

int
strcmp_iw_ordered (const char *string1, const char *string2)
{
  const char *saved_string1 = string1, *saved_string2 = string2;
  enum case_sensitivity case_pass = case_sensitive_off;

  for (;;)
    {
      /* C1 and C2 are valid only if *string1 != '\0' && *string2 != '\0'.
	 Provide stub characters if we are already at the end of one of the
	 strings.  */
      char c1 = 'X', c2 = 'X';

      while (*string1 != '\0' && *string2 != '\0')
	{
	  while (ISSPACE (*string1))
	    string1++;
	  while (ISSPACE (*string2))
	    string2++;

	  switch (case_pass)
	  {
	    case case_sensitive_off:
	      c1 = TOLOWER ((unsigned char) *string1);
	      c2 = TOLOWER ((unsigned char) *string2);
	      break;
	    case case_sensitive_on:
	      c1 = *string1;
	      c2 = *string2;
	      break;
	  }
	  if (c1 != c2)
	    break;

	  if (*string1 != '\0')
	    {
	      string1++;
	      string2++;
	    }
	}

      switch (*string1)
	{
	  /* Characters are non-equal unless they're both '\0'; we want to
	     make sure we get the comparison right according to our
	     comparison in the cases where one of them is '\0' or '('.  */
	case '\0':
	  if (*string2 == '\0')
	    break;
	  else
	    return -1;
	case '(':
	  if (*string2 == '\0')
	    return 1;
	  else
	    return -1;
	default:
	  if (*string2 == '\0' || *string2 == '(')
	    return 1;
	  else if (c1 > c2)
	    return 1;
	  else if (c1 < c2)
	    return -1;
	  /* PASSTHRU */
	}

      if (case_pass == case_sensitive_on)
	return 0;
      
      /* Otherwise the strings were equal in case insensitive way, make
	 a more fine grained comparison in a case sensitive way.  */

      case_pass = case_sensitive_on;
      string1 = saved_string1;
      string2 = saved_string2;
    }
}



static void
show_debug_timestamp (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Timestamping debugging messages is %s.\n"),
	      value);
}


const char *
paddress (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  /* Truncate address to the size of a target address, avoiding shifts
     larger or equal than the width of a CORE_ADDR.  The local
     variable ADDR_BIT stops the compiler reporting a shift overflow
     when it won't occur.  */
  /* NOTE: This assumes that the significant address information is
     kept in the least significant bits of ADDR - the upper bits were
     either zero or sign extended.  Should gdbarch_address_to_pointer or
     some ADDRESS_TO_PRINTABLE() be used to do the conversion?  */

  int addr_bit = gdbarch_addr_bit (gdbarch);

  if (addr_bit < (sizeof (CORE_ADDR) * HOST_CHAR_BIT))
    addr &= ((CORE_ADDR) 1 << addr_bit) - 1;
  return hex_string (addr);
}

/* This function is described in "defs.h".  */

const char *
print_core_address (struct gdbarch *gdbarch, CORE_ADDR address)
{
  int addr_bit = gdbarch_addr_bit (gdbarch);

  if (addr_bit < (sizeof (CORE_ADDR) * HOST_CHAR_BIT))
    address &= ((CORE_ADDR) 1 << addr_bit) - 1;

  /* FIXME: cagney/2002-05-03: Need local_address_string() function
     that returns the language localized string formatted to a width
     based on gdbarch_addr_bit.  */
  if (addr_bit <= 32)
    return hex_string_custom (address, 8);
  else
    return hex_string_custom (address, 16);
}

/* Convert a string back into a CORE_ADDR.  */
CORE_ADDR
string_to_core_addr (const char *my_string)
{
  CORE_ADDR addr = 0;

  if (my_string[0] == '0' && TOLOWER (my_string[1]) == 'x')
    {
      /* Assume that it is in hex.  */
      int i;

      for (i = 2; my_string[i] != '\0'; i++)
	{
	  if (ISDIGIT (my_string[i]))
	    addr = (my_string[i] - '0') + (addr * 16);
	  else if (ISXDIGIT (my_string[i]))
	    addr = (TOLOWER (my_string[i]) - 'a' + 0xa) + (addr * 16);
	  else
	    error (_("invalid hex \"%s\""), my_string);
	}
    }
  else
    {
      /* Assume that it is in decimal.  */
      int i;

      for (i = 0; my_string[i] != '\0'; i++)
	{
	  if (ISDIGIT (my_string[i]))
	    addr = (my_string[i] - '0') + (addr * 10);
	  else
	    error (_("invalid decimal \"%s\""), my_string);
	}
    }

  return addr;
}

#if GDB_SELF_TEST

static void
gdb_realpath_check_trailer (const char *input, const char *trailer)
{
  gdb::unique_xmalloc_ptr<char> result = gdb_realpath (input);

  size_t len = strlen (result.get ());
  size_t trail_len = strlen (trailer);

  SELF_CHECK (len >= trail_len
	      && strcmp (result.get () + len - trail_len, trailer) == 0);
}

static void
gdb_realpath_tests ()
{
  /* A file which contains a directory prefix.  */
  gdb_realpath_check_trailer ("./xfullpath.exp", "/xfullpath.exp");
  /* A file which contains a directory prefix.  */
  gdb_realpath_check_trailer ("../../defs.h", "/defs.h");
  /* A one-character filename.  */
  gdb_realpath_check_trailer ("./a", "/a");
  /* A file in the root directory.  */
  gdb_realpath_check_trailer ("/root_file_which_should_exist",
			      "/root_file_which_should_exist");
  /* A file which does not have a directory prefix.  */
  gdb_realpath_check_trailer ("xfullpath.exp", "xfullpath.exp");
  /* A one-char filename without any directory prefix.  */
  gdb_realpath_check_trailer ("a", "a");
  /* An empty filename.  */
  gdb_realpath_check_trailer ("", "");
}

/* Test the gdb_argv::as_array_view method.  */

static void
gdb_argv_as_array_view_test ()
{
  {
    gdb_argv argv;

    gdb::array_view<char *> view = argv.as_array_view ();

    SELF_CHECK (view.data () == nullptr);
    SELF_CHECK (view.size () == 0);
  }
  {
    gdb_argv argv ("une bonne 50");

    gdb::array_view<char *> view = argv.as_array_view ();

    SELF_CHECK (view.size () == 3);
    SELF_CHECK (strcmp (view[0], "une") == 0);
    SELF_CHECK (strcmp (view[1], "bonne") == 0);
    SELF_CHECK (strcmp (view[2], "50") == 0);
  }
}

#endif /* GDB_SELF_TEST */

/* Simple, portable version of dirname that does not modify its
   argument.  */

std::string
ldirname (const char *filename)
{
  std::string dirname;
  const char *base = lbasename (filename);

  while (base > filename && IS_DIR_SEPARATOR (base[-1]))
    --base;

  if (base == filename)
    return dirname;

  dirname = std::string (filename, base - filename);

  /* On DOS based file systems, convert "d:foo" to "d:.", so that we
     create "d:./bar" later instead of the (different) "d:/bar".  */
  if (base - filename == 2 && IS_ABSOLUTE_PATH (base)
      && !IS_DIR_SEPARATOR (filename[0]))
    dirname[base++ - filename] = '.';

  return dirname;
}

/* Return ARGS parsed as a valid pid, or throw an error.  */

int
parse_pid_to_attach (const char *args)
{
  unsigned long pid;
  char *dummy;

  if (!args)
    error_no_arg (_("process-id to attach"));

  dummy = (char *) args;
  pid = strtoul (args, &dummy, 0);
  /* Some targets don't set errno on errors, grrr!  */
  if ((pid == 0 && dummy == args) || dummy != &args[strlen (args)])
    error (_("Illegal process-id: %s."), args);

  return pid;
}

/* Substitute all occurrences of string FROM by string TO in *STRINGP.  *STRINGP
   must come from xrealloc-compatible allocator and it may be updated.  FROM
   needs to be delimited by IS_DIR_SEPARATOR or DIRNAME_SEPARATOR (or be
   located at the start or end of *STRINGP.  */

void
substitute_path_component (char **stringp, const char *from, const char *to)
{
  char *string = *stringp, *s;
  const size_t from_len = strlen (from);
  const size_t to_len = strlen (to);

  for (s = string;;)
    {
      s = strstr (s, from);
      if (s == NULL)
	break;

      if ((s == string || IS_DIR_SEPARATOR (s[-1])
	   || s[-1] == DIRNAME_SEPARATOR)
	  && (s[from_len] == '\0' || IS_DIR_SEPARATOR (s[from_len])
	      || s[from_len] == DIRNAME_SEPARATOR))
	{
	  char *string_new;

	  string_new
	    = (char *) xrealloc (string, (strlen (string) + to_len + 1));

	  /* Relocate the current S pointer.  */
	  s = s - string + string_new;
	  string = string_new;

	  /* Replace from by to.  */
	  memmove (&s[to_len], &s[from_len], strlen (&s[from_len]) + 1);
	  memcpy (s, to, to_len);

	  s += to_len;
	}
      else
	s++;
    }

  *stringp = string;
}

#ifdef HAVE_WAITPID

#ifdef SIGALRM

/* SIGALRM handler for waitpid_with_timeout.  */

static void
sigalrm_handler (int signo)
{
  /* Nothing to do.  */
}

#endif

/* Wrapper to wait for child PID to die with TIMEOUT.
   TIMEOUT is the time to stop waiting in seconds.
   If TIMEOUT is zero, pass WNOHANG to waitpid.
   Returns PID if it was successfully waited for, otherwise -1.

   Timeouts are currently implemented with alarm and SIGALRM.
   If the host does not support them, this waits "forever".
   It would be odd though for a host to have waitpid and not SIGALRM.  */

pid_t
wait_to_die_with_timeout (pid_t pid, int *status, int timeout)
{
  pid_t waitpid_result;

  gdb_assert (pid > 0);
  gdb_assert (timeout >= 0);

  if (timeout > 0)
    {
#ifdef SIGALRM
#if defined (HAVE_SIGACTION) && defined (SA_RESTART)
      struct sigaction sa, old_sa;

      sa.sa_handler = sigalrm_handler;
      sigemptyset (&sa.sa_mask);
      sa.sa_flags = 0;
      sigaction (SIGALRM, &sa, &old_sa);
#else
      sighandler_t ofunc;

      ofunc = signal (SIGALRM, sigalrm_handler);
#endif

      alarm (timeout);
#endif

      waitpid_result = waitpid (pid, status, 0);

#ifdef SIGALRM
      alarm (0);
#if defined (HAVE_SIGACTION) && defined (SA_RESTART)
      sigaction (SIGALRM, &old_sa, NULL);
#else
      signal (SIGALRM, ofunc);
#endif
#endif
    }
  else
    waitpid_result = waitpid (pid, status, WNOHANG);

  if (waitpid_result == pid)
    return pid;
  else
    return -1;
}

#endif /* HAVE_WAITPID */

/* Provide fnmatch compatible function for FNM_FILE_NAME matching of host files.
   Both FNM_FILE_NAME and FNM_NOESCAPE must be set in FLAGS.

   It handles correctly HAVE_DOS_BASED_FILE_SYSTEM and
   HAVE_CASE_INSENSITIVE_FILE_SYSTEM.  */

int
gdb_filename_fnmatch (const char *pattern, const char *string, int flags)
{
  gdb_assert ((flags & FNM_FILE_NAME) != 0);

  /* It is unclear how '\' escaping vs. directory separator should coexist.  */
  gdb_assert ((flags & FNM_NOESCAPE) != 0);

#ifdef HAVE_DOS_BASED_FILE_SYSTEM
  {
    char *pattern_slash, *string_slash;

    /* Replace '\' by '/' in both strings.  */

    pattern_slash = (char *) alloca (strlen (pattern) + 1);
    strcpy (pattern_slash, pattern);
    pattern = pattern_slash;
    for (; *pattern_slash != 0; pattern_slash++)
      if (IS_DIR_SEPARATOR (*pattern_slash))
	*pattern_slash = '/';

    string_slash = (char *) alloca (strlen (string) + 1);
    strcpy (string_slash, string);
    string = string_slash;
    for (; *string_slash != 0; string_slash++)
      if (IS_DIR_SEPARATOR (*string_slash))
	*string_slash = '/';
  }
#endif /* HAVE_DOS_BASED_FILE_SYSTEM */

#ifdef HAVE_CASE_INSENSITIVE_FILE_SYSTEM
  flags |= FNM_CASEFOLD;
#endif /* HAVE_CASE_INSENSITIVE_FILE_SYSTEM */

  return fnmatch (pattern, string, flags);
}

/* Return the number of path elements in PATH.
   / = 1
   /foo = 2
   /foo/ = 2
   foo/bar = 2
   foo/ = 1  */

int
count_path_elements (const char *path)
{
  int count = 0;
  const char *p = path;

  if (HAS_DRIVE_SPEC (p))
    {
      p = STRIP_DRIVE_SPEC (p);
      ++count;
    }

  while (*p != '\0')
    {
      if (IS_DIR_SEPARATOR (*p))
	++count;
      ++p;
    }

  /* Backup one if last character is /, unless it's the only one.  */
  if (p > path + 1 && IS_DIR_SEPARATOR (p[-1]))
    --count;

  /* Add one for the file name, if present.  */
  if (p > path && !IS_DIR_SEPARATOR (p[-1]))
    ++count;

  return count;
}

/* Remove N leading path elements from PATH.
   N must be non-negative.
   If PATH has more than N path elements then return NULL.
   If PATH has exactly N path elements then return "".
   See count_path_elements for a description of how we do the counting.  */

const char *
strip_leading_path_elements (const char *path, int n)
{
  int i = 0;
  const char *p = path;

  gdb_assert (n >= 0);

  if (n == 0)
    return p;

  if (HAS_DRIVE_SPEC (p))
    {
      p = STRIP_DRIVE_SPEC (p);
      ++i;
    }

  while (i < n)
    {
      while (*p != '\0' && !IS_DIR_SEPARATOR (*p))
	++p;
      if (*p == '\0')
	{
	  if (i + 1 == n)
	    return "";
	  return NULL;
	}
      ++p;
      ++i;
    }

  return p;
}

/* See utils.h.  */

void
copy_bitwise (gdb_byte *dest, ULONGEST dest_offset,
	      const gdb_byte *source, ULONGEST source_offset,
	      ULONGEST nbits, int bits_big_endian)
{
  unsigned int buf, avail;

  if (nbits == 0)
    return;

  if (bits_big_endian)
    {
      /* Start from the end, then work backwards.  */
      dest_offset += nbits - 1;
      dest += dest_offset / 8;
      dest_offset = 7 - dest_offset % 8;
      source_offset += nbits - 1;
      source += source_offset / 8;
      source_offset = 7 - source_offset % 8;
    }
  else
    {
      dest += dest_offset / 8;
      dest_offset %= 8;
      source += source_offset / 8;
      source_offset %= 8;
    }

  /* Fill BUF with DEST_OFFSET bits from the destination and 8 -
     SOURCE_OFFSET bits from the source.  */
  buf = *(bits_big_endian ? source-- : source++) >> source_offset;
  buf <<= dest_offset;
  buf |= *dest & ((1 << dest_offset) - 1);

  /* NBITS: bits yet to be written; AVAIL: BUF's fill level.  */
  nbits += dest_offset;
  avail = dest_offset + 8 - source_offset;

  /* Flush 8 bits from BUF, if appropriate.  */
  if (nbits >= 8 && avail >= 8)
    {
      *(bits_big_endian ? dest-- : dest++) = buf;
      buf >>= 8;
      avail -= 8;
      nbits -= 8;
    }

  /* Copy the middle part.  */
  if (nbits >= 8)
    {
      size_t len = nbits / 8;

      /* Use a faster method for byte-aligned copies.  */
      if (avail == 0)
	{
	  if (bits_big_endian)
	    {
	      dest -= len;
	      source -= len;
	      memcpy (dest + 1, source + 1, len);
	    }
	  else
	    {
	      memcpy (dest, source, len);
	      dest += len;
	      source += len;
	    }
	}
      else
	{
	  while (len--)
	    {
	      buf |= *(bits_big_endian ? source-- : source++) << avail;
	      *(bits_big_endian ? dest-- : dest++) = buf;
	      buf >>= 8;
	    }
	}
      nbits %= 8;
    }

  /* Write the last byte.  */
  if (nbits)
    {
      if (avail < nbits)
	buf |= *source << avail;

      buf &= (1 << nbits) - 1;
      *dest = (*dest & (~0U << nbits)) | buf;
    }
}

#if GDB_SELF_TEST
static void
test_assign_set_return_if_changed ()
{
  bool changed;
  int a;

  for (bool initial : { false, true })
    {
      changed = initial;
      a = 1;
      assign_set_if_changed (a, 1, changed);
      SELF_CHECK (a == 1);
      SELF_CHECK (changed == initial);
    }

  for (bool initial : { false, true })
    {
      changed = initial;
      a = 1;
      assign_set_if_changed (a, 2, changed);
      SELF_CHECK (a == 2);
      SELF_CHECK (changed == true);
    }

  a = 1;
  changed = assign_return_if_changed (a, 1);
  SELF_CHECK (a == 1);
  SELF_CHECK (changed == false);

  a = 1;
  assign_set_if_changed (a, 2, changed);
  SELF_CHECK (a == 2);
  SELF_CHECK (changed == true);
}
#endif

void _initialize_utils ();
void
_initialize_utils ()
{
  add_setshow_uinteger_cmd ("width", class_support, &chars_per_line, _("\
Set number of characters where GDB should wrap lines of its output."), _("\
Show number of characters where GDB should wrap lines of its output."), _("\
This affects where GDB wraps its output to fit the screen width.\n\
Setting this to \"unlimited\" or zero prevents GDB from wrapping its output."),
			    set_width_command,
			    show_chars_per_line,
			    &setlist, &showlist);

  add_setshow_uinteger_cmd ("height", class_support, &lines_per_page, _("\
Set number of lines in a page for GDB output pagination."), _("\
Show number of lines in a page for GDB output pagination."), _("\
This affects the number of lines after which GDB will pause\n\
its output and ask you whether to continue.\n\
Setting this to \"unlimited\" or zero causes GDB never pause during output."),
			    set_height_command,
			    show_lines_per_page,
			    &setlist, &showlist);

  add_setshow_boolean_cmd ("pagination", class_support,
			   &pagination_enabled, _("\
Set state of GDB output pagination."), _("\
Show state of GDB output pagination."), _("\
When pagination is ON, GDB pauses at end of each screenful of\n\
its output and asks you whether to continue.\n\
Turning pagination off is an alternative to \"set height unlimited\"."),
			   NULL,
			   show_pagination_enabled,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("sevenbit-strings", class_support,
			   &sevenbit_strings, _("\
Set printing of 8-bit characters in strings as \\nnn."), _("\
Show printing of 8-bit characters in strings as \\nnn."), NULL,
			   NULL,
			   show_sevenbit_strings,
			   &setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("timestamp", class_maintenance,
			    &debug_timestamp, _("\
Set timestamping of debugging messages."), _("\
Show timestamping of debugging messages."), _("\
When set, debugging messages will be marked with seconds and microseconds."),
			   NULL,
			   show_debug_timestamp,
			   &setdebuglist, &showdebuglist);

  add_internal_problem_command (&internal_error_problem);
  add_internal_problem_command (&internal_warning_problem);
  add_internal_problem_command (&demangler_warning_problem);

  add_cmd ("screen", class_maintenance, &maintenance_info_screen,
	 _("Show screen characteristics."), &maintenanceinfolist);

#if GDB_SELF_TEST
  selftests::register_test ("gdb_realpath", gdb_realpath_tests);
  selftests::register_test ("gdb_argv_array_view", gdb_argv_as_array_view_test);
  selftests::register_test ("strncmp_iw_with_mode",
			    strncmp_iw_with_mode_tests);
  selftests::register_test ("pager", test_pager);
  selftests::register_test ("assign_set_return_if_changed",
			    test_assign_set_return_if_changed);
#endif
}
