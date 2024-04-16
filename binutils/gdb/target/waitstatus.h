/* Target waitstatus definitions and prototypes.

   Copyright (C) 1990-2024 Free Software Foundation, Inc.

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

#ifndef TARGET_WAITSTATUS_H
#define TARGET_WAITSTATUS_H

#include "diagnostics.h"
#include "gdbsupport/gdb_signals.h"

/* Stuff for target_wait.  */

/* Generally, what has the program done?  */
enum target_waitkind
{
  /* The program has exited.  The exit status is in value.integer.  */
  TARGET_WAITKIND_EXITED,

  /* The program has stopped with a signal.  Which signal is in
     value.sig.  */
  TARGET_WAITKIND_STOPPED,

  /* The program has terminated with a signal.  Which signal is in
     value.sig.  */
  TARGET_WAITKIND_SIGNALLED,

  /* The program is letting us know that it dynamically loaded
     something (e.g. it called load(2) on AIX).  */
  TARGET_WAITKIND_LOADED,

  /* The program has forked.  A "related" process' PTID is in
     value.related_pid.  I.e., if the child forks, value.related_pid
     is the parent's ID.  */
  TARGET_WAITKIND_FORKED,
 
  /* The program has vforked.  A "related" process's PTID is in
     value.related_pid.  */
  TARGET_WAITKIND_VFORKED,
 
  /* The program has exec'ed a new executable file.  The new file's
     pathname is pointed to by value.execd_pathname.  */
  TARGET_WAITKIND_EXECD,
  
  /* The program had previously vforked, and now the child is done
     with the shared memory region, because it exec'ed or exited.
     Note that the event is reported to the vfork parent.  This is
     only used if GDB did not stay attached to the vfork child,
     otherwise, a TARGET_WAITKIND_EXECD or
     TARGET_WAITKIND_EXIT|SIGNALLED event associated with the child
     has the same effect.  */
  TARGET_WAITKIND_VFORK_DONE,

  /* The program has entered or returned from a system call.  On
     HP-UX, this is used in the hardware watchpoint implementation.
     The syscall's unique integer ID number is in
     value.syscall_id.  */
  TARGET_WAITKIND_SYSCALL_ENTRY,
  TARGET_WAITKIND_SYSCALL_RETURN,

  /* Nothing happened, but we stopped anyway.  This perhaps should
     be handled within target_wait, but I'm not sure target_wait
     should be resuming the inferior.  */
  TARGET_WAITKIND_SPURIOUS,

  /* An event has occurred, but we should wait again.
     Remote_async_wait() returns this when there is an event
     on the inferior, but the rest of the world is not interested in
     it.  The inferior has not stopped, but has just sent some output
     to the console, for instance.  In this case, we want to go back
     to the event loop and wait there for another event from the
     inferior, rather than being stuck in the remote_async_wait()
     function.  This way the event loop is responsive to other events,
     like for instance the user typing.  */
  TARGET_WAITKIND_IGNORE,
 
  /* The target has run out of history information,
     and cannot run backward any further.  */
  TARGET_WAITKIND_NO_HISTORY,
 
  /* There are no resumed children left in the program.  */
  TARGET_WAITKIND_NO_RESUMED,

  /* The thread was cloned.  The event's ptid corresponds to the
     cloned parent.  The cloned child is held stopped at its entry
     point, and its ptid is in the event's m_child_ptid.  The target
     must not add the cloned child to GDB's thread list until
     target_ops::follow_clone() is called.  */
  TARGET_WAITKIND_THREAD_CLONED,

  /* The thread was created.  */
  TARGET_WAITKIND_THREAD_CREATED,

  /* The thread has exited.  The exit status is in value.integer.  */
  TARGET_WAITKIND_THREAD_EXITED,
};

/* Determine if KIND represents an event with a new child - a fork,
   vfork, or clone.  */

static inline bool
is_new_child_status (target_waitkind kind)
{
  return (kind == TARGET_WAITKIND_FORKED
	  || kind == TARGET_WAITKIND_VFORKED
	  || kind == TARGET_WAITKIND_THREAD_CLONED);
}

/* Return KIND as a string.  */

