/* Target operations for the remote server for GDB.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.

   Contributed by MontaVista Software.

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

#include "server.h"
#include "tracepoint.h"
#include "gdbsupport/byte-vector.h"
#include "hostio.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

process_stratum_target *the_target;

/* See target.h.  */

bool
set_desired_thread ()
{
  client_state &cs = get_client_state ();
  thread_info *found = find_thread_ptid (cs.general_thread);

  if (found == nullptr)
    {
      process_info *proc = find_process_pid (cs.general_thread.pid ());
      if (proc == nullptr)
	{
	  threads_debug_printf
	    ("did not find thread nor process for general_thread %s",
	     cs.general_thread.to_string ().c_str ());
	}
      else
	{
	  threads_debug_printf
	    ("did not find thread for general_thread %s, but found process",
	     cs.general_thread.to_string ().c_str ());
	}
      switch_to_process (proc);
    }
  else
    switch_to_thread (found);

  return (current_thread != NULL);
}

/* See target.h.  */

bool
set_desired_process ()
{
  client_state &cs = get_client_state ();

  process_info *proc = find_process_pid (cs.general_thread.pid ());
  if (proc == nullptr)
    {
      threads_debug_printf
	("did not find process for general_thread %s",
	 cs.general_thread.to_string ().c_str ());
    }
  switch_to_process (proc);

  return proc != nullptr;
}

/* See target.h.  */

int
read_inferior_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
{
  /* At the time of writing, GDB only sends write packets with LEN==0,
     not read packets (see comment in target_write_memory), but it
     doesn't hurt to prevent problems if it ever does, or we're
     connected to some client other than GDB that does.  */
  if (len == 0)
    return 0;

  int res = the_target->read_memory (memaddr, myaddr, len);
  check_mem_read (memaddr, myaddr, len);
  return res;
}

/* See target/target.h.  */

int
target_read_memory (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len)
{
  return read_inferior_memory (memaddr, myaddr, len);
}

/* See target/target.h.  */

int
target_read_uint32 (CORE_ADDR memaddr, uint32_t *result)
{
  return read_inferior_memory (memaddr, (gdb_byte *) result, sizeof (*result));
}

/* See target/target.h.  */

int
target_write_memory (CORE_ADDR memaddr, const unsigned char *myaddr,
		     ssize_t len)
{
  /* GDB may send X packets with LEN==0, for probing packet support.
     If we let such a request go through, then buffer.data() below may
     return NULL, which may confuse target implementations.  Handle it
     here to avoid lower levels having to care about this case.  */
  if (len == 0)
    return 0;

  /* Make a copy of the data because check_mem_write may need to
     update it.  */
  gdb::byte_vector buffer (myaddr, myaddr + len);
  check_mem_write (memaddr, buffer.data (), myaddr, len);
  return the_target->write_memory (memaddr, buffer.data (), len);
}

ptid_t
mywait (ptid_t ptid, struct target_waitstatus *ourstatus,
	target_wait_flags options, int connected_wait)
{
  ptid_t ret;

  if (connected_wait)
    server_waiting = 1;

  ret = target_wait (ptid, ourstatus, options);

  /* We don't expose _LOADED events to gdbserver core.  See the
     `dlls_changed' global.  */
  if (ourstatus->kind () == TARGET_WAITKIND_LOADED)
    ourstatus->set_stopped (GDB_SIGNAL_0);

  /* If GDB is connected through TCP/serial, then GDBserver will most
     probably be running on its own terminal/console, so it's nice to
     print there why is GDBserver exiting.  If however, GDB is
     connected through stdio, then there's no need to spam the GDB
     console with this -- the user will already see the exit through
     regular GDB output, in that same terminal.  */
  if (!remote_connection_is_stdio ())
    {
      if (ourstatus->kind () == TARGET_WAITKIND_EXITED)
	fprintf (stderr,
		 "\nChild exited with status %d\n", ourstatus->exit_status ());
      else if (ourstatus->kind () == TARGET_WAITKIND_SIGNALLED)
	fprintf (stderr, "\nChild terminated with signal = 0x%x (%s)\n",
		 gdb_signal_to_host (ourstatus->sig ()),
		 gdb_signal_to_name (ourstatus->sig ()));
    }

  if (connected_wait)
    server_waiting = 0;

  return ret;
}

/* See target/target.h.  */

