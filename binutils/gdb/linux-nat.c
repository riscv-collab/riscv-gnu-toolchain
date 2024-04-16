/* GNU/Linux native-dependent code common to multiple platforms.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "infrun.h"
#include "target.h"
#include "nat/linux-nat.h"
#include "nat/linux-waitpid.h"
#include "gdbsupport/gdb_wait.h"
#include <unistd.h>
#include <sys/syscall.h>
#include "nat/gdb_ptrace.h"
#include "linux-nat.h"
#include "nat/linux-ptrace.h"
#include "nat/linux-procfs.h"
#include "nat/linux-personality.h"
#include "linux-fork.h"
#include "gdbthread.h"
#include "gdbcmd.h"
#include "regcache.h"
#include "regset.h"
#include "inf-child.h"
#include "inf-ptrace.h"
#include "auxv.h"
#include <sys/procfs.h>
#include "elf-bfd.h"
#include "gregset.h"
#include "gdbcore.h"
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "inf-loop.h"
#include "gdbsupport/event-loop.h"
#include "event-top.h"
#include <pwd.h>
#include <sys/types.h>
#include <dirent.h>
#include "xml-support.h"
#include <sys/vfs.h>
#include "solib.h"
#include "nat/linux-osdata.h"
#include "linux-tdep.h"
#include "symfile.h"
#include "gdbsupport/agent.h"
#include "tracepoint.h"
#include "target-descriptions.h"
#include "gdbsupport/filestuff.h"
#include "objfiles.h"
#include "nat/linux-namespaces.h"
#include "gdbsupport/block-signals.h"
#include "gdbsupport/fileio.h"
#include "gdbsupport/scope-exit.h"
#include "gdbsupport/gdb-sigmask.h"
#include "gdbsupport/common-debug.h"
#include <unordered_map>

/* This comment documents high-level logic of this file.

Waiting for events in sync mode
===============================

When waiting for an event in a specific thread, we just use waitpid,
passing the specific pid, and not passing WNOHANG.

When waiting for an event in all threads, waitpid is not quite good:

- If the thread group leader exits while other threads in the thread
  group still exist, waitpid(TGID, ...) hangs.  That waitpid won't
  return an exit status until the other threads in the group are
  reaped.

- When a non-leader thread execs, that thread just vanishes without
  reporting an exit (so we'd hang if we waited for it explicitly in
  that case).  The exec event is instead reported to the TGID pid.

The solution is to always use -1 and WNOHANG, together with
sigsuspend.

First, we use non-blocking waitpid to check for events.  If nothing is
found, we use sigsuspend to wait for SIGCHLD.  When SIGCHLD arrives,
it means something happened to a child process.  As soon as we know
there's an event, we get back to calling nonblocking waitpid.

Note that SIGCHLD should be blocked between waitpid and sigsuspend
calls, so that we don't miss a signal.  If SIGCHLD arrives in between,
when it's blocked, the signal becomes pending and sigsuspend
immediately notices it and returns.

Waiting for events in async mode (TARGET_WNOHANG)
=================================================

In async mode, GDB should always be ready to handle both user input
and target events, so neither blocking waitpid nor sigsuspend are
viable options.  Instead, we should asynchronously notify the GDB main
event loop whenever there's an unprocessed event from the target.  We
detect asynchronous target events by handling SIGCHLD signals.  To
notify the event loop about target events, an event pipe is used
--- the pipe is registered as waitable event source in the event loop,
the event loop select/poll's on the read end of this pipe (as well on
other event sources, e.g., stdin), and the SIGCHLD handler marks the
event pipe to raise an event.  This is more portable than relying on
pselect/ppoll, since on kernels that lack those syscalls, libc
emulates them with select/poll+sigprocmask, and that is racy
(a.k.a. plain broken).

Obviously, if we fail to notify the event loop if there's a target
event, it's bad.  OTOH, if we notify the event loop when there's no
event from the target, linux_nat_wait will detect that there's no real
event to report, and return event of type TARGET_WAITKIND_IGNORE.
This is mostly harmless, but it will waste time and is better avoided.

The main design point is that every time GDB is outside linux-nat.c,
we have a SIGCHLD handler installed that is called when something
happens to the target and notifies the GDB event loop.  Whenever GDB
core decides to handle the event, and calls into linux-nat.c, we
process things as in sync mode, except that the we never block in
sigsuspend.

While processing an event, we may end up momentarily blocked in
waitpid calls.  Those waitpid calls, while blocking, are guarantied to
return quickly.  E.g., in all-stop mode, before reporting to the core
that an LWP hit a breakpoint, all LWPs are stopped by sending them
SIGSTOP, and synchronously waiting for the SIGSTOP to be reported.
Note that this is different from blocking indefinitely waiting for the
next event --- here, we're already handling an event.

Use of signals
==============

We stop threads by sending a SIGSTOP.  The use of SIGSTOP instead of another
signal is not entirely significant; we just need for a signal to be delivered,
so that we can intercept it.  SIGSTOP's advantage is that it can not be
blocked.  A disadvantage is that it is not a real-time signal, so it can only
be queued once; we do not keep track of other sources of SIGSTOP.

Two other signals that can't be blocked are SIGCONT and SIGKILL.  But we can't
use them, because they have special behavior when the signal is generated -
not when it is delivered.  SIGCONT resumes the entire thread group and SIGKILL
kills the entire thread group.

A delivered SIGSTOP would stop the entire thread group, not just the thread we
tkill'd.  But we never let the SIGSTOP be delivered; we always intercept and 
cancel it (by PTRACE_CONT without passing SIGSTOP).

We could use a real-time signal instead.  This would solve those problems; we
could use PTRACE_GETSIGINFO to locate the specific stop signals sent by GDB.
But we would still have to have some support for SIGSTOP, since PTRACE_ATTACH
generates it, and there are races with trying to find a signal that is not
blocked.

Exec events
===========

The case of a thread group (process) with 3 or more threads, and a
thread other than the leader execs is worth detailing:

On an exec, the Linux kernel destroys all threads except the execing
one in the thread group, and resets the execing thread's tid to the
tgid.  No exit notification is sent for the execing thread -- from the
ptracer's perspective, it appears as though the execing thread just
vanishes.  Until we reap all other threads except the leader and the
execing thread, the leader will be zombie, and the execing thread will
be in `D (disc sleep)' state.  As soon as all other threads are
reaped, the execing thread changes its tid to the tgid, and the
previous (zombie) leader vanishes, giving place to the "new"
leader.  */

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

struct linux_nat_target *linux_target;

/* Does the current host support PTRACE_GETREGSET?  */
enum tribool have_ptrace_getregset = TRIBOOL_UNKNOWN;

/* When true, print debug messages relating to the linux native target.  */

static bool debug_linux_nat;

/* Implement 'show debug linux-nat'.  */

static void
show_debug_linux_nat (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Debugging of GNU/Linux native targets is %s.\n"),
	      value);
}

/* Print a linux-nat debug statement.  */

#define linux_nat_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (debug_linux_nat, "linux-nat", fmt, ##__VA_ARGS__)

/* Print "linux-nat" enter/exit debug statements.  */

#define LINUX_NAT_SCOPED_DEBUG_ENTER_EXIT \
  scoped_debug_enter_exit (debug_linux_nat, "linux-nat")

struct simple_pid_list
{
  int pid;
  int status;
  struct simple_pid_list *next;
};
static struct simple_pid_list *stopped_pids;

/* Whether target_thread_events is in effect.  */
static int report_thread_events;

static int kill_lwp (int lwpid, int signo);

static int stop_callback (struct lwp_info *lp);

static void block_child_signals (sigset_t *prev_mask);
static void restore_child_signals_mask (sigset_t *prev_mask);

struct lwp_info;
static struct lwp_info *add_lwp (ptid_t ptid);
static void purge_lwp_list (int pid);
static void delete_lwp (ptid_t ptid);
static struct lwp_info *find_lwp_pid (ptid_t ptid);

static int lwp_status_pending_p (struct lwp_info *lp);

static void save_stop_reason (struct lwp_info *lp);

static bool proc_mem_file_is_writable ();
static void close_proc_mem_file (pid_t pid);
static void open_proc_mem_file (ptid_t ptid);

/* Return TRUE if LWP is the leader thread of the process.  */

static bool
is_leader (lwp_info *lp)
{
  return lp->ptid.pid () == lp->ptid.lwp ();
}

/* Convert an LWP's pending status to a std::string.  */

static std::string
pending_status_str (lwp_info *lp)
{
  gdb_assert (lwp_status_pending_p (lp));

  if (lp->waitstatus.kind () != TARGET_WAITKIND_IGNORE)
    return lp->waitstatus.to_string ();
  else
    return status_to_str (lp->status);
}

/* Return true if we should report exit events for LP.  */

static bool
report_exit_events_for (lwp_info *lp)
{
  thread_info *thr = linux_target->find_thread (lp->ptid);
  gdb_assert (thr != nullptr);

  return (report_thread_events
	  || (thr->thread_options () & GDB_THREAD_OPTION_EXIT) != 0);
}


/* LWP accessors.  */

/* See nat/linux-nat.h.  */

ptid_t
ptid_of_lwp (struct lwp_info *lwp)
{
  return lwp->ptid;
}

/* See nat/linux-nat.h.  */

void
lwp_set_arch_private_info (struct lwp_info *lwp,
			   struct arch_lwp_info *info)
{
  lwp->arch_private = info;
}

/* See nat/linux-nat.h.  */

struct arch_lwp_info *
lwp_arch_private_info (struct lwp_info *lwp)
{
  return lwp->arch_private;
}

/* See nat/linux-nat.h.  */

int
lwp_is_stopped (struct lwp_info *lwp)
{
  return lwp->stopped;
}

/* See nat/linux-nat.h.  */

enum target_stop_reason
lwp_stop_reason (struct lwp_info *lwp)
{
  return lwp->stop_reason;
}

/* See nat/linux-nat.h.  */

int
lwp_is_stepping (struct lwp_info *lwp)
{
  return lwp->step;
}


/* Trivial list manipulation functions to keep track of a list of
   new stopped processes.  */
static void
add_to_pid_list (struct simple_pid_list **listp, int pid, int status)
{
  struct simple_pid_list *new_pid = XNEW (struct simple_pid_list);

  new_pid->pid = pid;
  new_pid->status = status;
  new_pid->next = *listp;
  *listp = new_pid;
}

static int
pull_pid_from_list (struct simple_pid_list **listp, int pid, int *statusp)
{
  struct simple_pid_list **p;

  for (p = listp; *p != NULL; p = &(*p)->next)
    if ((*p)->pid == pid)
      {
	struct simple_pid_list *next = (*p)->next;

	*statusp = (*p)->status;
	xfree (*p);
	*p = next;
	return 1;
      }
  return 0;
}

/* Return the ptrace options that we want to try to enable.  */

static int
linux_nat_ptrace_options (int attached)
{
  int options = 0;

  if (!attached)
    options |= PTRACE_O_EXITKILL;

  options |= (PTRACE_O_TRACESYSGOOD
	      | PTRACE_O_TRACEVFORKDONE
	      | PTRACE_O_TRACEVFORK
	      | PTRACE_O_TRACEFORK
	      | PTRACE_O_TRACEEXEC);

  return options;
}

/* Initialize ptrace and procfs warnings and check for supported
   ptrace features given PID.

   ATTACHED should be nonzero iff we attached to the inferior.  */

static void
linux_init_ptrace_procfs (pid_t pid, int attached)
{
  int options = linux_nat_ptrace_options (attached);

  linux_enable_event_reporting (pid, options);
  linux_ptrace_init_warnings ();
  linux_proc_init_warnings ();
  proc_mem_file_is_writable ();
}

linux_nat_target::~linux_nat_target ()
{}

void
linux_nat_target::post_attach (int pid)
{
  linux_init_ptrace_procfs (pid, 1);
}

/* Implement the virtual inf_ptrace_target::post_startup_inferior method.  */

void
linux_nat_target::post_startup_inferior (ptid_t ptid)
{
  linux_init_ptrace_procfs (ptid.pid (), 0);
}

/* Return the number of known LWPs in the tgid given by PID.  */

static int
num_lwps (int pid)
{
  int count = 0;

  for (const lwp_info *lp ATTRIBUTE_UNUSED : all_lwps ())
    if (lp->ptid.pid () == pid)
      count++;

  return count;
}

/* Deleter for lwp_info unique_ptr specialisation.  */

struct lwp_deleter
{
  void operator() (struct lwp_info *lwp) const
  {
    delete_lwp (lwp->ptid);
  }
};

/* A unique_ptr specialisation for lwp_info.  */

typedef std::unique_ptr<struct lwp_info, lwp_deleter> lwp_info_up;

/* Target hook for follow_fork.  */

void
linux_nat_target::follow_fork (inferior *child_inf, ptid_t child_ptid,
			       target_waitkind fork_kind, bool follow_child,
			       bool detach_fork)
{
  inf_ptrace_target::follow_fork (child_inf, child_ptid, fork_kind,
				  follow_child, detach_fork);

  if (!follow_child)
    {
      bool has_vforked = fork_kind == TARGET_WAITKIND_VFORKED;
      ptid_t parent_ptid = inferior_ptid;
      int parent_pid = parent_ptid.lwp ();
      int child_pid = child_ptid.lwp ();

      /* We're already attached to the parent, by default.  */
      lwp_info *child_lp = add_lwp (child_ptid);
      child_lp->stopped = 1;
      child_lp->last_resume_kind = resume_stop;

      /* Detach new forked process?  */
      if (detach_fork)
	{
	  int child_stop_signal = 0;
	  bool detach_child = true;

	  /* Move CHILD_LP into a unique_ptr and clear the source pointer
	     to prevent us doing anything stupid with it.  */
	  lwp_info_up child_lp_ptr (child_lp);
	  child_lp = nullptr;

	  linux_target->low_prepare_to_resume (child_lp_ptr.get ());

	  /* When debugging an inferior in an architecture that supports
	     hardware single stepping on a kernel without commit
	     6580807da14c423f0d0a708108e6df6ebc8bc83d, the vfork child
	     process starts with the TIF_SINGLESTEP/X86_EFLAGS_TF bits
	     set if the parent process had them set.
	     To work around this, single step the child process
	     once before detaching to clear the flags.  */

	  /* Note that we consult the parent's architecture instead of
	     the child's because there's no inferior for the child at
	     this point.  */
	  if (!gdbarch_software_single_step_p (target_thread_architecture
					       (parent_ptid)))
	    {
	      int status;

	      linux_disable_event_reporting (child_pid);
	      if (ptrace (PTRACE_SINGLESTEP, child_pid, 0, 0) < 0)
		perror_with_name (_("Couldn't do single step"));
	      if (my_waitpid (child_pid, &status, 0) < 0)
		perror_with_name (_("Couldn't wait vfork process"));
	      else
		{
		  detach_child = WIFSTOPPED (status);
		  child_stop_signal = WSTOPSIG (status);
		}
	    }

	  if (detach_child)
	    {
	      int signo = child_stop_signal;

	      if (signo != 0
		  && !signal_pass_state (gdb_signal_from_host (signo)))
		signo = 0;
	      ptrace (PTRACE_DETACH, child_pid, 0, signo);

	      close_proc_mem_file (child_pid);
	    }
	}

      if (has_vforked)
	{
	  lwp_info *parent_lp = find_lwp_pid (parent_ptid);
	  linux_nat_debug_printf ("waiting for VFORK_DONE on %d", parent_pid);
	  parent_lp->stopped = 1;

	  /* We'll handle the VFORK_DONE event like any other
	     event, in target_wait.  */
	}
    }
  else
    {
      struct lwp_info *child_lp;

      child_lp = add_lwp (child_ptid);
      child_lp->stopped = 1;
      child_lp->last_resume_kind = resume_stop;
    }
}


int
linux_nat_target::insert_fork_catchpoint (int pid)
{
  return 0;
}

int
linux_nat_target::remove_fork_catchpoint (int pid)
{
  return 0;
}

int
linux_nat_target::insert_vfork_catchpoint (int pid)
{
  return 0;
}

int
linux_nat_target::remove_vfork_catchpoint (int pid)
{
  return 0;
}

int
linux_nat_target::insert_exec_catchpoint (int pid)
{
  return 0;
}

int
linux_nat_target::remove_exec_catchpoint (int pid)
{
  return 0;
}

int
linux_nat_target::set_syscall_catchpoint (int pid, bool needed, int any_count,
					  gdb::array_view<const int> syscall_counts)
{
  /* On GNU/Linux, we ignore the arguments.  It means that we only
     enable the syscall catchpoints, but do not disable them.

     Also, we do not use the `syscall_counts' information because we do not
     filter system calls here.  We let GDB do the logic for us.  */
  return 0;
}

/* List of known LWPs, keyed by LWP PID.  This speeds up the common
   case of mapping a PID returned from the kernel to our corresponding
   lwp_info data structure.  */
static htab_t lwp_lwpid_htab;

/* Calculate a hash from a lwp_info's LWP PID.  */

static hashval_t
lwp_info_hash (const void *ap)
{
  const struct lwp_info *lp = (struct lwp_info *) ap;
  pid_t pid = lp->ptid.lwp ();

  return iterative_hash_object (pid, 0);
}

/* Equality function for the lwp_info hash table.  Compares the LWP's
   PID.  */

static int
lwp_lwpid_htab_eq (const void *a, const void *b)
{
  const struct lwp_info *entry = (const struct lwp_info *) a;
  const struct lwp_info *element = (const struct lwp_info *) b;

  return entry->ptid.lwp () == element->ptid.lwp ();
}

/* Create the lwp_lwpid_htab hash table.  */

static void
lwp_lwpid_htab_create (void)
{
  lwp_lwpid_htab = htab_create (100, lwp_info_hash, lwp_lwpid_htab_eq, NULL);
}

/* Add LP to the hash table.  */

static void
lwp_lwpid_htab_add_lwp (struct lwp_info *lp)
{
  void **slot;

  slot = htab_find_slot (lwp_lwpid_htab, lp, INSERT);
  gdb_assert (slot != NULL && *slot == NULL);
  *slot = lp;
}

/* Head of doubly-linked list of known LWPs.  Sorted by reverse
   creation order.  This order is assumed in some cases.  E.g.,
   reaping status after killing alls lwps of a process: the leader LWP
   must be reaped last.  */

static intrusive_list<lwp_info> lwp_list;

/* See linux-nat.h.  */

lwp_info_range
all_lwps ()
{
  return lwp_info_range (lwp_list.begin ());
}

/* See linux-nat.h.  */