static inline const char *
target_waitkind_str (target_waitkind kind)
{
/* Make sure the compiler warns if a new TARGET_WAITKIND enumerator is added
   but not handled here.  */
DIAGNOSTIC_PUSH
DIAGNOSTIC_ERROR_SWITCH
  switch (kind)
  {
    case TARGET_WAITKIND_EXITED:
      return "EXITED";
    case TARGET_WAITKIND_STOPPED:
      return "STOPPED";
    case TARGET_WAITKIND_SIGNALLED:
      return "SIGNALLED";
    case TARGET_WAITKIND_LOADED:
      return "LOADED";
    case TARGET_WAITKIND_FORKED:
      return "FORKED";
    case TARGET_WAITKIND_VFORKED:
      return "VFORKED";
    case TARGET_WAITKIND_THREAD_CLONED:
      return "THREAD_CLONED";
    case TARGET_WAITKIND_EXECD:
      return "EXECD";
    case TARGET_WAITKIND_VFORK_DONE:
      return "VFORK_DONE";
    case TARGET_WAITKIND_SYSCALL_ENTRY:
      return "SYSCALL_ENTRY";
    case TARGET_WAITKIND_SYSCALL_RETURN:
      return "SYSCALL_RETURN";
    case TARGET_WAITKIND_SPURIOUS:
      return "SPURIOUS";
    case TARGET_WAITKIND_IGNORE:
      return "IGNORE";
    case TARGET_WAITKIND_NO_HISTORY:
      return "NO_HISTORY";
    case TARGET_WAITKIND_NO_RESUMED:
      return "NO_RESUMED";
    case TARGET_WAITKIND_THREAD_CREATED:
      return "THREAD_CREATED";
    case TARGET_WAITKIND_THREAD_EXITED:
      return "THREAD_EXITED";
  };
DIAGNOSTIC_POP

  gdb_assert_not_reached ("invalid target_waitkind value: %d\n", (int) kind);
}

struct target_waitstatus
{
  /* Default constructor.  */
  target_waitstatus () = default;

  /* Copy constructor.  */

  target_waitstatus (const target_waitstatus &other)
  {
    m_kind = other.m_kind;
    m_value = other.m_value;

    if (m_kind == TARGET_WAITKIND_EXECD)
      m_value.execd_pathname = xstrdup (m_value.execd_pathname);
  }

  /* Move constructor.  */

  target_waitstatus (target_waitstatus &&other)
  {
    m_kind = other.m_kind;
    m_value = other.m_value;

    if (m_kind == TARGET_WAITKIND_EXECD)
      other.m_value.execd_pathname = nullptr;

    other.reset ();
  }

  /* Copy assignment operator.  */

  target_waitstatus &operator= (const target_waitstatus &rhs)
  {
    this->reset ();
    m_kind = rhs.m_kind;
    m_value = rhs.m_value;

    if (m_kind == TARGET_WAITKIND_EXECD)
      m_value.execd_pathname = xstrdup (m_value.execd_pathname);

    return *this;
  }

  /* Move assignment operator.  */

  target_waitstatus &operator= (target_waitstatus &&rhs)
  {
    this->reset ();
    m_kind = rhs.m_kind;
    m_value = rhs.m_value;

    if (m_kind == TARGET_WAITKIND_EXECD)
      rhs.m_value.execd_pathname = nullptr;

    rhs.reset ();

    return *this;
  }

  /* Destructor.  */

  ~target_waitstatus ()
  {
    this->reset ();
  }

  /* Setters: set the wait status kind plus any associated data.  */

