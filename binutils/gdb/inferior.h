/* Variables that describe the inferior process running under GDB:
   Where it is, why it stopped, and how to step it.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#if !defined (INFERIOR_H)
#define INFERIOR_H 1

#include <exception>
#include <list>

struct target_waitstatus;
class frame_info_ptr;
struct ui_file;
struct type;
struct gdbarch;
struct regcache;
struct ui_out;
struct terminal_info;
struct target_desc_info;
struct inferior;
struct thread_info;

/* For bpstat.  */
#include "breakpoint.h"

/* For enum gdb_signal.  */
#include "target.h"

/* For struct frame_id.  */
#include "frame.h"

/* For gdb_environ.  */
#include "gdbsupport/environ.h"

#include "progspace.h"
#include "registry.h"

#include "symfile-add-flags.h"
#include "gdbsupport/refcounted-object.h"
#include "gdbsupport/forward-scope-exit.h"
#include "gdbsupport/gdb_unique_ptr.h"
#include "gdbsupport/intrusive_list.h"

#include "gdbsupport/common-inferior.h"
#include "gdbthread.h"

#include "process-stratum-target.h"
#include "displaced-stepping.h"

#include <unordered_map>

struct infcall_suspend_state;
struct infcall_control_state;

extern void restore_infcall_suspend_state (struct infcall_suspend_state *);
extern void restore_infcall_control_state (struct infcall_control_state *);

/* A deleter for infcall_suspend_state that calls
   restore_infcall_suspend_state.  */
struct infcall_suspend_state_deleter
{
  void operator() (struct infcall_suspend_state *state) const
  {
    try
      {
	restore_infcall_suspend_state (state);
      }
    catch (const gdb_exception_error &e)
      {
	/* If we are restoring the inferior state due to an exception,
	   some error message will be printed.  So, only warn the user
	   when we cannot restore during normal execution.  */
	bool unwinding;
#if __cpp_lib_uncaught_exceptions
	unwinding = std::uncaught_exceptions () > 0;
#else
	unwinding = std::uncaught_exception ();
#endif
	if (!unwinding)
	  warning (_("Failed to restore inferior state: %s"), e.what ());
      }
  }
};

/* A unique_ptr specialization for infcall_suspend_state.  */
typedef std::unique_ptr<infcall_suspend_state, infcall_suspend_state_deleter>
    infcall_suspend_state_up;

extern infcall_suspend_state_up save_infcall_suspend_state ();

/* A deleter for infcall_control_state that calls
   restore_infcall_control_state.  */
struct infcall_control_state_deleter
{
  void operator() (struct infcall_control_state *state) const
  {
    restore_infcall_control_state (state);
  }
};

/* A unique_ptr specialization for infcall_control_state.  */
typedef std::unique_ptr<infcall_control_state, infcall_control_state_deleter>
    infcall_control_state_up;

extern infcall_control_state_up save_infcall_control_state ();

extern void discard_infcall_suspend_state (struct infcall_suspend_state *);
extern void discard_infcall_control_state (struct infcall_control_state *);

extern readonly_detached_regcache *
  get_infcall_suspend_state_regcache (struct infcall_suspend_state *);

extern void set_sigint_trap (void);

extern void clear_sigint_trap (void);

/* Collected pid, tid, etc. of the debugged inferior.  When there's
   no inferior, inferior_ptid.pid () will be 0.  */

extern ptid_t inferior_ptid;

extern void generic_mourn_inferior (void);

extern CORE_ADDR unsigned_pointer_to_address (struct gdbarch *gdbarch,
					      struct type *type,
					      const gdb_byte *buf);
extern void unsigned_address_to_pointer (struct gdbarch *gdbarch,
					 struct type *type, gdb_byte *buf,
					 CORE_ADDR addr);
extern CORE_ADDR signed_pointer_to_address (struct gdbarch *gdbarch,
					    struct type *type,
					    const gdb_byte *buf);
