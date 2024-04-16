/* MI Command Set for GDB, the GNU debugger.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions (a Red Hat company).

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
#include "mi-cmds.h"
#include "mi-main.h"
#include "mi-parse.h"
#include <map>
#include <string>

/* MI command table (built at run time). */

static std::map<std::string, mi_command_up> mi_cmd_table;

/* MI command with a pure MI implementation.  */

struct mi_command_mi : public mi_command
{
  /* Constructor.  For NAME and SUPPRESS_NOTIFICATION see mi_command
     constructor, FUNC is the function called from do_invoke, which
     implements this MI command.  */
  mi_command_mi (const char *name, mi_cmd_argv_ftype func,
		 int *suppress_notification)
    : mi_command (name, suppress_notification),
      m_argv_function (func)
  {
    gdb_assert (func != nullptr);
  }

  /* Called when this MI command has been invoked, calls m_argv_function
     with arguments contained within PARSE.  */
  void invoke (struct mi_parse *parse) const override
  {
    parse->parse_argv ();

    if (parse->argv == nullptr)
      error (_("Problem parsing arguments: %s %s"), parse->command.get (),
	     parse->args ());

    this->m_argv_function (parse->command.get (), parse->argv, parse->argc);
  }

private:

  /* The function that implements this MI command.  */
  mi_cmd_argv_ftype *m_argv_function;
};

/* MI command implemented on top of a CLI command.  */

struct mi_command_cli : public mi_command
{
  /* Constructor.  For NAME and SUPPRESS_NOTIFICATION see mi_command
     constructor, CLI_NAME is the name of a CLI command that should be
     invoked to implement this MI command.  If ARGS_P is true then any
     arguments from entered by the user as part of the MI command line are
     forwarded to CLI_NAME as its argument string, otherwise, if ARGS_P is
     false, nullptr is send to CLI_NAME as its argument string.  */
  mi_command_cli (const char *name, const char *cli_name, bool args_p,
		  int *suppress_notification)
    : mi_command (name, suppress_notification),
      m_cli_name (cli_name),
      m_args_p (args_p)
  { /* Nothing.  */ }

  /* Called when this MI command has been invoked, calls the m_cli_name
     CLI function.  In m_args_p is true then the argument string from
     within PARSE is passed through to the CLI function, otherwise nullptr
     is passed through to the CLI function as its argument string.  */
  void invoke (struct mi_parse *parse) const override
  {
    const char *args = m_args_p ? parse->args () : nullptr;
    mi_execute_cli_command (m_cli_name, m_args_p, args);
  }

private:

  /* The name of the CLI command to execute.  */
  const char *m_cli_name;

  /* Should we be passing an argument string to the m_cli_name function?  */
  bool m_args_p;
};

/* See mi-cmds.h.  */

bool
insert_mi_cmd_entry (mi_command_up command)
{
  gdb_assert (command != nullptr);

  const std::string &name = command->name ();

  if (mi_cmd_table.find (name) != mi_cmd_table.end ())
    return false;

  mi_cmd_table[name] = std::move (command);
  return true;
}

/* See mi-cmds.h.  */

bool
remove_mi_cmd_entry (const std::string &name)
{
  if (mi_cmd_table.find (name) == mi_cmd_table.end ())
    return false;

  mi_cmd_table.erase (name);
  return true;
}

/* See mi-cmds.h.  */

void
remove_mi_cmd_entries (remove_mi_cmd_entries_ftype callback)
{
  for (auto it = mi_cmd_table.cbegin (); it != mi_cmd_table.cend (); )
    {
      if (callback (it->second.get ()))
	it = mi_cmd_table.erase (it);
      else
	++it;
    }
}

/* Create and register a new MI command with an MI specific implementation.
   NAME must name an MI command that does not already exist, otherwise an
   assertion will trigger.  */

static void
add_mi_cmd_mi (const char *name, mi_cmd_argv_ftype function,
	       int *suppress_notification = nullptr)
{
  mi_command_up command (new mi_command_mi (name, function,
					    suppress_notification));

  bool success = insert_mi_cmd_entry (std::move (command));
  gdb_assert (success);
}

/* Create and register a new MI command implemented on top of a CLI
   command.  NAME must name an MI command that does not already exist,
   otherwise an assertion will trigger.  */

static void
add_mi_cmd_cli (const char *name, const char *cli_name, int args_p,
		int *suppress_notification = nullptr)
{
  mi_command_up command (new mi_command_cli (name, cli_name, args_p != 0,
					     suppress_notification));

  bool success = insert_mi_cmd_entry (std::move (command));
  gdb_assert (success);
}

