/* MI Interpreter Definitions and Commands for GDB, the GNU debugger.

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

#include "mi-interp.h"

#include "interps.h"
#include "event-top.h"
#include "gdbsupport/event-loop.h"
#include "inferior.h"
#include "infrun.h"
#include "ui-out.h"
#include "ui.h"
#include "mi-main.h"
#include "mi-cmds.h"
#include "mi-out.h"
#include "mi-console.h"
#include "mi-common.h"
#include "observable.h"
#include "gdbthread.h"
#include "solist.h"
#include "objfiles.h"
#include "tracepoint.h"
#include "cli-out.h"
#include "thread-fsm.h"
#include "cli/cli-interp.h"
#include "gdbsupport/scope-exit.h"

/* These are the interpreter setup, etc. functions for the MI
   interpreter.  */

static void mi_execute_command_wrapper (const char *cmd);
static void mi_execute_command_input_handler
  (gdb::unique_xmalloc_ptr<char> &&cmd);

/* These are hooks that we put in place while doing interpreter_exec
   so we can report interesting things that happened "behind the MI's
   back" in this command.  */

static int mi_interp_query_hook (const char *ctlstr, va_list ap)
  ATTRIBUTE_PRINTF (1, 0);

static void mi_insert_notify_hooks (void);
static void mi_remove_notify_hooks (void);

/* Display the MI prompt.  */

static void
display_mi_prompt (struct mi_interp *mi)
{
  struct ui *ui = current_ui;

  gdb_puts ("(gdb) \n", mi->raw_stdout);
  gdb_flush (mi->raw_stdout);
  ui->prompt_state = PROMPTED;
}

void
mi_interp::on_command_error ()
{
  display_mi_prompt (this);
}

void
mi_interp::init (bool top_level)
{
  mi_interp *mi = this;

  /* Store the current output channel, so that we can create a console
     channel that encapsulates and prefixes all gdb_output-type bits
     coming from the rest of the debugger.  */
  mi->raw_stdout = gdb_stdout;

  /* Create MI console channels, each with a different prefix so they
     can be distinguished.  */
  mi->out = new mi_console_file (mi->raw_stdout, "~", '"');
  mi->err = new mi_console_file (mi->raw_stdout, "&", '"');
  mi->log = mi->err;
  mi->targ = new mi_console_file (mi->raw_stdout, "@", '"');
  mi->event_channel = new mi_console_file (mi->raw_stdout, "=", 0);
  mi->mi_uiout = mi_out_new (name ()).release ();
  gdb_assert (mi->mi_uiout != nullptr);
  mi->cli_uiout = new cli_ui_out (mi->out);

  if (top_level)
    {
      /* The initial inferior is created before this function is called, so we
	 need to report it explicitly when initializing the top-level MI
	 interpreter.

	 This is also called when additional MI interpreters are added (using
	 the new-ui command), when multiple inferiors possibly exist, so we need
	 to use iteration to report all the inferiors.  */

      for (inferior *inf : all_inferiors ())
	mi->on_inferior_added (inf);
  }
}

void
mi_interp::resume ()
{
  struct mi_interp *mi = this;
  struct ui *ui = current_ui;

  /* As per hack note in mi_interpreter_init, swap in the output
     channels... */
  gdb_setup_readline (0);

  ui->call_readline = gdb_readline_no_editing_callback;
  ui->input_handler = mi_execute_command_input_handler;

  gdb_stdout = mi->out;
  /* Route error and log output through the MI.  */
  gdb_stderr = mi->err;
  gdb_stdlog = mi->log;
  /* Route target output through the MI.  */
  gdb_stdtarg = mi->targ;
  /* Route target error through the MI as well.  */
  gdb_stdtargerr = mi->targ;

  deprecated_show_load_progress = mi_load_progress;
}

void
mi_interp::suspend ()
{
  gdb_disable_readline ();
}

void
mi_interp::exec (const char *command)
{
  mi_execute_command_wrapper (command);
}

