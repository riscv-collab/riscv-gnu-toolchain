/* Low-level child interface to ptrace.

   Copyright (C) 1988-2024 Free Software Foundation, Inc.

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
#include "command.h"
#include "inferior.h"
#include "terminal.h"
#include "gdbcore.h"
#include "regcache.h"
#include "nat/gdb_ptrace.h"
#include "gdbsupport/gdb_wait.h"
#include <signal.h>

#include "inf-ptrace.h"
#include "inf-child.h"
#include "gdbthread.h"
#include "nat/fork-inferior.h"
#include "utils.h"
#include "gdbarch.h"



static PTRACE_TYPE_RET
gdb_ptrace (PTRACE_TYPE_ARG1 request, ptid_t ptid, PTRACE_TYPE_ARG3 addr,
	    PTRACE_TYPE_ARG4 data)
{
#ifdef __NetBSD__
  return ptrace (request, ptid.pid (), addr, data);
#else
  pid_t pid = get_ptrace_pid (ptid);
  return ptrace (request, pid, addr, data);
#endif
}

/* The event pipe registered as a waitable file in the event loop.  */
event_pipe inf_ptrace_target::m_event_pipe;

inf_ptrace_target::~inf_ptrace_target ()
{}



/* Prepare to be traced.  */

static void
inf_ptrace_me (void)
{
  /* "Trace me, Dr. Memory!"  */
  if (ptrace (PT_TRACE_ME, 0, (PTRACE_TYPE_ARG3) 0, 0) < 0)
    trace_start_error_with_name ("ptrace");
}

/* Start a new inferior Unix child process.  EXEC_FILE is the file to
   run, ALLARGS is a string containing the arguments to the program.
   ENV is the environment vector to pass.  If FROM_TTY is non-zero, be
   chatty about it.  */

void
inf_ptrace_target::create_inferior (const char *exec_file,
				    const std::string &allargs,
				    char **env, int from_tty)
{
  inferior *inf = current_inferior ();

  /* Do not change either targets above or the same target if already present.
     The reason is the target stack is shared across multiple inferiors.  */
  int ops_already_pushed = inf->target_is_pushed (this);

  target_unpush_up unpusher;
  if (! ops_already_pushed)
    {
      /* Clear possible core file with its process_stratum.  */
      inf->push_target (this);
      unpusher.reset (this);
    }

  pid_t pid = fork_inferior (exec_file, allargs, env, inf_ptrace_me, NULL,
			     NULL, NULL, NULL);

  ptid_t ptid (pid);
  /* We have something that executes now.  We'll be running through
     the shell at this point (if startup-with-shell is true), but the
     pid shouldn't change.  */
  thread_info *thr = add_thread_silent (this, ptid);
  switch_to_thread (thr);

  unpusher.release ();

  gdb_startup_inferior (pid, START_INFERIOR_TRAPS_EXPECTED);

  /* On some targets, there must be some explicit actions taken after
     the inferior has been started up.  */
  post_startup_inferior (ptid);
}

/* Clean up a rotting corpse of an inferior after it died.  */

void
inf_ptrace_target::mourn_inferior ()
{
  int status;

  /* Wait just one more time to collect the inferior's exit status.
     Do not check whether this succeeds though, since we may be
     dealing with a process that we attached to.  Such a process will
     only report its exit status to its original parent.  */
  waitpid (inferior_ptid.pid (), &status, 0);

  inf_child_target::mourn_inferior ();
}

/* Attach to the process specified by ARGS.  If FROM_TTY is non-zero,
   be chatty about it.  */

