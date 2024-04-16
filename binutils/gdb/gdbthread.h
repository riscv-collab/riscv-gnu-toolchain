/* Multi-process/thread control defs for GDB, the GNU debugger.
   Copyright (C) 1987-2024 Free Software Foundation, Inc.
   Contributed by Lynx Real-Time Systems, Inc.  Los Gatos, CA.
   

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

#ifndef GDBTHREAD_H
#define GDBTHREAD_H

struct symtab;

#include "breakpoint.h"
#include "frame.h"
#include "ui-out.h"
#include "btrace.h"
#include "target/waitstatus.h"
#include "target/target.h"
#include "cli/cli-utils.h"
#include "gdbsupport/refcounted-object.h"
#include "gdbsupport/common-gdbthread.h"
#include "gdbsupport/forward-scope-exit.h"
#include "displaced-stepping.h"
#include "gdbsupport/intrusive_list.h"
#include "thread-fsm.h"
#include "language.h"

struct inferior;
struct process_stratum_target;

/* When true, print debug messages related to GDB thread creation and
   deletion.  */

extern bool debug_threads;

/* Print a "threads" debug statement.  */

#define threads_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (debug_threads, "threads", fmt, ##__VA_ARGS__)

/* Frontend view of the thread state.  Possible extensions: stepping,
   finishing, until(ling),...

   NOTE: Since the thread state is not a boolean, most times, you do
   not want to check it with negation.  If you really want to check if
   the thread is stopped,

    use (good):

     if (tp->state == THREAD_STOPPED)

    instead of (bad):

     if (tp->state != THREAD_RUNNING)

   The latter is also true for exited threads, most likely not what
   you want.  */
enum thread_state
{
  /* In the frontend's perpective, the thread is stopped.  */
  THREAD_STOPPED,

  /* In the frontend's perpective, the thread is running.  */
  THREAD_RUNNING,

  /* The thread is listed, but known to have exited.  We keep it
     listed (but not visible) until it's safe to delete it.  */
  THREAD_EXITED,
};

/* STEP_OVER_ALL means step over all subroutine calls.
   STEP_OVER_UNDEBUGGABLE means step over calls to undebuggable functions.
   STEP_OVER_NONE means don't step over any subroutine calls.  */

enum step_over_calls_kind
  {
    STEP_OVER_NONE,
    STEP_OVER_ALL,
    STEP_OVER_UNDEBUGGABLE
  };

/* Inferior thread specific part of `struct infcall_control_state'.

   Inferior process counterpart is `struct inferior_control_state'.  */

struct thread_control_state
{
  /* User/external stepping state.  */

  /* Step-resume or longjmp-resume breakpoint.  */
  struct breakpoint *step_resume_breakpoint = nullptr;

  /* Exception-resume breakpoint.  */
  struct breakpoint *exception_resume_breakpoint = nullptr;

  /* Breakpoints used for software single stepping.  Plural, because
     it may have multiple locations.  E.g., if stepping over a
     conditional branch instruction we can't decode the condition for,
     we'll need to put a breakpoint at the branch destination, and
     another at the instruction after the branch.  */
  struct breakpoint *single_step_breakpoints = nullptr;

  /* Range to single step within.

     If this is nonzero, respond to a single-step signal by continuing
     to step if the pc is in this range.

     If step_range_start and step_range_end are both 1, it means to
     step for a single instruction (FIXME: it might clean up
     wait_for_inferior in a minor way if this were changed to the
     address of the instruction and that address plus one.  But maybe
     not).  */
  CORE_ADDR step_range_start = 0;	/* Inclusive */
  CORE_ADDR step_range_end = 0;		/* Exclusive */

  /* Function the thread was in as of last it started stepping.  */
  struct symbol *step_start_function = nullptr;

  /* If GDB issues a target step request, and this is nonzero, the
     target should single-step this thread once, and then continue
     single-stepping it without GDB core involvement as long as the
     thread stops in the step range above.  If this is zero, the
     target should ignore the step range, and only issue one single
     step.  */
  int may_range_step = 0;

  /* Stack frame address as of when stepping command was issued.
     This is how we know when we step into a subroutine call, and how
     to set the frame for the breakpoint used to step out.  */
  struct frame_id step_frame_id {};

  /* Similarly, the frame ID of the underlying stack frame (skipping
     any inlined frames).  */
  struct frame_id step_stack_frame_id {};

  /* True if the the thread is presently stepping over a breakpoint or
     a watchpoint, either with an inline step over or a displaced (out
     of line) step, and we're now expecting it to report a trap for
     the finished single step.  */
  int trap_expected = 0;