void
target_stop_and_wait (ptid_t ptid)
{
  struct target_waitstatus status;
  bool was_non_stop = non_stop;
  struct thread_resume resume_info;

  resume_info.thread = ptid;
  resume_info.kind = resume_stop;
  resume_info.sig = GDB_SIGNAL_0;
  the_target->resume (&resume_info, 1);

  non_stop = true;
  mywait (ptid, &status, 0, 0);
  non_stop = was_non_stop;
}

/* See target/target.h.  */

ptid_t
target_wait (ptid_t ptid, struct target_waitstatus *status,
	     target_wait_flags options)
{
  return the_target->wait (ptid, status, options);
}

/* See target/target.h.  */

void
target_mourn_inferior (ptid_t ptid)
{
  the_target->mourn (find_process_pid (ptid.pid ()));
}

/* See target/target.h.  */

void
target_continue_no_signal (ptid_t ptid)
{
  struct thread_resume resume_info;

  resume_info.thread = ptid;
  resume_info.kind = resume_continue;
  resume_info.sig = GDB_SIGNAL_0;
  the_target->resume (&resume_info, 1);
}

/* See target/target.h.  */

void
target_continue (ptid_t ptid, enum gdb_signal signal)
{
  struct thread_resume resume_info;

  resume_info.thread = ptid;
  resume_info.kind = resume_continue;
  resume_info.sig = gdb_signal_to_host (signal);
  the_target->resume (&resume_info, 1);
}

/* See target/target.h.  */

int
target_supports_multi_process (void)
{
  return the_target->supports_multi_process ();
}

void
set_target_ops (process_stratum_target *target)
{
  the_target = target;
}

/* Convert pid to printable format.  */

std::string
target_pid_to_str (ptid_t ptid)
{
  if (ptid == minus_one_ptid)
    return string_printf("<all threads>");
  else if (ptid == null_ptid)
    return string_printf("<null thread>");
  else if (ptid.tid () != 0)
    return string_printf("Thread %d.0x%s",
			 ptid.pid (),
			 phex_nz (ptid.tid (), sizeof (ULONGEST)));
  else if (ptid.lwp () != 0)
    return string_printf("LWP %d.%ld",
			 ptid.pid (), ptid.lwp ());
  else
    return string_printf("Process %d",
			 ptid.pid ());
}

int
kill_inferior (process_info *proc)
{
  gdb_agent_about_to_close (proc->pid);

  return the_target->kill (proc);
}

/* Define it.  */

target_terminal_state target_terminal::m_terminal_state
  = target_terminal_state::is_ours;

/* See target/target.h.  */

void
target_terminal::init ()
{
  /* Placeholder needed because of fork_inferior.  Not necessary on
     GDBserver.  */
}

/* See target/target.h.  */

void
target_terminal::inferior ()
{
  /* Placeholder needed because of fork_inferior.  Not necessary on
     GDBserver.  */
}

/* See target/target.h.  */

void
target_terminal::ours ()
{
  /* Placeholder needed because of fork_inferior.  Not necessary on
     GDBserver.  */
}

/* See target/target.h.  */

void
target_terminal::ours_for_output (void)
{
  /* Placeholder.  */
}

/* See target/target.h.  */

void
target_terminal::info (const char *arg, int from_tty)
{
  /* Placeholder.  */
}

/* Default implementations of target ops.
   See target.h for definitions.  */

void
process_stratum_target::post_create_inferior ()
{
  /* Nop.  */
}

void
process_stratum_target::look_up_symbols ()
{
  /* Nop.  */
}

bool
process_stratum_target::supports_read_auxv ()
{
  return false;
}

int
process_stratum_target::read_auxv (int pid, CORE_ADDR offset,
				   unsigned char *myaddr, unsigned int len)
{
  gdb_assert_not_reached ("target op read_auxv not supported");
}

bool
process_stratum_target::supports_z_point_type (char z_type)
{
  return false;
}

int
process_stratum_target::insert_point (enum raw_bkpt_type type,
				      CORE_ADDR addr,
				      int size, raw_breakpoint *bp)
{
  return 1;
}

int
process_stratum_target::remove_point (enum raw_bkpt_type type,
				      CORE_ADDR addr,
				      int size, raw_breakpoint *bp)
{
  return 1;
}

bool
process_stratum_target::stopped_by_sw_breakpoint ()
{
  return false;
}

bool
process_stratum_target::supports_stopped_by_sw_breakpoint ()
{
  return false;
}

bool
process_stratum_target::stopped_by_hw_breakpoint ()
{
  return false;
}

bool
process_stratum_target::supports_stopped_by_hw_breakpoint ()
{
  return false;
}