lwp_info_safe_range
all_lwps_safe ()
{
  return lwp_info_safe_range (lwp_list.begin ());
}

/* Add LP to sorted-by-reverse-creation-order doubly-linked list.  */

static void
lwp_list_add (struct lwp_info *lp)
{
  lwp_list.push_front (*lp);
}

/* Remove LP from sorted-by-reverse-creation-order doubly-linked
   list.  */

static void
lwp_list_remove (struct lwp_info *lp)
{
  /* Remove from sorted-by-creation-order list.  */
  lwp_list.erase (lwp_list.iterator_to (*lp));
}



/* Signal mask for use with sigsuspend in linux_nat_wait, initialized in
   _initialize_linux_nat.  */
static sigset_t suspend_mask;

/* Signals to block to make that sigsuspend work.  */
static sigset_t blocked_mask;

/* SIGCHLD action.  */
static struct sigaction sigchld_action;

/* Block child signals (SIGCHLD and linux threads signals), and store
   the previous mask in PREV_MASK.  */

static void
block_child_signals (sigset_t *prev_mask)
{
  /* Make sure SIGCHLD is blocked.  */
  if (!sigismember (&blocked_mask, SIGCHLD))
    sigaddset (&blocked_mask, SIGCHLD);

  gdb_sigmask (SIG_BLOCK, &blocked_mask, prev_mask);
}

/* Restore child signals mask, previously returned by
   block_child_signals.  */

static void
restore_child_signals_mask (sigset_t *prev_mask)
{
  gdb_sigmask (SIG_SETMASK, prev_mask, NULL);
}

/* Mask of signals to pass directly to the inferior.  */
static sigset_t pass_mask;

/* Update signals to pass to the inferior.  */
void
linux_nat_target::pass_signals
  (gdb::array_view<const unsigned char> pass_signals)
{
  int signo;

  sigemptyset (&pass_mask);

  for (signo = 1; signo < NSIG; signo++)
    {
      int target_signo = gdb_signal_from_host (signo);
      if (target_signo < pass_signals.size () && pass_signals[target_signo])
	sigaddset (&pass_mask, signo);
    }
}



/* Prototypes for local functions.  */
static int stop_wait_callback (struct lwp_info *lp);
static int resume_stopped_resumed_lwps (struct lwp_info *lp, const ptid_t wait_ptid);
static int check_ptrace_stopped_lwp_gone (struct lwp_info *lp);



/* Destroy and free LP.  */

lwp_info::~lwp_info ()
{
  /* Let the arch specific bits release arch_lwp_info.  */
  linux_target->low_delete_thread (this->arch_private);
}

/* Traversal function for purge_lwp_list.  */

static int
lwp_lwpid_htab_remove_pid (void **slot, void *info)
{
  struct lwp_info *lp = (struct lwp_info *) *slot;
  int pid = *(int *) info;

  if (lp->ptid.pid () == pid)
    {
      htab_clear_slot (lwp_lwpid_htab, slot);
      lwp_list_remove (lp);
      delete lp;
    }

  return 1;
}

/* Remove all LWPs belong to PID from the lwp list.  */

static void
purge_lwp_list (int pid)
{
  htab_traverse_noresize (lwp_lwpid_htab, lwp_lwpid_htab_remove_pid, &pid);
}

/* Add the LWP specified by PTID to the list.  PTID is the first LWP
   in the process.  Return a pointer to the structure describing the
   new LWP.

   This differs from add_lwp in that we don't let the arch specific
   bits know about this new thread.  Current clients of this callback
   take the opportunity to install watchpoints in the new thread, and
   we shouldn't do that for the first thread.  If we're spawning a
   child ("run"), the thread executes the shell wrapper first, and we
   shouldn't touch it until it execs the program we want to debug.
   For "attach", it'd be okay to call the callback, but it's not
   necessary, because watchpoints can't yet have been inserted into
   the inferior.  */

static struct lwp_info *
add_initial_lwp (ptid_t ptid)
{
  gdb_assert (ptid.lwp_p ());

  lwp_info *lp = new lwp_info (ptid);


  /* Add to sorted-by-reverse-creation-order list.  */
  lwp_list_add (lp);

  /* Add to keyed-by-pid htab.  */
  lwp_lwpid_htab_add_lwp (lp);

  return lp;
}

/* Add the LWP specified by PID to the list.  Return a pointer to the
   structure describing the new LWP.  The LWP should already be
   stopped.  */

static struct lwp_info *
add_lwp (ptid_t ptid)
{
  struct lwp_info *lp;

  lp = add_initial_lwp (ptid);

  /* Let the arch specific bits know about this new thread.  Current
     clients of this callback take the opportunity to install
     watchpoints in the new thread.  We don't do this for the first
     thread though.  See add_initial_lwp.  */
  linux_target->low_new_thread (lp);

  return lp;
}

/* Remove the LWP specified by PID from the list.  */

static void
delete_lwp (ptid_t ptid)
{
  lwp_info dummy (ptid);

  void **slot = htab_find_slot (lwp_lwpid_htab, &dummy, NO_INSERT);
  if (slot == NULL)
    return;

  lwp_info *lp = *(struct lwp_info **) slot;
  gdb_assert (lp != NULL);

  htab_clear_slot (lwp_lwpid_htab, slot);

  /* Remove from sorted-by-creation-order list.  */
  lwp_list_remove (lp);

  /* Release.  */
  delete lp;
}

/* Return a pointer to the structure describing the LWP corresponding
   to PID.  If no corresponding LWP could be found, return NULL.  */

static struct lwp_info *
find_lwp_pid (ptid_t ptid)
{
  int lwp;

  if (ptid.lwp_p ())
    lwp = ptid.lwp ();
  else
    lwp = ptid.pid ();

  lwp_info dummy (ptid_t (0, lwp));
  return (struct lwp_info *) htab_find (lwp_lwpid_htab, &dummy);
}

/* See nat/linux-nat.h.  */

struct lwp_info *
iterate_over_lwps (ptid_t filter,
		   gdb::function_view<iterate_over_lwps_ftype> callback)
{
  for (lwp_info *lp : all_lwps_safe ())
    {
      if (lp->ptid.matches (filter))
	{
	  if (callback (lp) != 0)
	    return lp;
	}
    }

  return NULL;
}

/* Update our internal state when changing from one checkpoint to
   another indicated by NEW_PTID.  We can only switch single-threaded
   applications, so we only create one new LWP, and the previous list
   is discarded.  */

void
linux_nat_switch_fork (ptid_t new_ptid)
{
  struct lwp_info *lp;

  purge_lwp_list (inferior_ptid.pid ());

  lp = add_lwp (new_ptid);
  lp->stopped = 1;

  /* This changes the thread's ptid while preserving the gdb thread
     num.  Also changes the inferior pid, while preserving the
     inferior num.  */
  thread_change_ptid (linux_target, inferior_ptid, new_ptid);

  /* We've just told GDB core that the thread changed target id, but,
     in fact, it really is a different thread, with different register
     contents.  */
  registers_changed ();
}

/* Handle the exit of a single thread LP.  If DEL_THREAD is true,
   delete the thread_info associated to LP, if it exists.  */

static void
exit_lwp (struct lwp_info *lp, bool del_thread = true)
{
  struct thread_info *th = linux_target->find_thread (lp->ptid);

  if (th != nullptr && del_thread)
    delete_thread (th);

  delete_lwp (lp->ptid);
}

/* Wait for the LWP specified by LP, which we have just attached to.
   Returns a wait status for that LWP, to cache.  */

static int
linux_nat_post_attach_wait (ptid_t ptid, int *signalled)
{
  pid_t new_pid, pid = ptid.lwp ();
  int status;

  if (linux_proc_pid_is_stopped (pid))
    {
      linux_nat_debug_printf ("Attaching to a stopped process");

      /* The process is definitely stopped.  It is in a job control
	 stop, unless the kernel predates the TASK_STOPPED /
	 TASK_TRACED distinction, in which case it might be in a
	 ptrace stop.  Make sure it is in a ptrace stop; from there we
	 can kill it, signal it, et cetera.

	 First make sure there is a pending SIGSTOP.  Since we are
	 already attached, the process can not transition from stopped
	 to running without a PTRACE_CONT; so we know this signal will
	 go into the queue.  The SIGSTOP generated by PTRACE_ATTACH is
	 probably already in the queue (unless this kernel is old
	 enough to use TASK_STOPPED for ptrace stops); but since SIGSTOP
	 is not an RT signal, it can only be queued once.  */
      kill_lwp (pid, SIGSTOP);

      /* Finally, resume the stopped process.  This will deliver the SIGSTOP
	 (or a higher priority signal, just like normal PTRACE_ATTACH).  */
      ptrace (PTRACE_CONT, pid, 0, 0);
    }

  /* Make sure the initial process is stopped.  The user-level threads
     layer might want to poke around in the inferior, and that won't
     work if things haven't stabilized yet.  */
  new_pid = my_waitpid (pid, &status, __WALL);
  gdb_assert (pid == new_pid);

  if (!WIFSTOPPED (status))
    {
      /* The pid we tried to attach has apparently just exited.  */
      linux_nat_debug_printf ("Failed to stop %d: %s", pid,
			      status_to_str (status).c_str ());
      return status;
    }

  if (WSTOPSIG (status) != SIGSTOP)
    {
      *signalled = 1;
      linux_nat_debug_printf ("Received %s after attaching",
			      status_to_str (status).c_str ());
    }

  return status;
}

void
linux_nat_target::create_inferior (const char *exec_file,
				   const std::string &allargs,
				   char **env, int from_tty)
{
  maybe_disable_address_space_randomization restore_personality
    (disable_randomization);

  /* The fork_child mechanism is synchronous and calls target_wait, so
     we have to mask the async mode.  */

  /* Make sure we report all signals during startup.  */
  pass_signals ({});

  inf_ptrace_target::create_inferior (exec_file, allargs, env, from_tty);

  open_proc_mem_file (inferior_ptid);
}

/* Callback for linux_proc_attach_tgid_threads.  Attach to PTID if not
   already attached.  Returns true if a new LWP is found, false
   otherwise.  */

static int
attach_proc_task_lwp_callback (ptid_t ptid)
{
  struct lwp_info *lp;

  /* Ignore LWPs we're already attached to.  */
  lp = find_lwp_pid (ptid);
  if (lp == NULL)
    {
      int lwpid = ptid.lwp ();

      if (ptrace (PTRACE_ATTACH, lwpid, 0, 0) < 0)
	{
	  int err = errno;

	  /* Be quiet if we simply raced with the thread exiting.
	     EPERM is returned if the thread's task still exists, and
	     is marked as exited or zombie, as well as other
	     conditions, so in that case, confirm the status in
	     /proc/PID/status.  */
	  if (err == ESRCH
	      || (err == EPERM && linux_proc_pid_is_gone (lwpid)))
	    {
	      linux_nat_debug_printf
		("Cannot attach to lwp %d: thread is gone (%d: %s)",
		 lwpid, err, safe_strerror (err));

	    }
	  else
	    {
	      std::string reason
		= linux_ptrace_attach_fail_reason_string (ptid, err);

	      error (_("Cannot attach to lwp %d: %s"),
		     lwpid, reason.c_str ());
	    }
	}
      else
	{
	  linux_nat_debug_printf ("PTRACE_ATTACH %s, 0, 0 (OK)",
				  ptid.to_string ().c_str ());

	  lp = add_lwp (ptid);

	  /* The next time we wait for this LWP we'll see a SIGSTOP as
	     PTRACE_ATTACH brings it to a halt.  */
	  lp->signalled = 1;

	  /* We need to wait for a stop before being able to make the
	     next ptrace call on this LWP.  */
	  lp->must_set_ptrace_flags = 1;

	  /* So that wait collects the SIGSTOP.  */
	  lp->resumed = 1;
	}

      return 1;
    }
  return 0;
}

void
linux_nat_target::attach (const char *args, int from_tty)
{
  struct lwp_info *lp;
  int status;
  ptid_t ptid;

  /* Make sure we report all signals during attach.  */
  pass_signals ({});

  try
    {
      inf_ptrace_target::attach (args, from_tty);
    }
  catch (const gdb_exception_error &ex)
    {
      pid_t pid = parse_pid_to_attach (args);
      std::string reason = linux_ptrace_attach_fail_reason (pid);

      if (!reason.empty ())
	throw_error (ex.error, "warning: %s\n%s", reason.c_str (),
		     ex.what ());
      else
	throw_error (ex.error, "%s", ex.what ());
    }

  /* The ptrace base target adds the main thread with (pid,0,0)
     format.  Decorate it with lwp info.  */
  ptid = ptid_t (inferior_ptid.pid (),
		 inferior_ptid.pid ());
  thread_change_ptid (linux_target, inferior_ptid, ptid);

  /* Add the initial process as the first LWP to the list.  */
  lp = add_initial_lwp (ptid);

  status = linux_nat_post_attach_wait (lp->ptid, &lp->signalled);
  if (!WIFSTOPPED (status))
    {
      if (WIFEXITED (status))
	{
	  int exit_code = WEXITSTATUS (status);

	  target_terminal::ours ();
	  target_mourn_inferior (inferior_ptid);
	  if (exit_code == 0)
	    error (_("Unable to attach: program exited normally."));
	  else
	    error (_("Unable to attach: program exited with code %d."),
		   exit_code);
	}
      else if (WIFSIGNALED (status))
	{
	  enum gdb_signal signo;

	  target_terminal::ours ();
	  target_mourn_inferior (inferior_ptid);

	  signo = gdb_signal_from_host (WTERMSIG (status));
	  error (_("Unable to attach: program terminated with signal "
		   "%s, %s."),
		 gdb_signal_to_name (signo),
		 gdb_signal_to_string (signo));
	}

      internal_error (_("unexpected status %d for PID %ld"),
		      status, (long) ptid.lwp ());
    }

  lp->stopped = 1;

  open_proc_mem_file (lp->ptid);

  /* Save the wait status to report later.  */
  lp->resumed = 1;
  linux_nat_debug_printf ("waitpid %ld, saving status %s",
			  (long) lp->ptid.pid (),
			  status_to_str (status).c_str ());

  lp->status = status;

  /* We must attach to every LWP.  If /proc is mounted, use that to
     find them now.  The inferior may be using raw clone instead of
     using pthreads.  But even if it is using pthreads, thread_db
     walks structures in the inferior's address space to find the list
     of threads/LWPs, and those structures may well be corrupted.
     Note that once thread_db is loaded, we'll still use it to list
     threads and associate pthread info with each LWP.  */
  try
    {
      linux_proc_attach_tgid_threads (lp->ptid.pid (),
				      attach_proc_task_lwp_callback);
    }
  catch (const gdb_exception_error &)
    {
      /* Failed to attach to some LWP.  Detach any we've already
	 attached to.  */
      iterate_over_lwps (ptid_t (ptid.pid ()),
			 [] (struct lwp_info *lwp) -> int
			 {
			   /* Ignore errors when detaching.  */
			   ptrace (PTRACE_DETACH, lwp->ptid.lwp (), 0, 0);
			   delete_lwp (lwp->ptid);
			   return 0;
			 });

      target_terminal::ours ();
      target_mourn_inferior (inferior_ptid);

      throw;
    }

  /* Add all the LWPs to gdb's thread list.  */
  iterate_over_lwps (ptid_t (ptid.pid ()),
		     [] (struct lwp_info *lwp) -> int
		     {
		       if (lwp->ptid.pid () != lwp->ptid.lwp ())
			 {
			   add_thread (linux_target, lwp->ptid);
			   set_running (linux_target, lwp->ptid, true);
			   set_executing (linux_target, lwp->ptid, true);
			 }
		       return 0;
		     });
}

/* Ptrace-detach the thread with pid PID.  */

static void
detach_one_pid (int pid, int signo)
{
  if (ptrace (PTRACE_DETACH, pid, 0, signo) < 0)
    {
      int save_errno = errno;

      /* We know the thread exists, so ESRCH must mean the lwp is
	 zombie.  This can happen if one of the already-detached
	 threads exits the whole thread group.  In that case we're
	 still attached, and must reap the lwp.  */
      if (save_errno == ESRCH)
	{
	  int ret, status;

	  ret = my_waitpid (pid, &status, __WALL);
	  if (ret == -1)
	    {
	      warning (_("Couldn't reap LWP %d while detaching: %s"),
		       pid, safe_strerror (errno));
	    }
	  else if (!WIFEXITED (status) && !WIFSIGNALED (status))
	    {
	      warning (_("Reaping LWP %d while detaching "
			 "returned unexpected status 0x%x"),
		       pid, status);
	    }
	}
      else
	error (_("Can't detach %d: %s"),
	       pid, safe_strerror (save_errno));
    }
  else
    linux_nat_debug_printf ("PTRACE_DETACH (%d, %s, 0) (OK)",
			    pid, strsignal (signo));
}

/* Get pending signal of THREAD as a host signal number, for detaching
   purposes.  This is the signal the thread last stopped for, which we
   need to deliver to the thread when detaching, otherwise, it'd be
   suppressed/lost.  */