  /* Nonzero if the thread is being proceeded for a "finish" command
     or a similar situation when return value should be printed.  */
  int proceed_to_finish = 0;

  /* Nonzero if the thread is being proceeded for an inferior function
     call.  */
  int in_infcall = 0;

  enum step_over_calls_kind step_over_calls = STEP_OVER_NONE;

  /* Nonzero if stopped due to a step command.  */
  int stop_step = 0;

  /* Chain containing status of breakpoint(s) the thread stopped
     at.  */
  bpstat *stop_bpstat = nullptr;

  /* Whether the command that started the thread was a stepping
     command.  This is used to decide whether "set scheduler-locking
     step" behaves like "on" or "off".  */
  int stepping_command = 0;
};

/* Inferior thread specific part of `struct infcall_suspend_state'.  */

struct thread_suspend_state
{
  /* Last signal that the inferior received (why it stopped).  When
     the thread is resumed, this signal is delivered.  Note: the
     target should not check whether the signal is in pass state,
     because the signal may have been explicitly passed with the
     "signal" command, which overrides "handle nopass".  If the signal
     should be suppressed, the core will take care of clearing this
     before the target is resumed.  */
  enum gdb_signal stop_signal = GDB_SIGNAL_0;

  /* The reason the thread last stopped, if we need to track it
     (breakpoint, watchpoint, etc.)  */
  enum target_stop_reason stop_reason = TARGET_STOPPED_BY_NO_REASON;

  /* The waitstatus for this thread's last event.  */
  struct target_waitstatus waitstatus;
  /* If true WAITSTATUS hasn't been handled yet.  */
  int waitstatus_pending_p = 0;

  /* Record the pc of the thread the last time it stopped.  (This is
     not the current thread's PC as that may have changed since the
     last stop, e.g., "return" command, or "p $pc = 0xf000").

     - If the thread's PC has not changed since the thread last
       stopped, then proceed skips a breakpoint at the current PC,
       otherwise we let the thread run into the breakpoint.

     - If the thread has an unprocessed event pending, as indicated by
       waitstatus_pending_p, this is used in coordination with
       stop_reason: if the thread's PC has changed since the thread
       last stopped, a pending breakpoint waitstatus is discarded.

     - If the thread is running, then this field has its value removed by
       calling stop_pc.reset() (see thread_info::set_executing()).
       Attempting to read a std::optional with no value is undefined
       behaviour and will trigger an assertion error when _GLIBCXX_DEBUG is
       defined, which should make error easier to track down.  */
  std::optional<CORE_ADDR> stop_pc;
};

/* Base class for target-specific thread data.  */
struct private_thread_info
{
  virtual ~private_thread_info () = 0;
};

/* Unique pointer wrapper for private_thread_info.  */
using private_thread_info_up = std::unique_ptr<private_thread_info>;

/* Threads are intrusively refcounted objects.  Being the
   user-selected thread is normally considered an implicit strong
   reference and is thus not accounted in the refcount, unlike
   inferior objects.  This is necessary, because there's no "current
   thread" pointer.  Instead the current thread is inferred from the
   inferior_ptid global.  However, when GDB needs to remember the
   selected thread to later restore it, GDB bumps the thread object's
   refcount, to prevent something deleting the thread object before
   reverting back (e.g., due to a "kill" command).  If the thread
   meanwhile exits before being re-selected, then the thread object is
   left listed in the thread list, but marked with state
   THREAD_EXITED.  (See scoped_restore_current_thread and
   delete_thread).  All other thread references are considered weak
   references.  Placing a thread in the thread list is an implicit
   strong reference, and is thus not accounted for in the thread's
   refcount.

   The intrusive_list_node base links threads in a per-inferior list.  */