bool
process_stratum_target::supports_hardware_single_step ()
{
  return false;
}

bool
process_stratum_target::stopped_by_watchpoint ()
{
  return false;
}

CORE_ADDR
process_stratum_target::stopped_data_address ()
{
  return 0;
}

bool
process_stratum_target::supports_read_offsets ()
{
  return false;
}

bool
process_stratum_target::supports_memory_tagging ()
{
  return false;
}

bool
process_stratum_target::fetch_memtags (CORE_ADDR address, size_t len,
				       gdb::byte_vector &tags, int type)
{
  gdb_assert_not_reached ("target op fetch_memtags not supported");
}

bool
process_stratum_target::store_memtags (CORE_ADDR address, size_t len,
				       const gdb::byte_vector &tags, int type)
{
  gdb_assert_not_reached ("target op store_memtags not supported");
}

int
process_stratum_target::read_offsets (CORE_ADDR *text, CORE_ADDR *data)
{
  gdb_assert_not_reached ("target op read_offsets not supported");
}

bool
process_stratum_target::supports_get_tls_address ()
{
  return false;
}

int
process_stratum_target::get_tls_address (thread_info *thread,
					 CORE_ADDR offset,
					 CORE_ADDR load_module,
					 CORE_ADDR *address)
{
  gdb_assert_not_reached ("target op get_tls_address not supported");
}

bool
process_stratum_target::supports_qxfer_osdata ()
{
  return false;
}

int
process_stratum_target::qxfer_osdata (const char *annex,
				      unsigned char *readbuf,
				      unsigned const char *writebuf,
				      CORE_ADDR offset, int len)
{
  gdb_assert_not_reached ("target op qxfer_osdata not supported");
}

bool
process_stratum_target::supports_qxfer_siginfo ()
{
  return false;
}

int
process_stratum_target::qxfer_siginfo (const char *annex,
				       unsigned char *readbuf,
				       unsigned const char *writebuf,
				       CORE_ADDR offset, int len)
{
  gdb_assert_not_reached ("target op qxfer_siginfo not supported");
}

bool
process_stratum_target::supports_non_stop ()
{
  return false;
}

bool
process_stratum_target::async (bool enable)
{
  return false;
}

int
process_stratum_target::start_non_stop (bool enable)
{
  if (enable)
    return -1;
  else
    return 0;
}

bool
process_stratum_target::supports_multi_process ()
{
  return false;
}

bool
process_stratum_target::supports_fork_events ()
{
  return false;
}

bool
process_stratum_target::supports_vfork_events ()
{
  return false;
}

gdb_thread_options
process_stratum_target::supported_thread_options ()
{
  return 0;
}

bool
process_stratum_target::supports_exec_events ()
{
  return false;
}

void
process_stratum_target::handle_new_gdb_connection ()
{
  /* Nop.  */
}

int
process_stratum_target::handle_monitor_command (char *mon)
{
  return 0;
}

int
process_stratum_target::core_of_thread (ptid_t ptid)
{
  return -1;
}

bool
process_stratum_target::supports_read_loadmap ()
{
  return false;
}

int
process_stratum_target::read_loadmap (const char *annex,
				      CORE_ADDR offset,
				      unsigned char *myaddr,
				      unsigned int len)
{
  gdb_assert_not_reached ("target op read_loadmap not supported");
}

void
process_stratum_target::process_qsupported
  (gdb::array_view<const char * const> features)
{
  /* Nop.  */
}

bool
process_stratum_target::supports_tracepoints ()
{
  return false;
}

CORE_ADDR
process_stratum_target::read_pc (regcache *regcache)
{
  gdb_assert_not_reached ("process_target::read_pc: Unable to find PC");
}

void
process_stratum_target::write_pc (regcache *regcache, CORE_ADDR pc)
{
  gdb_assert_not_reached ("process_target::write_pc: Unable to update PC");
}

bool
process_stratum_target::supports_thread_stopped ()
{
  return false;
}

bool
process_stratum_target::thread_stopped (thread_info *thread)
{
  gdb_assert_not_reached ("target op thread_stopped not supported");
}

bool
process_stratum_target::any_resumed ()
{
  return true;
}

bool
process_stratum_target::supports_get_tib_address ()
{
  return false;
}

int
process_stratum_target::get_tib_address (ptid_t ptid, CORE_ADDR *address)
{
  gdb_assert_not_reached ("target op get_tib_address not supported");
}