static int
get_detach_signal (struct lwp_info *lp)
{
  enum gdb_signal signo = GDB_SIGNAL_0;

  /* If we paused threads momentarily, we may have stored pending
     events in lp->status or lp->waitstatus (see stop_wait_callback),
     and GDB core hasn't seen any signal for those threads.
     Otherwise, the last signal reported to the core is found in the
     thread object's stop_signal.

     There's a corner case that isn't handled here at present.  Only
     if the thread stopped with a TARGET_WAITKIND_STOPPED does
     stop_signal make sense as a real signal to pass to the inferior.
     Some catchpoint related events, like
     TARGET_WAITKIND_(V)FORK|EXEC|SYSCALL, have their stop_signal set
     to GDB_SIGNAL_SIGTRAP when the catchpoint triggers.  But,
     those traps are debug API (ptrace in our case) related and
     induced; the inferior wouldn't see them if it wasn't being
     traced.  Hence, we should never pass them to the inferior, even
     when set to pass state.  Since this corner case isn't handled by
     infrun.c when proceeding with a signal, for consistency, neither
     do we handle it here (or elsewhere in the file we check for
     signal pass state).  Normally SIGTRAP isn't set to pass state, so
     this is really a corner case.  */

  if (lp->waitstatus.kind () != TARGET_WAITKIND_IGNORE)
    signo = GDB_SIGNAL_0; /* a pending ptrace event, not a real signal.  */
  else if (lp->status)
    signo = gdb_signal_from_host (WSTOPSIG (lp->status));
  else
    {
      thread_info *tp = linux_target->find_thread (lp->ptid);

      if (target_is_non_stop_p () && !tp->executing ())
	{
	  if (tp->has_pending_waitstatus ())
	    {
	      /* If the thread has a pending event, and it was stopped with a
		 signal, use that signal to resume it.  If it has a pending
		 event of another kind, it was not stopped with a signal, so
		 resume it without a signal.  */
	      if (tp->pending_waitstatus ().kind () == TARGET_WAITKIND_STOPPED)
		signo = tp->pending_waitstatus ().sig ();
	      else
		signo = GDB_SIGNAL_0;
	    }
	  else
	    signo = tp->stop_signal ();
	}
      else if (!target_is_non_stop_p ())
	{
	  ptid_t last_ptid;
	  process_stratum_target *last_target;

	  get_last_target_status (&last_target, &last_ptid, nullptr);

	  if (last_target == linux_target
	      && lp->ptid.lwp () == last_ptid.lwp ())
	    signo = tp->stop_signal ();
	}
    }

  if (signo == GDB_SIGNAL_0)
    {
      linux_nat_debug_printf ("lwp %s has no pending signal",
			      lp->ptid.to_string ().c_str ());
    }
  else if (!signal_pass_state (signo))
    {
      linux_nat_debug_printf
	("lwp %s had signal %s but it is in no pass state",
	 lp->ptid.to_string ().c_str (), gdb_signal_to_string (signo));
    }
  else
    {
      linux_nat_debug_printf ("lwp %s has pending signal %s",
			      lp->ptid.to_string ().c_str (),
			      gdb_signal_to_string (signo));

      return gdb_signal_to_host (signo);
    }

  return 0;
}

/* If LP has a pending fork/vfork/clone status, return it.  */

static std::optional<target_waitstatus>
get_pending_child_status (lwp_info *lp)
{
  LINUX_NAT_SCOPED_DEBUG_ENTER_EXIT;

  linux_nat_debug_printf ("lwp %s (stopped = %d)",
			  lp->ptid.to_string ().c_str (), lp->stopped);

  /* Check in lwp_info::status.  */
  if (WIFSTOPPED (lp->status) && linux_is_extended_waitstatus (lp->status))
    {
      int event = linux_ptrace_get_extended_event (lp->status);

      if (event == PTRACE_EVENT_FORK
	  || event == PTRACE_EVENT_VFORK
	  || event == PTRACE_EVENT_CLONE)
	{
	  unsigned long child_pid;
	  int ret = ptrace (PTRACE_GETEVENTMSG, lp->ptid.lwp (), 0, &child_pid);
	  if (ret == 0)
	    {
	      target_waitstatus ws;

	      if (event == PTRACE_EVENT_FORK)
		ws.set_forked (ptid_t (child_pid, child_pid));
	      else if (event == PTRACE_EVENT_VFORK)
		ws.set_vforked (ptid_t (child_pid, child_pid));
	      else if (event == PTRACE_EVENT_CLONE)
		ws.set_thread_cloned (ptid_t (lp->ptid.pid (), child_pid));
	      else
		gdb_assert_not_reached ("unhandled");

	      return ws;
	    }
	  else
	    {
	      perror_warning_with_name (_("Failed to retrieve event msg"));
	      return {};
	    }
	}
    }

  /* Check in lwp_info::waitstatus.  */
  if (is_new_child_status (lp->waitstatus.kind ()))
    return lp->waitstatus;

  thread_info *tp = linux_target->find_thread (lp->ptid);

  /* Check in thread_info::pending_waitstatus.  */
  if (tp->has_pending_waitstatus ()
      && is_new_child_status (tp->pending_waitstatus ().kind ()))
    return tp->pending_waitstatus ();

  /* Check in thread_info::pending_follow.  */
  if (is_new_child_status (tp->pending_follow.kind ()))
    return tp->pending_follow;

  return {};
}

/* Detach from LP.  If SIGNO_P is non-NULL, then it points to the
   signal number that should be passed to the LWP when detaching.
   Otherwise pass any pending signal the LWP may have, if any.  */

static void
detach_one_lwp (struct lwp_info *lp, int *signo_p)
{
  int lwpid = lp->ptid.lwp ();
  int signo;

  /* If the lwp/thread we are about to detach has a pending fork/clone
     event, there is a process/thread GDB is attached to that the core
     of GDB doesn't know about.  Detach from it.  */

  std::optional<target_waitstatus> ws = get_pending_child_status (lp);
  if (ws.has_value ())
    detach_one_pid (ws->child_ptid ().lwp (), 0);

  /* If there is a pending SIGSTOP, get rid of it.  */
  if (lp->signalled)
    {
      linux_nat_debug_printf ("Sending SIGCONT to %s",
			      lp->ptid.to_string ().c_str ());

      kill_lwp (lwpid, SIGCONT);
      lp->signalled = 0;
    }

  /* If the lwp has exited or was terminated due to a signal, there's
     nothing left to do.  */
  if (lp->waitstatus.kind () == TARGET_WAITKIND_EXITED
      || lp->waitstatus.kind () == TARGET_WAITKIND_THREAD_EXITED
      || lp->waitstatus.kind () == TARGET_WAITKIND_SIGNALLED)
    {
      linux_nat_debug_printf
	("Can't detach %s - it has exited or was terminated: %s.",
	 lp->ptid.to_string ().c_str (),
	 lp->waitstatus.to_string ().c_str ());
      delete_lwp (lp->ptid);
      return;
    }

  if (signo_p == NULL)
    {
      /* Pass on any pending signal for this LWP.  */
      signo = get_detach_signal (lp);
    }
  else
    signo = *signo_p;

  linux_nat_debug_printf ("preparing to resume lwp %s (stopped = %d)",
			  lp->ptid.to_string ().c_str (),
			  lp->stopped);

  /* Preparing to resume may try to write registers, and fail if the
     lwp is zombie.  If that happens, ignore the error.  We'll handle
     it below, when detach fails with ESRCH.  */
  try
    {
      linux_target->low_prepare_to_resume (lp);
    }
  catch (const gdb_exception_error &ex)
    {
      if (!check_ptrace_stopped_lwp_gone (lp))
	throw;
    }

  detach_one_pid (lwpid, signo);

  delete_lwp (lp->ptid);
}

static int
detach_callback (struct lwp_info *lp)
{
  /* We don't actually detach from the thread group leader just yet.
     If the thread group exits, we must reap the zombie clone lwps
     before we're able to reap the leader.  */
  if (lp->ptid.lwp () != lp->ptid.pid ())
    detach_one_lwp (lp, NULL);
  return 0;
}

void
linux_nat_target::detach (inferior *inf, int from_tty)
{
  LINUX_NAT_SCOPED_DEBUG_ENTER_EXIT;

  struct lwp_info *main_lwp;
  int pid = inf->pid;

  /* Don't unregister from the event loop, as there may be other
     inferiors running. */

  /* Stop all threads before detaching.  ptrace requires that the
     thread is stopped to successfully detach.  */
  iterate_over_lwps (ptid_t (pid), stop_callback);
  /* ... and wait until all of them have reported back that
     they're no longer running.  */
  iterate_over_lwps (ptid_t (pid), stop_wait_callback);

  /* We can now safely remove breakpoints.  We don't this in earlier
     in common code because this target doesn't currently support
     writing memory while the inferior is running.  */
  remove_breakpoints_inf (current_inferior ());

  iterate_over_lwps (ptid_t (pid), detach_callback);

  /* We have detached from everything except the main thread now, so
     should only have one thread left.  However, in non-stop mode the
     main thread might have exited, in which case we'll have no threads
     left.  */
  gdb_assert (num_lwps (pid) == 1
	      || (target_is_non_stop_p () && num_lwps (pid) == 0));

  if (pid == inferior_ptid.pid () && forks_exist_p ())
    {
      /* Multi-fork case.  The current inferior_ptid is being detached
	 from, but there are other viable forks to debug.  Detach from
	 the current fork, and context-switch to the first
	 available.  */
      linux_fork_detach (from_tty, find_lwp_pid (ptid_t (pid)));
    }
  else
    {
      target_announce_detach (from_tty);

      /* In non-stop mode it is possible that the main thread has exited,
	 in which case we don't try to detach.  */
      main_lwp = find_lwp_pid (ptid_t (pid));
      if (main_lwp != nullptr)
	{
	  /* Pass on any pending signal for the last LWP.  */
	  int signo = get_detach_signal (main_lwp);

	  detach_one_lwp (main_lwp, &signo);
	}
      else
	gdb_assert (target_is_non_stop_p ());

      detach_success (inf);
    }

  close_proc_mem_file (pid);
}

/* Resume execution of the inferior process.  If STEP is nonzero,
   single-step it.  If SIGNAL is nonzero, give it that signal.  */

static void
linux_resume_one_lwp_throw (struct lwp_info *lp, int step,
			    enum gdb_signal signo)
{
  lp->step = step;

  /* stop_pc doubles as the PC the LWP had when it was last resumed.
     We only presently need that if the LWP is stepped though (to
     handle the case of stepping a breakpoint instruction).  */
  if (step)
    {
      struct regcache *regcache = get_thread_regcache (linux_target, lp->ptid);

      lp->stop_pc = regcache_read_pc (regcache);
    }
  else
    lp->stop_pc = 0;

  linux_target->low_prepare_to_resume (lp);
  linux_target->low_resume (lp->ptid, step, signo);

  /* Successfully resumed.  Clear state that no longer makes sense,
     and mark the LWP as running.  Must not do this before resuming
     otherwise if that fails other code will be confused.  E.g., we'd
     later try to stop the LWP and hang forever waiting for a stop
     status.  Note that we must not throw after this is cleared,
     otherwise handle_zombie_lwp_error would get confused.  */
  lp->stopped = 0;
  lp->core = -1;
  lp->stop_reason = TARGET_STOPPED_BY_NO_REASON;
  registers_changed_ptid (linux_target, lp->ptid);
}

/* Called when we try to resume a stopped LWP and that errors out.  If
   the LWP is no longer in ptrace-stopped state (meaning it's zombie,
   or about to become), discard the error, clear any pending status
   the LWP may have, and return true (we'll collect the exit status
   soon enough).  Otherwise, return false.  */

static int
check_ptrace_stopped_lwp_gone (struct lwp_info *lp)
{
  /* If we get an error after resuming the LWP successfully, we'd
     confuse !T state for the LWP being gone.  */
  gdb_assert (lp->stopped);

  /* We can't just check whether the LWP is in 'Z (Zombie)' state,
     because even if ptrace failed with ESRCH, the tracee may be "not
     yet fully dead", but already refusing ptrace requests.  In that
     case the tracee has 'R (Running)' state for a little bit
     (observed in Linux 3.18).  See also the note on ESRCH in the
     ptrace(2) man page.  Instead, check whether the LWP has any state
     other than ptrace-stopped.  */

  /* Don't assume anything if /proc/PID/status can't be read.  */
  if (linux_proc_pid_is_trace_stopped_nowarn (lp->ptid.lwp ()) == 0)
    {
      lp->stop_reason = TARGET_STOPPED_BY_NO_REASON;
      lp->status = 0;
      lp->waitstatus.set_ignore ();
      return 1;
    }
  return 0;
}

/* Like linux_resume_one_lwp_throw, but no error is thrown if the LWP
   disappears while we try to resume it.  */

static void
linux_resume_one_lwp (struct lwp_info *lp, int step, enum gdb_signal signo)
{
  try
    {
      linux_resume_one_lwp_throw (lp, step, signo);
    }
  catch (const gdb_exception_error &ex)
    {
      if (!check_ptrace_stopped_lwp_gone (lp))
	throw;
    }
}

/* Resume LP.  */

static void
resume_lwp (struct lwp_info *lp, int step, enum gdb_signal signo)
{
  if (lp->stopped)
    {
      struct inferior *inf = find_inferior_ptid (linux_target, lp->ptid);

      if (inf->vfork_child != NULL)
	{
	  linux_nat_debug_printf ("Not resuming sibling %s (vfork parent)",
				  lp->ptid.to_string ().c_str ());
	}
      else if (!lwp_status_pending_p (lp))
	{
	  linux_nat_debug_printf ("Resuming sibling %s, %s, %s",
				  lp->ptid.to_string ().c_str (),
				  (signo != GDB_SIGNAL_0
				   ? strsignal (gdb_signal_to_host (signo))
				   : "0"),
				  step ? "step" : "resume");

	  linux_resume_one_lwp (lp, step, signo);
	}
      else
	{
	  linux_nat_debug_printf ("Not resuming sibling %s (has pending)",
				  lp->ptid.to_string ().c_str ());
	}
    }
  else
    linux_nat_debug_printf ("Not resuming sibling %s (not stopped)",
			    lp->ptid.to_string ().c_str ());
}

/* Callback for iterate_over_lwps.  If LWP is EXCEPT, do nothing.
   Resume LWP with the last stop signal, if it is in pass state.  */

static int
linux_nat_resume_callback (struct lwp_info *lp, struct lwp_info *except)
{
  enum gdb_signal signo = GDB_SIGNAL_0;

  if (lp == except)
    return 0;

  if (lp->stopped)
    {
      struct thread_info *thread;

      thread = linux_target->find_thread (lp->ptid);
      if (thread != NULL)
	{
	  signo = thread->stop_signal ();
	  thread->set_stop_signal (GDB_SIGNAL_0);
	}
    }

  resume_lwp (lp, 0, signo);
  return 0;
}

static int
resume_clear_callback (struct lwp_info *lp)
{
  lp->resumed = 0;
  lp->last_resume_kind = resume_stop;
  return 0;
}

static int
resume_set_callback (struct lwp_info *lp)
{
  lp->resumed = 1;
  lp->last_resume_kind = resume_continue;
  return 0;
}

void
linux_nat_target::resume (ptid_t scope_ptid, int step, enum gdb_signal signo)
{
  struct lwp_info *lp;

  linux_nat_debug_printf ("Preparing to %s %s, %s, inferior_ptid %s",
			  step ? "step" : "resume",
			  scope_ptid.to_string ().c_str (),
			  (signo != GDB_SIGNAL_0
			   ? strsignal (gdb_signal_to_host (signo)) : "0"),
			  inferior_ptid.to_string ().c_str ());

  /* Mark the lwps we're resuming as resumed and update their
     last_resume_kind to resume_continue.  */
  iterate_over_lwps (scope_ptid, resume_set_callback);

  lp = find_lwp_pid (inferior_ptid);
  gdb_assert (lp != NULL);

  /* Remember if we're stepping.  */
  lp->last_resume_kind = step ? resume_step : resume_continue;

  /* If we have a pending wait status for this thread, there is no
     point in resuming the process.  But first make sure that
     linux_nat_wait won't preemptively handle the event - we
     should never take this short-circuit if we are going to
     leave LP running, since we have skipped resuming all the
     other threads.  This bit of code needs to be synchronized
     with linux_nat_wait.  */

  if (lp->status && WIFSTOPPED (lp->status))
    {
      if (!lp->step
	  && WSTOPSIG (lp->status)
	  && sigismember (&pass_mask, WSTOPSIG (lp->status)))
	{
	  linux_nat_debug_printf
	    ("Not short circuiting for ignored status 0x%x", lp->status);

	  /* FIXME: What should we do if we are supposed to continue
	     this thread with a signal?  */
	  gdb_assert (signo == GDB_SIGNAL_0);
	  signo = gdb_signal_from_host (WSTOPSIG (lp->status));
	  lp->status = 0;
	}
    }

  if (lwp_status_pending_p (lp))
    {
      /* FIXME: What should we do if we are supposed to continue
	 this thread with a signal?  */
      gdb_assert (signo == GDB_SIGNAL_0);

      linux_nat_debug_printf ("Short circuiting for status %s",
			      pending_status_str (lp).c_str ());

      if (target_can_async_p ())
	{
	  target_async (true);
	  /* Tell the event loop we have something to process.  */
	  async_file_mark ();
	}
      return;
    }

  /* No use iterating unless we're resuming other threads.  */
  if (scope_ptid != lp->ptid)
    iterate_over_lwps (scope_ptid, [=] (struct lwp_info *info)
      {
	return linux_nat_resume_callback (info, lp);
      });

  linux_nat_debug_printf ("%s %s, %s (resume event thread)",
			  step ? "PTRACE_SINGLESTEP" : "PTRACE_CONT",
			  lp->ptid.to_string ().c_str (),
			  (signo != GDB_SIGNAL_0
			   ? strsignal (gdb_signal_to_host (signo)) : "0"));

  linux_resume_one_lwp (lp, step, signo);
}

/* Send a signal to an LWP.  */

static int
kill_lwp (int lwpid, int signo)
{
  int ret;

  errno = 0;
  ret = syscall (__NR_tkill, lwpid, signo);
  if (errno == ENOSYS)
    {
      /* If tkill fails, then we are not using nptl threads, a
	 configuration we no longer support.  */
      perror_with_name (("tkill"));
    }
  return ret;
}

/* Handle a GNU/Linux syscall trap wait response.  If we see a syscall
   event, check if the core is interested in it: if not, ignore the
   event, and keep waiting; otherwise, we need to toggle the LWP's
   syscall entry/exit status, since the ptrace event itself doesn't
   indicate it, and report the trap to higher layers.  */