extern void address_to_signed_pointer (struct gdbarch *gdbarch,
				       struct type *type, gdb_byte *buf,
				       CORE_ADDR addr);

extern void reopen_exec_file (void);

/* From misc files */

extern void default_print_registers_info (struct gdbarch *gdbarch,
					  struct ui_file *file,
					  frame_info_ptr frame,
					  int regnum, int all);

/* Default implementation of gdbarch_print_float_info.  Print
   the values of all floating point registers.  */

extern void default_print_float_info (struct gdbarch *gdbarch,
				      struct ui_file *file,
				      frame_info_ptr frame,
				      const char *args);

/* Try to determine whether TTY is GDB's input terminal.  Returns
   TRIBOOL_UNKNOWN if we can't tell.  */

extern tribool is_gdb_terminal (const char *tty);

/* Helper for sharing_input_terminal.  Try to determine whether pid
   PID is using the same TTY for input as GDB is.  Returns
   TRIBOOL_UNKNOWN if we can't tell.  */

extern tribool sharing_input_terminal (int pid);

/* The type of the function that is called when SIGINT is handled.  */

typedef void c_c_handler_ftype (int);

/* Install a new SIGINT handler in a host-dependent way.  The previous
   handler is returned.  It is fine to pass SIG_IGN for FN, but not
   SIG_DFL.  */

extern c_c_handler_ftype *install_sigint_handler (c_c_handler_ftype *fn);

extern void child_terminal_info (struct target_ops *self, const char *, int);

extern void child_terminal_ours (struct target_ops *self);

extern void child_terminal_ours_for_output (struct target_ops *self);

extern void child_terminal_inferior (struct target_ops *self);

extern void child_terminal_save_inferior (struct target_ops *self);

extern void child_terminal_init (struct target_ops *self);

extern void child_pass_ctrlc (struct target_ops *self);

extern void child_interrupt (struct target_ops *self);

/* From fork-child.c */

/* Helper function to call STARTUP_INFERIOR with PID and NUM_TRAPS.
   This function already calls set_executing.  Return the ptid_t from
   STARTUP_INFERIOR.  */
extern ptid_t gdb_startup_inferior (pid_t pid, int num_traps);

/* From infcmd.c */

/* Initial inferior setup.  Determines the exec file is not yet known,
   takes any necessary post-attaching actions, fetches the target
   description and syncs the shared library list.  */

extern void setup_inferior (int from_tty);

extern void post_create_inferior (int from_tty);

extern void attach_command (const char *, int);

extern void registers_info (const char *, int);

extern void continue_1 (int all_threads);

extern void interrupt_target_1 (bool all_threads);

using delete_longjmp_breakpoint_cleanup
  = FORWARD_SCOPE_EXIT (delete_longjmp_breakpoint);

extern void detach_command (const char *, int);

extern void notice_new_inferior (struct thread_info *, bool, int);

/* Return the value of the result of a function at the end of a 'finish'
   command/BP.  If the result's value cannot be retrieved, return NULL.

   FUNC_SYMBOL is the symbol of the function being returned from.  FUNCTION is
   a value containing the address of the function.  */

extern struct value *get_return_value (struct symbol *func_symbol,
				       struct value *function);

/* Prepare for execution command.  TARGET is the target that will run
   the command.  BACKGROUND determines whether this is a foreground
   (synchronous) or background (asynchronous) command.  */

extern void prepare_execution_command (struct target_ops *target,
				       int background);

/* Nonzero if stopped due to completion of a stack dummy routine.  */

extern enum stop_stack_kind stop_stack_dummy;

/* Nonzero if program stopped due to a random (unexpected) signal in
   inferior process.  */

extern int stopped_by_random_signal;

/* Print notices on inferior events (attach, detach, etc.), set with
   `set print inferior-events'.  */
extern bool print_inferior_events;

