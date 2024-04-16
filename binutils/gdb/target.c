/* Select target systems and architectures at runtime for GDB.

   Copyright (C) 1990-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Support.

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
#include "target.h"
#include "target-dcache.h"
#include "gdbcmd.h"
#include "symtab.h"
#include "inferior.h"
#include "infrun.h"
#include "observable.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "dcache.h"
#include <signal.h>
#include "regcache.h"
#include "gdbcore.h"
#include "target-descriptions.h"
#include "gdbthread.h"
#include "solib.h"
#include "exec.h"
#include "inline-frame.h"
#include "tracepoint.h"
#include "gdbsupport/fileio.h"
#include "gdbsupport/agent.h"
#include "auxv.h"
#include "target-debug.h"
#include "ui.h"
#include "event-top.h"
#include <algorithm>
#include "gdbsupport/byte-vector.h"
#include "gdbsupport/search.h"
#include "terminal.h"
#include <unordered_map>
#include "target-connection.h"
#include "valprint.h"
#include "cli/cli-decode.h"

static void generic_tls_error (void) ATTRIBUTE_NORETURN;

static void default_rcmd (struct target_ops *, const char *, struct ui_file *);

static int default_verify_memory (struct target_ops *self,
				  const gdb_byte *data,
				  CORE_ADDR memaddr, ULONGEST size);

static void tcomplain (void) ATTRIBUTE_NORETURN;

/* Mapping between target_info objects (which have address identity)
   and corresponding open/factory function/callback.  Each add_target
   call adds one entry to this map, and registers a "target
   TARGET_NAME" command that when invoked calls the factory registered
   here.  The target_info object is associated with the command via
   the command's context.  */
static std::unordered_map<const target_info *, target_open_ftype *>
  target_factories;

/* The singleton debug target.  */

static struct target_ops *the_debug_target;

/* Command list for target.  */

static struct cmd_list_element *targetlist = NULL;

/* See target.h.  */

bool trust_readonly = false;

/* Nonzero if we should show true memory content including
   memory breakpoint inserted by gdb.  */

static int show_memory_breakpoints = 0;

/* These globals control whether GDB attempts to perform these
   operations; they are useful for targets that need to prevent
   inadvertent disruption, such as in non-stop mode.  */

bool may_write_registers = true;

bool may_write_memory = true;

bool may_insert_breakpoints = true;

bool may_insert_tracepoints = true;

bool may_insert_fast_tracepoints = true;

bool may_stop = true;

/* Non-zero if we want to see trace of target level stuff.  */

static unsigned int targetdebug = 0;

static void
set_targetdebug  (const char *args, int from_tty, struct cmd_list_element *c)
{
  if (targetdebug)
    current_inferior ()->push_target (the_debug_target);
  else
    current_inferior ()->unpush_target (the_debug_target);
}

static void
show_targetdebug (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Target debugging is %s.\n"), value);
}

int
target_has_memory ()
{
  for (target_ops *t = current_inferior ()->top_target ();
       t != NULL;
       t = t->beneath ())
    if (t->has_memory ())
      return 1;

  return 0;
}

int
target_has_stack ()
{
  for (target_ops *t = current_inferior ()->top_target ();
       t != NULL;
       t = t->beneath ())
    if (t->has_stack ())
      return 1;

  return 0;
}

int
target_has_registers ()
{
  for (target_ops *t = current_inferior ()->top_target ();
       t != NULL;
       t = t->beneath ())
    if (t->has_registers ())
      return 1;

  return 0;
}

bool
target_has_execution (inferior *inf)
{
  if (inf == nullptr)
    inf = current_inferior ();

  for (target_ops *t = inf->top_target ();
       t != nullptr;
       t = inf->find_target_beneath (t))
    if (t->has_execution (inf))
      return true;

  return false;
}

const char *
target_shortname ()
{
  return current_inferior ()->top_target ()->shortname ();
}

/* See target.h.  */

bool
target_attach_no_wait ()
{
  return current_inferior ()->top_target ()->attach_no_wait ();
}

/* See target.h.  */

void
target_post_attach (int pid)
{
  return current_inferior ()->top_target ()->post_attach (pid);
}

/* See target.h.  */

void
target_prepare_to_store (regcache *regcache)
{
  return current_inferior ()->top_target ()->prepare_to_store (regcache);
}

/* See target.h.  */

bool
target_supports_enable_disable_tracepoint ()
{
  target_ops *target = current_inferior ()->top_target ();

  return target->supports_enable_disable_tracepoint ();
}

bool
target_supports_string_tracing ()
{
  return current_inferior ()->top_target ()->supports_string_tracing ();
}

/* See target.h.  */

bool
target_supports_evaluation_of_breakpoint_conditions ()
{
  target_ops *target = current_inferior ()->top_target ();

  return target->supports_evaluation_of_breakpoint_conditions ();
}

/* See target.h.  */

bool
target_supports_dumpcore ()
{
  return current_inferior ()->top_target ()->supports_dumpcore ();
}

/* See target.h.  */

void
target_dumpcore (const char *filename)
{
  return current_inferior ()->top_target ()->dumpcore (filename);
}

/* See target.h.  */

bool
target_can_run_breakpoint_commands ()
{
  return current_inferior ()->top_target ()->can_run_breakpoint_commands ();
}

/* See target.h.  */

void
target_files_info ()
{
  return current_inferior ()->top_target ()->files_info ();
}

/* See target.h.  */

int
target_insert_fork_catchpoint (int pid)
{
  return current_inferior ()->top_target ()->insert_fork_catchpoint (pid);
}

/* See target.h.  */

int
target_remove_fork_catchpoint (int pid)
{
  return current_inferior ()->top_target ()->remove_fork_catchpoint (pid);
}

/* See target.h.  */

int
target_insert_vfork_catchpoint (int pid)
{
  return current_inferior ()->top_target ()->insert_vfork_catchpoint (pid);
}

/* See target.h.  */

int
target_remove_vfork_catchpoint (int pid)
{
  return current_inferior ()->top_target ()->remove_vfork_catchpoint (pid);
}

/* See target.h.  */

int
target_insert_exec_catchpoint (int pid)
{
  return current_inferior ()->top_target ()->insert_exec_catchpoint (pid);
}

/* See target.h.  */

int
target_remove_exec_catchpoint (int pid)
{
  return current_inferior ()->top_target ()->remove_exec_catchpoint (pid);
}

/* See target.h.  */

int
target_set_syscall_catchpoint (int pid, bool needed, int any_count,
			       gdb::array_view<const int> syscall_counts)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->set_syscall_catchpoint (pid, needed, any_count,
					 syscall_counts);
}

/* See target.h.  */

void
target_rcmd (const char *command, struct ui_file *outbuf)
{
  return current_inferior ()->top_target ()->rcmd (command, outbuf);
}

/* See target.h.  */

bool
target_can_lock_scheduler ()
{
  target_ops *target = current_inferior ()->top_target ();

  return (target->get_thread_control_capabilities ()& tc_schedlock) != 0;
}

/* See target.h.  */

bool
target_can_async_p ()
{
  return target_can_async_p (current_inferior ()->top_target ());
}

/* See target.h.  */

bool
target_can_async_p (struct target_ops *target)
{
  if (!target_async_permitted)
    return false;
  return target->can_async_p ();
}

/* See target.h.  */

bool
target_is_async_p ()
{
  bool result = current_inferior ()->top_target ()->is_async_p ();
  gdb_assert (target_async_permitted || !result);
  return result;
}

exec_direction_kind
target_execution_direction ()
{
  return current_inferior ()->top_target ()->execution_direction ();
}

/* See target.h.  */

const char *
target_extra_thread_info (thread_info *tp)
{
  return current_inferior ()->top_target ()->extra_thread_info (tp);
}

/* See target.h.  */

const char *
target_pid_to_exec_file (int pid)
{
  return current_inferior ()->top_target ()->pid_to_exec_file (pid);
}

/* See target.h.  */

gdbarch *
target_thread_architecture (ptid_t ptid)
{
  return current_inferior ()->top_target ()->thread_architecture (ptid);
}

/* See target.h.  */

int
target_find_memory_regions (find_memory_region_ftype func, void *data)
{
  return current_inferior ()->top_target ()->find_memory_regions (func, data);
}

/* See target.h.  */

gdb::unique_xmalloc_ptr<char>
target_make_corefile_notes (bfd *bfd, int *size_p)
{
  return current_inferior ()->top_target ()->make_corefile_notes (bfd, size_p);
}

gdb_byte *
target_get_bookmark (const char *args, int from_tty)
{
  return current_inferior ()->top_target ()->get_bookmark (args, from_tty);
}

void
target_goto_bookmark (const gdb_byte *arg, int from_tty)
{
  return current_inferior ()->top_target ()->goto_bookmark (arg, from_tty);
}

/* See target.h.  */

bool
target_stopped_by_watchpoint ()
{
  return current_inferior ()->top_target ()->stopped_by_watchpoint ();
}

/* See target.h.  */

bool
target_stopped_by_sw_breakpoint ()
{
  return current_inferior ()->top_target ()->stopped_by_sw_breakpoint ();
}

bool
target_supports_stopped_by_sw_breakpoint ()
{
  target_ops *target = current_inferior ()->top_target ();

  return target->supports_stopped_by_sw_breakpoint ();
}

bool
target_stopped_by_hw_breakpoint ()
{
  return current_inferior ()->top_target ()->stopped_by_hw_breakpoint ();
}

bool
target_supports_stopped_by_hw_breakpoint ()
{
  target_ops *target = current_inferior ()->top_target ();

  return target->supports_stopped_by_hw_breakpoint ();
}

/* See target.h.  */

bool
target_have_steppable_watchpoint ()
{
  return current_inferior ()->top_target ()->have_steppable_watchpoint ();
}

/* See target.h.  */

int
target_can_use_hardware_watchpoint (bptype type, int cnt, int othertype)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->can_use_hw_breakpoint (type, cnt, othertype);
}

/* See target.h.  */

int
target_region_ok_for_hw_watchpoint (CORE_ADDR addr, int len)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->region_ok_for_hw_watchpoint (addr, len);
}


int
target_can_do_single_step ()
{
  return current_inferior ()->top_target ()->can_do_single_step ();
}

/* See target.h.  */

int
target_insert_watchpoint (CORE_ADDR addr, int len, target_hw_bp_type type,
			  expression *cond)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->insert_watchpoint (addr, len, type, cond);
}

/* See target.h.  */

int
target_remove_watchpoint (CORE_ADDR addr, int len, target_hw_bp_type type,
			  expression *cond)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->remove_watchpoint (addr, len, type, cond);
}

/* See target.h.  */

int
target_insert_hw_breakpoint (gdbarch *gdbarch, bp_target_info *bp_tgt)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->insert_hw_breakpoint (gdbarch, bp_tgt);
}

/* See target.h.  */

int
target_remove_hw_breakpoint (gdbarch *gdbarch, bp_target_info *bp_tgt)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->remove_hw_breakpoint (gdbarch, bp_tgt);
}

/* See target.h.  */

bool
target_can_accel_watchpoint_condition (CORE_ADDR addr, int len, int type,
				       expression *cond)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->can_accel_watchpoint_condition (addr, len, type, cond);
}

/* See target.h.  */

bool
target_can_execute_reverse ()
{
  return current_inferior ()->top_target ()->can_execute_reverse ();
}

ptid_t
target_get_ada_task_ptid (long lwp, ULONGEST tid)
{
  return current_inferior ()->top_target ()->get_ada_task_ptid (lwp, tid);
}

bool
target_filesystem_is_local ()
{
  return current_inferior ()->top_target ()->filesystem_is_local ();
}

void
target_trace_init ()
{
  return current_inferior ()->top_target ()->trace_init ();
}

void
target_download_tracepoint (bp_location *location)
{
  return current_inferior ()->top_target ()->download_tracepoint (location);
}

bool
target_can_download_tracepoint ()
{
  return current_inferior ()->top_target ()->can_download_tracepoint ();
}

void
target_download_trace_state_variable (const trace_state_variable &tsv)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->download_trace_state_variable (tsv);
}

void
target_enable_tracepoint (bp_location *loc)
{
  return current_inferior ()->top_target ()->enable_tracepoint (loc);
}

void
target_disable_tracepoint (bp_location *loc)
{
  return current_inferior ()->top_target ()->disable_tracepoint (loc);
}

void
target_trace_start ()
{
  return current_inferior ()->top_target ()->trace_start ();
}

void
target_trace_set_readonly_regions ()
{
  return current_inferior ()->top_target ()->trace_set_readonly_regions ();
}

int
target_get_trace_status (trace_status *ts)
{
  return current_inferior ()->top_target ()->get_trace_status (ts);
}

void
target_get_tracepoint_status (tracepoint *tp, uploaded_tp *utp)
{
  return current_inferior ()->top_target ()->get_tracepoint_status (tp, utp);
}

void
target_trace_stop ()
{
  return current_inferior ()->top_target ()->trace_stop ();
}

int
target_trace_find (trace_find_type type, int num,
		   CORE_ADDR addr1, CORE_ADDR addr2, int *tpp)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->trace_find (type, num, addr1, addr2, tpp);
}

bool
target_get_trace_state_variable_value (int tsv, LONGEST *val)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->get_trace_state_variable_value (tsv, val);
}

int
target_save_trace_data (const char *filename)
{
  return current_inferior ()->top_target ()->save_trace_data (filename);
}

int
target_upload_tracepoints (uploaded_tp **utpp)
{
  return current_inferior ()->top_target ()->upload_tracepoints (utpp);
}

int
target_upload_trace_state_variables (uploaded_tsv **utsvp)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->upload_trace_state_variables (utsvp);
}

LONGEST
target_get_raw_trace_data (gdb_byte *buf, ULONGEST offset, LONGEST len)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->get_raw_trace_data (buf, offset, len);
}

int
target_get_min_fast_tracepoint_insn_len ()
{
  target_ops *target = current_inferior ()->top_target ();

  return target->get_min_fast_tracepoint_insn_len ();
}

void
target_set_disconnected_tracing (int val)
{
  return current_inferior ()->top_target ()->set_disconnected_tracing (val);
}

void
target_set_circular_trace_buffer (int val)
{
  return current_inferior ()->top_target ()->set_circular_trace_buffer (val);
}

void
target_set_trace_buffer_size (LONGEST val)
{
  return current_inferior ()->top_target ()->set_trace_buffer_size (val);
}

bool
target_set_trace_notes (const char *user, const char *notes,
			const char *stopnotes)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->set_trace_notes (user, notes, stopnotes);
}

bool
target_get_tib_address (ptid_t ptid, CORE_ADDR *addr)
{
  return current_inferior ()->top_target ()->get_tib_address (ptid, addr);
}

void
target_set_permissions ()
{
  return current_inferior ()->top_target ()->set_permissions ();
}

bool
target_static_tracepoint_marker_at (CORE_ADDR addr,
				    static_tracepoint_marker *marker)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->static_tracepoint_marker_at (addr, marker);
}