static int
linux_handle_syscall_trap (struct lwp_info *lp, int stopping)
{
  struct target_waitstatus *ourstatus = &lp->waitstatus;
  struct gdbarch *gdbarch = target_thread_architecture (lp->ptid);
  thread_info *thread = linux_target->find_thread (lp->ptid);
  int syscall_number = (int) gdbarch_get_syscall_number (gdbarch, thread);

  if (stopping)
    {
      /* If we're stopping threads, there's a SIGSTOP pending, which
	 makes it so that the LWP reports an immediate syscall return,
	 followed by the SIGSTOP.  Skip seeing that "return" using
	 PTRACE_CONT directly, and let stop_wait_callback collect the
	 SIGSTOP.  Later when the thread is resumed, a new syscall
	 entry event.  If we didn't do this (and returned 0), we'd
	 leave a syscall entry pending, and our caller, by using
	 PTRACE_CONT to collect the SIGSTOP, skips the syscall return
	 itself.  Later, when the user re-resumes this LWP, we'd see
	 another syscall entry event and we'd mistake it for a return.

	 If stop_wait_callback didn't force the SIGSTOP out of the LWP
	 (leaving immediately with LWP->signalled set, without issuing
	 a PTRACE_CONT), it would still be problematic to leave this
	 syscall enter pending, as later when the thread is resumed,
	 it would then see the same syscall exit mentioned above,
	 followed by the delayed SIGSTOP, while the syscall didn't
	 actually get to execute.  It seems it would be even more
	 confusing to the user.  */

      linux_nat_debug_printf
	("ignoring syscall %d for LWP %ld (stopping threads), resuming with "
	 "PTRACE_CONT for SIGSTOP", syscall_number, lp->ptid.lwp ());

      lp->syscall_state = TARGET_WAITKIND_IGNORE;
      ptrace (PTRACE_CONT, lp->ptid.lwp (), 0, 0);
      lp->stopped = 0;
      return 1;
    }

  /* Always update the entry/return state, even if this particular
     syscall isn't interesting to the core now.  In async mode,
     the user could install a new catchpoint for this syscall
     between syscall enter/return, and we'll need to know to
     report a syscall return if that happens.  */
  lp->syscall_state = (lp->syscall_state == TARGET_WAITKIND_SYSCALL_ENTRY
		       ? TARGET_WAITKIND_SYSCALL_RETURN
		       : TARGET_WAITKIND_SYSCALL_ENTRY);

  if (catch_syscall_enabled ())
    {
      if (catching_syscall_number (syscall_number))
	{
	  /* Alright, an event to report.  */
	  if (lp->syscall_state == TARGET_WAITKIND_SYSCALL_ENTRY)
	    ourstatus->set_syscall_entry (syscall_number);
	  else if (lp->syscall_state == TARGET_WAITKIND_SYSCALL_RETURN)
	    ourstatus->set_syscall_return (syscall_number);
	  else
	    gdb_assert_not_reached ("unexpected syscall state");

	  linux_nat_debug_printf
	    ("stopping for %s of syscall %d for LWP %ld",
	     (lp->syscall_state == TARGET_WAITKIND_SYSCALL_ENTRY
	      ? "entry" : "return"), syscall_number, lp->ptid.lwp ());

	  return 0;
	}

      linux_nat_debug_printf
	("ignoring %s of syscall %d for LWP %ld",
	 (lp->syscall_state == TARGET_WAITKIND_SYSCALL_ENTRY
	  ? "entry" : "return"), syscall_number, lp->ptid.lwp ());
    }
  else
    {
      /* If we had been syscall tracing, and hence used PT_SYSCALL
	 before on this LWP, it could happen that the user removes all
	 syscall catchpoints before we get to process this event.
	 There are two noteworthy issues here:

	 - When stopped at a syscall entry event, resuming with
	   PT_STEP still resumes executing the syscall and reports a
	   syscall return.

	 - Only PT_SYSCALL catches syscall enters.  If we last
	   single-stepped this thread, then this event can't be a
	   syscall enter.  If we last single-stepped this thread, this
	   has to be a syscall exit.

	 The points above mean that the next resume, be it PT_STEP or
	 PT_CONTINUE, can not trigger a syscall trace event.  */
      linux_nat_debug_printf
	("caught syscall event with no syscall catchpoints. %d for LWP %ld, "
	 "ignoring", syscall_number, lp->ptid.lwp ());
      lp->syscall_state = TARGET_WAITKIND_IGNORE;
    }

  /* The core isn't interested in this event.  For efficiency, avoid
     stopping all threads only to have the core resume them all again.
     Since we're not stopping threads, if we're still syscall tracing
     and not stepping, we can't use PTRACE_CONT here, as we'd miss any
     subsequent syscall.  Simply resume using the inf-ptrace layer,
     which knows when to use PT_SYSCALL or PT_CONTINUE.  */

  linux_resume_one_lwp (lp, lp->step, GDB_SIGNAL_0);
  return 1;
}

/* See target.h.  */

void
linux_nat_target::follow_clone (ptid_t child_ptid)
{
  lwp_info *new_lp = add_lwp (child_ptid);
  new_lp->stopped = 1;

  /* If the thread_db layer is active, let it record the user
     level thread id and status, and add the thread to GDB's
     list.  */
  if (!thread_db_notice_clone (inferior_ptid, new_lp->ptid))
    {
      /* The process is not using thread_db.  Add the LWP to
	 GDB's list.  */
      add_thread (linux_target, new_lp->ptid);
    }

  /* We just created NEW_LP so it cannot yet contain STATUS.  */
  gdb_assert (new_lp->status == 0);

  if (!pull_pid_from_list (&stopped_pids, child_ptid.lwp (), &new_lp->status))
    internal_error (_("no saved status for clone lwp"));

  if (WSTOPSIG (new_lp->status) != SIGSTOP)
    {
      /* This can happen if someone starts sending signals to
	 the new thread before it gets a chance to run, which
	 have a lower number than SIGSTOP (e.g. SIGUSR1).
	 This is an unlikely case, and harder to handle for
	 fork / vfork than for clone, so we do not try - but
	 we handle it for clone events here.  */

      new_lp->signalled = 1;

      /* Save the wait status to report later.  */
      linux_nat_debug_printf
	("waitpid of new LWP %ld, saving status %s",
	 (long) new_lp->ptid.lwp (), status_to_str (new_lp->status).c_str ());
    }
  else
    {
      new_lp->status = 0;

      if (report_thread_events)
	new_lp->waitstatus.set_thread_created ();
    }
}

/* Handle a GNU/Linux extended wait response.  If we see a clone
   event, we need to add the new LWP to our list (and not report the
   trap to higher layers).  This function returns non-zero if the
   event should be ignored and we should wait again.  If STOPPING is
   true, the new LWP remains stopped, otherwise it is continued.  */

static int
linux_handle_extended_wait (struct lwp_info *lp, int status)
{
  int pid = lp->ptid.lwp ();
  struct target_waitstatus *ourstatus = &lp->waitstatus;
  int event = linux_ptrace_get_extended_event (status);

  /* All extended events we currently use are mid-syscall.  Only
     PTRACE_EVENT_STOP is delivered more like a signal-stop, but
     you have to be using PTRACE_SEIZE to get that.  */
  lp->syscall_state = TARGET_WAITKIND_SYSCALL_ENTRY;

  if (event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK
      || event == PTRACE_EVENT_CLONE)
    {
      unsigned long new_pid;
      int ret;

      ptrace (PTRACE_GETEVENTMSG, pid, 0, &new_pid);

      /* If we haven't already seen the new PID stop, wait for it now.  */
      if (! pull_pid_from_list (&stopped_pids, new_pid, &status))
	{
	  /* The new child has a pending SIGSTOP.  We can't affect it until it
	     hits the SIGSTOP, but we're already attached.  */
	  ret = my_waitpid (new_pid, &status, __WALL);
	  if (ret == -1)
	    perror_with_name (_("waiting for new child"));
	  else if (ret != new_pid)
	    internal_error (_("wait returned unexpected PID %d"), ret);
	  else if (!WIFSTOPPED (status))
	    internal_error (_("wait returned unexpected status 0x%x"), status);
	}

      if (event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK)
	{
	  open_proc_mem_file (ptid_t (new_pid, new_pid));

	  /* The arch-specific native code may need to know about new
	     forks even if those end up never mapped to an
	     inferior.  */
	  linux_target->low_new_fork (lp, new_pid);
	}
      else if (event == PTRACE_EVENT_CLONE)
	{
	  linux_target->low_new_clone (lp, new_pid);
	}

      if (event == PTRACE_EVENT_FORK
	  && linux_fork_checkpointing_p (lp->ptid.pid ()))
	{
	  /* Handle checkpointing by linux-fork.c here as a special
	     case.  We don't want the follow-fork-mode or 'catch fork'
	     to interfere with this.  */

	  /* This won't actually modify the breakpoint list, but will
	     physically remove the breakpoints from the child.  */
	  detach_breakpoints (ptid_t (new_pid, new_pid));

	  /* Retain child fork in ptrace (stopped) state.  */
	  if (!find_fork_pid (new_pid))
	    add_fork (new_pid);

	  /* Report as spurious, so that infrun doesn't want to follow
	     this fork.  We're actually doing an infcall in
	     linux-fork.c.  */
	  ourstatus->set_spurious ();

	  /* Report the stop to the core.  */
	  return 0;
	}

      if (event == PTRACE_EVENT_FORK)
	ourstatus->set_forked (ptid_t (new_pid, new_pid));
      else if (event == PTRACE_EVENT_VFORK)
	ourstatus->set_vforked (ptid_t (new_pid, new_pid));
      else if (event == PTRACE_EVENT_CLONE)
	{
	  linux_nat_debug_printf
	    ("Got clone event from LWP %d, new child is LWP %ld", pid, new_pid);

	  /* Save the status again, we'll use it in follow_clone.  */
	  add_to_pid_list (&stopped_pids, new_pid, status);

	  ourstatus->set_thread_cloned (ptid_t (lp->ptid.pid (), new_pid));
	}

      return 0;
    }

  if (event == PTRACE_EVENT_EXEC)
    {
      linux_nat_debug_printf ("Got exec event from LWP %ld", lp->ptid.lwp ());

      /* Close the previous /proc/PID/mem file for this inferior,
	 which was using the address space which is now gone.
	 Reading/writing from this file would return 0/EOF.  */
      close_proc_mem_file (lp->ptid.pid ());

      /* Open a new file for the new address space.  */
      open_proc_mem_file (lp->ptid);

      ourstatus->set_execd
	(make_unique_xstrdup (linux_proc_pid_to_exec_file (pid)));

      /* The thread that execed must have been resumed, but, when a
	 thread execs, it changes its tid to the tgid, and the old
	 tgid thread might have not been resumed.  */
      lp->resumed = 1;

      /* All other LWPs are gone now.  We'll have received a thread
	 exit notification for all threads other the execing one.
	 That one, if it wasn't the leader, just silently changes its
	 tid to the tgid, and the previous leader vanishes.  Since
	 Linux 3.0, the former thread ID can be retrieved with
	 PTRACE_GETEVENTMSG, but since we support older kernels, don't
	 bother with it, and just walk the LWP list.  Even with
	 PTRACE_GETEVENTMSG, we'd still need to lookup the
	 corresponding LWP object, and it would be an extra ptrace
	 syscall, so this way may even be more efficient.  */
      for (lwp_info *other_lp : all_lwps_safe ())
	if (other_lp != lp && other_lp->ptid.pid () == lp->ptid.pid ())
	  exit_lwp (other_lp);

      return 0;
    }

  if (event == PTRACE_EVENT_VFORK_DONE)
    {
      linux_nat_debug_printf
	("Got PTRACE_EVENT_VFORK_DONE from LWP %ld",
	 lp->ptid.lwp ());
	ourstatus->set_vfork_done ();
	return 0;
    }

  internal_error (_("unknown ptrace event %d"), event);
}

/* Suspend waiting for a signal.  We're mostly interested in
   SIGCHLD/SIGINT.  */

static void
wait_for_signal ()
{
  linux_nat_debug_printf ("about to sigsuspend");
  sigsuspend (&suspend_mask);

  /* If the quit flag is set, it means that the user pressed Ctrl-C
     and we're debugging a process that is running on a separate
     terminal, so we must forward the Ctrl-C to the inferior.  (If the
     inferior is sharing GDB's terminal, then the Ctrl-C reaches the
     inferior directly.)  We must do this here because functions that
     need to block waiting for a signal loop forever until there's an
     event to report before returning back to the event loop.  */
  if (!target_terminal::is_ours ())
    {
      if (check_quit_flag ())
	target_pass_ctrlc ();
    }
}

/* Wait for LP to stop.  Returns the wait status, or 0 if the LWP has
   exited.  */

static int
wait_lwp (struct lwp_info *lp)
{
  pid_t pid;
  int status = 0;
  int thread_dead = 0;
  sigset_t prev_mask;

  gdb_assert (!lp->stopped);
  gdb_assert (lp->status == 0);

  /* Make sure SIGCHLD is blocked for sigsuspend avoiding a race below.  */
  block_child_signals (&prev_mask);

  for (;;)
    {
      pid = my_waitpid (lp->ptid.lwp (), &status, __WALL | WNOHANG);
      if (pid == -1 && errno == ECHILD)
	{
	  /* The thread has previously exited.  We need to delete it
	     now because if this was a non-leader thread execing, we
	     won't get an exit event.  See comments on exec events at
	     the top of the file.  */
	  thread_dead = 1;
	  linux_nat_debug_printf ("%s vanished.",
				  lp->ptid.to_string ().c_str ());
	}
      if (pid != 0)
	break;

      /* Bugs 10970, 12702.
	 Thread group leader may have exited in which case we'll lock up in
	 waitpid if there are other threads, even if they are all zombies too.
	 Basically, we're not supposed to use waitpid this way.
	  tkill(pid,0) cannot be used here as it gets ESRCH for both
	 for zombie and running processes.

	 As a workaround, check if we're waiting for the thread group leader and
	 if it's a zombie, and avoid calling waitpid if it is.

	 This is racy, what if the tgl becomes a zombie right after we check?
	 Therefore always use WNOHANG with sigsuspend - it is equivalent to
	 waiting waitpid but linux_proc_pid_is_zombie is safe this way.  */

      if (lp->ptid.pid () == lp->ptid.lwp ()
	  && linux_proc_pid_is_zombie (lp->ptid.lwp ()))
	{
	  thread_dead = 1;
	  linux_nat_debug_printf ("Thread group leader %s vanished.",
				  lp->ptid.to_string ().c_str ());
	  break;
	}

      /* Wait for next SIGCHLD and try again.  This may let SIGCHLD handlers
	 get invoked despite our caller had them intentionally blocked by
	 block_child_signals.  This is sensitive only to the loop of
	 linux_nat_wait_1 and there if we get called my_waitpid gets called
	 again before it gets to sigsuspend so we can safely let the handlers
	 get executed here.  */
      wait_for_signal ();
    }

  restore_child_signals_mask (&prev_mask);

  if (!thread_dead)
    {
      gdb_assert (pid == lp->ptid.lwp ());

      linux_nat_debug_printf ("waitpid %s received %s",
			      lp->ptid.to_string ().c_str (),
			      status_to_str (status).c_str ());

      /* Check if the thread has exited.  */
      if (WIFEXITED (status) || WIFSIGNALED (status))
	{
	  if (report_exit_events_for (lp) || is_leader (lp))
	    {
	      linux_nat_debug_printf ("LWP %d exited.", lp->ptid.pid ());

	      /* If this is the leader exiting, it means the whole
		 process is gone.  Store the status to report to the
		 core.  Store it in lp->waitstatus, because lp->status
		 would be ambiguous (W_EXITCODE(0,0) == 0).  */
	      lp->waitstatus = host_status_to_waitstatus (status);
	      return 0;
	    }

	  thread_dead = 1;
	  linux_nat_debug_printf ("%s exited.",
				  lp->ptid.to_string ().c_str ());
	}
    }

  if (thread_dead)
    {
      exit_lwp (lp);
      return 0;
    }

  gdb_assert (WIFSTOPPED (status));
  lp->stopped = 1;

  if (lp->must_set_ptrace_flags)
    {
      inferior *inf = find_inferior_pid (linux_target, lp->ptid.pid ());
      int options = linux_nat_ptrace_options (inf->attach_flag);

      linux_enable_event_reporting (lp->ptid.lwp (), options);
      lp->must_set_ptrace_flags = 0;
    }

  /* Handle GNU/Linux's syscall SIGTRAPs.  */
  if (WIFSTOPPED (status) && WSTOPSIG (status) == SYSCALL_SIGTRAP)
    {
      /* No longer need the sysgood bit.  The ptrace event ends up
	 recorded in lp->waitstatus if we care for it.  We can carry
	 on handling the event like a regular SIGTRAP from here
	 on.  */
      status = W_STOPCODE (SIGTRAP);
      if (linux_handle_syscall_trap (lp, 1))
	return wait_lwp (lp);
    }
  else
    {
      /* Almost all other ptrace-stops are known to be outside of system
	 calls, with further exceptions in linux_handle_extended_wait.  */
      lp->syscall_state = TARGET_WAITKIND_IGNORE;
    }

  /* Handle GNU/Linux's extended waitstatus for trace events.  */
  if (WIFSTOPPED (status) && WSTOPSIG (status) == SIGTRAP
      && linux_is_extended_waitstatus (status))
    {
      linux_nat_debug_printf ("Handling extended status 0x%06x", status);
      linux_handle_extended_wait (lp, status);
      return 0;
    }

  return status;
}

/* Send a SIGSTOP to LP.  */

static int
stop_callback (struct lwp_info *lp)
{
  if (!lp->stopped && !lp->signalled)
    {
      int ret;

      linux_nat_debug_printf ("kill %s **<SIGSTOP>**",
			      lp->ptid.to_string ().c_str ());

      errno = 0;
      ret = kill_lwp (lp->ptid.lwp (), SIGSTOP);
      linux_nat_debug_printf ("lwp kill %d %s", ret,
			      errno ? safe_strerror (errno) : "ERRNO-OK");

      lp->signalled = 1;
      gdb_assert (lp->status == 0);
    }

  return 0;
}

/* Request a stop on LWP.  */

void
linux_stop_lwp (struct lwp_info *lwp)
{
  stop_callback (lwp);
}

/* See linux-nat.h  */

void
linux_stop_and_wait_all_lwps (void)
{
  /* Stop all LWP's ...  */
  iterate_over_lwps (minus_one_ptid, stop_callback);

  /* ... and wait until all of them have reported back that
     they're no longer running.  */
  iterate_over_lwps (minus_one_ptid, stop_wait_callback);
}

/* See linux-nat.h  */

void
linux_unstop_all_lwps (void)
{
  iterate_over_lwps (minus_one_ptid,
		     [] (struct lwp_info *info)
		     {
		       return resume_stopped_resumed_lwps (info, minus_one_ptid);
		     });
}

/* Return non-zero if LWP PID has a pending SIGINT.  */

static int
linux_nat_has_pending_sigint (int pid)
{
  sigset_t pending, blocked, ignored;

  linux_proc_pending_signals (pid, &pending, &blocked, &ignored);

  if (sigismember (&pending, SIGINT)
      && !sigismember (&ignored, SIGINT))
    return 1;

  return 0;
}

/* Set a flag in LP indicating that we should ignore its next SIGINT.  */

static int
set_ignore_sigint (struct lwp_info *lp)
{
  /* If a thread has a pending SIGINT, consume it; otherwise, set a
     flag to consume the next one.  */
  if (lp->stopped && lp->status != 0 && WIFSTOPPED (lp->status)
      && WSTOPSIG (lp->status) == SIGINT)
    lp->status = 0;
  else
    lp->ignore_sigint = 1;

  return 0;
}