class thread_info : public refcounted_object,
		    public intrusive_list_node<thread_info>
{
public:
  explicit thread_info (inferior *inf, ptid_t ptid);
  ~thread_info ();

  bool deletable () const;

  /* Mark this thread as running and notify observers.  */
  void set_running (bool running);

  ptid_t ptid;			/* "Actual process id";
				    In fact, this may be overloaded with 
				    kernel thread id, etc.  */

  /* Each thread has two GDB IDs.

     a) The thread ID (Id).  This consists of the pair of:

	- the number of the thread's inferior and,

	- the thread's thread number in its inferior, aka, the
	  per-inferior thread number.  This number is unique in the
	  inferior but not unique between inferiors.

     b) The global ID (GId).  This is a a single integer unique
	between all inferiors.

     E.g.:

      (gdb) info threads -gid
	Id    GId   Target Id   Frame
      * 1.1   1     Thread A    0x16a09237 in foo () at foo.c:10
	1.2   3     Thread B    0x15ebc6ed in bar () at foo.c:20
	1.3   5     Thread C    0x15ebc6ed in bar () at foo.c:20
	2.1   2     Thread A    0x16a09237 in foo () at foo.c:10
	2.2   4     Thread B    0x15ebc6ed in bar () at foo.c:20
	2.3   6     Thread C    0x15ebc6ed in bar () at foo.c:20

     Above, both inferiors 1 and 2 have threads numbered 1-3, but each
     thread has its own unique global ID.  */

  /* The thread's global GDB thread number.  This is exposed to MI,
     Python/Scheme, visible with "info threads -gid", and is also what
     the $_gthread convenience variable is bound to.  */
  int global_num;

  /* The per-inferior thread number.  This is unique in the inferior
     the thread belongs to, but not unique between inferiors.  This is
     what the $_thread convenience variable is bound to.  */
  int per_inf_num;

  /* The inferior this thread belongs to.  */
  struct inferior *inf;

  /* The user-given name of the thread.

     Returns nullptr if the thread does not have a user-given name.  */
  const char *name () const
  {
    return m_name.get ();
  }

  /* Set the user-given name of the thread.

     Pass nullptr to clear the name.  */
  void set_name (gdb::unique_xmalloc_ptr<char> name)
  {
    m_name = std::move (name);
  }

  bool executing () const
  { return m_executing; }

  /* Set the thread's 'm_executing' field from EXECUTING, and if EXECUTING
     is true also clears the thread's stop_pc.  */
  void set_executing (bool executing);

  bool resumed () const
  { return m_resumed; }

  /* Set the thread's 'm_resumed' field from RESUMED.  The thread may also
     be added to (when RESUMED is true), or removed from (when RESUMED is
     false), the list of threads with a pending wait status.  */
  void set_resumed (bool resumed);

  /* Frontend view of the thread state.  Note that the THREAD_RUNNING/
     THREAD_STOPPED states are different from EXECUTING.  When the
     thread is stopped internally while handling an internal event,
     like a software single-step breakpoint, EXECUTING will be false,
     but STATE will still be THREAD_RUNNING.  */
  enum thread_state state = THREAD_STOPPED;

  /* State of GDB control of inferior thread execution.
     See `struct thread_control_state'.  */
  thread_control_state control;

  /* Save M_SUSPEND to SUSPEND.  */

  void save_suspend_to (thread_suspend_state &suspend) const
  {
    suspend = m_suspend;
  }

  /* Restore M_SUSPEND from SUSPEND.  */

  void restore_suspend_from (const thread_suspend_state &suspend)
  {
    m_suspend = suspend;
  }

  /* Return this thread's stop PC.  This should only be called when it is
     known that stop_pc has a value.  If this function is being used in a
     situation where a thread may not have had a stop_pc assigned, then
     stop_pc_p() can be used to check if the stop_pc is defined.  */

  CORE_ADDR stop_pc () const
  {
    gdb_assert (m_suspend.stop_pc.has_value ());
    return *m_suspend.stop_pc;
  }

  /* Set this thread's stop PC.  */

  void set_stop_pc (CORE_ADDR stop_pc)
  {
    m_suspend.stop_pc = stop_pc;
  }

  /* Remove the stop_pc stored on this thread.  */

  void clear_stop_pc ()
  {
    m_suspend.stop_pc.reset ();
  }

  /* Return true if this thread has a cached stop pc value, otherwise
     return false.  */

  bool stop_pc_p () const
  {
    return m_suspend.stop_pc.has_value ();
  }

  /* Return true if this thread has a pending wait status.  */

  bool has_pending_waitstatus () const
  {
    return m_suspend.waitstatus_pending_p;
  }

  /* Get this thread's pending wait status.

     May only be called if has_pending_waitstatus returns true.  */

  const target_waitstatus &pending_waitstatus () const
  {
    gdb_assert (this->has_pending_waitstatus ());

    return m_suspend.waitstatus;
  }

  /* Set this thread's pending wait status.

     May only be called if has_pending_waitstatus returns false.  */

  void set_pending_waitstatus (const target_waitstatus &ws);

  /* Clear this thread's pending wait status.

     May only be called if has_pending_waitstatus returns true.  */

  void clear_pending_waitstatus ();

  /* Return this thread's stop signal.  */

  gdb_signal stop_signal () const
  {
    return m_suspend.stop_signal;
  }

  /* Set this thread's stop signal.  */

  void set_stop_signal (gdb_signal sig)
  {
    m_suspend.stop_signal = sig;
  }

  /* Return this thread's stop reason.  */

  target_stop_reason stop_reason () const
  {
    return m_suspend.stop_reason;
  }

  /* Set this thread's stop reason.  */

  void set_stop_reason (target_stop_reason reason)
  {
    m_suspend.stop_reason = reason;
  }

  /* Get the FSM associated with the thread.  */

  struct thread_fsm *thread_fsm () const
  {
    return m_thread_fsm.get ();
  }

  /* Get the owning reference to the FSM associated with the thread.

     After a call to this method, "thread_fsm () == nullptr".  */

  std::unique_ptr<struct thread_fsm> release_thread_fsm ()
  {
    return std::move (m_thread_fsm);
  }

  /* Set the FSM associated with the current thread.

     It is invalid to set the FSM if another FSM is already installed.  */

  void set_thread_fsm (std::unique_ptr<struct thread_fsm> fsm)
  {
    gdb_assert (m_thread_fsm == nullptr);
    m_thread_fsm = std::move (fsm);
  }

  /* Record the thread options last set for this thread.  */

  void set_thread_options (gdb_thread_options thread_options);

  /* Get the thread options last set for this thread.  */

  gdb_thread_options thread_options () const
  {
    return m_thread_options;
  }

  int current_line = 0;
  struct symtab *current_symtab = NULL;

  /* Internal stepping state.  */

  /* Record the pc of the thread the last time it was resumed.  (It
     can't be done on stop as the PC may change since the last stop,
     e.g., "return" command, or "p $pc = 0xf000").  This is maintained
     by proceed and keep_going, and among other things, it's used in
     adjust_pc_after_break to distinguish a hardware single-step
     SIGTRAP from a breakpoint SIGTRAP.  */
  CORE_ADDR prev_pc = 0;

  /* Did we set the thread stepping a breakpoint instruction?  This is
     used in conjunction with PREV_PC to decide whether to adjust the
     PC.  */
  int stepped_breakpoint = 0;

  /* Should we step over breakpoint next time keep_going is called?  */
  int stepping_over_breakpoint = 0;

  /* Should we step over a watchpoint next time keep_going is called?
     This is needed on targets with non-continuable, non-steppable
     watchpoints.  */
  int stepping_over_watchpoint = 0;

  /* Set to TRUE if we should finish single-stepping over a breakpoint
     after hitting the current step-resume breakpoint.  The context here
     is that GDB is to do `next' or `step' while signal arrives.
     When stepping over a breakpoint and signal arrives, GDB will attempt
     to skip signal handler, so it inserts a step_resume_breakpoint at the
     signal return address, and resume inferior.
     step_after_step_resume_breakpoint is set to TRUE at this moment in
     order to keep GDB in mind that there is still a breakpoint to step over
     when GDB gets back SIGTRAP from step_resume_breakpoint.  */
  int step_after_step_resume_breakpoint = 0;

  /* This is used to remember when a fork or vfork event was caught by
     a catchpoint, and thus the event is to be followed at the next
     resume of the thread, and not immediately.  */
  struct target_waitstatus pending_follow;

  /* True if this thread has been explicitly requested to stop.  */
  int stop_requested = 0;

  /* The initiating frame of a nexting operation, used for deciding
     which exceptions to intercept.  If it is null_frame_id no
     bp_longjmp or bp_exception but longjmp has been caught just for
     bp_longjmp_call_dummy.  */
  struct frame_id initiating_frame = null_frame_id;

  /* Private data used by the target vector implementation.  */
  private_thread_info_up priv;

  /* Branch trace information for this thread.  */
  struct btrace_thread_info btrace {};

  /* Flag which indicates that the stack temporaries should be stored while
     evaluating expressions.  */
  bool stack_temporaries_enabled = false;

  /* Values that are stored as temporaries on stack while evaluating
     expressions.  */
  std::vector<struct value *> stack_temporaries;

  /* Step-over chain.  A thread is in the step-over queue if this node is
     linked.  */
  intrusive_list_node<thread_info> step_over_list_node;

  /* Node for list of threads that are resumed and have a pending wait status.

     The list head for this is in process_stratum_target, hence all threads in
     this list belong to that process target.  */
  intrusive_list_node<thread_info> resumed_with_pending_wait_status_node;

  /* Displaced-step state for this thread.  */
  displaced_step_thread_state displaced_step_state;

private:
  /* True if this thread is resumed from infrun's perspective.
     Note that a thread can be marked both as not-executing and
     resumed at the same time.  This happens if we try to resume a
     thread that has a wait status pending.  We shouldn't let the
     thread really run until that wait status has been processed, but
     we should not process that wait status if we didn't try to let
     the thread run.  */
  bool m_resumed = false;

  /* True means the thread is executing.  Note: this is different
     from saying that there is an active target and we are stopped at
     a breakpoint, for instance.  This is a real indicator whether the
     thread is off and running.  */
  bool m_executing = false;

  /* State of inferior thread to restore after GDB is done with an inferior
     call.  See `struct thread_suspend_state'.  */
  thread_suspend_state m_suspend;

  /* The user-given name of the thread.

     Nullptr if the thread does not have a user-given name.  */
  gdb::unique_xmalloc_ptr<char> m_name;

  /* Pointer to the state machine manager object that handles what is
     left to do for the thread's execution command after the target
     stops.  Several execution commands use it.  */
  std::unique_ptr<struct thread_fsm> m_thread_fsm;

  /* The thread options as last set with a call to
     set_thread_options.  */
  gdb_thread_options m_thread_options;
};