void
mi_cmd_interpreter_exec (const char *command, const char *const *argv,
			 int argc)
{
  struct interp *interp_to_use;
  int i;

  if (argc < 2)
    error (_("-interpreter-exec: "
	     "Usage: -interpreter-exec interp command"));

  interp_to_use = interp_lookup (current_ui, argv[0]);
  if (interp_to_use == NULL)
    error (_("-interpreter-exec: could not find interpreter \"%s\""),
	   argv[0]);

  /* Note that unlike the CLI version of this command, we don't
     actually set INTERP_TO_USE as the current interpreter, as we
     still want gdb_stdout, etc. to point at MI streams.  */

  /* Insert the MI out hooks, making sure to also call the
     interpreter's hooks if it has any.  */
  /* KRS: We shouldn't need this... Events should be installed and
     they should just ALWAYS fire something out down the MI
     channel.  */
  mi_insert_notify_hooks ();

  /* Now run the code.  */

  SCOPE_EXIT
    {
      mi_remove_notify_hooks ();
    };

  for (i = 1; i < argc; i++)
    interp_exec (interp_to_use, argv[i]);
}

/* This inserts a number of hooks that are meant to produce
   async-notify ("=") MI messages while running commands in another
   interpreter using mi_interpreter_exec.  The canonical use for this
   is to allow access to the gdb CLI interpreter from within the MI,
   while still producing MI style output when actions in the CLI
   command change GDB's state.  */

static void
mi_insert_notify_hooks (void)
{
  deprecated_query_hook = mi_interp_query_hook;
}

static void
mi_remove_notify_hooks (void)
{
  deprecated_query_hook = NULL;
}

static int
mi_interp_query_hook (const char *ctlstr, va_list ap)
{
  return 1;
}

static void
mi_execute_command_wrapper (const char *cmd)
{
  struct ui *ui = current_ui;

  mi_execute_command (cmd, ui->instream == ui->stdin_stream);
}

void
mi_interp::on_sync_execution_done ()
{
  /* If MI is sync, then output the MI prompt now, indicating we're
     ready for further input.  */
  if (!mi_async_p ())
    display_mi_prompt (this);
}

/* mi_execute_command_wrapper wrapper suitable for INPUT_HANDLER.  */

static void
mi_execute_command_input_handler (gdb::unique_xmalloc_ptr<char> &&cmd)
{
  struct mi_interp *mi = as_mi_interp (top_level_interpreter ());
  struct ui *ui = current_ui;

  ui->prompt_state = PROMPT_NEEDED;

  mi_execute_command_wrapper (cmd.get ());

  /* Print a prompt, indicating we're ready for further input, unless
     we just started a synchronous command.  In that case, we're about
     to go back to the event loop and will output the prompt in the
     'synchronous_command_done' observer when the target next
     stops.  */
  if (ui->prompt_state == PROMPT_NEEDED)
    display_mi_prompt (mi);
}

void
mi_interp::pre_command_loop ()
{
  struct mi_interp *mi = this;

  /* Turn off 8 bit strings in quoted output.  Any character with the
     high bit set is printed using C's octal format.  */
  sevenbit_strings = 1;

  /* Tell the world that we're alive.  */
  display_mi_prompt (mi);
}

void
mi_interp::on_new_thread (thread_info *t)
{
  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  gdb_printf (this->event_channel, "thread-created,id=\"%d\",group-id=\"i%d\"",
	      t->global_num, t->inf->num);
  gdb_flush (this->event_channel);
}

void
mi_interp::on_thread_exited (thread_info *t,
			     std::optional<ULONGEST> /* exit_code */,
			     int /* silent */)
{
  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();
  gdb_printf (this->event_channel, "thread-exited,id=\"%d\",group-id=\"i%d\"",
	      t->global_num, t->inf->num);
  gdb_flush (this->event_channel);
}

void
mi_interp::on_record_changed (inferior *inferior, int started,
			      const char *method, const char *format)
{
  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  if (started)
    {
      if (format != NULL)
	gdb_printf (this->event_channel,
		    "record-started,thread-group=\"i%d\","
		    "method=\"%s\",format=\"%s\"",
		    inferior->num, method, format);
      else
	gdb_printf (this->event_channel,
		    "record-started,thread-group=\"i%d\","
		    "method=\"%s\"",
		    inferior->num, method);
    }
  else
    gdb_printf (this->event_channel,
		"record-stopped,thread-group=\"i%d\"",
		inferior->num);

  gdb_flush (this->event_channel);
}

void
mi_interp::on_inferior_added (inferior *inf)
{
  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  gdb_printf (this->event_channel, "thread-group-added,id=\"i%d\"", inf->num);
  gdb_flush (this->event_channel);
}