/* Anything but NO_STOP_QUIETLY means we expect a trap and the caller
   will handle it themselves.  STOP_QUIETLY is used when running in
   the shell before the child program has been exec'd and when running
   through shared library loading.  STOP_QUIETLY_REMOTE is used when
   setting up a remote connection; it is like STOP_QUIETLY_NO_SIGSTOP
   except that there is no need to hide a signal.  */

/* STOP_QUIETLY_NO_SIGSTOP is used to handle a tricky situation with attach.
   When doing an attach, the kernel stops the debuggee with a SIGSTOP.
   On newer GNU/Linux kernels (>= 2.5.61) the handling of SIGSTOP for
   a ptraced process has changed.  Earlier versions of the kernel
   would ignore these SIGSTOPs, while now SIGSTOP is treated like any
   other signal, i.e. it is not muffled.

   If the gdb user does a 'continue' after the 'attach', gdb passes
   the global variable stop_signal (which stores the signal from the
   attach, SIGSTOP) to the ptrace(PTRACE_CONT,...)  call.  This is
   problematic, because the kernel doesn't ignore such SIGSTOP
   now.  I.e. it is reported back to gdb, which in turn presents it
   back to the user.

   To avoid the problem, we use STOP_QUIETLY_NO_SIGSTOP, which allows
   gdb to clear the value of stop_signal after the attach, so that it
   is not passed back down to the kernel.  */

enum stop_kind
  {
    NO_STOP_QUIETLY = 0,
    STOP_QUIETLY,
    STOP_QUIETLY_REMOTE,
    STOP_QUIETLY_NO_SIGSTOP
  };



/* Base class for target-specific inferior data.  */

struct private_inferior
{
  virtual ~private_inferior () = 0;
};

/* Inferior process specific part of `struct infcall_control_state'.

   Inferior thread counterpart is `struct thread_control_state'.  */

struct inferior_control_state
{
  inferior_control_state ()
    : stop_soon (NO_STOP_QUIETLY)
  {
  }

  explicit inferior_control_state (enum stop_kind when)
    : stop_soon (when)
  {
  }

  /* See the definition of stop_kind above.  */
  enum stop_kind stop_soon;
};

/* Return a pointer to the current inferior.  */
extern inferior *current_inferior ();

extern void set_current_inferior (inferior *);

/* Switch inferior (and program space) to INF, and switch to no thread
   selected.  */
extern void switch_to_inferior_no_thread (inferior *inf);

/* Ensure INF is the current inferior.

   If the current inferior was changed, return an RAII object that will
   restore the original current context.  */
extern std::optional<scoped_restore_current_thread> maybe_switch_inferior
  (inferior *inf);

/* Info about an inferior's target description.  There's one of these
   for each inferior.  */

struct target_desc_info
{
  /* Returns true if this target description information has been supplied by
     the user.  */
  bool from_user_p ()
  { return !this->filename.empty (); }

  /* A flag indicating that a description has already been fetched
     from the target, so it should not be queried again.  */
  bool fetched = false;

  /* The description fetched from the target, or NULL if the target
     did not supply any description.  Only valid when
     FETCHED is set.  Only the description initialization
     code should access this; normally, the description should be
     accessed through the gdbarch object.  */
  const struct target_desc *tdesc = nullptr;

  /* If not empty, the filename to read a target description from, as set by
     "set tdesc filename ...".

     If empty, there is not filename specified by the user.  */
  std::string filename;
};