void
process_stratum_target::pause_all (bool freeze)
{
  /* Nop.  */
}

void
process_stratum_target::unpause_all (bool unfreeze)
{
  /* Nop.  */
}

void
process_stratum_target::stabilize_threads ()
{
  /* Nop.  */
}

bool
process_stratum_target::supports_fast_tracepoints ()
{
  return false;
}

int
process_stratum_target::install_fast_tracepoint_jump_pad
  (CORE_ADDR tpoint, CORE_ADDR tpaddr, CORE_ADDR collector,
   CORE_ADDR lockaddr, ULONGEST orig_size, CORE_ADDR *jump_entry,
   CORE_ADDR *trampoline, ULONGEST *trampoline_size,
   unsigned char *jjump_pad_insn, ULONGEST *jjump_pad_insn_size,
   CORE_ADDR *adjusted_insn_addr, CORE_ADDR *adjusted_insn_addr_end,
   char *err)
{
  gdb_assert_not_reached ("target op install_fast_tracepoint_jump_pad "
			  "not supported");
}

int
process_stratum_target::get_min_fast_tracepoint_insn_len ()
{
  return 0;
}

struct emit_ops *
process_stratum_target::emit_ops ()
{
  return nullptr;
}

bool
process_stratum_target::supports_disable_randomization ()
{
  return false;
}

bool
process_stratum_target::supports_qxfer_libraries_svr4 ()
{
  return false;
}

int
process_stratum_target::qxfer_libraries_svr4 (const char *annex,
					      unsigned char *readbuf,
					      unsigned const char *writebuf,
					      CORE_ADDR offset, int len)
{
  gdb_assert_not_reached ("target op qxfer_libraries_svr4 not supported");
}

bool
process_stratum_target::supports_agent ()
{
  return false;
}

bool
process_stratum_target::supports_btrace ()
{
  return false;
}

btrace_target_info *
process_stratum_target::enable_btrace (thread_info *tp,
				       const btrace_config *conf)
{
  error (_("Target does not support branch tracing."));
}

int
process_stratum_target::disable_btrace (btrace_target_info *tinfo)
{
  error (_("Target does not support branch tracing."));
}

int
process_stratum_target::read_btrace (btrace_target_info *tinfo,
				     std::string *buffer,
				     enum btrace_read_type type)
{
  error (_("Target does not support branch tracing."));
}

int
process_stratum_target::read_btrace_conf (const btrace_target_info *tinfo,
					  std::string *buffer)
{
  error (_("Target does not support branch tracing."));
}

bool
process_stratum_target::supports_range_stepping ()
{
  return false;
}

bool
process_stratum_target::supports_pid_to_exec_file ()
{
  return false;
}

const char *
process_stratum_target::pid_to_exec_file (int pid)
{
  gdb_assert_not_reached ("target op pid_to_exec_file not supported");
}

bool
process_stratum_target::supports_multifs ()
{
  return false;
}

int
process_stratum_target::multifs_open (int pid, const char *filename,
				      int flags, mode_t mode)
{
  return open (filename, flags, mode);
}

int
process_stratum_target::multifs_unlink (int pid, const char *filename)
{
  return unlink (filename);
}

ssize_t
process_stratum_target::multifs_readlink (int pid, const char *filename,
					  char *buf, size_t bufsiz)
{
  return readlink (filename, buf, bufsiz);
}

int
process_stratum_target::breakpoint_kind_from_pc (CORE_ADDR *pcptr)
{
  /* The default behavior is to use the size of a breakpoint as the
     kind.  */
  int size = 0;
  sw_breakpoint_from_kind (0, &size);
  return size;
}

int
process_stratum_target::breakpoint_kind_from_current_state (CORE_ADDR *pcptr)
{
  return breakpoint_kind_from_pc (pcptr);
}

const char *
process_stratum_target::thread_name (ptid_t thread)
{
  return nullptr;
}

bool
process_stratum_target::thread_handle (ptid_t ptid, gdb_byte **handle,
				       int *handle_len)
{
  return false;
}

thread_info *
process_stratum_target::thread_pending_parent (thread_info *thread)
{
  return nullptr;
}

thread_info *
process_stratum_target::thread_pending_child (thread_info *thread,
					      target_waitkind *kind)
{
  return nullptr;
}

bool
process_stratum_target::supports_software_single_step ()
{
  return false;
}

bool
process_stratum_target::supports_catch_syscall ()
{
  return false;
}

int
process_stratum_target::get_ipa_tdesc_idx ()
{
  return 0;
}