void
mi_interp::on_inferior_appeared (inferior *inf)
{
  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  gdb_printf (this->event_channel, "thread-group-started,id=\"i%d\",pid=\"%d\"",
	      inf->num, inf->pid);
  gdb_flush (this->event_channel);
}

void
mi_interp::on_inferior_disappeared (inferior *inf)
{
  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  if (inf->has_exit_code)
    gdb_printf (this->event_channel,
		"thread-group-exited,id=\"i%d\",exit-code=\"%s\"",
		inf->num, int_string (inf->exit_code, 8, 0, 0, 1));
  else
    gdb_printf (this->event_channel,
		"thread-group-exited,id=\"i%d\"", inf->num);

  gdb_flush (this->event_channel);
}

void
mi_interp::on_inferior_removed (inferior *inf)
{
  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  gdb_printf (this->event_channel, "thread-group-removed,id=\"i%d\"", inf->num);
  gdb_flush (this->event_channel);
}

/* Observers for several run control events that print why the
   inferior has stopped to both the MI event channel and to the MI
   console.  If the MI interpreter is not active, print nothing.  */

void
mi_interp::on_signal_received (enum gdb_signal siggnal)
{
  print_signal_received_reason (this->mi_uiout, siggnal);
  print_signal_received_reason (this->cli_uiout, siggnal);
}

void
mi_interp::on_signal_exited (gdb_signal sig)
{
  print_signal_exited_reason (this->mi_uiout, sig);
  print_signal_exited_reason (this->cli_uiout, sig);
}

void
mi_interp::on_exited (int status)
{
  print_exited_reason (this->mi_uiout, status);
  print_exited_reason (this->cli_uiout, status);
}

void
mi_interp::on_no_history ()
{
  print_no_history_reason (this->mi_uiout);
  print_no_history_reason (this->cli_uiout);
}

void
mi_interp::on_normal_stop (struct bpstat *bs, int print_frame)
{
  /* Since this can be called when CLI command is executing,
     using cli interpreter, be sure to use MI uiout for output,
     not the current one.  */
  ui_out *mi_uiout = this->interp_ui_out ();

  if (print_frame)
    {
      thread_info *tp = inferior_thread ();

      if (tp->thread_fsm () != nullptr
	  && tp->thread_fsm ()->finished_p ())
	{
	  async_reply_reason reason
	    = tp->thread_fsm ()->async_reply_reason ();
	  mi_uiout->field_string ("reason", async_reason_lookup (reason));
	}

      interp *console_interp = interp_lookup (current_ui, INTERP_CONSOLE);

      /* We only want to print the displays once, and we want it to
	 look just how it would on the console, so we use this to
	 decide whether the MI stop should include them.  */
      bool console_print = should_print_stop_to_console (console_interp, tp);
      print_stop_event (mi_uiout, !console_print);

      if (console_print)
	print_stop_event (this->cli_uiout);

      mi_uiout->field_signed ("thread-id", tp->global_num);
      if (non_stop)
	{
	  ui_out_emit_list list_emitter (mi_uiout, "stopped-threads");

	  mi_uiout->field_signed (NULL, tp->global_num);
	}
      else
	mi_uiout->field_string ("stopped-threads", "all");

      int core = target_core_of_thread (tp->ptid);
      if (core != -1)
	mi_uiout->field_signed ("core", core);
    }

  gdb_puts ("*stopped", this->raw_stdout);
  mi_out_put (mi_uiout, this->raw_stdout);
  mi_out_rewind (mi_uiout);
  mi_print_timing_maybe (this->raw_stdout);
  gdb_puts ("\n", this->raw_stdout);
  gdb_flush (this->raw_stdout);
}

void
mi_interp::on_about_to_proceed ()
{
  /* Suppress output while calling an inferior function.  */

  if (inferior_ptid != null_ptid)
    {
      struct thread_info *tp = inferior_thread ();

      if (tp->control.in_infcall)
	return;
    }

  this->mi_proceeded = 1;
}

/* When the element is non-zero, no MI notifications will be emitted in
   response to the corresponding observers.  */

struct mi_suppress_notification mi_suppress_notification =
  {
    0,
    0,
    0,
    0,
  };