/* If LP does not have a SIGINT pending, then clear the ignore_sigint flag.
   This function is called after we know the LWP has stopped; if the LWP
   stopped before the expected SIGINT was delivered, then it will never have
   arrived.  Also, if the signal was delivered to a shared queue and consumed
   by a different thread, it will never be delivered to this LWP.  */

static void
maybe_clear_ignore_sigint (struct lwp_info *lp)
{
  if (!lp->ignore_sigint)
    return;

  if (!linux_nat_has_pending_sigint (lp->ptid.lwp ()))
    {
      linux_nat_debug_printf ("Clearing bogus flag for %s",
			      lp->ptid.to_string ().c_str ());
      lp->ignore_sigint = 0;
    }
}

/* Fetch the possible triggered data watchpoint info and store it in
   LP.

   On some archs, like x86, that use debug registers to set
   watchpoints, it's possible that the way to know which watched
   address trapped, is to check the register that is used to select
   which address to watch.  Problem is, between setting the watchpoint
   and reading back which data address trapped, the user may change
   the set of watchpoints, and, as a consequence, GDB changes the
   debug registers in the inferior.  To avoid reading back a stale
   stopped-data-address when that happens, we cache in LP the fact
   that a watchpoint trapped, and the corresponding data address, as
   soon as we see LP stop with a SIGTRAP.  If GDB changes the debug
   registers meanwhile, we have the cached data we can rely on.  */

static int
check_stopped_by_watchpoint (struct lwp_info *lp)
{
  scoped_restore save_inferior_ptid = make_scoped_restore (&inferior_ptid);
  inferior_ptid = lp->ptid;

  if (linux_target->low_stopped_by_watchpoint ())
    {
      lp->stop_reason = TARGET_STOPPED_BY_WATCHPOINT;
      lp->stopped_data_address_p
	= linux_target->low_stopped_data_address (&lp->stopped_data_address);
    }

  return lp->stop_reason == TARGET_STOPPED_BY_WATCHPOINT;
}

/* Returns true if the LWP had stopped for a watchpoint.  */

bool
linux_nat_target::stopped_by_watchpoint ()
{
  struct lwp_info *lp = find_lwp_pid (inferior_ptid);

  gdb_assert (lp != NULL);

  return lp->stop_reason == TARGET_STOPPED_BY_WATCHPOINT;
}

bool
linux_nat_target::stopped_data_address (CORE_ADDR *addr_p)
{
  struct lwp_info *lp = find_lwp_pid (inferior_ptid);

  gdb_assert (lp != NULL);

  *addr_p = lp->stopped_data_address;

  return lp->stopped_data_address_p;
}

/* Commonly any breakpoint / watchpoint generate only SIGTRAP.  */

bool
linux_nat_target::low_status_is_event (int status)
{
  return WIFSTOPPED (status) && WSTOPSIG (status) == SIGTRAP;
}

/* Wait until LP is stopped.  */

static int
stop_wait_callback (struct lwp_info *lp)
{
  inferior *inf = find_inferior_ptid (linux_target, lp->ptid);

  /* If this is a vfork parent, bail out, it is not going to report
     any SIGSTOP until the vfork is done with.  */
  if (inf->vfork_child != NULL)
    return 0;

  if (!lp->stopped)
    {
      int status;

      status = wait_lwp (lp);
      if (status == 0)
	return 0;

      if (lp->ignore_sigint && WIFSTOPPED (status)
	  && WSTOPSIG (status) == SIGINT)
	{
	  lp->ignore_sigint = 0;

	  errno = 0;
	  ptrace (PTRACE_CONT, lp->ptid.lwp (), 0, 0);
	  lp->stopped = 0;
	  linux_nat_debug_printf
	    ("PTRACE_CONT %s, 0, 0 (%s) (discarding SIGINT)",
	     lp->ptid.to_string ().c_str (),
	     errno ? safe_strerror (errno) : "OK");

	  return stop_wait_callback (lp);
	}

      maybe_clear_ignore_sigint (lp);

      if (WSTOPSIG (status) != SIGSTOP)
	{
	  /* The thread was stopped with a signal other than SIGSTOP.  */

	  linux_nat_debug_printf ("Pending event %s in %s",
				  status_to_str ((int) status).c_str (),
				  lp->ptid.to_string ().c_str ());

	  /* Save the sigtrap event.  */
	  lp->status = status;
	  gdb_assert (lp->signalled);
	  save_stop_reason (lp);
	}
      else
	{
	  /* We caught the SIGSTOP that we intended to catch.  */

	  linux_nat_debug_printf ("Expected SIGSTOP caught for %s.",
				  lp->ptid.to_string ().c_str ());

	  lp->signalled = 0;

	  /* If we are waiting for this stop so we can report the thread
	     stopped then we need to record this status.  Otherwise, we can
	     now discard this stop event.  */
	  if (lp->last_resume_kind == resume_stop)
	    {
	      lp->status = status;
	      save_stop_reason (lp);
	    }
	}
    }

  return 0;
}

/* Get the inferior associated to LWP.  Must be called with an LWP that has
   an associated inferior.  Always return non-nullptr.  */

static inferior *
lwp_inferior (const lwp_info *lwp)
{
  inferior *inf = find_inferior_ptid (linux_target, lwp->ptid);
  gdb_assert (inf != nullptr);
  return inf;
}

/* Return non-zero if LP has a wait status pending.  Discard the
   pending event and resume the LWP if the event that originally
   caused the stop became uninteresting.  */

static int
status_callback (struct lwp_info *lp)
{
  /* Only report a pending wait status if we pretend that this has
     indeed been resumed.  */
  if (!lp->resumed)
    return 0;

  if (!lwp_status_pending_p (lp))
    return 0;

  if (lp->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT
      || lp->stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT)
    {
      struct regcache *regcache = get_thread_regcache (linux_target, lp->ptid);
      CORE_ADDR pc;
      int discard = 0;

      pc = regcache_read_pc (regcache);

      if (pc != lp->stop_pc)
	{
	  linux_nat_debug_printf ("PC of %s changed.  was=%s, now=%s",
				  lp->ptid.to_string ().c_str (),
				  paddress (current_inferior ()->arch (),
					    lp->stop_pc),
				  paddress (current_inferior ()->arch (), pc));
	  discard = 1;
	}

#if !USE_SIGTRAP_SIGINFO
      else if (!breakpoint_inserted_here_p (lwp_inferior (lp)->aspace, pc))
	{
	  linux_nat_debug_printf ("previous breakpoint of %s, at %s gone",
				  lp->ptid.to_string ().c_str (),
				  paddress (current_inferior ()->arch (),
					    lp->stop_pc));

	  discard = 1;
	}
#endif

      if (discard)
	{
	  linux_nat_debug_printf ("pending event of %s cancelled.",
				  lp->ptid.to_string ().c_str ());

	  lp->status = 0;
	  linux_resume_one_lwp (lp, lp->step, GDB_SIGNAL_0);
	  return 0;
	}
    }

  return 1;
}

/* Count the LWP's that have had events.  */

static int
count_events_callback (struct lwp_info *lp, int *count)
{
  gdb_assert (count != NULL);

  /* Select only resumed LWPs that have an event pending.  */
  if (lp->resumed && lwp_status_pending_p (lp))
    (*count)++;

  return 0;
}

/* Select the LWP (if any) that is currently being single-stepped.  */

static int
select_singlestep_lwp_callback (struct lwp_info *lp)
{
  if (lp->last_resume_kind == resume_step
      && lp->status != 0)
    return 1;
  else
    return 0;
}

/* Returns true if LP has a status pending.  */

static int
lwp_status_pending_p (struct lwp_info *lp)
{
  /* We check for lp->waitstatus in addition to lp->status, because we
     can have pending process exits recorded in lp->status and
     W_EXITCODE(0,0) happens to be 0.  */
  return lp->status != 0 || lp->waitstatus.kind () != TARGET_WAITKIND_IGNORE;
}

/* Select the Nth LWP that has had an event.  */

static int
select_event_lwp_callback (struct lwp_info *lp, int *selector)
{
  gdb_assert (selector != NULL);

  /* Select only resumed LWPs that have an event pending.  */
  if (lp->resumed && lwp_status_pending_p (lp))
    if ((*selector)-- == 0)
      return 1;

  return 0;
}

/* Called when the LWP stopped for a signal/trap.  If it stopped for a
   trap check what caused it (breakpoint, watchpoint, trace, etc.),
   and save the result in the LWP's stop_reason field.  If it stopped
   for a breakpoint, decrement the PC if necessary on the lwp's
   architecture.  */

static void
save_stop_reason (struct lwp_info *lp)
{
  struct regcache *regcache;
  struct gdbarch *gdbarch;
  CORE_ADDR pc;
  CORE_ADDR sw_bp_pc;
#if USE_SIGTRAP_SIGINFO
  siginfo_t siginfo;
#endif

  gdb_assert (lp->stop_reason == TARGET_STOPPED_BY_NO_REASON);
  gdb_assert (lp->status != 0);

  if (!linux_target->low_status_is_event (lp->status))
    return;

  inferior *inf = lwp_inferior (lp);
  if (inf->starting_up)
    return;

  regcache = get_thread_regcache (linux_target, lp->ptid);
  gdbarch = regcache->arch ();

  pc = regcache_read_pc (regcache);
  sw_bp_pc = pc - gdbarch_decr_pc_after_break (gdbarch);

#if USE_SIGTRAP_SIGINFO
  if (linux_nat_get_siginfo (lp->ptid, &siginfo))
    {
      if (siginfo.si_signo == SIGTRAP)
	{
	  if (GDB_ARCH_IS_TRAP_BRKPT (siginfo.si_code)
	      && GDB_ARCH_IS_TRAP_HWBKPT (siginfo.si_code))
	    {
	      /* The si_code is ambiguous on this arch -- check debug
		 registers.  */
	      if (!check_stopped_by_watchpoint (lp))
		lp->stop_reason = TARGET_STOPPED_BY_SW_BREAKPOINT;
	    }
	  else if (GDB_ARCH_IS_TRAP_BRKPT (siginfo.si_code))
	    {
	      /* If we determine the LWP stopped for a SW breakpoint,
		 trust it.  Particularly don't check watchpoint
		 registers, because, at least on s390, we'd find
		 stopped-by-watchpoint as long as there's a watchpoint
		 set.  */
	      lp->stop_reason = TARGET_STOPPED_BY_SW_BREAKPOINT;
	    }
	  else if (GDB_ARCH_IS_TRAP_HWBKPT (siginfo.si_code))
	    {
	      /* This can indicate either a hardware breakpoint or
		 hardware watchpoint.  Check debug registers.  */
	      if (!check_stopped_by_watchpoint (lp))
		lp->stop_reason = TARGET_STOPPED_BY_HW_BREAKPOINT;
	    }
	  else if (siginfo.si_code == TRAP_TRACE)
	    {
	      linux_nat_debug_printf ("%s stopped by trace",
				      lp->ptid.to_string ().c_str ());

	      /* We may have single stepped an instruction that
		 triggered a watchpoint.  In that case, on some
		 architectures (such as x86), instead of TRAP_HWBKPT,
		 si_code indicates TRAP_TRACE, and we need to check
		 the debug registers separately.  */
	      check_stopped_by_watchpoint (lp);
	    }
	}
    }
#else
  if ((!lp->step || lp->stop_pc == sw_bp_pc)
      && software_breakpoint_inserted_here_p (inf->aspace, sw_bp_pc))
    {
      /* The LWP was either continued, or stepped a software
	 breakpoint instruction.  */
      lp->stop_reason = TARGET_STOPPED_BY_SW_BREAKPOINT;
    }

  if (hardware_breakpoint_inserted_here_p (inf->aspace, pc))
    lp->stop_reason = TARGET_STOPPED_BY_HW_BREAKPOINT;

  if (lp->stop_reason == TARGET_STOPPED_BY_NO_REASON)
    check_stopped_by_watchpoint (lp);
#endif

  if (lp->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT)
    {
      linux_nat_debug_printf ("%s stopped by software breakpoint",
			      lp->ptid.to_string ().c_str ());

      /* Back up the PC if necessary.  */
      if (pc != sw_bp_pc)
	regcache_write_pc (regcache, sw_bp_pc);

      /* Update this so we record the correct stop PC below.  */
      pc = sw_bp_pc;
    }
  else if (lp->stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT)
    {
      linux_nat_debug_printf ("%s stopped by hardware breakpoint",
			      lp->ptid.to_string ().c_str ());
    }
  else if (lp->stop_reason == TARGET_STOPPED_BY_WATCHPOINT)
    {
      linux_nat_debug_printf ("%s stopped by hardware watchpoint",
			      lp->ptid.to_string ().c_str ());
    }

  lp->stop_pc = pc;
}


/* Returns true if the LWP had stopped for a software breakpoint.  */

bool
linux_nat_target::stopped_by_sw_breakpoint ()
{
  struct lwp_info *lp = find_lwp_pid (inferior_ptid);

  gdb_assert (lp != NULL);

  return lp->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT;
}

/* Implement the supports_stopped_by_sw_breakpoint method.  */

bool
linux_nat_target::supports_stopped_by_sw_breakpoint ()
{
  return USE_SIGTRAP_SIGINFO;
}

/* Returns true if the LWP had stopped for a hardware
   breakpoint/watchpoint.  */

bool
linux_nat_target::stopped_by_hw_breakpoint ()
{
  struct lwp_info *lp = find_lwp_pid (inferior_ptid);

  gdb_assert (lp != NULL);

  return lp->stop_reason == TARGET_STOPPED_BY_HW_BREAKPOINT;
}

/* Implement the supports_stopped_by_hw_breakpoint method.  */

bool
linux_nat_target::supports_stopped_by_hw_breakpoint ()
{
  return USE_SIGTRAP_SIGINFO;
}

/* Select one LWP out of those that have events pending.  */

static void
select_event_lwp (ptid_t filter, struct lwp_info **orig_lp, int *status)
{
  int num_events = 0;
  int random_selector;
  struct lwp_info *event_lp = NULL;

  /* Record the wait status for the original LWP.  */
  (*orig_lp)->status = *status;

  /* In all-stop, give preference to the LWP that is being
     single-stepped.  There will be at most one, and it will be the
     LWP that the core is most interested in.  If we didn't do this,
     then we'd have to handle pending step SIGTRAPs somehow in case
     the core later continues the previously-stepped thread, as
     otherwise we'd report the pending SIGTRAP then, and the core, not
     having stepped the thread, wouldn't understand what the trap was
     for, and therefore would report it to the user as a random
     signal.  */
  if (!target_is_non_stop_p ())
    {
      event_lp = iterate_over_lwps (filter, select_singlestep_lwp_callback);
      if (event_lp != NULL)
	{
	  linux_nat_debug_printf ("Select single-step %s",
				  event_lp->ptid.to_string ().c_str ());
	}
    }

  if (event_lp == NULL)
    {
      /* Pick one at random, out of those which have had events.  */

      /* First see how many events we have.  */
      iterate_over_lwps (filter,
			 [&] (struct lwp_info *info)
			 {
			   return count_events_callback (info, &num_events);
			 });
      gdb_assert (num_events > 0);

      /* Now randomly pick a LWP out of those that have had
	 events.  */
      random_selector = (int)
	((num_events * (double) rand ()) / (RAND_MAX + 1.0));

      if (num_events > 1)
	linux_nat_debug_printf ("Found %d events, selecting #%d",
				num_events, random_selector);

      event_lp
	= (iterate_over_lwps
	   (filter,
	    [&] (struct lwp_info *info)
	    {
	      return select_event_lwp_callback (info,
						&random_selector);
	    }));
    }

  if (event_lp != NULL)
    {
      /* Switch the event LWP.  */
      *orig_lp = event_lp;
      *status = event_lp->status;
    }

  /* Flush the wait status for the event LWP.  */
  (*orig_lp)->status = 0;
}

/* Return non-zero if LP has been resumed.  */

static int
resumed_callback (struct lwp_info *lp)
{
  return lp->resumed;
}

/* Check if we should go on and pass this event to common code.

   If so, save the status to the lwp_info structure associated to LWPID.  */