std::vector<static_tracepoint_marker>
target_static_tracepoint_markers_by_strid (const char *marker_id)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->static_tracepoint_markers_by_strid (marker_id);
}

traceframe_info_up
target_traceframe_info ()
{
  return current_inferior ()->top_target ()->traceframe_info ();
}

bool
target_use_agent (bool use)
{
  return current_inferior ()->top_target ()->use_agent (use);
}

bool
target_can_use_agent ()
{
  return current_inferior ()->top_target ()->can_use_agent ();
}

bool
target_augmented_libraries_svr4_read ()
{
  return current_inferior ()->top_target ()->augmented_libraries_svr4_read ();
}

bool
target_supports_memory_tagging ()
{
  return current_inferior ()->top_target ()->supports_memory_tagging ();
}

bool
target_fetch_memtags (CORE_ADDR address, size_t len, gdb::byte_vector &tags,
		      int type)
{
  return current_inferior ()->top_target ()->fetch_memtags (address, len, tags, type);
}

bool
target_store_memtags (CORE_ADDR address, size_t len,
		      const gdb::byte_vector &tags, int type)
{
  return current_inferior ()->top_target ()->store_memtags (address, len, tags, type);
}

x86_xsave_layout
target_fetch_x86_xsave_layout ()
{
  return current_inferior ()->top_target ()->fetch_x86_xsave_layout ();
}

void
target_log_command (const char *p)
{
  return current_inferior ()->top_target ()->log_command (p);
}

/* This is used to implement the various target commands.  */

static void
open_target (const char *args, int from_tty, struct cmd_list_element *command)
{
  auto *ti = static_cast<target_info *> (command->context ());
  target_open_ftype *func = target_factories[ti];

  if (targetdebug)
    gdb_printf (gdb_stdlog, "-> %s->open (...)\n",
		ti->shortname);

  func (args, from_tty);

  if (targetdebug)
    gdb_printf (gdb_stdlog, "<- %s->open (%s, %d)\n",
		ti->shortname, args, from_tty);
}

/* See target.h.  */

void
add_target (const target_info &t, target_open_ftype *func,
	    completer_ftype *completer)
{
  struct cmd_list_element *c;

  auto &func_slot = target_factories[&t];
  if (func_slot != nullptr)
    internal_error (_("target already added (\"%s\")."), t.shortname);
  func_slot = func;

  if (targetlist == NULL)
    add_basic_prefix_cmd ("target", class_run, _("\
Connect to a target machine or process.\n\
The first argument is the type or protocol of the target machine.\n\
Remaining arguments are interpreted by the target protocol.  For more\n\
information on the arguments for a particular protocol, type\n\
`help target ' followed by the protocol name."),
			  &targetlist, 0, &cmdlist);
  c = add_cmd (t.shortname, no_class, t.doc, &targetlist);
  c->set_context ((void *) &t);
  c->func = open_target;
  if (completer != NULL)
    set_cmd_completer (c, completer);
}

/* See target.h.  */

void
add_deprecated_target_alias (const target_info &tinfo, const char *alias)
{
  struct cmd_list_element *c;

  /* If we use add_alias_cmd, here, we do not get the deprecated warning,
     see PR cli/15104.  */
  c = add_cmd (alias, no_class, tinfo.doc, &targetlist);
  c->func = open_target;
  c->set_context ((void *) &tinfo);
  gdb::unique_xmalloc_ptr<char> alt
    = xstrprintf ("target %s", tinfo.shortname);
  deprecate_cmd (c, alt.release ());
}

/* Stub functions */

void
target_kill (void)
{

  /* If the commit_resume_state of the to-be-killed-inferior's process stratum
     is true, and this inferior is the last live inferior with resumed threads
     of that target, then we want to leave commit_resume_state to false, as the
     target won't have any resumed threads anymore.  We achieve this with
     this scoped_disable_commit_resumed.  On construction, it will set the flag
     to false.  On destruction, it will only set it to true if there are resumed
     threads left.  */
  scoped_disable_commit_resumed disable ("killing");
  current_inferior ()->top_target ()->kill ();
}

void
target_load (const char *arg, int from_tty)
{
  target_dcache_invalidate (current_program_space->aspace);
  current_inferior ()->top_target ()->load (arg, from_tty);
}

/* Define it.  */

target_terminal_state target_terminal::m_terminal_state
  = target_terminal_state::is_ours;

/* See target/target.h.  */

void
target_terminal::init (void)
{
  current_inferior ()->top_target ()->terminal_init ();

  m_terminal_state = target_terminal_state::is_ours;
}

/* See target/target.h.  */

void
target_terminal::inferior (void)
{
  struct ui *ui = current_ui;

  /* A background resume (``run&'') should leave GDB in control of the
     terminal.  */
  if (ui->prompt_state != PROMPT_BLOCKED)
    return;

  /* Since we always run the inferior in the main console (unless "set
     inferior-tty" is in effect), when some UI other than the main one
     calls target_terminal::inferior, then we leave the main UI's
     terminal settings as is.  */
  if (ui != main_ui)
    return;

  /* If GDB is resuming the inferior in the foreground, install
     inferior's terminal modes.  */

  struct inferior *inf = current_inferior ();

  if (inf->terminal_state != target_terminal_state::is_inferior)
    {
      current_inferior ()->top_target ()->terminal_inferior ();
      inf->terminal_state = target_terminal_state::is_inferior;
    }

  m_terminal_state = target_terminal_state::is_inferior;

  /* If the user hit C-c before, pretend that it was hit right
     here.  */
  if (check_quit_flag ())
    target_pass_ctrlc ();
}

/* See target/target.h.  */

void
target_terminal::restore_inferior (void)
{
  struct ui *ui = current_ui;

  /* See target_terminal::inferior().  */
  if (ui->prompt_state != PROMPT_BLOCKED || ui != main_ui)
    return;

  /* Restore the terminal settings of inferiors that were in the
     foreground but are now ours_for_output due to a temporary
     target_target::ours_for_output() call.  */

  {
    scoped_restore_current_inferior restore_inferior;

    for (::inferior *inf : all_inferiors ())
      {
	if (inf->terminal_state == target_terminal_state::is_ours_for_output)
	  {
	    set_current_inferior (inf);
	    current_inferior ()->top_target ()->terminal_inferior ();
	    inf->terminal_state = target_terminal_state::is_inferior;
	  }
      }
  }

  m_terminal_state = target_terminal_state::is_inferior;

  /* If the user hit C-c before, pretend that it was hit right
     here.  */
  if (check_quit_flag ())
    target_pass_ctrlc ();
}

/* Switch terminal state to DESIRED_STATE, either is_ours, or
   is_ours_for_output.  */

static void
target_terminal_is_ours_kind (target_terminal_state desired_state)
{
  scoped_restore_current_inferior restore_inferior;

  /* Must do this in two passes.  First, have all inferiors save the
     current terminal settings.  Then, after all inferiors have add a
     chance to safely save the terminal settings, restore GDB's
     terminal settings.  */

  for (inferior *inf : all_inferiors ())
    {
      if (inf->terminal_state == target_terminal_state::is_inferior)
	{
	  set_current_inferior (inf);
	  current_inferior ()->top_target ()->terminal_save_inferior ();
	}
    }

  for (inferior *inf : all_inferiors ())
    {
      /* Note we don't check is_inferior here like above because we
	 need to handle 'is_ours_for_output -> is_ours' too.  Careful
	 to never transition from 'is_ours' to 'is_ours_for_output',
	 though.  */
      if (inf->terminal_state != target_terminal_state::is_ours
	  && inf->terminal_state != desired_state)
	{
	  set_current_inferior (inf);
	  if (desired_state == target_terminal_state::is_ours)
	    current_inferior ()->top_target ()->terminal_ours ();
	  else if (desired_state == target_terminal_state::is_ours_for_output)
	    current_inferior ()->top_target ()->terminal_ours_for_output ();
	  else
	    gdb_assert_not_reached ("unhandled desired state");
	  inf->terminal_state = desired_state;
	}
    }
}

/* See target/target.h.  */

void
target_terminal::ours ()
{
  struct ui *ui = current_ui;

  /* See target_terminal::inferior.  */
  if (ui != main_ui)
    return;

  if (m_terminal_state == target_terminal_state::is_ours)
    return;

  target_terminal_is_ours_kind (target_terminal_state::is_ours);
  m_terminal_state = target_terminal_state::is_ours;
}

/* See target/target.h.  */

void
target_terminal::ours_for_output ()
{
  struct ui *ui = current_ui;

  /* See target_terminal::inferior.  */
  if (ui != main_ui)
    return;

  if (!target_terminal::is_inferior ())
    return;

  target_terminal_is_ours_kind (target_terminal_state::is_ours_for_output);
  target_terminal::m_terminal_state = target_terminal_state::is_ours_for_output;
}

/* See target/target.h.  */

void
target_terminal::info (const char *arg, int from_tty)
{
  current_inferior ()->top_target ()->terminal_info (arg, from_tty);
}

/* See target.h.  */

bool
target_supports_terminal_ours (void)
{
  /* The current top target is the target at the top of the target
     stack of the current inferior.  While normally there's always an
     inferior, we must check for nullptr here because we can get here
     very early during startup, before the initial inferior is first
     created.  */
  inferior *inf = current_inferior ();

  if (inf == nullptr)
    return false;
  return inf->top_target ()->supports_terminal_ours ();
}

static void
tcomplain (void)
{
  error (_("You can't do that when your target is `%s'"),
	 current_inferior ()->top_target ()->shortname ());
}

void
noprocess (void)
{
  error (_("You can't do that without a process to debug."));
}

static void
default_terminal_info (struct target_ops *self, const char *args, int from_tty)
{
  gdb_printf (_("No saved terminal information.\n"));
}

/* A default implementation for the to_get_ada_task_ptid target method.

   This function builds the PTID by using both LWP and TID as part of
   the PTID lwp and tid elements.  The pid used is the pid of the
   inferior_ptid.  */

static ptid_t
default_get_ada_task_ptid (struct target_ops *self, long lwp, ULONGEST tid)
{
  return ptid_t (inferior_ptid.pid (), lwp, tid);
}

static enum exec_direction_kind
default_execution_direction (struct target_ops *self)
{
  if (!target_can_execute_reverse ())
    return EXEC_FORWARD;
  else if (!target_can_async_p ())
    return EXEC_FORWARD;
  else
    gdb_assert_not_reached ("\
to_execution_direction must be implemented for reverse async");
}

/* See target.h.  */

void
target_ops_ref_policy::decref (target_ops *t)
{
  t->decref ();
  if (t->refcount () == 0)
    {
      if (t->stratum () == process_stratum)
	connection_list_remove (as_process_stratum_target (t));

      for (inferior *inf : all_inferiors ())
	gdb_assert (!inf->target_is_pushed (t));

      fileio_handles_invalidate_target (t);

      t->close ();

      if (targetdebug)
	gdb_printf (gdb_stdlog, "closing target\n");
    }
}

/* See target.h.  */

void
target_stack::push (target_ops *t)
{
  /* We must create a new reference first.  It is possible that T is
     already pushed on this target stack, in which case we will first
     unpush it below, before re-pushing it.  If we don't increment the
     reference count now, then when we unpush it, we might end up deleting
     T, which is not good.  */
  auto ref = target_ops_ref::new_reference (t);

  strata stratum = t->stratum ();

  /* If there's already a target at this stratum, remove it.  */

  if (m_stack[stratum].get () != nullptr)
    unpush (m_stack[stratum].get ());

  /* Now add the new one.  */
  m_stack[stratum] = std::move (ref);

  if (m_top < stratum)
    m_top = stratum;

  if (stratum == process_stratum)
    connection_list_add (as_process_stratum_target (t));
}

/* See target.h.  */

bool
target_stack::unpush (target_ops *t)
{
  gdb_assert (t != NULL);

  strata stratum = t->stratum ();

  if (stratum == dummy_stratum)
    internal_error (_("Attempt to unpush the dummy target"));

  /* Look for the specified target.  Note that a target can only occur
     once in the target stack.  */

  if (m_stack[stratum] != t)
    {
      /* If T wasn't pushed, quit.  Only open targets should be
	 closed.  */
      return false;
    }

  if (m_top == stratum)
    m_top = this->find_beneath (t)->stratum ();

  /* Move the target reference off the target stack, this sets the pointer
     held in m_stack to nullptr, and places the reference in ref.  When
     ref goes out of scope its reference count will be decremented, which
     might cause the target to close.

     We have to do it this way, and not just set the value in m_stack to
     nullptr directly, because doing so would decrement the reference
     count first, which might close the target, and closing the target
     does a check that the target is not on any inferiors target_stack.  */
  auto ref = std::move (m_stack[stratum]);

  return true;
}

void
target_unpusher::operator() (struct target_ops *ops) const
{
  current_inferior ()->unpush_target (ops);
}

/* Default implementation of to_get_thread_local_address.  */

static void
generic_tls_error (void)
{
  throw_error (TLS_GENERIC_ERROR,
	       _("Cannot find thread-local variables on this target"));
}

/* Using the objfile specified in OBJFILE, find the address for the
   current thread's thread-local storage with offset OFFSET.  */
CORE_ADDR
target_translate_tls_address (struct objfile *objfile, CORE_ADDR offset)
{
  volatile CORE_ADDR addr = 0;
  struct target_ops *target = current_inferior ()->top_target ();
  gdbarch *gdbarch = current_inferior ()->arch ();

  /* If OBJFILE is a separate debug object file, look for the
     original object file.  */
  if (objfile->separate_debug_objfile_backlink != NULL)
    objfile = objfile->separate_debug_objfile_backlink;

  if (gdbarch_fetch_tls_load_module_address_p (gdbarch))
    {
      ptid_t ptid = inferior_ptid;

      try
	{
	  CORE_ADDR lm_addr;
	  
	  /* Fetch the load module address for this objfile.  */
	  lm_addr = gdbarch_fetch_tls_load_module_address (gdbarch,
							   objfile);

	  if (gdbarch_get_thread_local_address_p (gdbarch))
	    addr = gdbarch_get_thread_local_address (gdbarch, ptid, lm_addr,
						     offset);
	  else
	    addr = target->get_thread_local_address (ptid, lm_addr, offset);
	}
      /* If an error occurred, print TLS related messages here.  Otherwise,
	 throw the error to some higher catcher.  */
      catch (const gdb_exception &ex)
	{
	  int objfile_is_library = (objfile->flags & OBJF_SHARED);

	  switch (ex.error)
	    {
	    case TLS_NO_LIBRARY_SUPPORT_ERROR:
	      error (_("Cannot find thread-local variables "
		       "in this thread library."));
	      break;
	    case TLS_LOAD_MODULE_NOT_FOUND_ERROR:
	      if (objfile_is_library)
		error (_("Cannot find shared library `%s' in dynamic"
			 " linker's load module list"), objfile_name (objfile));
	      else
		error (_("Cannot find executable file `%s' in dynamic"
			 " linker's load module list"), objfile_name (objfile));
	      break;
	    case TLS_NOT_ALLOCATED_YET_ERROR:
	      if (objfile_is_library)
		error (_("The inferior has not yet allocated storage for"
			 " thread-local variables in\n"
			 "the shared library `%s'\n"
			 "for %s"),
		       objfile_name (objfile),
		       target_pid_to_str (ptid).c_str ());
	      else
		error (_("The inferior has not yet allocated storage for"
			 " thread-local variables in\n"
			 "the executable `%s'\n"
			 "for %s"),
		       objfile_name (objfile),
		       target_pid_to_str (ptid).c_str ());
	      break;
	    case TLS_GENERIC_ERROR:
	      if (objfile_is_library)
		error (_("Cannot find thread-local storage for %s, "
			 "shared library %s:\n%s"),
		       target_pid_to_str (ptid).c_str (),
		       objfile_name (objfile), ex.what ());
	      else
		error (_("Cannot find thread-local storage for %s, "
			 "executable file %s:\n%s"),
		       target_pid_to_str (ptid).c_str (),
		       objfile_name (objfile), ex.what ());
	      break;
	    default:
	      throw;
	      break;
	    }
	}
    }
  else
    error (_("Cannot find thread-local variables on this target"));

  return addr;
}

