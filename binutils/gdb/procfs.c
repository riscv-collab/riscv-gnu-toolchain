/* Machine independent support for Solaris /proc (process file system) for GDB.

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

   Written by Michael Snyder at Cygnus Solutions.
   Based on work by Fred Fish, Stu Grossman, Geoff Noer, and others.

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
#include "gdbcore.h"
#include "elf-bfd.h"
#include "gdbcmd.h"
#include "gdbthread.h"
#include "regcache.h"
#include "inf-child.h"
#include "nat/fork-inferior.h"
#include "gdbarch.h"

#include <sys/procfs.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include "gdbsupport/gdb_wait.h"
#include <signal.h>
#include <ctype.h>
#include "gdb_bfd.h"
#include "auxv.h"
#include "procfs.h"
#include "observable.h"
#include "gdbsupport/scoped_fd.h"
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/buildargv.h"
#include "cli/cli-style.h"

/* This module provides the interface between GDB and the
   /proc file system, which is used on many versions of Unix
   as a means for debuggers to control other processes.

   /proc works by imitating a file system: you open a simulated file
   that represents the process you wish to interact with, and perform
   operations on that "file" in order to examine or change the state
   of the other process.

   The most important thing to know about /proc and this module is
   that there are two very different interfaces to /proc:

     One that uses the ioctl system call, and another that uses read
     and write system calls.

   This module supports only the Solaris version of the read/write
   interface.  */

#include <sys/types.h>
#include <dirent.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* Note: procfs-utils.h must be included after the above system header
   files, because it redefines various system calls using macros.
   This may be incompatible with the prototype declarations.  */

#include "proc-utils.h"

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"

/* =================== TARGET_OPS "MODULE" =================== */

/* This module defines the GDB target vector and its methods.  */


static enum target_xfer_status procfs_xfer_memory (gdb_byte *,
						   const gdb_byte *,
						   ULONGEST, ULONGEST,
						   ULONGEST *);

class procfs_target final : public inf_child_target
{
public:
  void create_inferior (const char *, const std::string &,
			char **, int) override;

  void kill () override;

  void mourn_inferior () override;

  void attach (const char *, int) override;
  void detach (inferior *inf, int) override;

  void resume (ptid_t, int, enum gdb_signal) override;
  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;

  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  void pass_signals (gdb::array_view<const unsigned char>) override;

  void files_info () override;

  void update_thread_list () override;

  bool thread_alive (ptid_t ptid) override;

  std::string pid_to_str (ptid_t) override;

  const char *pid_to_exec_file (int pid) override;

  thread_control_capabilities get_thread_control_capabilities () override
  { return tc_schedlock; }

  /* find_memory_regions support method for gcore */
  int find_memory_regions (find_memory_region_ftype func, void *data)
    override;

  gdb::unique_xmalloc_ptr<char> make_corefile_notes (bfd *, int *) override;

  bool info_proc (const char *, enum info_proc_what) override;

#if PR_MODEL_NATIVE == PR_MODEL_LP64
  int auxv_parse (const gdb_byte **readptr,
		  const gdb_byte *endptr, CORE_ADDR *typep, CORE_ADDR *valp)
    override;
#endif

  bool stopped_by_watchpoint () override;

  int insert_watchpoint (CORE_ADDR, int, enum target_hw_bp_type,
			 struct expression *) override;

  int remove_watchpoint (CORE_ADDR, int, enum target_hw_bp_type,
			 struct expression *) override;

  int region_ok_for_hw_watchpoint (CORE_ADDR, int) override;

  int can_use_hw_breakpoint (enum bptype, int, int) override;
  bool stopped_data_address (CORE_ADDR *) override;

  void procfs_init_inferior (int pid);
};

static procfs_target the_procfs_target;

#if PR_MODEL_NATIVE == PR_MODEL_LP64
/* When GDB is built as 64-bit application on Solaris, the auxv data
   is presented in 64-bit format.  We need to provide a custom parser
   to handle that.  */
int
procfs_target::auxv_parse (const gdb_byte **readptr,
			   const gdb_byte *endptr, CORE_ADDR *typep,
			   CORE_ADDR *valp)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  const gdb_byte *ptr = *readptr;

  if (endptr == ptr)
    return 0;

  if (endptr - ptr < 8 * 2)
    return -1;

  *typep = extract_unsigned_integer (ptr, 4, byte_order);
  ptr += 8;
  /* The size of data is always 64-bit.  If the application is 32-bit,
     it will be zero extended, as expected.  */
  *valp = extract_unsigned_integer (ptr, 8, byte_order);
  ptr += 8;

  *readptr = ptr;
  return 1;
}
#endif

/* =================== END, TARGET_OPS "MODULE" =================== */

/* =================== STRUCT PROCINFO "MODULE" =================== */

     /* FIXME: this comment will soon be out of date W.R.T. threads.  */

/* The procinfo struct is a wrapper to hold all the state information
   concerning a /proc process.  There should be exactly one procinfo
   for each process, and since GDB currently can debug only one
   process at a time, that means there should be only one procinfo.
   All of the LWP's of a process can be accessed indirectly thru the
   single process procinfo.

   However, against the day when GDB may debug more than one process,
   this data structure is kept in a list (which for now will hold no
   more than one member), and many functions will have a pointer to a
   procinfo as an argument.

   There will be a separate procinfo structure for use by the (not yet
   implemented) "info proc" command, so that we can print useful
   information about any random process without interfering with the
   inferior's procinfo information.  */

/* format strings for /proc paths */
#define CTL_PROC_NAME_FMT    "/proc/%d/ctl"
#define AS_PROC_NAME_FMT     "/proc/%d/as"
#define MAP_PROC_NAME_FMT    "/proc/%d/map"
#define STATUS_PROC_NAME_FMT "/proc/%d/status"
#define MAX_PROC_NAME_SIZE sizeof("/proc/999999/lwp/0123456789/lwpstatus")

typedef struct procinfo {
  struct procinfo *next;
  int pid;			/* Process ID    */
  int tid;			/* Thread/LWP id */

  /* process state */
  int was_stopped;
  int ignore_next_sigstop;

  int ctl_fd;			/* File descriptor for /proc control file */
  int status_fd;		/* File descriptor for /proc status file */
  int as_fd;			/* File descriptor for /proc as file */

  char pathname[MAX_PROC_NAME_SIZE];	/* Pathname to /proc entry */

  fltset_t saved_fltset;	/* Saved traced hardware fault set */
  sigset_t saved_sigset;	/* Saved traced signal set */
  sigset_t saved_sighold;	/* Saved held signal set */
  sysset_t *saved_exitset;	/* Saved traced system call exit set */
  sysset_t *saved_entryset;	/* Saved traced system call entry set */

  pstatus_t prstatus;		/* Current process status info */

  struct procinfo *thread_list;

  int status_valid : 1;
  int gregs_valid  : 1;
  int fpregs_valid : 1;
  int threads_valid: 1;
} procinfo;

/* Function prototypes for procinfo module: */

static procinfo *find_procinfo_or_die (int pid, int tid);
static procinfo *find_procinfo (int pid, int tid);
static procinfo *create_procinfo (int pid, int tid);
static void destroy_procinfo (procinfo *p);
static void dead_procinfo (procinfo *p, const char *msg, int killp);
static int open_procinfo_files (procinfo *p, int which);
static void close_procinfo_files (procinfo *p);

static int iterate_over_mappings
  (procinfo *pi, find_memory_region_ftype child_func, void *data,
   int (*func) (struct prmap *map, find_memory_region_ftype child_func,
		void *data));

/* The head of the procinfo list: */
static procinfo *procinfo_list;

/* Search the procinfo list.  Return a pointer to procinfo, or NULL if
   not found.  */

static procinfo *
find_procinfo (int pid, int tid)
{
  procinfo *pi;

  for (pi = procinfo_list; pi; pi = pi->next)
    if (pi->pid == pid)
      break;

  if (pi)
    if (tid)
      {
	/* Don't check threads_valid.  If we're updating the
	   thread_list, we want to find whatever threads are already
	   here.  This means that in general it is the caller's
	   responsibility to check threads_valid and update before
	   calling find_procinfo, if the caller wants to find a new
	   thread.  */

	for (pi = pi->thread_list; pi; pi = pi->next)
	  if (pi->tid == tid)
	    break;
      }

  return pi;
}

/* Calls find_procinfo, but errors on failure.  */

static procinfo *
find_procinfo_or_die (int pid, int tid)
{
  procinfo *pi = find_procinfo (pid, tid);

  if (pi == NULL)
    {
      if (tid)
	error (_("procfs: couldn't find pid %d "
		 "(kernel thread %d) in procinfo list."),
	       pid, tid);
      else
	error (_("procfs: couldn't find pid %d in procinfo list."), pid);
    }
  return pi;
}

/* Wrapper for `open'.  The appropriate open call is attempted; if
   unsuccessful, it will be retried as many times as needed for the
   EAGAIN and EINTR conditions.

   For other conditions, retry the open a limited number of times.  In
   addition, a short sleep is imposed prior to retrying the open.  The
   reason for this sleep is to give the kernel a chance to catch up
   and create the file in question in the event that GDB "wins" the
   race to open a file before the kernel has created it.  */

static int
open_with_retry (const char *pathname, int flags)
{
  int retries_remaining, status;

  retries_remaining = 2;

  while (1)
    {
      status = open (pathname, flags);

      if (status >= 0 || retries_remaining == 0)
	break;
      else if (errno != EINTR && errno != EAGAIN)
	{
	  retries_remaining--;
	  sleep (1);
	}
    }

  return status;
}

/* Open the file descriptor for the process or LWP.  We only open the
   control file descriptor; the others are opened lazily as needed.
   Returns the file descriptor, or zero for failure.  */

enum { FD_CTL, FD_STATUS, FD_AS };

static int
open_procinfo_files (procinfo *pi, int which)
{
  char tmp[MAX_PROC_NAME_SIZE];
  int  fd;

  /* This function is getting ALMOST long enough to break up into
     several.  Here is some rationale:

     There are several file descriptors that may need to be open
     for any given process or LWP.  The ones we're interested in are:
	 - control	 (ctl)	  write-only	change the state
	 - status	 (status) read-only	query the state
	 - address space (as)	  read/write	access memory
	 - map		 (map)	  read-only	virtual addr map
     Most of these are opened lazily as they are needed.
     The pathnames for the 'files' for an LWP look slightly
     different from those of a first-class process:
	 Pathnames for a process (<proc-id>):
	   /proc/<proc-id>/ctl
	   /proc/<proc-id>/status
	   /proc/<proc-id>/as
	   /proc/<proc-id>/map
	 Pathnames for an LWP (lwp-id):
	   /proc/<proc-id>/lwp/<lwp-id>/lwpctl
	   /proc/<proc-id>/lwp/<lwp-id>/lwpstatus
     An LWP has no map or address space file descriptor, since
     the memory map and address space are shared by all LWPs.  */

  /* In this case, there are several different file descriptors that
     we might be asked to open.  The control file descriptor will be
     opened early, but the others will be opened lazily as they are
     needed.  */

  strcpy (tmp, pi->pathname);
  switch (which) {	/* Which file descriptor to open?  */
  case FD_CTL:
    if (pi->tid)
      strcat (tmp, "/lwpctl");
    else
      strcat (tmp, "/ctl");
    fd = open_with_retry (tmp, O_WRONLY);
    if (fd < 0)
      return 0;		/* fail */
    pi->ctl_fd = fd;
    break;
  case FD_AS:
    if (pi->tid)
      return 0;		/* There is no 'as' file descriptor for an lwp.  */
    strcat (tmp, "/as");
    fd = open_with_retry (tmp, O_RDWR);
    if (fd < 0)
      return 0;		/* fail */
    pi->as_fd = fd;
    break;
  case FD_STATUS:
    if (pi->tid)
      strcat (tmp, "/lwpstatus");
    else
      strcat (tmp, "/status");
    fd = open_with_retry (tmp, O_RDONLY);
    if (fd < 0)
      return 0;		/* fail */
    pi->status_fd = fd;
    break;
  default:
    return 0;		/* unknown file descriptor */
  }

  return 1;		/* success */
}

/* Allocate a data structure and link it into the procinfo list.
   First tries to find a pre-existing one (FIXME: why?).  Returns the
   pointer to new procinfo struct.  */

static procinfo *
create_procinfo (int pid, int tid)
{
  procinfo *pi, *parent = NULL;

  pi = find_procinfo (pid, tid);
  if (pi != NULL)
    return pi;			/* Already exists, nothing to do.  */

  /* Find parent before doing malloc, to save having to cleanup.  */
  if (tid != 0)
    parent = find_procinfo_or_die (pid, 0);	/* FIXME: should I
						   create it if it
						   doesn't exist yet?  */

  pi = XNEW (procinfo);
  memset (pi, 0, sizeof (procinfo));
  pi->pid = pid;
  pi->tid = tid;

  pi->saved_entryset = XNEW (sysset_t);
  pi->saved_exitset = XNEW (sysset_t);

  /* Chain into list.  */
  if (tid == 0)
    {
      xsnprintf (pi->pathname, sizeof (pi->pathname), "/proc/%d", pid);
      pi->next = procinfo_list;
      procinfo_list = pi;
    }
  else
    {
      xsnprintf (pi->pathname, sizeof (pi->pathname), "/proc/%d/lwp/%d",
		 pid, tid);
      pi->next = parent->thread_list;
      parent->thread_list = pi;
    }
  return pi;
}

/* Close all file descriptors associated with the procinfo.  */

