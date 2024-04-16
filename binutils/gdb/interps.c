/* Manages interpreters for GDB, the GNU debugger.

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

   Written by Jim Ingham <jingham@apple.com> of Apple Computer, Inc.

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

/* This is just a first cut at separating out the "interpreter"
   functions of gdb into self-contained modules.  There are a couple
   of open areas that need to be sorted out:

   1) The interpreter explicitly contains a UI_OUT, and can insert itself
   into the event loop, but it doesn't explicitly contain hooks for readline.
   I did this because it seems to me many interpreters won't want to use
   the readline command interface, and it is probably simpler to just let
   them take over the input in their resume proc.  */

#include "defs.h"
#include "gdbcmd.h"
#include "ui-out.h"
#include "gdbsupport/event-loop.h"
#include "event-top.h"
#include "interps.h"
#include "completer.h"
#include "ui.h"
#include "main.h"
#include "gdbsupport/buildargv.h"
#include "gdbsupport/scope-exit.h"

/* The magic initialization routine for this module.  */

static struct interp *interp_lookup_existing (struct ui *ui,
					      const char *name);

interp::interp (const char *name)
  : m_name (name)
{
}

interp::~interp () = default;

/* An interpreter factory.  Maps an interpreter name to the factory
   function that instantiates an interpreter by that name.  */

struct interp_factory
{
  interp_factory (const char *name_, interp_factory_func func_)
  : name (name_), func (func_)
  {}

  /* This is the name in "-i=INTERP" and "interpreter-exec INTERP".  */
  const char *name;

  /* The function that creates the interpreter.  */
  interp_factory_func func;
};

/* The registered interpreter factories.  */
static std::vector<interp_factory> interpreter_factories;

/* See interps.h.  */

void
interp_factory_register (const char *name, interp_factory_func func)
{
  /* Assert that no factory for NAME is already registered.  */
  for (const interp_factory &f : interpreter_factories)
    if (strcmp (f.name, name) == 0)
      {
	internal_error (_("interpreter factory already registered: \"%s\"\n"),
			name);
      }

  interpreter_factories.emplace_back (name, func);
}

/* Add interpreter INTERP to the gdb interpreter list.  The
   interpreter must not have previously been added.  */
static void
interp_add (struct ui *ui, struct interp *interp)
{
  gdb_assert (interp_lookup_existing (ui, interp->name ()) == NULL);

  ui->interp_list.push_back (*interp);
}

/* This sets the current interpreter to be INTERP.  If INTERP has not
   been initialized, then this will also run the init method.

   The TOP_LEVEL parameter tells if this new interpreter is
   the top-level one.  The top-level is what is requested
   on the command line, and is responsible for reporting general
   notification about target state changes.  For example, if
   MI is the top-level interpreter, then it will always report
   events such as target stops and new thread creation, even if they
   are caused by CLI commands.  */

static void
interp_set (struct interp *interp, bool top_level)
{
  struct interp *old_interp = current_ui->current_interpreter;

  /* If we already have an interpreter, then trying to
     set top level interpreter is kinda pointless.  */
  gdb_assert (!top_level || !current_ui->current_interpreter);
  gdb_assert (!top_level || !current_ui->top_level_interpreter);

  if (old_interp != NULL)
    {
      current_uiout->flush ();
      old_interp->suspend ();
    }

  current_ui->current_interpreter = interp;
  if (top_level)
    current_ui->top_level_interpreter = interp;

  if (interpreter_p != interp->name ())
    interpreter_p = interp->name ();

  /* Run the init proc.  */
  if (!interp->inited)
    {
      interp->init (top_level);
      interp->inited = true;
    }

  /* Do this only after the interpreter is initialized.  */
  current_uiout = interp->interp_ui_out ();

  /* Clear out any installed interpreter hooks/event handlers.  */
  clear_interpreter_hooks ();

  interp->resume ();
}

/* Look up the interpreter for NAME.  If no such interpreter exists,
   return NULL, otherwise return a pointer to the interpreter.  */

static struct interp *
interp_lookup_existing (struct ui *ui, const char *name)
{
  for (interp &interp : ui->interp_list)
    if (strcmp (interp.name (), name) == 0)
      return &interp;

  return nullptr;
}

/* See interps.h.  */

struct interp *
interp_lookup (struct ui *ui, const char *name)
{
  if (name == NULL || strlen (name) == 0)
    return NULL;

  /* Only create each interpreter once per top level.  */
  struct interp *interp = interp_lookup_existing (ui, name);
  if (interp != NULL)
    return interp;

  for (const interp_factory &factory : interpreter_factories)
    if (strcmp (factory.name, name) == 0)
      {
	interp = factory.func (factory.name);
	interp_add (ui, interp);
	return interp;
      }

  return NULL;
}

/* See interps.h.  */

void
set_top_level_interpreter (const char *name)
{
  /* Find it.  */
  struct interp *interp = interp_lookup (current_ui, name);

  if (interp == NULL)
    error (_("Interpreter `%s' unrecognized"), name);
  /* Install it.  */
  interp_set (interp, true);
}