void
inf_ptrace_target::attach (const char *args, int from_tty)
{
  inferior *inf = current_inferior ();

  /* Do not change either targets above or the same target if already present.
     The reason is the target stack is shared across multiple inferiors.  */
  int ops_already_pushed = inf->target_is_pushed (this);

  pid_t pid = parse_pid_to_attach (args);

  if (pid == getpid ())		/* Trying to masturbate?  */
    error (_("I refuse to debug myself!"));

  target_unpush_up unpusher;
  if (! ops_already_pushed)
    {
      /* target_pid_to_str already uses the target.  Also clear possible core
	 file with its process_stratum.  */
      inf->push_target (this);
      unpusher.reset (this);
    }

  target_announce_attach (from_tty, pid);

#ifdef PT_ATTACH
  errno = 0;
  ptrace (PT_ATTACH, pid, (PTRACE_TYPE_ARG3)0, 0);
  if (errno != 0)
    perror_with_name (("ptrace"));
#else
  error (_("This system does not support attaching to a process"));
#endif

  inferior_appeared (inf, pid);
  inf->attach_flag = true;

  /* Always add a main thread.  If some target extends the ptrace
     target, it should decorate the ptid later with more info.  */
  thread_info *thr = add_thread_silent (this, ptid_t (pid));
  switch_to_thread (thr);

  /* Don't consider the thread stopped until we've processed its
     initial SIGSTOP stop.  */
  set_executing (this, thr->ptid, true);

  unpusher.release ();
}

/* Detach from the inferior.  If FROM_TTY is non-zero, be chatty about it.  */

void
inf_ptrace_target::detach (inferior *inf, int from_tty)
{
  pid_t pid = inferior_ptid.pid ();

  target_announce_detach (from_tty);

#ifdef PT_DETACH
  /* We'd better not have left any breakpoints in the program or it'll
     die when it hits one.  Also note that this may only work if we
     previously attached to the inferior.  It *might* work if we
     started the process ourselves.  */
  errno = 0;
  ptrace (PT_DETACH, pid, (PTRACE_TYPE_ARG3)1, 0);
  if (errno != 0)
    perror_with_name (("ptrace"));
#else
  error (_("This system does not support detaching from a process"));
#endif

  detach_success (inf);
}

/* See inf-ptrace.h.  */

void
inf_ptrace_target::detach_success (inferior *inf)
{
  switch_to_no_thread ();
  detach_inferior (inf);

  maybe_unpush_target ();
}

/* Kill the inferior.  */

void
inf_ptrace_target::kill ()
{
  pid_t pid = inferior_ptid.pid ();
  int status;

  if (pid == 0)
    return;

  ptrace (PT_KILL, pid, (PTRACE_TYPE_ARG3)0, 0);
  waitpid (pid, &status, 0);

  target_mourn_inferior (inferior_ptid);
}

#ifndef __NetBSD__

/* See inf-ptrace.h.  */

pid_t
get_ptrace_pid (ptid_t ptid)
{
  pid_t pid;

  /* If we have an LWPID to work with, use it.  Otherwise, we're
     dealing with a non-threaded program/target.  */
  pid = ptid.lwp ();
  if (pid == 0)
    pid = ptid.pid ();
  return pid;
}
#endif

/* Resume execution of thread PTID, or all threads if PTID is -1.  If
   STEP is nonzero, single-step it.  If SIGNAL is nonzero, give it
   that signal.  */

void
inf_ptrace_target::resume (ptid_t ptid, int step, enum gdb_signal signal)
{
  PTRACE_TYPE_ARG1 request;

  if (minus_one_ptid == ptid)
    /* Resume all threads.  Traditionally ptrace() only supports
       single-threaded processes, so simply resume the inferior.  */
    ptid = ptid_t (inferior_ptid.pid ());

  if (catch_syscall_enabled ())
    request = PT_SYSCALL;
  else
    request = PT_CONTINUE;

  if (step)
    {
      /* If this system does not support PT_STEP, a higher level
	 function will have called the appropriate functions to transmute the
	 step request into a continue request (by setting breakpoints on
	 all possible successor instructions), so we don't have to
	 worry about that here.  */
      request = PT_STEP;
    }

  /* An address of (PTRACE_TYPE_ARG3)1 tells ptrace to continue from
     where it was.  If GDB wanted it to start some other way, we have
     already written a new program counter value to the child.  */
  errno = 0;
  gdb_ptrace (request, ptid, (PTRACE_TYPE_ARG3)1, gdb_signal_to_host (signal));
  if (errno != 0)
    perror_with_name (("ptrace"));
}

/* Wait for the child specified by PTID to do something.  Return the
   process ID of the child, or MINUS_ONE_PTID in case of error; store
   the status in *OURSTATUS.  */