const char *
target_xfer_status_to_string (enum target_xfer_status status)
{
#define CASE(X) case X: return #X
  switch (status)
    {
      CASE(TARGET_XFER_E_IO);
      CASE(TARGET_XFER_UNAVAILABLE);
    default:
      return "<unknown>";
    }
#undef CASE
};


const std::vector<target_section> *
target_get_section_table (struct target_ops *target)
{
  return target->get_section_table ();
}

/* Find a section containing ADDR.  */

const struct target_section *
target_section_by_addr (struct target_ops *target, CORE_ADDR addr)
{
  const std::vector<target_section> *table = target_get_section_table (target);

  if (table == NULL)
    return NULL;

  for (const target_section &secp : *table)
    {
      if (addr >= secp.addr && addr < secp.endaddr)
	return &secp;
    }
  return NULL;
}

/* See target.h.  */

const std::vector<target_section> *
default_get_section_table ()
{
  return &current_program_space->target_sections ();
}

/* Helper for the memory xfer routines.  Checks the attributes of the
   memory region of MEMADDR against the read or write being attempted.
   If the access is permitted returns true, otherwise returns false.
   REGION_P is an optional output parameter.  If not-NULL, it is
   filled with a pointer to the memory region of MEMADDR.  REG_LEN
   returns LEN trimmed to the end of the region.  This is how much the
   caller can continue requesting, if the access is permitted.  A
   single xfer request must not straddle memory region boundaries.  */

static int
memory_xfer_check_region (gdb_byte *readbuf, const gdb_byte *writebuf,
			  ULONGEST memaddr, ULONGEST len, ULONGEST *reg_len,
			  struct mem_region **region_p)
{
  struct mem_region *region;

  region = lookup_mem_region (memaddr);

  if (region_p != NULL)
    *region_p = region;

  switch (region->attrib.mode)
    {
    case MEM_RO:
      if (writebuf != NULL)
	return 0;
      break;

    case MEM_WO:
      if (readbuf != NULL)
	return 0;
      break;

    case MEM_FLASH:
      /* We only support writing to flash during "load" for now.  */
      if (writebuf != NULL)
	error (_("Writing to flash memory forbidden in this context"));
      break;

    case MEM_NONE:
      return 0;
    }

  /* region->hi == 0 means there's no upper bound.  */
  if (memaddr + len < region->hi || region->hi == 0)
    *reg_len = len;
  else
    *reg_len = region->hi - memaddr;

  return 1;
}

/* Read memory from more than one valid target.  A core file, for
   instance, could have some of memory but delegate other bits to
   the target below it.  So, we must manually try all targets.  */

enum target_xfer_status
raw_memory_xfer_partial (struct target_ops *ops, gdb_byte *readbuf,
			 const gdb_byte *writebuf, ULONGEST memaddr, LONGEST len,
			 ULONGEST *xfered_len)
{
  enum target_xfer_status res;

  do
    {
      res = ops->xfer_partial (TARGET_OBJECT_MEMORY, NULL,
			       readbuf, writebuf, memaddr, len,
			       xfered_len);
      if (res == TARGET_XFER_OK)
	break;

      /* Stop if the target reports that the memory is not available.  */
      if (res == TARGET_XFER_UNAVAILABLE)
	break;

      /* Don't continue past targets which have all the memory.
	 At one time, this code was necessary to read data from
	 executables / shared libraries when data for the requested
	 addresses weren't available in the core file.  But now the
	 core target handles this case itself.  */
      if (ops->has_all_memory ())
	break;

      ops = ops->beneath ();
    }
  while (ops != NULL);

  /* The cache works at the raw memory level.  Make sure the cache
     gets updated with raw contents no matter what kind of memory
     object was originally being written.  Note we do write-through
     first, so that if it fails, we don't write to the cache contents
     that never made it to the target.  */
  if (writebuf != NULL
      && inferior_ptid != null_ptid
      && target_dcache_init_p (current_program_space->aspace)
      && (stack_cache_enabled_p () || code_cache_enabled_p ()))
    {
      DCACHE *dcache = target_dcache_get (current_program_space->aspace);

      /* Note that writing to an area of memory which wasn't present
	 in the cache doesn't cause it to be loaded in.  */
      dcache_update (dcache, res, memaddr, writebuf, *xfered_len);
    }

  return res;
}

/* Perform a partial memory transfer.
   For docs see target.h, to_xfer_partial.  */

static enum target_xfer_status
memory_xfer_partial_1 (struct target_ops *ops, enum target_object object,
		       gdb_byte *readbuf, const gdb_byte *writebuf, ULONGEST memaddr,
		       ULONGEST len, ULONGEST *xfered_len)
{
  enum target_xfer_status res;
  ULONGEST reg_len;
  struct mem_region *region;
  struct inferior *inf;

  /* For accesses to unmapped overlay sections, read directly from
     files.  Must do this first, as MEMADDR may need adjustment.  */
  if (readbuf != NULL && overlay_debugging)
    {
      struct obj_section *section = find_pc_overlay (memaddr);

      if (pc_in_unmapped_range (memaddr, section))
	{
	  const std::vector<target_section> *table = target_get_section_table (ops);
	  const char *section_name = section->the_bfd_section->name;

	  memaddr = overlay_mapped_address (memaddr, section);

	  auto match_cb = [=] (const struct target_section *s)
	    {
	      return (strcmp (section_name, s->the_bfd_section->name) == 0);
	    };

	  return section_table_xfer_memory_partial (readbuf, writebuf,
						    memaddr, len, xfered_len,
						    *table, match_cb);
	}
    }

  /* Try the executable files, if "trust-readonly-sections" is set.  */
  if (readbuf != NULL && trust_readonly)
    {
      const struct target_section *secp
	= target_section_by_addr (ops, memaddr);
      if (secp != NULL
	  && (bfd_section_flags (secp->the_bfd_section) & SEC_READONLY))
	{
	  const std::vector<target_section> *table = target_get_section_table (ops);
	  return section_table_xfer_memory_partial (readbuf, writebuf,
						    memaddr, len, xfered_len,
						    *table);
	}
    }

  /* Try GDB's internal data cache.  */

  if (!memory_xfer_check_region (readbuf, writebuf, memaddr, len, &reg_len,
				 &region))
    return TARGET_XFER_E_IO;

  if (inferior_ptid != null_ptid)
    inf = current_inferior ();
  else
    inf = NULL;

  if (inf != NULL
      && readbuf != NULL
      /* The dcache reads whole cache lines; that doesn't play well
	 with reading from a trace buffer, because reading outside of
	 the collected memory range fails.  */
      && get_traceframe_number () == -1
      && (region->attrib.cache
	  || (stack_cache_enabled_p () && object == TARGET_OBJECT_STACK_MEMORY)
	  || (code_cache_enabled_p () && object == TARGET_OBJECT_CODE_MEMORY)))
    {
      DCACHE *dcache
	= target_dcache_get_or_init (current_program_space->aspace);

      return dcache_read_memory_partial (ops, dcache, memaddr, readbuf,
					 reg_len, xfered_len);
    }

  /* If none of those methods found the memory we wanted, fall back
     to a target partial transfer.  Normally a single call to
     to_xfer_partial is enough; if it doesn't recognize an object
     it will call the to_xfer_partial of the next target down.
     But for memory this won't do.  Memory is the only target
     object which can be read from more than one valid target.
     A core file, for instance, could have some of memory but
     delegate other bits to the target below it.  So, we must
     manually try all targets.  */

  res = raw_memory_xfer_partial (ops, readbuf, writebuf, memaddr, reg_len,
				 xfered_len);

  /* If we still haven't got anything, return the last error.  We
     give up.  */
  return res;
}

/* Perform a partial memory transfer.  For docs see target.h,
   to_xfer_partial.  */

static enum target_xfer_status
memory_xfer_partial (struct target_ops *ops, enum target_object object,
		     gdb_byte *readbuf, const gdb_byte *writebuf,
		     ULONGEST memaddr, ULONGEST len, ULONGEST *xfered_len)
{
  enum target_xfer_status res;

  /* Zero length requests are ok and require no work.  */
  if (len == 0)
    return TARGET_XFER_EOF;

  memaddr = gdbarch_remove_non_address_bits (current_inferior ()->arch (),
					     memaddr);

  /* Fill in READBUF with breakpoint shadows, or WRITEBUF with
     breakpoint insns, thus hiding out from higher layers whether
     there are software breakpoints inserted in the code stream.  */
  if (readbuf != NULL)
    {
      res = memory_xfer_partial_1 (ops, object, readbuf, NULL, memaddr, len,
				   xfered_len);

      if (res == TARGET_XFER_OK && !show_memory_breakpoints)
	breakpoint_xfer_memory (readbuf, NULL, NULL, memaddr, *xfered_len);
    }
  else
    {
      /* A large write request is likely to be partially satisfied
	 by memory_xfer_partial_1.  We will continually malloc
	 and free a copy of the entire write request for breakpoint
	 shadow handling even though we only end up writing a small
	 subset of it.  Cap writes to a limit specified by the target
	 to mitigate this.  */
      len = std::min (ops->get_memory_xfer_limit (), len);

      gdb::byte_vector buf (writebuf, writebuf + len);
      breakpoint_xfer_memory (NULL, buf.data (), writebuf, memaddr, len);
      res = memory_xfer_partial_1 (ops, object, NULL, buf.data (), memaddr, len,
				   xfered_len);
    }

  return res;
}

scoped_restore_tmpl<int>
make_scoped_restore_show_memory_breakpoints (int show)
{
  return make_scoped_restore (&show_memory_breakpoints, show);
}

/* For docs see target.h, to_xfer_partial.  */

enum target_xfer_status
target_xfer_partial (struct target_ops *ops,
		     enum target_object object, const char *annex,
		     gdb_byte *readbuf, const gdb_byte *writebuf,
		     ULONGEST offset, ULONGEST len,
		     ULONGEST *xfered_len)
{
  enum target_xfer_status retval;

  /* Transfer is done when LEN is zero.  */
  if (len == 0)
    return TARGET_XFER_EOF;

  if (writebuf && !may_write_memory)
    error (_("Writing to memory is not allowed (addr %s, len %s)"),
	   core_addr_to_string_nz (offset), plongest (len));

  *xfered_len = 0;

  /* If this is a memory transfer, let the memory-specific code
     have a look at it instead.  Memory transfers are more
     complicated.  */
  if (object == TARGET_OBJECT_MEMORY || object == TARGET_OBJECT_STACK_MEMORY
      || object == TARGET_OBJECT_CODE_MEMORY)
    retval = memory_xfer_partial (ops, object, readbuf,
				  writebuf, offset, len, xfered_len);
  else if (object == TARGET_OBJECT_RAW_MEMORY)
    {
      /* Skip/avoid accessing the target if the memory region
	 attributes block the access.  Check this here instead of in
	 raw_memory_xfer_partial as otherwise we'd end up checking
	 this twice in the case of the memory_xfer_partial path is
	 taken; once before checking the dcache, and another in the
	 tail call to raw_memory_xfer_partial.  */
      if (!memory_xfer_check_region (readbuf, writebuf, offset, len, &len,
				     NULL))
	return TARGET_XFER_E_IO;

      /* Request the normal memory object from other layers.  */
      retval = raw_memory_xfer_partial (ops, readbuf, writebuf, offset, len,
					xfered_len);
    }
  else
    retval = ops->xfer_partial (object, annex, readbuf,
				writebuf, offset, len, xfered_len);

  if (targetdebug)
    {
      const unsigned char *myaddr = NULL;

      gdb_printf (gdb_stdlog,
		  "%s:target_xfer_partial "
		  "(%d, %s, %s, %s, %s, %s) = %d, %s",
		  ops->shortname (),
		  (int) object,
		  (annex ? annex : "(null)"),
		  host_address_to_string (readbuf),
		  host_address_to_string (writebuf),
		  core_addr_to_string_nz (offset),
		  pulongest (len), retval,
		  pulongest (*xfered_len));

      if (readbuf)
	myaddr = readbuf;
      if (writebuf)
	myaddr = writebuf;
      if (retval == TARGET_XFER_OK && myaddr != NULL)
	{
	  int i;

	  gdb_puts (", bytes =", gdb_stdlog);
	  for (i = 0; i < *xfered_len; i++)
	    {
	      if ((((intptr_t) &(myaddr[i])) & 0xf) == 0)
		{
		  if (targetdebug < 2 && i > 0)
		    {
		      gdb_printf (gdb_stdlog, " ...");
		      break;
		    }
		  gdb_printf (gdb_stdlog, "\n");
		}

	      gdb_printf (gdb_stdlog, " %02x", myaddr[i] & 0xff);
	    }
	}

      gdb_putc ('\n', gdb_stdlog);
    }

  /* Check implementations of to_xfer_partial update *XFERED_LEN
     properly.  Do assertion after printing debug messages, so that we
     can find more clues on assertion failure from debugging messages.  */
  if (retval == TARGET_XFER_OK || retval == TARGET_XFER_UNAVAILABLE)
    gdb_assert (*xfered_len > 0);

  return retval;
}

/* Read LEN bytes of target memory at address MEMADDR, placing the
   results in GDB's memory at MYADDR.  Returns either 0 for success or
   -1 if any error occurs.

   If an error occurs, no guarantee is made about the contents of the data at
   MYADDR.  In particular, the caller should not depend upon partial reads
   filling the buffer with good data.  There is no way for the caller to know
   how much good data might have been transfered anyway.  Callers that can
   deal with partial reads should call target_read (which will retry until
   it makes no progress, and then return how much was transferred).  */

