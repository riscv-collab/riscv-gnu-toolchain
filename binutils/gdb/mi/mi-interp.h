/* MI Interpreter Definitions and Commands for GDB, the GNU debugger.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef MI_MI_INTERP_H
#define MI_MI_INTERP_H

#include "interps.h"

struct mi_console_file;

/* An MI interpreter.  */

class mi_interp final : public interp
{
public:
  mi_interp (const char *name)
    : interp (name)
  {}

  void init (bool top_level) override;
  void resume () override;
  void suspend () override;
  void exec (const char *command_str) override;
  ui_out *interp_ui_out () override;
  void set_logging (ui_file_up logfile, bool logging_redirect,
		    bool debug_redirect) override;
  void pre_command_loop () override;

  void on_signal_received (gdb_signal sig) override;
  void on_signal_exited (gdb_signal sig) override;
  void on_normal_stop (struct bpstat *bs, int print_frame) override;
  void on_exited (int status) override;
  void on_no_history () override;
  void on_sync_execution_done () override;
  void on_command_error () override;
  void on_user_selected_context_changed (user_selected_what selection) override;
  void on_new_thread (thread_info *t) override;
  void on_thread_exited (thread_info *t, std::optional<ULONGEST> exit_code,
			 int silent) override;
  void on_inferior_added (inferior *inf) override;
  void on_inferior_appeared (inferior *inf) override;
  void on_inferior_disappeared (inferior *inf) override;
  void on_inferior_removed (inferior *inf) override;
  void on_record_changed (inferior *inf, int started, const char *method,
			  const char *format) override;
  void on_target_resumed (ptid_t ptid) override;
  void on_solib_loaded (const shobj &so) override;
  void on_solib_unloaded (const shobj &so) override;
  void on_about_to_proceed () override;
  void on_traceframe_changed (int tfnum, int tpnum) override;
  void on_tsv_created (const trace_state_variable *tsv) override;
  void on_tsv_deleted (const trace_state_variable *tsv) override;
  void on_tsv_modified (const trace_state_variable *tsv) override;
  void on_breakpoint_created (breakpoint *b) override;
  void on_breakpoint_deleted (breakpoint *b) override;
  void on_breakpoint_modified (breakpoint *b) override;
  void on_param_changed (const char *param, const char *value) override;
  void on_memory_changed (inferior *inf, CORE_ADDR addr, ssize_t len,
			  const bfd_byte *data) override;

  /* MI's output channels */
  mi_console_file *out;
  mi_console_file *err;
  mi_console_file *log;
  mi_console_file *targ;
  mi_console_file *event_channel;

  /* Raw console output.  */
  struct ui_file *raw_stdout;

  /* Save the original value of raw_stdout here when logging, and the
     file which we need to delete, so we can restore correctly when
     done.  */
  struct ui_file *saved_raw_stdout;
  ui_file_up logfile_holder;
  ui_file_up stdout_holder;

  /* MI's builder.  */
  struct ui_out *mi_uiout;

  /* MI's CLI builder (wraps OUT).  */
  struct ui_out *cli_uiout;

  int running_result_record_printed = 1;

  /* Flag indicating that the target has proceeded since the last
     command was issued.  */
  int mi_proceeded;

  const char *current_token;
};

/* Output the shared object attributes to UIOUT.  */

void mi_output_solib_attribs (ui_out *uiout, const shobj &solib);

/* Returns the INTERP's data cast as mi_interp if INTERP is an MI, and
   returns NULL otherwise.  */

static inline struct mi_interp *
as_mi_interp (struct interp *interp)
{
  return dynamic_cast<mi_interp *> (interp);
}

#endif /* MI_MI_INTERP_H */