using thread_info_resumed_with_pending_wait_status_node
  = intrusive_member_node<thread_info,
			  &thread_info::resumed_with_pending_wait_status_node>;
using thread_info_resumed_with_pending_wait_status_list
  = intrusive_list<thread_info,
		   thread_info_resumed_with_pending_wait_status_node>;

/* A gdb::ref_ptr pointer to a thread_info.  */

using thread_info_ref
  = gdb::ref_ptr<struct thread_info, refcounted_object_ref_policy>;

/* A gdb::ref_ptr pointer to an inferior.  This would ideally be in
   inferior.h, but it can't due to header dependencies (inferior.h
   includes gdbthread.h).  */

using inferior_ref
  = gdb::ref_ptr<struct inferior, refcounted_object_ref_policy>;

/* Create an empty thread list, or empty the existing one.  */
extern void init_thread_list (void);

/* Add a thread to the thread list, print a message
   that a new thread is found, and return the pointer to
   the new thread.  Caller my use this pointer to 
   initialize the private thread data.  */
extern struct thread_info *add_thread (process_stratum_target *targ,
				       ptid_t ptid);

/* Same as add_thread, but does not print a message about new
   thread.  */
extern struct thread_info *add_thread_silent (process_stratum_target *targ,
					      ptid_t ptid);