int
target_read_memory (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len)
{
  if (target_read (current_inferior ()->top_target (),
		   TARGET_OBJECT_MEMORY, NULL,
		   myaddr, memaddr, len) == len)
    return 0;
  else
    return -1;
}

/* See target/target.h.  */

int
target_read_uint32 (CORE_ADDR memaddr, uint32_t *result)
{
  gdb_byte buf[4];
  int r;

  r = target_read_memory (memaddr, buf, sizeof buf);
  if (r != 0)
    return r;
  *result = extract_unsigned_integer
	      (buf, sizeof buf,
	       gdbarch_byte_order (current_inferior ()->arch ()));
  return 0;
}

/* Like target_read_memory, but specify explicitly that this is a read
   from the target's raw memory.  That is, this read bypasses the
   dcache, breakpoint shadowing, etc.  */

int
target_read_raw_memory (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len)
{
  if (target_read (current_inferior ()->top_target (),
		   TARGET_OBJECT_RAW_MEMORY, NULL,
		   myaddr, memaddr, len) == len)
    return 0;
  else
    return -1;
}

/* Like target_read_memory, but specify explicitly that this is a read from
   the target's stack.  This may trigger different cache behavior.  */

int
target_read_stack (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len)
{
  if (target_read (current_inferior ()->top_target (),
		   TARGET_OBJECT_STACK_MEMORY, NULL,
		   myaddr, memaddr, len) == len)
    return 0;
  else
    return -1;
}

/* Like target_read_memory, but specify explicitly that this is a read from
   the target's code.  This may trigger different cache behavior.  */

int
target_read_code (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len)
{
  if (target_read (current_inferior ()->top_target (),
		   TARGET_OBJECT_CODE_MEMORY, NULL,
		   myaddr, memaddr, len) == len)
    return 0;
  else
    return -1;
}

/* Write LEN bytes from MYADDR to target memory at address MEMADDR.
   Returns either 0 for success or -1 if any error occurs.  If an
   error occurs, no guarantee is made about how much data got written.
   Callers that can deal with partial writes should call
   target_write.  */

int
target_write_memory (CORE_ADDR memaddr, const gdb_byte *myaddr, ssize_t len)
{
  if (target_write (current_inferior ()->top_target (),
		    TARGET_OBJECT_MEMORY, NULL,
		    myaddr, memaddr, len) == len)
    return 0;
  else
    return -1;
}

/* Write LEN bytes from MYADDR to target raw memory at address
   MEMADDR.  Returns either 0 for success or -1 if any error occurs.
   If an error occurs, no guarantee is made about how much data got
   written.  Callers that can deal with partial writes should call
   target_write.  */

int
target_write_raw_memory (CORE_ADDR memaddr, const gdb_byte *myaddr, ssize_t len)
{
  if (target_write (current_inferior ()->top_target (),
		    TARGET_OBJECT_RAW_MEMORY, NULL,
		    myaddr, memaddr, len) == len)
    return 0;
  else
    return -1;
}

/* Fetch the target's memory map.  */

std::vector<mem_region>
target_memory_map (void)
{
  target_ops *target = current_inferior ()->top_target ();
  std::vector<mem_region> result = target->memory_map ();
  if (result.empty ())
    return result;

  std::sort (result.begin (), result.end ());

  /* Check that regions do not overlap.  Simultaneously assign
     a numbering for the "mem" commands to use to refer to
     each region.  */
  mem_region *last_one = NULL;
  for (size_t ix = 0; ix < result.size (); ix++)
    {
      mem_region *this_one = &result[ix];
      this_one->number = ix;

      if (last_one != NULL && last_one->hi > this_one->lo)
	{
	  warning (_("Overlapping regions in memory map: ignoring"));
	  return std::vector<mem_region> ();
	}

      last_one = this_one;
    }

  return result;
}

void
target_flash_erase (ULONGEST address, LONGEST length)
{
  current_inferior ()->top_target ()->flash_erase (address, length);
}

void
target_flash_done (void)
{
  current_inferior ()->top_target ()->flash_done ();
}

static void
show_trust_readonly (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Mode for reading from readonly sections is %s.\n"),
	      value);
}

/* Target vector read/write partial wrapper functions.  */

static enum target_xfer_status
target_read_partial (struct target_ops *ops,
		     enum target_object object,
		     const char *annex, gdb_byte *buf,
		     ULONGEST offset, ULONGEST len,
		     ULONGEST *xfered_len)
{
  return target_xfer_partial (ops, object, annex, buf, NULL, offset, len,
			      xfered_len);
}

static enum target_xfer_status
target_write_partial (struct target_ops *ops,
		      enum target_object object,
		      const char *annex, const gdb_byte *buf,
		      ULONGEST offset, LONGEST len, ULONGEST *xfered_len)
{
  return target_xfer_partial (ops, object, annex, NULL, buf, offset, len,
			      xfered_len);
}

/* Wrappers to perform the full transfer.  */

/* For docs on target_read see target.h.  */

LONGEST
target_read (struct target_ops *ops,
	     enum target_object object,
	     const char *annex, gdb_byte *buf,
	     ULONGEST offset, LONGEST len)
{
  LONGEST xfered_total = 0;
  int unit_size = 1;

  /* If we are reading from a memory object, find the length of an addressable
     unit for that architecture.  */
  if (object == TARGET_OBJECT_MEMORY
      || object == TARGET_OBJECT_STACK_MEMORY
      || object == TARGET_OBJECT_CODE_MEMORY
      || object == TARGET_OBJECT_RAW_MEMORY)
    unit_size = gdbarch_addressable_memory_unit_size
		  (current_inferior ()->arch ());

  while (xfered_total < len)
    {
      ULONGEST xfered_partial;
      enum target_xfer_status status;

      status = target_read_partial (ops, object, annex,
				    buf + xfered_total * unit_size,
				    offset + xfered_total, len - xfered_total,
				    &xfered_partial);

      /* Call an observer, notifying them of the xfer progress?  */
      if (status == TARGET_XFER_EOF)
	return xfered_total;
      else if (status == TARGET_XFER_OK)
	{
	  xfered_total += xfered_partial;
	  QUIT;
	}
      else
	return TARGET_XFER_E_IO;

    }
  return len;
}

/* Assuming that the entire [begin, end) range of memory cannot be
   read, try to read whatever subrange is possible to read.

   The function returns, in RESULT, either zero or one memory block.
   If there's a readable subrange at the beginning, it is completely
   read and returned.  Any further readable subrange will not be read.
   Otherwise, if there's a readable subrange at the end, it will be
   completely read and returned.  Any readable subranges before it
   (obviously, not starting at the beginning), will be ignored.  In
   other cases -- either no readable subrange, or readable subrange(s)
   that is neither at the beginning, or end, nothing is returned.

   The purpose of this function is to handle a read across a boundary
   of accessible memory in a case when memory map is not available.
   The above restrictions are fine for this case, but will give
   incorrect results if the memory is 'patchy'.  However, supporting
   'patchy' memory would require trying to read every single byte,
   and it seems unacceptable solution.  Explicit memory map is
   recommended for this case -- and target_read_memory_robust will
   take care of reading multiple ranges then.  */

static void
read_whatever_is_readable (struct target_ops *ops,
			   const ULONGEST begin, const ULONGEST end,
			   int unit_size,
			   std::vector<memory_read_result> *result)
{
  ULONGEST current_begin = begin;
  ULONGEST current_end = end;
  int forward;
  ULONGEST xfered_len;

  /* If we previously failed to read 1 byte, nothing can be done here.  */
  if (end - begin <= 1)
    return;

  gdb::unique_xmalloc_ptr<gdb_byte> buf ((gdb_byte *) xmalloc (end - begin));

  /* Check that either first or the last byte is readable, and give up
     if not.  This heuristic is meant to permit reading accessible memory
     at the boundary of accessible region.  */
  if (target_read_partial (ops, TARGET_OBJECT_MEMORY, NULL,
			   buf.get (), begin, 1, &xfered_len) == TARGET_XFER_OK)
    {
      forward = 1;
      ++current_begin;
    }
  else if (target_read_partial (ops, TARGET_OBJECT_MEMORY, NULL,
				buf.get () + (end - begin) - 1, end - 1, 1,
				&xfered_len) == TARGET_XFER_OK)
    {
      forward = 0;
      --current_end;
    }
  else
    return;

  /* Loop invariant is that the [current_begin, current_end) was previously
     found to be not readable as a whole.

     Note loop condition -- if the range has 1 byte, we can't divide the range
     so there's no point trying further.  */
  while (current_end - current_begin > 1)
    {
      ULONGEST first_half_begin, first_half_end;
      ULONGEST second_half_begin, second_half_end;
      LONGEST xfer;
      ULONGEST middle = current_begin + (current_end - current_begin) / 2;

      if (forward)
	{
	  first_half_begin = current_begin;
	  first_half_end = middle;
	  second_half_begin = middle;
	  second_half_end = current_end;
	}
      else
	{
	  first_half_begin = middle;
	  first_half_end = current_end;
	  second_half_begin = current_begin;
	  second_half_end = middle;
	}

      xfer = target_read (ops, TARGET_OBJECT_MEMORY, NULL,
			  buf.get () + (first_half_begin - begin) * unit_size,
			  first_half_begin,
			  first_half_end - first_half_begin);

      if (xfer == first_half_end - first_half_begin)
	{
	  /* This half reads up fine.  So, the error must be in the
	     other half.  */
	  current_begin = second_half_begin;
	  current_end = second_half_end;
	}
      else
	{
	  /* This half is not readable.  Because we've tried one byte, we
	     know some part of this half if actually readable.  Go to the next
	     iteration to divide again and try to read.

	     We don't handle the other half, because this function only tries
	     to read a single readable subrange.  */
	  current_begin = first_half_begin;
	  current_end = first_half_end;
	}
    }

  if (forward)
    {
      /* The [begin, current_begin) range has been read.  */
      result->emplace_back (begin, current_end, std::move (buf));
    }
  else
    {
      /* The [current_end, end) range has been read.  */
      LONGEST region_len = end - current_end;

      gdb::unique_xmalloc_ptr<gdb_byte> data
	((gdb_byte *) xmalloc (region_len * unit_size));
      memcpy (data.get (), buf.get () + (current_end - begin) * unit_size,
	      region_len * unit_size);
      result->emplace_back (current_end, end, std::move (data));
    }
}

std::vector<memory_read_result>
read_memory_robust (struct target_ops *ops,
		    const ULONGEST offset, const LONGEST len)
{
  std::vector<memory_read_result> result;
  int unit_size
    = gdbarch_addressable_memory_unit_size (current_inferior ()->arch ());

  LONGEST xfered_total = 0;
  while (xfered_total < len)
    {
      struct mem_region *region = lookup_mem_region (offset + xfered_total);
      LONGEST region_len;

      /* If there is no explicit region, a fake one should be created.  */
      gdb_assert (region);

      if (region->hi == 0)
	region_len = len - xfered_total;
      else
	region_len = region->hi - offset;

      if (region->attrib.mode == MEM_NONE || region->attrib.mode == MEM_WO)
	{
	  /* Cannot read this region.  Note that we can end up here only
	     if the region is explicitly marked inaccessible, or
	     'inaccessible-by-default' is in effect.  */
	  xfered_total += region_len;
	}
      else
	{
	  LONGEST to_read = std::min (len - xfered_total, region_len);
	  gdb::unique_xmalloc_ptr<gdb_byte> buffer
	    ((gdb_byte *) xmalloc (to_read * unit_size));

	  LONGEST xfered_partial =
	      target_read (ops, TARGET_OBJECT_MEMORY, NULL, buffer.get (),
			   offset + xfered_total, to_read);
	  /* Call an observer, notifying them of the xfer progress?  */
	  if (xfered_partial <= 0)
	    {
	      /* Got an error reading full chunk.  See if maybe we can read
		 some subrange.  */
	      read_whatever_is_readable (ops, offset + xfered_total,
					 offset + xfered_total + to_read,
					 unit_size, &result);
	      xfered_total += to_read;
	    }
	  else
	    {
	      result.emplace_back (offset + xfered_total,
				   offset + xfered_total + xfered_partial,
				   std::move (buffer));
	      xfered_total += xfered_partial;
	    }
	  QUIT;
	}
    }

  return result;
}


/* An alternative to target_write with progress callbacks.  */

LONGEST
target_write_with_progress (struct target_ops *ops,
			    enum target_object object,
			    const char *annex, const gdb_byte *buf,
			    ULONGEST offset, LONGEST len,
			    void (*progress) (ULONGEST, void *), void *baton)
{
  LONGEST xfered_total = 0;
  int unit_size = 1;

  /* If we are writing to a memory object, find the length of an addressable
     unit for that architecture.  */
  if (object == TARGET_OBJECT_MEMORY
      || object == TARGET_OBJECT_STACK_MEMORY
      || object == TARGET_OBJECT_CODE_MEMORY
      || object == TARGET_OBJECT_RAW_MEMORY)
    unit_size = gdbarch_addressable_memory_unit_size
		  (current_inferior ()->arch ());

  /* Give the progress callback a chance to set up.  */
  if (progress)
    (*progress) (0, baton);

  while (xfered_total < len)
    {
      ULONGEST xfered_partial;
      enum target_xfer_status status;

      status = target_write_partial (ops, object, annex,
				     buf + xfered_total * unit_size,
				     offset + xfered_total, len - xfered_total,
				     &xfered_partial);

      if (status != TARGET_XFER_OK)
	return status == TARGET_XFER_EOF ? xfered_total : TARGET_XFER_E_IO;

      if (progress)
	(*progress) (xfered_partial, baton);

      xfered_total += xfered_partial;
      QUIT;
    }
  return len;
}

/* For docs on target_write see target.h.  */

LONGEST
target_write (struct target_ops *ops,
	      enum target_object object,
	      const char *annex, const gdb_byte *buf,
	      ULONGEST offset, LONGEST len)
{
  return target_write_with_progress (ops, object, annex, buf, offset, len,
				     NULL, NULL);
}

/* Help for target_read_alloc and target_read_stralloc.  See their comments
   for details.  */

template <typename T>
std::optional<gdb::def_vector<T>>
target_read_alloc_1 (struct target_ops *ops, enum target_object object,
		     const char *annex)
{
  gdb::def_vector<T> buf;
  size_t buf_pos = 0;
  const int chunk = 4096;

  /* This function does not have a length parameter; it reads the
     entire OBJECT).  Also, it doesn't support objects fetched partly
     from one target and partly from another (in a different stratum,
     e.g. a core file and an executable).  Both reasons make it
     unsuitable for reading memory.  */
  gdb_assert (object != TARGET_OBJECT_MEMORY);

  /* Start by reading up to 4K at a time.  The target will throttle
     this number down if necessary.  */
  while (1)
    {
      ULONGEST xfered_len;
      enum target_xfer_status status;

      buf.resize (buf_pos + chunk);

      status = target_read_partial (ops, object, annex,
				    (gdb_byte *) &buf[buf_pos],
				    buf_pos, chunk,
				    &xfered_len);

      if (status == TARGET_XFER_EOF)
	{
	  /* Read all there was.  */
	  buf.resize (buf_pos);
	  return buf;
	}
      else if (status != TARGET_XFER_OK)
	{
	  /* An error occurred.  */
	  return {};
	}

      buf_pos += xfered_len;

      QUIT;
    }
}