static void
linux_nat_filter_event (int lwpid, int status)
{
  struct lwp_info *lp;
  int event = linux_ptrace_get_extended_event (status);

  lp = find_lwp_pid (ptid_t (lwpid));

  /* Check for events reported by anything not in our LWP list.  */
  if (lp == nullptr)
    {
      if (WIFSTOPPED (status))
	{
	  if (WSTOPSIG (status) == SIGTRAP && event == PTRACE_EVENT_EXEC)
	    {
	      /* A non-leader thread exec'ed after we've seen the
		 leader zombie, and removed it from our lists (in
		 check_zombie_leaders).  The non-leader thread changes
		 its tid to the tgid.  */
	      linux_nat_debug_printf
		("Re-adding thread group leader LWP %d after exec.",
		 lwpid);

	      lp = add_lwp (ptid_t (lwpid, lwpid));
	      lp->stopped = 1;
	      lp->resumed = 1;
	      add_thread (linux_target, lp->ptid);
	    }
	  else
	    {
	      /* A process we are controlling has forked and the new
		 child's stop was reported to us by the kernel.  Save
		 its PID and go back to waiting for the fork event to
		 be reported - the stopped process might be returned
		 from waitpid before or after the fork event is.  */
	      linux_nat_debug_printf
		("Saving LWP %d status %s in stopped_pids list",
		 lwpid, status_to_str (status).c_str ());
	      add_to_pid_list (&stopped_pids, lwpid, status);
	    }
	}
      else
	{
	  /* Don't report an event for the exit of an LWP not in our
	     list, i.e. not part of any inferior we're debugging.
	     This can happen if we detach from a program we originally
	     forked and then it exits.  However, note that we may have
	     earlier deleted a leader of an inferior we're debugging,
	     in check_zombie_leaders.  Re-add it back here if so.  */
	  for (inferior *inf : all_inferiors (linux_target))
	    {
	      if (inf->pid == lwpid)
		{
		  linux_nat_debug_printf
		    ("Re-adding thread group leader LWP %d after exit.",
		     lwpid);

		  lp = add_lwp (ptid_t (lwpid, lwpid));
		  lp->resumed = 1;
		  add_thread (linux_target, lp->ptid);
		  break;
		}
	    }
	}

      if (lp == nullptr)
	return;
    }

  /* This LWP is stopped now.  (And if dead, this prevents it from
     ever being continued.)  */
  lp->stopped = 1;

  if (WIFSTOPPED (status) && lp->must_set_ptrace_flags)
    {
      inferior *inf = find_inferior_pid (linux_target, lp->ptid.pid ());
      int options = linux_nat_ptrace_options (inf->attach_flag);

      linux_enable_event_reporting (lp->ptid.lwp (), options);
      lp->must_set_ptrace_flags = 0;
    }

  /* Handle GNU/Linux's syscall SIGTRAPs.  */
  if (WIFSTOPPED (status) && WSTOPSIG (status) == SYSCALL_SIGTRAP)
    {
      /* No longer need the sysgood bit.  The ptrace event ends up
	 recorded in lp->waitstatus if we care for it.  We can carry
	 on handling the event like a regular SIGTRAP from here
	 on.  */
      status = W_STOPCODE (SIGTRAP);
      if (linux_handle_syscall_trap (lp, 0))
	return;
    }
  else
    {
      /* Almost all other ptrace-stops are known to be outside of system
	 calls, with further exceptions in linux_handle_extended_wait.  */
      lp->syscall_state = TARGET_WAITKIND_IGNORE;
    }

  /* Handle GNU/Linux's extended waitstatus for trace events.  */
  if (WIFSTOPPED (status) && WSTOPSIG (status) == SIGTRAP
      && linux_is_extended_waitstatus (status))
    {
      linux_nat_debug_printf ("Handling extended status 0x%06x", status);

      if (linux_handle_extended_wait (lp, status))
	return;
    }

  /* Check if the thread has exited.  */
  if (WIFEXITED (status) || WIFSIGNALED (status))
    {
      if (!report_exit_events_for (lp) && !is_leader (lp))
	{
	  linux_nat_debug_printf ("%s exited.",
				  lp->ptid.to_string ().c_str ());

	  /* If this was not the leader exiting, then the exit signal
	     was not the end of the debugged application and should be
	     ignored.  */
	  exit_lwp (lp);
	  return;
	}

      /* Note that even if the leader was ptrace-stopped, it can still
	 exit, if e.g., some other thread brings down the whole
	 process (calls `exit').  So don't assert that the lwp is
	 resumed.  */
      linux_nat_debug_printf ("LWP %ld exited (resumed=%d)",
			      lp->ptid.lwp (), lp->resumed);

      /* Dead LWP's aren't expected to reported a pending sigstop.  */
      lp->signalled = 0;

      /* Store the pending event in the waitstatus, because
	 W_EXITCODE(0,0) == 0.  */
      lp->waitstatus = host_status_to_waitstatus (status);
      return;
    }

  /* Make sure we don't report a SIGSTOP that we sent ourselves in
     an attempt to stop an LWP.  */
  if (lp->signalled
      && WIFSTOPPED (status) && WSTOPSIG (status) == SIGSTOP)
    {
      lp->signalled = 0;

      if (lp->last_resume_kind == resume_stop)
	{
	  linux_nat_debug_printf ("resume_stop SIGSTOP caught for %s.",
				  lp->ptid.to_string ().c_str ());
	}
      else
	{
	  /* This is a delayed SIGSTOP.  Filter out the event.  */

	  linux_nat_debug_printf
	    ("%s %s, 0, 0 (discard delayed SIGSTOP)",
	     lp->step ? "PTRACE_SINGLESTEP" : "PTRACE_CONT",
	     lp->ptid.to_string ().c_str ());

	  linux_resume_one_lwp (lp, lp->step, GDB_SIGNAL_0);
	  gdb_assert (lp->resumed);
	  return;
	}
    }

  /* Make sure we don't report a SIGINT that we have already displayed
     for another thread.  */
  if (lp->ignore_sigint
      && WIFSTOPPED (status) && WSTOPSIG (status) == SIGINT)
    {
      linux_nat_debug_printf ("Delayed SIGINT caught for %s.",
			      lp->ptid.to_string ().c_str ());

      /* This is a delayed SIGINT.  */
      lp->ignore_sigint = 0;

      linux_resume_one_lwp (lp, lp->step, GDB_SIGNAL_0);
      linux_nat_debug_printf ("%s %s, 0, 0 (discard SIGINT)",
			      lp->step ? "PTRACE_SINGLESTEP" : "PTRACE_CONT",
			      lp->ptid.to_string ().c_str ());
      gdb_assert (lp->resumed);

      /* Discard the event.  */
      return;
    }

  /* Don't report signals that GDB isn't interested in, such as
     signals that are neither printed nor stopped upon.  Stopping all
     threads can be a bit time-consuming, so if we want decent
     performance with heavily multi-threaded programs, especially when
     they're using a high frequency timer, we'd better avoid it if we
     can.  */
  if (WIFSTOPPED (status))
    {
      enum gdb_signal signo = gdb_signal_from_host (WSTOPSIG (status));

      if (!target_is_non_stop_p ())
	{
	  /* Only do the below in all-stop, as we currently use SIGSTOP
	     to implement target_stop (see linux_nat_stop) in
	     non-stop.  */
	  if (signo == GDB_SIGNAL_INT && signal_pass_state (signo) == 0)
	    {
	      /* If ^C/BREAK is typed at the tty/console, SIGINT gets
		 forwarded to the entire process group, that is, all LWPs
		 will receive it - unless they're using CLONE_THREAD to
		 share signals.  Since we only want to report it once, we
		 mark it as ignored for all LWPs except this one.  */
	      iterate_over_lwps (ptid_t (lp->ptid.pid ()), set_ignore_sigint);
	      lp->ignore_sigint = 0;
	    }
	  else
	    maybe_clear_ignore_sigint (lp);
	}

      /* When using hardware single-step, we need to report every signal.
	 Otherwise, signals in pass_mask may be short-circuited
	 except signals that might be caused by a breakpoint, or SIGSTOP
	 if we sent the SIGSTOP and are waiting for it to arrive.  */
      if (!lp->step
	  && WSTOPSIG (status) && sigismember (&pass_mask, WSTOPSIG (status))
	  && (WSTOPSIG (status) != SIGSTOP
	      || !linux_target->find_thread (lp->ptid)->stop_requested)
	  && !linux_wstatus_maybe_breakpoint (status))
	{
	  linux_resume_one_lwp (lp, lp->step, signo);
	  linux_nat_debug_printf
	    ("%s %s, %s (preempt 'handle')",
	     lp->step ? "PTRACE_SINGLESTEP" : "PTRACE_CONT",
	     lp->ptid.to_string ().c_str (),
	     (signo != GDB_SIGNAL_0
	      ? strsignal (gdb_signal_to_host (signo)) : "0"));
	  return;
	}
    }

  /* An interesting event.  */
  gdb_assert (lp);
  lp->status = status;
  save_stop_reason (lp);
}

/* Detect zombie thread group leaders, and "exit" them.  We can't reap
   their exits until all other threads in the group have exited.  */

static void
check_zombie_leaders (void)
{
  for (inferior *inf : all_inferiors ())
    {
      struct lwp_info *leader_lp;

      if (inf->pid == 0)
	continue;

      leader_lp = find_lwp_pid (ptid_t (inf->pid));
      if (leader_lp != NULL
	  /* Check if there are other threads in the group, as we may
	     have raced with the inferior simply exiting.  Note this
	     isn't a watertight check.  If the inferior is
	     multi-threaded and is exiting, it may be we see the
	     leader as zombie before we reap all the non-leader
	     threads.  See comments below.  */
	  && num_lwps (inf->pid) > 1
	  && linux_proc_pid_is_zombie (inf->pid))
	{
	  /* A zombie leader in a multi-threaded program can mean one
	     of three things:

	     #1 - Only the leader exited, not the whole program, e.g.,
	     with pthread_exit.  Since we can't reap the leader's exit
	     status until all other threads are gone and reaped too,
	     we want to delete the zombie leader right away, as it
	     can't be debugged, we can't read its registers, etc.
	     This is the main reason we check for zombie leaders
	     disappearing.

	     #2 - The whole thread-group/process exited (a group exit,
	     via e.g. exit(3), and there is (or will be shortly) an
	     exit reported for each thread in the process, and then
	     finally an exit for the leader once the non-leaders are
	     reaped.

	     #3 - There are 3 or more threads in the group, and a
	     thread other than the leader exec'd.  See comments on
	     exec events at the top of the file.

	     Ideally we would never delete the leader for case #2.
	     Instead, we want to collect the exit status of each
	     non-leader thread, and then finally collect the exit
	     status of the leader as normal and use its exit code as
	     whole-process exit code.  Unfortunately, there's no
	     race-free way to distinguish cases #1 and #2.  We can't
	     assume the exit events for the non-leaders threads are
	     already pending in the kernel, nor can we assume the
	     non-leader threads are in zombie state already.  Between
	     the leader becoming zombie and the non-leaders exiting
	     and becoming zombie themselves, there's a small time
	     window, so such a check would be racy.  Temporarily
	     pausing all threads and checking to see if all threads
	     exit or not before re-resuming them would work in the
	     case that all threads are running right now, but it
	     wouldn't work if some thread is currently already
	     ptrace-stopped, e.g., due to scheduler-locking.

	     So what we do is we delete the leader anyhow, and then
	     later on when we see its exit status, we re-add it back.
	     We also make sure that we only report a whole-process
	     exit when we see the leader exiting, as opposed to when
	     the last LWP in the LWP list exits, which can be a
	     non-leader if we deleted the leader here.  */
	  linux_nat_debug_printf ("Thread group leader %d zombie "
				  "(it exited, or another thread execd), "
				  "deleting it.",
				  inf->pid);
	  exit_lwp (leader_lp);
	}
    }
}

/* Convenience function that is called when we're about to return an
   event to the core.  If the event is an exit or signalled event,
   then this decides whether to report it as process-wide event, as a
   thread exit event, or to suppress it.  All other event kinds are
   passed through unmodified.  */

static ptid_t
filter_exit_event (struct lwp_info *event_child,
		   struct target_waitstatus *ourstatus)
{
  ptid_t ptid = event_child->ptid;

  /* Note we must filter TARGET_WAITKIND_SIGNALLED as well, otherwise
     if a non-leader thread exits with a signal, we'd report it to the
     core which would interpret it as the whole-process exiting.
     There is no TARGET_WAITKIND_THREAD_SIGNALLED event kind.  */
  if (ourstatus->kind () != TARGET_WAITKIND_EXITED
      && ourstatus->kind () != TARGET_WAITKIND_SIGNALLED)
    return ptid;

  if (!is_leader (event_child))
    {
      if (report_exit_events_for (event_child))
	{
	  ourstatus->set_thread_exited (0);
	  /* Delete lwp, but not thread_info, infrun will need it to
	     process the event.  */
	  exit_lwp (event_child, false);
	}
      else
	{
	  ourstatus->set_ignore ();
	  exit_lwp (event_child);
	}
    }

  return ptid;
}

static ptid_t
linux_nat_wait_1 (ptid_t ptid, struct target_waitstatus *ourstatus,
		  target_wait_flags target_options)
{
  LINUX_NAT_SCOPED_DEBUG_ENTER_EXIT;

  sigset_t prev_mask;
  enum resume_kind last_resume_kind;
  struct lwp_info *lp;
  int status;

  /* The first time we get here after starting a new inferior, we may
     not have added it to the LWP list yet - this is the earliest
     moment at which we know its PID.  */
  if (ptid.is_pid () && find_lwp_pid (ptid) == nullptr)
    {
      ptid_t lwp_ptid (ptid.pid (), ptid.pid ());

      /* Upgrade the main thread's ptid.  */
      thread_change_ptid (linux_target, ptid, lwp_ptid);
      lp = add_initial_lwp (lwp_ptid);
      lp->resumed = 1;
    }

  /* Make sure SIGCHLD is blocked until the sigsuspend below.  */
  block_child_signals (&prev_mask);

  /* First check if there is a LWP with a wait status pending.  */
  lp = iterate_over_lwps (ptid, status_callback);
  if (lp != NULL)
    {
      linux_nat_debug_printf ("Using pending wait status %s for %s.",
			      pending_status_str (lp).c_str (),
			      lp->ptid.to_string ().c_str ());
    }

  /* But if we don't find a pending event, we'll have to wait.  Always
     pull all events out of the kernel.  We'll randomly select an
     event LWP out of all that have events, to prevent starvation.  */

  while (lp == NULL)
    {
      pid_t lwpid;

      /* Always use -1 and WNOHANG, due to couple of a kernel/ptrace
	 quirks:

	 - If the thread group leader exits while other threads in the
	   thread group still exist, waitpid(TGID, ...) hangs.  That
	   waitpid won't return an exit status until the other threads
	   in the group are reaped.

	 - When a non-leader thread execs, that thread just vanishes
	   without reporting an exit (so we'd hang if we waited for it
	   explicitly in that case).  The exec event is reported to
	   the TGID pid.  */

      errno = 0;
      lwpid = my_waitpid (-1, &status,  __WALL | WNOHANG);

      linux_nat_debug_printf ("waitpid(-1, ...) returned %d, %s",
			      lwpid,
			      errno ? safe_strerror (errno) : "ERRNO-OK");

      if (lwpid > 0)
	{
	  linux_nat_debug_printf ("waitpid %ld received %s",
				  (long) lwpid,
				  status_to_str (status).c_str ());

	  linux_nat_filter_event (lwpid, status);
	  /* Retry until nothing comes out of waitpid.  A single
	     SIGCHLD can indicate more than one child stopped.  */
	  continue;
	}

      /* Now that we've pulled all events out of the kernel, resume
	 LWPs that don't have an interesting event to report.  */
      iterate_over_lwps (minus_one_ptid,
			 [] (struct lwp_info *info)
			 {
			   return resume_stopped_resumed_lwps (info, minus_one_ptid);
			 });

      /* ... and find an LWP with a status to report to the core, if
	 any.  */
      lp = iterate_over_lwps (ptid, status_callback);
      if (lp != NULL)
	break;

      /* Check for zombie thread group leaders.  Those can't be reaped
	 until all other threads in the thread group are.  */
      check_zombie_leaders ();

      /* If there are no resumed children left, bail.  We'd be stuck
	 forever in the sigsuspend call below otherwise.  */
      if (iterate_over_lwps (ptid, resumed_callback) == NULL)
	{
	  linux_nat_debug_printf ("exit (no resumed LWP)");

	  ourstatus->set_no_resumed ();

	  restore_child_signals_mask (&prev_mask);
	  return minus_one_ptid;
	}

      /* No interesting event to report to the core.  */

      if (target_options & TARGET_WNOHANG)
	{
	  linux_nat_debug_printf ("no interesting events found");

	  ourstatus->set_ignore ();
	  restore_child_signals_mask (&prev_mask);
	  return minus_one_ptid;
	}

      /* We shouldn't end up here unless we want to try again.  */
      gdb_assert (lp == NULL);

      /* Block until we get an event reported with SIGCHLD.  */
      wait_for_signal ();
    }

  gdb_assert (lp);

  status = lp->status;
  lp->status = 0;

  if (!target_is_non_stop_p ())
    {
      /* Now stop all other LWP's ...  */
      iterate_over_lwps (minus_one_ptid, stop_callback);

      /* ... and wait until all of them have reported back that
	 they're no longer running.  */
      iterate_over_lwps (minus_one_ptid, stop_wait_callback);
    }

  /* If we're not waiting for a specific LWP, choose an event LWP from
     among those that have had events.  Giving equal priority to all
     LWPs that have had events helps prevent starvation.  */
  if (ptid == minus_one_ptid || ptid.is_pid ())
    select_event_lwp (ptid, &lp, &status);

  gdb_assert (lp != NULL);

  /* Now that we've selected our final event LWP, un-adjust its PC if
     it was a software breakpoint, and we can't reliably support the
     "stopped by software breakpoint" stop reason.  */
  if (lp->stop_reason == TARGET_STOPPED_BY_SW_BREAKPOINT
      && !USE_SIGTRAP_SIGINFO)
    {
      struct regcache *regcache = get_thread_regcache (linux_target, lp->ptid);
      struct gdbarch *gdbarch = regcache->arch ();
      int decr_pc = gdbarch_decr_pc_after_break (gdbarch);

      if (decr_pc != 0)
	{
	  CORE_ADDR pc;

	  pc = regcache_read_pc (regcache);
	  regcache_write_pc (regcache, pc + decr_pc);
	}
    }

  /* We'll need this to determine whether to report a SIGSTOP as
     GDB_SIGNAL_0.  Need to take a copy because resume_clear_callback
     clears it.  */
  last_resume_kind = lp->last_resume_kind;

  if (!target_is_non_stop_p ())
    {
      /* In all-stop, from the core's perspective, all LWPs are now
	 stopped until a new resume action is sent over.  */
      iterate_over_lwps (minus_one_ptid, resume_clear_callback);
    }
  else
    {
      resume_clear_callback (lp);
    }

  if (linux_target->low_status_is_event (status))
    {
      linux_nat_debug_printf ("trap ptid is %s.",
			      lp->ptid.to_string ().c_str ());
    }

  if (lp->waitstatus.kind () != TARGET_WAITKIND_IGNORE)
    {
      *ourstatus = lp->waitstatus;
      lp->waitstatus.set_ignore ();
    }
  else
    *ourstatus = host_status_to_waitstatus (status);

  linux_nat_debug_printf ("event found");

  restore_child_signals_mask (&prev_mask);

  if (last_resume_kind == resume_stop
      && ourstatus->kind () == TARGET_WAITKIND_STOPPED
      && WSTOPSIG (status) == SIGSTOP)
    {
      /* A thread that has been requested to stop by GDB with
	 target_stop, and it stopped cleanly, so report as SIG0.  The
	 use of SIGSTOP is an implementation detail.  */
      ourstatus->set_stopped (GDB_SIGNAL_0);
    }

  if (ourstatus->kind () == TARGET_WAITKIND_EXITED
      || ourstatus->kind () == TARGET_WAITKIND_SIGNALLED)
    lp->core = -1;
  else
    lp->core = linux_common_core_of_thread (lp->ptid);

  return filter_exit_event (lp, ourstatus);
}

/* Resume LWPs that are currently stopped without any pending status
   to report, but are resumed from the core's perspective.  */