/* See mi-cmds.h.  */

mi_command::mi_command (const char *name, int *suppress_notification)
  : m_name (name),
    m_suppress_notification (suppress_notification)
{
  gdb_assert (m_name != nullptr && m_name[0] != '\0');
}

/* See mi-cmds.h.  */

std::optional<scoped_restore_tmpl<int>>
mi_command::do_suppress_notification () const
{
  if (m_suppress_notification != nullptr)
    return scoped_restore_tmpl<int> (m_suppress_notification, 1);
  else
    return {};
}

/* Initialize the available MI commands.  */

static void
add_builtin_mi_commands ()
{
  add_mi_cmd_mi ("ada-task-info", mi_cmd_ada_task_info);
  add_mi_cmd_mi ("add-inferior", mi_cmd_add_inferior);
  add_mi_cmd_cli ("break-after", "ignore", 1,
		  &mi_suppress_notification.breakpoint);
  add_mi_cmd_mi ("break-condition",mi_cmd_break_condition,
		  &mi_suppress_notification.breakpoint);
  add_mi_cmd_mi ("break-commands", mi_cmd_break_commands,
		 &mi_suppress_notification.breakpoint);
  add_mi_cmd_cli ("break-delete", "delete breakpoint", 1,
		  &mi_suppress_notification.breakpoint);
  add_mi_cmd_cli ("break-disable", "disable breakpoint", 1,
		  &mi_suppress_notification.breakpoint);
  add_mi_cmd_cli ("break-enable", "enable breakpoint", 1,
		  &mi_suppress_notification.breakpoint);
  add_mi_cmd_cli ("break-info", "info break", 1);
  add_mi_cmd_mi ("break-insert", mi_cmd_break_insert,
		 &mi_suppress_notification.breakpoint);
  add_mi_cmd_mi ("dprintf-insert", mi_cmd_dprintf_insert,
		 &mi_suppress_notification.breakpoint);
  add_mi_cmd_cli ("break-list", "info break", 0);
  add_mi_cmd_mi ("break-passcount", mi_cmd_break_passcount,
		 &mi_suppress_notification.breakpoint);
  add_mi_cmd_mi ("break-watch", mi_cmd_break_watch,
		 &mi_suppress_notification.breakpoint);
  add_mi_cmd_mi ("catch-assert", mi_cmd_catch_assert,
		 &mi_suppress_notification.breakpoint);
  add_mi_cmd_mi ("catch-exception", mi_cmd_catch_exception,
		 &mi_suppress_notification.breakpoint);
  add_mi_cmd_mi ("catch-handlers", mi_cmd_catch_handlers,
		 &mi_suppress_notification.breakpoint);
  add_mi_cmd_mi ("catch-load", mi_cmd_catch_load,
		 &mi_suppress_notification.breakpoint);
  add_mi_cmd_mi ("catch-unload", mi_cmd_catch_unload,
		 &mi_suppress_notification.breakpoint);
  add_mi_cmd_mi ("catch-throw", mi_cmd_catch_throw,
		 &mi_suppress_notification.breakpoint),
  add_mi_cmd_mi ("catch-rethrow", mi_cmd_catch_rethrow,
		 &mi_suppress_notification.breakpoint),
  add_mi_cmd_mi ("catch-catch", mi_cmd_catch_catch,
		 &mi_suppress_notification.breakpoint),
  add_mi_cmd_mi ("complete", mi_cmd_complete);
  add_mi_cmd_mi ("data-disassemble", mi_cmd_disassemble);
  add_mi_cmd_mi ("data-evaluate-expression", mi_cmd_data_evaluate_expression);
  add_mi_cmd_mi ("data-list-changed-registers",
		 mi_cmd_data_list_changed_registers);
  add_mi_cmd_mi ("data-list-register-names", mi_cmd_data_list_register_names);
  add_mi_cmd_mi ("data-list-register-values",
		 mi_cmd_data_list_register_values);
  add_mi_cmd_mi ("data-read-memory", mi_cmd_data_read_memory);
  add_mi_cmd_mi ("data-read-memory-bytes", mi_cmd_data_read_memory_bytes);
  add_mi_cmd_mi ("data-write-memory", mi_cmd_data_write_memory,
		 &mi_suppress_notification.memory);
  add_mi_cmd_mi ("data-write-memory-bytes", mi_cmd_data_write_memory_bytes,
		 &mi_suppress_notification.memory);
  add_mi_cmd_mi ("data-write-register-values",
		 mi_cmd_data_write_register_values);
  add_mi_cmd_mi ("enable-timings", mi_cmd_enable_timings);
  add_mi_cmd_mi ("enable-pretty-printing", mi_cmd_enable_pretty_printing);
  add_mi_cmd_mi ("enable-frame-filters", mi_cmd_enable_frame_filters);
  add_mi_cmd_mi ("environment-cd", mi_cmd_env_cd);
  add_mi_cmd_mi ("environment-directory", mi_cmd_env_dir);
  add_mi_cmd_mi ("environment-path", mi_cmd_env_path);
  add_mi_cmd_mi ("environment-pwd", mi_cmd_env_pwd);
  add_mi_cmd_cli ("exec-arguments", "set args", 1,
		  &mi_suppress_notification.cmd_param_changed);
  add_mi_cmd_mi ("exec-continue", mi_cmd_exec_continue);
  add_mi_cmd_mi ("exec-finish", mi_cmd_exec_finish);
  add_mi_cmd_mi ("exec-jump", mi_cmd_exec_jump);
  add_mi_cmd_mi ("exec-interrupt", mi_cmd_exec_interrupt);
  add_mi_cmd_mi ("exec-next", mi_cmd_exec_next);
  add_mi_cmd_mi ("exec-next-instruction", mi_cmd_exec_next_instruction);
  add_mi_cmd_mi ("exec-return", mi_cmd_exec_return);
  add_mi_cmd_mi ("exec-run", mi_cmd_exec_run);
  add_mi_cmd_mi ("exec-step", mi_cmd_exec_step);
  add_mi_cmd_mi ("exec-step-instruction", mi_cmd_exec_step_instruction);
  add_mi_cmd_cli ("exec-until", "until", 1);
  add_mi_cmd_cli ("file-exec-and-symbols", "file", 1);
  add_mi_cmd_cli ("file-exec-file", "exec-file", 1);
  add_mi_cmd_mi ("file-list-exec-source-file",
		 mi_cmd_file_list_exec_source_file);
  add_mi_cmd_mi ("file-list-exec-source-files",
		 mi_cmd_file_list_exec_source_files);
  add_mi_cmd_mi ("file-list-shared-libraries",
     mi_cmd_file_list_shared_libraries),
  add_mi_cmd_cli ("file-symbol-file", "symbol-file", 1);
  add_mi_cmd_mi ("fix-breakpoint-script-output",
		 mi_cmd_fix_breakpoint_script_output),
  add_mi_cmd_mi ("fix-multi-location-breakpoint-output",
		 mi_cmd_fix_multi_location_breakpoint_output),
  add_mi_cmd_mi ("gdb-exit", mi_cmd_gdb_exit);
  add_mi_cmd_cli ("gdb-set", "set", 1,
		  &mi_suppress_notification.cmd_param_changed);
  add_mi_cmd_cli ("gdb-show", "show", 1);
  add_mi_cmd_cli ("gdb-version", "show version", 0);
  add_mi_cmd_mi ("inferior-tty-set", mi_cmd_inferior_tty_set);
  add_mi_cmd_mi ("inferior-tty-show", mi_cmd_inferior_tty_show);
  add_mi_cmd_mi ("info-ada-exceptions", mi_cmd_info_ada_exceptions);
  add_mi_cmd_mi ("info-gdb-mi-command", mi_cmd_info_gdb_mi_command);
  add_mi_cmd_mi ("info-os", mi_cmd_info_os);
  add_mi_cmd_mi ("interpreter-exec", mi_cmd_interpreter_exec);
  add_mi_cmd_mi ("list-features", mi_cmd_list_features);
  add_mi_cmd_mi ("list-target-features", mi_cmd_list_target_features);
  add_mi_cmd_mi ("list-thread-groups", mi_cmd_list_thread_groups);
  add_mi_cmd_mi ("remove-inferior", mi_cmd_remove_inferior);
  add_mi_cmd_mi ("stack-info-depth", mi_cmd_stack_info_depth);
  add_mi_cmd_mi ("stack-info-frame", mi_cmd_stack_info_frame);
  add_mi_cmd_mi ("stack-list-arguments", mi_cmd_stack_list_args);
  add_mi_cmd_mi ("stack-list-frames", mi_cmd_stack_list_frames);
  add_mi_cmd_mi ("stack-list-locals", mi_cmd_stack_list_locals);
  add_mi_cmd_mi ("stack-list-variables", mi_cmd_stack_list_variables);
  add_mi_cmd_mi ("stack-select-frame", mi_cmd_stack_select_frame,
		 &mi_suppress_notification.user_selected_context);
  add_mi_cmd_mi ("symbol-list-lines", mi_cmd_symbol_list_lines);
  add_mi_cmd_mi ("symbol-info-functions", mi_cmd_symbol_info_functions);
  add_mi_cmd_mi ("symbol-info-variables", mi_cmd_symbol_info_variables);
  add_mi_cmd_mi ("symbol-info-types", mi_cmd_symbol_info_types);
  add_mi_cmd_mi ("symbol-info-modules", mi_cmd_symbol_info_modules);
  add_mi_cmd_mi ("symbol-info-module-functions",
		 mi_cmd_symbol_info_module_functions);
  add_mi_cmd_mi ("symbol-info-module-variables",
		 mi_cmd_symbol_info_module_variables);
  add_mi_cmd_cli ("target-attach", "attach", 1);
  add_mi_cmd_mi ("target-detach", mi_cmd_target_detach);
  add_mi_cmd_cli ("target-disconnect", "disconnect", 0);
  add_mi_cmd_cli ("target-download", "load", 1);
  add_mi_cmd_mi ("target-file-delete", mi_cmd_target_file_delete);
  add_mi_cmd_mi ("target-file-get", mi_cmd_target_file_get);
  add_mi_cmd_mi ("target-file-put", mi_cmd_target_file_put);
  add_mi_cmd_mi ("target-flash-erase", mi_cmd_target_flash_erase);
  add_mi_cmd_cli ("target-select", "target", 1);
  add_mi_cmd_mi ("thread-info", mi_cmd_thread_info);
  add_mi_cmd_mi ("thread-list-ids", mi_cmd_thread_list_ids);
  add_mi_cmd_mi ("thread-select", mi_cmd_thread_select,
		 &mi_suppress_notification.user_selected_context);
  add_mi_cmd_mi ("trace-define-variable", mi_cmd_trace_define_variable);
  add_mi_cmd_mi ("trace-find", mi_cmd_trace_find,
		 &mi_suppress_notification.traceframe);
  add_mi_cmd_mi ("trace-frame-collected", mi_cmd_trace_frame_collected);
  add_mi_cmd_mi ("trace-list-variables", mi_cmd_trace_list_variables);
  add_mi_cmd_mi ("trace-save", mi_cmd_trace_save);
  add_mi_cmd_mi ("trace-start", mi_cmd_trace_start);
  add_mi_cmd_mi ("trace-status", mi_cmd_trace_status);
  add_mi_cmd_mi ("trace-stop", mi_cmd_trace_stop);
  add_mi_cmd_mi ("var-assign", mi_cmd_var_assign);
  add_mi_cmd_mi ("var-create", mi_cmd_var_create);
  add_mi_cmd_mi ("var-delete", mi_cmd_var_delete);
  add_mi_cmd_mi ("var-evaluate-expression", mi_cmd_var_evaluate_expression);
  add_mi_cmd_mi ("var-info-path-expression", mi_cmd_var_info_path_expression);
  add_mi_cmd_mi ("var-info-expression", mi_cmd_var_info_expression);
  add_mi_cmd_mi ("var-info-num-children", mi_cmd_var_info_num_children);
  add_mi_cmd_mi ("var-info-type", mi_cmd_var_info_type);
  add_mi_cmd_mi ("var-list-children", mi_cmd_var_list_children);
  add_mi_cmd_mi ("var-set-format", mi_cmd_var_set_format);
  add_mi_cmd_mi ("var-set-frozen", mi_cmd_var_set_frozen);
  add_mi_cmd_mi ("var-set-update-range", mi_cmd_var_set_update_range);
  add_mi_cmd_mi ("var-set-visualizer", mi_cmd_var_set_visualizer);
  add_mi_cmd_mi ("var-show-attributes", mi_cmd_var_show_attributes);
  add_mi_cmd_mi ("var-show-format", mi_cmd_var_show_format);
  add_mi_cmd_mi ("var-update", mi_cmd_var_update);
}

/* See mi-cmds.h.  */

mi_command *
mi_cmd_lookup (const char *command)
{
  gdb_assert (command != nullptr);

  auto it = mi_cmd_table.find (command);
  if (it == mi_cmd_table.end ())
    return nullptr;
  return it->second.get ();
}

void _initialize_mi_cmds ();
void
_initialize_mi_cmds ()
{
  add_builtin_mi_commands ();
}