/* See target.h  */

std::optional<gdb::byte_vector>
target_read_alloc (struct target_ops *ops, enum target_object object,
		   const char *annex)
{
  return target_read_alloc_1<gdb_byte> (ops, object, annex);
}

/* See target.h.  */

std::optional<gdb::char_vector>
target_read_stralloc (struct target_ops *ops, enum target_object object,
		      const char *annex)
{
  std::optional<gdb::char_vector> buf
    = target_read_alloc_1<char> (ops, object, annex);

  if (!buf)
    return {};

  if (buf->empty () || buf->back () != '\0')
    buf->push_back ('\0');

  /* Check for embedded NUL bytes; but allow trailing NULs.  */
  for (auto it = std::find (buf->begin (), buf->end (), '\0');
       it != buf->end (); it++)
    if (*it != '\0')
      {
	warning (_("target object %d, annex %s, "
		   "contained unexpected null characters"),
		 (int) object, annex ? annex : "(none)");
	break;
      }

  return buf;
}

/* Memory transfer methods.  */

void
get_target_memory (struct target_ops *ops, CORE_ADDR addr, gdb_byte *buf,
		   LONGEST len)
{
  /* This method is used to read from an alternate, non-current
     target.  This read must bypass the overlay support (as symbols
     don't match this target), and GDB's internal cache (wrong cache
     for this target).  */
  if (target_read (ops, TARGET_OBJECT_RAW_MEMORY, NULL, buf, addr, len)
      != len)
    memory_error (TARGET_XFER_E_IO, addr);
}

ULONGEST
get_target_memory_unsigned (struct target_ops *ops, CORE_ADDR addr,
			    int len, enum bfd_endian byte_order)
{
  gdb_byte buf[sizeof (ULONGEST)];

  gdb_assert (len <= sizeof (buf));
  get_target_memory (ops, addr, buf, len);
  return extract_unsigned_integer (buf, len, byte_order);
}

/* See target.h.  */

int
target_insert_breakpoint (struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
{
  if (!may_insert_breakpoints)
    {
      warning (_("May not insert breakpoints"));
      return 1;
    }

  target_ops *target = current_inferior ()->top_target ();

  return target->insert_breakpoint (gdbarch, bp_tgt);
}

/* See target.h.  */

int
target_remove_breakpoint (struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt,
			  enum remove_bp_reason reason)
{
  /* This is kind of a weird case to handle, but the permission might
     have been changed after breakpoints were inserted - in which case
     we should just take the user literally and assume that any
     breakpoints should be left in place.  */
  if (!may_insert_breakpoints)
    {
      warning (_("May not remove breakpoints"));
      return 1;
    }

  target_ops *target = current_inferior ()->top_target ();

  return target->remove_breakpoint (gdbarch, bp_tgt, reason);
}

static void
info_target_command (const char *args, int from_tty)
{
  int has_all_mem = 0;

  if (current_program_space->symfile_object_file != NULL)
    {
      objfile *objf = current_program_space->symfile_object_file;
      gdb_printf (_("Symbols from \"%s\".\n"),
		  objfile_name (objf));
    }

  for (target_ops *t = current_inferior ()->top_target ();
       t != NULL;
       t = t->beneath ())
    {
      if (!t->has_memory ())
	continue;

      if ((int) (t->stratum ()) <= (int) dummy_stratum)
	continue;
      if (has_all_mem)
	gdb_printf (_("\tWhile running this, "
		      "GDB does not access memory from...\n"));
      gdb_printf ("%s:\n", t->longname ());
      t->files_info ();
      has_all_mem = t->has_all_memory ();
    }
}

/* This function is called before any new inferior is created, e.g.
   by running a program, attaching, or connecting to a target.
   It cleans up any state from previous invocations which might
   change between runs.  This is a subset of what target_preopen
   resets (things which might change between targets).  */

void
target_pre_inferior (int from_tty)
{
  /* Clear out solib state.  Otherwise the solib state of the previous
     inferior might have survived and is entirely wrong for the new
     target.  This has been observed on GNU/Linux using glibc 2.3.  How
     to reproduce:

     bash$ ./foo&
     [1] 4711
     bash$ ./foo&
     [1] 4712
     bash$ gdb ./foo
     [...]
     (gdb) attach 4711
     (gdb) detach
     (gdb) attach 4712
     Cannot access memory at address 0xdeadbeef
  */

  /* In some OSs, the shared library list is the same/global/shared
     across inferiors.  If code is shared between processes, so are
     memory regions and features.  */
  if (!gdbarch_has_global_solist (current_inferior ()->arch ()))
    {
      no_shared_libraries (NULL, from_tty);

      invalidate_target_mem_regions ();

      target_clear_description ();
    }

  /* attach_flag may be set if the previous process associated with
     the inferior was attached to.  */
  current_inferior ()->attach_flag = false;

  current_inferior ()->highest_thread_num = 0;

  update_previous_thread ();

  agent_capability_invalidate ();
}

/* This is to be called by the open routine before it does
   anything.  */

void
target_preopen (int from_tty)
{
  dont_repeat ();

  if (current_inferior ()->pid != 0)
    {
      if (!from_tty
	  || !target_has_execution ()
	  || query (_("A program is being debugged already.  Kill it? ")))
	{
	  /* Core inferiors actually should be detached, not
	     killed.  */
	  if (target_has_execution ())
	    target_kill ();
	  else
	    target_detach (current_inferior (), 0);
	}
      else
	error (_("Program not killed."));
    }

  /* Release reference to old previous thread.  */
  update_previous_thread ();

  /* Calling target_kill may remove the target from the stack.  But if
     it doesn't (which seems like a win for UDI), remove it now.  */
  /* Leave the exec target, though.  The user may be switching from a
     live process to a core of the same program.  */
  current_inferior ()->pop_all_targets_above (file_stratum);

  target_pre_inferior (from_tty);
}

/* See target.h.  */

void
target_detach (inferior *inf, int from_tty)
{
  /* Thread's don't need to be resumed until the end of this function.  */
  scoped_disable_commit_resumed disable_commit_resumed ("detaching");

  /* After we have detached, we will clear the register cache for this inferior
     by calling registers_changed_ptid.  We must save the pid_ptid before
     detaching, as the target detach method will clear inf->pid.  */
  ptid_t save_pid_ptid = ptid_t (inf->pid);

  /* As long as some to_detach implementations rely on the current_inferior
     (either directly, or indirectly, like through reading memory), INF needs
     to be the current inferior.  When that requirement will become no longer
     true, then we can remove this assertion.  */
  gdb_assert (inf == current_inferior ());

  prepare_for_detach ();

  gdb::observers::inferior_pre_detach.notify (inf);

  /* Hold a strong reference because detaching may unpush the
     target.  */
  auto proc_target_ref = target_ops_ref::new_reference (inf->process_target ());

  current_inferior ()->top_target ()->detach (inf, from_tty);

  process_stratum_target *proc_target
    = as_process_stratum_target (proc_target_ref.get ());

  registers_changed_ptid (proc_target, save_pid_ptid);

  /* We have to ensure we have no frame cache left.  Normally,
     registers_changed_ptid (save_pid_ptid) calls reinit_frame_cache when
     inferior_ptid matches save_pid_ptid, but in our case, it does not
     call it, as inferior_ptid has been reset.  */
  reinit_frame_cache ();

  disable_commit_resumed.reset_and_commit ();
}

void
target_disconnect (const char *args, int from_tty)
{
  /* If we're in breakpoints-always-inserted mode or if breakpoints
     are global across processes, we have to remove them before
     disconnecting.  */
  remove_breakpoints ();

  current_inferior ()->top_target ()->disconnect (args, from_tty);
}

/* See target/target.h.  */

ptid_t
target_wait (ptid_t ptid, struct target_waitstatus *status,
	     target_wait_flags options)
{
  target_ops *target = current_inferior ()->top_target ();
  process_stratum_target *proc_target = current_inferior ()->process_target ();

  gdb_assert (!proc_target->commit_resumed_state);

  if (!target_can_async_p (target))
    gdb_assert ((options & TARGET_WNOHANG) == 0);

  try
    {
      gdb::observers::target_pre_wait.notify (ptid);
      ptid_t event_ptid = target->wait (ptid, status, options);
      gdb::observers::target_post_wait.notify (event_ptid);
      return event_ptid;
    }
  catch (...)
    {
      gdb::observers::target_post_wait.notify (null_ptid);
      throw;
    }
}

/* See target.h.  */

ptid_t
default_target_wait (struct target_ops *ops,
		     ptid_t ptid, struct target_waitstatus *status,
		     target_wait_flags options)
{
  status->set_ignore ();
  return minus_one_ptid;
}

std::string
target_pid_to_str (ptid_t ptid)
{
  return current_inferior ()->top_target ()->pid_to_str (ptid);
}

const char *
target_thread_name (struct thread_info *info)
{
  gdb_assert (info->inf == current_inferior ());

  return current_inferior ()->top_target ()->thread_name (info);
}

struct thread_info *
target_thread_handle_to_thread_info (const gdb_byte *thread_handle,
				     int handle_len,
				     struct inferior *inf)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->thread_handle_to_thread_info (thread_handle, handle_len, inf);
}

/* See target.h.  */

gdb::array_view<const gdb_byte>
target_thread_info_to_thread_handle (struct thread_info *tip)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->thread_info_to_thread_handle (tip);
}

void
target_resume (ptid_t scope_ptid, int step, enum gdb_signal signal)
{
  process_stratum_target *curr_target = current_inferior ()->process_target ();
  gdb_assert (!curr_target->commit_resumed_state);

  gdb_assert (inferior_ptid != null_ptid);
  gdb_assert (inferior_ptid.matches (scope_ptid));

  target_dcache_invalidate (current_program_space->aspace);

  current_inferior ()->top_target ()->resume (scope_ptid, step, signal);

  registers_changed_ptid (curr_target, scope_ptid);
  /* We only set the internal executing state here.  The user/frontend
     running state is set at a higher level.  This also clears the
     thread's stop_pc as side effect.  */
  set_executing (curr_target, scope_ptid, true);
  clear_inline_frame_state (curr_target, scope_ptid);

  if (target_can_async_p ())
    target_async (true);
}

/* See target.h.  */

void
target_commit_resumed ()
{
  gdb_assert (current_inferior ()->process_target ()->commit_resumed_state);
  current_inferior ()->top_target ()->commit_resumed ();
}

/* See target.h.  */

bool
target_has_pending_events ()
{
  return current_inferior ()->top_target ()->has_pending_events ();
}

void
target_pass_signals (gdb::array_view<const unsigned char> pass_signals)
{
  current_inferior ()->top_target ()->pass_signals (pass_signals);
}

void
target_program_signals (gdb::array_view<const unsigned char> program_signals)
{
  current_inferior ()->top_target ()->program_signals (program_signals);
}

static void
default_follow_fork (struct target_ops *self, inferior *child_inf,
		     ptid_t child_ptid, target_waitkind fork_kind,
		     bool follow_child, bool detach_fork)
{
  /* Some target returned a fork event, but did not know how to follow it.  */
  internal_error (_("could not find a target to follow fork"));
}

static void
default_follow_clone (struct target_ops *self, ptid_t child_ptid)
{
  /* Some target returned a clone event, but did not know how to follow it.  */
  internal_error (_("could not find a target to follow clone"));
}

/* See target.h.  */

void
target_follow_fork (inferior *child_inf, ptid_t child_ptid,
		    target_waitkind fork_kind, bool follow_child,
		    bool detach_fork)
{
  target_ops *target = current_inferior ()->top_target ();

  /* Check consistency between CHILD_INF, CHILD_PTID, FOLLOW_CHILD and
     DETACH_FORK.  */
  if (child_inf != nullptr)
    {
      gdb_assert (follow_child || !detach_fork);
      gdb_assert (child_inf->pid == child_ptid.pid ());
    }
  else
    gdb_assert (!follow_child && detach_fork);

  return target->follow_fork (child_inf, child_ptid, fork_kind, follow_child,
			      detach_fork);
}

/* See target.h.  */

void
target_follow_exec (inferior *follow_inf, ptid_t ptid,
		    const char *execd_pathname)
{
  current_inferior ()->top_target ()->follow_exec (follow_inf, ptid,
						   execd_pathname);
}

static void
default_mourn_inferior (struct target_ops *self)
{
  internal_error (_("could not find a target to follow mourn inferior"));
}

void
target_mourn_inferior (ptid_t ptid)
{
  gdb_assert (ptid.pid () == inferior_ptid.pid ());
  current_inferior ()->top_target ()->mourn_inferior ();
}

/* Look for a target which can describe architectural features, starting
   from TARGET.  If we find one, return its description.  */

const struct target_desc *
target_read_description (struct target_ops *target)
{
  return target->read_description ();
}


/* Default implementation of memory-searching.  */

static int
default_search_memory (struct target_ops *self,
		       CORE_ADDR start_addr, ULONGEST search_space_len,
		       const gdb_byte *pattern, ULONGEST pattern_len,
		       CORE_ADDR *found_addrp)
{
  auto read_memory = [=] (CORE_ADDR addr, gdb_byte *result, size_t len)
    {
      return target_read (current_inferior ()->top_target (),
			  TARGET_OBJECT_MEMORY, NULL,
			  result, addr, len) == len;
    };

  /* Start over from the top of the target stack.  */
  return simple_search_memory (read_memory, start_addr, search_space_len,
			       pattern, pattern_len, found_addrp);
}

/* Search SEARCH_SPACE_LEN bytes beginning at START_ADDR for the
   sequence of bytes in PATTERN with length PATTERN_LEN.

   The result is 1 if found, 0 if not found, and -1 if there was an error
   requiring halting of the search (e.g. memory read error).
   If the pattern is found the address is recorded in FOUND_ADDRP.  */

int
target_search_memory (CORE_ADDR start_addr, ULONGEST search_space_len,
		      const gdb_byte *pattern, ULONGEST pattern_len,
		      CORE_ADDR *found_addrp)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->search_memory (start_addr, search_space_len, pattern,
				pattern_len, found_addrp);
}

/* Look through the currently pushed targets.  If none of them will
   be able to restart the currently running process, issue an error
   message.  */