/* Same as add_thread, and sets the private info.  */
extern struct thread_info *add_thread_with_info (process_stratum_target *targ,
						 ptid_t ptid,
						 private_thread_info_up);

/* Delete thread THREAD and notify of thread exit.  If the thread is
   currently not deletable, don't actually delete it but still tag it
   as exited and do the notification.  EXIT_CODE is the thread's exit
   code.  If SILENT, don't actually notify the CLI.  THREAD must not
   be NULL or an assertion will fail.  */
extern void delete_thread_with_exit_code (thread_info *thread,
					  ULONGEST exit_code,
					  bool silent = false);

/* Delete thread THREAD and notify of thread exit.  If the thread is
   currently not deletable, don't actually delete it but still tag it
   as exited and do the notification.  THREAD must not be NULL or an
   assertion will fail.  */
extern void delete_thread (thread_info *thread);

/* Like delete_thread, but be quiet about it.  Used when the process
   this thread belonged to has already exited, for example.  */
extern void delete_thread_silent (struct thread_info *thread);

/* Mark the thread exited, but don't delete it or remove it from the
   inferior thread list.  EXIT_CODE is the thread's exit code, if
   available.  If SILENT, then don't inform the CLI about the
   exit.  */
extern void set_thread_exited (thread_info *tp,
			       std::optional<ULONGEST> exit_code = {},
			       bool silent = false);

/* Delete a step_resume_breakpoint from the thread database.  */
extern void delete_step_resume_breakpoint (struct thread_info *);

/* Delete an exception_resume_breakpoint from the thread database.  */
extern void delete_exception_resume_breakpoint (struct thread_info *);

/* Delete the single-step breakpoints of thread TP, if any.  */
extern void delete_single_step_breakpoints (struct thread_info *tp);

/* Check if the thread has software single stepping breakpoints
   set.  */
extern int thread_has_single_step_breakpoints_set (struct thread_info *tp);

/* Check whether the thread has software single stepping breakpoints
   set at PC.  */
extern int thread_has_single_step_breakpoint_here (struct thread_info *tp,
						   const address_space *aspace,
						   CORE_ADDR addr);