void
current_interp_set_logging (ui_file_up logfile, bool logging_redirect,
			    bool debug_redirect)
{
  struct interp *interp = current_ui->current_interpreter;

  interp->set_logging (std::move (logfile), logging_redirect, debug_redirect);
}

/* Temporarily overrides the current interpreter.  */
struct interp *
scoped_restore_interp::set_interp (const char *name)
{
  struct interp *interp = interp_lookup (current_ui, name);
  struct interp *old_interp = current_ui->current_interpreter;

  if (interp)
    current_ui->current_interpreter = interp;

  return old_interp;
}

/* Returns true if the current interp is the passed in name.  */
int
current_interp_named_p (const char *interp_name)
{
  interp *interp = current_ui->current_interpreter;

  if (interp != NULL)
    return (strcmp (interp->name (), interp_name) == 0);

  return 0;
}

/* The interpreter that was active when a command was executed.
   Normally that'd always be CURRENT_INTERPRETER, except that MI's
   -interpreter-exec command doesn't actually flip the current
   interpreter when running its sub-command.  The
   `command_interpreter' global tracks when interp_exec is called
   (IOW, when -interpreter-exec is called).  If that is set, it is
   INTERP in '-interpreter-exec INTERP "CMD"' or in 'interpreter-exec
   INTERP "CMD".  Otherwise, interp_exec isn't active, and so the
   interpreter running the command is the current interpreter.  */

struct interp *
command_interp (void)
{
  if (current_ui->command_interpreter != nullptr)
    return current_ui->command_interpreter;
  else
    return current_ui->current_interpreter;
}

/* interp_exec - This executes COMMAND_STR in the current 
   interpreter.  */

void
interp_exec (struct interp *interp, const char *command_str)
{
  /* See `command_interp' for why we do this.  */
  scoped_restore save_command_interp
    = make_scoped_restore (&current_ui->command_interpreter, interp);

  interp->exec (command_str);
}

/* A convenience routine that nulls out all the common command hooks.
   Use it when removing your interpreter in its suspend proc.  */
void
clear_interpreter_hooks (void)
{
  deprecated_print_frame_info_listing_hook = 0;
  /*print_frame_more_info_hook = 0; */
  deprecated_query_hook = 0;
  deprecated_warning_hook = 0;
  deprecated_readline_begin_hook = 0;
  deprecated_readline_hook = 0;
  deprecated_readline_end_hook = 0;
  deprecated_context_hook = 0;
  deprecated_call_command_hook = 0;
  deprecated_error_begin_hook = 0;
}

static void
interpreter_exec_cmd (const char *args, int from_tty)
{
  struct interp *interp_to_use;
  unsigned int nrules;
  unsigned int i;

  /* Interpreters may clobber stdout/stderr (e.g.  in mi_interp::resume at time
     of writing), preserve their state here.  */
  scoped_restore save_stdout = make_scoped_restore (&gdb_stdout);
  scoped_restore save_stderr = make_scoped_restore (&gdb_stderr);
  scoped_restore save_stdlog = make_scoped_restore (&gdb_stdlog);
  scoped_restore save_stdtarg = make_scoped_restore (&gdb_stdtarg);
  scoped_restore save_stdtargerr = make_scoped_restore (&gdb_stdtargerr);

  if (args == NULL)
    error_no_arg (_("interpreter-exec command"));

  gdb_argv prules (args);
  nrules = prules.count ();

  if (nrules < 2)
    error (_("Usage: interpreter-exec INTERPRETER COMMAND..."));

  interp *old_interp = current_ui->current_interpreter;

  interp_to_use = interp_lookup (current_ui, prules[0]);
  if (interp_to_use == NULL)
    error (_("Could not find interpreter \"%s\"."), prules[0]);

  interp_set (interp_to_use, false);
  SCOPE_EXIT
    {
      interp_set (old_interp, false);
    };

  for (i = 1; i < nrules; i++)
    interp_exec (interp_to_use, prules[i]);
}

/* See interps.h.  */

void
interpreter_completer (struct cmd_list_element *ignore,
		       completion_tracker &tracker,
		       const char *text, const char *word)
{
  int textlen = strlen (text);

  for (const interp_factory &interp : interpreter_factories)
    {
      if (strncmp (interp.name, text, textlen) == 0)
	{
	  tracker.add_completion
	    (make_completion_match_str (interp.name, text, word));
	}
    }
}

struct interp *
top_level_interpreter (void)
{
  return current_ui->top_level_interpreter;
}

/* See interps.h.  */

struct interp *
current_interpreter (void)
{
  return current_ui->current_interpreter;
}

/* Helper interps_notify_* functions.  Call METHOD on the top-level interpreter
   of all UIs.  */

template <typename MethodType, typename ...Args>
void
interps_notify (MethodType method, Args&&... args)
{
  SWITCH_THRU_ALL_UIS ()
    {
      interp *tli = top_level_interpreter ();
      if (tli != nullptr)
	(tli->*method) (std::forward<Args> (args)...);
    }
}

