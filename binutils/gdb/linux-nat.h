/* Native debugging support for GNU/Linux (LWP layer).

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#ifndef LINUX_NAT_H
#define LINUX_NAT_H

#include "nat/linux-nat.h"
#include "inf-ptrace.h"
#include "target.h"
#include <signal.h>

/* A prototype generic GNU/Linux target.  A concrete instance should
   override it with local methods.  */

class linux_nat_target : public inf_ptrace_target
{
public:
  linux_nat_target ();
  ~linux_nat_target () override = 0;

  thread_control_capabilities get_thread_control_capabilities () override
  { return tc_schedlock; }

  void create_inferior (const char *, const std::string &,
			char **, int) override;

  void attach (const char *, int) override;

  void detach (inferior *, int) override;

  void resume (ptid_t, int, enum gdb_signal) override;

  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;

  void pass_signals (gdb::array_view<const unsigned char>) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  void kill () override;

  void mourn_inferior () override;
  bool thread_alive (ptid_t ptid) override;

  void update_thread_list () override;

  std::string pid_to_str (ptid_t) override;

  const char *thread_name (struct thread_info *) override;

  bool stopped_by_watchpoint () override;

  bool stopped_data_address (CORE_ADDR *) override;

  bool stopped_by_sw_breakpoint () override;
  bool supports_stopped_by_sw_breakpoint () override;

  bool stopped_by_hw_breakpoint () override;
  bool supports_stopped_by_hw_breakpoint () override;

  void thread_events (int) override;

  bool supports_set_thread_options (gdb_thread_options options) override;

  bool can_async_p () override;

  bool supports_non_stop () override;
  bool always_non_stop_p () override;

  void async (bool) override;

  void stop (ptid_t) override;

  bool supports_multi_process () override;

  bool supports_disable_randomization () override;

  int core_of_thread (ptid_t ptid) override;

  bool filesystem_is_local () override;

  int fileio_open (struct inferior *inf, const char *filename,
		   int flags, int mode, int warn_if_slow,
		   fileio_error *target_errno) override;

  std::optional<std::string>
    fileio_readlink (struct inferior *inf,
		     const char *filename,
		     fileio_error *target_errno) override;

  int fileio_unlink (struct inferior *inf,
		     const char *filename,
		     fileio_error *target_errno) override;

  int insert_fork_catchpoint (int) override;
  int remove_fork_catchpoint (int) override;
  int insert_vfork_catchpoint (int) override;
  int remove_vfork_catchpoint (int) override;

  int insert_exec_catchpoint (int) override;
  int remove_exec_catchpoint (int) override;

  int set_syscall_catchpoint (int pid, bool needed, int any_count,
			      gdb::array_view<const int> syscall_counts) override;

  const char *pid_to_exec_file (int pid) override;

  void post_attach (int) override;

  void follow_fork (inferior *, ptid_t, target_waitkind, bool, bool) override;

  void follow_clone (ptid_t) override;

  std::vector<static_tracepoint_marker>
    static_tracepoint_markers_by_strid (const char *id) override;

  /* Methods that are meant to overridden by the concrete
     arch-specific target instance.  */

  virtual void low_resume (ptid_t ptid, int step, enum gdb_signal sig)
  { inf_ptrace_target::resume (ptid, step, sig); }

  virtual bool low_stopped_by_watchpoint ()
  { return false; }

  virtual bool low_stopped_data_address (CORE_ADDR *addr_p)
  { return false; }

  /* The method to call, if any, when a new thread is attached.  */
  virtual void low_new_thread (struct lwp_info *)
  {}

  /* The method to call, if any, when a thread is destroyed.  */
  virtual void low_delete_thread (struct arch_lwp_info *lp)
  {
    gdb_assert (lp == NULL);
  }

  /* The method to call, if any, when a new fork is attached.  */
  virtual void low_new_fork (struct lwp_info *parent, pid_t child_pid)
  {}

  /* The method to call, if any, when a new clone event is detected.  */
  virtual void low_new_clone (struct lwp_info *parent, pid_t child_lwp)
  {}

  /* The method to call, if any, when a process is no longer
     attached.  */
  virtual void low_forget_process (pid_t pid)
  {}

  /* Hook to call prior to resuming a thread.  */
  virtual void low_prepare_to_resume (struct lwp_info *)
  {}

  /* Convert a ptrace/host siginfo object, into/from the siginfo in
     the layout of the inferiors' architecture.  Returns true if any
     conversion was done; false otherwise, in which case the caller
     does a straight memcpy.  If DIRECTION is 1, then copy from INF to
     PTRACE.  If DIRECTION is 0, copy from PTRACE to INF.  */
  virtual bool low_siginfo_fixup (siginfo_t *ptrace, gdb_byte *inf,
				  int direction)
  { return false; }

  /* SIGTRAP-like breakpoint status events recognizer.  The default
     recognizes SIGTRAP only.  */
  virtual bool low_status_is_event (int status);

protected:

    void post_startup_inferior (ptid_t) override;
};

/* The final/concrete instance.  */
extern linux_nat_target *linux_target;

struct arch_lwp_info;

/* Structure describing an LWP.  */

struct lwp_info : intrusive_list_node<lwp_info>
{
  lwp_info (ptid_t ptid)
    : ptid (ptid)
  {}

  ~lwp_info ();

  DISABLE_COPY_AND_ASSIGN (lwp_info);