ptid_t
inf_ptrace_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
			 target_wait_flags target_options)
{
  pid_t pid;
  int options, status, save_errno;

  options = 0;
  if (target_options & TARGET_WNOHANG)
    options |= WNOHANG;

  do
    {
      set_sigint_trap ();

      do
	{
	  pid = waitpid (ptid.pid (), &status, options);
	  save_errno = errno;
	}
      while (pid == -1 && errno == EINTR);

      clear_sigint_trap ();

      if (pid == 0)
	{
	  gdb_assert (target_options & TARGET_WNOHANG);
	  ourstatus->set_ignore ();
	  return minus_one_ptid;
	}

      if (pid == -1)
	{
	  /* In async mode the SIGCHLD might have raced and triggered
	     a check for an event that had already been reported.  If
	     the event was the exit of the only remaining child,
	     waitpid() will fail with ECHILD.  */
	  if (ptid == minus_one_ptid && save_errno == ECHILD)
	    {
	      ourstatus->set_no_resumed ();
	      return minus_one_ptid;
	    }

	  gdb_printf (gdb_stderr,
		      _("Child process unexpectedly missing: %s.\n"),
		      safe_strerror (save_errno));

	  ourstatus->set_ignore ();
	  return minus_one_ptid;
	}

      /* Ignore terminated detached child processes.  */
      if (!WIFSTOPPED (status) && find_inferior_pid (this, pid) == nullptr)
	pid = -1;
    }
  while (pid == -1);

  *ourstatus = host_status_to_waitstatus (status);

  return ptid_t (pid);
}

/* Transfer data via ptrace into process PID's memory from WRITEBUF, or
   from process PID's memory into READBUF.  Start at target address ADDR
   and transfer up to LEN bytes.  Exactly one of READBUF and WRITEBUF must
   be non-null.  Return the number of transferred bytes.  */

static ULONGEST
inf_ptrace_peek_poke (ptid_t ptid, gdb_byte *readbuf,
		      const gdb_byte *writebuf,
		      ULONGEST addr, ULONGEST len)
{
  ULONGEST n;
  unsigned int chunk;

  /* We transfer aligned words.  Thus align ADDR down to a word
     boundary and determine how many bytes to skip at the
     beginning.  */
  ULONGEST skip = addr & (sizeof (PTRACE_TYPE_RET) - 1);
  addr -= skip;

  for (n = 0;
       n < len;
       n += chunk, addr += sizeof (PTRACE_TYPE_RET), skip = 0)
    {
      /* Restrict to a chunk that fits in the current word.  */
      chunk = std::min (sizeof (PTRACE_TYPE_RET) - skip, len - n);

      /* Use a union for type punning.  */
      union
      {
	PTRACE_TYPE_RET word;
	gdb_byte byte[sizeof (PTRACE_TYPE_RET)];
      } buf;

      /* Read the word, also when doing a partial word write.  */
      if (readbuf != NULL || chunk < sizeof (PTRACE_TYPE_RET))
	{
	  errno = 0;
	  buf.word = gdb_ptrace (PT_READ_I, ptid,
				 (PTRACE_TYPE_ARG3)(uintptr_t) addr, 0);
	  if (errno != 0)
	    break;
	  if (readbuf != NULL)
	    memcpy (readbuf + n, buf.byte + skip, chunk);
	}
      if (writebuf != NULL)
	{
	  memcpy (buf.byte + skip, writebuf + n, chunk);
	  errno = 0;
	  gdb_ptrace (PT_WRITE_D, ptid, (PTRACE_TYPE_ARG3)(uintptr_t) addr,
		  buf.word);
	  if (errno != 0)
	    {
	      /* Using the appropriate one (I or D) is necessary for
		 Gould NP1, at least.  */
	      errno = 0;
	      gdb_ptrace (PT_WRITE_I, ptid, (PTRACE_TYPE_ARG3)(uintptr_t) addr,
			  buf.word);
	      if (errno != 0)
		break;
	    }
	}
    }

  return n;
}

/* Implement the to_xfer_partial target_ops method.  */