static void
close_procinfo_files (procinfo *pi)
{
  if (pi->ctl_fd > 0)
    close (pi->ctl_fd);
  if (pi->as_fd > 0)
    close (pi->as_fd);
  if (pi->status_fd > 0)
    close (pi->status_fd);
  pi->ctl_fd = pi->as_fd = pi->status_fd = 0;
}

/* Destructor function.  Close, unlink and deallocate the object.  */

static void
destroy_one_procinfo (procinfo **list, procinfo *pi)
{
  procinfo *ptr;

  /* Step one: unlink the procinfo from its list.  */
  if (pi == *list)
    *list = pi->next;
  else
    for (ptr = *list; ptr; ptr = ptr->next)
      if (ptr->next == pi)
	{
	  ptr->next =  pi->next;
	  break;
	}

  /* Step two: close any open file descriptors.  */
  close_procinfo_files (pi);

  /* Step three: free the memory.  */
  xfree (pi->saved_entryset);
  xfree (pi->saved_exitset);
  xfree (pi);
}

static void
destroy_procinfo (procinfo *pi)
{
  procinfo *tmp;

  if (pi->tid != 0)	/* Destroy a thread procinfo.  */
    {
      tmp = find_procinfo (pi->pid, 0);	/* Find the parent process.  */
      destroy_one_procinfo (&tmp->thread_list, pi);
    }
  else			/* Destroy a process procinfo and all its threads.  */
    {
      /* First destroy the children, if any; */
      while (pi->thread_list != NULL)
	destroy_one_procinfo (&pi->thread_list, pi->thread_list);
      /* Then destroy the parent.  Genocide!!!  */
      destroy_one_procinfo (&procinfo_list, pi);
    }
}

/* A deleter that calls destroy_procinfo.  */
struct procinfo_deleter
{
  void operator() (procinfo *pi) const
  {
    destroy_procinfo (pi);
  }
};

typedef std::unique_ptr<procinfo, procinfo_deleter> procinfo_up;

enum { NOKILL, KILL };

/* To be called on a non_recoverable error for a procinfo.  Prints
   error messages, optionally sends a SIGKILL to the process, then
   destroys the data structure.  */

static void
dead_procinfo (procinfo *pi, const char *msg, int kill_p)
{
  warning_filename_and_errno (pi->pathname, errno);
  if (kill_p == KILL)
    kill (pi->pid, SIGKILL);

  destroy_procinfo (pi);
  error ("%s", msg);
}

/* =================== END, STRUCT PROCINFO "MODULE" =================== */

/* ===================  /proc "MODULE" =================== */

/* This "module" is the interface layer between the /proc system API
   and the gdb target vector functions.  This layer consists of access
   functions that encapsulate each of the basic operations that we
   need to use from the /proc API.

   The main motivation for this layer is to hide the fact that there
   were two very different implementations of the /proc API.  */

static long proc_flags (procinfo *pi);
static int proc_why (procinfo *pi);
static int proc_what (procinfo *pi);
static int proc_set_current_signal (procinfo *pi, int signo);
static int proc_get_current_thread (procinfo *pi);
static int proc_iterate_over_threads
  (procinfo *pi,
   int (*func) (procinfo *, procinfo *, void *),
   void *ptr);
static void proc_resume (procinfo *pi, ptid_t scope_ptid,
			 int step, enum gdb_signal signo);

static void
proc_warn (procinfo *pi, const char *func, int line)
{
  int saved_errno = errno;
  warning ("procfs: %s line %d, %ps: %s",
	   func, line, styled_string (file_name_style.style (),
				      pi->pathname),
	   safe_strerror (saved_errno));
}

static void
proc_error (procinfo *pi, const char *func, int line)
{
  int saved_errno = errno;
  error ("procfs: %s line %d, %s: %s",
	 func, line, pi->pathname, safe_strerror (saved_errno));
}

/* Updates the status struct in the procinfo.  There is a 'valid'
   flag, to let other functions know when this function needs to be
   called (so the status is only read when it is needed).  The status
   file descriptor is also only opened when it is needed.  Returns
   non-zero for success, zero for failure.  */

static int
proc_get_status (procinfo *pi)
{
  /* Status file descriptor is opened "lazily".  */
  if (pi->status_fd == 0 && open_procinfo_files (pi, FD_STATUS) == 0)
    {
      pi->status_valid = 0;
      return 0;
    }

  if (lseek (pi->status_fd, 0, SEEK_SET) < 0)
    pi->status_valid = 0;			/* fail */
  else
    {
      /* Sigh... I have to read a different data structure,
	 depending on whether this is a main process or an LWP.  */
      if (pi->tid)
	pi->status_valid = (read (pi->status_fd,
				  (char *) &pi->prstatus.pr_lwp,
				  sizeof (lwpstatus_t))
			    == sizeof (lwpstatus_t));
      else
	{
	  pi->status_valid = (read (pi->status_fd,
				    (char *) &pi->prstatus,
				    sizeof (pstatus_t))
			      == sizeof (pstatus_t));
	}
    }

  if (pi->status_valid)
    {
      PROC_PRETTYFPRINT_STATUS (proc_flags (pi),
				proc_why (pi),
				proc_what (pi),
				proc_get_current_thread (pi));
    }

  /* The status struct includes general regs, so mark them valid too.  */
  pi->gregs_valid  = pi->status_valid;
  /* In the read/write multiple-fd model, the status struct includes
     the fp regs too, so mark them valid too.  */
  pi->fpregs_valid = pi->status_valid;
  return pi->status_valid;	/* True if success, false if failure.  */
}

/* Returns the process flags (pr_flags field).  */

static long
proc_flags (procinfo *pi)
{
  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return 0;	/* FIXME: not a good failure value (but what is?)  */

  return pi->prstatus.pr_lwp.pr_flags;
}

/* Returns the pr_why field (why the process stopped).  */

static int
proc_why (procinfo *pi)
{
  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return 0;	/* FIXME: not a good failure value (but what is?)  */

  return pi->prstatus.pr_lwp.pr_why;
}

/* Returns the pr_what field (details of why the process stopped).  */

static int
proc_what (procinfo *pi)
{
  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return 0;	/* FIXME: not a good failure value (but what is?)  */

  return pi->prstatus.pr_lwp.pr_what;
}

/* This function is only called when PI is stopped by a watchpoint.
   Assuming the OS supports it, write to *ADDR the data address which
   triggered it and return 1.  Return 0 if it is not possible to know
   the address.  */

static int
proc_watchpoint_address (procinfo *pi, CORE_ADDR *addr)
{
  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return 0;

  gdbarch *arch = current_inferior ()->arch ();
  *addr = gdbarch_pointer_to_address
	    (arch, builtin_type (arch)->builtin_data_ptr,
	     (gdb_byte *) &pi->prstatus.pr_lwp.pr_info.si_addr);
  return 1;
}

/* Returns the pr_nsysarg field (number of args to the current
   syscall).  */

static int
proc_nsysarg (procinfo *pi)
{
  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return 0;

  return pi->prstatus.pr_lwp.pr_nsysarg;
}

/* Returns the pr_sysarg field (pointer to the arguments of current
   syscall).  */

static long *
proc_sysargs (procinfo *pi)
{
  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return NULL;

  return (long *) &pi->prstatus.pr_lwp.pr_sysarg;
}

/* Set or reset any of the following process flags:
      PR_FORK	-- forked child will inherit trace flags
      PR_RLC	-- traced process runs when last /proc file closed.
      PR_KLC    -- traced process is killed when last /proc file closed.
      PR_ASYNC	-- LWP's get to run/stop independently.

   This function is done using read/write [PCSET/PCRESET/PCUNSET].

   Arguments:
      pi   -- the procinfo
      flag -- one of PR_FORK, PR_RLC, or PR_ASYNC
      mode -- 1 for set, 0 for reset.

   Returns non-zero for success, zero for failure.  */

enum { FLAG_RESET, FLAG_SET };