void
mi_interp::on_traceframe_changed (int tfnum, int tpnum)
{
  if (mi_suppress_notification.traceframe)
    return;

  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  if (tfnum >= 0)
    gdb_printf (this->event_channel, "traceframe-changed,"
		"num=\"%d\",tracepoint=\"%d\"",
		tfnum, tpnum);
  else
    gdb_printf (this->event_channel, "traceframe-changed,end");

  gdb_flush (this->event_channel);
}

void
mi_interp::on_tsv_created (const trace_state_variable *tsv)
{
  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  gdb_printf (this->event_channel, "tsv-created,"
	      "name=\"%s\",initial=\"%s\"",
	      tsv->name.c_str (), plongest (tsv->initial_value));

  gdb_flush (this->event_channel);
}

void
mi_interp::on_tsv_deleted (const trace_state_variable *tsv)
{
  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  if (tsv != nullptr)
    gdb_printf (this->event_channel, "tsv-deleted,name=\"%s\"",
		tsv->name.c_str ());
  else
    gdb_printf (this->event_channel, "tsv-deleted");

  gdb_flush (this->event_channel);
}

void
mi_interp::on_tsv_modified (const trace_state_variable *tsv)
{
  ui_out *mi_uiout = this->interp_ui_out ();

  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  gdb_printf (this->event_channel,
	      "tsv-modified");

  ui_out_redirect_pop redir (mi_uiout, this->event_channel);

  mi_uiout->field_string ("name", tsv->name);
  mi_uiout->field_string ("initial",
			  plongest (tsv->initial_value));
  if (tsv->value_known)
    mi_uiout->field_string ("current", plongest (tsv->value));

  gdb_flush (this->event_channel);
}

/* Print breakpoint BP on MI's event channel.  */

static void
mi_print_breakpoint_for_event (struct mi_interp *mi, breakpoint *bp)
{
  ui_out *mi_uiout = mi->interp_ui_out ();

  /* We want the output from print_breakpoint to go to
     mi->event_channel.  One approach would be to just call
     print_breakpoint, and then use mi_out_put to send the current
     content of mi_uiout into mi->event_channel.  However, that will
     break if anything is output to mi_uiout prior to calling the
     breakpoint_created notifications.  So, we use
     ui_out_redirect.  */
  ui_out_redirect_pop redir (mi_uiout, mi->event_channel);

  try
    {
      scoped_restore restore_uiout
	= make_scoped_restore (&current_uiout, mi_uiout);

      print_breakpoint (bp);
    }
  catch (const gdb_exception_error &ex)
    {
      exception_print (gdb_stderr, ex);
    }
}

void
mi_interp::on_breakpoint_created (breakpoint *b)
{
  if (mi_suppress_notification.breakpoint)
    return;

  if (b->number <= 0)
    return;

  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  gdb_printf (this->event_channel, "breakpoint-created");
  mi_print_breakpoint_for_event (this, b);

  gdb_flush (this->event_channel);
}

void
mi_interp::on_breakpoint_deleted (breakpoint *b)
{
  if (mi_suppress_notification.breakpoint)
    return;

  if (b->number <= 0)
    return;

  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  gdb_printf (this->event_channel, "breakpoint-deleted,id=\"%d\"", b->number);
  gdb_flush (this->event_channel);
}

void
mi_interp::on_breakpoint_modified (breakpoint *b)
{
  if (mi_suppress_notification.breakpoint)
    return;

  if (b->number <= 0)
    return;

  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();
  gdb_printf (this->event_channel, "breakpoint-modified");
  mi_print_breakpoint_for_event (this, b);

  gdb_flush (this->event_channel);
}

static void
mi_output_running (struct thread_info *thread)
{
  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == NULL)
	continue;

      gdb_printf (mi->raw_stdout,
		  "*running,thread-id=\"%d\"\n",
		  thread->global_num);
    }
}

/* Return true if there are multiple inferiors loaded.  This is used
   for backwards compatibility -- if there's only one inferior, output
   "all", otherwise, output each resumed thread individually.  */

static bool
multiple_inferiors_p ()
{
  int count = 0;
  for (inferior *inf ATTRIBUTE_UNUSED : all_non_exited_inferiors ())
    {
      count++;
      if (count > 1)
	return true;
    }

  return false;
}