enum target_xfer_status
inf_ptrace_target::xfer_partial (enum target_object object,
				 const char *annex, gdb_byte *readbuf,
				 const gdb_byte *writebuf,
				 ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  ptid_t ptid = inferior_ptid;

  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
#ifdef PT_IO
      /* OpenBSD 3.1, NetBSD 1.6 and FreeBSD 5.0 have a new PT_IO
	 request that promises to be much more efficient in reading
	 and writing data in the traced process's address space.  */
      {
	struct ptrace_io_desc piod;

	/* NOTE: We assume that there are no distinct address spaces
	   for instruction and data.  However, on OpenBSD 3.9 and
	   later, PIOD_WRITE_D doesn't allow changing memory that's
	   mapped read-only.  Since most code segments will be
	   read-only, using PIOD_WRITE_D will prevent us from
	   inserting breakpoints, so we use PIOD_WRITE_I instead.  */
	piod.piod_op = writebuf ? PIOD_WRITE_I : PIOD_READ_D;
	piod.piod_addr = writebuf ? (void *) writebuf : readbuf;
	piod.piod_offs = (void *) (long) offset;
	piod.piod_len = len;

	errno = 0;
	if (gdb_ptrace (PT_IO, ptid, (caddr_t)&piod, 0) == 0)
	  {
	    /* Return the actual number of bytes read or written.  */
	    *xfered_len = piod.piod_len;
	    return (piod.piod_len == 0) ? TARGET_XFER_EOF : TARGET_XFER_OK;
	  }
	/* If the PT_IO request is somehow not supported, fallback on
	   using PT_WRITE_D/PT_READ_D.  Otherwise we will return zero
	   to indicate failure.  */
	if (errno != EINVAL)
	  return TARGET_XFER_EOF;
      }
#endif
      *xfered_len = inf_ptrace_peek_poke (ptid, readbuf, writebuf,
					  offset, len);
      return *xfered_len != 0 ? TARGET_XFER_OK : TARGET_XFER_EOF;

    case TARGET_OBJECT_UNWIND_TABLE:
      return TARGET_XFER_E_IO;

    case TARGET_OBJECT_AUXV:
#if defined (PT_IO) && defined (PIOD_READ_AUXV)
      /* OpenBSD 4.5 has a new PIOD_READ_AUXV operation for the PT_IO
	 request that allows us to read the auxilliary vector.  Other
	 BSD's may follow if they feel the need to support PIE.  */
      {
	struct ptrace_io_desc piod;

	if (writebuf)
	  return TARGET_XFER_E_IO;
	piod.piod_op = PIOD_READ_AUXV;
	piod.piod_addr = readbuf;
	piod.piod_offs = (void *) (long) offset;
	piod.piod_len = len;

	errno = 0;
	if (gdb_ptrace (PT_IO, ptid, (caddr_t)&piod, 0) == 0)
	  {
	    /* Return the actual number of bytes read or written.  */
	    *xfered_len = piod.piod_len;
	    return (piod.piod_len == 0) ? TARGET_XFER_EOF : TARGET_XFER_OK;
	  }
      }
#endif
      return TARGET_XFER_E_IO;

    case TARGET_OBJECT_WCOOKIE:
      return TARGET_XFER_E_IO;

    default:
      return TARGET_XFER_E_IO;
    }
}

/* Return non-zero if the thread specified by PTID is alive.  */

bool
inf_ptrace_target::thread_alive (ptid_t ptid)
{
  /* ??? Is kill the right way to do this?  */
  return (::kill (ptid.pid (), 0) != -1);
}

/* Print status information about what we're accessing.  */

void
inf_ptrace_target::files_info ()
{
  struct inferior *inf = current_inferior ();

  gdb_printf (_("\tUsing the running image of %s %s.\n"),
	      inf->attach_flag ? "attached" : "child",
	      target_pid_to_str (ptid_t (inf->pid)).c_str ());
}

std::string
inf_ptrace_target::pid_to_str (ptid_t ptid)
{
  return normal_pid_to_str (ptid);
}

/* Implement the "close" target method.  */

void
inf_ptrace_target::close ()
{
  /* Unregister from the event loop.  */
  if (is_async_p ())
    async (false);

  inf_child_target::close ();
}