static int
proc_modify_flag (procinfo *pi, long flag, long mode)
{
  long win = 0;		/* default to fail */

  /* These operations affect the process as a whole, and applying them
     to an individual LWP has the same meaning as applying them to the
     main process.  Therefore, if we're ever called with a pointer to
     an LWP's procinfo, let's substitute the process's procinfo and
     avoid opening the LWP's file descriptor unnecessarily.  */

  if (pi->pid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  procfs_ctl_t arg[2];

  if (mode == FLAG_SET)	/* Set the flag (RLC, FORK, or ASYNC).  */
    arg[0] = PCSET;
  else			/* Reset the flag.  */
    arg[0] = PCUNSET;

  arg[1] = flag;
  win = (write (pi->ctl_fd, (void *) &arg, sizeof (arg)) == sizeof (arg));

  /* The above operation renders the procinfo's cached pstatus
     obsolete.  */
  pi->status_valid = 0;

  if (!win)
    warning (_("procfs: modify_flag failed to turn %s %s"),
	     flag == PR_FORK  ? "PR_FORK"  :
	     flag == PR_RLC   ? "PR_RLC"   :
	     flag == PR_ASYNC ? "PR_ASYNC" :
	     flag == PR_KLC   ? "PR_KLC"   :
	     "<unknown flag>",
	     mode == FLAG_RESET ? "off" : "on");

  return win;
}

/* Set the run_on_last_close flag.  Process with all threads will
   become runnable when debugger closes all /proc fds.  Returns
   non-zero for success, zero for failure.  */

static int
proc_set_run_on_last_close (procinfo *pi)
{
  return proc_modify_flag (pi, PR_RLC, FLAG_SET);
}

/* Reset the run_on_last_close flag.  The process will NOT become
   runnable when debugger closes its file handles.  Returns non-zero
   for success, zero for failure.  */

static int
proc_unset_run_on_last_close (procinfo *pi)
{
  return proc_modify_flag (pi, PR_RLC, FLAG_RESET);
}

/* Reset inherit_on_fork flag.  If the process forks a child while we
   are registered for events in the parent, then we will NOT receive
   events from the child.  Returns non-zero for success, zero for
   failure.  */

static int
proc_unset_inherit_on_fork (procinfo *pi)
{
  return proc_modify_flag (pi, PR_FORK, FLAG_RESET);
}

/* Set PR_ASYNC flag.  If one LWP stops because of a debug event
   (signal etc.), the remaining LWPs will continue to run.  Returns
   non-zero for success, zero for failure.  */

static int
proc_set_async (procinfo *pi)
{
  return proc_modify_flag (pi, PR_ASYNC, FLAG_SET);
}

/* Reset PR_ASYNC flag.  If one LWP stops because of a debug event
   (signal etc.), then all other LWPs will stop as well.  Returns
   non-zero for success, zero for failure.  */

static int
proc_unset_async (procinfo *pi)
{
  return proc_modify_flag (pi, PR_ASYNC, FLAG_RESET);
}

/* Request the process/LWP to stop.  Does not wait.  Returns non-zero
   for success, zero for failure.  */

static int
proc_stop_process (procinfo *pi)
{
  int win;

  /* We might conceivably apply this operation to an LWP, and the
     LWP's ctl file descriptor might not be open.  */

  if (pi->ctl_fd == 0 && open_procinfo_files (pi, FD_CTL) == 0)
    return 0;
  else
    {
      procfs_ctl_t cmd = PCSTOP;

      win = (write (pi->ctl_fd, (char *) &cmd, sizeof (cmd)) == sizeof (cmd));
    }

  return win;
}

/* Wait for the process or LWP to stop (block until it does).  Returns
   non-zero for success, zero for failure.  */

static int
proc_wait_for_stop (procinfo *pi)
{
  int win;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  procfs_ctl_t cmd = PCWSTOP;

  set_sigint_trap ();

  win = (write (pi->ctl_fd, (char *) &cmd, sizeof (cmd)) == sizeof (cmd));

  clear_sigint_trap ();

  /* We been runnin' and we stopped -- need to update status.  */
  pi->status_valid = 0;

  return win;
}

/* Make the process or LWP runnable.

   Options (not all are implemented):
     - single-step
     - clear current fault
     - clear current signal
     - abort the current system call
     - stop as soon as finished with system call

   Always clears the current fault.  PI is the process or LWP to
   operate on.  If STEP is true, set the process or LWP to trap after
   one instruction.  If SIGNO is zero, clear the current signal if
   any; if non-zero, set the current signal to this one.  Returns
   non-zero for success, zero for failure.  */

static int
proc_run_process (procinfo *pi, int step, int signo)
{
  int win;
  int runflags;

  /* We will probably have to apply this operation to individual
     threads, so make sure the control file descriptor is open.  */

  if (pi->ctl_fd == 0 && open_procinfo_files (pi, FD_CTL) == 0)
    return 0;

  runflags    = PRCFAULT;	/* Always clear current fault.  */
  if (step)
    runflags |= PRSTEP;
  if (signo == 0)
    runflags |= PRCSIG;
  else if (signo != -1)		/* -1 means do nothing W.R.T. signals.  */
    proc_set_current_signal (pi, signo);

  procfs_ctl_t cmd[2];

  cmd[0]  = PCRUN;
  cmd[1]  = runflags;
  win = (write (pi->ctl_fd, (char *) &cmd, sizeof (cmd)) == sizeof (cmd));

  return win;
}

/* Register to trace signals in the process or LWP.  Returns non-zero
   for success, zero for failure.  */

static int
proc_set_traced_signals (procinfo *pi, sigset_t *sigset)
{
  int win;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  struct {
    procfs_ctl_t cmd;
    /* Use char array to avoid alignment issues.  */
    char sigset[sizeof (sigset_t)];
  } arg;

  arg.cmd = PCSTRACE;
  memcpy (&arg.sigset, sigset, sizeof (sigset_t));

  win = (write (pi->ctl_fd, (char *) &arg, sizeof (arg)) == sizeof (arg));

  /* The above operation renders the procinfo's cached pstatus obsolete.  */
  pi->status_valid = 0;

  if (!win)
    warning (_("procfs: set_traced_signals failed"));
  return win;
}

/* Register to trace hardware faults in the process or LWP.  Returns
   non-zero for success, zero for failure.  */

static int
proc_set_traced_faults (procinfo *pi, fltset_t *fltset)
{
  int win;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  struct {
    procfs_ctl_t cmd;
    /* Use char array to avoid alignment issues.  */
    char fltset[sizeof (fltset_t)];
  } arg;

  arg.cmd = PCSFAULT;
  memcpy (&arg.fltset, fltset, sizeof (fltset_t));

  win = (write (pi->ctl_fd, (char *) &arg, sizeof (arg)) == sizeof (arg));

  /* The above operation renders the procinfo's cached pstatus obsolete.  */
  pi->status_valid = 0;

  return win;
}

/* Register to trace entry to system calls in the process or LWP.
   Returns non-zero for success, zero for failure.  */

static int
proc_set_traced_sysentry (procinfo *pi, sysset_t *sysset)
{
  int win;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  struct {
    procfs_ctl_t cmd;
    /* Use char array to avoid alignment issues.  */
    char sysset[sizeof (sysset_t)];
  } arg;

  arg.cmd = PCSENTRY;
  memcpy (&arg.sysset, sysset, sizeof (sysset_t));

  win = (write (pi->ctl_fd, (char *) &arg, sizeof (arg)) == sizeof (arg));

  /* The above operation renders the procinfo's cached pstatus
     obsolete.  */
  pi->status_valid = 0;

  return win;
}

/* Register to trace exit from system calls in the process or LWP.
   Returns non-zero for success, zero for failure.  */

static int
proc_set_traced_sysexit (procinfo *pi, sysset_t *sysset)
{
  int win;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  struct gdb_proc_ctl_pcsexit {
    procfs_ctl_t cmd;
    /* Use char array to avoid alignment issues.  */
    char sysset[sizeof (sysset_t)];
  } arg;

  arg.cmd = PCSEXIT;
  memcpy (&arg.sysset, sysset, sizeof (sysset_t));

  win = (write (pi->ctl_fd, (char *) &arg, sizeof (arg)) == sizeof (arg));

  /* The above operation renders the procinfo's cached pstatus
     obsolete.  */
  pi->status_valid = 0;

  return win;
}

/* Specify the set of blocked / held signals in the process or LWP.
   Returns non-zero for success, zero for failure.  */

static int
proc_set_held_signals (procinfo *pi, sigset_t *sighold)
{
  int win;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  struct {
    procfs_ctl_t cmd;
    /* Use char array to avoid alignment issues.  */
    char hold[sizeof (sigset_t)];
  } arg;

  arg.cmd  = PCSHOLD;
  memcpy (&arg.hold, sighold, sizeof (sigset_t));
  win = (write (pi->ctl_fd, (void *) &arg, sizeof (arg)) == sizeof (arg));

  /* The above operation renders the procinfo's cached pstatus
     obsolete.  */
  pi->status_valid = 0;

  return win;
}

/* Returns the set of signals that are held / blocked.  Will also copy
   the sigset if SAVE is non-zero.  */

static sigset_t *
proc_get_held_signals (procinfo *pi, sigset_t *save)
{
  sigset_t *ret = NULL;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return NULL;

  ret = &pi->prstatus.pr_lwp.pr_lwphold;
  if (save && ret)
    memcpy (save, ret, sizeof (sigset_t));

  return ret;
}

/* Returns the set of signals that are traced / debugged.  Will also
   copy the sigset if SAVE is non-zero.  */

static sigset_t *
proc_get_traced_signals (procinfo *pi, sigset_t *save)
{
  sigset_t *ret = NULL;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return NULL;

  ret = &pi->prstatus.pr_sigtrace;
  if (save && ret)
    memcpy (save, ret, sizeof (sigset_t));

  return ret;
}

/* Returns the set of hardware faults that are traced /debugged.  Will
   also copy the faultset if SAVE is non-zero.  */

static fltset_t *
proc_get_traced_faults (procinfo *pi, fltset_t *save)
{
  fltset_t *ret = NULL;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return NULL;

  ret = &pi->prstatus.pr_flttrace;
  if (save && ret)
    memcpy (save, ret, sizeof (fltset_t));

  return ret;
}

/* Returns the set of syscalls that are traced /debugged on entry.
   Will also copy the syscall set if SAVE is non-zero.  */

static sysset_t *
proc_get_traced_sysentry (procinfo *pi, sysset_t *save)
{
  sysset_t *ret = NULL;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return NULL;

  ret = &pi->prstatus.pr_sysentry;
  if (save && ret)
    memcpy (save, ret, sizeof (sysset_t));

  return ret;
}

/* Returns the set of syscalls that are traced /debugged on exit.
   Will also copy the syscall set if SAVE is non-zero.  */

static sysset_t *
proc_get_traced_sysexit (procinfo *pi, sysset_t *save)
{
  sysset_t *ret = NULL;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return NULL;

  ret = &pi->prstatus.pr_sysexit;
  if (save && ret)
    memcpy (save, ret, sizeof (sysset_t));

  return ret;
}

/* The current fault (if any) is cleared; the associated signal will
   not be sent to the process or LWP when it resumes.  Returns
   non-zero for success, zero for failure.  */

static int
proc_clear_current_fault (procinfo *pi)
{
  int win;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  procfs_ctl_t cmd = PCCFAULT;

  win = (write (pi->ctl_fd, (void *) &cmd, sizeof (cmd)) == sizeof (cmd));

  return win;
}

/* Set the "current signal" that will be delivered next to the
   process.  NOTE: semantics are different from those of KILL.  This
   signal will be delivered to the process or LWP immediately when it
   is resumed (even if the signal is held/blocked); it will NOT
   immediately cause another event of interest, and will NOT first
   trap back to the debugger.  Returns non-zero for success, zero for
   failure.  */

static int
proc_set_current_signal (procinfo *pi, int signo)
{
  int win;
  struct {
    procfs_ctl_t cmd;
    /* Use char array to avoid alignment issues.  */
    char sinfo[sizeof (siginfo_t)];
  } arg;
  siginfo_t mysinfo;
  process_stratum_target *wait_target;
  ptid_t wait_ptid;
  struct target_waitstatus wait_status;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  /* The pointer is just a type alias.  */
  get_last_target_status (&wait_target, &wait_ptid, &wait_status);
  if (wait_target == &the_procfs_target
      && wait_ptid == inferior_ptid
      && wait_status.kind () == TARGET_WAITKIND_STOPPED
      && wait_status.sig () == gdb_signal_from_host (signo)
      && proc_get_status (pi)
      && pi->prstatus.pr_lwp.pr_info.si_signo == signo
      )
    /* Use the siginfo associated with the signal being
       redelivered.  */
    memcpy (arg.sinfo, &pi->prstatus.pr_lwp.pr_info, sizeof (siginfo_t));
  else
    {
      mysinfo.si_signo = signo;
      mysinfo.si_code  = 0;
      mysinfo.si_pid   = getpid ();       /* ?why? */
      mysinfo.si_uid   = getuid ();       /* ?why? */
      memcpy (arg.sinfo, &mysinfo, sizeof (siginfo_t));
    }

  arg.cmd = PCSSIG;
  win = (write (pi->ctl_fd, (void *) &arg, sizeof (arg))  == sizeof (arg));

  return win;
}

/* The current signal (if any) is cleared, and is not sent to the
   process or LWP when it resumes.  Returns non-zero for success, zero
   for failure.  */

static int
proc_clear_current_signal (procinfo *pi)
{
  int win;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  struct {
    procfs_ctl_t cmd;
    /* Use char array to avoid alignment issues.  */
    char sinfo[sizeof (siginfo_t)];
  } arg;
  siginfo_t mysinfo;

  arg.cmd = PCSSIG;
  /* The pointer is just a type alias.  */
  mysinfo.si_signo = 0;
  mysinfo.si_code  = 0;
  mysinfo.si_errno = 0;
  mysinfo.si_pid   = getpid ();       /* ?why? */
  mysinfo.si_uid   = getuid ();       /* ?why? */
  memcpy (arg.sinfo, &mysinfo, sizeof (siginfo_t));

  win = (write (pi->ctl_fd, (void *) &arg, sizeof (arg)) == sizeof (arg));

  return win;
}

/* Return the general-purpose registers for the process or LWP
   corresponding to PI.  Upon failure, return NULL.  */

static gdb_gregset_t *
proc_get_gregs (procinfo *pi)
{
  if (!pi->status_valid || !pi->gregs_valid)
    if (!proc_get_status (pi))
      return NULL;

  return &pi->prstatus.pr_lwp.pr_reg;
}

/* Return the general-purpose registers for the process or LWP
   corresponding to PI.  Upon failure, return NULL.  */

static gdb_fpregset_t *
proc_get_fpregs (procinfo *pi)
{
  if (!pi->status_valid || !pi->fpregs_valid)
    if (!proc_get_status (pi))
      return NULL;

  return &pi->prstatus.pr_lwp.pr_fpreg;
}

/* Write the general-purpose registers back to the process or LWP
   corresponding to PI.  Return non-zero for success, zero for
   failure.  */

static int
proc_set_gregs (procinfo *pi)
{
  gdb_gregset_t *gregs;
  int win;

  gregs = proc_get_gregs (pi);
  if (gregs == NULL)
    return 0;			/* proc_get_regs has already warned.  */

  if (pi->ctl_fd == 0 && open_procinfo_files (pi, FD_CTL) == 0)
    return 0;
  else
    {
      struct {
	procfs_ctl_t cmd;
	/* Use char array to avoid alignment issues.  */
	char gregs[sizeof (gdb_gregset_t)];
      } arg;

      arg.cmd = PCSREG;
      memcpy (&arg.gregs, gregs, sizeof (arg.gregs));
      win = (write (pi->ctl_fd, (void *) &arg, sizeof (arg)) == sizeof (arg));
    }

  /* Policy: writing the registers invalidates our cache.  */
  pi->gregs_valid = 0;
  return win;
}

/* Write the floating-pointer registers back to the process or LWP
   corresponding to PI.  Return non-zero for success, zero for
   failure.  */

static int
proc_set_fpregs (procinfo *pi)
{
  gdb_fpregset_t *fpregs;
  int win;

  fpregs = proc_get_fpregs (pi);
  if (fpregs == NULL)
    return 0;			/* proc_get_fpregs has already warned.  */

  if (pi->ctl_fd == 0 && open_procinfo_files (pi, FD_CTL) == 0)
    return 0;
  else
    {
      struct {
	procfs_ctl_t cmd;
	/* Use char array to avoid alignment issues.  */
	char fpregs[sizeof (gdb_fpregset_t)];
      } arg;

      arg.cmd = PCSFPREG;
      memcpy (&arg.fpregs, fpregs, sizeof (arg.fpregs));
      win = (write (pi->ctl_fd, (void *) &arg, sizeof (arg)) == sizeof (arg));
    }

  /* Policy: writing the registers invalidates our cache.  */
  pi->fpregs_valid = 0;
  return win;
}

/* Send a signal to the proc or lwp with the semantics of "kill()".
   Returns non-zero for success, zero for failure.  */

static int
proc_kill (procinfo *pi, int signo)
{
  int win;

  /* We might conceivably apply this operation to an LWP, and the
     LWP's ctl file descriptor might not be open.  */

  if (pi->ctl_fd == 0 && open_procinfo_files (pi, FD_CTL) == 0)
    return 0;
  else
    {
      procfs_ctl_t cmd[2];

      cmd[0] = PCKILL;
      cmd[1] = signo;
      win = (write (pi->ctl_fd, (char *) &cmd, sizeof (cmd)) == sizeof (cmd));
  }

  return win;
}

/* Find the pid of the process that started this one.  Returns the
   parent process pid, or zero.  */

static int
proc_parent_pid (procinfo *pi)
{
  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return 0;

  return pi->prstatus.pr_ppid;
}

/* Convert a target address (a.k.a. CORE_ADDR) into a host address
   (a.k.a void pointer)!  */

static void *
procfs_address_to_host_pointer (CORE_ADDR addr)
{
  gdbarch *arch = current_inferior ()->arch ();
  type *ptr_type = builtin_type (arch)->builtin_data_ptr;
  void *ptr;

  gdb_assert (sizeof (ptr) == ptr_type->length ());
  gdbarch_address_to_pointer (arch, ptr_type, (gdb_byte *) &ptr, addr);
  return ptr;
}

static int
proc_set_watchpoint (procinfo *pi, CORE_ADDR addr, int len, int wflags)
{
  struct {
    procfs_ctl_t cmd;
    char watch[sizeof (prwatch_t)];
  } arg;
  prwatch_t pwatch;

  /* NOTE: cagney/2003-02-01: Even more horrible hack.  Need to
     convert a target address into something that can be stored in a
     native data structure.  */
  pwatch.pr_vaddr  = (uintptr_t) procfs_address_to_host_pointer (addr);
  pwatch.pr_size   = len;
  pwatch.pr_wflags = wflags;
  arg.cmd = PCWATCH;
  memcpy (arg.watch, &pwatch, sizeof (prwatch_t));
  return (write (pi->ctl_fd, &arg, sizeof (arg)) == sizeof (arg));
}

/* =============== END, non-thread part of /proc  "MODULE" =============== */

/* =================== Thread "MODULE" =================== */

/* Returns the number of threads for the process.  */

static int
proc_get_nthreads (procinfo *pi)
{
  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return 0;

  /* Only works for the process procinfo, because the LWP procinfos do not
     get prstatus filled in.  */
  if (pi->tid != 0)	/* Find the parent process procinfo.  */
    pi = find_procinfo_or_die (pi->pid, 0);
  return pi->prstatus.pr_nlwp;
}

/* Return the ID of the thread that had an event of interest.
   (ie. the one that hit a breakpoint or other traced event).  All
   other things being equal, this should be the ID of a thread that is
   currently executing.  */

static int
proc_get_current_thread (procinfo *pi)
{
  /* Note: this should be applied to the root procinfo for the
     process, not to the procinfo for an LWP.  If applied to the
     procinfo for an LWP, it will simply return that LWP's ID.  In
     that case, find the parent process procinfo.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  if (!pi->status_valid)
    if (!proc_get_status (pi))
      return 0;

  return pi->prstatus.pr_lwp.pr_lwpid;
}

/* Discover the IDs of all the threads within the process, and create
   a procinfo for each of them (chained to the parent).  Returns
   non-zero for success, zero for failure.  */

static int
proc_delete_dead_threads (procinfo *parent, procinfo *thread, void *ignore)
{
  if (thread && parent)	/* sanity */
    {
      thread->status_valid = 0;
      if (!proc_get_status (thread))
	destroy_one_procinfo (&parent->thread_list, thread);
    }
  return 0;	/* keep iterating */
}

static int
proc_update_threads (procinfo *pi)
{
  char pathname[MAX_PROC_NAME_SIZE + 16];
  struct dirent *direntry;
  procinfo *thread;
  gdb_dir_up dirp;
  int lwpid;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  proc_iterate_over_threads (pi, proc_delete_dead_threads, NULL);

  /* Note: this brute-force method was originally devised for Unixware
     (support removed since), and will also work on Solaris 2.6 and
     2.7.  The original comment mentioned the existence of a much
     simpler and more elegant way to do this on Solaris, but didn't
     point out what that was.  */

  strcpy (pathname, pi->pathname);
  strcat (pathname, "/lwp");
  dirp.reset (opendir (pathname));
  if (dirp == NULL)
    proc_error (pi, "update_threads, opendir", __LINE__);

  while ((direntry = readdir (dirp.get ())) != NULL)
    if (direntry->d_name[0] != '.')		/* skip '.' and '..' */
      {
	lwpid = atoi (&direntry->d_name[0]);
	thread = create_procinfo (pi->pid, lwpid);
	if (thread == NULL)
	  proc_error (pi, "update_threads, create_procinfo", __LINE__);
      }
  pi->threads_valid = 1;
  return 1;
}

/* Given a pointer to a function, call that function once for each lwp
   in the procinfo list, until the function returns non-zero, in which
   event return the value returned by the function.

   Note: this function does NOT call update_threads.  If you want to
   discover new threads first, you must call that function explicitly.
   This function just makes a quick pass over the currently-known
   procinfos.

   PI is the parent process procinfo.  FUNC is the per-thread
   function.  PTR is an opaque parameter for function.  Returns the
   first non-zero return value from the callee, or zero.  */

static int
proc_iterate_over_threads (procinfo *pi,
			   int (*func) (procinfo *, procinfo *, void *),
			   void *ptr)
{
  procinfo *thread, *next;
  int retval = 0;

  /* We should never have to apply this operation to any procinfo
     except the one for the main process.  If that ever changes for
     any reason, then take out the following clause and replace it
     with one that makes sure the ctl_fd is open.  */

  if (pi->tid != 0)
    pi = find_procinfo_or_die (pi->pid, 0);

  for (thread = pi->thread_list; thread != NULL; thread = next)
    {
      next = thread->next;	/* In case thread is destroyed.  */
      retval = (*func) (pi, thread, ptr);
      if (retval != 0)
	break;
    }

  return retval;
}

/* =================== END, Thread "MODULE" =================== */

/* =================== END, /proc  "MODULE" =================== */

/* ===================  GDB  "MODULE" =================== */

/* Here are all of the gdb target vector functions and their
   friends.  */

static void do_attach (ptid_t ptid);
static void do_detach ();
static void proc_trace_syscalls_1 (procinfo *pi, int syscallnum,
				   int entry_or_exit, int mode, int from_tty);

/* Sets up the inferior to be debugged.  Registers to trace signals,
   hardware faults, and syscalls.  Note: does not set RLC flag: caller
   may want to customize that.  Returns zero for success (note!
   unlike most functions in this module); on failure, returns the LINE
   NUMBER where it failed!  */

static int
procfs_debug_inferior (procinfo *pi)
{
  fltset_t traced_faults;
  sigset_t traced_signals;
  sysset_t *traced_syscall_entries;
  sysset_t *traced_syscall_exits;
  int status;

  /* Register to trace hardware faults in the child.  */
  prfillset (&traced_faults);		/* trace all faults...  */
  prdelset  (&traced_faults, FLTPAGE);	/* except page fault.  */
  if (!proc_set_traced_faults  (pi, &traced_faults))
    return __LINE__;

  /* Initially, register to trace all signals in the child.  */
  prfillset (&traced_signals);
  if (!proc_set_traced_signals (pi, &traced_signals))
    return __LINE__;


  /* Register to trace the 'exit' system call (on entry).  */
  traced_syscall_entries = XNEW (sysset_t);
  premptyset (traced_syscall_entries);
  praddset (traced_syscall_entries, SYS_exit);
  praddset (traced_syscall_entries, SYS_lwp_exit);

  status = proc_set_traced_sysentry (pi, traced_syscall_entries);
  xfree (traced_syscall_entries);
  if (!status)
    return __LINE__;

  /* Method for tracing exec syscalls.  */
  traced_syscall_exits = XNEW (sysset_t);
  premptyset (traced_syscall_exits);
  praddset (traced_syscall_exits, SYS_execve);
  praddset (traced_syscall_exits, SYS_lwp_create);
  praddset (traced_syscall_exits, SYS_lwp_exit);

  status = proc_set_traced_sysexit (pi, traced_syscall_exits);
  xfree (traced_syscall_exits);
  if (!status)
    return __LINE__;

  return 0;
}

void
procfs_target::attach (const char *args, int from_tty)
{
  int   pid;

  pid = parse_pid_to_attach (args);

  if (pid == getpid ())
    error (_("Attaching GDB to itself is not a good idea..."));

  /* Push the target if needed, ensure it gets un-pushed it if attach fails.  */
  inferior *inf = current_inferior ();
  target_unpush_up unpusher;
  if (!inf->target_is_pushed (this))
    {
      inf->push_target (this);
      unpusher.reset (this);
    }

  target_announce_attach (from_tty, pid);

  do_attach (ptid_t (pid));

  /* Everything went fine, keep the target pushed.  */
  unpusher.release ();
}

void
procfs_target::detach (inferior *inf, int from_tty)
{
  target_announce_detach (from_tty);

  do_detach ();

  switch_to_no_thread ();
  detach_inferior (inf);
  maybe_unpush_target ();
}

static void
do_attach (ptid_t ptid)
{
  procinfo *pi;
  struct inferior *inf;
  int fail;
  int lwpid;

  pi = create_procinfo (ptid.pid (), 0);
  if (pi == NULL)
    perror (_("procfs: out of memory in 'attach'"));

  if (!open_procinfo_files (pi, FD_CTL))
    {
      int saved_errno = errno;
      std::string errmsg
	= string_printf ("procfs:%d -- do_attach: couldn't open /proc "
			 "file for process %d", __LINE__, ptid.pid ());
      errno = saved_errno;
      dead_procinfo (pi, errmsg.c_str (), NOKILL);
    }

  /* Stop the process (if it isn't already stopped).  */
  if (proc_flags (pi) & (PR_STOPPED | PR_ISTOP))
    {
      pi->was_stopped = 1;
      proc_prettyprint_why (proc_why (pi), proc_what (pi), 1);
    }
  else
    {
      pi->was_stopped = 0;
      /* Set the process to run again when we close it.  */
      if (!proc_set_run_on_last_close (pi))
	dead_procinfo (pi, "do_attach: couldn't set RLC.", NOKILL);

      /* Now stop the process.  */
      if (!proc_stop_process (pi))
	dead_procinfo (pi, "do_attach: couldn't stop the process.", NOKILL);
      pi->ignore_next_sigstop = 1;
    }
  /* Save some of the /proc state to be restored if we detach.  */
  if (!proc_get_traced_faults   (pi, &pi->saved_fltset))
    dead_procinfo (pi, "do_attach: couldn't save traced faults.", NOKILL);
  if (!proc_get_traced_signals  (pi, &pi->saved_sigset))
    dead_procinfo (pi, "do_attach: couldn't save traced signals.", NOKILL);
  if (!proc_get_traced_sysentry (pi, pi->saved_entryset))
    dead_procinfo (pi, "do_attach: couldn't save traced syscall entries.",
		   NOKILL);
  if (!proc_get_traced_sysexit  (pi, pi->saved_exitset))
    dead_procinfo (pi, "do_attach: couldn't save traced syscall exits.",
		   NOKILL);
  if (!proc_get_held_signals    (pi, &pi->saved_sighold))
    dead_procinfo (pi, "do_attach: couldn't save held signals.", NOKILL);

  fail = procfs_debug_inferior (pi);
  if (fail != 0)
    dead_procinfo (pi, "do_attach: failed in procfs_debug_inferior", NOKILL);

  inf = current_inferior ();
  inferior_appeared (inf, pi->pid);
  /* Let GDB know that the inferior was attached.  */
  inf->attach_flag = true;

  /* Create a procinfo for the current lwp.  */
  lwpid = proc_get_current_thread (pi);
  create_procinfo (pi->pid, lwpid);

  /* Add it to gdb's thread list.  */
  ptid = ptid_t (pi->pid, lwpid, 0);
  thread_info *thr = add_thread (&the_procfs_target, ptid);
  switch_to_thread (thr);
}

static void
do_detach ()
{
  procinfo *pi;

  /* Find procinfo for the main process.  */
  pi = find_procinfo_or_die (inferior_ptid.pid (),
			     0); /* FIXME: threads */

  if (!proc_set_traced_signals (pi, &pi->saved_sigset))
    proc_warn (pi, "do_detach, set_traced_signal", __LINE__);

  if (!proc_set_traced_faults (pi, &pi->saved_fltset))
    proc_warn (pi, "do_detach, set_traced_faults", __LINE__);

  if (!proc_set_traced_sysentry (pi, pi->saved_entryset))
    proc_warn (pi, "do_detach, set_traced_sysentry", __LINE__);

  if (!proc_set_traced_sysexit (pi, pi->saved_exitset))
    proc_warn (pi, "do_detach, set_traced_sysexit", __LINE__);

  if (!proc_set_held_signals (pi, &pi->saved_sighold))
    proc_warn (pi, "do_detach, set_held_signals", __LINE__);

  if (proc_flags (pi) & (PR_STOPPED | PR_ISTOP))
    if (!(pi->was_stopped)
	|| query (_("Was stopped when attached, make it runnable again? ")))
      {
	/* Clear any pending signal.  */
	if (!proc_clear_current_fault (pi))
	  proc_warn (pi, "do_detach, clear_current_fault", __LINE__);

	if (!proc_clear_current_signal (pi))
	  proc_warn (pi, "do_detach, clear_current_signal", __LINE__);

	if (!proc_set_run_on_last_close (pi))
	  proc_warn (pi, "do_detach, set_rlc", __LINE__);
      }

  destroy_procinfo (pi);
}

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.

   NOTE: Since the /proc interface cannot give us individual
   registers, we pay no attention to REGNUM, and just fetch them all.
   This results in the possibility that we will do unnecessarily many
   fetches, since we may be called repeatedly for individual
   registers.  So we cache the results, and mark the cache invalid
   when the process is resumed.  */

void
procfs_target::fetch_registers (struct regcache *regcache, int regnum)
{
  gdb_gregset_t *gregs;
  procinfo *pi;
  ptid_t ptid = regcache->ptid ();
  int pid = ptid.pid ();
  int tid = ptid.lwp ();
  struct gdbarch *gdbarch = regcache->arch ();

  pi = find_procinfo_or_die (pid, tid);

  if (pi == NULL)
    error (_("procfs: fetch_registers failed to find procinfo for %s"),
	   target_pid_to_str (ptid).c_str ());

  gregs = proc_get_gregs (pi);
  if (gregs == NULL)
    proc_error (pi, "fetch_registers, get_gregs", __LINE__);

  supply_gregset (regcache, (const gdb_gregset_t *) gregs);

  if (gdbarch_fp0_regnum (gdbarch) >= 0) /* Do we have an FPU?  */
    {
      gdb_fpregset_t *fpregs;

      if ((regnum >= 0 && regnum < gdbarch_fp0_regnum (gdbarch))
	  || regnum == gdbarch_pc_regnum (gdbarch)
	  || regnum == gdbarch_sp_regnum (gdbarch))
	return;			/* Not a floating point register.  */

      fpregs = proc_get_fpregs (pi);
      if (fpregs == NULL)
	proc_error (pi, "fetch_registers, get_fpregs", __LINE__);

      supply_fpregset (regcache, (const gdb_fpregset_t *) fpregs);
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.

   NOTE: Since the /proc interface will not read individual registers,
   we will cache these requests until the process is resumed, and only
   then write them back to the inferior process.

   FIXME: is that a really bad idea?  Have to think about cases where
   writing one register might affect the value of others, etc.  */

void
procfs_target::store_registers (struct regcache *regcache, int regnum)
{
  gdb_gregset_t *gregs;
  procinfo *pi;
  ptid_t ptid = regcache->ptid ();
  int pid = ptid.pid ();
  int tid = ptid.lwp ();
  struct gdbarch *gdbarch = regcache->arch ();

  pi = find_procinfo_or_die (pid, tid);

  if (pi == NULL)
    error (_("procfs: store_registers: failed to find procinfo for %s"),
	   target_pid_to_str (ptid).c_str ());

  gregs = proc_get_gregs (pi);
  if (gregs == NULL)
    proc_error (pi, "store_registers, get_gregs", __LINE__);

  fill_gregset (regcache, gregs, regnum);
  if (!proc_set_gregs (pi))
    proc_error (pi, "store_registers, set_gregs", __LINE__);

  if (gdbarch_fp0_regnum (gdbarch) >= 0) /* Do we have an FPU?  */
    {
      gdb_fpregset_t *fpregs;

      if ((regnum >= 0 && regnum < gdbarch_fp0_regnum (gdbarch))
	  || regnum == gdbarch_pc_regnum (gdbarch)
	  || regnum == gdbarch_sp_regnum (gdbarch))
	return;			/* Not a floating point register.  */

      fpregs = proc_get_fpregs (pi);
      if (fpregs == NULL)
	proc_error (pi, "store_registers, get_fpregs", __LINE__);

      fill_fpregset (regcache, fpregs, regnum);
      if (!proc_set_fpregs (pi))
	proc_error (pi, "store_registers, set_fpregs", __LINE__);
    }
}

/* Retrieve the next stop event from the child process.  If child has
   not stopped yet, wait for it to stop.  Translate /proc eventcodes
   (or possibly wait eventcodes) into gdb internal event codes.
   Returns the id of process (and possibly thread) that incurred the
   event.  Event codes are returned through a pointer parameter.  */

ptid_t
procfs_target::wait (ptid_t ptid, struct target_waitstatus *status,
		     target_wait_flags options)
{
  /* First cut: loosely based on original version 2.1.  */
  procinfo *pi;
  int       wstat;
  int       temp_tid;
  ptid_t    retval, temp_ptid;
  int       why, what, flags;
  int       retry = 0;

wait_again:

  retry++;
  wstat    = 0;
  retval   = ptid_t (-1);

  /* Find procinfo for main process.  */

  /* procfs_target currently only supports one inferior.  */
  inferior *inf = current_inferior ();

  pi = find_procinfo_or_die (inf->pid, 0);
  if (pi)
    {
      /* We must assume that the status is stale now...  */
      pi->status_valid = 0;
      pi->gregs_valid  = 0;
      pi->fpregs_valid = 0;

#if 0	/* just try this out...  */
      flags = proc_flags (pi);
      why   = proc_why (pi);
      if ((flags & PR_STOPPED) && (why == PR_REQUESTED))
	pi->status_valid = 0;	/* re-read again, IMMEDIATELY...  */
#endif
      /* If child is not stopped, wait for it to stop.  */
      if (!(proc_flags (pi) & (PR_STOPPED | PR_ISTOP))
	  && !proc_wait_for_stop (pi))
	{
	  /* wait_for_stop failed: has the child terminated?  */
	  if (errno == ENOENT)
	    {
	      int wait_retval;

	      /* /proc file not found; presumably child has terminated.  */
	      wait_retval = ::wait (&wstat); /* "wait" for the child's exit.  */

	      /* Wrong child?  */
	      if (wait_retval != inf->pid)
		error (_("procfs: couldn't stop "
			 "process %d: wait returned %d."),
		       inf->pid, wait_retval);
	      /* FIXME: might I not just use waitpid?
		 Or try find_procinfo to see if I know about this child?  */
	      retval = ptid_t (wait_retval);
	    }
	  else if (errno == EINTR)
	    goto wait_again;
	  else
	    {
	      /* Unknown error from wait_for_stop.  */
	      proc_error (pi, "target_wait (wait_for_stop)", __LINE__);
	    }
	}
      else
	{
	  /* This long block is reached if either:
	     a) the child was already stopped, or
	     b) we successfully waited for the child with wait_for_stop.
	     This block will analyze the /proc status, and translate it
	     into a waitstatus for GDB.

	     If we actually had to call wait because the /proc file
	     is gone (child terminated), then we skip this block,
	     because we already have a waitstatus.  */

	  flags = proc_flags (pi);
	  why   = proc_why (pi);
	  what  = proc_what (pi);

	  if (flags & (PR_STOPPED | PR_ISTOP))
	    {
	      /* If it's running async (for single_thread control),
		 set it back to normal again.  */
	      if (flags & PR_ASYNC)
		if (!proc_unset_async (pi))
		  proc_error (pi, "target_wait, unset_async", __LINE__);

	      if (info_verbose)
		proc_prettyprint_why (why, what, 1);

	      /* The 'pid' we will return to GDB is composed of
		 the process ID plus the lwp ID.  */
	      retval = ptid_t (pi->pid, proc_get_current_thread (pi), 0);

	      switch (why) {
	      case PR_SIGNALLED:
		wstat = (what << 8) | 0177;
		break;
	      case PR_SYSENTRY:
		if (what == SYS_lwp_exit)
		  {
		    delete_thread (this->find_thread (retval));
		    proc_resume (pi, ptid, 0, GDB_SIGNAL_0);
		    goto wait_again;
		  }
		else if (what == SYS_exit)
		  {
		    /* Handle SYS_exit call only.  */
		    /* Stopped at entry to SYS_exit.
		       Make it runnable, resume it, then use
		       the wait system call to get its exit code.
		       Proc_run_process always clears the current
		       fault and signal.
		       Then return its exit status.  */
		    pi->status_valid = 0;
		    wstat = 0;
		    /* FIXME: what we should do is return
		       TARGET_WAITKIND_SPURIOUS.  */
		    if (!proc_run_process (pi, 0, 0))
		      proc_error (pi, "target_wait, run_process", __LINE__);

		    if (inf->attach_flag)
		      {
			/* Don't call wait: simulate waiting for exit,
			   return a "success" exit code.  Bogus: what if
			   it returns something else?  */
			wstat = 0;
			retval = ptid_t (inf->pid);  /* ? ? ? */
		      }
		    else
		      {
			int temp = ::wait (&wstat);

			/* FIXME: shouldn't I make sure I get the right
			   event from the right process?  If (for
			   instance) I have killed an earlier inferior
			   process but failed to clean up after it
			   somehow, I could get its termination event
			   here.  */

			/* If wait returns -1, that's what we return
			   to GDB.  */
			if (temp < 0)
			  retval = ptid_t (temp);
		      }
		  }
		else
		  {
		    gdb_printf (_("procfs: trapped on entry to "));
		    proc_prettyprint_syscall (proc_what (pi), 0);
		    gdb_printf ("\n");

		    long i, nsysargs, *sysargs;

		    nsysargs = proc_nsysarg (pi);
		    sysargs  = proc_sysargs (pi);

		    if (nsysargs > 0 && sysargs != NULL)
		      {
			gdb_printf (_("%ld syscall arguments:\n"),
				    nsysargs);
			for (i = 0; i < nsysargs; i++)
			  gdb_printf ("#%ld: 0x%08lx\n",
				      i, sysargs[i]);
		      }

		    proc_resume (pi, ptid, 0, GDB_SIGNAL_0);
		    goto wait_again;
		  }
		break;
	      case PR_SYSEXIT:
		if (what == SYS_execve)
		  {
		    /* Hopefully this is our own "fork-child" execing
		       the real child.  Hoax this event into a trap, and
		       GDB will see the child about to execute its start
		       address.  */
		    wstat = (SIGTRAP << 8) | 0177;
		  }
		else if (what == SYS_lwp_create)
		  {
		    /* This syscall is somewhat like fork/exec.  We
		       will get the event twice: once for the parent
		       LWP, and once for the child.  We should already
		       know about the parent LWP, but the child will
		       be new to us.  So, whenever we get this event,
		       if it represents a new thread, simply add the
		       thread to the list.  */

		    /* If not in procinfo list, add it.  */
		    temp_tid = proc_get_current_thread (pi);
		    if (!find_procinfo (pi->pid, temp_tid))
		      create_procinfo  (pi->pid, temp_tid);

		    temp_ptid = ptid_t (pi->pid, temp_tid, 0);
		    /* If not in GDB's thread list, add it.  */
		    if (!in_thread_list (this, temp_ptid))
		      add_thread (this, temp_ptid);

		    proc_resume (pi, ptid, 0, GDB_SIGNAL_0);
		    goto wait_again;
		  }
		else if (what == SYS_lwp_exit)
		  {
		    delete_thread (this->find_thread (retval));
		    status->set_spurious ();
		    return retval;
		  }
		else
		  {
		    gdb_printf (_("procfs: trapped on exit from "));
		    proc_prettyprint_syscall (proc_what (pi), 0);
		    gdb_printf ("\n");

		    long i, nsysargs, *sysargs;

		    nsysargs = proc_nsysarg (pi);
		    sysargs = proc_sysargs (pi);

		    if (nsysargs > 0 && sysargs != NULL)
		      {
			gdb_printf (_("%ld syscall arguments:\n"),
				    nsysargs);
			for (i = 0; i < nsysargs; i++)
			  gdb_printf ("#%ld: 0x%08lx\n",
				      i, sysargs[i]);
		      }

		    proc_resume (pi, ptid, 0, GDB_SIGNAL_0);
		    goto wait_again;
		  }
		break;
	      case PR_REQUESTED:
#if 0	/* FIXME */
		wstat = (SIGSTOP << 8) | 0177;
		break;
#else
		if (retry < 5)
		  {
		    gdb_printf (_("Retry #%d:\n"), retry);
		    pi->status_valid = 0;
		    goto wait_again;
		  }
		else
		  {
		    /* If not in procinfo list, add it.  */
		    temp_tid = proc_get_current_thread (pi);
		    if (!find_procinfo (pi->pid, temp_tid))
		      create_procinfo  (pi->pid, temp_tid);

		    /* If not in GDB's thread list, add it.  */
		    temp_ptid = ptid_t (pi->pid, temp_tid, 0);
		    if (!in_thread_list (this, temp_ptid))
		      add_thread (this, temp_ptid);

		    status->set_stopped (GDB_SIGNAL_0);
		    return retval;
		  }
#endif
	      case PR_JOBCONTROL:
		wstat = (what << 8) | 0177;
		break;
	      case PR_FAULTED:
		{
		  int signo = pi->prstatus.pr_lwp.pr_info.si_signo;
		  if (signo != 0)
		    wstat = (signo << 8) | 0177;
		}
		break;
	      default:	/* switch (why) unmatched */
		gdb_printf ("procfs:%d -- ", __LINE__);
		gdb_printf (_("child stopped for unknown reason:\n"));
		proc_prettyprint_why (why, what, 1);
		error (_("... giving up..."));
		break;
	      }
	      /* Got this far without error: If retval isn't in the
		 threads database, add it.  */
	      if (retval.pid () > 0
		  && !in_thread_list (this, retval))
		{
		  /* We have a new thread.  We need to add it both to
		     GDB's list and to our own.  If we don't create a
		     procinfo, resume may be unhappy later.  */
		  add_thread (this, retval);
		  if (find_procinfo (retval.pid (),
				     retval.lwp ()) == NULL)
		    create_procinfo (retval.pid (),
				     retval.lwp ());
		}
	    }
	  else	/* Flags do not indicate STOPPED.  */
	    {
	      /* surely this can't happen...  */
	      gdb_printf ("procfs:%d -- process not stopped.\n",
			  __LINE__);
	      proc_prettyprint_flags (flags, 1);
	      error (_("procfs: ...giving up..."));
	    }
	}

      if (status)
	*status = host_status_to_waitstatus (wstat);
    }

  return retval;
}

/* Perform a partial transfer to/from the specified object.  For
   memory transfers, fall back to the old memory xfer functions.  */

enum target_xfer_status
procfs_target::xfer_partial (enum target_object object,
			     const char *annex, gdb_byte *readbuf,
			     const gdb_byte *writebuf, ULONGEST offset,
			     ULONGEST len, ULONGEST *xfered_len)
{
  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
      return procfs_xfer_memory (readbuf, writebuf, offset, len, xfered_len);

    case TARGET_OBJECT_AUXV:
      return memory_xfer_auxv (this, object, annex, readbuf, writebuf,
			       offset, len, xfered_len);

    default:
      return this->beneath ()->xfer_partial (object, annex,
					     readbuf, writebuf, offset, len,
					     xfered_len);
    }
}

/* Helper for procfs_xfer_partial that handles memory transfers.
   Arguments are like target_xfer_partial.  */

static enum target_xfer_status
procfs_xfer_memory (gdb_byte *readbuf, const gdb_byte *writebuf,
		    ULONGEST memaddr, ULONGEST len, ULONGEST *xfered_len)
{
  procinfo *pi;
  int nbytes;

  /* Find procinfo for main process.  */
  pi = find_procinfo_or_die (inferior_ptid.pid (), 0);
  if (pi->as_fd == 0 && open_procinfo_files (pi, FD_AS) == 0)
    {
      proc_warn (pi, "xfer_memory, open_proc_files", __LINE__);
      return TARGET_XFER_E_IO;
    }

  if (lseek (pi->as_fd, (off_t) memaddr, SEEK_SET) != (off_t) memaddr)
    return TARGET_XFER_E_IO;

  if (writebuf != NULL)
    {
      PROCFS_NOTE ("write memory:\n");
      nbytes = write (pi->as_fd, writebuf, len);
    }
  else
    {
      PROCFS_NOTE ("read  memory:\n");
      nbytes = read (pi->as_fd, readbuf, len);
    }
  if (nbytes <= 0)
    return TARGET_XFER_E_IO;
  *xfered_len = nbytes;
  return TARGET_XFER_OK;
}

/* Called by target_resume before making child runnable.  Mark cached
   registers and status's invalid.  If there are "dirty" caches that
   need to be written back to the child process, do that.

   File descriptors are also cached.  As they are a limited resource,
   we cannot hold onto them indefinitely.  However, as they are
   expensive to open, we don't want to throw them away
   indiscriminately either.  As a compromise, we will keep the file
   descriptors for the parent process, but discard any file
   descriptors we may have accumulated for the threads.

   As this function is called by iterate_over_threads, it always
   returns zero (so that iterate_over_threads will keep
   iterating).  */

static int
invalidate_cache (procinfo *parent, procinfo *pi, void *ptr)
{
  /* About to run the child; invalidate caches and do any other
     cleanup.  */

  if (parent != NULL)
    {
      /* The presence of a parent indicates that this is an LWP.
	 Close any file descriptors that it might have open.
	 We don't do this to the master (parent) procinfo.  */

      close_procinfo_files (pi);
    }
  pi->gregs_valid   = 0;
  pi->fpregs_valid  = 0;
  pi->status_valid  = 0;
  pi->threads_valid = 0;

  return 0;
}

/* Make child process PI runnable.

   If STEP is true, then arrange for the child to stop again after
   executing a single instruction.  SCOPE_PTID, STEP and SIGNO are
   like in the target_resume interface.  */

static void
proc_resume (procinfo *pi, ptid_t scope_ptid, int step, enum gdb_signal signo)
{
  procinfo *thread;
  int native_signo;

  /* FIXME: Check/reword.  */

  /* prrun.prflags |= PRCFAULT;    clear current fault.
     PRCFAULT may be replaced by a PCCFAULT call (proc_clear_current_fault)
     This basically leaves PRSTEP and PRCSIG.
     PRCSIG is like PCSSIG (proc_clear_current_signal).
     So basically PR_STEP is the sole argument that must be passed
     to proc_run_process.  */

  errno = 0;

  /* Convert signal to host numbering.  */
  if (signo == 0 || (signo == GDB_SIGNAL_STOP && pi->ignore_next_sigstop))
    native_signo = 0;
  else
    native_signo = gdb_signal_to_host (signo);

  pi->ignore_next_sigstop = 0;

  /* Running the process voids all cached registers and status.  */
  /* Void the threads' caches first.  */
  proc_iterate_over_threads (pi, invalidate_cache, NULL);
  /* Void the process procinfo's caches.  */
  invalidate_cache (NULL, pi, NULL);

  if (scope_ptid.pid () != -1)
    {
      /* Resume a specific thread, presumably suppressing the
	 others.  */
      thread = find_procinfo (scope_ptid.pid (), scope_ptid.lwp ());
      if (thread != NULL)
	{
	  if (thread->tid != 0)
	    {
	      /* We're to resume a specific thread, and not the
		 others.  Set the child process's PR_ASYNC flag.  */
	      if (!proc_set_async (pi))
		proc_error (pi, "target_resume, set_async", __LINE__);
	      pi = thread;	/* Substitute the thread's procinfo
				   for run.  */
	    }
	}
    }

  if (!proc_run_process (pi, step, native_signo))
    {
      if (errno == EBUSY)
	warning (_("resume: target already running.  "
		   "Pretend to resume, and hope for the best!"));
      else
	proc_error (pi, "target_resume", __LINE__);
    }
}

/* Implementation of target_ops::resume.  */

void
procfs_target::resume (ptid_t scope_ptid, int step, enum gdb_signal signo)
{
  /* Find procinfo for main process.  */
  procinfo *pi = find_procinfo_or_die (inferior_ptid.pid (), 0);

  proc_resume (pi, scope_ptid, step, signo);
}

/* Set up to trace signals in the child process.  */

void
procfs_target::pass_signals (gdb::array_view<const unsigned char> pass_signals)
{
  sigset_t signals;
  procinfo *pi = find_procinfo_or_die (inferior_ptid.pid (), 0);
  int signo;

  prfillset (&signals);

  for (signo = 0; signo < NSIG; signo++)
    {
      int target_signo = gdb_signal_from_host (signo);
      if (target_signo < pass_signals.size () && pass_signals[target_signo])
	prdelset (&signals, signo);
    }

  if (!proc_set_traced_signals (pi, &signals))
    proc_error (pi, "pass_signals", __LINE__);
}

/* Print status information about the child process.  */

void
procfs_target::files_info ()
{
  struct inferior *inf = current_inferior ();

  gdb_printf (_("\tUsing the running image of %s %s via /proc.\n"),
	      inf->attach_flag? "attached": "child",
	      target_pid_to_str (ptid_t (inf->pid)).c_str ());
}

/* Make it die.  Wait for it to die.  Clean up after it.  Note: this
   should only be applied to the real process, not to an LWP, because
   of the check for parent-process.  If we need this to work for an
   LWP, it needs some more logic.  */

static void
unconditionally_kill_inferior (procinfo *pi)
{
  int parent_pid;

  parent_pid = proc_parent_pid (pi);
  if (!proc_kill (pi, SIGKILL))
    proc_error (pi, "unconditionally_kill, proc_kill", __LINE__);
  destroy_procinfo (pi);

  /* If pi is GDB's child, wait for it to die.  */
  if (parent_pid == getpid ())
    /* FIXME: should we use waitpid to make sure we get the right event?
       Should we check the returned event?  */
    {
#if 0
      int status, ret;

      ret = waitpid (pi->pid, &status, 0);
#else
      wait (NULL);
#endif
    }
}

/* We're done debugging it, and we want it to go away.  Then we want
   GDB to forget all about it.  */

void
procfs_target::kill ()
{
  if (inferior_ptid != null_ptid) /* ? */
    {
      /* Find procinfo for main process.  */
      procinfo *pi = find_procinfo (inferior_ptid.pid (), 0);

      if (pi)
	unconditionally_kill_inferior (pi);
      target_mourn_inferior (inferior_ptid);
    }
}

/* Forget we ever debugged this thing!  */

void
procfs_target::mourn_inferior ()
{
  procinfo *pi;

  if (inferior_ptid != null_ptid)
    {
      /* Find procinfo for main process.  */
      pi = find_procinfo (inferior_ptid.pid (), 0);
      if (pi)
	destroy_procinfo (pi);
    }

  generic_mourn_inferior ();

  maybe_unpush_target ();
}

/* When GDB forks to create a runnable inferior process, this function
   is called on the parent side of the fork.  It's job is to do
   whatever is necessary to make the child ready to be debugged, and
   then wait for the child to synchronize.  */

void
procfs_target::procfs_init_inferior (int pid)
{
  procinfo *pi;
  int fail;
  int lwpid;

  pi = create_procinfo (pid, 0);
  if (pi == NULL)
    perror (_("procfs: out of memory in 'init_inferior'"));

  if (!open_procinfo_files (pi, FD_CTL))
    proc_error (pi, "init_inferior, open_proc_files", __LINE__);

  /*
    xmalloc			// done
    open_procinfo_files		// done
    link list			// done
    prfillset (trace)
    procfs_notice_signals
    prfillset (fault)
    prdelset (FLTPAGE)
    */

  /* If not stopped yet, wait for it to stop.  */
  if (!(proc_flags (pi) & PR_STOPPED) && !(proc_wait_for_stop (pi)))
    dead_procinfo (pi, "init_inferior: wait_for_stop failed", KILL);

  /* Save some of the /proc state to be restored if we detach.  */
  /* FIXME: Why?  In case another debugger was debugging it?
     We're it's parent, for Ghu's sake!  */
  if (!proc_get_traced_signals  (pi, &pi->saved_sigset))
    proc_error (pi, "init_inferior, get_traced_signals", __LINE__);
  if (!proc_get_held_signals    (pi, &pi->saved_sighold))
    proc_error (pi, "init_inferior, get_held_signals", __LINE__);
  if (!proc_get_traced_faults   (pi, &pi->saved_fltset))
    proc_error (pi, "init_inferior, get_traced_faults", __LINE__);
  if (!proc_get_traced_sysentry (pi, pi->saved_entryset))
    proc_error (pi, "init_inferior, get_traced_sysentry", __LINE__);
  if (!proc_get_traced_sysexit  (pi, pi->saved_exitset))
    proc_error (pi, "init_inferior, get_traced_sysexit", __LINE__);

  fail = procfs_debug_inferior (pi);
  if (fail != 0)
    proc_error (pi, "init_inferior (procfs_debug_inferior)", fail);

  /* FIXME: logically, we should really be turning OFF run-on-last-close,
     and possibly even turning ON kill-on-last-close at this point.  But
     I can't make that change without careful testing which I don't have
     time to do right now...  */
  /* Turn on run-on-last-close flag so that the child
     will die if GDB goes away for some reason.  */
  if (!proc_set_run_on_last_close (pi))
    proc_error (pi, "init_inferior, set_RLC", __LINE__);

  /* We now have have access to the lwpid of the main thread/lwp.  */
  lwpid = proc_get_current_thread (pi);

  /* Create a procinfo for the main lwp.  */
  create_procinfo (pid, lwpid);

  /* We already have a main thread registered in the thread table at
     this point, but it didn't have any lwp info yet.  Notify the core
     about it.  This changes inferior_ptid as well.  */
  thread_change_ptid (this, ptid_t (pid), ptid_t (pid, lwpid, 0));

  gdb_startup_inferior (pid, START_INFERIOR_TRAPS_EXPECTED);
}

/* When GDB forks to create a new process, this function is called on
   the child side of the fork before GDB exec's the user program.  Its
   job is to make the child minimally debuggable, so that the parent
   GDB process can connect to the child and take over.  This function
   should do only the minimum to make that possible, and to
   synchronize with the parent process.  The parent process should
   take care of the details.  */

static void
procfs_set_exec_trap (void)
{
  /* This routine called on the child side (inferior side)
     after GDB forks the inferior.  It must use only local variables,
     because it may be sharing data space with its parent.  */

  procinfo *pi;
  sysset_t *exitset;

  pi = create_procinfo (getpid (), 0);
  if (pi == NULL)
    perror_with_name (_("procfs: create_procinfo failed in child"));

  if (open_procinfo_files (pi, FD_CTL) == 0)
    {
      proc_warn (pi, "set_exec_trap, open_proc_files", __LINE__);
      gdb_flush (gdb_stderr);
      /* No need to call "dead_procinfo", because we're going to
	 exit.  */
      _exit (127);
    }

  exitset = XNEW (sysset_t);
  premptyset (exitset);
  praddset (exitset, SYS_execve);

  if (!proc_set_traced_sysexit (pi, exitset))
    {
      proc_warn (pi, "set_exec_trap, set_traced_sysexit", __LINE__);
      gdb_flush (gdb_stderr);
      _exit (127);
    }

  /* FIXME: should this be done in the parent instead?  */
  /* Turn off inherit on fork flag so that all grand-children
     of gdb start with tracing flags cleared.  */
  if (!proc_unset_inherit_on_fork (pi))
    proc_warn (pi, "set_exec_trap, unset_inherit", __LINE__);

  /* Turn off run on last close flag, so that the child process
     cannot run away just because we close our handle on it.
     We want it to wait for the parent to attach.  */
  if (!proc_unset_run_on_last_close (pi))
    proc_warn (pi, "set_exec_trap, unset_RLC", __LINE__);

  /* FIXME: No need to destroy the procinfo --
     we have our own address space, and we're about to do an exec!  */
  /*destroy_procinfo (pi);*/
}

/* Dummy function to be sure fork_inferior uses fork(2) and not vfork(2).
   This avoids a possible deadlock gdb and its vfork'ed child.  */
static void
procfs_pre_trace (void)
{
}

/* This function is called BEFORE gdb forks the inferior process.  Its
   only real responsibility is to set things up for the fork, and tell
   GDB which two functions to call after the fork (one for the parent,
   and one for the child).

   This function does a complicated search for a unix shell program,
   which it then uses to parse arguments and environment variables to
   be sent to the child.  I wonder whether this code could not be
   abstracted out and shared with other unix targets such as
   inf-ptrace?  */

void
procfs_target::create_inferior (const char *exec_file,
				const std::string &allargs,
				char **env, int from_tty)
{
  const char *shell_file = get_shell ();
  char *tryname;
  int pid;

  if (strchr (shell_file, '/') == NULL)
    {

      /* We will be looking down the PATH to find shell_file.  If we
	 just do this the normal way (via execlp, which operates by
	 attempting an exec for each element of the PATH until it
	 finds one which succeeds), then there will be an exec for
	 each failed attempt, each of which will cause a PR_SYSEXIT
	 stop, and we won't know how to distinguish the PR_SYSEXIT's
	 for these failed execs with the ones for successful execs
	 (whether the exec has succeeded is stored at that time in the
	 carry bit or some such architecture-specific and
	 non-ABI-specified place).

	 So I can't think of anything better than to search the PATH
	 now.  This has several disadvantages: (1) There is a race
	 condition; if we find a file now and it is deleted before we
	 exec it, we lose, even if the deletion leaves a valid file
	 further down in the PATH, (2) there is no way to know exactly
	 what an executable (in the sense of "capable of being
	 exec'd") file is.  Using access() loses because it may lose
	 if the caller is the superuser; failing to use it loses if
	 there are ACLs or some such.  */

      const char *p;
      const char *p1;
      /* FIXME-maybe: might want "set path" command so user can change what
	 path is used from within GDB.  */
      const char *path = getenv ("PATH");
      int len;
      struct stat statbuf;

      if (path == NULL)
	path = "/bin:/usr/bin";

      tryname = (char *) alloca (strlen (path) + strlen (shell_file) + 2);
      for (p = path; p != NULL; p = p1 ? p1 + 1: NULL)
	{
	  p1 = strchr (p, ':');
	  if (p1 != NULL)
	    len = p1 - p;
	  else
	    len = strlen (p);
	  memcpy (tryname, p, len);
	  tryname[len] = '\0';
	  strcat (tryname, "/");
	  strcat (tryname, shell_file);
	  if (access (tryname, X_OK) < 0)
	    continue;
	  if (stat (tryname, &statbuf) < 0)
	    continue;
	  if (!S_ISREG (statbuf.st_mode))
	    /* We certainly need to reject directories.  I'm not quite
	       as sure about FIFOs, sockets, etc., but I kind of doubt
	       that people want to exec() these things.  */
	    continue;
	  break;
	}
      if (p == NULL)
	/* Not found.  This must be an error rather than merely passing
	   the file to execlp(), because execlp() would try all the
	   exec()s, causing GDB to get confused.  */
	error (_("procfs:%d -- Can't find shell %s in PATH"),
	       __LINE__, shell_file);

      shell_file = tryname;
    }

  inferior *inf = current_inferior ();
  if (!inf->target_is_pushed (this))
    inf->push_target (this);

  pid = fork_inferior (exec_file, allargs, env, procfs_set_exec_trap,
		       NULL, procfs_pre_trace, shell_file, NULL);

  /* We have something that executes now.  We'll be running through
     the shell at this point (if startup-with-shell is true), but the
     pid shouldn't change.  */
  thread_info *thr = add_thread_silent (this, ptid_t (pid));
  switch_to_thread (thr);

  procfs_init_inferior (pid);
}

/* Callback for update_thread_list.  Calls "add_thread".  */

static int
procfs_notice_thread (procinfo *pi, procinfo *thread, void *ptr)
{
  ptid_t gdb_threadid = ptid_t (pi->pid, thread->tid, 0);

  thread_info *thr = the_procfs_target.find_thread (gdb_threadid);
  if (thr == NULL || thr->state == THREAD_EXITED)
    add_thread (&the_procfs_target, gdb_threadid);

  return 0;
}

/* Query all the threads that the target knows about, and give them
   back to GDB to add to its list.  */

void
procfs_target::update_thread_list ()
{
  procinfo *pi;

  prune_threads ();

  /* Find procinfo for main process.  */
  pi = find_procinfo_or_die (inferior_ptid.pid (), 0);
  proc_update_threads (pi);
  proc_iterate_over_threads (pi, procfs_notice_thread, NULL);
}

/* Return true if the thread is still 'alive'.  This guy doesn't
   really seem to be doing his job.  Got to investigate how to tell
   when a thread is really gone.  */

bool
procfs_target::thread_alive (ptid_t ptid)
{
  int proc, thread;
  procinfo *pi;

  proc    = ptid.pid ();
  thread  = ptid.lwp ();
  /* If I don't know it, it ain't alive!  */
  pi = find_procinfo (proc, thread);
  if (pi == NULL)
    return false;

  /* If I can't get its status, it ain't alive!
     What's more, I need to forget about it!  */
  if (!proc_get_status (pi))
    {
      destroy_procinfo (pi);
      return false;
    }
  /* I couldn't have got its status if it weren't alive, so it's
     alive.  */
  return true;
}

/* Convert PTID to a string.  */

std::string
procfs_target::pid_to_str (ptid_t ptid)
{
  if (ptid.lwp () == 0)
    return string_printf ("process %d", ptid.pid ());
  else
    return string_printf ("LWP %ld", ptid.lwp ());
}

/* Accepts an integer PID; Returns a string representing a file that
   can be opened to get the symbols for the child process.  */

const char *
procfs_target::pid_to_exec_file (int pid)
{
  static char buf[PATH_MAX];
  char name[PATH_MAX];

  /* Solaris 11 introduced /proc/<proc-id>/execname.  */
  xsnprintf (name, sizeof (name), "/proc/%d/execname", pid);
  scoped_fd fd (gdb_open_cloexec (name, O_RDONLY, 0));
  if (fd.get () < 0 || read (fd.get (), buf, PATH_MAX - 1) < 0)
    {
      /* If that fails, fall back to /proc/<proc-id>/path/a.out introduced in
	 Solaris 10.  */
      ssize_t len;

      xsnprintf (name, sizeof (name), "/proc/%d/path/a.out", pid);
      len = readlink (name, buf, PATH_MAX - 1);
      if (len <= 0)
	strcpy (buf, name);
      else
	buf[len] = '\0';
    }

  return buf;
}

/* Insert a watchpoint.  */

static int
procfs_set_watchpoint (ptid_t ptid, CORE_ADDR addr, int len, int rwflag,
		       int after)
{
  int       pflags = 0;
  procinfo *pi;

  pi = find_procinfo_or_die (ptid.pid () == -1 ?
			     inferior_ptid.pid () : ptid.pid (),
			     0);

  /* Translate from GDB's flags to /proc's.  */
  if (len > 0)	/* len == 0 means delete watchpoint.  */
    {
      switch (rwflag) {		/* FIXME: need an enum!  */
      case hw_write:		/* default watchpoint (write) */
	pflags = WA_WRITE;
	break;
      case hw_read:		/* read watchpoint */
	pflags = WA_READ;
	break;
      case hw_access:		/* access watchpoint */
	pflags = WA_READ | WA_WRITE;
	break;
      case hw_execute:		/* execution HW breakpoint */
	pflags = WA_EXEC;
	break;
      default:			/* Something weird.  Return error.  */
	return -1;
      }
      if (after)		/* Stop after r/w access is completed.  */
	pflags |= WA_TRAPAFTER;
    }

  if (!proc_set_watchpoint (pi, addr, len, pflags))
    {
      if (errno == E2BIG)	/* Typical error for no resources.  */
	return -1;		/* fail */
      /* GDB may try to remove the same watchpoint twice.
	 If a remove request returns no match, don't error.  */
      if (errno == ESRCH && len == 0)
	return 0;		/* ignore */
      proc_error (pi, "set_watchpoint", __LINE__);
    }
  return 0;
}

/* Return non-zero if we can set a hardware watchpoint of type TYPE.  TYPE
   is one of bp_hardware_watchpoint, bp_read_watchpoint, bp_write_watchpoint,
   or bp_hardware_watchpoint.  CNT is the number of watchpoints used so
   far.  */

int
procfs_target::can_use_hw_breakpoint (enum bptype type, int cnt, int othertype)
{
  /* Due to the way that proc_set_watchpoint() is implemented, host
     and target pointers must be of the same size.  If they are not,
     we can't use hardware watchpoints.  This limitation is due to the
     fact that proc_set_watchpoint() calls
     procfs_address_to_host_pointer(); a close inspection of
     procfs_address_to_host_pointer will reveal that an internal error
     will be generated when the host and target pointer sizes are
     different.  */
  struct type *ptr_type
    = builtin_type (current_inferior ()->arch ())->builtin_data_ptr;

  if (sizeof (void *) != ptr_type->length ())
    return 0;

  /* Other tests here???  */

  return 1;
}

/* Returns non-zero if process is stopped on a hardware watchpoint
   fault, else returns zero.  */

bool
procfs_target::stopped_by_watchpoint ()
{
  procinfo *pi;

  pi = find_procinfo_or_die (inferior_ptid.pid (), 0);

  if (proc_flags (pi) & (PR_STOPPED | PR_ISTOP))
    if (proc_why (pi) == PR_FAULTED)
      if (proc_what (pi) == FLTWATCH)
	return true;
  return false;
}

/* Returns 1 if the OS knows the position of the triggered watchpoint,
   and sets *ADDR to that address.  Returns 0 if OS cannot report that
   address.  This function is only called if
   procfs_stopped_by_watchpoint returned 1, thus no further checks are
   done.  The function also assumes that ADDR is not NULL.  */

bool
procfs_target::stopped_data_address (CORE_ADDR *addr)
{
  procinfo *pi;

  pi = find_procinfo_or_die (inferior_ptid.pid (), 0);
  return proc_watchpoint_address (pi, addr);
}

int
procfs_target::insert_watchpoint (CORE_ADDR addr, int len,
				  enum target_hw_bp_type type,
				  struct expression *cond)
{
  if (!target_have_steppable_watchpoint ()
      && !gdbarch_have_nonsteppable_watchpoint (current_inferior ()->arch ()))
    /* When a hardware watchpoint fires off the PC will be left at
       the instruction following the one which caused the
       watchpoint.  It will *NOT* be necessary for GDB to step over
       the watchpoint.  */
    return procfs_set_watchpoint (inferior_ptid, addr, len, type, 1);
  else
    /* When a hardware watchpoint fires off the PC will be left at
       the instruction which caused the watchpoint.  It will be
       necessary for GDB to step over the watchpoint.  */
    return procfs_set_watchpoint (inferior_ptid, addr, len, type, 0);
}

int
procfs_target::remove_watchpoint (CORE_ADDR addr, int len,
				  enum target_hw_bp_type type,
				  struct expression *cond)
{
  return procfs_set_watchpoint (inferior_ptid, addr, 0, 0, 0);
}

int
procfs_target::region_ok_for_hw_watchpoint (CORE_ADDR addr, int len)
{
  /* The man page for proc(4) on Solaris 2.6 and up says that the
     system can support "thousands" of hardware watchpoints, but gives
     no method for finding out how many; It doesn't say anything about
     the allowed size for the watched area either.  So we just tell
     GDB 'yes'.  */
  return 1;
}

/* Memory Mappings Functions: */

/* Call a callback function once for each mapping, passing it the
   mapping, an optional secondary callback function, and some optional
   opaque data.  Quit and return the first non-zero value returned
   from the callback.

   PI is the procinfo struct for the process to be mapped.  FUNC is
   the callback function to be called by this iterator.  DATA is the
   optional opaque data to be passed to the callback function.
   CHILD_FUNC is the optional secondary function pointer to be passed
   to the child function.  Returns the first non-zero return value
   from the callback function, or zero.  */

static int
iterate_over_mappings (procinfo *pi, find_memory_region_ftype child_func,
		       void *data,
		       int (*func) (struct prmap *map,
				    find_memory_region_ftype child_func,
				    void *data))
{
  char pathname[MAX_PROC_NAME_SIZE];
  struct prmap *prmaps;
  struct prmap *prmap;
  int funcstat;
  int nmap;
  struct stat sbuf;

  /* Get the number of mappings, allocate space,
     and read the mappings into prmaps.  */
  /* Open map fd.  */
  xsnprintf (pathname, sizeof (pathname), "/proc/%d/map", pi->pid);

  scoped_fd map_fd (open (pathname, O_RDONLY));
  if (map_fd.get () < 0)
    proc_error (pi, "iterate_over_mappings (open)", __LINE__);

  /* Use stat to determine the file size, and compute
     the number of prmap_t objects it contains.  */
  if (fstat (map_fd.get (), &sbuf) != 0)
    proc_error (pi, "iterate_over_mappings (fstat)", __LINE__);

  nmap = sbuf.st_size / sizeof (prmap_t);
  prmaps = (struct prmap *) alloca ((nmap + 1) * sizeof (*prmaps));
  if (read (map_fd.get (), (char *) prmaps, nmap * sizeof (*prmaps))
      != (nmap * sizeof (*prmaps)))
    proc_error (pi, "iterate_over_mappings (read)", __LINE__);

  for (prmap = prmaps; nmap > 0; prmap++, nmap--)
    {
      funcstat = (*func) (prmap, child_func, data);
      if (funcstat != 0)
	return funcstat;
    }

  return 0;
}

/* Implements the to_find_memory_regions method.  Calls an external
   function for each memory region.
   Returns the integer value returned by the callback.  */

static int
find_memory_regions_callback (struct prmap *map,
			      find_memory_region_ftype func, void *data)
{
  return (*func) ((CORE_ADDR) map->pr_vaddr,
		  map->pr_size,
		  (map->pr_mflags & MA_READ) != 0,
		  (map->pr_mflags & MA_WRITE) != 0,
		  (map->pr_mflags & MA_EXEC) != 0,
		  1, /* MODIFIED is unknown, pass it as true.  */
		  false,
		  data);
}

/* External interface.  Calls a callback function once for each
   mapped memory region in the child process, passing as arguments:

	CORE_ADDR virtual_address,
	unsigned long size,
	int read,	TRUE if region is readable by the child
	int write,	TRUE if region is writable by the child
	int execute	TRUE if region is executable by the child.

   Stops iterating and returns the first non-zero value returned by
   the callback.  */

int
procfs_target::find_memory_regions (find_memory_region_ftype func, void *data)
{
  procinfo *pi = find_procinfo_or_die (inferior_ptid.pid (), 0);

  return iterate_over_mappings (pi, func, data,
				find_memory_regions_callback);
}

/* Returns an ascii representation of a memory mapping's flags.  */

static char *
mappingflags (long flags)
{
  static char asciiflags[8];

  strcpy (asciiflags, "-------");
  if (flags & MA_STACK)
    asciiflags[1] = 's';
  if (flags & MA_BREAK)
    asciiflags[2] = 'b';
  if (flags & MA_SHARED)
    asciiflags[3] = 's';
  if (flags & MA_READ)
    asciiflags[4] = 'r';
  if (flags & MA_WRITE)
    asciiflags[5] = 'w';
  if (flags & MA_EXEC)
    asciiflags[6] = 'x';
  return (asciiflags);
}

/* Callback function, does the actual work for 'info proc
   mappings'.  */

static int
info_mappings_callback (struct prmap *map, find_memory_region_ftype ignore,
			void *unused)
{
  unsigned int pr_off;

  pr_off = (unsigned int) map->pr_offset;

  if (gdbarch_addr_bit (current_inferior ()->arch ()) == 32)
    gdb_printf ("\t%#10lx %#10lx %#10lx %#10x %7s\n",
		(unsigned long) map->pr_vaddr,
		(unsigned long) map->pr_vaddr + map->pr_size - 1,
		(unsigned long) map->pr_size,
		pr_off,
		mappingflags (map->pr_mflags));
  else
    gdb_printf ("  %#18lx %#18lx %#10lx %#10x %7s\n",
		(unsigned long) map->pr_vaddr,
		(unsigned long) map->pr_vaddr + map->pr_size - 1,
		(unsigned long) map->pr_size,
		pr_off,
		mappingflags (map->pr_mflags));

  return 0;
}

/* Implement the "info proc mappings" subcommand.  */

static void
info_proc_mappings (procinfo *pi, int summary)
{
  if (summary)
    return;	/* No output for summary mode.  */

  gdb_printf (_("Mapped address spaces:\n\n"));
  if (gdbarch_ptr_bit (current_inferior ()->arch ()) == 32)
    gdb_printf ("\t%10s %10s %10s %10s %7s\n",
		"Start Addr",
		"  End Addr",
		"      Size",
		"    Offset",
		"Flags");
  else
    gdb_printf ("  %18s %18s %10s %10s %7s\n",
		"Start Addr",
		"  End Addr",
		"      Size",
		"    Offset",
		"Flags");

  iterate_over_mappings (pi, NULL, NULL, info_mappings_callback);
  gdb_printf ("\n");
}

/* Implement the "info proc" command.  */

bool
procfs_target::info_proc (const char *args, enum info_proc_what what)
{
  procinfo *process  = NULL;
  procinfo *thread   = NULL;
  char     *tmp      = NULL;
  int       pid      = 0;
  int       tid      = 0;
  int       mappings = 0;

  switch (what)
    {
    case IP_MINIMAL:
      break;

    case IP_MAPPINGS:
    case IP_ALL:
      mappings = 1;
      break;

    default:
      error (_("Not supported on this target."));
    }

  gdb_argv built_argv (args);
  for (char *arg : built_argv)
    {
      if (isdigit (arg[0]))
	{
	  pid = strtoul (arg, &tmp, 10);
	  if (*tmp == '/')
	    tid = strtoul (++tmp, NULL, 10);
	}
      else if (arg[0] == '/')
	{
	  tid = strtoul (arg + 1, NULL, 10);
	}
    }

  procinfo_up temporary_procinfo;
  if (pid == 0)
    pid = inferior_ptid.pid ();
  if (pid == 0)
    error (_("No current process: you must name one."));
  else
    {
      /* Have pid, will travel.
	 First see if it's a process we're already debugging.  */
      process = find_procinfo (pid, 0);
       if (process == NULL)
	 {
	   /* No.  So open a procinfo for it, but
	      remember to close it again when finished.  */
	   process = create_procinfo (pid, 0);
	   temporary_procinfo.reset (process);
	   if (!open_procinfo_files (process, FD_CTL))
	     proc_error (process, "info proc, open_procinfo_files", __LINE__);
	 }
    }
  if (tid != 0)
    thread = create_procinfo (pid, tid);

  if (process)
    {
      gdb_printf (_("process %d flags:\n"), process->pid);
      proc_prettyprint_flags (proc_flags (process), 1);
      if (proc_flags (process) & (PR_STOPPED | PR_ISTOP))
	proc_prettyprint_why (proc_why (process), proc_what (process), 1);
      if (proc_get_nthreads (process) > 1)
	gdb_printf ("Process has %d threads.\n",
		    proc_get_nthreads (process));
    }
  if (thread)
    {
      gdb_printf (_("thread %d flags:\n"), thread->tid);
      proc_prettyprint_flags (proc_flags (thread), 1);
      if (proc_flags (thread) & (PR_STOPPED | PR_ISTOP))
	proc_prettyprint_why (proc_why (thread), proc_what (thread), 1);
    }

  if (mappings)
    info_proc_mappings (process, 0);

  return true;
}

/* Modify the status of the system call identified by SYSCALLNUM in
   the set of syscalls that are currently traced/debugged.

   If ENTRY_OR_EXIT is set to PR_SYSENTRY, then the entry syscalls set
   will be updated.  Otherwise, the exit syscalls set will be updated.

   If MODE is FLAG_SET, then traces will be enabled.  Otherwise, they
   will be disabled.  */

static void
proc_trace_syscalls_1 (procinfo *pi, int syscallnum, int entry_or_exit,
		       int mode, int from_tty)
{
  sysset_t *sysset;

  if (entry_or_exit == PR_SYSENTRY)
    sysset = proc_get_traced_sysentry (pi, NULL);
  else
    sysset = proc_get_traced_sysexit (pi, NULL);

  if (sysset == NULL)
    proc_error (pi, "proc-trace, get_traced_sysset", __LINE__);

  if (mode == FLAG_SET)
    praddset (sysset, syscallnum);
  else
    prdelset (sysset, syscallnum);

  if (entry_or_exit == PR_SYSENTRY)
    {
      if (!proc_set_traced_sysentry (pi, sysset))
	proc_error (pi, "proc-trace, set_traced_sysentry", __LINE__);
    }
  else
    {
      if (!proc_set_traced_sysexit (pi, sysset))
	proc_error (pi, "proc-trace, set_traced_sysexit", __LINE__);
    }
}

static void
proc_trace_syscalls (const char *args, int from_tty, int entry_or_exit, int mode)
{
  procinfo *pi;

  if (inferior_ptid.pid () <= 0)
    error (_("you must be debugging a process to use this command."));

  if (args == NULL || args[0] == 0)
    error_no_arg (_("system call to trace"));

  pi = find_procinfo_or_die (inferior_ptid.pid (), 0);
  if (isdigit (args[0]))
    {
      const int syscallnum = atoi (args);

      proc_trace_syscalls_1 (pi, syscallnum, entry_or_exit, mode, from_tty);
    }
}

static void
proc_trace_sysentry_cmd (const char *args, int from_tty)
{
  proc_trace_syscalls (args, from_tty, PR_SYSENTRY, FLAG_SET);
}

static void
proc_trace_sysexit_cmd (const char *args, int from_tty)
{
  proc_trace_syscalls (args, from_tty, PR_SYSEXIT, FLAG_SET);
}

static void
proc_untrace_sysentry_cmd (const char *args, int from_tty)
{
  proc_trace_syscalls (args, from_tty, PR_SYSENTRY, FLAG_RESET);
}

static void
proc_untrace_sysexit_cmd (const char *args, int from_tty)
{
  proc_trace_syscalls (args, from_tty, PR_SYSEXIT, FLAG_RESET);
}

void _initialize_procfs ();
void
_initialize_procfs ()
{
  add_com ("proc-trace-entry", no_class, proc_trace_sysentry_cmd,
	   _("Give a trace of entries into the syscall."));
  add_com ("proc-trace-exit", no_class, proc_trace_sysexit_cmd,
	   _("Give a trace of exits from the syscall."));
  add_com ("proc-untrace-entry", no_class, proc_untrace_sysentry_cmd,
	   _("Cancel a trace of entries into the syscall."));
  add_com ("proc-untrace-exit", no_class, proc_untrace_sysexit_cmd,
	   _("Cancel a trace of exits from the syscall."));

  add_inf_child_target (&the_procfs_target);
}

/* =================== END, GDB  "MODULE" =================== */



/* miscellaneous stubs: */

/* The following satisfy a few random symbols mostly created by the
   solaris threads implementation, which I will chase down later.  */

/* Return a pid for which we guarantee we will be able to find a
   'live' procinfo.  */

ptid_t
procfs_first_available (void)
{
  return ptid_t (procinfo_list ? procinfo_list->pid : -1);
}

/* ===================  GCORE .NOTE "MODULE" =================== */

static void
procfs_do_thread_registers (bfd *obfd, ptid_t ptid,
			    gdb::unique_xmalloc_ptr<char> &note_data,
			    int *note_size, enum gdb_signal stop_signal)
{
  struct regcache *regcache = get_thread_regcache (&the_procfs_target, ptid);
  gdb_gregset_t gregs;
  gdb_fpregset_t fpregs;
  unsigned long merged_pid;

  merged_pid = ptid.lwp () << 16 | ptid.pid ();

  /* This part is the old method for fetching registers.
     It should be replaced by the newer one using regsets
     once it is implemented in this platform:
     gdbarch_iterate_over_regset_sections().  */

  target_fetch_registers (regcache, -1);

  fill_gregset (regcache, &gregs, -1);
  note_data.reset (elfcore_write_lwpstatus (obfd,
					    note_data.release (),
					    note_size,
					    merged_pid,
					    stop_signal,
					    &gregs));
  fill_fpregset (regcache, &fpregs, -1);
  note_data.reset (elfcore_write_prfpreg (obfd,
					  note_data.release (),
					  note_size,
					  &fpregs,
					  sizeof (fpregs)));
}

struct procfs_corefile_thread_data
{
  procfs_corefile_thread_data (bfd *obfd,
			       gdb::unique_xmalloc_ptr<char> &note_data,
			       int *note_size, gdb_signal stop_signal)
    : obfd (obfd), note_data (note_data), note_size (note_size),
      stop_signal (stop_signal)
  {}

  bfd *obfd;
  gdb::unique_xmalloc_ptr<char> &note_data;
  int *note_size;
  enum gdb_signal stop_signal;
};

static int
procfs_corefile_thread_callback (procinfo *pi, procinfo *thread, void *data)
{
  struct procfs_corefile_thread_data *args
    = (struct procfs_corefile_thread_data *) data;

  if (pi != NULL)
    {
      ptid_t ptid = ptid_t (pi->pid, thread->tid, 0);

      procfs_do_thread_registers (args->obfd, ptid,
				  args->note_data,
				  args->note_size,
				  args->stop_signal);
    }
  return 0;
}

static int
find_signalled_thread (struct thread_info *info, void *data)
{
  if (info->stop_signal () != GDB_SIGNAL_0
      && info->ptid.pid () == inferior_ptid.pid ())
    return 1;

  return 0;
}

static enum gdb_signal
find_stop_signal (void)
{
  struct thread_info *info =
    iterate_over_threads (find_signalled_thread, NULL);

  if (info)
    return info->stop_signal ();
  else
    return GDB_SIGNAL_0;
}

gdb::unique_xmalloc_ptr<char>
procfs_target::make_corefile_notes (bfd *obfd, int *note_size)
{
  gdb_gregset_t gregs;
  char fname[16] = {'\0'};
  char psargs[80] = {'\0'};
  procinfo *pi = find_procinfo_or_die (inferior_ptid.pid (), 0);
  gdb::unique_xmalloc_ptr<char> note_data;
  enum gdb_signal stop_signal;

  if (get_exec_file (0))
    {
      strncpy (fname, lbasename (get_exec_file (0)), sizeof (fname));
      fname[sizeof (fname) - 1] = 0;
      strncpy (psargs, get_exec_file (0), sizeof (psargs));
      psargs[sizeof (psargs) - 1] = 0;

      const std::string &inf_args = current_inferior ()->args ();
      if (!inf_args.empty () &&
	  inf_args.length () < ((int) sizeof (psargs) - (int) strlen (psargs)))
	{
	  strncat (psargs, " ",
		   sizeof (psargs) - strlen (psargs));
	  strncat (psargs, inf_args.c_str (),
		   sizeof (psargs) - strlen (psargs));
	}
    }

  note_data.reset (elfcore_write_prpsinfo (obfd,
					   note_data.release (),
					   note_size,
					   fname,
					   psargs));

  stop_signal = find_stop_signal ();

  fill_gregset (get_thread_regcache (inferior_thread ()), &gregs, -1);
  note_data.reset (elfcore_write_pstatus (obfd, note_data.release (), note_size,
					  inferior_ptid.pid (),
					  stop_signal, &gregs));

  procfs_corefile_thread_data thread_args (obfd, note_data, note_size,
					   stop_signal);
  proc_iterate_over_threads (pi, procfs_corefile_thread_callback,
			     &thread_args);

  std::optional<gdb::byte_vector> auxv =
    target_read_alloc (current_inferior ()->top_target (),
		       TARGET_OBJECT_AUXV, NULL);
  if (auxv && !auxv->empty ())
    note_data.reset (elfcore_write_note (obfd, note_data.release (), note_size,
					 "CORE", NT_AUXV, auxv->data (),
					 auxv->size ()));

  return note_data;
}
/* ===================  END GCORE .NOTE "MODULE" =================== */
