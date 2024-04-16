/* THIS FILE IS GENERATED -*- buffer-read-only: t -*- */
/* vi:set ro: */

/* Boilerplate target methods for GDB

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

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

/* To regenerate this file, run:
   ./make-target-delegates.py
*/

struct dummy_target : public target_ops
{
  const target_info &info () const override;

  strata stratum () const override;

  void post_attach (int arg0) override;
  void detach (inferior *arg0, int arg1) override;
  void disconnect (const char *arg0, int arg1) override;
  void resume (ptid_t arg0, int arg1, enum gdb_signal arg2) override;
  void commit_resumed () override;
  ptid_t wait (ptid_t arg0, struct target_waitstatus *arg1, target_wait_flags arg2) override;
  void fetch_registers (struct regcache *arg0, int arg1) override;
  void store_registers (struct regcache *arg0, int arg1) override;
  void prepare_to_store (struct regcache *arg0) override;
  void files_info () override;
  int insert_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1) override;
  int remove_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1, enum remove_bp_reason arg2) override;
  bool stopped_by_sw_breakpoint () override;
  bool supports_stopped_by_sw_breakpoint () override;
  bool stopped_by_hw_breakpoint () override;
  bool supports_stopped_by_hw_breakpoint () override;
  int can_use_hw_breakpoint (enum bptype arg0, int arg1, int arg2) override;
  int ranged_break_num_registers () override;
  int insert_hw_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1) override;
  int remove_hw_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1) override;
  int remove_watchpoint (CORE_ADDR arg0, int arg1, enum target_hw_bp_type arg2, struct expression *arg3) override;
  int insert_watchpoint (CORE_ADDR arg0, int arg1, enum target_hw_bp_type arg2, struct expression *arg3) override;
  int insert_mask_watchpoint (CORE_ADDR arg0, CORE_ADDR arg1, enum target_hw_bp_type arg2) override;
  int remove_mask_watchpoint (CORE_ADDR arg0, CORE_ADDR arg1, enum target_hw_bp_type arg2) override;
  bool stopped_by_watchpoint () override;
  bool have_steppable_watchpoint () override;
  bool stopped_data_address (CORE_ADDR *arg0) override;
  bool watchpoint_addr_within_range (CORE_ADDR arg0, CORE_ADDR arg1, int arg2) override;
  int region_ok_for_hw_watchpoint (CORE_ADDR arg0, int arg1) override;
  bool can_accel_watchpoint_condition (CORE_ADDR arg0, int arg1, int arg2, struct expression *arg3) override;
  int masked_watch_num_registers (CORE_ADDR arg0, CORE_ADDR arg1) override;
  int can_do_single_step () override;
  bool supports_terminal_ours () override;
  void terminal_init () override;
  void terminal_inferior () override;
  void terminal_save_inferior () override;
  void terminal_ours_for_output () override;
  void terminal_ours () override;
  void terminal_info (const char *arg0, int arg1) override;
  void kill () override;
  void load (const char *arg0, int arg1) override;
  int insert_fork_catchpoint (int arg0) override;
  int remove_fork_catchpoint (int arg0) override;
  int insert_vfork_catchpoint (int arg0) override;
  int remove_vfork_catchpoint (int arg0) override;
  void follow_fork (inferior *arg0, ptid_t arg1, target_waitkind arg2, bool arg3, bool arg4) override;
  void follow_clone (ptid_t arg0) override;
  int insert_exec_catchpoint (int arg0) override;
  int remove_exec_catchpoint (int arg0) override;
  void follow_exec (inferior *arg0, ptid_t arg1, const char *arg2) override;
  int set_syscall_catchpoint (int arg0, bool arg1, int arg2, gdb::array_view<const int> arg3) override;
  void mourn_inferior () override;
  void pass_signals (gdb::array_view<const unsigned char> arg0) override;
  void program_signals (gdb::array_view<const unsigned char> arg0) override;
  bool thread_alive (ptid_t arg0) override;
  void update_thread_list () override;
  std::string pid_to_str (ptid_t arg0) override;
  const char *extra_thread_info (thread_info *arg0) override;
  const char *thread_name (thread_info *arg0) override;
  thread_info *thread_handle_to_thread_info (const gdb_byte *arg0, int arg1, inferior *arg2) override;
  gdb::array_view<const_gdb_byte> thread_info_to_thread_handle (struct thread_info *arg0) override;
  void stop (ptid_t arg0) override;
  void interrupt () override;
  void pass_ctrlc () override;
  void rcmd (const char *arg0, struct ui_file *arg1) override;
  const char *pid_to_exec_file (int arg0) override;
  void log_command (const char *arg0) override;
  const std::vector<target_section> *get_section_table () override;
  thread_control_capabilities get_thread_control_capabilities () override;
  bool attach_no_wait () override;
  bool can_async_p () override;
  bool is_async_p () override;
  void async (bool arg0) override;
  int async_wait_fd () override;
  bool has_pending_events () override;
  void thread_events (int arg0) override;
  bool supports_set_thread_options (gdb_thread_options arg0) override;
  bool supports_non_stop () override;
  bool always_non_stop_p () override;
  int find_memory_regions (find_memory_region_ftype arg0, void *arg1) override;
  gdb::unique_xmalloc_ptr<char> make_corefile_notes (bfd *arg0, int *arg1) override;
  gdb_byte *get_bookmark (const char *arg0, int arg1) override;
  void goto_bookmark (const gdb_byte *arg0, int arg1) override;
  CORE_ADDR get_thread_local_address (ptid_t arg0, CORE_ADDR arg1, CORE_ADDR arg2) override;
  enum target_xfer_status xfer_partial (enum target_object arg0, const char *arg1, gdb_byte *arg2, const gdb_byte *arg3, ULONGEST arg4, ULONGEST arg5, ULONGEST *arg6) override;
  ULONGEST get_memory_xfer_limit () override;
  std::vector<mem_region> memory_map () override;
  void flash_erase (ULONGEST arg0, LONGEST arg1) override;
  void flash_done () override;
  const struct target_desc *read_description () override;
  ptid_t get_ada_task_ptid (long arg0, ULONGEST arg1) override;
  int auxv_parse (const gdb_byte **arg0, const gdb_byte *arg1, CORE_ADDR *arg2, CORE_ADDR *arg3) override;
  int search_memory (CORE_ADDR arg0, ULONGEST arg1, const gdb_byte *arg2, ULONGEST arg3, CORE_ADDR *arg4) override;
  bool can_execute_reverse () override;
  enum exec_direction_kind execution_direction () override;
  bool supports_multi_process () override;
  bool supports_enable_disable_tracepoint () override;
  bool supports_disable_randomization () override;
  bool supports_string_tracing () override;
  bool supports_evaluation_of_breakpoint_conditions () override;
  bool supports_dumpcore () override;
  void dumpcore (const char *arg0) override;
  bool can_run_breakpoint_commands () override;
  struct gdbarch *thread_architecture (ptid_t arg0) override;
  bool filesystem_is_local () override;
  void trace_init () override;
  void download_tracepoint (struct bp_location *arg0) override;
  bool can_download_tracepoint () override;
  void download_trace_state_variable (const trace_state_variable &arg0) override;
  void enable_tracepoint (struct bp_location *arg0) override;
  void disable_tracepoint (struct bp_location *arg0) override;
  void trace_set_readonly_regions () override;
  void trace_start () override;
  int get_trace_status (struct trace_status *arg0) override;
  void get_tracepoint_status (tracepoint *arg0, struct uploaded_tp *arg1) override;
  void trace_stop () override;
  int trace_find (enum trace_find_type arg0, int arg1, CORE_ADDR arg2, CORE_ADDR arg3, int *arg4) override;
  bool get_trace_state_variable_value (int arg0, LONGEST *arg1) override;
  int save_trace_data (const char *arg0) override;
  int upload_tracepoints (struct uploaded_tp **arg0) override;
  int upload_trace_state_variables (struct uploaded_tsv **arg0) override;
  LONGEST get_raw_trace_data (gdb_byte *arg0, ULONGEST arg1, LONGEST arg2) override;
  int get_min_fast_tracepoint_insn_len () override;
  void set_disconnected_tracing (int arg0) override;
  void set_circular_trace_buffer (int arg0) override;
  void set_trace_buffer_size (LONGEST arg0) override;
  bool set_trace_notes (const char *arg0, const char *arg1, const char *arg2) override;
  int core_of_thread (ptid_t arg0) override;
  int verify_memory (const gdb_byte *arg0, CORE_ADDR arg1, ULONGEST arg2) override;
  bool get_tib_address (ptid_t arg0, CORE_ADDR *arg1) override;
  void set_permissions () override;
  bool static_tracepoint_marker_at (CORE_ADDR arg0, static_tracepoint_marker *arg1) override;
  std::vector<static_tracepoint_marker> static_tracepoint_markers_by_strid (const char *arg0) override;
  traceframe_info_up traceframe_info () override;
  bool use_agent (bool arg0) override;
  bool can_use_agent () override;
  struct btrace_target_info *enable_btrace (thread_info *arg0, const struct btrace_config *arg1) override;
  void disable_btrace (struct btrace_target_info *arg0) override;
  void teardown_btrace (struct btrace_target_info *arg0) override;
  enum btrace_error read_btrace (struct btrace_data *arg0, struct btrace_target_info *arg1, enum btrace_read_type arg2) override;
  const struct btrace_config *btrace_conf (const struct btrace_target_info *arg0) override;
  enum record_method record_method (ptid_t arg0) override;
  void stop_recording () override;
  void info_record () override;
  void save_record (const char *arg0) override;
  bool supports_delete_record () override;
  void delete_record () override;
  bool record_is_replaying (ptid_t arg0) override;
  bool record_will_replay (ptid_t arg0, int arg1) override;
  void record_stop_replaying () override;
  void goto_record_begin () override;
  void goto_record_end () override;
  void goto_record (ULONGEST arg0) override;
  void insn_history (int arg0, gdb_disassembly_flags arg1) override;
  void insn_history_from (ULONGEST arg0, int arg1, gdb_disassembly_flags arg2) override;
  void insn_history_range (ULONGEST arg0, ULONGEST arg1, gdb_disassembly_flags arg2) override;
  void call_history (int arg0, record_print_flags arg1) override;
  void call_history_from (ULONGEST arg0, int arg1, record_print_flags arg2) override;
  void call_history_range (ULONGEST arg0, ULONGEST arg1, record_print_flags arg2) override;
  bool augmented_libraries_svr4_read () override;
  const struct frame_unwind *get_unwinder () override;
  const struct frame_unwind *get_tailcall_unwinder () override;
  void prepare_to_generate_core () override;
  void done_generating_core () override;
  bool supports_memory_tagging () override;
  bool fetch_memtags (CORE_ADDR arg0, size_t arg1, gdb::byte_vector &arg2, int arg3) override;
  bool store_memtags (CORE_ADDR arg0, size_t arg1, const gdb::byte_vector &arg2, int arg3) override;
  x86_xsave_layout fetch_x86_xsave_layout () override;
};

struct debug_target : public target_ops
{
  const target_info &info () const override;

  strata stratum () const override;