void
target_require_runnable (void)
{
  for (target_ops *t = current_inferior ()->top_target ();
       t != NULL;
       t = t->beneath ())
    {
      /* If this target knows how to create a new program, then
	 assume we will still be able to after killing the current
	 one.  Either killing and mourning will not pop T, or else
	 find_default_run_target will find it again.  */
      if (t->can_create_inferior ())
	return;

      /* Do not worry about targets at certain strata that can not
	 create inferiors.  Assume they will be pushed again if
	 necessary, and continue to the process_stratum.  */
      if (t->stratum () > process_stratum)
	continue;

      error (_("The \"%s\" target does not support \"run\".  "
	       "Try \"help target\" or \"continue\"."),
	     t->shortname ());
    }

  /* This function is only called if the target is running.  In that
     case there should have been a process_stratum target and it
     should either know how to create inferiors, or not...  */
  internal_error (_("No targets found"));
}

/* Whether GDB is allowed to fall back to the default run target for
   "run", "attach", etc. when no target is connected yet.  */
static bool auto_connect_native_target = true;

static void
show_auto_connect_native_target (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Whether GDB may automatically connect to the "
		"native target is %s.\n"),
	      value);
}

/* A pointer to the target that can respond to "run" or "attach".
   Native targets are always singletons and instantiated early at GDB
   startup.  */
static target_ops *the_native_target;

/* See target.h.  */

void
set_native_target (target_ops *target)
{
  if (the_native_target != NULL)
    internal_error (_("native target already set (\"%s\")."),
		    the_native_target->longname ());

  the_native_target = target;
}

/* See target.h.  */

target_ops *
get_native_target ()
{
  return the_native_target;
}

/* Look through the list of possible targets for a target that can
   execute a run or attach command without any other data.  This is
   used to locate the default process stratum.

   If DO_MESG is not NULL, the result is always valid (error() is
   called for errors); else, return NULL on error.  */

static struct target_ops *
find_default_run_target (const char *do_mesg)
{
  if (auto_connect_native_target && the_native_target != NULL)
    return the_native_target;

  if (do_mesg != NULL)
    error (_("Don't know how to %s.  Try \"help target\"."), do_mesg);
  return NULL;
}

/* See target.h.  */

struct target_ops *
find_attach_target (void)
{
  /* If a target on the current stack can attach, use it.  */
  for (target_ops *t = current_inferior ()->top_target ();
       t != NULL;
       t = t->beneath ())
    {
      if (t->can_attach ())
	return t;
    }

  /* Otherwise, use the default run target for attaching.  */
  return find_default_run_target ("attach");
}

/* See target.h.  */

struct target_ops *
find_run_target (void)
{
  /* If a target on the current stack can run, use it.  */
  for (target_ops *t = current_inferior ()->top_target ();
       t != NULL;
       t = t->beneath ())
    {
      if (t->can_create_inferior ())
	return t;
    }

  /* Otherwise, use the default run target.  */
  return find_default_run_target ("run");
}

bool
target_ops::info_proc (const char *args, enum info_proc_what what)
{
  return false;
}

/* Implement the "info proc" command.  */

int
target_info_proc (const char *args, enum info_proc_what what)
{
  struct target_ops *t;

  /* If we're already connected to something that can get us OS
     related data, use it.  Otherwise, try using the native
     target.  */
  t = find_target_at (process_stratum);
  if (t == NULL)
    t = find_default_run_target (NULL);

  for (; t != NULL; t = t->beneath ())
    {
      if (t->info_proc (args, what))
	{
	  if (targetdebug)
	    gdb_printf (gdb_stdlog,
			"target_info_proc (\"%s\", %d)\n", args, what);

	  return 1;
	}
    }

  return 0;
}

static int
find_default_supports_disable_randomization (struct target_ops *self)
{
  struct target_ops *t;

  t = find_default_run_target (NULL);
  if (t != NULL)
    return t->supports_disable_randomization ();
  return 0;
}

int
target_supports_disable_randomization (void)
{
  return current_inferior ()->top_target ()->supports_disable_randomization ();
}

/* See target/target.h.  */

int
target_supports_multi_process (void)
{
  return current_inferior ()->top_target ()->supports_multi_process ();
}

/* See target.h.  */

std::optional<gdb::char_vector>
target_get_osdata (const char *type)
{
  struct target_ops *t;

  /* If we're already connected to something that can get us OS
     related data, use it.  Otherwise, try using the native
     target.  */
  t = find_target_at (process_stratum);
  if (t == NULL)
    t = find_default_run_target ("get OS data");

  if (!t)
    return {};

  return target_read_stralloc (t, TARGET_OBJECT_OSDATA, type);
}

/* See target.h.  */

target_ops *
target_ops::beneath () const
{
  return current_inferior ()->find_target_beneath (this);
}

void
target_ops::close ()
{
}

bool
target_ops::can_attach ()
{
  return 0;
}

void
target_ops::attach (const char *, int)
{
  gdb_assert_not_reached ("target_ops::attach called");
}

bool
target_ops::can_create_inferior ()
{
  return 0;
}

void
target_ops::create_inferior (const char *, const std::string &,
			     char **, int)
{
  gdb_assert_not_reached ("target_ops::create_inferior called");
}

bool
target_ops::can_run ()
{
  return false;
}

int
target_can_run ()
{
  for (target_ops *t = current_inferior ()->top_target ();
       t != NULL;
       t = t->beneath ())
    {
      if (t->can_run ())
	return 1;
    }

  return 0;
}

/* Target file operations.  */

static struct target_ops *
default_fileio_target (void)
{
  struct target_ops *t;

  /* If we're already connected to something that can perform
     file I/O, use it. Otherwise, try using the native target.  */
  t = find_target_at (process_stratum);
  if (t != NULL)
    return t;
  return find_default_run_target ("file I/O");
}

/* File handle for target file operations.  */

struct fileio_fh_t
{
  /* The target on which this file is open.  NULL if the target is
     meanwhile closed while the handle is open.  */
  target_ops *target;

  /* The file descriptor on the target.  */
  int target_fd;

  /* Check whether this fileio_fh_t represents a closed file.  */
  bool is_closed ()
  {
    return target_fd < 0;
  }
};

/* Vector of currently open file handles.  The value returned by
   target_fileio_open and passed as the FD argument to other
   target_fileio_* functions is an index into this vector.  This
   vector's entries are never freed; instead, files are marked as
   closed, and the handle becomes available for reuse.  */
static std::vector<fileio_fh_t> fileio_fhandles;

/* Index into fileio_fhandles of the lowest handle that might be
   closed.  This permits handle reuse without searching the whole
   list each time a new file is opened.  */
static int lowest_closed_fd;

/* See target.h.  */

void
fileio_handles_invalidate_target (target_ops *targ)
{
  for (fileio_fh_t &fh : fileio_fhandles)
    if (fh.target == targ)
      fh.target = NULL;
}

/* Acquire a target fileio file descriptor.  */

static int
acquire_fileio_fd (target_ops *target, int target_fd)
{
  /* Search for closed handles to reuse.  */
  for (; lowest_closed_fd < fileio_fhandles.size (); lowest_closed_fd++)
    {
      fileio_fh_t &fh = fileio_fhandles[lowest_closed_fd];

      if (fh.is_closed ())
	break;
    }

  /* Push a new handle if no closed handles were found.  */
  if (lowest_closed_fd == fileio_fhandles.size ())
    fileio_fhandles.push_back (fileio_fh_t {target, target_fd});
  else
    fileio_fhandles[lowest_closed_fd] = {target, target_fd};

  /* Should no longer be marked closed.  */
  gdb_assert (!fileio_fhandles[lowest_closed_fd].is_closed ());

  /* Return its index, and start the next lookup at
     the next index.  */
  return lowest_closed_fd++;
}

/* Release a target fileio file descriptor.  */

static void
release_fileio_fd (int fd, fileio_fh_t *fh)
{
  fh->target_fd = -1;
  lowest_closed_fd = std::min (lowest_closed_fd, fd);
}

/* Return a pointer to the fileio_fhandle_t corresponding to FD.  */

static fileio_fh_t *
fileio_fd_to_fh (int fd)
{
  return &fileio_fhandles[fd];
}


/* Default implementations of file i/o methods.  We don't want these
   to delegate automatically, because we need to know which target
   supported the method, in order to call it directly from within
   pread/pwrite, etc.  */

int
target_ops::fileio_open (struct inferior *inf, const char *filename,
			 int flags, int mode, int warn_if_slow,
			 fileio_error *target_errno)
{
  *target_errno = FILEIO_ENOSYS;
  return -1;
}

int
target_ops::fileio_pwrite (int fd, const gdb_byte *write_buf, int len,
			   ULONGEST offset, fileio_error *target_errno)
{
  *target_errno = FILEIO_ENOSYS;
  return -1;
}

int
target_ops::fileio_pread (int fd, gdb_byte *read_buf, int len,
			  ULONGEST offset, fileio_error *target_errno)
{
  *target_errno = FILEIO_ENOSYS;
  return -1;
}

int
target_ops::fileio_fstat (int fd, struct stat *sb, fileio_error *target_errno)
{
  *target_errno = FILEIO_ENOSYS;
  return -1;
}

int
target_ops::fileio_close (int fd, fileio_error *target_errno)
{
  *target_errno = FILEIO_ENOSYS;
  return -1;
}

int
target_ops::fileio_unlink (struct inferior *inf, const char *filename,
			   fileio_error *target_errno)
{
  *target_errno = FILEIO_ENOSYS;
  return -1;
}

std::optional<std::string>
target_ops::fileio_readlink (struct inferior *inf, const char *filename,
			     fileio_error *target_errno)
{
  *target_errno = FILEIO_ENOSYS;
  return {};
}

/* See target.h.  */

int
target_fileio_open (struct inferior *inf, const char *filename,
		    int flags, int mode, bool warn_if_slow, fileio_error *target_errno)
{
  for (target_ops *t = default_fileio_target (); t != NULL; t = t->beneath ())
    {
      int fd = t->fileio_open (inf, filename, flags, mode,
			       warn_if_slow, target_errno);

      if (fd == -1 && *target_errno == FILEIO_ENOSYS)
	continue;

      if (fd < 0)
	fd = -1;
      else
	fd = acquire_fileio_fd (t, fd);

      if (targetdebug)
	gdb_printf (gdb_stdlog,
		    "target_fileio_open (%d,%s,0x%x,0%o,%d)"
		    " = %d (%d)\n",
		    inf == NULL ? 0 : inf->num,
		    filename, flags, mode,
		    warn_if_slow, fd,
		    fd != -1 ? 0 : *target_errno);
      return fd;
    }

  *target_errno = FILEIO_ENOSYS;
  return -1;
}

/* See target.h.  */

int
target_fileio_pwrite (int fd, const gdb_byte *write_buf, int len,
		      ULONGEST offset, fileio_error *target_errno)
{
  fileio_fh_t *fh = fileio_fd_to_fh (fd);
  int ret = -1;

  if (fh->is_closed ())
    *target_errno = FILEIO_EBADF;
  else if (fh->target == NULL)
    *target_errno = FILEIO_EIO;
  else
    ret = fh->target->fileio_pwrite (fh->target_fd, write_buf,
				     len, offset, target_errno);

  if (targetdebug)
    gdb_printf (gdb_stdlog,
		"target_fileio_pwrite (%d,...,%d,%s) "
		"= %d (%d)\n",
		fd, len, pulongest (offset),
		ret, ret != -1 ? 0 : *target_errno);
  return ret;
}

/* See target.h.  */

int
target_fileio_pread (int fd, gdb_byte *read_buf, int len,
		     ULONGEST offset, fileio_error *target_errno)
{
  fileio_fh_t *fh = fileio_fd_to_fh (fd);
  int ret = -1;

  if (fh->is_closed ())
    *target_errno = FILEIO_EBADF;
  else if (fh->target == NULL)
    *target_errno = FILEIO_EIO;
  else
    ret = fh->target->fileio_pread (fh->target_fd, read_buf,
				    len, offset, target_errno);

  if (targetdebug)
    gdb_printf (gdb_stdlog,
		"target_fileio_pread (%d,...,%d,%s) "
		"= %d (%d)\n",
		fd, len, pulongest (offset),
		ret, ret != -1 ? 0 : *target_errno);
  return ret;
}

/* See target.h.  */

int
target_fileio_fstat (int fd, struct stat *sb, fileio_error *target_errno)
{
  fileio_fh_t *fh = fileio_fd_to_fh (fd);
  int ret = -1;

  if (fh->is_closed ())
    *target_errno = FILEIO_EBADF;
  else if (fh->target == NULL)
    *target_errno = FILEIO_EIO;
  else
    ret = fh->target->fileio_fstat (fh->target_fd, sb, target_errno);

  if (targetdebug)
    gdb_printf (gdb_stdlog,
		"target_fileio_fstat (%d) = %d (%d)\n",
		fd, ret, ret != -1 ? 0 : *target_errno);
  return ret;
}

/* See target.h.  */

int
target_fileio_close (int fd, fileio_error *target_errno)
{
  fileio_fh_t *fh = fileio_fd_to_fh (fd);
  int ret = -1;

  if (fh->is_closed ())
    *target_errno = FILEIO_EBADF;
  else
    {
      if (fh->target != NULL)
	ret = fh->target->fileio_close (fh->target_fd,
					target_errno);
      else
	ret = 0;
      release_fileio_fd (fd, fh);
    }

  if (targetdebug)
    gdb_printf (gdb_stdlog,
		"target_fileio_close (%d) = %d (%d)\n",
		fd, ret, ret != -1 ? 0 : *target_errno);
  return ret;
}

/* See target.h.  */

int
target_fileio_unlink (struct inferior *inf, const char *filename,
		      fileio_error *target_errno)
{
  for (target_ops *t = default_fileio_target (); t != NULL; t = t->beneath ())
    {
      int ret = t->fileio_unlink (inf, filename, target_errno);

      if (ret == -1 && *target_errno == FILEIO_ENOSYS)
	continue;

      if (targetdebug)
	gdb_printf (gdb_stdlog,
		    "target_fileio_unlink (%d,%s)"
		    " = %d (%d)\n",
		    inf == NULL ? 0 : inf->num, filename,
		    ret, ret != -1 ? 0 : *target_errno);
      return ret;
    }

  *target_errno = FILEIO_ENOSYS;
  return -1;
}

/* See target.h.  */

std::optional<std::string>
target_fileio_readlink (struct inferior *inf, const char *filename,
			fileio_error *target_errno)
{
  for (target_ops *t = default_fileio_target (); t != NULL; t = t->beneath ())
    {
      std::optional<std::string> ret
	= t->fileio_readlink (inf, filename, target_errno);

      if (!ret.has_value () && *target_errno == FILEIO_ENOSYS)
	continue;

      if (targetdebug)
	gdb_printf (gdb_stdlog,
		    "target_fileio_readlink (%d,%s)"
		    " = %s (%d)\n",
		    inf == NULL ? 0 : inf->num,
		    filename, ret ? ret->c_str () : "(nil)",
		    ret ? 0 : *target_errno);
      return ret;
    }

  *target_errno = FILEIO_ENOSYS;
  return {};
}

/* Like scoped_fd, but specific to target fileio.  */