static void
mi_on_resume_1 (struct mi_interp *mi,
		process_stratum_target *targ, ptid_t ptid)
{
  /* To cater for older frontends, emit ^running, but do it only once
     per each command.  We do it here, since at this point we know
     that the target was successfully resumed, and in non-async mode,
     we won't return back to MI interpreter code until the target
     is done running, so delaying the output of "^running" until then
     will make it impossible for frontend to know what's going on.

     In future (MI3), we'll be outputting "^done" here.  */
  if (!mi->running_result_record_printed && mi->mi_proceeded)
    {
      gdb_printf (mi->raw_stdout, "%s^running\n",
		  mi->current_token ? mi->current_token : "");
    }

  /* Backwards compatibility.  If doing a wildcard resume and there's
     only one inferior, output "all", otherwise, output each resumed
     thread individually.  */
  if ((ptid == minus_one_ptid || ptid.is_pid ())
      && !multiple_inferiors_p ())
    gdb_printf (mi->raw_stdout, "*running,thread-id=\"all\"\n");
  else
    for (thread_info *tp : all_non_exited_threads (targ, ptid))
      mi_output_running (tp);

  if (!mi->running_result_record_printed && mi->mi_proceeded)
    {
      mi->running_result_record_printed = 1;
      /* This is what gdb used to do historically -- printing prompt
	 even if it cannot actually accept any input.  This will be
	 surely removed for MI3, and may be removed even earlier.  */
      if (current_ui->prompt_state == PROMPT_BLOCKED)
	gdb_puts ("(gdb) \n", mi->raw_stdout);
    }
  gdb_flush (mi->raw_stdout);
}

void
mi_interp::on_target_resumed (ptid_t ptid)
{
  struct thread_info *tp = NULL;

  process_stratum_target *target = current_inferior ()->process_target ();
  if (ptid == minus_one_ptid || ptid.is_pid ())
    tp = inferior_thread ();
  else
    tp = target->find_thread (ptid);

  /* Suppress output while calling an inferior function.  */
  if (tp->control.in_infcall)
    return;

  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  mi_on_resume_1 (this, target, ptid);
}

/* See mi-interp.h.  */

void
mi_output_solib_attribs (ui_out *uiout, const shobj &solib)
{
  gdbarch *gdbarch = current_inferior ()->arch ();

  uiout->field_string ("id", solib.so_original_name);
  uiout->field_string ("target-name", solib.so_original_name);
  uiout->field_string ("host-name", solib.so_name);
  uiout->field_signed ("symbols-loaded", solib.symbols_loaded);
  if (!gdbarch_has_global_solist (current_inferior ()->arch ()))
      uiout->field_fmt ("thread-group", "i%d", current_inferior ()->num);

  ui_out_emit_list list_emitter (uiout, "ranges");
  ui_out_emit_tuple tuple_emitter (uiout, NULL);
  if (solib.addr_high != 0)
    {
      uiout->field_core_addr ("from", gdbarch, solib.addr_low);
      uiout->field_core_addr ("to", gdbarch, solib.addr_high);
    }
}

void
mi_interp::on_solib_loaded (const shobj &solib)
{
  ui_out *uiout = this->interp_ui_out ();

  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  gdb_printf (this->event_channel, "library-loaded");

  ui_out_redirect_pop redir (uiout, this->event_channel);

  mi_output_solib_attribs (uiout, solib);

  gdb_flush (this->event_channel);
}

void
mi_interp::on_solib_unloaded (const shobj &solib)
{
  ui_out *uiout = this->interp_ui_out ();

  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  gdb_printf (this->event_channel, "library-unloaded");

  ui_out_redirect_pop redir (uiout, this->event_channel);

  uiout->field_string ("id", solib.so_original_name);
  uiout->field_string ("target-name", solib.so_original_name);
  uiout->field_string ("host-name", solib.so_name);
  if (!gdbarch_has_global_solist (current_inferior ()->arch ()))
    uiout->field_fmt ("thread-group", "i%d", current_inferior ()->num);

  gdb_flush (this->event_channel);
}

void
mi_interp::on_param_changed (const char *param, const char *value)
{
  if (mi_suppress_notification.cmd_param_changed)
    return;

  ui_out *mi_uiout = this->interp_ui_out ();

  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  gdb_printf (this->event_channel, "cmd-param-changed");

  ui_out_redirect_pop redir (mi_uiout, this->event_channel);

  mi_uiout->field_string ("param", param);
  mi_uiout->field_string ("value", value);

  gdb_flush (this->event_channel);
}