/* Returns whether to show inferior-qualified thread IDs, or plain
   thread numbers.  Inferior-qualified IDs are shown whenever we have
   multiple inferiors, or the only inferior left has number > 1.  */
extern int show_inferior_qualified_tids (void);

/* Return a string version of THR's thread ID.  If there are multiple
   inferiors, then this prints the inferior-qualifier form, otherwise
   it only prints the thread number.  The result is stored in a
   circular static buffer, NUMCELLS deep.  */
const char *print_thread_id (struct thread_info *thr);

/* Like print_thread_id, but always prints the inferior-qualified form,
   even when there is only a single inferior.  */
const char *print_full_thread_id (struct thread_info *thr);

/* Boolean test for an already-known ptid.  */
extern bool in_thread_list (process_stratum_target *targ, ptid_t ptid);

/* Boolean test for an already-known global thread id (GDB's homegrown
   global id, not the system's).  */
extern int valid_global_thread_id (int global_id);

/* Find thread by GDB global thread ID.  */
struct thread_info *find_thread_global_id (int global_id);

/* Find thread by thread library specific handle in inferior INF.  */
struct thread_info *find_thread_by_handle
  (gdb::array_view<const gdb_byte> handle, struct inferior *inf);

/* Finds the first thread of the specified inferior.  */
extern struct thread_info *first_thread_of_inferior (inferior *inf);

/* Returns any thread of inferior INF, giving preference to the
   current thread.  */
extern struct thread_info *any_thread_of_inferior (inferior *inf);

/* Returns any non-exited thread of inferior INF, giving preference to
   the current thread, and to not executing threads.  */
extern struct thread_info *any_live_thread_of_inferior (inferior *inf);

/* Change the ptid of thread OLD_PTID to NEW_PTID.  */
void thread_change_ptid (process_stratum_target *targ,
			 ptid_t old_ptid, ptid_t new_ptid);

/* Iterator function to call a user-provided callback function
   once for each known thread.  */
typedef int (*thread_callback_func) (struct thread_info *, void *);
extern struct thread_info *iterate_over_threads (thread_callback_func, void *);

/* Pull in the internals of the inferiors/threads ranges and
   iterators.  Must be done after struct thread_info is defined.  */
#include "thread-iter.h"

/* Return a range that can be used to walk over threads, with
   range-for.

   Used like this, it walks over all threads of all inferiors of all
   targets:

       for (thread_info *thr : all_threads ())
	 { .... }

   FILTER_PTID can be used to filter out threads that don't match.
   FILTER_PTID can be:

   - minus_one_ptid, meaning walk all threads of all inferiors of
     PROC_TARGET.  If PROC_TARGET is NULL, then of all targets.

   - A process ptid, in which case walk all threads of the specified
     process.  PROC_TARGET must be non-NULL in this case.

   - A thread ptid, in which case walk that thread only.  PROC_TARGET
     must be non-NULL in this case.
*/

inline all_matching_threads_range
all_threads (process_stratum_target *proc_target = nullptr,
	     ptid_t filter_ptid = minus_one_ptid)
{
  return all_matching_threads_range (proc_target, filter_ptid);
}

/* Return a range that can be used to walk over all non-exited threads
   of all inferiors, with range-for.  Arguments are like all_threads
   above.  */

inline all_non_exited_threads_range
all_non_exited_threads (process_stratum_target *proc_target = nullptr,
			ptid_t filter_ptid = minus_one_ptid)
{
  return all_non_exited_threads_range (proc_target, filter_ptid);
}

/* Return a range that can be used to walk over all threads of all
   inferiors, with range-for, safely.  I.e., it is safe to delete the
   currently-iterated thread.  When combined with range-for, this
   allow convenient patterns like this:

     for (thread_info *t : all_threads_safe ())
       if (some_condition ())
	 delete f;
*/

inline all_threads_safe_range
all_threads_safe ()
{
  return all_threads_safe_range (all_threads_iterator::begin_t {});
}

extern int thread_count (process_stratum_target *proc_target);

/* Return true if we have any thread in any inferior.  */
extern bool any_thread_p ();

/* Switch context to thread THR.  */
extern void switch_to_thread (struct thread_info *thr);

/* Switch context to no thread selected.  */
extern void switch_to_no_thread ();

/* Switch from one thread to another.  Does not read registers.  */
extern void switch_to_thread_no_regs (struct thread_info *thread);

/* Marks or clears thread(s) PTID of TARG as resumed.  If PTID is
   MINUS_ONE_PTID, applies to all threads of TARG.  If
   ptid_is_pid(PTID) is true, applies to all threads of the process
   pointed at by {TARG,PTID}.  */