  void post_attach (int arg0) override;
  void detach (inferior *arg0, int arg1) override;
  void disconnect (const char *arg0, int arg1) override;
  void resume (ptid_t arg0, int arg1, enum gdb_signal arg2) override;
  void commit_resumed () override;
  ptid_t wait (ptid_t arg0, struct target_waitstatus *arg1, target_wait_flags arg2) override;
  void fetch_registers (struct regcache *arg0, int arg1) override;
  void store_registers (struct regcache *arg0, int arg1) override;
  void prepare_to_store (struct regcache *arg0) override;
  void files_info () override;
  int insert_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1) override;
  int remove_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1, enum remove_bp_reason arg2) override;
  bool stopped_by_sw_breakpoint () override;
  bool supports_stopped_by_sw_breakpoint () override;
  bool stopped_by_hw_breakpoint () override;
  bool supports_stopped_by_hw_breakpoint () override;
  int can_use_hw_breakpoint (enum bptype arg0, int arg1, int arg2) override;
  int ranged_break_num_registers () override;
  int insert_hw_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1) override;
  int remove_hw_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1) override;
  int remove_watchpoint (CORE_ADDR arg0, int arg1, enum target_hw_bp_type arg2, struct expression *arg3) override;
  int insert_watchpoint (CORE_ADDR arg0, int arg1, enum target_hw_bp_type arg2, struct expression *arg3) override;
  int insert_mask_watchpoint (CORE_ADDR arg0, CORE_ADDR arg1, enum target_hw_bp_type arg2) override;
  int remove_mask_watchpoint (CORE_ADDR arg0, CORE_ADDR arg1, enum target_hw_bp_type arg2) override;
  bool stopped_by_watchpoint () override;
  bool have_steppable_watchpoint () override;
  bool stopped_data_address (CORE_ADDR *arg0) override;
  bool watchpoint_addr_within_range (CORE_ADDR arg0, CORE_ADDR arg1, int arg2) override;
  int region_ok_for_hw_watchpoint (CORE_ADDR arg0, int arg1) override;
  bool can_accel_watchpoint_condition (CORE_ADDR arg0, int arg1, int arg2, struct expression *arg3) override;
  int masked_watch_num_registers (CORE_ADDR arg0, CORE_ADDR arg1) override;
  int can_do_single_step () override;
  bool supports_terminal_ours () override;
  void terminal_init () override;
  void terminal_inferior () override;
  void terminal_save_inferior () override;
  void terminal_ours_for_output () override;
  void terminal_ours () override;
  void terminal_info (const char *arg0, int arg1) override;
  void kill () override;
  void load (const char *arg0, int arg1) override;
  int insert_fork_catchpoint (int arg0) override;
  int remove_fork_catchpoint (int arg0) override;
  int insert_vfork_catchpoint (int arg0) override;
  int remove_vfork_catchpoint (int arg0) override;
  void follow_fork (inferior *arg0, ptid_t arg1, target_waitkind arg2, bool arg3, bool arg4) override;
  void follow_clone (ptid_t arg0) override;
  int insert_exec_catchpoint (int arg0) override;
  int remove_exec_catchpoint (int arg0) override;
  void follow_exec (inferior *arg0, ptid_t arg1, const char *arg2) override;
  int set_syscall_catchpoint (int arg0, bool arg1, int arg2, gdb::array_view<const int> arg3) override;
  void mourn_inferior () override;
  void pass_signals (gdb::array_view<const unsigned char> arg0) override;
  void program_signals (gdb::array_view<const unsigned char> arg0) override;
  bool thread_alive (ptid_t arg0) override;
  void update_thread_list () override;
  std::string pid_to_str (ptid_t arg0) override;
  const char *extra_thread_info (thread_info *arg0) override;
  const char *thread_name (thread_info *arg0) override;
  thread_info *thread_handle_to_thread_info (const gdb_byte *arg0, int arg1, inferior *arg2) override;
  gdb::array_view<const_gdb_byte> thread_info_to_thread_handle (struct thread_info *arg0) override;
  void stop (ptid_t arg0) override;
  void interrupt () override;
  void pass_ctrlc () override;
  void rcmd (const char *arg0, struct ui_file *arg1) override;
  const char *pid_to_exec_file (int arg0) override;
  void log_command (const char *arg0) override;
  const std::vector<target_section> *get_section_table () override;
  thread_control_capabilities get_thread_control_capabilities () override;
  bool attach_no_wait () override;
  bool can_async_p () override;
  bool is_async_p () override;
  void async (bool arg0) override;
  int async_wait_fd () override;
  bool has_pending_events () override;
  void thread_events (int arg0) override;
  bool supports_set_thread_options (gdb_thread_options arg0) override;
  bool supports_non_stop () override;
  bool always_non_stop_p () override;
  int find_memory_regions (find_memory_region_ftype arg0, void *arg1) override;
  gdb::unique_xmalloc_ptr<char> make_corefile_notes (bfd *arg0, int *arg1) override;
  gdb_byte *get_bookmark (const char *arg0, int arg1) override;
  void goto_bookmark (const gdb_byte *arg0, int arg1) override;
  CORE_ADDR get_thread_local_address (ptid_t arg0, CORE_ADDR arg1, CORE_ADDR arg2) override;
  enum target_xfer_status xfer_partial (enum target_object arg0, const char *arg1, gdb_byte *arg2, const gdb_byte *arg3, ULONGEST arg4, ULONGEST arg5, ULONGEST *arg6) override;
  ULONGEST get_memory_xfer_limit () override;
  std::vector<mem_region> memory_map () override;
  void flash_erase (ULONGEST arg0, LONGEST arg1) override;
  void flash_done () override;
  const struct target_desc *read_description () override;
  ptid_t get_ada_task_ptid (long arg0, ULONGEST arg1) override;
  int auxv_parse (const gdb_byte **arg0, const gdb_byte *arg1, CORE_ADDR *arg2, CORE_ADDR *arg3) override;
  int search_memory (CORE_ADDR arg0, ULONGEST arg1, const gdb_byte *arg2, ULONGEST arg3, CORE_ADDR *arg4) override;
  bool can_execute_reverse () override;
  enum exec_direction_kind execution_direction () override;
  bool supports_multi_process () override;
  bool supports_enable_disable_tracepoint () override;
  bool supports_disable_randomization () override;
  bool supports_string_tracing () override;
  bool supports_evaluation_of_breakpoint_conditions () override;
  bool supports_dumpcore () override;
  void dumpcore (const char *arg0) override;
  bool can_run_breakpoint_commands () override;
  struct gdbarch *thread_architecture (ptid_t arg0) override;
  bool filesystem_is_local () override;
  void trace_init () override;
  void download_tracepoint (struct bp_location *arg0) override;
  bool can_download_tracepoint () override;
  void download_trace_state_variable (const trace_state_variable &arg0) override;
  void enable_tracepoint (struct bp_location *arg0) override;
  void disable_tracepoint (struct bp_location *arg0) override;
  void trace_set_readonly_regions () override;
  void trace_start () override;
  int get_trace_status (struct trace_status *arg0) override;
  void get_tracepoint_status (tracepoint *arg0, struct uploaded_tp *arg1) override;
  void trace_stop () override;
  int trace_find (enum trace_find_type arg0, int arg1, CORE_ADDR arg2, CORE_ADDR arg3, int *arg4) override;
  bool get_trace_state_variable_value (int arg0, LONGEST *arg1) override;
  int save_trace_data (const char *arg0) override;
  int upload_tracepoints (struct uploaded_tp **arg0) override;
  int upload_trace_state_variables (struct uploaded_tsv **arg0) override;
  LONGEST get_raw_trace_data (gdb_byte *arg0, ULONGEST arg1, LONGEST arg2) override;
  int get_min_fast_tracepoint_insn_len () override;
  void set_disconnected_tracing (int arg0) override;
  void set_circular_trace_buffer (int arg0) override;
  void set_trace_buffer_size (LONGEST arg0) override;
  bool set_trace_notes (const char *arg0, const char *arg1, const char *arg2) override;
  int core_of_thread (ptid_t arg0) override;
  int verify_memory (const gdb_byte *arg0, CORE_ADDR arg1, ULONGEST arg2) override;
  bool get_tib_address (ptid_t arg0, CORE_ADDR *arg1) override;
  void set_permissions () override;
  bool static_tracepoint_marker_at (CORE_ADDR arg0, static_tracepoint_marker *arg1) override;
  std::vector<static_tracepoint_marker> static_tracepoint_markers_by_strid (const char *arg0) override;
  traceframe_info_up traceframe_info () override;
  bool use_agent (bool arg0) override;
  bool can_use_agent () override;
  struct btrace_target_info *enable_btrace (thread_info *arg0, const struct btrace_config *arg1) override;
  void disable_btrace (struct btrace_target_info *arg0) override;
  void teardown_btrace (struct btrace_target_info *arg0) override;
  enum btrace_error read_btrace (struct btrace_data *arg0, struct btrace_target_info *arg1, enum btrace_read_type arg2) override;
  const struct btrace_config *btrace_conf (const struct btrace_target_info *arg0) override;
  enum record_method record_method (ptid_t arg0) override;
  void stop_recording () override;
  void info_record () override;
  void save_record (const char *arg0) override;
  bool supports_delete_record () override;
  void delete_record () override;
  bool record_is_replaying (ptid_t arg0) override;
  bool record_will_replay (ptid_t arg0, int arg1) override;
  void record_stop_replaying () override;
  void goto_record_begin () override;
  void goto_record_end () override;
  void goto_record (ULONGEST arg0) override;
  void insn_history (int arg0, gdb_disassembly_flags arg1) override;
  void insn_history_from (ULONGEST arg0, int arg1, gdb_disassembly_flags arg2) override;
  void insn_history_range (ULONGEST arg0, ULONGEST arg1, gdb_disassembly_flags arg2) override;
  void call_history (int arg0, record_print_flags arg1) override;
  void call_history_from (ULONGEST arg0, int arg1, record_print_flags arg2) override;
  void call_history_range (ULONGEST arg0, ULONGEST arg1, record_print_flags arg2) override;
  bool augmented_libraries_svr4_read () override;
  const struct frame_unwind *get_unwinder () override;
  const struct frame_unwind *get_tailcall_unwinder () override;
  void prepare_to_generate_core () override;
  void done_generating_core () override;
  bool supports_memory_tagging () override;
  bool fetch_memtags (CORE_ADDR arg0, size_t arg1, gdb::byte_vector &arg2, int arg3) override;
  bool store_memtags (CORE_ADDR arg0, size_t arg1, const gdb::byte_vector &arg2, int arg3) override;
  x86_xsave_layout fetch_x86_xsave_layout () override;
};

void
target_ops::post_attach (int arg0)
{
  this->beneath ()->post_attach (arg0);
}

void
dummy_target::post_attach (int arg0)
{
}