/* See interps.h.  */

void
interps_notify_signal_received (gdb_signal sig)
{
  interps_notify (&interp::on_signal_received, sig);
}

/* See interps.h.  */

void
interps_notify_signal_exited (gdb_signal sig)
{
  interps_notify (&interp::on_signal_exited, sig);
}

/* See interps.h.  */

void
interps_notify_no_history ()
{
  interps_notify (&interp::on_no_history);
}

/* See interps.h.  */

void
interps_notify_normal_stop (bpstat *bs, int print_frame)
{
  interps_notify (&interp::on_normal_stop, bs, print_frame);
}

/* See interps.h.  */

void
interps_notify_exited (int status)
{
  interps_notify (&interp::on_exited, status);
}

/* See interps.h.  */

void
interps_notify_user_selected_context_changed (user_selected_what selection)
{
  interps_notify (&interp::on_user_selected_context_changed, selection);
}

/* See interps.h.  */

void
interps_notify_new_thread (thread_info *t)
{
  interps_notify (&interp::on_new_thread, t);
}

/* See interps.h.  */

void
interps_notify_thread_exited (thread_info *t,
			      std::optional<ULONGEST> exit_code,
			      int silent)
{
  interps_notify (&interp::on_thread_exited, t, exit_code, silent);
}

/* See interps.h.  */

void
interps_notify_inferior_added (inferior *inf)
{
  interps_notify (&interp::on_inferior_added, inf);
}

/* See interps.h.  */

void
interps_notify_inferior_appeared (inferior *inf)
{
  interps_notify (&interp::on_inferior_appeared, inf);
}

/* See interps.h.  */

void
interps_notify_inferior_disappeared (inferior *inf)
{
  interps_notify (&interp::on_inferior_disappeared, inf);
}

/* See interps.h.  */

void
interps_notify_inferior_removed (inferior *inf)
{
  interps_notify (&interp::on_inferior_removed, inf);
}

/* See interps.h.  */

void
interps_notify_record_changed (inferior *inf, int started, const char *method,
			       const char *format)
{
  interps_notify (&interp::on_record_changed, inf, started, method, format);
}

/* See interps.h.  */

void
interps_notify_target_resumed (ptid_t ptid)
{
  interps_notify (&interp::on_target_resumed, ptid);
}

/* See interps.h.  */

void
interps_notify_solib_loaded (const shobj &so)
{
  interps_notify (&interp::on_solib_loaded, so);
}

/* See interps.h.  */

void
interps_notify_solib_unloaded (const shobj &so)
{
  interps_notify (&interp::on_solib_unloaded, so);
}

/* See interps.h.  */

void
interps_notify_traceframe_changed (int tfnum, int tpnum)
{
  interps_notify (&interp::on_traceframe_changed, tfnum, tpnum);
}

/* See interps.h.  */

void
interps_notify_tsv_created (const trace_state_variable *tsv)
{
  interps_notify (&interp::on_tsv_created, tsv);
}

/* See interps.h.  */

void
interps_notify_tsv_deleted (const trace_state_variable *tsv)
{
  interps_notify (&interp::on_tsv_deleted, tsv);
}

/* See interps.h.  */

void
interps_notify_tsv_modified (const trace_state_variable *tsv)
{
  interps_notify (&interp::on_tsv_modified, tsv);
}

/* See interps.h.  */

void
interps_notify_breakpoint_created (breakpoint *b)
{
  interps_notify (&interp::on_breakpoint_created, b);
}

/* See interps.h.  */

void
interps_notify_breakpoint_deleted (breakpoint *b)
{
  interps_notify (&interp::on_breakpoint_deleted, b);
}

/* See interps.h.  */

void
interps_notify_breakpoint_modified (breakpoint *b)
{
  interps_notify (&interp::on_breakpoint_modified, b);
}

/* See interps.h.  */

void
interps_notify_param_changed (const char *param, const char *value)
{
  interps_notify (&interp::on_param_changed, param, value);
}

/* See interps.h.  */

void
interps_notify_memory_changed (inferior *inf, CORE_ADDR addr, ssize_t len,
			       const bfd_byte *data)
{
  interps_notify (&interp::on_memory_changed, inf, addr, len, data);
}

/* This just adds the "interpreter-exec" command.  */
void _initialize_interpreter ();
void
_initialize_interpreter ()
{
  struct cmd_list_element *c;

  c = add_cmd ("interpreter-exec", class_support,
	       interpreter_exec_cmd, _("\
Execute a command in an interpreter.\n\
Usage: interpreter-exec INTERPRETER COMMAND...\n\
The first argument is the name of the interpreter to use.\n\
The following arguments are the commands to execute.\n\
A command can have arguments, separated by spaces.\n\
These spaces must be escaped using \\ or the command\n\
and its arguments must be enclosed in double quotes."), &cmdlist);
  set_cmd_completer (c, interpreter_completer);
}