class scoped_target_fd
{
public:
  explicit scoped_target_fd (int fd) noexcept
    : m_fd (fd)
  {
  }

  ~scoped_target_fd ()
  {
    if (m_fd >= 0)
      {
	fileio_error target_errno;

	target_fileio_close (m_fd, &target_errno);
      }
  }

  DISABLE_COPY_AND_ASSIGN (scoped_target_fd);

  int get () const noexcept
  {
    return m_fd;
  }

private:
  int m_fd;
};

/* Read target file FILENAME, in the filesystem as seen by INF.  If
   INF is NULL, use the filesystem seen by the debugger (GDB or, for
   remote targets, the remote stub).  Store the result in *BUF_P and
   return the size of the transferred data.  PADDING additional bytes
   are available in *BUF_P.  This is a helper function for
   target_fileio_read_alloc; see the declaration of that function for
   more information.  */

static LONGEST
target_fileio_read_alloc_1 (struct inferior *inf, const char *filename,
			    gdb_byte **buf_p, int padding)
{
  size_t buf_alloc, buf_pos;
  gdb_byte *buf;
  LONGEST n;
  fileio_error target_errno;

  scoped_target_fd fd (target_fileio_open (inf, filename, FILEIO_O_RDONLY,
					   0700, false, &target_errno));
  if (fd.get () == -1)
    return -1;

  /* Start by reading up to 4K at a time.  The target will throttle
     this number down if necessary.  */
  buf_alloc = 4096;
  buf = (gdb_byte *) xmalloc (buf_alloc);
  buf_pos = 0;
  while (1)
    {
      n = target_fileio_pread (fd.get (), &buf[buf_pos],
			       buf_alloc - buf_pos - padding, buf_pos,
			       &target_errno);
      if (n < 0)
	{
	  /* An error occurred.  */
	  xfree (buf);
	  return -1;
	}
      else if (n == 0)
	{
	  /* Read all there was.  */
	  if (buf_pos == 0)
	    xfree (buf);
	  else
	    *buf_p = buf;
	  return buf_pos;
	}

      buf_pos += n;

      /* If the buffer is filling up, expand it.  */
      if (buf_alloc < buf_pos * 2)
	{
	  buf_alloc *= 2;
	  buf = (gdb_byte *) xrealloc (buf, buf_alloc);
	}

      QUIT;
    }
}

/* See target.h.  */

LONGEST
target_fileio_read_alloc (struct inferior *inf, const char *filename,
			  gdb_byte **buf_p)
{
  return target_fileio_read_alloc_1 (inf, filename, buf_p, 0);
}

/* See target.h.  */

gdb::unique_xmalloc_ptr<char> 
target_fileio_read_stralloc (struct inferior *inf, const char *filename)
{
  gdb_byte *buffer;
  char *bufstr;
  LONGEST i, transferred;

  transferred = target_fileio_read_alloc_1 (inf, filename, &buffer, 1);
  bufstr = (char *) buffer;

  if (transferred < 0)
    return gdb::unique_xmalloc_ptr<char> (nullptr);

  if (transferred == 0)
    return make_unique_xstrdup ("");

  bufstr[transferred] = 0;

  /* Check for embedded NUL bytes; but allow trailing NULs.  */
  for (i = strlen (bufstr); i < transferred; i++)
    if (bufstr[i] != 0)
      {
	warning (_("target file %s "
		   "contained unexpected null characters"),
		 filename);
	break;
      }

  return gdb::unique_xmalloc_ptr<char> (bufstr);
}


static int
default_region_ok_for_hw_watchpoint (struct target_ops *self,
				     CORE_ADDR addr, int len)
{
  gdbarch *arch = current_inferior ()->arch ();
  return (len <= gdbarch_ptr_bit (arch) / TARGET_CHAR_BIT);
}

static int
default_watchpoint_addr_within_range (struct target_ops *target,
				      CORE_ADDR addr,
				      CORE_ADDR start, int length)
{
  return addr >= start && addr < start + length;
}

/* See target.h.  */

target_ops *
target_stack::find_beneath (const target_ops *t) const
{
  /* Look for a non-empty slot at stratum levels beneath T's.  */
  for (int stratum = t->stratum () - 1; stratum >= 0; --stratum)
    if (m_stack[stratum].get () != NULL)
      return m_stack[stratum].get ();

  return NULL;
}

/* See target.h.  */

struct target_ops *
find_target_at (enum strata stratum)
{
  return current_inferior ()->target_at (stratum);
}



/* See target.h  */

void
target_announce_detach (int from_tty)
{
  pid_t pid;
  const char *exec_file;

  if (!from_tty)
    return;

  pid = inferior_ptid.pid ();
  exec_file = get_exec_file (0);
  if (exec_file == nullptr)
    gdb_printf ("Detaching from pid %s\n",
		target_pid_to_str (ptid_t (pid)).c_str ());
  else
    gdb_printf (_("Detaching from program: %s, %s\n"), exec_file,
		target_pid_to_str (ptid_t (pid)).c_str ());
}

/* See target.h  */

void
target_announce_attach (int from_tty, int pid)
{
  if (!from_tty)
    return;

  const char *exec_file = get_exec_file (0);

  if (exec_file != nullptr)
    gdb_printf ("Attaching to program: %s, %s\n", exec_file,
		target_pid_to_str (ptid_t (pid)).c_str ());
  else
    gdb_printf ("Attaching to %s\n",
		target_pid_to_str (ptid_t (pid)).c_str ());
}

/* The inferior process has died.  Long live the inferior!  */

void
generic_mourn_inferior (void)
{
  inferior *inf = current_inferior ();

  switch_to_no_thread ();

  /* Mark breakpoints uninserted in case something tries to delete a
     breakpoint while we delete the inferior's threads (which would
     fail, since the inferior is long gone).  */
  mark_breakpoints_out ();

  if (inf->pid != 0)
    exit_inferior (inf);

  /* Note this wipes step-resume breakpoints, so needs to be done
     after exit_inferior, which ends up referencing the step-resume
     breakpoints through clear_thread_inferior_resources.  */
  breakpoint_init_inferior (inf_exited);

  registers_changed ();

  reopen_exec_file ();
  reinit_frame_cache ();

  if (deprecated_detach_hook)
    deprecated_detach_hook ();
}

/* Convert a normal process ID to a string.  Returns the string in a
   static buffer.  */

std::string
normal_pid_to_str (ptid_t ptid)
{
  return string_printf ("process %d", ptid.pid ());
}

static std::string
default_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  return normal_pid_to_str (ptid);
}

/* Error-catcher for target_find_memory_regions.  */
static int
dummy_find_memory_regions (struct target_ops *self,
			   find_memory_region_ftype ignore1, void *ignore2)
{
  error (_("Command not implemented for this target."));
  return 0;
}

/* Error-catcher for target_make_corefile_notes.  */
static gdb::unique_xmalloc_ptr<char>
dummy_make_corefile_notes (struct target_ops *self,
			   bfd *ignore1, int *ignore2)
{
  error (_("Command not implemented for this target."));
  return NULL;
}

#include "target-delegates.c"

/* The initial current target, so that there is always a semi-valid
   current target.  */

static dummy_target the_dummy_target;

/* See target.h.  */

target_ops *
get_dummy_target ()
{
  return &the_dummy_target;
}

static const target_info dummy_target_info = {
  "None",
  N_("None"),
  ""
};

strata
dummy_target::stratum () const
{
  return dummy_stratum;
}

strata
debug_target::stratum () const
{
  return debug_stratum;
}

const target_info &
dummy_target::info () const
{
  return dummy_target_info;
}

const target_info &
debug_target::info () const
{
  return beneath ()->info ();
}



int
target_thread_alive (ptid_t ptid)
{
  return current_inferior ()->top_target ()->thread_alive (ptid);
}

void
target_update_thread_list (void)
{
  current_inferior ()->top_target ()->update_thread_list ();
}

void
target_stop (ptid_t ptid)
{
  process_stratum_target *proc_target = current_inferior ()->process_target ();

  gdb_assert (!proc_target->commit_resumed_state);

  if (!may_stop)
    {
      warning (_("May not interrupt or stop the target, ignoring attempt"));
      return;
    }

  current_inferior ()->top_target ()->stop (ptid);
}

void
target_interrupt ()
{
  if (!may_stop)
    {
      warning (_("May not interrupt or stop the target, ignoring attempt"));
      return;
    }

  current_inferior ()->top_target ()->interrupt ();
}

/* See target.h.  */

void
target_pass_ctrlc (void)
{
  /* Pass the Ctrl-C to the first target that has a thread
     running.  */
  for (inferior *inf : all_inferiors ())
    {
      target_ops *proc_target = inf->process_target ();
      if (proc_target == NULL)
	continue;

      for (thread_info *thr : inf->non_exited_threads ())
	{
	  /* A thread can be THREAD_STOPPED and executing, while
	     running an infcall.  */
	  if (thr->state == THREAD_RUNNING || thr->executing ())
	    {
	      /* We can get here quite deep in target layers.  Avoid
		 switching thread context or anything that would
		 communicate with the target (e.g., to fetch
		 registers), or flushing e.g., the frame cache.  We
		 just switch inferior in order to be able to call
		 through the target_stack.  */
	      scoped_restore_current_inferior restore_inferior;
	      set_current_inferior (inf);
	      current_inferior ()->top_target ()->pass_ctrlc ();
	      return;
	    }
	}
    }
}

/* See target.h.  */

void
default_target_pass_ctrlc (struct target_ops *ops)
{
  target_interrupt ();
}

/* See target/target.h.  */

void
target_stop_and_wait (ptid_t ptid)
{
  struct target_waitstatus status;
  bool was_non_stop = non_stop;

  non_stop = true;
  target_stop (ptid);

  target_wait (ptid, &status, 0);

  non_stop = was_non_stop;
}

/* See target/target.h.  */

void
target_continue_no_signal (ptid_t ptid)
{
  target_resume (ptid, 0, GDB_SIGNAL_0);
}

/* See target/target.h.  */

void
target_continue (ptid_t ptid, enum gdb_signal signal)
{
  target_resume (ptid, 0, signal);
}

/* Concatenate ELEM to LIST, a comma-separated list.  */

static void
str_comma_list_concat_elem (std::string *list, const char *elem)
{
  if (!list->empty ())
    list->append (", ");

  list->append (elem);
}

/* Helper for target_options_to_string.  If OPT is present in
   TARGET_OPTIONS, append the OPT_STR (string version of OPT) in RET.
   OPT is removed from TARGET_OPTIONS.  */

static void
do_option (target_wait_flags *target_options, std::string *ret,
	   target_wait_flag opt, const char *opt_str)
{
  if ((*target_options & opt) != 0)
    {
      str_comma_list_concat_elem (ret, opt_str);
      *target_options &= ~opt;
    }
}

/* See target.h.  */

std::string
target_options_to_string (target_wait_flags target_options)
{
  std::string ret;

#define DO_TARG_OPTION(OPT) \
  do_option (&target_options, &ret, OPT, #OPT)

  DO_TARG_OPTION (TARGET_WNOHANG);

  if (target_options != 0)
    str_comma_list_concat_elem (&ret, "unknown???");

  return ret;
}

void
target_fetch_registers (struct regcache *regcache, int regno)
{
  current_inferior ()->top_target ()->fetch_registers (regcache, regno);
  if (targetdebug)
    regcache->debug_print_register ("target_fetch_registers", regno);
}

void
target_store_registers (struct regcache *regcache, int regno)
{
  if (!may_write_registers)
    error (_("Writing to registers is not allowed (regno %d)"), regno);

  current_inferior ()->top_target ()->store_registers (regcache, regno);
  if (targetdebug)
    {
      regcache->debug_print_register ("target_store_registers", regno);
    }
}

int
target_core_of_thread (ptid_t ptid)
{
  return current_inferior ()->top_target ()->core_of_thread (ptid);
}

int
simple_verify_memory (struct target_ops *ops,
		      const gdb_byte *data, CORE_ADDR lma, ULONGEST size)
{
  LONGEST total_xfered = 0;

  while (total_xfered < size)
    {
      ULONGEST xfered_len;
      enum target_xfer_status status;
      gdb_byte buf[1024];
      ULONGEST howmuch = std::min<ULONGEST> (sizeof (buf), size - total_xfered);

      status = target_xfer_partial (ops, TARGET_OBJECT_MEMORY, NULL,
				    buf, NULL, lma + total_xfered, howmuch,
				    &xfered_len);
      if (status == TARGET_XFER_OK
	  && memcmp (data + total_xfered, buf, xfered_len) == 0)
	{
	  total_xfered += xfered_len;
	  QUIT;
	}
      else
	return 0;
    }
  return 1;
}

/* Default implementation of memory verification.  */

static int
default_verify_memory (struct target_ops *self,
		       const gdb_byte *data, CORE_ADDR memaddr, ULONGEST size)
{
  /* Start over from the top of the target stack.  */
  return simple_verify_memory (current_inferior ()->top_target (),
			       data, memaddr, size);
}

int
target_verify_memory (const gdb_byte *data, CORE_ADDR memaddr, ULONGEST size)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->verify_memory (data, memaddr, size);
}

/* The documentation for this function is in its prototype declaration in
   target.h.  */

int
target_insert_mask_watchpoint (CORE_ADDR addr, CORE_ADDR mask,
			       enum target_hw_bp_type rw)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->insert_mask_watchpoint (addr, mask, rw);
}

/* The documentation for this function is in its prototype declaration in
   target.h.  */

int
target_remove_mask_watchpoint (CORE_ADDR addr, CORE_ADDR mask,
			       enum target_hw_bp_type rw)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->remove_mask_watchpoint (addr, mask, rw);
}

/* The documentation for this function is in its prototype declaration
   in target.h.  */

int
target_masked_watch_num_registers (CORE_ADDR addr, CORE_ADDR mask)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->masked_watch_num_registers (addr, mask);
}

/* The documentation for this function is in its prototype declaration
   in target.h.  */

int
target_ranged_break_num_registers (void)
{
  return current_inferior ()->top_target ()->ranged_break_num_registers ();
}

/* See target.h.  */

struct btrace_target_info *
target_enable_btrace (thread_info *tp, const struct btrace_config *conf)
{
  return current_inferior ()->top_target ()->enable_btrace (tp, conf);
}

/* See target.h.  */

void
target_disable_btrace (struct btrace_target_info *btinfo)
{
  current_inferior ()->top_target ()->disable_btrace (btinfo);
}

/* See target.h.  */

void
target_teardown_btrace (struct btrace_target_info *btinfo)
{
  current_inferior ()->top_target ()->teardown_btrace (btinfo);
}

/* See target.h.  */

enum btrace_error
target_read_btrace (struct btrace_data *btrace,
		    struct btrace_target_info *btinfo,
		    enum btrace_read_type type)
{
  target_ops *target = current_inferior ()->top_target ();

  return target->read_btrace (btrace, btinfo, type);
}

/* See target.h.  */