/* GDB represents the state of each program execution with an object
   called an inferior.  An inferior typically corresponds to a process
   but is more general and applies also to targets that do not have a
   notion of processes.  Each run of an executable creates a new
   inferior, as does each attachment to an existing process.
   Inferiors have unique internal identifiers that are different from
   target process ids.  Each inferior may in turn have multiple
   threads running in it.

   Inferiors are intrusively refcounted objects.  Unlike thread
   objects, being the user-selected inferior is considered a strong
   reference and is thus accounted for in the inferior object's
   refcount (see set_current_inferior).  When GDB needs to remember
   the selected inferior to later restore it, GDB temporarily bumps
   the inferior object's refcount, to prevent something deleting the
   inferior object before reverting back (e.g., due to a
   "remove-inferiors" command (see
   scoped_restore_current_inferior).  All other inferior
   references are considered weak references.  Inferiors are always
   listed exactly once in the inferior list, so placing an inferior in
   the inferior list is an implicit, not counted strong reference.  */

class inferior : public refcounted_object,
		 public intrusive_list_node<inferior>
{
public:
  explicit inferior (int pid);
  ~inferior ();

  /* Returns true if we can delete this inferior.  */
  bool deletable () const { return refcount () == 0; }

  /* Push T in this inferior's target stack.  */
  void push_target (struct target_ops *t)
  { m_target_stack.push (t); }

  /* An overload that deletes the target on failure.  */
  void push_target (target_ops_up &&t)
  {
    m_target_stack.push (t.get ());
    t.release ();
  }

  /* Unpush T from this inferior's target stack.  */
  int unpush_target (struct target_ops *t);

  /* Returns true if T is pushed in this inferior's target stack.  */
  bool target_is_pushed (const target_ops *t) const
  { return m_target_stack.is_pushed (t); }

  /* Find the target beneath T in this inferior's target stack.  */
  target_ops *find_target_beneath (const target_ops *t)
  { return m_target_stack.find_beneath (t); }

  /* Return the target at the top of this inferior's target stack.  */
  target_ops *top_target ()
  { return m_target_stack.top (); }

  /* Unpush all targets except the dummy target from m_target_stack.  As
     targets are removed from m_target_stack their reference count is
     decremented, which may cause a target to close.  */
  void pop_all_targets ()
  { pop_all_targets_above (dummy_stratum); }

  /* Unpush all targets above STRATUM from m_target_stack.  As targets are
     removed from m_target_stack their reference count is decremented,
     which may cause a target to close.  */
  void pop_all_targets_above (enum strata stratum);

  /* Unpush all targets at and above STRATUM from m_target_stack.  As
     targets are removed from m_target_stack their reference count is
     decremented, which may cause a target to close.  */
  void pop_all_targets_at_and_above (enum strata stratum);

  /* Return the target at process_stratum level in this inferior's
     target stack.  */
  struct process_stratum_target *process_target ()
  { return (process_stratum_target *) m_target_stack.at (process_stratum); }

  /* Return the target at STRATUM in this inferior's target stack.  */
  target_ops *target_at (enum strata stratum)
  { return m_target_stack.at (stratum); }

  bool has_execution ()
  { return target_has_execution (this); }

  /* This inferior's thread list, sorted by creation order.  */
  intrusive_list<thread_info> thread_list;

  /* A map of ptid_t to thread_info*, for average O(1) ptid_t lookup.
     Exited threads do not appear in the map.  */
  std::unordered_map<ptid_t, thread_info *> ptid_thread_map;

  /* Returns a range adapter covering the inferior's threads,
     including exited threads.  Used like this:

       for (thread_info *thr : inf->threads ())
	 { .... }
  */
  inf_threads_range threads ()
  { return inf_threads_range (this->thread_list.begin ()); }

  /* Returns a range adapter covering the inferior's non-exited
     threads.  Used like this:

       for (thread_info *thr : inf->non_exited_threads ())
	 { .... }
  */
  inf_non_exited_threads_range non_exited_threads ()
  { return inf_non_exited_threads_range (this->thread_list.begin ()); }

  /* Like inferior::threads(), but returns a range adapter that can be
     used with range-for, safely.  I.e., it is safe to delete the
     currently-iterated thread, like this:

     for (thread_info *t : inf->threads_safe ())
       if (some_condition ())
	 delete f;
  */
  inline safe_inf_threads_range threads_safe ()
  { return safe_inf_threads_range (this->thread_list.begin ()); }

  /* Find (non-exited) thread PTID of this inferior.  */
  thread_info *find_thread (ptid_t ptid);

  /* Delete all threads in the thread list, silently.  */
  void clear_thread_list ();

  /* Continuations-related methods.  A continuation is an std::function
     to be called to finish the execution of a command when running
     GDB asynchronously.  A continuation is executed after any thread
     of this inferior stops.  Continuations are used by the attach
     command and the remote target when a new inferior is detected.  */
  void add_continuation (std::function<void ()> &&cont);
  void do_all_continuations ();

  /* Set/get file name for default use for standard in/out in the inferior.

     On Unix systems, we try to make TERMINAL_NAME the inferior's controlling
     terminal.

     If TERMINAL_NAME is the empty string, then the inferior inherits GDB's
     terminal (or GDBserver's if spawning a remote process).  */
  void set_tty (std::string terminal_name);
  const std::string &tty ();

  /* Set the argument string to use when running this inferior.

     An empty string can be used to represent "no arguments".  */
  void set_args (std::string args)
  {
    m_args = std::move (args);
  };

  /* Set the argument string from some strings.  */
  void set_args (gdb::array_view<char * const> args);

  /* Get the argument string to use when running this inferior.

     No arguments is represented by an empty string.  */
  const std::string &args () const
  {
    return m_args;
  }

  /* Set the inferior current working directory.

     If CWD is empty, unset the directory.  */
  void set_cwd (std::string cwd)
  {
    m_cwd = std::move (cwd);
  }

  /* Get the inferior current working directory.

     Return an empty string if the current working directory is not
     specified.  */
  const std::string &cwd () const
  {
    return m_cwd;
  }

  /* Set this inferior's arch.  */
  void set_arch (gdbarch *arch);

  /* Get this inferior's arch.  */
  gdbarch *arch ()
  { return m_gdbarch; }

  /* Convenient handle (GDB inferior id).  Unique across all
     inferiors.  */
  int num = 0;

  /* Actual target inferior id, usually, a process id.  This matches
     the ptid_t.pid member of threads of this inferior.  */
  int pid = 0;
  /* True if the PID was actually faked by GDB.  */
  bool fake_pid_p = false;

  /* The highest thread number this inferior ever had.  */
  int highest_thread_num = 0;

  /* State of GDB control of inferior process execution.
     See `struct inferior_control_state'.  */
  inferior_control_state control;

  /* True if this was an auto-created inferior, e.g. created from
     following a fork; false, if this inferior was manually added by
     the user, and we should not attempt to prune it
     automatically.  */
  bool removable = false;

  /* The address space bound to this inferior.  */
  address_space_ref_ptr aspace;

  /* The program space bound to this inferior.  */
  struct program_space *pspace = NULL;

  /* The terminal state as set by the last target_terminal::terminal_*
     call.  */
  target_terminal_state terminal_state = target_terminal_state::is_ours;

  /* Environment to use for running inferior,
     in format described in environ.h.  */
  gdb_environ environment;

  /* True if this child process was attached rather than forked.  */
  bool attach_flag = false;

  /* If this inferior is a vfork child, then this is the pointer to
     its vfork parent, if GDB is still attached to it.  */
  inferior *vfork_parent = NULL;

  /* If this process is a vfork parent, this is the pointer to the
     child.  Since a vfork parent is left frozen by the kernel until
     the child execs or exits, a process can only have one vfork child
     at a given time.  */
  inferior *vfork_child = NULL;

  /* True if this inferior should be detached when it's vfork sibling
     exits or execs.  */
  bool pending_detach = false;

  /* If non-nullptr, points to a thread that called vfork and is now waiting
     for a vfork child not under our control to be done with the shared memory
     region, either by exiting or execing.  */
  thread_info *thread_waiting_for_vfork_done = nullptr;

  /* True if we're in the process of detaching from this inferior.  */
  bool detaching = false;

  /* True if setup_inferior wasn't called for this inferior yet.
     Until that is done, we must not access inferior memory or
     registers, as we haven't determined the target
     architecture/description.  */
  bool needs_setup = false;

  /* True if the inferior is starting up (inside startup_inferior),
     and we're nursing it along (through the shell) until it is ready
     to execute its first instruction.  Until that is done, we must
     not access inferior memory or registers, as we haven't determined
     the target architecture/description.  */
  bool starting_up = false;

  /* True when we are reading the library list of the inferior during an
     attach or handling a fork child.  */
  bool in_initial_library_scan = false;

  /* Private data used by the process_stratum target.  */
  std::unique_ptr<private_inferior> priv;

  /* HAS_EXIT_CODE is true if the inferior exited with an exit code.
     In this case, the EXIT_CODE field is also valid.  */
  bool has_exit_code = false;
  LONGEST exit_code = 0;

  /* Default flags to pass to the symbol reading functions.  These are
     used whenever a new objfile is created.  */
  symfile_add_flags symfile_flags = 0;

  /* Info about an inferior's target description (if it's fetched; the
     user supplied description's filename, if any; etc.).  */
  target_desc_info tdesc_info;

  /* Data related to displaced stepping.  */
  displaced_step_inferior_state displaced_step_state;

  /* Per inferior data-pointers required by other GDB modules.  */
  registry<inferior> registry_fields;

private:

  /* Unpush TARGET and assert that it worked.  */
  void unpush_target_and_assert (struct target_ops *target);

  /* The inferior's target stack.  */
  target_stack m_target_stack;

  /* The name of terminal device to use for I/O.  */
  std::string m_terminal;

  /* The list of continuations.  */
  std::list<std::function<void ()>> m_continuations;

  /* The arguments string to use when running.  */
  std::string m_args;

  /* The current working directory that will be used when starting
     this inferior.  */
  std::string m_cwd;

  /* The architecture associated with the inferior through the
     connection to the target.

     The architecture vector provides some information that is really
     a property of the inferior, accessed through a particular target:
     ptrace operations; the layout of certain RSP packets; the
     solib_ops vector; etc.  To differentiate architecture accesses to
     per-inferior/target properties from
     per-thread/per-frame/per-objfile properties, accesses to
     per-inferior/target properties should be made through
     this gdbarch.  */
  gdbarch *m_gdbarch = nullptr;
};