  /* The process id of the LWP.  This is a combination of the LWP id
     and overall process id.  */
  ptid_t ptid = null_ptid;

  /* If this flag is set, we need to set the event request flags the
     next time we see this LWP stop.  */
  int must_set_ptrace_flags = 0;

  /* Non-zero if we sent this LWP a SIGSTOP (but the LWP didn't report
     it back yet).  */
  int signalled = 0;

  /* Non-zero if this LWP is stopped.  */
  int stopped = 0;

  /* Non-zero if this LWP will be/has been resumed.  Note that an LWP
     can be marked both as stopped and resumed at the same time.  This
     happens if we try to resume an LWP that has a wait status
     pending.  We shouldn't let the LWP run until that wait status has
     been processed, but we should not report that wait status if GDB
     didn't try to let the LWP run.  */
  int resumed = 0;

  /* The last resume GDB requested on this thread.  */
  resume_kind last_resume_kind = resume_continue;

  /* If non-zero, a pending wait status.  A pending process exit is
     recorded in WAITSTATUS, because W_EXITCODE(0,0) happens to be
     0.  */
  int status = 0;

  /* When 'stopped' is set, this is where the lwp last stopped, with
     decr_pc_after_break already accounted for.  If the LWP is
     running and stepping, this is the address at which the lwp was
     resumed (that is, it's the previous stop PC).  If the LWP is
     running and not stepping, this is 0.  */
  CORE_ADDR stop_pc = 0;

  /* Non-zero if we were stepping this LWP.  */
  int step = 0;

  /* The reason the LWP last stopped, if we need to track it
     (breakpoint, watchpoint, etc.).  */
  target_stop_reason stop_reason = TARGET_STOPPED_BY_NO_REASON;

  /* On architectures where it is possible to know the data address of
     a triggered watchpoint, STOPPED_DATA_ADDRESS_P is non-zero, and
     STOPPED_DATA_ADDRESS contains such data address.  Otherwise,
     STOPPED_DATA_ADDRESS_P is false, and STOPPED_DATA_ADDRESS is
     undefined.  Only valid if STOPPED_BY_WATCHPOINT is true.  */
  int stopped_data_address_p = 0;
  CORE_ADDR stopped_data_address = 0;

  /* Non-zero if we expect a duplicated SIGINT.  */
  int ignore_sigint = 0;

  /* If WAITSTATUS->KIND != TARGET_WAITKIND_IGNORE, the waitstatus for
     this LWP's last event.  This usually corresponds to STATUS above,
     however because W_EXITCODE(0,0) happens to be 0, a process exit
     will be recorded here, while 'status == 0' is ambiguous.  */
  struct target_waitstatus waitstatus;

  /* Signal whether we are in a SYSCALL_ENTRY or
     in a SYSCALL_RETURN event.
     Values:
     - TARGET_WAITKIND_SYSCALL_ENTRY
     - TARGET_WAITKIND_SYSCALL_RETURN */
  enum target_waitkind syscall_state;

  /* The processor core this LWP was last seen on.  */
  int core = -1;

  /* Arch-specific additions.  */
  struct arch_lwp_info *arch_private = nullptr;
};

/* lwp_info iterator and range types.  */

using lwp_info_iterator
  = reference_to_pointer_iterator<intrusive_list<lwp_info>::iterator>;
using lwp_info_range = iterator_range<lwp_info_iterator>;
using lwp_info_safe_range = basic_safe_range<lwp_info_range>;

/* Get an iterable range over all lwps.  */

lwp_info_range all_lwps ();

/* Same as the above, but safe against deletion while iterating.  */

lwp_info_safe_range all_lwps_safe ();

/* Does the current host support PTRACE_GETREGSET?  */
extern enum tribool have_ptrace_getregset;

/* Called from the LWP layer to inform the thread_db layer that PARENT
   spawned CHILD.  Both LWPs are currently stopped.  This function
   does whatever is required to have the child LWP under the
   thread_db's control --- e.g., enabling event reporting.  Returns
   true on success, false if the process isn't using libpthread.  */
extern int thread_db_notice_clone (ptid_t parent, ptid_t child);

/* Return the number of signals used by the threads library.  */
extern unsigned int lin_thread_get_thread_signal_num (void);

/* Return the i-th signal used by the threads library.  */
extern int lin_thread_get_thread_signal (unsigned int i);

/* Find process PID's pending signal set from /proc/pid/status.  */
void linux_proc_pending_signals (int pid, sigset_t *pending,
				 sigset_t *blocked, sigset_t *ignored);

/* For linux_stop_lwp see nat/linux-nat.h.  */

/* Stop all LWPs, synchronously.  (Any events that trigger while LWPs
   are being stopped are left pending.)  */
extern void linux_stop_and_wait_all_lwps (void);

/* Set resumed LWPs running again, as they were before being stopped
   with linux_stop_and_wait_all_lwps.  (LWPS with pending events are
   left stopped.)  */
extern void linux_unstop_all_lwps (void);

/* Update linux-nat internal state when changing from one fork
   to another.  */
void linux_nat_switch_fork (ptid_t new_ptid);

/* Store the saved siginfo associated with PTID in *SIGINFO.
   Return true if it was retrieved successfully, false otherwise (*SIGINFO is
   uninitialized in such case).  */
bool linux_nat_get_siginfo (ptid_t ptid, siginfo_t *siginfo);

#endif /* LINUX_NAT_H */