extern void set_resumed (process_stratum_target *targ,
			 ptid_t ptid, bool resumed);

/* Marks thread PTID of TARG as running, or as stopped.  If PTID is
   minus_one_ptid, marks all threads of TARG.  */
extern void set_running (process_stratum_target *targ,
			 ptid_t ptid, bool running);

/* Marks or clears thread(s) PTID of TARG as having been requested to
   stop.  If PTID is MINUS_ONE_PTID, applies to all threads of TARG.
   If ptid_is_pid(PTID) is true, applies to all threads of the process
   pointed at by {TARG, PTID}.  If STOP, then the
   THREAD_STOP_REQUESTED observer is called with PTID as argument.  */
extern void set_stop_requested (process_stratum_target *targ,
				ptid_t ptid, bool stop);

/* Marks thread PTID of TARG as executing, or not.  If PTID is
   minus_one_ptid, marks all threads of TARG.

   Note that this is different from the running state.  See the
   description of state and executing fields of struct
   thread_info.  */
extern void set_executing (process_stratum_target *targ,
			   ptid_t ptid, bool executing);

/* True if any (known or unknown) thread of TARG is or may be
   executing.  */
extern bool threads_are_executing (process_stratum_target *targ);

/* Merge the executing property of thread PTID of TARG over to its
   thread state property (frontend running/stopped view).

   "not executing" -> "stopped"
   "executing"     -> "running"
   "exited"        -> "exited"

   If PTID is minus_one_ptid, go over all threads of TARG.

   Notifications are only emitted if the thread state did change.  */
extern void finish_thread_state (process_stratum_target *targ, ptid_t ptid);

/* Calls finish_thread_state on scope exit, unless release() is called
   to disengage.  */
using scoped_finish_thread_state
  = FORWARD_SCOPE_EXIT (finish_thread_state);

/* Commands with a prefix of `thread'.  */
extern struct cmd_list_element *thread_cmd_list;

extern void thread_command (const char *tidstr, int from_tty);

/* Print notices on thread events (attach, detach, etc.), set with
   `set print thread-events'.  */
extern bool print_thread_events;

/* Prints the list of threads and their details on UIOUT.  If
   REQUESTED_THREADS, a list of GDB ids/ranges, is not NULL, only
   print threads whose ID is included in the list.  If PID is not -1,
   only print threads from the process PID.  Otherwise, threads from
   all attached PIDs are printed.  If both REQUESTED_THREADS is not
   NULL and PID is not -1, then the thread is printed if it belongs to
   the specified process.  Otherwise, an error is raised.  */
extern void print_thread_info (struct ui_out *uiout,
			       const char *requested_threads,
			       int pid);

/* Save/restore current inferior/thread/frame.  */

class scoped_restore_current_thread
{
public:
  scoped_restore_current_thread ();
  ~scoped_restore_current_thread ();

  scoped_restore_current_thread (scoped_restore_current_thread &&rhs);

  DISABLE_COPY_AND_ASSIGN (scoped_restore_current_thread);

  /* Cancel restoring on scope exit.  */
  void dont_restore () { m_dont_restore = true; }

private:
  void restore ();

  bool m_dont_restore = false;
  thread_info_ref m_thread;
  inferior_ref m_inf;

  frame_id m_selected_frame_id;
  int m_selected_frame_level;
  bool m_was_stopped;
  /* Save/restore the language as well, because selecting a frame
     changes the current language to the frame's language if "set
     language auto".  */
  scoped_restore_current_language m_lang;
};

/* Returns a pointer into the thread_info corresponding to
   INFERIOR_PTID.  INFERIOR_PTID *must* be in the thread list.  */
extern struct thread_info* inferior_thread (void);

extern void update_thread_list (void);

/* Delete any thread the target says is no longer alive.  */

extern void prune_threads (void);

/* Delete threads marked THREAD_EXITED.  Unlike prune_threads, this
   does not consult the target about whether the thread is alive right
   now.  */
extern void delete_exited_threads (void);

/* Return true if PC is in the stepping range of THREAD.  */

bool pc_in_thread_step_range (CORE_ADDR pc, struct thread_info *thread);

/* Enable storing stack temporaries for thread THR and disable and
   clear the stack temporaries on destruction.  Holds a strong
   reference to THR.  */

class enable_thread_stack_temporaries
{
public:

  explicit enable_thread_stack_temporaries (struct thread_info *thr)
    : m_thr (thread_info_ref::new_reference (thr))
  {
    m_thr->stack_temporaries_enabled = true;
    m_thr->stack_temporaries.clear ();
  }