/* Add an inferior to the inferior list, print a message that a new
   inferior is found, and return the pointer to the new inferior.
   Caller may use this pointer to initialize the private inferior
   data.  */
extern struct inferior *add_inferior (int pid);

/* Same as add_inferior, but don't print new inferior notifications to
   the CLI.  */
extern struct inferior *add_inferior_silent (int pid);

extern void delete_inferior (struct inferior *todel);

/* Delete an existing inferior list entry, due to inferior detaching.  */
extern void detach_inferior (inferior *inf);

/* Notify observers and interpreters that INF has gone away.  Reset the INF
   object back to an default, empty, state.  Clear register and frame
   caches.  */
extern void exit_inferior (inferior *inf);

extern void inferior_appeared (struct inferior *inf, int pid);

/* Search function to lookup an inferior of TARG by target 'pid'.  */
extern struct inferior *find_inferior_pid (process_stratum_target *targ,
					   int pid);

/* Search function to lookup an inferior of TARG whose pid is equal to
   'ptid.pid'. */
extern struct inferior *find_inferior_ptid (process_stratum_target *targ,
					    ptid_t ptid);

/* Search function to lookup an inferior by GDB 'num'.  */
extern struct inferior *find_inferior_id (int num);

/* Find an inferior bound to PSPACE, giving preference to the current
   inferior.  */