void
debug_target::post_attach (int arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->post_attach (...)\n", this->beneath ()->shortname ());
  this->beneath ()->post_attach (arg0);
  gdb_printf (gdb_stdlog, "<- %s->post_attach (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::detach (inferior *arg0, int arg1)
{
  this->beneath ()->detach (arg0, arg1);
}

void
dummy_target::detach (inferior *arg0, int arg1)
{
}

void
debug_target::detach (inferior *arg0, int arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->detach (...)\n", this->beneath ()->shortname ());
  this->beneath ()->detach (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->detach (", this->beneath ()->shortname ());
  target_debug_print_inferior_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::disconnect (const char *arg0, int arg1)
{
  this->beneath ()->disconnect (arg0, arg1);
}

void
dummy_target::disconnect (const char *arg0, int arg1)
{
  tcomplain ();
}

void
debug_target::disconnect (const char *arg0, int arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->disconnect (...)\n", this->beneath ()->shortname ());
  this->beneath ()->disconnect (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->disconnect (", this->beneath ()->shortname ());
  target_debug_print_const_char_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::resume (ptid_t arg0, int arg1, enum gdb_signal arg2)
{
  this->beneath ()->resume (arg0, arg1, arg2);
}

void
dummy_target::resume (ptid_t arg0, int arg1, enum gdb_signal arg2)
{
  noprocess ();
}

void
debug_target::resume (ptid_t arg0, int arg1, enum gdb_signal arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->resume (...)\n", this->beneath ()->shortname ());
  this->beneath ()->resume (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->resume (", this->beneath ()->shortname ());
  target_debug_print_ptid_t (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_step (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_gdb_signal (arg2);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::commit_resumed ()
{
  this->beneath ()->commit_resumed ();
}

void
dummy_target::commit_resumed ()
{
}

void
debug_target::commit_resumed ()
{
  gdb_printf (gdb_stdlog, "-> %s->commit_resumed (...)\n", this->beneath ()->shortname ());
  this->beneath ()->commit_resumed ();
  gdb_printf (gdb_stdlog, "<- %s->commit_resumed (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

ptid_t
target_ops::wait (ptid_t arg0, struct target_waitstatus *arg1, target_wait_flags arg2)
{
  return this->beneath ()->wait (arg0, arg1, arg2);
}

ptid_t
dummy_target::wait (ptid_t arg0, struct target_waitstatus *arg1, target_wait_flags arg2)
{
  return default_target_wait (this, arg0, arg1, arg2);
}

ptid_t
debug_target::wait (ptid_t arg0, struct target_waitstatus *arg1, target_wait_flags arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->wait (...)\n", this->beneath ()->shortname ());
  ptid_t result
    = this->beneath ()->wait (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->wait (", this->beneath ()->shortname ());
  target_debug_print_ptid_t (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_target_waitstatus_p (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_target_wait_flags (arg2);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_ptid_t (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::fetch_registers (struct regcache *arg0, int arg1)
{
  this->beneath ()->fetch_registers (arg0, arg1);
}

void
dummy_target::fetch_registers (struct regcache *arg0, int arg1)
{
}

void
debug_target::fetch_registers (struct regcache *arg0, int arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->fetch_registers (...)\n", this->beneath ()->shortname ());
  this->beneath ()->fetch_registers (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->fetch_registers (", this->beneath ()->shortname ());
  target_debug_print_regcache_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::store_registers (struct regcache *arg0, int arg1)
{
  this->beneath ()->store_registers (arg0, arg1);
}

void
dummy_target::store_registers (struct regcache *arg0, int arg1)
{
  noprocess ();
}

void
debug_target::store_registers (struct regcache *arg0, int arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->store_registers (...)\n", this->beneath ()->shortname ());
  this->beneath ()->store_registers (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->store_registers (", this->beneath ()->shortname ());
  target_debug_print_regcache_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::prepare_to_store (struct regcache *arg0)
{
  this->beneath ()->prepare_to_store (arg0);
}

void
dummy_target::prepare_to_store (struct regcache *arg0)
{
  noprocess ();
}

void
debug_target::prepare_to_store (struct regcache *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->prepare_to_store (...)\n", this->beneath ()->shortname ());
  this->beneath ()->prepare_to_store (arg0);
  gdb_printf (gdb_stdlog, "<- %s->prepare_to_store (", this->beneath ()->shortname ());
  target_debug_print_regcache_p (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::files_info ()
{
  this->beneath ()->files_info ();
}

void
dummy_target::files_info ()
{
}

void
debug_target::files_info ()
{
  gdb_printf (gdb_stdlog, "-> %s->files_info (...)\n", this->beneath ()->shortname ());
  this->beneath ()->files_info ();
  gdb_printf (gdb_stdlog, "<- %s->files_info (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

int
target_ops::insert_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1)
{
  return this->beneath ()->insert_breakpoint (arg0, arg1);
}

int
dummy_target::insert_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1)
{
  noprocess ();
}

int
debug_target::insert_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->insert_breakpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->insert_breakpoint (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->insert_breakpoint (", this->beneath ()->shortname ());
  target_debug_print_gdbarch_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_bp_target_info_p (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::remove_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1, enum remove_bp_reason arg2)
{
  return this->beneath ()->remove_breakpoint (arg0, arg1, arg2);
}

int
dummy_target::remove_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1, enum remove_bp_reason arg2)
{
  noprocess ();
}

int
debug_target::remove_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1, enum remove_bp_reason arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->remove_breakpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->remove_breakpoint (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->remove_breakpoint (", this->beneath ()->shortname ());
  target_debug_print_gdbarch_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_bp_target_info_p (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_remove_bp_reason (arg2);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::stopped_by_sw_breakpoint ()
{
  return this->beneath ()->stopped_by_sw_breakpoint ();
}

bool
dummy_target::stopped_by_sw_breakpoint ()
{
  return false;
}

bool
debug_target::stopped_by_sw_breakpoint ()
{
  gdb_printf (gdb_stdlog, "-> %s->stopped_by_sw_breakpoint (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->stopped_by_sw_breakpoint ();
  gdb_printf (gdb_stdlog, "<- %s->stopped_by_sw_breakpoint (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::supports_stopped_by_sw_breakpoint ()
{
  return this->beneath ()->supports_stopped_by_sw_breakpoint ();
}

bool
dummy_target::supports_stopped_by_sw_breakpoint ()
{
  return false;
}

bool
debug_target::supports_stopped_by_sw_breakpoint ()
{
  gdb_printf (gdb_stdlog, "-> %s->supports_stopped_by_sw_breakpoint (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->supports_stopped_by_sw_breakpoint ();
  gdb_printf (gdb_stdlog, "<- %s->supports_stopped_by_sw_breakpoint (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::stopped_by_hw_breakpoint ()
{
  return this->beneath ()->stopped_by_hw_breakpoint ();
}

bool
dummy_target::stopped_by_hw_breakpoint ()
{
  return false;
}

bool
debug_target::stopped_by_hw_breakpoint ()
{
  gdb_printf (gdb_stdlog, "-> %s->stopped_by_hw_breakpoint (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->stopped_by_hw_breakpoint ();
  gdb_printf (gdb_stdlog, "<- %s->stopped_by_hw_breakpoint (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::supports_stopped_by_hw_breakpoint ()
{
  return this->beneath ()->supports_stopped_by_hw_breakpoint ();
}

bool
dummy_target::supports_stopped_by_hw_breakpoint ()
{
  return false;
}

bool
debug_target::supports_stopped_by_hw_breakpoint ()
{
  gdb_printf (gdb_stdlog, "-> %s->supports_stopped_by_hw_breakpoint (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->supports_stopped_by_hw_breakpoint ();
  gdb_printf (gdb_stdlog, "<- %s->supports_stopped_by_hw_breakpoint (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::can_use_hw_breakpoint (enum bptype arg0, int arg1, int arg2)
{
  return this->beneath ()->can_use_hw_breakpoint (arg0, arg1, arg2);
}

int
dummy_target::can_use_hw_breakpoint (enum bptype arg0, int arg1, int arg2)
{
  return 0;
}

int
debug_target::can_use_hw_breakpoint (enum bptype arg0, int arg1, int arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->can_use_hw_breakpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->can_use_hw_breakpoint (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->can_use_hw_breakpoint (", this->beneath ()->shortname ());
  target_debug_print_bptype (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::ranged_break_num_registers ()
{
  return this->beneath ()->ranged_break_num_registers ();
}

int
dummy_target::ranged_break_num_registers ()
{
  return -1;
}

int
debug_target::ranged_break_num_registers ()
{
  gdb_printf (gdb_stdlog, "-> %s->ranged_break_num_registers (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->ranged_break_num_registers ();
  gdb_printf (gdb_stdlog, "<- %s->ranged_break_num_registers (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::insert_hw_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1)
{
  return this->beneath ()->insert_hw_breakpoint (arg0, arg1);
}

int
dummy_target::insert_hw_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1)
{
  return -1;
}

int
debug_target::insert_hw_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->insert_hw_breakpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->insert_hw_breakpoint (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->insert_hw_breakpoint (", this->beneath ()->shortname ());
  target_debug_print_gdbarch_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_bp_target_info_p (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::remove_hw_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1)
{
  return this->beneath ()->remove_hw_breakpoint (arg0, arg1);
}

int
dummy_target::remove_hw_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1)
{
  return -1;
}

int
debug_target::remove_hw_breakpoint (struct gdbarch *arg0, struct bp_target_info *arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->remove_hw_breakpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->remove_hw_breakpoint (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->remove_hw_breakpoint (", this->beneath ()->shortname ());
  target_debug_print_gdbarch_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_bp_target_info_p (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::remove_watchpoint (CORE_ADDR arg0, int arg1, enum target_hw_bp_type arg2, struct expression *arg3)
{
  return this->beneath ()->remove_watchpoint (arg0, arg1, arg2, arg3);
}

int
dummy_target::remove_watchpoint (CORE_ADDR arg0, int arg1, enum target_hw_bp_type arg2, struct expression *arg3)
{
  return -1;
}

int
debug_target::remove_watchpoint (CORE_ADDR arg0, int arg1, enum target_hw_bp_type arg2, struct expression *arg3)
{
  gdb_printf (gdb_stdlog, "-> %s->remove_watchpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->remove_watchpoint (arg0, arg1, arg2, arg3);
  gdb_printf (gdb_stdlog, "<- %s->remove_watchpoint (", this->beneath ()->shortname ());
  target_debug_print_CORE_ADDR (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_target_hw_bp_type (arg2);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_expression_p (arg3);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::insert_watchpoint (CORE_ADDR arg0, int arg1, enum target_hw_bp_type arg2, struct expression *arg3)
{
  return this->beneath ()->insert_watchpoint (arg0, arg1, arg2, arg3);
}

int
dummy_target::insert_watchpoint (CORE_ADDR arg0, int arg1, enum target_hw_bp_type arg2, struct expression *arg3)
{
  return -1;
}

int
debug_target::insert_watchpoint (CORE_ADDR arg0, int arg1, enum target_hw_bp_type arg2, struct expression *arg3)
{
  gdb_printf (gdb_stdlog, "-> %s->insert_watchpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->insert_watchpoint (arg0, arg1, arg2, arg3);
  gdb_printf (gdb_stdlog, "<- %s->insert_watchpoint (", this->beneath ()->shortname ());
  target_debug_print_CORE_ADDR (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_target_hw_bp_type (arg2);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_expression_p (arg3);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::insert_mask_watchpoint (CORE_ADDR arg0, CORE_ADDR arg1, enum target_hw_bp_type arg2)
{
  return this->beneath ()->insert_mask_watchpoint (arg0, arg1, arg2);
}

int
dummy_target::insert_mask_watchpoint (CORE_ADDR arg0, CORE_ADDR arg1, enum target_hw_bp_type arg2)
{
  return 1;
}

int
debug_target::insert_mask_watchpoint (CORE_ADDR arg0, CORE_ADDR arg1, enum target_hw_bp_type arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->insert_mask_watchpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->insert_mask_watchpoint (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->insert_mask_watchpoint (", this->beneath ()->shortname ());
  target_debug_print_CORE_ADDR (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_target_hw_bp_type (arg2);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::remove_mask_watchpoint (CORE_ADDR arg0, CORE_ADDR arg1, enum target_hw_bp_type arg2)
{
  return this->beneath ()->remove_mask_watchpoint (arg0, arg1, arg2);
}

int
dummy_target::remove_mask_watchpoint (CORE_ADDR arg0, CORE_ADDR arg1, enum target_hw_bp_type arg2)
{
  return 1;
}

int
debug_target::remove_mask_watchpoint (CORE_ADDR arg0, CORE_ADDR arg1, enum target_hw_bp_type arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->remove_mask_watchpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->remove_mask_watchpoint (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->remove_mask_watchpoint (", this->beneath ()->shortname ());
  target_debug_print_CORE_ADDR (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_target_hw_bp_type (arg2);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::stopped_by_watchpoint ()
{
  return this->beneath ()->stopped_by_watchpoint ();
}

bool
dummy_target::stopped_by_watchpoint ()
{
  return false;
}

bool
debug_target::stopped_by_watchpoint ()
{
  gdb_printf (gdb_stdlog, "-> %s->stopped_by_watchpoint (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->stopped_by_watchpoint ();
  gdb_printf (gdb_stdlog, "<- %s->stopped_by_watchpoint (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::have_steppable_watchpoint ()
{
  return this->beneath ()->have_steppable_watchpoint ();
}

bool
dummy_target::have_steppable_watchpoint ()
{
  return false;
}

bool
debug_target::have_steppable_watchpoint ()
{
  gdb_printf (gdb_stdlog, "-> %s->have_steppable_watchpoint (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->have_steppable_watchpoint ();
  gdb_printf (gdb_stdlog, "<- %s->have_steppable_watchpoint (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::stopped_data_address (CORE_ADDR *arg0)
{
  return this->beneath ()->stopped_data_address (arg0);
}

bool
dummy_target::stopped_data_address (CORE_ADDR *arg0)
{
  return false;
}

bool
debug_target::stopped_data_address (CORE_ADDR *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->stopped_data_address (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->stopped_data_address (arg0);
  gdb_printf (gdb_stdlog, "<- %s->stopped_data_address (", this->beneath ()->shortname ());
  target_debug_print_CORE_ADDR_p (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::watchpoint_addr_within_range (CORE_ADDR arg0, CORE_ADDR arg1, int arg2)
{
  return this->beneath ()->watchpoint_addr_within_range (arg0, arg1, arg2);
}

bool
dummy_target::watchpoint_addr_within_range (CORE_ADDR arg0, CORE_ADDR arg1, int arg2)
{
  return default_watchpoint_addr_within_range (this, arg0, arg1, arg2);
}

bool
debug_target::watchpoint_addr_within_range (CORE_ADDR arg0, CORE_ADDR arg1, int arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->watchpoint_addr_within_range (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->watchpoint_addr_within_range (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->watchpoint_addr_within_range (", this->beneath ()->shortname ());
  target_debug_print_CORE_ADDR (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::region_ok_for_hw_watchpoint (CORE_ADDR arg0, int arg1)
{
  return this->beneath ()->region_ok_for_hw_watchpoint (arg0, arg1);
}

int
dummy_target::region_ok_for_hw_watchpoint (CORE_ADDR arg0, int arg1)
{
  return default_region_ok_for_hw_watchpoint (this, arg0, arg1);
}

int
debug_target::region_ok_for_hw_watchpoint (CORE_ADDR arg0, int arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->region_ok_for_hw_watchpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->region_ok_for_hw_watchpoint (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->region_ok_for_hw_watchpoint (", this->beneath ()->shortname ());
  target_debug_print_CORE_ADDR (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::can_accel_watchpoint_condition (CORE_ADDR arg0, int arg1, int arg2, struct expression *arg3)
{
  return this->beneath ()->can_accel_watchpoint_condition (arg0, arg1, arg2, arg3);
}

bool
dummy_target::can_accel_watchpoint_condition (CORE_ADDR arg0, int arg1, int arg2, struct expression *arg3)
{
  return false;
}

bool
debug_target::can_accel_watchpoint_condition (CORE_ADDR arg0, int arg1, int arg2, struct expression *arg3)
{
  gdb_printf (gdb_stdlog, "-> %s->can_accel_watchpoint_condition (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->can_accel_watchpoint_condition (arg0, arg1, arg2, arg3);
  gdb_printf (gdb_stdlog, "<- %s->can_accel_watchpoint_condition (", this->beneath ()->shortname ());
  target_debug_print_CORE_ADDR (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_expression_p (arg3);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::masked_watch_num_registers (CORE_ADDR arg0, CORE_ADDR arg1)
{
  return this->beneath ()->masked_watch_num_registers (arg0, arg1);
}

int
dummy_target::masked_watch_num_registers (CORE_ADDR arg0, CORE_ADDR arg1)
{
  return -1;
}

int
debug_target::masked_watch_num_registers (CORE_ADDR arg0, CORE_ADDR arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->masked_watch_num_registers (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->masked_watch_num_registers (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->masked_watch_num_registers (", this->beneath ()->shortname ());
  target_debug_print_CORE_ADDR (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::can_do_single_step ()
{
  return this->beneath ()->can_do_single_step ();
}

int
dummy_target::can_do_single_step ()
{
  return -1;
}

int
debug_target::can_do_single_step ()
{
  gdb_printf (gdb_stdlog, "-> %s->can_do_single_step (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->can_do_single_step ();
  gdb_printf (gdb_stdlog, "<- %s->can_do_single_step (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::supports_terminal_ours ()
{
  return this->beneath ()->supports_terminal_ours ();
}

bool
dummy_target::supports_terminal_ours ()
{
  return false;
}

bool
debug_target::supports_terminal_ours ()
{
  gdb_printf (gdb_stdlog, "-> %s->supports_terminal_ours (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->supports_terminal_ours ();
  gdb_printf (gdb_stdlog, "<- %s->supports_terminal_ours (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::terminal_init ()
{
  this->beneath ()->terminal_init ();
}

void
dummy_target::terminal_init ()
{
}

void
debug_target::terminal_init ()
{
  gdb_printf (gdb_stdlog, "-> %s->terminal_init (...)\n", this->beneath ()->shortname ());
  this->beneath ()->terminal_init ();
  gdb_printf (gdb_stdlog, "<- %s->terminal_init (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::terminal_inferior ()
{
  this->beneath ()->terminal_inferior ();
}

void
dummy_target::terminal_inferior ()
{
}

void
debug_target::terminal_inferior ()
{
  gdb_printf (gdb_stdlog, "-> %s->terminal_inferior (...)\n", this->beneath ()->shortname ());
  this->beneath ()->terminal_inferior ();
  gdb_printf (gdb_stdlog, "<- %s->terminal_inferior (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::terminal_save_inferior ()
{
  this->beneath ()->terminal_save_inferior ();
}

void
dummy_target::terminal_save_inferior ()
{
}

void
debug_target::terminal_save_inferior ()
{
  gdb_printf (gdb_stdlog, "-> %s->terminal_save_inferior (...)\n", this->beneath ()->shortname ());
  this->beneath ()->terminal_save_inferior ();
  gdb_printf (gdb_stdlog, "<- %s->terminal_save_inferior (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::terminal_ours_for_output ()
{
  this->beneath ()->terminal_ours_for_output ();
}

void
dummy_target::terminal_ours_for_output ()
{
}

void
debug_target::terminal_ours_for_output ()
{
  gdb_printf (gdb_stdlog, "-> %s->terminal_ours_for_output (...)\n", this->beneath ()->shortname ());
  this->beneath ()->terminal_ours_for_output ();
  gdb_printf (gdb_stdlog, "<- %s->terminal_ours_for_output (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::terminal_ours ()
{
  this->beneath ()->terminal_ours ();
}

void
dummy_target::terminal_ours ()
{
}

void
debug_target::terminal_ours ()
{
  gdb_printf (gdb_stdlog, "-> %s->terminal_ours (...)\n", this->beneath ()->shortname ());
  this->beneath ()->terminal_ours ();
  gdb_printf (gdb_stdlog, "<- %s->terminal_ours (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::terminal_info (const char *arg0, int arg1)
{
  this->beneath ()->terminal_info (arg0, arg1);
}

void
dummy_target::terminal_info (const char *arg0, int arg1)
{
  default_terminal_info (this, arg0, arg1);
}

void
debug_target::terminal_info (const char *arg0, int arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->terminal_info (...)\n", this->beneath ()->shortname ());
  this->beneath ()->terminal_info (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->terminal_info (", this->beneath ()->shortname ());
  target_debug_print_const_char_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::kill ()
{
  this->beneath ()->kill ();
}

void
dummy_target::kill ()
{
  noprocess ();
}

void
debug_target::kill ()
{
  gdb_printf (gdb_stdlog, "-> %s->kill (...)\n", this->beneath ()->shortname ());
  this->beneath ()->kill ();
  gdb_printf (gdb_stdlog, "<- %s->kill (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::load (const char *arg0, int arg1)
{
  this->beneath ()->load (arg0, arg1);
}

void
dummy_target::load (const char *arg0, int arg1)
{
  tcomplain ();
}

void
debug_target::load (const char *arg0, int arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->load (...)\n", this->beneath ()->shortname ());
  this->beneath ()->load (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->load (", this->beneath ()->shortname ());
  target_debug_print_const_char_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (")\n", gdb_stdlog);
}

int
target_ops::insert_fork_catchpoint (int arg0)
{
  return this->beneath ()->insert_fork_catchpoint (arg0);
}

int
dummy_target::insert_fork_catchpoint (int arg0)
{
  return 1;
}

int
debug_target::insert_fork_catchpoint (int arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->insert_fork_catchpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->insert_fork_catchpoint (arg0);
  gdb_printf (gdb_stdlog, "<- %s->insert_fork_catchpoint (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::remove_fork_catchpoint (int arg0)
{
  return this->beneath ()->remove_fork_catchpoint (arg0);
}

int
dummy_target::remove_fork_catchpoint (int arg0)
{
  return 1;
}

int
debug_target::remove_fork_catchpoint (int arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->remove_fork_catchpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->remove_fork_catchpoint (arg0);
  gdb_printf (gdb_stdlog, "<- %s->remove_fork_catchpoint (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::insert_vfork_catchpoint (int arg0)
{
  return this->beneath ()->insert_vfork_catchpoint (arg0);
}

int
dummy_target::insert_vfork_catchpoint (int arg0)
{
  return 1;
}

int
debug_target::insert_vfork_catchpoint (int arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->insert_vfork_catchpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->insert_vfork_catchpoint (arg0);
  gdb_printf (gdb_stdlog, "<- %s->insert_vfork_catchpoint (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::remove_vfork_catchpoint (int arg0)
{
  return this->beneath ()->remove_vfork_catchpoint (arg0);
}

int
dummy_target::remove_vfork_catchpoint (int arg0)
{
  return 1;
}

int
debug_target::remove_vfork_catchpoint (int arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->remove_vfork_catchpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->remove_vfork_catchpoint (arg0);
  gdb_printf (gdb_stdlog, "<- %s->remove_vfork_catchpoint (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::follow_fork (inferior *arg0, ptid_t arg1, target_waitkind arg2, bool arg3, bool arg4)
{
  this->beneath ()->follow_fork (arg0, arg1, arg2, arg3, arg4);
}

void
dummy_target::follow_fork (inferior *arg0, ptid_t arg1, target_waitkind arg2, bool arg3, bool arg4)
{
  default_follow_fork (this, arg0, arg1, arg2, arg3, arg4);
}

void
debug_target::follow_fork (inferior *arg0, ptid_t arg1, target_waitkind arg2, bool arg3, bool arg4)
{
  gdb_printf (gdb_stdlog, "-> %s->follow_fork (...)\n", this->beneath ()->shortname ());
  this->beneath ()->follow_fork (arg0, arg1, arg2, arg3, arg4);
  gdb_printf (gdb_stdlog, "<- %s->follow_fork (", this->beneath ()->shortname ());
  target_debug_print_inferior_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_target_waitkind (arg2);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_bool (arg3);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_bool (arg4);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::follow_clone (ptid_t arg0)
{
  this->beneath ()->follow_clone (arg0);
}

void
dummy_target::follow_clone (ptid_t arg0)
{
  default_follow_clone (this, arg0);
}

void
debug_target::follow_clone (ptid_t arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->follow_clone (...)\n", this->beneath ()->shortname ());
  this->beneath ()->follow_clone (arg0);
  gdb_printf (gdb_stdlog, "<- %s->follow_clone (", this->beneath ()->shortname ());
  target_debug_print_ptid_t (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

int
target_ops::insert_exec_catchpoint (int arg0)
{
  return this->beneath ()->insert_exec_catchpoint (arg0);
}

int
dummy_target::insert_exec_catchpoint (int arg0)
{
  return 1;
}

int
debug_target::insert_exec_catchpoint (int arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->insert_exec_catchpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->insert_exec_catchpoint (arg0);
  gdb_printf (gdb_stdlog, "<- %s->insert_exec_catchpoint (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::remove_exec_catchpoint (int arg0)
{
  return this->beneath ()->remove_exec_catchpoint (arg0);
}

int
dummy_target::remove_exec_catchpoint (int arg0)
{
  return 1;
}

int
debug_target::remove_exec_catchpoint (int arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->remove_exec_catchpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->remove_exec_catchpoint (arg0);
  gdb_printf (gdb_stdlog, "<- %s->remove_exec_catchpoint (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::follow_exec (inferior *arg0, ptid_t arg1, const char *arg2)
{
  this->beneath ()->follow_exec (arg0, arg1, arg2);
}

void
dummy_target::follow_exec (inferior *arg0, ptid_t arg1, const char *arg2)
{
}

void
debug_target::follow_exec (inferior *arg0, ptid_t arg1, const char *arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->follow_exec (...)\n", this->beneath ()->shortname ());
  this->beneath ()->follow_exec (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->follow_exec (", this->beneath ()->shortname ());
  target_debug_print_inferior_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_ptid_t (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg2);
  gdb_puts (")\n", gdb_stdlog);
}

int
target_ops::set_syscall_catchpoint (int arg0, bool arg1, int arg2, gdb::array_view<const int> arg3)
{
  return this->beneath ()->set_syscall_catchpoint (arg0, arg1, arg2, arg3);
}

int
dummy_target::set_syscall_catchpoint (int arg0, bool arg1, int arg2, gdb::array_view<const int> arg3)
{
  return 1;
}

int
debug_target::set_syscall_catchpoint (int arg0, bool arg1, int arg2, gdb::array_view<const int> arg3)
{
  gdb_printf (gdb_stdlog, "-> %s->set_syscall_catchpoint (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->set_syscall_catchpoint (arg0, arg1, arg2, arg3);
  gdb_printf (gdb_stdlog, "<- %s->set_syscall_catchpoint (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_bool (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg2);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_gdb_array_view_const_int (arg3);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::mourn_inferior ()
{
  this->beneath ()->mourn_inferior ();
}

void
dummy_target::mourn_inferior ()
{
  default_mourn_inferior (this);
}

void
debug_target::mourn_inferior ()
{
  gdb_printf (gdb_stdlog, "-> %s->mourn_inferior (...)\n", this->beneath ()->shortname ());
  this->beneath ()->mourn_inferior ();
  gdb_printf (gdb_stdlog, "<- %s->mourn_inferior (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::pass_signals (gdb::array_view<const unsigned char> arg0)
{
  this->beneath ()->pass_signals (arg0);
}

void
dummy_target::pass_signals (gdb::array_view<const unsigned char> arg0)
{
}

void
debug_target::pass_signals (gdb::array_view<const unsigned char> arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->pass_signals (...)\n", this->beneath ()->shortname ());
  this->beneath ()->pass_signals (arg0);
  gdb_printf (gdb_stdlog, "<- %s->pass_signals (", this->beneath ()->shortname ());
  target_debug_print_signals (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::program_signals (gdb::array_view<const unsigned char> arg0)
{
  this->beneath ()->program_signals (arg0);
}

void
dummy_target::program_signals (gdb::array_view<const unsigned char> arg0)
{
}

void
debug_target::program_signals (gdb::array_view<const unsigned char> arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->program_signals (...)\n", this->beneath ()->shortname ());
  this->beneath ()->program_signals (arg0);
  gdb_printf (gdb_stdlog, "<- %s->program_signals (", this->beneath ()->shortname ());
  target_debug_print_signals (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

bool
target_ops::thread_alive (ptid_t arg0)
{
  return this->beneath ()->thread_alive (arg0);
}

bool
dummy_target::thread_alive (ptid_t arg0)
{
  return false;
}

bool
debug_target::thread_alive (ptid_t arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->thread_alive (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->thread_alive (arg0);
  gdb_printf (gdb_stdlog, "<- %s->thread_alive (", this->beneath ()->shortname ());
  target_debug_print_ptid_t (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::update_thread_list ()
{
  this->beneath ()->update_thread_list ();
}

void
dummy_target::update_thread_list ()
{
}

void
debug_target::update_thread_list ()
{
  gdb_printf (gdb_stdlog, "-> %s->update_thread_list (...)\n", this->beneath ()->shortname ());
  this->beneath ()->update_thread_list ();
  gdb_printf (gdb_stdlog, "<- %s->update_thread_list (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

std::string
target_ops::pid_to_str (ptid_t arg0)
{
  return this->beneath ()->pid_to_str (arg0);
}

std::string
dummy_target::pid_to_str (ptid_t arg0)
{
  return default_pid_to_str (this, arg0);
}

std::string
debug_target::pid_to_str (ptid_t arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->pid_to_str (...)\n", this->beneath ()->shortname ());
  std::string result
    = this->beneath ()->pid_to_str (arg0);
  gdb_printf (gdb_stdlog, "<- %s->pid_to_str (", this->beneath ()->shortname ());
  target_debug_print_ptid_t (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_std_string (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

const char *
target_ops::extra_thread_info (thread_info *arg0)
{
  return this->beneath ()->extra_thread_info (arg0);
}

const char *
dummy_target::extra_thread_info (thread_info *arg0)
{
  return NULL;
}

const char *
debug_target::extra_thread_info (thread_info *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->extra_thread_info (...)\n", this->beneath ()->shortname ());
  const char * result
    = this->beneath ()->extra_thread_info (arg0);
  gdb_printf (gdb_stdlog, "<- %s->extra_thread_info (", this->beneath ()->shortname ());
  target_debug_print_thread_info_p (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_const_char_p (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

const char *
target_ops::thread_name (thread_info *arg0)
{
  return this->beneath ()->thread_name (arg0);
}

const char *
dummy_target::thread_name (thread_info *arg0)
{
  return NULL;
}

const char *
debug_target::thread_name (thread_info *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->thread_name (...)\n", this->beneath ()->shortname ());
  const char * result
    = this->beneath ()->thread_name (arg0);
  gdb_printf (gdb_stdlog, "<- %s->thread_name (", this->beneath ()->shortname ());
  target_debug_print_thread_info_p (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_const_char_p (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

thread_info *
target_ops::thread_handle_to_thread_info (const gdb_byte *arg0, int arg1, inferior *arg2)
{
  return this->beneath ()->thread_handle_to_thread_info (arg0, arg1, arg2);
}

thread_info *
dummy_target::thread_handle_to_thread_info (const gdb_byte *arg0, int arg1, inferior *arg2)
{
  return NULL;
}

thread_info *
debug_target::thread_handle_to_thread_info (const gdb_byte *arg0, int arg1, inferior *arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->thread_handle_to_thread_info (...)\n", this->beneath ()->shortname ());
  thread_info * result
    = this->beneath ()->thread_handle_to_thread_info (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->thread_handle_to_thread_info (", this->beneath ()->shortname ());
  target_debug_print_const_gdb_byte_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_inferior_p (arg2);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_thread_info_p (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

gdb::array_view<const_gdb_byte>
target_ops::thread_info_to_thread_handle (struct thread_info *arg0)
{
  return this->beneath ()->thread_info_to_thread_handle (arg0);
}

gdb::array_view<const_gdb_byte>
dummy_target::thread_info_to_thread_handle (struct thread_info *arg0)
{
  return gdb::array_view<const gdb_byte> ();
}

gdb::array_view<const_gdb_byte>
debug_target::thread_info_to_thread_handle (struct thread_info *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->thread_info_to_thread_handle (...)\n", this->beneath ()->shortname ());
  gdb::array_view<const_gdb_byte> result
    = this->beneath ()->thread_info_to_thread_handle (arg0);
  gdb_printf (gdb_stdlog, "<- %s->thread_info_to_thread_handle (", this->beneath ()->shortname ());
  target_debug_print_thread_info_p (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_gdb_array_view_const_gdb_byte (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::stop (ptid_t arg0)
{
  this->beneath ()->stop (arg0);
}

void
dummy_target::stop (ptid_t arg0)
{
}

void
debug_target::stop (ptid_t arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->stop (...)\n", this->beneath ()->shortname ());
  this->beneath ()->stop (arg0);
  gdb_printf (gdb_stdlog, "<- %s->stop (", this->beneath ()->shortname ());
  target_debug_print_ptid_t (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::interrupt ()
{
  this->beneath ()->interrupt ();
}

void
dummy_target::interrupt ()
{
}

void
debug_target::interrupt ()
{
  gdb_printf (gdb_stdlog, "-> %s->interrupt (...)\n", this->beneath ()->shortname ());
  this->beneath ()->interrupt ();
  gdb_printf (gdb_stdlog, "<- %s->interrupt (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::pass_ctrlc ()
{
  this->beneath ()->pass_ctrlc ();
}

void
dummy_target::pass_ctrlc ()
{
  default_target_pass_ctrlc (this);
}

void
debug_target::pass_ctrlc ()
{
  gdb_printf (gdb_stdlog, "-> %s->pass_ctrlc (...)\n", this->beneath ()->shortname ());
  this->beneath ()->pass_ctrlc ();
  gdb_printf (gdb_stdlog, "<- %s->pass_ctrlc (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::rcmd (const char *arg0, struct ui_file *arg1)
{
  this->beneath ()->rcmd (arg0, arg1);
}

void
dummy_target::rcmd (const char *arg0, struct ui_file *arg1)
{
  default_rcmd (this, arg0, arg1);
}

void
debug_target::rcmd (const char *arg0, struct ui_file *arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->rcmd (...)\n", this->beneath ()->shortname ());
  this->beneath ()->rcmd (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->rcmd (", this->beneath ()->shortname ());
  target_debug_print_const_char_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_ui_file_p (arg1);
  gdb_puts (")\n", gdb_stdlog);
}

const char *
target_ops::pid_to_exec_file (int arg0)
{
  return this->beneath ()->pid_to_exec_file (arg0);
}

const char *
dummy_target::pid_to_exec_file (int arg0)
{
  return NULL;
}

const char *
debug_target::pid_to_exec_file (int arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->pid_to_exec_file (...)\n", this->beneath ()->shortname ());
  const char * result
    = this->beneath ()->pid_to_exec_file (arg0);
  gdb_printf (gdb_stdlog, "<- %s->pid_to_exec_file (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_const_char_p (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::log_command (const char *arg0)
{
  this->beneath ()->log_command (arg0);
}

void
dummy_target::log_command (const char *arg0)
{
}

void
debug_target::log_command (const char *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->log_command (...)\n", this->beneath ()->shortname ());
  this->beneath ()->log_command (arg0);
  gdb_printf (gdb_stdlog, "<- %s->log_command (", this->beneath ()->shortname ());
  target_debug_print_const_char_p (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

const std::vector<target_section> *
target_ops::get_section_table ()
{
  return this->beneath ()->get_section_table ();
}

const std::vector<target_section> *
dummy_target::get_section_table ()
{
  return default_get_section_table ();
}

const std::vector<target_section> *
debug_target::get_section_table ()
{
  gdb_printf (gdb_stdlog, "-> %s->get_section_table (...)\n", this->beneath ()->shortname ());
  const std::vector<target_section> * result
    = this->beneath ()->get_section_table ();
  gdb_printf (gdb_stdlog, "<- %s->get_section_table (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_const_std_vector_target_section_p (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

thread_control_capabilities
target_ops::get_thread_control_capabilities ()
{
  return this->beneath ()->get_thread_control_capabilities ();
}

thread_control_capabilities
dummy_target::get_thread_control_capabilities ()
{
  return tc_none;
}

thread_control_capabilities
debug_target::get_thread_control_capabilities ()
{
  gdb_printf (gdb_stdlog, "-> %s->get_thread_control_capabilities (...)\n", this->beneath ()->shortname ());
  thread_control_capabilities result
    = this->beneath ()->get_thread_control_capabilities ();
  gdb_printf (gdb_stdlog, "<- %s->get_thread_control_capabilities (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_thread_control_capabilities (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::attach_no_wait ()
{
  return this->beneath ()->attach_no_wait ();
}

bool
dummy_target::attach_no_wait ()
{
  return 0;
}

bool
debug_target::attach_no_wait ()
{
  gdb_printf (gdb_stdlog, "-> %s->attach_no_wait (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->attach_no_wait ();
  gdb_printf (gdb_stdlog, "<- %s->attach_no_wait (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::can_async_p ()
{
  return this->beneath ()->can_async_p ();
}

bool
dummy_target::can_async_p ()
{
  return false;
}

bool
debug_target::can_async_p ()
{
  gdb_printf (gdb_stdlog, "-> %s->can_async_p (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->can_async_p ();
  gdb_printf (gdb_stdlog, "<- %s->can_async_p (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::is_async_p ()
{
  return this->beneath ()->is_async_p ();
}

bool
dummy_target::is_async_p ()
{
  return false;
}

bool
debug_target::is_async_p ()
{
  gdb_printf (gdb_stdlog, "-> %s->is_async_p (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->is_async_p ();
  gdb_printf (gdb_stdlog, "<- %s->is_async_p (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::async (bool arg0)
{
  this->beneath ()->async (arg0);
}

void
dummy_target::async (bool arg0)
{
  tcomplain ();
}

void
debug_target::async (bool arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->async (...)\n", this->beneath ()->shortname ());
  this->beneath ()->async (arg0);
  gdb_printf (gdb_stdlog, "<- %s->async (", this->beneath ()->shortname ());
  target_debug_print_bool (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

int
target_ops::async_wait_fd ()
{
  return this->beneath ()->async_wait_fd ();
}

int
dummy_target::async_wait_fd ()
{
  noprocess ();
}

int
debug_target::async_wait_fd ()
{
  gdb_printf (gdb_stdlog, "-> %s->async_wait_fd (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->async_wait_fd ();
  gdb_printf (gdb_stdlog, "<- %s->async_wait_fd (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::has_pending_events ()
{
  return this->beneath ()->has_pending_events ();
}

bool
dummy_target::has_pending_events ()
{
  return false;
}

bool
debug_target::has_pending_events ()
{
  gdb_printf (gdb_stdlog, "-> %s->has_pending_events (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->has_pending_events ();
  gdb_printf (gdb_stdlog, "<- %s->has_pending_events (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::thread_events (int arg0)
{
  this->beneath ()->thread_events (arg0);
}

void
dummy_target::thread_events (int arg0)
{
}

void
debug_target::thread_events (int arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->thread_events (...)\n", this->beneath ()->shortname ());
  this->beneath ()->thread_events (arg0);
  gdb_printf (gdb_stdlog, "<- %s->thread_events (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

bool
target_ops::supports_set_thread_options (gdb_thread_options arg0)
{
  return this->beneath ()->supports_set_thread_options (arg0);
}

bool
dummy_target::supports_set_thread_options (gdb_thread_options arg0)
{
  return false;
}

bool
debug_target::supports_set_thread_options (gdb_thread_options arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->supports_set_thread_options (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->supports_set_thread_options (arg0);
  gdb_printf (gdb_stdlog, "<- %s->supports_set_thread_options (", this->beneath ()->shortname ());
  target_debug_print_gdb_thread_options (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::supports_non_stop ()
{
  return this->beneath ()->supports_non_stop ();
}

bool
dummy_target::supports_non_stop ()
{
  return false;
}

bool
debug_target::supports_non_stop ()
{
  gdb_printf (gdb_stdlog, "-> %s->supports_non_stop (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->supports_non_stop ();
  gdb_printf (gdb_stdlog, "<- %s->supports_non_stop (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::always_non_stop_p ()
{
  return this->beneath ()->always_non_stop_p ();
}

bool
dummy_target::always_non_stop_p ()
{
  return false;
}

bool
debug_target::always_non_stop_p ()
{
  gdb_printf (gdb_stdlog, "-> %s->always_non_stop_p (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->always_non_stop_p ();
  gdb_printf (gdb_stdlog, "<- %s->always_non_stop_p (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::find_memory_regions (find_memory_region_ftype arg0, void *arg1)
{
  return this->beneath ()->find_memory_regions (arg0, arg1);
}

int
dummy_target::find_memory_regions (find_memory_region_ftype arg0, void *arg1)
{
  return dummy_find_memory_regions (this, arg0, arg1);
}

int
debug_target::find_memory_regions (find_memory_region_ftype arg0, void *arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->find_memory_regions (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->find_memory_regions (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->find_memory_regions (", this->beneath ()->shortname ());
  target_debug_print_find_memory_region_ftype (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_void_p (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

gdb::unique_xmalloc_ptr<char>
target_ops::make_corefile_notes (bfd *arg0, int *arg1)
{
  return this->beneath ()->make_corefile_notes (arg0, arg1);
}

gdb::unique_xmalloc_ptr<char>
dummy_target::make_corefile_notes (bfd *arg0, int *arg1)
{
  return dummy_make_corefile_notes (this, arg0, arg1);
}

gdb::unique_xmalloc_ptr<char>
debug_target::make_corefile_notes (bfd *arg0, int *arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->make_corefile_notes (...)\n", this->beneath ()->shortname ());
  gdb::unique_xmalloc_ptr<char> result
    = this->beneath ()->make_corefile_notes (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->make_corefile_notes (", this->beneath ()->shortname ());
  target_debug_print_bfd_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int_p (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_gdb_unique_xmalloc_ptr_char (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

gdb_byte *
target_ops::get_bookmark (const char *arg0, int arg1)
{
  return this->beneath ()->get_bookmark (arg0, arg1);
}

gdb_byte *
dummy_target::get_bookmark (const char *arg0, int arg1)
{
  tcomplain ();
}

gdb_byte *
debug_target::get_bookmark (const char *arg0, int arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->get_bookmark (...)\n", this->beneath ()->shortname ());
  gdb_byte * result
    = this->beneath ()->get_bookmark (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->get_bookmark (", this->beneath ()->shortname ());
  target_debug_print_const_char_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_gdb_byte_p (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::goto_bookmark (const gdb_byte *arg0, int arg1)
{
  this->beneath ()->goto_bookmark (arg0, arg1);
}

void
dummy_target::goto_bookmark (const gdb_byte *arg0, int arg1)
{
  tcomplain ();
}

void
debug_target::goto_bookmark (const gdb_byte *arg0, int arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->goto_bookmark (...)\n", this->beneath ()->shortname ());
  this->beneath ()->goto_bookmark (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->goto_bookmark (", this->beneath ()->shortname ());
  target_debug_print_const_gdb_byte_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (")\n", gdb_stdlog);
}

CORE_ADDR
target_ops::get_thread_local_address (ptid_t arg0, CORE_ADDR arg1, CORE_ADDR arg2)
{
  return this->beneath ()->get_thread_local_address (arg0, arg1, arg2);
}

CORE_ADDR
dummy_target::get_thread_local_address (ptid_t arg0, CORE_ADDR arg1, CORE_ADDR arg2)
{
  generic_tls_error ();
}

CORE_ADDR
debug_target::get_thread_local_address (ptid_t arg0, CORE_ADDR arg1, CORE_ADDR arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->get_thread_local_address (...)\n", this->beneath ()->shortname ());
  CORE_ADDR result
    = this->beneath ()->get_thread_local_address (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->get_thread_local_address (", this->beneath ()->shortname ());
  target_debug_print_ptid_t (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg2);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_CORE_ADDR (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

enum target_xfer_status
target_ops::xfer_partial (enum target_object arg0, const char *arg1, gdb_byte *arg2, const gdb_byte *arg3, ULONGEST arg4, ULONGEST arg5, ULONGEST *arg6)
{
  return this->beneath ()->xfer_partial (arg0, arg1, arg2, arg3, arg4, arg5, arg6);
}

enum target_xfer_status
dummy_target::xfer_partial (enum target_object arg0, const char *arg1, gdb_byte *arg2, const gdb_byte *arg3, ULONGEST arg4, ULONGEST arg5, ULONGEST *arg6)
{
  return TARGET_XFER_E_IO;
}

enum target_xfer_status
debug_target::xfer_partial (enum target_object arg0, const char *arg1, gdb_byte *arg2, const gdb_byte *arg3, ULONGEST arg4, ULONGEST arg5, ULONGEST *arg6)
{
  gdb_printf (gdb_stdlog, "-> %s->xfer_partial (...)\n", this->beneath ()->shortname ());
  enum target_xfer_status result
    = this->beneath ()->xfer_partial (arg0, arg1, arg2, arg3, arg4, arg5, arg6);
  gdb_printf (gdb_stdlog, "<- %s->xfer_partial (", this->beneath ()->shortname ());
  target_debug_print_target_object (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_gdb_byte_p (arg2);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_const_gdb_byte_p (arg3);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg4);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg5);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_ULONGEST_p (arg6);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_target_xfer_status (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

ULONGEST
target_ops::get_memory_xfer_limit ()
{
  return this->beneath ()->get_memory_xfer_limit ();
}

ULONGEST
dummy_target::get_memory_xfer_limit ()
{
  return ULONGEST_MAX;
}

ULONGEST
debug_target::get_memory_xfer_limit ()
{
  gdb_printf (gdb_stdlog, "-> %s->get_memory_xfer_limit (...)\n", this->beneath ()->shortname ());
  ULONGEST result
    = this->beneath ()->get_memory_xfer_limit ();
  gdb_printf (gdb_stdlog, "<- %s->get_memory_xfer_limit (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_ULONGEST (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

std::vector<mem_region>
target_ops::memory_map ()
{
  return this->beneath ()->memory_map ();
}

std::vector<mem_region>
dummy_target::memory_map ()
{
  return std::vector<mem_region> ();
}

std::vector<mem_region>
debug_target::memory_map ()
{
  gdb_printf (gdb_stdlog, "-> %s->memory_map (...)\n", this->beneath ()->shortname ());
  std::vector<mem_region> result
    = this->beneath ()->memory_map ();
  gdb_printf (gdb_stdlog, "<- %s->memory_map (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_std_vector_mem_region (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::flash_erase (ULONGEST arg0, LONGEST arg1)
{
  this->beneath ()->flash_erase (arg0, arg1);
}

void
dummy_target::flash_erase (ULONGEST arg0, LONGEST arg1)
{
  tcomplain ();
}

void
debug_target::flash_erase (ULONGEST arg0, LONGEST arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->flash_erase (...)\n", this->beneath ()->shortname ());
  this->beneath ()->flash_erase (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->flash_erase (", this->beneath ()->shortname ());
  target_debug_print_ULONGEST (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_LONGEST (arg1);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::flash_done ()
{
  this->beneath ()->flash_done ();
}

void
dummy_target::flash_done ()
{
  tcomplain ();
}

void
debug_target::flash_done ()
{
  gdb_printf (gdb_stdlog, "-> %s->flash_done (...)\n", this->beneath ()->shortname ());
  this->beneath ()->flash_done ();
  gdb_printf (gdb_stdlog, "<- %s->flash_done (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

const struct target_desc *
target_ops::read_description ()
{
  return this->beneath ()->read_description ();
}

const struct target_desc *
dummy_target::read_description ()
{
  return NULL;
}

const struct target_desc *
debug_target::read_description ()
{
  gdb_printf (gdb_stdlog, "-> %s->read_description (...)\n", this->beneath ()->shortname ());
  const struct target_desc * result
    = this->beneath ()->read_description ();
  gdb_printf (gdb_stdlog, "<- %s->read_description (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_const_target_desc_p (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

ptid_t
target_ops::get_ada_task_ptid (long arg0, ULONGEST arg1)
{
  return this->beneath ()->get_ada_task_ptid (arg0, arg1);
}

ptid_t
dummy_target::get_ada_task_ptid (long arg0, ULONGEST arg1)
{
  return default_get_ada_task_ptid (this, arg0, arg1);
}

ptid_t
debug_target::get_ada_task_ptid (long arg0, ULONGEST arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->get_ada_task_ptid (...)\n", this->beneath ()->shortname ());
  ptid_t result
    = this->beneath ()->get_ada_task_ptid (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->get_ada_task_ptid (", this->beneath ()->shortname ());
  target_debug_print_long (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_ptid_t (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::auxv_parse (const gdb_byte **arg0, const gdb_byte *arg1, CORE_ADDR *arg2, CORE_ADDR *arg3)
{
  return this->beneath ()->auxv_parse (arg0, arg1, arg2, arg3);
}

int
dummy_target::auxv_parse (const gdb_byte **arg0, const gdb_byte *arg1, CORE_ADDR *arg2, CORE_ADDR *arg3)
{
  return default_auxv_parse (this, arg0, arg1, arg2, arg3);
}

int
debug_target::auxv_parse (const gdb_byte **arg0, const gdb_byte *arg1, CORE_ADDR *arg2, CORE_ADDR *arg3)
{
  gdb_printf (gdb_stdlog, "-> %s->auxv_parse (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->auxv_parse (arg0, arg1, arg2, arg3);
  gdb_printf (gdb_stdlog, "<- %s->auxv_parse (", this->beneath ()->shortname ());
  target_debug_print_const_gdb_byte_pp (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_const_gdb_byte_p (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR_p (arg2);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR_p (arg3);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::search_memory (CORE_ADDR arg0, ULONGEST arg1, const gdb_byte *arg2, ULONGEST arg3, CORE_ADDR *arg4)
{
  return this->beneath ()->search_memory (arg0, arg1, arg2, arg3, arg4);
}

int
dummy_target::search_memory (CORE_ADDR arg0, ULONGEST arg1, const gdb_byte *arg2, ULONGEST arg3, CORE_ADDR *arg4)
{
  return default_search_memory (this, arg0, arg1, arg2, arg3, arg4);
}

int
debug_target::search_memory (CORE_ADDR arg0, ULONGEST arg1, const gdb_byte *arg2, ULONGEST arg3, CORE_ADDR *arg4)
{
  gdb_printf (gdb_stdlog, "-> %s->search_memory (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->search_memory (arg0, arg1, arg2, arg3, arg4);
  gdb_printf (gdb_stdlog, "<- %s->search_memory (", this->beneath ()->shortname ());
  target_debug_print_CORE_ADDR (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_const_gdb_byte_p (arg2);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg3);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR_p (arg4);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::can_execute_reverse ()
{
  return this->beneath ()->can_execute_reverse ();
}

bool
dummy_target::can_execute_reverse ()
{
  return false;
}

bool
debug_target::can_execute_reverse ()
{
  gdb_printf (gdb_stdlog, "-> %s->can_execute_reverse (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->can_execute_reverse ();
  gdb_printf (gdb_stdlog, "<- %s->can_execute_reverse (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

enum exec_direction_kind
target_ops::execution_direction ()
{
  return this->beneath ()->execution_direction ();
}

enum exec_direction_kind
dummy_target::execution_direction ()
{
  return default_execution_direction (this);
}

enum exec_direction_kind
debug_target::execution_direction ()
{
  gdb_printf (gdb_stdlog, "-> %s->execution_direction (...)\n", this->beneath ()->shortname ());
  enum exec_direction_kind result
    = this->beneath ()->execution_direction ();
  gdb_printf (gdb_stdlog, "<- %s->execution_direction (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_exec_direction_kind (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::supports_multi_process ()
{
  return this->beneath ()->supports_multi_process ();
}

bool
dummy_target::supports_multi_process ()
{
  return false;
}

bool
debug_target::supports_multi_process ()
{
  gdb_printf (gdb_stdlog, "-> %s->supports_multi_process (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->supports_multi_process ();
  gdb_printf (gdb_stdlog, "<- %s->supports_multi_process (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::supports_enable_disable_tracepoint ()
{
  return this->beneath ()->supports_enable_disable_tracepoint ();
}

bool
dummy_target::supports_enable_disable_tracepoint ()
{
  return false;
}

bool
debug_target::supports_enable_disable_tracepoint ()
{
  gdb_printf (gdb_stdlog, "-> %s->supports_enable_disable_tracepoint (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->supports_enable_disable_tracepoint ();
  gdb_printf (gdb_stdlog, "<- %s->supports_enable_disable_tracepoint (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::supports_disable_randomization ()
{
  return this->beneath ()->supports_disable_randomization ();
}

bool
dummy_target::supports_disable_randomization ()
{
  return find_default_supports_disable_randomization (this);
}

bool
debug_target::supports_disable_randomization ()
{
  gdb_printf (gdb_stdlog, "-> %s->supports_disable_randomization (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->supports_disable_randomization ();
  gdb_printf (gdb_stdlog, "<- %s->supports_disable_randomization (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::supports_string_tracing ()
{
  return this->beneath ()->supports_string_tracing ();
}

bool
dummy_target::supports_string_tracing ()
{
  return false;
}

bool
debug_target::supports_string_tracing ()
{
  gdb_printf (gdb_stdlog, "-> %s->supports_string_tracing (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->supports_string_tracing ();
  gdb_printf (gdb_stdlog, "<- %s->supports_string_tracing (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::supports_evaluation_of_breakpoint_conditions ()
{
  return this->beneath ()->supports_evaluation_of_breakpoint_conditions ();
}

bool
dummy_target::supports_evaluation_of_breakpoint_conditions ()
{
  return false;
}

bool
debug_target::supports_evaluation_of_breakpoint_conditions ()
{
  gdb_printf (gdb_stdlog, "-> %s->supports_evaluation_of_breakpoint_conditions (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->supports_evaluation_of_breakpoint_conditions ();
  gdb_printf (gdb_stdlog, "<- %s->supports_evaluation_of_breakpoint_conditions (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::supports_dumpcore ()
{
  return this->beneath ()->supports_dumpcore ();
}

bool
dummy_target::supports_dumpcore ()
{
  return false;
}

bool
debug_target::supports_dumpcore ()
{
  gdb_printf (gdb_stdlog, "-> %s->supports_dumpcore (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->supports_dumpcore ();
  gdb_printf (gdb_stdlog, "<- %s->supports_dumpcore (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::dumpcore (const char *arg0)
{
  this->beneath ()->dumpcore (arg0);
}

void
dummy_target::dumpcore (const char *arg0)
{
}

void
debug_target::dumpcore (const char *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->dumpcore (...)\n", this->beneath ()->shortname ());
  this->beneath ()->dumpcore (arg0);
  gdb_printf (gdb_stdlog, "<- %s->dumpcore (", this->beneath ()->shortname ());
  target_debug_print_const_char_p (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

bool
target_ops::can_run_breakpoint_commands ()
{
  return this->beneath ()->can_run_breakpoint_commands ();
}

bool
dummy_target::can_run_breakpoint_commands ()
{
  return false;
}

bool
debug_target::can_run_breakpoint_commands ()
{
  gdb_printf (gdb_stdlog, "-> %s->can_run_breakpoint_commands (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->can_run_breakpoint_commands ();
  gdb_printf (gdb_stdlog, "<- %s->can_run_breakpoint_commands (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

struct gdbarch *
target_ops::thread_architecture (ptid_t arg0)
{
  return this->beneath ()->thread_architecture (arg0);
}

struct gdbarch *
dummy_target::thread_architecture (ptid_t arg0)
{
  return NULL;
}

struct gdbarch *
debug_target::thread_architecture (ptid_t arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->thread_architecture (...)\n", this->beneath ()->shortname ());
  struct gdbarch * result
    = this->beneath ()->thread_architecture (arg0);
  gdb_printf (gdb_stdlog, "<- %s->thread_architecture (", this->beneath ()->shortname ());
  target_debug_print_ptid_t (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_gdbarch_p (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::filesystem_is_local ()
{
  return this->beneath ()->filesystem_is_local ();
}

bool
dummy_target::filesystem_is_local ()
{
  return true;
}

bool
debug_target::filesystem_is_local ()
{
  gdb_printf (gdb_stdlog, "-> %s->filesystem_is_local (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->filesystem_is_local ();
  gdb_printf (gdb_stdlog, "<- %s->filesystem_is_local (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::trace_init ()
{
  this->beneath ()->trace_init ();
}

void
dummy_target::trace_init ()
{
  tcomplain ();
}

void
debug_target::trace_init ()
{
  gdb_printf (gdb_stdlog, "-> %s->trace_init (...)\n", this->beneath ()->shortname ());
  this->beneath ()->trace_init ();
  gdb_printf (gdb_stdlog, "<- %s->trace_init (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::download_tracepoint (struct bp_location *arg0)
{
  this->beneath ()->download_tracepoint (arg0);
}

void
dummy_target::download_tracepoint (struct bp_location *arg0)
{
  tcomplain ();
}

void
debug_target::download_tracepoint (struct bp_location *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->download_tracepoint (...)\n", this->beneath ()->shortname ());
  this->beneath ()->download_tracepoint (arg0);
  gdb_printf (gdb_stdlog, "<- %s->download_tracepoint (", this->beneath ()->shortname ());
  target_debug_print_bp_location_p (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

bool
target_ops::can_download_tracepoint ()
{
  return this->beneath ()->can_download_tracepoint ();
}

bool
dummy_target::can_download_tracepoint ()
{
  return false;
}

bool
debug_target::can_download_tracepoint ()
{
  gdb_printf (gdb_stdlog, "-> %s->can_download_tracepoint (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->can_download_tracepoint ();
  gdb_printf (gdb_stdlog, "<- %s->can_download_tracepoint (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::download_trace_state_variable (const trace_state_variable &arg0)
{
  this->beneath ()->download_trace_state_variable (arg0);
}

void
dummy_target::download_trace_state_variable (const trace_state_variable &arg0)
{
  tcomplain ();
}

void
debug_target::download_trace_state_variable (const trace_state_variable &arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->download_trace_state_variable (...)\n", this->beneath ()->shortname ());
  this->beneath ()->download_trace_state_variable (arg0);
  gdb_printf (gdb_stdlog, "<- %s->download_trace_state_variable (", this->beneath ()->shortname ());
  target_debug_print_const_trace_state_variable_r (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::enable_tracepoint (struct bp_location *arg0)
{
  this->beneath ()->enable_tracepoint (arg0);
}

void
dummy_target::enable_tracepoint (struct bp_location *arg0)
{
  tcomplain ();
}

void
debug_target::enable_tracepoint (struct bp_location *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->enable_tracepoint (...)\n", this->beneath ()->shortname ());
  this->beneath ()->enable_tracepoint (arg0);
  gdb_printf (gdb_stdlog, "<- %s->enable_tracepoint (", this->beneath ()->shortname ());
  target_debug_print_bp_location_p (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::disable_tracepoint (struct bp_location *arg0)
{
  this->beneath ()->disable_tracepoint (arg0);
}

void
dummy_target::disable_tracepoint (struct bp_location *arg0)
{
  tcomplain ();
}

void
debug_target::disable_tracepoint (struct bp_location *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->disable_tracepoint (...)\n", this->beneath ()->shortname ());
  this->beneath ()->disable_tracepoint (arg0);
  gdb_printf (gdb_stdlog, "<- %s->disable_tracepoint (", this->beneath ()->shortname ());
  target_debug_print_bp_location_p (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::trace_set_readonly_regions ()
{
  this->beneath ()->trace_set_readonly_regions ();
}

void
dummy_target::trace_set_readonly_regions ()
{
  tcomplain ();
}

void
debug_target::trace_set_readonly_regions ()
{
  gdb_printf (gdb_stdlog, "-> %s->trace_set_readonly_regions (...)\n", this->beneath ()->shortname ());
  this->beneath ()->trace_set_readonly_regions ();
  gdb_printf (gdb_stdlog, "<- %s->trace_set_readonly_regions (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::trace_start ()
{
  this->beneath ()->trace_start ();
}

void
dummy_target::trace_start ()
{
  tcomplain ();
}

void
debug_target::trace_start ()
{
  gdb_printf (gdb_stdlog, "-> %s->trace_start (...)\n", this->beneath ()->shortname ());
  this->beneath ()->trace_start ();
  gdb_printf (gdb_stdlog, "<- %s->trace_start (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

int
target_ops::get_trace_status (struct trace_status *arg0)
{
  return this->beneath ()->get_trace_status (arg0);
}

int
dummy_target::get_trace_status (struct trace_status *arg0)
{
  return -1;
}

int
debug_target::get_trace_status (struct trace_status *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->get_trace_status (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->get_trace_status (arg0);
  gdb_printf (gdb_stdlog, "<- %s->get_trace_status (", this->beneath ()->shortname ());
  target_debug_print_trace_status_p (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::get_tracepoint_status (tracepoint *arg0, struct uploaded_tp *arg1)
{
  this->beneath ()->get_tracepoint_status (arg0, arg1);
}

void
dummy_target::get_tracepoint_status (tracepoint *arg0, struct uploaded_tp *arg1)
{
  tcomplain ();
}

void
debug_target::get_tracepoint_status (tracepoint *arg0, struct uploaded_tp *arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->get_tracepoint_status (...)\n", this->beneath ()->shortname ());
  this->beneath ()->get_tracepoint_status (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->get_tracepoint_status (", this->beneath ()->shortname ());
  target_debug_print_tracepoint_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_uploaded_tp_p (arg1);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::trace_stop ()
{
  this->beneath ()->trace_stop ();
}

void
dummy_target::trace_stop ()
{
  tcomplain ();
}

void
debug_target::trace_stop ()
{
  gdb_printf (gdb_stdlog, "-> %s->trace_stop (...)\n", this->beneath ()->shortname ());
  this->beneath ()->trace_stop ();
  gdb_printf (gdb_stdlog, "<- %s->trace_stop (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

int
target_ops::trace_find (enum trace_find_type arg0, int arg1, CORE_ADDR arg2, CORE_ADDR arg3, int *arg4)
{
  return this->beneath ()->trace_find (arg0, arg1, arg2, arg3, arg4);
}

int
dummy_target::trace_find (enum trace_find_type arg0, int arg1, CORE_ADDR arg2, CORE_ADDR arg3, int *arg4)
{
  return -1;
}

int
debug_target::trace_find (enum trace_find_type arg0, int arg1, CORE_ADDR arg2, CORE_ADDR arg3, int *arg4)
{
  gdb_printf (gdb_stdlog, "-> %s->trace_find (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->trace_find (arg0, arg1, arg2, arg3, arg4);
  gdb_printf (gdb_stdlog, "<- %s->trace_find (", this->beneath ()->shortname ());
  target_debug_print_trace_find_type (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg2);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg3);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int_p (arg4);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::get_trace_state_variable_value (int arg0, LONGEST *arg1)
{
  return this->beneath ()->get_trace_state_variable_value (arg0, arg1);
}

bool
dummy_target::get_trace_state_variable_value (int arg0, LONGEST *arg1)
{
  return false;
}

bool
debug_target::get_trace_state_variable_value (int arg0, LONGEST *arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->get_trace_state_variable_value (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->get_trace_state_variable_value (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->get_trace_state_variable_value (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_LONGEST_p (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::save_trace_data (const char *arg0)
{
  return this->beneath ()->save_trace_data (arg0);
}

int
dummy_target::save_trace_data (const char *arg0)
{
  tcomplain ();
}

int
debug_target::save_trace_data (const char *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->save_trace_data (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->save_trace_data (arg0);
  gdb_printf (gdb_stdlog, "<- %s->save_trace_data (", this->beneath ()->shortname ());
  target_debug_print_const_char_p (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::upload_tracepoints (struct uploaded_tp **arg0)
{
  return this->beneath ()->upload_tracepoints (arg0);
}

int
dummy_target::upload_tracepoints (struct uploaded_tp **arg0)
{
  return 0;
}

int
debug_target::upload_tracepoints (struct uploaded_tp **arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->upload_tracepoints (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->upload_tracepoints (arg0);
  gdb_printf (gdb_stdlog, "<- %s->upload_tracepoints (", this->beneath ()->shortname ());
  target_debug_print_uploaded_tp_pp (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::upload_trace_state_variables (struct uploaded_tsv **arg0)
{
  return this->beneath ()->upload_trace_state_variables (arg0);
}

int
dummy_target::upload_trace_state_variables (struct uploaded_tsv **arg0)
{
  return 0;
}

int
debug_target::upload_trace_state_variables (struct uploaded_tsv **arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->upload_trace_state_variables (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->upload_trace_state_variables (arg0);
  gdb_printf (gdb_stdlog, "<- %s->upload_trace_state_variables (", this->beneath ()->shortname ());
  target_debug_print_uploaded_tsv_pp (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

LONGEST
target_ops::get_raw_trace_data (gdb_byte *arg0, ULONGEST arg1, LONGEST arg2)
{
  return this->beneath ()->get_raw_trace_data (arg0, arg1, arg2);
}

LONGEST
dummy_target::get_raw_trace_data (gdb_byte *arg0, ULONGEST arg1, LONGEST arg2)
{
  tcomplain ();
}

LONGEST
debug_target::get_raw_trace_data (gdb_byte *arg0, ULONGEST arg1, LONGEST arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->get_raw_trace_data (...)\n", this->beneath ()->shortname ());
  LONGEST result
    = this->beneath ()->get_raw_trace_data (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->get_raw_trace_data (", this->beneath ()->shortname ());
  target_debug_print_gdb_byte_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_LONGEST (arg2);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_LONGEST (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::get_min_fast_tracepoint_insn_len ()
{
  return this->beneath ()->get_min_fast_tracepoint_insn_len ();
}

int
dummy_target::get_min_fast_tracepoint_insn_len ()
{
  return -1;
}

int
debug_target::get_min_fast_tracepoint_insn_len ()
{
  gdb_printf (gdb_stdlog, "-> %s->get_min_fast_tracepoint_insn_len (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->get_min_fast_tracepoint_insn_len ();
  gdb_printf (gdb_stdlog, "<- %s->get_min_fast_tracepoint_insn_len (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::set_disconnected_tracing (int arg0)
{
  this->beneath ()->set_disconnected_tracing (arg0);
}

void
dummy_target::set_disconnected_tracing (int arg0)
{
}

void
debug_target::set_disconnected_tracing (int arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->set_disconnected_tracing (...)\n", this->beneath ()->shortname ());
  this->beneath ()->set_disconnected_tracing (arg0);
  gdb_printf (gdb_stdlog, "<- %s->set_disconnected_tracing (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::set_circular_trace_buffer (int arg0)
{
  this->beneath ()->set_circular_trace_buffer (arg0);
}

void
dummy_target::set_circular_trace_buffer (int arg0)
{
}

void
debug_target::set_circular_trace_buffer (int arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->set_circular_trace_buffer (...)\n", this->beneath ()->shortname ());
  this->beneath ()->set_circular_trace_buffer (arg0);
  gdb_printf (gdb_stdlog, "<- %s->set_circular_trace_buffer (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::set_trace_buffer_size (LONGEST arg0)
{
  this->beneath ()->set_trace_buffer_size (arg0);
}

void
dummy_target::set_trace_buffer_size (LONGEST arg0)
{
}

void
debug_target::set_trace_buffer_size (LONGEST arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->set_trace_buffer_size (...)\n", this->beneath ()->shortname ());
  this->beneath ()->set_trace_buffer_size (arg0);
  gdb_printf (gdb_stdlog, "<- %s->set_trace_buffer_size (", this->beneath ()->shortname ());
  target_debug_print_LONGEST (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

bool
target_ops::set_trace_notes (const char *arg0, const char *arg1, const char *arg2)
{
  return this->beneath ()->set_trace_notes (arg0, arg1, arg2);
}

bool
dummy_target::set_trace_notes (const char *arg0, const char *arg1, const char *arg2)
{
  return false;
}

bool
debug_target::set_trace_notes (const char *arg0, const char *arg1, const char *arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->set_trace_notes (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->set_trace_notes (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->set_trace_notes (", this->beneath ()->shortname ());
  target_debug_print_const_char_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_const_char_p (arg2);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::core_of_thread (ptid_t arg0)
{
  return this->beneath ()->core_of_thread (arg0);
}

int
dummy_target::core_of_thread (ptid_t arg0)
{
  return -1;
}

int
debug_target::core_of_thread (ptid_t arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->core_of_thread (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->core_of_thread (arg0);
  gdb_printf (gdb_stdlog, "<- %s->core_of_thread (", this->beneath ()->shortname ());
  target_debug_print_ptid_t (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

int
target_ops::verify_memory (const gdb_byte *arg0, CORE_ADDR arg1, ULONGEST arg2)
{
  return this->beneath ()->verify_memory (arg0, arg1, arg2);
}

int
dummy_target::verify_memory (const gdb_byte *arg0, CORE_ADDR arg1, ULONGEST arg2)
{
  return default_verify_memory (this, arg0, arg1, arg2);
}

int
debug_target::verify_memory (const gdb_byte *arg0, CORE_ADDR arg1, ULONGEST arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->verify_memory (...)\n", this->beneath ()->shortname ());
  int result
    = this->beneath ()->verify_memory (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->verify_memory (", this->beneath ()->shortname ());
  target_debug_print_const_gdb_byte_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg2);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_int (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::get_tib_address (ptid_t arg0, CORE_ADDR *arg1)
{
  return this->beneath ()->get_tib_address (arg0, arg1);
}

bool
dummy_target::get_tib_address (ptid_t arg0, CORE_ADDR *arg1)
{
  tcomplain ();
}

bool
debug_target::get_tib_address (ptid_t arg0, CORE_ADDR *arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->get_tib_address (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->get_tib_address (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->get_tib_address (", this->beneath ()->shortname ());
  target_debug_print_ptid_t (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_CORE_ADDR_p (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::set_permissions ()
{
  this->beneath ()->set_permissions ();
}

void
dummy_target::set_permissions ()
{
}

void
debug_target::set_permissions ()
{
  gdb_printf (gdb_stdlog, "-> %s->set_permissions (...)\n", this->beneath ()->shortname ());
  this->beneath ()->set_permissions ();
  gdb_printf (gdb_stdlog, "<- %s->set_permissions (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

bool
target_ops::static_tracepoint_marker_at (CORE_ADDR arg0, static_tracepoint_marker *arg1)
{
  return this->beneath ()->static_tracepoint_marker_at (arg0, arg1);
}

bool
dummy_target::static_tracepoint_marker_at (CORE_ADDR arg0, static_tracepoint_marker *arg1)
{
  return false;
}

bool
debug_target::static_tracepoint_marker_at (CORE_ADDR arg0, static_tracepoint_marker *arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->static_tracepoint_marker_at (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->static_tracepoint_marker_at (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->static_tracepoint_marker_at (", this->beneath ()->shortname ());
  target_debug_print_CORE_ADDR (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_static_tracepoint_marker_p (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

std::vector<static_tracepoint_marker>
target_ops::static_tracepoint_markers_by_strid (const char *arg0)
{
  return this->beneath ()->static_tracepoint_markers_by_strid (arg0);
}

std::vector<static_tracepoint_marker>
dummy_target::static_tracepoint_markers_by_strid (const char *arg0)
{
  tcomplain ();
}

std::vector<static_tracepoint_marker>
debug_target::static_tracepoint_markers_by_strid (const char *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->static_tracepoint_markers_by_strid (...)\n", this->beneath ()->shortname ());
  std::vector<static_tracepoint_marker> result
    = this->beneath ()->static_tracepoint_markers_by_strid (arg0);
  gdb_printf (gdb_stdlog, "<- %s->static_tracepoint_markers_by_strid (", this->beneath ()->shortname ());
  target_debug_print_const_char_p (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_std_vector_static_tracepoint_marker (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

traceframe_info_up
target_ops::traceframe_info ()
{
  return this->beneath ()->traceframe_info ();
}

traceframe_info_up
dummy_target::traceframe_info ()
{
  tcomplain ();
}

traceframe_info_up
debug_target::traceframe_info ()
{
  gdb_printf (gdb_stdlog, "-> %s->traceframe_info (...)\n", this->beneath ()->shortname ());
  traceframe_info_up result
    = this->beneath ()->traceframe_info ();
  gdb_printf (gdb_stdlog, "<- %s->traceframe_info (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_traceframe_info_up (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::use_agent (bool arg0)
{
  return this->beneath ()->use_agent (arg0);
}

bool
dummy_target::use_agent (bool arg0)
{
  tcomplain ();
}

bool
debug_target::use_agent (bool arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->use_agent (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->use_agent (arg0);
  gdb_printf (gdb_stdlog, "<- %s->use_agent (", this->beneath ()->shortname ());
  target_debug_print_bool (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::can_use_agent ()
{
  return this->beneath ()->can_use_agent ();
}

bool
dummy_target::can_use_agent ()
{
  return false;
}

bool
debug_target::can_use_agent ()
{
  gdb_printf (gdb_stdlog, "-> %s->can_use_agent (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->can_use_agent ();
  gdb_printf (gdb_stdlog, "<- %s->can_use_agent (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

struct btrace_target_info *
target_ops::enable_btrace (thread_info *arg0, const struct btrace_config *arg1)
{
  return this->beneath ()->enable_btrace (arg0, arg1);
}

struct btrace_target_info *
dummy_target::enable_btrace (thread_info *arg0, const struct btrace_config *arg1)
{
  tcomplain ();
}

struct btrace_target_info *
debug_target::enable_btrace (thread_info *arg0, const struct btrace_config *arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->enable_btrace (...)\n", this->beneath ()->shortname ());
  struct btrace_target_info * result
    = this->beneath ()->enable_btrace (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->enable_btrace (", this->beneath ()->shortname ());
  target_debug_print_thread_info_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_const_btrace_config_p (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_btrace_target_info_p (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::disable_btrace (struct btrace_target_info *arg0)
{
  this->beneath ()->disable_btrace (arg0);
}

void
dummy_target::disable_btrace (struct btrace_target_info *arg0)
{
  tcomplain ();
}

void
debug_target::disable_btrace (struct btrace_target_info *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->disable_btrace (...)\n", this->beneath ()->shortname ());
  this->beneath ()->disable_btrace (arg0);
  gdb_printf (gdb_stdlog, "<- %s->disable_btrace (", this->beneath ()->shortname ());
  target_debug_print_btrace_target_info_p (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::teardown_btrace (struct btrace_target_info *arg0)
{
  this->beneath ()->teardown_btrace (arg0);
}

void
dummy_target::teardown_btrace (struct btrace_target_info *arg0)
{
  tcomplain ();
}

void
debug_target::teardown_btrace (struct btrace_target_info *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->teardown_btrace (...)\n", this->beneath ()->shortname ());
  this->beneath ()->teardown_btrace (arg0);
  gdb_printf (gdb_stdlog, "<- %s->teardown_btrace (", this->beneath ()->shortname ());
  target_debug_print_btrace_target_info_p (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

enum btrace_error
target_ops::read_btrace (struct btrace_data *arg0, struct btrace_target_info *arg1, enum btrace_read_type arg2)
{
  return this->beneath ()->read_btrace (arg0, arg1, arg2);
}

enum btrace_error
dummy_target::read_btrace (struct btrace_data *arg0, struct btrace_target_info *arg1, enum btrace_read_type arg2)
{
  tcomplain ();
}

enum btrace_error
debug_target::read_btrace (struct btrace_data *arg0, struct btrace_target_info *arg1, enum btrace_read_type arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->read_btrace (...)\n", this->beneath ()->shortname ());
  enum btrace_error result
    = this->beneath ()->read_btrace (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->read_btrace (", this->beneath ()->shortname ());
  target_debug_print_btrace_data_p (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_btrace_target_info_p (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_btrace_read_type (arg2);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_btrace_error (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

const struct btrace_config *
target_ops::btrace_conf (const struct btrace_target_info *arg0)
{
  return this->beneath ()->btrace_conf (arg0);
}

const struct btrace_config *
dummy_target::btrace_conf (const struct btrace_target_info *arg0)
{
  return NULL;
}

const struct btrace_config *
debug_target::btrace_conf (const struct btrace_target_info *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->btrace_conf (...)\n", this->beneath ()->shortname ());
  const struct btrace_config * result
    = this->beneath ()->btrace_conf (arg0);
  gdb_printf (gdb_stdlog, "<- %s->btrace_conf (", this->beneath ()->shortname ());
  target_debug_print_const_btrace_target_info_p (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_const_btrace_config_p (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

enum record_method
target_ops::record_method (ptid_t arg0)
{
  return this->beneath ()->record_method (arg0);
}

enum record_method
dummy_target::record_method (ptid_t arg0)
{
  return RECORD_METHOD_NONE;
}

enum record_method
debug_target::record_method (ptid_t arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->record_method (...)\n", this->beneath ()->shortname ());
  enum record_method result
    = this->beneath ()->record_method (arg0);
  gdb_printf (gdb_stdlog, "<- %s->record_method (", this->beneath ()->shortname ());
  target_debug_print_ptid_t (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_record_method (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::stop_recording ()
{
  this->beneath ()->stop_recording ();
}

void
dummy_target::stop_recording ()
{
}

void
debug_target::stop_recording ()
{
  gdb_printf (gdb_stdlog, "-> %s->stop_recording (...)\n", this->beneath ()->shortname ());
  this->beneath ()->stop_recording ();
  gdb_printf (gdb_stdlog, "<- %s->stop_recording (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::info_record ()
{
  this->beneath ()->info_record ();
}

void
dummy_target::info_record ()
{
}

void
debug_target::info_record ()
{
  gdb_printf (gdb_stdlog, "-> %s->info_record (...)\n", this->beneath ()->shortname ());
  this->beneath ()->info_record ();
  gdb_printf (gdb_stdlog, "<- %s->info_record (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::save_record (const char *arg0)
{
  this->beneath ()->save_record (arg0);
}

void
dummy_target::save_record (const char *arg0)
{
  tcomplain ();
}

void
debug_target::save_record (const char *arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->save_record (...)\n", this->beneath ()->shortname ());
  this->beneath ()->save_record (arg0);
  gdb_printf (gdb_stdlog, "<- %s->save_record (", this->beneath ()->shortname ());
  target_debug_print_const_char_p (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

bool
target_ops::supports_delete_record ()
{
  return this->beneath ()->supports_delete_record ();
}

bool
dummy_target::supports_delete_record ()
{
  return false;
}

bool
debug_target::supports_delete_record ()
{
  gdb_printf (gdb_stdlog, "-> %s->supports_delete_record (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->supports_delete_record ();
  gdb_printf (gdb_stdlog, "<- %s->supports_delete_record (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::delete_record ()
{
  this->beneath ()->delete_record ();
}

void
dummy_target::delete_record ()
{
  tcomplain ();
}

void
debug_target::delete_record ()
{
  gdb_printf (gdb_stdlog, "-> %s->delete_record (...)\n", this->beneath ()->shortname ());
  this->beneath ()->delete_record ();
  gdb_printf (gdb_stdlog, "<- %s->delete_record (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

bool
target_ops::record_is_replaying (ptid_t arg0)
{
  return this->beneath ()->record_is_replaying (arg0);
}

bool
dummy_target::record_is_replaying (ptid_t arg0)
{
  return false;
}

bool
debug_target::record_is_replaying (ptid_t arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->record_is_replaying (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->record_is_replaying (arg0);
  gdb_printf (gdb_stdlog, "<- %s->record_is_replaying (", this->beneath ()->shortname ());
  target_debug_print_ptid_t (arg0);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::record_will_replay (ptid_t arg0, int arg1)
{
  return this->beneath ()->record_will_replay (arg0, arg1);
}

bool
dummy_target::record_will_replay (ptid_t arg0, int arg1)
{
  return false;
}

bool
debug_target::record_will_replay (ptid_t arg0, int arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->record_will_replay (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->record_will_replay (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->record_will_replay (", this->beneath ()->shortname ());
  target_debug_print_ptid_t (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::record_stop_replaying ()
{
  this->beneath ()->record_stop_replaying ();
}

void
dummy_target::record_stop_replaying ()
{
}

void
debug_target::record_stop_replaying ()
{
  gdb_printf (gdb_stdlog, "-> %s->record_stop_replaying (...)\n", this->beneath ()->shortname ());
  this->beneath ()->record_stop_replaying ();
  gdb_printf (gdb_stdlog, "<- %s->record_stop_replaying (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::goto_record_begin ()
{
  this->beneath ()->goto_record_begin ();
}

void
dummy_target::goto_record_begin ()
{
  tcomplain ();
}

void
debug_target::goto_record_begin ()
{
  gdb_printf (gdb_stdlog, "-> %s->goto_record_begin (...)\n", this->beneath ()->shortname ());
  this->beneath ()->goto_record_begin ();
  gdb_printf (gdb_stdlog, "<- %s->goto_record_begin (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::goto_record_end ()
{
  this->beneath ()->goto_record_end ();
}

void
dummy_target::goto_record_end ()
{
  tcomplain ();
}

void
debug_target::goto_record_end ()
{
  gdb_printf (gdb_stdlog, "-> %s->goto_record_end (...)\n", this->beneath ()->shortname ());
  this->beneath ()->goto_record_end ();
  gdb_printf (gdb_stdlog, "<- %s->goto_record_end (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::goto_record (ULONGEST arg0)
{
  this->beneath ()->goto_record (arg0);
}

void
dummy_target::goto_record (ULONGEST arg0)
{
  tcomplain ();
}

void
debug_target::goto_record (ULONGEST arg0)
{
  gdb_printf (gdb_stdlog, "-> %s->goto_record (...)\n", this->beneath ()->shortname ());
  this->beneath ()->goto_record (arg0);
  gdb_printf (gdb_stdlog, "<- %s->goto_record (", this->beneath ()->shortname ());
  target_debug_print_ULONGEST (arg0);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::insn_history (int arg0, gdb_disassembly_flags arg1)
{
  this->beneath ()->insn_history (arg0, arg1);
}

void
dummy_target::insn_history (int arg0, gdb_disassembly_flags arg1)
{
  tcomplain ();
}

void
debug_target::insn_history (int arg0, gdb_disassembly_flags arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->insn_history (...)\n", this->beneath ()->shortname ());
  this->beneath ()->insn_history (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->insn_history (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_gdb_disassembly_flags (arg1);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::insn_history_from (ULONGEST arg0, int arg1, gdb_disassembly_flags arg2)
{
  this->beneath ()->insn_history_from (arg0, arg1, arg2);
}

void
dummy_target::insn_history_from (ULONGEST arg0, int arg1, gdb_disassembly_flags arg2)
{
  tcomplain ();
}

void
debug_target::insn_history_from (ULONGEST arg0, int arg1, gdb_disassembly_flags arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->insn_history_from (...)\n", this->beneath ()->shortname ());
  this->beneath ()->insn_history_from (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->insn_history_from (", this->beneath ()->shortname ());
  target_debug_print_ULONGEST (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_gdb_disassembly_flags (arg2);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::insn_history_range (ULONGEST arg0, ULONGEST arg1, gdb_disassembly_flags arg2)
{
  this->beneath ()->insn_history_range (arg0, arg1, arg2);
}

void
dummy_target::insn_history_range (ULONGEST arg0, ULONGEST arg1, gdb_disassembly_flags arg2)
{
  tcomplain ();
}

void
debug_target::insn_history_range (ULONGEST arg0, ULONGEST arg1, gdb_disassembly_flags arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->insn_history_range (...)\n", this->beneath ()->shortname ());
  this->beneath ()->insn_history_range (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->insn_history_range (", this->beneath ()->shortname ());
  target_debug_print_ULONGEST (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_gdb_disassembly_flags (arg2);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::call_history (int arg0, record_print_flags arg1)
{
  this->beneath ()->call_history (arg0, arg1);
}

void
dummy_target::call_history (int arg0, record_print_flags arg1)
{
  tcomplain ();
}

void
debug_target::call_history (int arg0, record_print_flags arg1)
{
  gdb_printf (gdb_stdlog, "-> %s->call_history (...)\n", this->beneath ()->shortname ());
  this->beneath ()->call_history (arg0, arg1);
  gdb_printf (gdb_stdlog, "<- %s->call_history (", this->beneath ()->shortname ());
  target_debug_print_int (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_record_print_flags (arg1);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::call_history_from (ULONGEST arg0, int arg1, record_print_flags arg2)
{
  this->beneath ()->call_history_from (arg0, arg1, arg2);
}

void
dummy_target::call_history_from (ULONGEST arg0, int arg1, record_print_flags arg2)
{
  tcomplain ();
}

void
debug_target::call_history_from (ULONGEST arg0, int arg1, record_print_flags arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->call_history_from (...)\n", this->beneath ()->shortname ());
  this->beneath ()->call_history_from (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->call_history_from (", this->beneath ()->shortname ());
  target_debug_print_ULONGEST (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_record_print_flags (arg2);
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::call_history_range (ULONGEST arg0, ULONGEST arg1, record_print_flags arg2)
{
  this->beneath ()->call_history_range (arg0, arg1, arg2);
}

void
dummy_target::call_history_range (ULONGEST arg0, ULONGEST arg1, record_print_flags arg2)
{
  tcomplain ();
}

void
debug_target::call_history_range (ULONGEST arg0, ULONGEST arg1, record_print_flags arg2)
{
  gdb_printf (gdb_stdlog, "-> %s->call_history_range (...)\n", this->beneath ()->shortname ());
  this->beneath ()->call_history_range (arg0, arg1, arg2);
  gdb_printf (gdb_stdlog, "<- %s->call_history_range (", this->beneath ()->shortname ());
  target_debug_print_ULONGEST (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_ULONGEST (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_record_print_flags (arg2);
  gdb_puts (")\n", gdb_stdlog);
}

bool
target_ops::augmented_libraries_svr4_read ()
{
  return this->beneath ()->augmented_libraries_svr4_read ();
}

bool
dummy_target::augmented_libraries_svr4_read ()
{
  return false;
}

bool
debug_target::augmented_libraries_svr4_read ()
{
  gdb_printf (gdb_stdlog, "-> %s->augmented_libraries_svr4_read (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->augmented_libraries_svr4_read ();
  gdb_printf (gdb_stdlog, "<- %s->augmented_libraries_svr4_read (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

const struct frame_unwind *
target_ops::get_unwinder ()
{
  return this->beneath ()->get_unwinder ();
}

const struct frame_unwind *
dummy_target::get_unwinder ()
{
  return NULL;
}

const struct frame_unwind *
debug_target::get_unwinder ()
{
  gdb_printf (gdb_stdlog, "-> %s->get_unwinder (...)\n", this->beneath ()->shortname ());
  const struct frame_unwind * result
    = this->beneath ()->get_unwinder ();
  gdb_printf (gdb_stdlog, "<- %s->get_unwinder (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_const_frame_unwind_p (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

const struct frame_unwind *
target_ops::get_tailcall_unwinder ()
{
  return this->beneath ()->get_tailcall_unwinder ();
}

const struct frame_unwind *
dummy_target::get_tailcall_unwinder ()
{
  return NULL;
}

const struct frame_unwind *
debug_target::get_tailcall_unwinder ()
{
  gdb_printf (gdb_stdlog, "-> %s->get_tailcall_unwinder (...)\n", this->beneath ()->shortname ());
  const struct frame_unwind * result
    = this->beneath ()->get_tailcall_unwinder ();
  gdb_printf (gdb_stdlog, "<- %s->get_tailcall_unwinder (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_const_frame_unwind_p (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

void
target_ops::prepare_to_generate_core ()
{
  this->beneath ()->prepare_to_generate_core ();
}

void
dummy_target::prepare_to_generate_core ()
{
}

void
debug_target::prepare_to_generate_core ()
{
  gdb_printf (gdb_stdlog, "-> %s->prepare_to_generate_core (...)\n", this->beneath ()->shortname ());
  this->beneath ()->prepare_to_generate_core ();
  gdb_printf (gdb_stdlog, "<- %s->prepare_to_generate_core (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

void
target_ops::done_generating_core ()
{
  this->beneath ()->done_generating_core ();
}

void
dummy_target::done_generating_core ()
{
}

void
debug_target::done_generating_core ()
{
  gdb_printf (gdb_stdlog, "-> %s->done_generating_core (...)\n", this->beneath ()->shortname ());
  this->beneath ()->done_generating_core ();
  gdb_printf (gdb_stdlog, "<- %s->done_generating_core (", this->beneath ()->shortname ());
  gdb_puts (")\n", gdb_stdlog);
}

bool
target_ops::supports_memory_tagging ()
{
  return this->beneath ()->supports_memory_tagging ();
}

bool
dummy_target::supports_memory_tagging ()
{
  return false;
}

bool
debug_target::supports_memory_tagging ()
{
  gdb_printf (gdb_stdlog, "-> %s->supports_memory_tagging (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->supports_memory_tagging ();
  gdb_printf (gdb_stdlog, "<- %s->supports_memory_tagging (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::fetch_memtags (CORE_ADDR arg0, size_t arg1, gdb::byte_vector &arg2, int arg3)
{
  return this->beneath ()->fetch_memtags (arg0, arg1, arg2, arg3);
}

bool
dummy_target::fetch_memtags (CORE_ADDR arg0, size_t arg1, gdb::byte_vector &arg2, int arg3)
{
  tcomplain ();
}

bool
debug_target::fetch_memtags (CORE_ADDR arg0, size_t arg1, gdb::byte_vector &arg2, int arg3)
{
  gdb_printf (gdb_stdlog, "-> %s->fetch_memtags (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->fetch_memtags (arg0, arg1, arg2, arg3);
  gdb_printf (gdb_stdlog, "<- %s->fetch_memtags (", this->beneath ()->shortname ());
  target_debug_print_CORE_ADDR (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_size_t (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_gdb_byte_vector_r (arg2);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg3);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

bool
target_ops::store_memtags (CORE_ADDR arg0, size_t arg1, const gdb::byte_vector &arg2, int arg3)
{
  return this->beneath ()->store_memtags (arg0, arg1, arg2, arg3);
}

bool
dummy_target::store_memtags (CORE_ADDR arg0, size_t arg1, const gdb::byte_vector &arg2, int arg3)
{
  tcomplain ();
}

bool
debug_target::store_memtags (CORE_ADDR arg0, size_t arg1, const gdb::byte_vector &arg2, int arg3)
{
  gdb_printf (gdb_stdlog, "-> %s->store_memtags (...)\n", this->beneath ()->shortname ());
  bool result
    = this->beneath ()->store_memtags (arg0, arg1, arg2, arg3);
  gdb_printf (gdb_stdlog, "<- %s->store_memtags (", this->beneath ()->shortname ());
  target_debug_print_CORE_ADDR (arg0);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_size_t (arg1);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_const_gdb_byte_vector_r (arg2);
  gdb_puts (", ", gdb_stdlog);
  target_debug_print_int (arg3);
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_bool (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}

x86_xsave_layout
target_ops::fetch_x86_xsave_layout ()
{
  return this->beneath ()->fetch_x86_xsave_layout ();
}

x86_xsave_layout
dummy_target::fetch_x86_xsave_layout ()
{
  return x86_xsave_layout ();
}

x86_xsave_layout
debug_target::fetch_x86_xsave_layout ()
{
  gdb_printf (gdb_stdlog, "-> %s->fetch_x86_xsave_layout (...)\n", this->beneath ()->shortname ());
  x86_xsave_layout result
    = this->beneath ()->fetch_x86_xsave_layout ();
  gdb_printf (gdb_stdlog, "<- %s->fetch_x86_xsave_layout (", this->beneath ()->shortname ());
  gdb_puts (") = ", gdb_stdlog);
  target_debug_print_x86_xsave_layout (result);
  gdb_puts ("\n", gdb_stdlog);
  return result;
}