  target_waitstatus &set_exited (int exit_status)
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_EXITED;
    m_value.exit_status = exit_status;
    return *this;
  }

  target_waitstatus &set_stopped (gdb_signal sig)
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_STOPPED;
    m_value.sig = sig;
    return *this;
  }

  target_waitstatus &set_signalled (gdb_signal sig)
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_SIGNALLED;
    m_value.sig = sig;
    return *this;
  }

  target_waitstatus &set_loaded ()
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_LOADED;
    return *this;
  }

  target_waitstatus &set_forked (ptid_t child_ptid)
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_FORKED;
    m_value.child_ptid = child_ptid;
    return *this;
  }

  target_waitstatus &set_vforked (ptid_t child_ptid)
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_VFORKED;
    m_value.child_ptid = child_ptid;
    return *this;
  }

  target_waitstatus &set_execd (gdb::unique_xmalloc_ptr<char> execd_pathname)
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_EXECD;
    m_value.execd_pathname = execd_pathname.release ();
    return *this;
  }

  target_waitstatus &set_vfork_done ()
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_VFORK_DONE;
    return *this;
  }

  target_waitstatus &set_syscall_entry (int syscall_number)
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_SYSCALL_ENTRY;
    m_value.syscall_number = syscall_number;
    return *this;
  }

  target_waitstatus &set_syscall_return (int syscall_number)
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_SYSCALL_RETURN;
    m_value.syscall_number = syscall_number;
    return *this;
  }

  target_waitstatus &set_spurious ()
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_SPURIOUS;
    return *this;
  }

  target_waitstatus &set_ignore ()
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_IGNORE;
    return *this;
  }

  target_waitstatus &set_no_history ()
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_NO_HISTORY;
    return *this;
  }

  target_waitstatus &set_no_resumed ()
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_NO_RESUMED;
    return *this;
  }

  target_waitstatus &set_thread_cloned (ptid_t child_ptid)
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_THREAD_CLONED;
    m_value.child_ptid = child_ptid;
    return *this;
  }

  target_waitstatus &set_thread_created ()
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_THREAD_CREATED;
    return *this;
  }

  target_waitstatus &set_thread_exited (int exit_status)
  {
    this->reset ();
    m_kind = TARGET_WAITKIND_THREAD_EXITED;
    m_value.exit_status = exit_status;
    return *this;
  }

  /* Get the kind of this wait status.  */

  target_waitkind kind () const
  {
    return m_kind;
  }

  /* Getters for the associated data.

     Getters can only be used if the wait status is of the appropriate kind.
     See the setters above or the assertions below to know which data is
     associated to which kind.  */

  int exit_status () const
  {
    gdb_assert (m_kind == TARGET_WAITKIND_EXITED
		|| m_kind == TARGET_WAITKIND_THREAD_EXITED);
    return m_value.exit_status;
  }

  gdb_signal sig () const
  {
    gdb_assert (m_kind == TARGET_WAITKIND_STOPPED
		|| m_kind == TARGET_WAITKIND_SIGNALLED);
    return m_value.sig;
  }

  ptid_t child_ptid () const
  {
    gdb_assert (is_new_child_status (m_kind));
    return m_value.child_ptid;
  }

  const char *execd_pathname () const
  {
    gdb_assert (m_kind == TARGET_WAITKIND_EXECD);
    return m_value.execd_pathname;
  }

  int syscall_number () const
  {
    gdb_assert (m_kind == TARGET_WAITKIND_SYSCALL_ENTRY
		|| m_kind == TARGET_WAITKIND_SYSCALL_RETURN);
    return m_value.syscall_number;
  }

  /* Return a pretty printed form of target_waitstatus.

     This is only meant to be used in debug messages, not for user-visible
     messages.  */
  std::string to_string () const;

private:
  /* Reset the wait status to its original state.  */
  void reset ()
  {
    if (m_kind == TARGET_WAITKIND_EXECD)
      xfree (m_value.execd_pathname);

    m_kind = TARGET_WAITKIND_IGNORE;
  }

  target_waitkind m_kind = TARGET_WAITKIND_IGNORE;

  /* Additional information about the event.  */
  union
    {
      /* Exit status */
      int exit_status;
      /* Signal number */
      enum gdb_signal sig;
      /* Forked child pid */
      ptid_t child_ptid;
      /* execd pathname */
      char *execd_pathname;
      /* Syscall number */
      int syscall_number;
    } m_value {};
};

/* Extended reasons that can explain why a target/thread stopped for a
   trap signal.  */

enum target_stop_reason
{
  /* Either not stopped, or stopped for a reason that doesn't require
     special tracking.  */
  TARGET_STOPPED_BY_NO_REASON,

  /* Stopped by a software breakpoint.  */
  TARGET_STOPPED_BY_SW_BREAKPOINT,

  /* Stopped by a hardware breakpoint.  */
  TARGET_STOPPED_BY_HW_BREAKPOINT,

  /* Stopped by a watchpoint.  */
  TARGET_STOPPED_BY_WATCHPOINT,

  /* Stopped by a single step finishing.  */
  TARGET_STOPPED_BY_SINGLE_STEP
};

#endif /* TARGET_WAITSTATUS_H */