extern struct inferior *
  find_inferior_for_program_space (struct program_space *pspace);

/* Returns true if the inferior list is not empty.  */
extern int have_inferiors (void);

/* Returns the number of live inferiors running on PROC_TARGET (real
   live processes with execution).  */
extern int number_of_live_inferiors (process_stratum_target *proc_target);

/* Returns true if there are any live inferiors in the inferior list
   (not cores, not executables, real live processes).  */
extern int have_live_inferiors (void);

/* Save/restore the current inferior.  */

class scoped_restore_current_inferior
{
public:
  scoped_restore_current_inferior ()
    : m_saved_inf (current_inferior ())
  {}

  ~scoped_restore_current_inferior ()
  { set_current_inferior (m_saved_inf); }

  DISABLE_COPY_AND_ASSIGN (scoped_restore_current_inferior);

private:
  inferior *m_saved_inf;
};

/* When reading memory from an inferior, the global inferior_ptid must
   also be set.  This class arranges to save and restore the necessary
   state for reading or writing memory, but without invalidating the
   frame cache.  */

class scoped_restore_current_inferior_for_memory
{
public:

  /* Save the current globals and switch to the given inferior and the
     inferior's program space.  inferior_ptid is set to point to the
     inferior's process id (and not to any particular thread).  */
  explicit scoped_restore_current_inferior_for_memory (inferior *inf)
    : m_save_ptid (&inferior_ptid)
  {
    set_current_inferior (inf);
    set_current_program_space (inf->pspace);
    inferior_ptid = ptid_t (inf->pid);
  }