  ~enable_thread_stack_temporaries ()
  {
    m_thr->stack_temporaries_enabled = false;
    m_thr->stack_temporaries.clear ();
  }

  DISABLE_COPY_AND_ASSIGN (enable_thread_stack_temporaries);

private:

  thread_info_ref m_thr;
};

extern bool thread_stack_temporaries_enabled_p (struct thread_info *tp);

extern void push_thread_stack_temporary (struct thread_info *tp, struct value *v);

extern value *get_last_thread_stack_temporary (struct thread_info *tp);

extern bool value_in_thread_stack_temporaries (struct value *,
					       struct thread_info *thr);

/* Thread step-over list type.  */
using thread_step_over_list_node
  = intrusive_member_node<thread_info, &thread_info::step_over_list_node>;
using thread_step_over_list
  = intrusive_list<thread_info, thread_step_over_list_node>;
using thread_step_over_list_iterator
  = reference_to_pointer_iterator<thread_step_over_list::iterator>;
using thread_step_over_list_safe_iterator
  = basic_safe_iterator<thread_step_over_list_iterator>;
using thread_step_over_list_safe_range
  = iterator_range<thread_step_over_list_safe_iterator>;

static inline thread_step_over_list_safe_range
make_thread_step_over_list_safe_range (thread_step_over_list &list)
{
  return thread_step_over_list_safe_range
    (thread_step_over_list_safe_iterator (list.begin (),
					  list.end ()),
     thread_step_over_list_safe_iterator (list.end (),
					  list.end ()));
}

/* Add TP to the end of the global pending step-over chain.  */

extern void global_thread_step_over_chain_enqueue (thread_info *tp);

/* Append the thread step over list LIST to the global thread step over
   chain. */

extern void global_thread_step_over_chain_enqueue_chain
  (thread_step_over_list &&list);

/* Remove TP from the global pending step-over chain.  */

extern void global_thread_step_over_chain_remove (thread_info *tp);

/* Return true if TP is in any step-over chain.  */

extern int thread_is_in_step_over_chain (struct thread_info *tp);

/* Return the length of the the step over chain TP is in.

   If TP is non-nullptr, the thread must be in a step over chain.
   TP may be nullptr, in which case it denotes an empty list, so a length of
   0.  */

extern int thread_step_over_chain_length (const thread_step_over_list &l);

/* Cancel any ongoing execution command.  */

extern void thread_cancel_execution_command (struct thread_info *thr);

/* Check whether it makes sense to access a register of the current
   thread at this point.  If not, throw an error (e.g., the thread is
   executing).  */
extern void validate_registers_access (void);

/* Check whether it makes sense to access a register of THREAD at this point.
   Returns true if registers may be accessed; false otherwise.  */
extern bool can_access_registers_thread (struct thread_info *thread);

/* Returns whether to show which thread hit the breakpoint, received a
   signal, etc. and ended up causing a user-visible stop.  This is
   true iff we ever detected multiple threads.  */
extern int show_thread_that_caused_stop (void);

/* Print the message for a thread or/and frame selected.  */
extern void print_selected_thread_frame (struct ui_out *uiout,
					 user_selected_what selection);

/* Helper for the CLI's "thread" command and for MI's -thread-select.
   Selects thread THR.  TIDSTR is the original string the thread ID
   was parsed from.  This is used in the error message if THR is not
   alive anymore.  */
extern void thread_select (const char *tidstr, class thread_info *thr);

/* Return THREAD's name.

   If THREAD has a user-given name, return it.  Otherwise, query the thread's
   target to get the name.  May return nullptr.  */
extern const char *thread_name (thread_info *thread);

/* Switch to thread TP if it is alive.  Returns true if successfully
   switched, false otherwise.  */

extern bool switch_to_thread_if_alive (thread_info *thr);

/* Assuming that THR is the current thread, execute CMD.
   If ADA_TASK is not empty, it is the Ada task ID, and will
   be printed instead of the thread information.
   FLAGS.QUIET controls the printing of the thread information.
   FLAGS.CONT and FLAGS.SILENT control how to handle errors.  Can throw an
   exception if !FLAGS.SILENT and !FLAGS.CONT and CMD fails.  */

extern void thread_try_catch_cmd (thread_info *thr,
				  std::optional<int> ada_task,
				  const char *cmd, int from_tty,
				  const qcs_flags &flags);

/* Return a string representation of STATE.  */

extern const char *thread_state_string (enum thread_state state);

#endif /* GDBTHREAD_H */