const struct btrace_config *
target_btrace_conf (const struct btrace_target_info *btinfo)
{
  return current_inferior ()->top_target ()->btrace_conf (btinfo);
}

/* See target.h.  */

void
target_stop_recording (void)
{
  current_inferior ()->top_target ()->stop_recording ();
}

/* See target.h.  */

void
target_save_record (const char *filename)
{
  current_inferior ()->top_target ()->save_record (filename);
}

/* See target.h.  */

int
target_supports_delete_record ()
{
  return current_inferior ()->top_target ()->supports_delete_record ();
}

/* See target.h.  */

void
target_delete_record (void)
{
  current_inferior ()->top_target ()->delete_record ();
}

/* See target.h.  */

enum record_method
target_record_method (ptid_t ptid)
{
  return current_inferior ()->top_target ()->record_method (ptid);
}

/* See target.h.  */

int
target_record_is_replaying (ptid_t ptid)
{
  return current_inferior ()->top_target ()->record_is_replaying (ptid);
}

/* See target.h.  */

int
target_record_will_replay (ptid_t ptid, int dir)
{
  return current_inferior ()->top_target ()->record_will_replay (ptid, dir);
}

/* See target.h.  */

void
target_record_stop_replaying (void)
{
  current_inferior ()->top_target ()->record_stop_replaying ();
}

/* See target.h.  */

void
target_goto_record_begin (void)
{
  current_inferior ()->top_target ()->goto_record_begin ();
}

/* See target.h.  */

void
target_goto_record_end (void)
{
  current_inferior ()->top_target ()->goto_record_end ();
}

/* See target.h.  */

void
target_goto_record (ULONGEST insn)
{
  current_inferior ()->top_target ()->goto_record (insn);
}

/* See target.h.  */

void
target_insn_history (int size, gdb_disassembly_flags flags)
{
  current_inferior ()->top_target ()->insn_history (size, flags);
}

/* See target.h.  */

void
target_insn_history_from (ULONGEST from, int size,
			  gdb_disassembly_flags flags)
{
  current_inferior ()->top_target ()->insn_history_from (from, size, flags);
}

/* See target.h.  */

void
target_insn_history_range (ULONGEST begin, ULONGEST end,
			   gdb_disassembly_flags flags)
{
  current_inferior ()->top_target ()->insn_history_range (begin, end, flags);
}

/* See target.h.  */

void
target_call_history (int size, record_print_flags flags)
{
  current_inferior ()->top_target ()->call_history (size, flags);
}

/* See target.h.  */

void
target_call_history_from (ULONGEST begin, int size, record_print_flags flags)
{
  current_inferior ()->top_target ()->call_history_from (begin, size, flags);
}

/* See target.h.  */

void
target_call_history_range (ULONGEST begin, ULONGEST end, record_print_flags flags)
{
  current_inferior ()->top_target ()->call_history_range (begin, end, flags);
}

/* See target.h.  */

const struct frame_unwind *
target_get_unwinder (void)
{
  return current_inferior ()->top_target ()->get_unwinder ();
}

/* See target.h.  */

const struct frame_unwind *
target_get_tailcall_unwinder (void)
{
  return current_inferior ()->top_target ()->get_tailcall_unwinder ();
}

/* See target.h.  */

void
target_prepare_to_generate_core (void)
{
  current_inferior ()->top_target ()->prepare_to_generate_core ();
}

/* See target.h.  */

void
target_done_generating_core (void)
{
  current_inferior ()->top_target ()->done_generating_core ();
}



static char targ_desc[] =
"Names of targets and files being debugged.\nShows the entire \
stack of targets currently in use (including the exec-file,\n\
core-file, and process, if any), as well as the symbol file name.";

static void
default_rcmd (struct target_ops *self, const char *command,
	      struct ui_file *output)
{
  error (_("\"monitor\" command not supported by this target."));
}

static void
do_monitor_command (const char *cmd, int from_tty)
{
  target_rcmd (cmd, gdb_stdtarg);
}

/* Erases all the memory regions marked as flash.  CMD and FROM_TTY are
   ignored.  */

void
flash_erase_command (const char *cmd, int from_tty)
{
  /* Used to communicate termination of flash operations to the target.  */
  bool found_flash_region = false;
  gdbarch *gdbarch = current_inferior ()->arch ();

  std::vector<mem_region> mem_regions = target_memory_map ();

  /* Iterate over all memory regions.  */
  for (const mem_region &m : mem_regions)
    {
      /* Is this a flash memory region?  */
      if (m.attrib.mode == MEM_FLASH)
	{
	  found_flash_region = true;
	  target_flash_erase (m.lo, m.hi - m.lo);

	  ui_out_emit_tuple tuple_emitter (current_uiout, "erased-regions");

	  current_uiout->message (_("Erasing flash memory region at address "));
	  current_uiout->field_core_addr ("address", gdbarch, m.lo);
	  current_uiout->message (", size = ");
	  current_uiout->field_string ("size", hex_string (m.hi - m.lo));
	  current_uiout->message ("\n");
	}
    }

  /* Did we do any flash operations?  If so, we need to finalize them.  */
  if (found_flash_region)
    target_flash_done ();
  else
    current_uiout->message (_("No flash memory regions found.\n"));
}

/* Print the name of each layers of our target stack.  */

static void
maintenance_print_target_stack (const char *cmd, int from_tty)
{
  gdb_printf (_("The current target stack is:\n"));

  for (target_ops *t = current_inferior ()->top_target ();
       t != NULL;
       t = t->beneath ())
    {
      if (t->stratum () == debug_stratum)
	continue;
      gdb_printf ("  - %s (%s)\n", t->shortname (), t->longname ());
    }
}

/* See target.h.  */

void
target_async (bool enable)
{
  /* If we are trying to enable async mode then it must be the case that
     async mode is possible for this target.  */
  gdb_assert (!enable || target_can_async_p ());
  infrun_async (enable);
  current_inferior ()->top_target ()->async (enable);
}

/* See target.h.  */

void
target_thread_events (int enable)
{
  current_inferior ()->top_target ()->thread_events (enable);
}

/* See target.h.  */

bool
target_supports_set_thread_options (gdb_thread_options options)
{
  inferior *inf = current_inferior ();
  return inf->top_target ()->supports_set_thread_options (options);
}

/* Controls if targets can report that they can/are async.  This is
   just for maintainers to use when debugging gdb.  */
bool target_async_permitted = true;

static void
set_maint_target_async (bool permitted)
{
  if (have_live_inferiors ())
    error (_("Cannot change this setting while the inferior is running."));

  target_async_permitted = permitted;
}

static bool
get_maint_target_async ()
{
  return target_async_permitted;
}

static void
show_maint_target_async (ui_file *file, int from_tty,
			 cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Controlling the inferior in "
		"asynchronous mode is %s.\n"), value);
}

/* Return true if the target operates in non-stop mode even with "set
   non-stop off".  */

static int
target_always_non_stop_p (void)
{
  return current_inferior ()->top_target ()->always_non_stop_p ();
}

/* See target.h.  */

bool
target_is_non_stop_p ()
{
  return ((non_stop
	   || target_non_stop_enabled == AUTO_BOOLEAN_TRUE
	   || (target_non_stop_enabled == AUTO_BOOLEAN_AUTO
	       && target_always_non_stop_p ()))
	  && target_can_async_p ());
}

/* See target.h.  */

bool
exists_non_stop_target ()
{
  if (target_is_non_stop_p ())
    return true;

  scoped_restore_current_thread restore_thread;

  for (inferior *inf : all_inferiors ())
    {
      switch_to_inferior_no_thread (inf);
      if (target_is_non_stop_p ())
	return true;
    }

  return false;
}

/* Controls if targets can report that they always run in non-stop
   mode.  This is just for maintainers to use when debugging gdb.  */
enum auto_boolean target_non_stop_enabled = AUTO_BOOLEAN_AUTO;

/* Set callback for maint target-non-stop setting.  */

static void
set_maint_target_non_stop (auto_boolean enabled)
{
  if (have_live_inferiors ())
    error (_("Cannot change this setting while the inferior is running."));

  target_non_stop_enabled = enabled;
}

/* Get callback for maint target-non-stop setting.  */

static auto_boolean
get_maint_target_non_stop ()
{
  return target_non_stop_enabled;
}

static void
show_maint_target_non_stop (ui_file *file, int from_tty,
			    cmd_list_element *c, const char *value)
{
  if (target_non_stop_enabled == AUTO_BOOLEAN_AUTO)
    gdb_printf (file,
		_("Whether the target is always in non-stop mode "
		  "is %s (currently %s).\n"), value,
		target_always_non_stop_p () ? "on" : "off");
  else
    gdb_printf (file,
		_("Whether the target is always in non-stop mode "
		  "is %s.\n"), value);
}

/* Temporary copies of permission settings.  */

static bool may_write_registers_1 = true;
static bool may_write_memory_1 = true;
static bool may_insert_breakpoints_1 = true;
static bool may_insert_tracepoints_1 = true;
static bool may_insert_fast_tracepoints_1 = true;
static bool may_stop_1 = true;

/* Make the user-set values match the real values again.  */

void
update_target_permissions (void)
{
  may_write_registers_1 = may_write_registers;
  may_write_memory_1 = may_write_memory;
  may_insert_breakpoints_1 = may_insert_breakpoints;
  may_insert_tracepoints_1 = may_insert_tracepoints;
  may_insert_fast_tracepoints_1 = may_insert_fast_tracepoints;
  may_stop_1 = may_stop;
}

/* The one function handles (most of) the permission flags in the same
   way.  */

static void
set_target_permissions (const char *args, int from_tty,
			struct cmd_list_element *c)
{
  if (target_has_execution ())
    {
      update_target_permissions ();
      error (_("Cannot change this setting while the inferior is running."));
    }

  /* Make the real values match the user-changed values.  */
  may_insert_breakpoints = may_insert_breakpoints_1;
  may_insert_tracepoints = may_insert_tracepoints_1;
  may_insert_fast_tracepoints = may_insert_fast_tracepoints_1;
  may_stop = may_stop_1;
  update_observer_mode ();
}

/* Set some permissions independently of observer mode.  */

static void
set_write_memory_registers_permission (const char *args, int from_tty,
				       struct cmd_list_element *c)
{
  /* Make the real values match the user-changed values.  */
  may_write_memory = may_write_memory_1;
  may_write_registers = may_write_registers_1;
  update_observer_mode ();
}

void _initialize_target ();

void
_initialize_target ()
{
  the_debug_target = new debug_target ();

  add_info ("target", info_target_command, targ_desc);
  add_info ("files", info_target_command, targ_desc);

  add_setshow_zuinteger_cmd ("target", class_maintenance, &targetdebug, _("\
Set target debugging."), _("\
Show target debugging."), _("\
When non-zero, target debugging is enabled.  Higher numbers are more\n\
verbose."),
			     set_targetdebug,
			     show_targetdebug,
			     &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("trust-readonly-sections", class_support,
			   &trust_readonly, _("\
Set mode for reading from readonly sections."), _("\
Show mode for reading from readonly sections."), _("\
When this mode is on, memory reads from readonly sections (such as .text)\n\
will be read from the object file instead of from the target.  This will\n\
result in significant performance improvement for remote targets."),
			   NULL,
			   show_trust_readonly,
			   &setlist, &showlist);

  add_com ("monitor", class_obscure, do_monitor_command,
	   _("Send a command to the remote monitor (remote targets only)."));

  add_cmd ("target-stack", class_maintenance, maintenance_print_target_stack,
	   _("Print the name of each layer of the internal target stack."),
	   &maintenanceprintlist);

  add_setshow_boolean_cmd ("target-async", no_class,
			   _("\
Set whether gdb controls the inferior in asynchronous mode."), _("\
Show whether gdb controls the inferior in asynchronous mode."), _("\
Tells gdb whether to control the inferior in asynchronous mode."),
			   set_maint_target_async,
			   get_maint_target_async,
			   show_maint_target_async,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);

  add_setshow_auto_boolean_cmd ("target-non-stop", no_class,
				_("\
Set whether gdb always controls the inferior in non-stop mode."), _("\
Show whether gdb always controls the inferior in non-stop mode."), _("\
Tells gdb whether to control the inferior in non-stop mode."),
			   set_maint_target_non_stop,
			   get_maint_target_non_stop,
			   show_maint_target_non_stop,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);

  add_setshow_boolean_cmd ("may-write-registers", class_support,
			   &may_write_registers_1, _("\
Set permission to write into registers."), _("\
Show permission to write into registers."), _("\
When this permission is on, GDB may write into the target's registers.\n\
Otherwise, any sort of write attempt will result in an error."),
			   set_write_memory_registers_permission, NULL,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("may-write-memory", class_support,
			   &may_write_memory_1, _("\
Set permission to write into target memory."), _("\
Show permission to write into target memory."), _("\
When this permission is on, GDB may write into the target's memory.\n\
Otherwise, any sort of write attempt will result in an error."),
			   set_write_memory_registers_permission, NULL,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("may-insert-breakpoints", class_support,
			   &may_insert_breakpoints_1, _("\
Set permission to insert breakpoints in the target."), _("\
Show permission to insert breakpoints in the target."), _("\
When this permission is on, GDB may insert breakpoints in the program.\n\
Otherwise, any sort of insertion attempt will result in an error."),
			   set_target_permissions, NULL,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("may-insert-tracepoints", class_support,
			   &may_insert_tracepoints_1, _("\
Set permission to insert tracepoints in the target."), _("\
Show permission to insert tracepoints in the target."), _("\
When this permission is on, GDB may insert tracepoints in the program.\n\
Otherwise, any sort of insertion attempt will result in an error."),
			   set_target_permissions, NULL,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("may-insert-fast-tracepoints", class_support,
			   &may_insert_fast_tracepoints_1, _("\
Set permission to insert fast tracepoints in the target."), _("\
Show permission to insert fast tracepoints in the target."), _("\
When this permission is on, GDB may insert fast tracepoints.\n\
Otherwise, any sort of insertion attempt will result in an error."),
			   set_target_permissions, NULL,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("may-interrupt", class_support,
			   &may_stop_1, _("\
Set permission to interrupt or signal the target."), _("\
Show permission to interrupt or signal the target."), _("\
When this permission is on, GDB may interrupt/stop the target's execution.\n\
Otherwise, any attempt to interrupt or stop will be ignored."),
			   set_target_permissions, NULL,
			   &setlist, &showlist);

  add_com ("flash-erase", no_class, flash_erase_command,
	   _("Erase all flash memory regions."));

  add_setshow_boolean_cmd ("auto-connect-native-target", class_support,
			   &auto_connect_native_target, _("\
Set whether GDB may automatically connect to the native target."), _("\
Show whether GDB may automatically connect to the native target."), _("\
When on, and GDB is not connected to a target yet, GDB\n\
attempts \"run\" and other commands with the native target."),
			   NULL, show_auto_connect_native_target,
			   &setlist, &showlist);
}
