/* CLI Definitions for GDB, the GNU debugger.

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#ifndef CLI_CLI_INTERP_H
#define CLI_CLI_INTERP_H

#include "interps.h"

/* A console-like interpreter.  Implements functionality common to the
   CLI and the TUI.  */
class cli_interp_base : public interp
{
public:
  explicit cli_interp_base (const char *name);
  virtual ~cli_interp_base () = 0;

  void set_logging (ui_file_up logfile, bool logging_redirect,
		    bool debug_redirect) override;
  void pre_command_loop () override;
  bool supports_command_editing () override;

  void on_signal_received (gdb_signal sig) override;
  void on_signal_exited (gdb_signal sig) override;
  void on_normal_stop (bpstat *bs, int print_frame) override;
  void on_exited (int status) override;
  void on_no_history () override;
  void on_sync_execution_done () override;
  void on_command_error () override;
  void on_user_selected_context_changed (user_selected_what selection) override;

private:
  struct saved_output_files
  {
    /* Saved gdb_stdout, gdb_stderr, etc.  */
    ui_file *out;
    ui_file *err;
    ui_file *log;
    ui_file *targ;
    ui_file *targerr;
    /* When redirecting, some or all of these may be non-null
       depending on the logging mode.  */
    ui_file_up stdout_holder;
    ui_file_up stderr_holder;
    ui_file_up stdlog_holder;
    ui_file_up logfile_holder;
  };

  /* These hold the pushed copies of the gdb output files.  If NULL
     then nothing has yet been pushed.  */
  std::unique_ptr<saved_output_files> m_saved_output;
};

/* Returns true if the current stop should be printed to
   CONSOLE_INTERP.  */
extern int should_print_stop_to_console (struct interp *interp,
					 struct thread_info *tp);

#endif /* CLI_CLI_INTERP_H */