static int
resume_stopped_resumed_lwps (struct lwp_info *lp, const ptid_t wait_ptid)
{
  inferior *inf = lwp_inferior (lp);

  if (!lp->stopped)
    {
      linux_nat_debug_printf ("NOT resuming LWP %s, not stopped",
			      lp->ptid.to_string ().c_str ());
    }
  else if (!lp->resumed)
    {
      linux_nat_debug_printf ("NOT resuming LWP %s, not resumed",
			      lp->ptid.to_string ().c_str ());
    }
  else if (lwp_status_pending_p (lp))
    {
      linux_nat_debug_printf ("NOT resuming LWP %s, has pending status",
			      lp->ptid.to_string ().c_str ());
    }
  else if (inf->vfork_child != nullptr)
    {
      linux_nat_debug_printf ("NOT resuming LWP %s (vfork parent)",
			      lp->ptid.to_string ().c_str ());
    }
  else
    {
      struct regcache *regcache = get_thread_regcache (linux_target, lp->ptid);
      struct gdbarch *gdbarch = regcache->arch ();

      try
	{
	  CORE_ADDR pc = regcache_read_pc (regcache);
	  int leave_stopped = 0;

	  /* Don't bother if there's a breakpoint at PC that we'd hit
	     immediately, and we're not waiting for this LWP.  */
	  if (!lp->ptid.matches (wait_ptid))
	    {
	      if (breakpoint_inserted_here_p (inf->aspace.get (), pc))
		leave_stopped = 1;
	    }

	  if (!leave_stopped)
	    {
	      linux_nat_debug_printf
		("resuming stopped-resumed LWP %s at %s: step=%d",
		 lp->ptid.to_string ().c_str (), paddress (gdbarch, pc),
		 lp->step);

	      linux_resume_one_lwp_throw (lp, lp->step, GDB_SIGNAL_0);
	    }
	}
      catch (const gdb_exception_error &ex)
	{
	  if (!check_ptrace_stopped_lwp_gone (lp))
	    throw;
	}
    }

  return 0;
}

ptid_t
linux_nat_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
			target_wait_flags target_options)
{
  LINUX_NAT_SCOPED_DEBUG_ENTER_EXIT;

  ptid_t event_ptid;

  linux_nat_debug_printf ("[%s], [%s]", ptid.to_string ().c_str (),
			  target_options_to_string (target_options).c_str ());

  /* Flush the async file first.  */
  if (target_is_async_p ())
    async_file_flush ();

  /* Resume LWPs that are currently stopped without any pending status
     to report, but are resumed from the core's perspective.  LWPs get
     in this state if we find them stopping at a time we're not
     interested in reporting the event (target_wait on a
     specific_process, for example, see linux_nat_wait_1), and
     meanwhile the event became uninteresting.  Don't bother resuming
     LWPs we're not going to wait for if they'd stop immediately.  */
  if (target_is_non_stop_p ())
    iterate_over_lwps (minus_one_ptid,
		       [=] (struct lwp_info *info)
		       {
			 return resume_stopped_resumed_lwps (info, ptid);
		       });

  event_ptid = linux_nat_wait_1 (ptid, ourstatus, target_options);

  /* If we requested any event, and something came out, assume there
     may be more.  If we requested a specific lwp or process, also
     assume there may be more.  */
  if (target_is_async_p ()
      && ((ourstatus->kind () != TARGET_WAITKIND_IGNORE
	   && ourstatus->kind () != TARGET_WAITKIND_NO_RESUMED)
	  || ptid != minus_one_ptid))
    async_file_mark ();

  return event_ptid;
}

/* Kill one LWP.  */

static void
kill_one_lwp (pid_t pid)
{
  /* PTRACE_KILL may resume the inferior.  Send SIGKILL first.  */

  errno = 0;
  kill_lwp (pid, SIGKILL);

  if (debug_linux_nat)
    {
      int save_errno = errno;

      linux_nat_debug_printf
	("kill (SIGKILL) %ld, 0, 0 (%s)", (long) pid,
	 save_errno != 0 ? safe_strerror (save_errno) : "OK");
    }

  /* Some kernels ignore even SIGKILL for processes under ptrace.  */

  errno = 0;
  ptrace (PTRACE_KILL, pid, 0, 0);
  if (debug_linux_nat)
    {
      int save_errno = errno;

      linux_nat_debug_printf
	("PTRACE_KILL %ld, 0, 0 (%s)", (long) pid,
	 save_errno ? safe_strerror (save_errno) : "OK");
    }
}

/* Wait for an LWP to die.  */

static void
kill_wait_one_lwp (pid_t pid)
{
  pid_t res;

  /* We must make sure that there are no pending events (delayed
     SIGSTOPs, pending SIGTRAPs, etc.) to make sure the current
     program doesn't interfere with any following debugging session.  */

  do
    {
      res = my_waitpid (pid, NULL, __WALL);
      if (res != (pid_t) -1)
	{
	  linux_nat_debug_printf ("wait %ld received unknown.", (long) pid);

	  /* The Linux kernel sometimes fails to kill a thread
	     completely after PTRACE_KILL; that goes from the stop
	     point in do_fork out to the one in get_signal_to_deliver
	     and waits again.  So kill it again.  */
	  kill_one_lwp (pid);
	}
    }
  while (res == pid);

  gdb_assert (res == -1 && errno == ECHILD);
}

/* Callback for iterate_over_lwps.  */

static int
kill_callback (struct lwp_info *lp)
{
  kill_one_lwp (lp->ptid.lwp ());
  return 0;
}

/* Callback for iterate_over_lwps.  */

static int
kill_wait_callback (struct lwp_info *lp)
{
  kill_wait_one_lwp (lp->ptid.lwp ());
  return 0;
}

/* Kill the fork/clone child of LP if it has an unfollowed child.  */

static int
kill_unfollowed_child_callback (lwp_info *lp)
{
  std::optional<target_waitstatus> ws = get_pending_child_status (lp);
  if (ws.has_value ())
    {
      ptid_t child_ptid = ws->child_ptid ();
      int child_pid = child_ptid.pid ();
      int child_lwp = child_ptid.lwp ();

      kill_one_lwp (child_lwp);
      kill_wait_one_lwp (child_lwp);

      /* Let the arch-specific native code know this process is
	 gone.  */
      if (ws->kind () != TARGET_WAITKIND_THREAD_CLONED)
	linux_target->low_forget_process (child_pid);
    }

  return 0;
}

void
linux_nat_target::kill ()
{
  ptid_t pid_ptid (inferior_ptid.pid ());

  /* If we're stopped while forking/cloning and we haven't followed
     yet, kill the child task.  We need to do this first because the
     parent will be sleeping if this is a vfork.  */
  iterate_over_lwps (pid_ptid, kill_unfollowed_child_callback);

  if (forks_exist_p ())
    linux_fork_killall ();
  else
    {
      /* Stop all threads before killing them, since ptrace requires
	 that the thread is stopped to successfully PTRACE_KILL.  */
      iterate_over_lwps (pid_ptid, stop_callback);
      /* ... and wait until all of them have reported back that
	 they're no longer running.  */
      iterate_over_lwps (pid_ptid, stop_wait_callback);

      /* Kill all LWP's ...  */
      iterate_over_lwps (pid_ptid, kill_callback);

      /* ... and wait until we've flushed all events.  */
      iterate_over_lwps (pid_ptid, kill_wait_callback);
    }

  target_mourn_inferior (inferior_ptid);
}

void
linux_nat_target::mourn_inferior ()
{
  LINUX_NAT_SCOPED_DEBUG_ENTER_EXIT;

  int pid = inferior_ptid.pid ();

  purge_lwp_list (pid);

  close_proc_mem_file (pid);

  if (! forks_exist_p ())
    /* Normal case, no other forks available.  */
    inf_ptrace_target::mourn_inferior ();
  else
    /* Multi-fork case.  The current inferior_ptid has exited, but
       there are other viable forks to debug.  Delete the exiting
       one and context-switch to the first available.  */
    linux_fork_mourn_inferior ();

  /* Let the arch-specific native code know this process is gone.  */
  linux_target->low_forget_process (pid);
}

/* Convert a native/host siginfo object, into/from the siginfo in the
   layout of the inferiors' architecture.  */

static void
siginfo_fixup (siginfo_t *siginfo, gdb_byte *inf_siginfo, int direction)
{
  /* If the low target didn't do anything, then just do a straight
     memcpy.  */
  if (!linux_target->low_siginfo_fixup (siginfo, inf_siginfo, direction))
    {
      if (direction == 1)
	memcpy (siginfo, inf_siginfo, sizeof (siginfo_t));
      else
	memcpy (inf_siginfo, siginfo, sizeof (siginfo_t));
    }
}

static enum target_xfer_status
linux_xfer_siginfo (ptid_t ptid, enum target_object object,
		    const char *annex, gdb_byte *readbuf,
		    const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
		    ULONGEST *xfered_len)
{
  siginfo_t siginfo;
  gdb_byte inf_siginfo[sizeof (siginfo_t)];

  gdb_assert (object == TARGET_OBJECT_SIGNAL_INFO);
  gdb_assert (readbuf || writebuf);

  if (offset > sizeof (siginfo))
    return TARGET_XFER_E_IO;

  if (!linux_nat_get_siginfo (ptid, &siginfo))
    return TARGET_XFER_E_IO;

  /* When GDB is built as a 64-bit application, ptrace writes into
     SIGINFO an object with 64-bit layout.  Since debugging a 32-bit
     inferior with a 64-bit GDB should look the same as debugging it
     with a 32-bit GDB, we need to convert it.  GDB core always sees
     the converted layout, so any read/write will have to be done
     post-conversion.  */
  siginfo_fixup (&siginfo, inf_siginfo, 0);

  if (offset + len > sizeof (siginfo))
    len = sizeof (siginfo) - offset;

  if (readbuf != NULL)
    memcpy (readbuf, inf_siginfo + offset, len);
  else
    {
      memcpy (inf_siginfo + offset, writebuf, len);

      /* Convert back to ptrace layout before flushing it out.  */
      siginfo_fixup (&siginfo, inf_siginfo, 1);

      int pid = get_ptrace_pid (ptid);
      errno = 0;
      ptrace (PTRACE_SETSIGINFO, pid, (PTRACE_TYPE_ARG3) 0, &siginfo);
      if (errno != 0)
	return TARGET_XFER_E_IO;
    }

  *xfered_len = len;
  return TARGET_XFER_OK;
}

static enum target_xfer_status
linux_nat_xfer_osdata (enum target_object object,
		       const char *annex, gdb_byte *readbuf,
		       const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
		       ULONGEST *xfered_len);

static enum target_xfer_status
linux_proc_xfer_memory_partial (int pid, gdb_byte *readbuf,
				const gdb_byte *writebuf, ULONGEST offset,
				LONGEST len, ULONGEST *xfered_len);

enum target_xfer_status
linux_nat_target::xfer_partial (enum target_object object,
				const char *annex, gdb_byte *readbuf,
				const gdb_byte *writebuf,
				ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  if (object == TARGET_OBJECT_SIGNAL_INFO)
    return linux_xfer_siginfo (inferior_ptid, object, annex, readbuf, writebuf,
			       offset, len, xfered_len);

  /* The target is connected but no live inferior is selected.  Pass
     this request down to a lower stratum (e.g., the executable
     file).  */
  if (object == TARGET_OBJECT_MEMORY && inferior_ptid == null_ptid)
    return TARGET_XFER_EOF;

  if (object == TARGET_OBJECT_AUXV)
    return memory_xfer_auxv (this, object, annex, readbuf, writebuf,
			     offset, len, xfered_len);

  if (object == TARGET_OBJECT_OSDATA)
    return linux_nat_xfer_osdata (object, annex, readbuf, writebuf,
				  offset, len, xfered_len);

  if (object == TARGET_OBJECT_MEMORY)
    {
      /* GDB calculates all addresses in the largest possible address
	 width.  The address width must be masked before its final use
	 by linux_proc_xfer_partial.

	 Compare ADDR_BIT first to avoid a compiler warning on shift overflow.  */
      int addr_bit = gdbarch_addr_bit (current_inferior ()->arch ());

      if (addr_bit < (sizeof (ULONGEST) * HOST_CHAR_BIT))
	offset &= ((ULONGEST) 1 << addr_bit) - 1;

      /* If /proc/pid/mem is writable, don't fallback to ptrace.  If
	 the write via /proc/pid/mem fails because the inferior execed
	 (and we haven't seen the exec event yet), a subsequent ptrace
	 poke would incorrectly write memory to the post-exec address
	 space, while the core was trying to write to the pre-exec
	 address space.  */
      if (proc_mem_file_is_writable ())
	return linux_proc_xfer_memory_partial (inferior_ptid.pid (), readbuf,
					       writebuf, offset, len,
					       xfered_len);
    }

  return inf_ptrace_target::xfer_partial (object, annex, readbuf, writebuf,
					  offset, len, xfered_len);
}

bool
linux_nat_target::thread_alive (ptid_t ptid)
{
  /* As long as a PTID is in lwp list, consider it alive.  */
  return find_lwp_pid (ptid) != NULL;
}

/* Implement the to_update_thread_list target method for this
   target.  */

void
linux_nat_target::update_thread_list ()
{
  /* We add/delete threads from the list as clone/exit events are
     processed, so just try deleting exited threads still in the
     thread list.  */
  delete_exited_threads ();

  /* Update the processor core that each lwp/thread was last seen
     running on.  */
  for (lwp_info *lwp : all_lwps ())
    {
      /* Avoid accessing /proc if the thread hasn't run since we last
	 time we fetched the thread's core.  Accessing /proc becomes
	 noticeably expensive when we have thousands of LWPs.  */
      if (lwp->core == -1)
	lwp->core = linux_common_core_of_thread (lwp->ptid);
    }
}

std::string
linux_nat_target::pid_to_str (ptid_t ptid)
{
  if (ptid.lwp_p ()
      && (ptid.pid () != ptid.lwp ()
	  || num_lwps (ptid.pid ()) > 1))
    return string_printf ("LWP %ld", ptid.lwp ());

  return normal_pid_to_str (ptid);
}

const char *
linux_nat_target::thread_name (struct thread_info *thr)
{
  return linux_proc_tid_get_name (thr->ptid);
}

/* Accepts an integer PID; Returns a string representing a file that
   can be opened to get the symbols for the child process.  */

const char *
linux_nat_target::pid_to_exec_file (int pid)
{
  return linux_proc_pid_to_exec_file (pid);
}

/* Object representing an /proc/PID/mem open file.  We keep one such
   file open per inferior.

   It might be tempting to think about only ever opening one file at
   most for all inferiors, closing/reopening the file as we access
   memory of different inferiors, to minimize number of file
   descriptors open, which can otherwise run into resource limits.
   However, that does not work correctly -- if the inferior execs and
   we haven't processed the exec event yet, and, we opened a
   /proc/PID/mem file, we will get a mem file accessing the post-exec
   address space, thinking we're opening it for the pre-exec address
   space.  That is dangerous as we can poke memory (e.g. clearing
   breakpoints) in the post-exec memory by mistake, corrupting the
   inferior.  For that reason, we open the mem file as early as
   possible, right after spawning, forking or attaching to the
   inferior, when the inferior is stopped and thus before it has a
   chance of execing.

   Note that after opening the file, even if the thread we opened it
   for subsequently exits, the open file is still usable for accessing
   memory.  It's only when the whole process exits or execs that the
   file becomes invalid, at which point reads/writes return EOF.  */

class proc_mem_file
{
public:
  proc_mem_file (ptid_t ptid, int fd)
    : m_ptid (ptid), m_fd (fd)
  {
    gdb_assert (m_fd != -1);
  }

  ~proc_mem_file ()
  {
    linux_nat_debug_printf ("closing fd %d for /proc/%d/task/%ld/mem",
			    m_fd, m_ptid.pid (), m_ptid.lwp ());
    close (m_fd);
  }

  DISABLE_COPY_AND_ASSIGN (proc_mem_file);

  int fd ()
  {
    return m_fd;
  }

private:
  /* The LWP this file was opened for.  Just for debugging
     purposes.  */
  ptid_t m_ptid;

  /* The file descriptor.  */
  int m_fd = -1;
};

/* The map between an inferior process id, and the open /proc/PID/mem
   file.  This is stored in a map instead of in a per-inferior
   structure because we need to be able to access memory of processes
   which don't have a corresponding struct inferior object.  E.g.,
   with "detach-on-fork on" (the default), and "follow-fork parent"
   (also default), we don't create an inferior for the fork child, but
   we still need to remove breakpoints from the fork child's
   memory.  */
static std::unordered_map<int, proc_mem_file> proc_mem_file_map;

/* Close the /proc/PID/mem file for PID.  */

static void
close_proc_mem_file (pid_t pid)
{
  proc_mem_file_map.erase (pid);
}

/* Open the /proc/PID/mem file for the process (thread group) of PTID.
   We actually open /proc/PID/task/LWP/mem, as that's the LWP we know
   exists and is stopped right now.  We prefer the
   /proc/PID/task/LWP/mem form over /proc/LWP/mem to avoid tid-reuse
   races, just in case this is ever called on an already-waited
   LWP.  */

static void
open_proc_mem_file (ptid_t ptid)
{
  auto iter = proc_mem_file_map.find (ptid.pid ());
  gdb_assert (iter == proc_mem_file_map.end ());

  char filename[64];
  xsnprintf (filename, sizeof filename,
	     "/proc/%d/task/%ld/mem", ptid.pid (), ptid.lwp ());

  int fd = gdb_open_cloexec (filename, O_RDWR | O_LARGEFILE, 0).release ();

  if (fd == -1)
    {
      warning (_("opening /proc/PID/mem file for lwp %d.%ld failed: %s (%d)"),
	       ptid.pid (), ptid.lwp (),
	       safe_strerror (errno), errno);
      return;
    }

  proc_mem_file_map.emplace (std::piecewise_construct,
			     std::forward_as_tuple (ptid.pid ()),
			     std::forward_as_tuple (ptid, fd));

  linux_nat_debug_printf ("opened fd %d for lwp %d.%ld",
			  fd, ptid.pid (), ptid.lwp ());
}

/* Helper for linux_proc_xfer_memory_partial and
   proc_mem_file_is_writable.  FD is the already opened /proc/pid/mem
   file, and PID is the pid of the corresponding process.  The rest of
   the arguments are like linux_proc_xfer_memory_partial's.  */