  DISABLE_COPY_AND_ASSIGN (scoped_restore_current_inferior_for_memory);

private:

  scoped_restore_current_inferior m_save_inferior;
  scoped_restore_current_program_space m_save_progspace;
  scoped_restore_tmpl<ptid_t> m_save_ptid;
};


/* Traverse all inferiors.  */

extern intrusive_list<inferior> inferior_list;

/* Pull in the internals of the inferiors ranges and iterators.  Must
   be done after struct inferior is defined.  */
#include "inferior-iter.h"

/* Return a range that can be used to walk over all inferiors
   inferiors, with range-for, safely.  I.e., it is safe to delete the
   currently-iterated inferior.  When combined with range-for, this
   allow convenient patterns like this:

     for (inferior *inf : all_inferiors_safe ())
       if (some_condition ())
	 delete inf;
*/

inline all_inferiors_safe_range
all_inferiors_safe ()
{
  return all_inferiors_safe_range (nullptr, inferior_list);
}

/* Returns a range representing all inferiors, suitable to use with
   range-for, like this:

   for (inferior *inf : all_inferiors ())
     [...]
*/

inline all_inferiors_range
all_inferiors (process_stratum_target *proc_target = nullptr)
{
  return all_inferiors_range (proc_target, inferior_list);
}

/* Return a range that can be used to walk over all inferiors with PID
   not zero, with range-for.  */

inline all_non_exited_inferiors_range
all_non_exited_inferiors (process_stratum_target *proc_target = nullptr)
{
  return all_non_exited_inferiors_range (proc_target, inferior_list);
}

/* Prune away automatically added inferiors that aren't required
   anymore.  */
extern void prune_inferiors (void);

extern int number_of_inferiors (void);

extern struct inferior *add_inferior_with_spaces (void);

/* Print the current selected inferior.  */
extern void print_selected_inferior (struct ui_out *uiout);

/* Switch to inferior NEW_INF, a new inferior, and unless
   NO_CONNECTION is true, push the process_stratum_target of ORG_INF
   to NEW_INF.  */

extern void switch_to_inferior_and_push_target
  (inferior *new_inf, bool no_connection, inferior *org_inf);

/* Return true if ID is a valid global inferior number.  */

inline bool
valid_global_inferior_id (int id)
{
  for (inferior *inf : all_inferiors ())
    if (inf->num == id)
      return true;
  return false;
}

#endif /* !defined (INFERIOR_H) */