void
mi_interp::on_memory_changed (inferior *inferior, CORE_ADDR memaddr,
			      ssize_t len, const bfd_byte *myaddr)
{
  if (mi_suppress_notification.memory)
    return;


  ui_out *mi_uiout = this->interp_ui_out ();

  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  gdb_printf (this->event_channel, "memory-changed");

  ui_out_redirect_pop redir (mi_uiout, this->event_channel);

  mi_uiout->field_fmt ("thread-group", "i%d", inferior->num);
  mi_uiout->field_core_addr ("addr", current_inferior ()->arch (), memaddr);
  mi_uiout->field_string ("len", hex_string (len));

  /* Append 'type=code' into notification if MEMADDR falls in the range of
     sections contain code.  */
  obj_section *sec = find_pc_section (memaddr);
  if (sec != nullptr && sec->objfile != nullptr)
    {
      flagword flags = bfd_section_flags (sec->the_bfd_section);

      if (flags & SEC_CODE)
	mi_uiout->field_string ("type", "code");
    }

  gdb_flush (this->event_channel);
}

void
mi_interp::on_user_selected_context_changed (user_selected_what selection)
{
  /* Don't send an event if we're responding to an MI command.  */
  if (mi_suppress_notification.user_selected_context)
    return;

  thread_info *tp = inferior_ptid != null_ptid ? inferior_thread () : nullptr;
  ui_out *mi_uiout = this->interp_ui_out ();
  ui_out_redirect_pop redirect_popper (mi_uiout, this->event_channel);

  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  if (selection & USER_SELECTED_INFERIOR)
    print_selected_inferior (this->cli_uiout);

  if (tp != NULL
      && (selection & (USER_SELECTED_THREAD | USER_SELECTED_FRAME)))
    {
      print_selected_thread_frame (this->cli_uiout, selection);

      gdb_printf (this->event_channel, "thread-selected,id=\"%d\"",
		  tp->global_num);

      if (tp->state != THREAD_RUNNING)
	{
	  if (has_stack_frames ())
	    print_stack_frame_to_uiout (mi_uiout, get_selected_frame (NULL),
					1, SRC_AND_LOC, 1);
	}
    }

  gdb_flush (this->event_channel);
}

ui_out *
mi_interp::interp_ui_out ()
{
  return this->mi_uiout;
}

/* Do MI-specific logging actions; save raw_stdout, and change all
   the consoles to use the supplied ui-file(s).  */

void
mi_interp::set_logging (ui_file_up logfile, bool logging_redirect,
			bool debug_redirect)
{
  struct mi_interp *mi = this;

  if (logfile != NULL)
    {
      mi->saved_raw_stdout = mi->raw_stdout;

      ui_file *logfile_p = logfile.get ();
      mi->logfile_holder = std::move (logfile);

      /* If something is not being redirected, then a tee containing both the
	 logfile and stdout.  */
      ui_file *tee = nullptr;
      if (!logging_redirect || !debug_redirect)
	{
	  tee = new tee_file (mi->raw_stdout, logfile_p);
	  mi->stdout_holder.reset (tee);
	}

      mi->raw_stdout = logging_redirect ? logfile_p : tee;
    }
  else
    {
      mi->logfile_holder.reset ();
      mi->stdout_holder.reset ();
      mi->raw_stdout = mi->saved_raw_stdout;
      mi->saved_raw_stdout = nullptr;
    }

  mi->out->set_raw (mi->raw_stdout);
  mi->err->set_raw (mi->raw_stdout);
  mi->log->set_raw (mi->raw_stdout);
  mi->targ->set_raw (mi->raw_stdout);
  mi->event_channel->set_raw (mi->raw_stdout);
}

/* Factory for MI interpreters.  */

static struct interp *
mi_interp_factory (const char *name)
{
  return new mi_interp (name);
}

void _initialize_mi_interp ();
void
_initialize_mi_interp ()
{
  /* The various interpreter levels.  */
  interp_factory_register (INTERP_MI2, mi_interp_factory);
  interp_factory_register (INTERP_MI3, mi_interp_factory);
  interp_factory_register (INTERP_MI4, mi_interp_factory);
  interp_factory_register (INTERP_MI, mi_interp_factory);
}