static enum target_xfer_status
linux_proc_xfer_memory_partial_fd (int fd, int pid,
				   gdb_byte *readbuf, const gdb_byte *writebuf,
				   ULONGEST offset, LONGEST len,
				   ULONGEST *xfered_len)
{
  ssize_t ret;

  gdb_assert (fd != -1);

  /* Use pread64/pwrite64 if available, since they save a syscall and
     can handle 64-bit offsets even on 32-bit platforms (for instance,
     SPARC debugging a SPARC64 application).  But only use them if the
     offset isn't so high that when cast to off_t it'd be negative, as
     seen on SPARC64.  pread64/pwrite64 outright reject such offsets.
     lseek does not.  */
#ifdef HAVE_PREAD64
  if ((off_t) offset >= 0)
    ret = (readbuf != nullptr
	   ? pread64 (fd, readbuf, len, offset)
	   : pwrite64 (fd, writebuf, len, offset));
  else
#endif
    {
      ret = lseek (fd, offset, SEEK_SET);
      if (ret != -1)
	ret = (readbuf != nullptr
	       ? read (fd, readbuf, len)
	       : write (fd, writebuf, len));
    }

  if (ret == -1)
    {
      linux_nat_debug_printf ("accessing fd %d for pid %d failed: %s (%d)",
			      fd, pid, safe_strerror (errno), errno);
      return TARGET_XFER_E_IO;
    }
  else if (ret == 0)
    {
      /* EOF means the address space is gone, the whole process exited
	 or execed.  */
      linux_nat_debug_printf ("accessing fd %d for pid %d got EOF",
			      fd, pid);
      return TARGET_XFER_EOF;
    }
  else
    {
      *xfered_len = ret;
      return TARGET_XFER_OK;
    }
}

/* Implement the to_xfer_partial target method using /proc/PID/mem.
   Because we can use a single read/write call, this can be much more
   efficient than banging away at PTRACE_PEEKTEXT.  Also, unlike
   PTRACE_PEEKTEXT/PTRACE_POKETEXT, this works with running
   threads.  */

static enum target_xfer_status
linux_proc_xfer_memory_partial (int pid, gdb_byte *readbuf,
				const gdb_byte *writebuf, ULONGEST offset,
				LONGEST len, ULONGEST *xfered_len)
{
  auto iter = proc_mem_file_map.find (pid);
  if (iter == proc_mem_file_map.end ())
    return TARGET_XFER_EOF;

  int fd = iter->second.fd ();

  return linux_proc_xfer_memory_partial_fd (fd, pid, readbuf, writebuf, offset,
					    len, xfered_len);
}

/* Check whether /proc/pid/mem is writable in the current kernel, and
   return true if so.  It wasn't writable before Linux 2.6.39, but
   there's no way to know whether the feature was backported to older
   kernels.  So we check to see if it works.  The result is cached,
   and this is guaranteed to be called once early during inferior
   startup, so that any warning is printed out consistently between
   GDB invocations.  Note we don't call it during GDB startup instead
   though, because then we might warn with e.g. just "gdb --version"
   on sandboxed systems.  See PR gdb/29907.  */

static bool
proc_mem_file_is_writable ()
{
  static std::optional<bool> writable;

  if (writable.has_value ())
    return *writable;

  writable.emplace (false);

  /* We check whether /proc/pid/mem is writable by trying to write to
     one of our variables via /proc/self/mem.  */

  int fd = gdb_open_cloexec ("/proc/self/mem", O_RDWR | O_LARGEFILE, 0).release ();

  if (fd == -1)
    {
      warning (_("opening /proc/self/mem file failed: %s (%d)"),
	       safe_strerror (errno), errno);
      return *writable;
    }

  SCOPE_EXIT { close (fd); };

  /* This is the variable we try to write to.  Note OFFSET below.  */
  volatile gdb_byte test_var = 0;

  gdb_byte writebuf[] = {0x55};
  ULONGEST offset = (uintptr_t) &test_var;
  ULONGEST xfered_len;

  enum target_xfer_status res
    = linux_proc_xfer_memory_partial_fd (fd, getpid (), nullptr, writebuf,
					 offset, 1, &xfered_len);

  if (res == TARGET_XFER_OK)
    {
      gdb_assert (xfered_len == 1);
      gdb_assert (test_var == 0x55);
      /* Success.  */
      *writable = true;
    }

  return *writable;
}

/* Parse LINE as a signal set and add its set bits to SIGS.  */

static void
add_line_to_sigset (const char *line, sigset_t *sigs)
{
  int len = strlen (line) - 1;
  const char *p;
  int signum;

  if (line[len] != '\n')
    error (_("Could not parse signal set: %s"), line);

  p = line;
  signum = len * 4;
  while (len-- > 0)
    {
      int digit;

      if (*p >= '0' && *p <= '9')
	digit = *p - '0';
      else if (*p >= 'a' && *p <= 'f')
	digit = *p - 'a' + 10;
      else
	error (_("Could not parse signal set: %s"), line);

      signum -= 4;

      if (digit & 1)
	sigaddset (sigs, signum + 1);
      if (digit & 2)
	sigaddset (sigs, signum + 2);
      if (digit & 4)
	sigaddset (sigs, signum + 3);
      if (digit & 8)
	sigaddset (sigs, signum + 4);

      p++;
    }
}

/* Find process PID's pending signals from /proc/pid/status and set
   SIGS to match.  */

void
linux_proc_pending_signals (int pid, sigset_t *pending,
			    sigset_t *blocked, sigset_t *ignored)
{
  char buffer[PATH_MAX], fname[PATH_MAX];

  sigemptyset (pending);
  sigemptyset (blocked);
  sigemptyset (ignored);
  xsnprintf (fname, sizeof fname, "/proc/%d/status", pid);
  gdb_file_up procfile = gdb_fopen_cloexec (fname, "r");
  if (procfile == NULL)
    error (_("Could not open %s"), fname);

  while (fgets (buffer, PATH_MAX, procfile.get ()) != NULL)
    {
      /* Normal queued signals are on the SigPnd line in the status
	 file.  However, 2.6 kernels also have a "shared" pending
	 queue for delivering signals to a thread group, so check for
	 a ShdPnd line also.

	 Unfortunately some Red Hat kernels include the shared pending
	 queue but not the ShdPnd status field.  */

      if (startswith (buffer, "SigPnd:\t"))
	add_line_to_sigset (buffer + 8, pending);
      else if (startswith (buffer, "ShdPnd:\t"))
	add_line_to_sigset (buffer + 8, pending);
      else if (startswith (buffer, "SigBlk:\t"))
	add_line_to_sigset (buffer + 8, blocked);
      else if (startswith (buffer, "SigIgn:\t"))
	add_line_to_sigset (buffer + 8, ignored);
    }
}

static enum target_xfer_status
linux_nat_xfer_osdata (enum target_object object,
		       const char *annex, gdb_byte *readbuf,
		       const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
		       ULONGEST *xfered_len)
{
  gdb_assert (object == TARGET_OBJECT_OSDATA);

  *xfered_len = linux_common_xfer_osdata (annex, readbuf, offset, len);
  if (*xfered_len == 0)
    return TARGET_XFER_EOF;
  else
    return TARGET_XFER_OK;
}

std::vector<static_tracepoint_marker>
linux_nat_target::static_tracepoint_markers_by_strid (const char *strid)
{
  char s[IPA_CMD_BUF_SIZE];
  int pid = inferior_ptid.pid ();
  std::vector<static_tracepoint_marker> markers;
  const char *p = s;
  ptid_t ptid = ptid_t (pid, 0);
  static_tracepoint_marker marker;

  /* Pause all */
  target_stop (ptid);

  strcpy (s, "qTfSTM");
  agent_run_command (pid, s, strlen (s) + 1);

  /* Unpause all.  */
  SCOPE_EXIT { target_continue_no_signal (ptid); };

  while (*p++ == 'm')
    {
      do
	{
	  parse_static_tracepoint_marker_definition (p, &p, &marker);

	  if (strid == NULL || marker.str_id == strid)
	    markers.push_back (std::move (marker));
	}
      while (*p++ == ',');	/* comma-separated list */

      strcpy (s, "qTsSTM");
      agent_run_command (pid, s, strlen (s) + 1);
      p = s;
    }

  return markers;
}

/* target_can_async_p implementation.  */

bool
linux_nat_target::can_async_p ()
{
  /* This flag should be checked in the common target.c code.  */
  gdb_assert (target_async_permitted);
  
  /* Otherwise, this targets is always able to support async mode.  */
  return true;
}

bool
linux_nat_target::supports_non_stop ()
{
  return true;
}

/* to_always_non_stop_p implementation.  */

bool
linux_nat_target::always_non_stop_p ()
{
  return true;
}

bool
linux_nat_target::supports_multi_process ()
{
  return true;
}

bool
linux_nat_target::supports_disable_randomization ()
{
  return true;
}

/* SIGCHLD handler that serves two purposes: In non-stop/async mode,
   so we notice when any child changes state, and notify the
   event-loop; it allows us to use sigsuspend in linux_nat_wait_1
   above to wait for the arrival of a SIGCHLD.  */

static void
sigchld_handler (int signo)
{
  int old_errno = errno;

  if (debug_linux_nat)
    gdb_stdlog->write_async_safe ("sigchld\n", sizeof ("sigchld\n") - 1);

  if (signo == SIGCHLD)
    {
      /* Let the event loop know that there are events to handle.  */
      linux_nat_target::async_file_mark_if_open ();
    }

  errno = old_errno;
}

/* Callback registered with the target events file descriptor.  */

static void
handle_target_event (int error, gdb_client_data client_data)
{
  inferior_event_handler (INF_REG_EVENT);
}

/* target_async implementation.  */

void
linux_nat_target::async (bool enable)
{
  if (enable == is_async_p ())
    return;

  /* Block child signals while we create/destroy the pipe, as their
     handler writes to it.  */
  gdb::block_signals blocker;

  if (enable)
    {
      if (!async_file_open ())
	internal_error ("creating event pipe failed.");

      add_file_handler (async_wait_fd (), handle_target_event, NULL,
			"linux-nat");

      /* There may be pending events to handle.  Tell the event loop
	 to poll them.  */
      async_file_mark ();
    }
  else
    {
      delete_file_handler (async_wait_fd ());
      async_file_close ();
    }
}

/* Stop an LWP, and push a GDB_SIGNAL_0 stop status if no other
   event came out.  */

static int
linux_nat_stop_lwp (struct lwp_info *lwp)
{
  if (!lwp->stopped)
    {
      linux_nat_debug_printf ("running -> suspending %s",
			      lwp->ptid.to_string ().c_str ());


      if (lwp->last_resume_kind == resume_stop)
	{
	  linux_nat_debug_printf ("already stopping LWP %ld at GDB's request",
				  lwp->ptid.lwp ());
	  return 0;
	}

      stop_callback (lwp);
      lwp->last_resume_kind = resume_stop;
    }
  else
    {
      /* Already known to be stopped; do nothing.  */

      if (debug_linux_nat)
	{
	  if (linux_target->find_thread (lwp->ptid)->stop_requested)
	    linux_nat_debug_printf ("already stopped/stop_requested %s",
				    lwp->ptid.to_string ().c_str ());
	  else
	    linux_nat_debug_printf ("already stopped/no stop_requested yet %s",
				    lwp->ptid.to_string ().c_str ());
	}
    }
  return 0;
}

void
linux_nat_target::stop (ptid_t ptid)
{
  LINUX_NAT_SCOPED_DEBUG_ENTER_EXIT;
  iterate_over_lwps (ptid, linux_nat_stop_lwp);
}

/* Return the cached value of the processor core for thread PTID.  */

int
linux_nat_target::core_of_thread (ptid_t ptid)
{
  struct lwp_info *info = find_lwp_pid (ptid);

  if (info)
    return info->core;
  return -1;
}

/* Implementation of to_filesystem_is_local.  */

bool
linux_nat_target::filesystem_is_local ()
{
  struct inferior *inf = current_inferior ();

  if (inf->fake_pid_p || inf->pid == 0)
    return true;

  return linux_ns_same (inf->pid, LINUX_NS_MNT);
}

/* Convert the INF argument passed to a to_fileio_* method
   to a process ID suitable for passing to its corresponding
   linux_mntns_* function.  If INF is non-NULL then the
   caller is requesting the filesystem seen by INF.  If INF
   is NULL then the caller is requesting the filesystem seen
   by the GDB.  We fall back to GDB's filesystem in the case
   that INF is non-NULL but its PID is unknown.  */

static pid_t
linux_nat_fileio_pid_of (struct inferior *inf)
{
  if (inf == NULL || inf->fake_pid_p || inf->pid == 0)
    return getpid ();
  else
    return inf->pid;
}

/* Implementation of to_fileio_open.  */

int
linux_nat_target::fileio_open (struct inferior *inf, const char *filename,
			       int flags, int mode, int warn_if_slow,
			       fileio_error *target_errno)
{
  int nat_flags;
  mode_t nat_mode;
  int fd;

  if (fileio_to_host_openflags (flags, &nat_flags) == -1
      || fileio_to_host_mode (mode, &nat_mode) == -1)
    {
      *target_errno = FILEIO_EINVAL;
      return -1;
    }

  fd = linux_mntns_open_cloexec (linux_nat_fileio_pid_of (inf),
				 filename, nat_flags, nat_mode);
  if (fd == -1)
    *target_errno = host_to_fileio_error (errno);

  return fd;
}

/* Implementation of to_fileio_readlink.  */

std::optional<std::string>
linux_nat_target::fileio_readlink (struct inferior *inf, const char *filename,
				   fileio_error *target_errno)
{
  char buf[PATH_MAX];
  int len;

  len = linux_mntns_readlink (linux_nat_fileio_pid_of (inf),
			      filename, buf, sizeof (buf));
  if (len < 0)
    {
      *target_errno = host_to_fileio_error (errno);
      return {};
    }

  return std::string (buf, len);
}

/* Implementation of to_fileio_unlink.  */

int
linux_nat_target::fileio_unlink (struct inferior *inf, const char *filename,
				 fileio_error *target_errno)
{
  int ret;

  ret = linux_mntns_unlink (linux_nat_fileio_pid_of (inf),
			    filename);
  if (ret == -1)
    *target_errno = host_to_fileio_error (errno);

  return ret;
}

/* Implementation of the to_thread_events method.  */

void
linux_nat_target::thread_events (int enable)
{
  report_thread_events = enable;
}

bool
linux_nat_target::supports_set_thread_options (gdb_thread_options options)
{
  constexpr gdb_thread_options supported_options
    = GDB_THREAD_OPTION_CLONE | GDB_THREAD_OPTION_EXIT;
  return ((options & supported_options) == options);
}

linux_nat_target::linux_nat_target ()
{
  /* We don't change the stratum; this target will sit at
     process_stratum and thread_db will set at thread_stratum.  This
     is a little strange, since this is a multi-threaded-capable
     target, but we want to be on the stack below thread_db, and we
     also want to be used for single-threaded processes.  */
}

/* See linux-nat.h.  */

bool
linux_nat_get_siginfo (ptid_t ptid, siginfo_t *siginfo)
{
  int pid = get_ptrace_pid (ptid);
  return ptrace (PTRACE_GETSIGINFO, pid, (PTRACE_TYPE_ARG3) 0, siginfo) == 0;
}

/* See nat/linux-nat.h.  */

ptid_t
current_lwp_ptid (void)
{
  gdb_assert (inferior_ptid.lwp_p ());
  return inferior_ptid;
}

/* Implement 'maintenance info linux-lwps'.  Displays some basic
   information about all the current lwp_info objects.  */

static void
maintenance_info_lwps (const char *arg, int from_tty)
{
  if (all_lwps ().size () == 0)
    {
      gdb_printf ("No Linux LWPs\n");
      return;
    }

  /* Start the width at 8 to match the column heading below, then
     figure out the widest ptid string.  We'll use this to build our
     output table below.  */
  size_t ptid_width = 8;
  for (lwp_info *lp : all_lwps ())
    ptid_width = std::max (ptid_width, lp->ptid.to_string ().size ());

  /* Setup the table headers.  */
  struct ui_out *uiout = current_uiout;
  ui_out_emit_table table_emitter (uiout, 2, -1, "linux-lwps");
  uiout->table_header (ptid_width, ui_left, "lwp-ptid", _("LWP Ptid"));
  uiout->table_header (9, ui_left, "thread-info", _("Thread ID"));
  uiout->table_body ();

  /* Display one table row for each lwp_info.  */
  for (lwp_info *lp : all_lwps ())
    {
      ui_out_emit_tuple tuple_emitter (uiout, "lwp-entry");

      thread_info *th = linux_target->find_thread (lp->ptid);

      uiout->field_string ("lwp-ptid", lp->ptid.to_string ().c_str ());
      if (th == nullptr)
	uiout->field_string ("thread-info", "None");
      else
	uiout->field_string ("thread-info", print_full_thread_id (th));

      uiout->message ("\n");
    }
}

void _initialize_linux_nat ();
void
_initialize_linux_nat ()
{
  add_setshow_boolean_cmd ("linux-nat", class_maintenance,
			   &debug_linux_nat, _("\
Set debugging of GNU/Linux native target."), _("	\
Show debugging of GNU/Linux native target."), _("	\
When on, print debug messages relating to the GNU/Linux native target."),
			   nullptr,
			   show_debug_linux_nat,
			   &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("linux-namespaces", class_maintenance,
			   &debug_linux_namespaces, _("\
Set debugging of GNU/Linux namespaces module."), _("\
Show debugging of GNU/Linux namespaces module."), _("\
Enables printf debugging output."),
			   NULL,
			   NULL,
			   &setdebuglist, &showdebuglist);

  /* Install a SIGCHLD handler.  */
  sigchld_action.sa_handler = sigchld_handler;
  sigemptyset (&sigchld_action.sa_mask);
  sigchld_action.sa_flags = SA_RESTART;

  /* Make it the default.  */
  sigaction (SIGCHLD, &sigchld_action, NULL);

  /* Make sure we don't block SIGCHLD during a sigsuspend.  */
  gdb_sigmask (SIG_SETMASK, NULL, &suspend_mask);
  sigdelset (&suspend_mask, SIGCHLD);

  sigemptyset (&blocked_mask);

  lwp_lwpid_htab_create ();

  add_cmd ("linux-lwps", class_maintenance, maintenance_info_lwps,
	 _("List the Linux LWPS."), &maintenanceinfolist);
}


/* FIXME: kettenis/2000-08-26: The stuff on this page is specific to
   the GNU/Linux Threads library and therefore doesn't really belong
   here.  */

/* NPTL reserves the first two RT signals, but does not provide any
   way for the debugger to query the signal numbers - fortunately
   they don't change.  */
static int lin_thread_signals[] = { __SIGRTMIN, __SIGRTMIN + 1 };

/* See linux-nat.h.  */

unsigned int
lin_thread_get_thread_signal_num (void)
{
  return sizeof (lin_thread_signals) / sizeof (lin_thread_signals[0]);
}

/* See linux-nat.h.  */

int
lin_thread_get_thread_signal (unsigned int i)
{
  gdb_assert (i < lin_thread_get_thread_signal_num ());
  return lin_thread_signals[i];
}
