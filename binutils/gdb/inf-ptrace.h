/* Low level child interface to ptrace.

   Copyright (C) 2004-2024 Free Software Foundation, Inc.

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

#ifndef INF_PTRACE_H
#define INF_PTRACE_H

#include "gdbsupport/event-pipe.h"
#include "inf-child.h"

/* An abstract prototype ptrace target.  The client can override it
   with local methods.  */

struct inf_ptrace_target : public inf_child_target
{
  ~inf_ptrace_target () override = 0;

  void attach (const char *, int) override;

  void detach (inferior *inf, int) override;

  void close () override;

  void resume (ptid_t, int, enum gdb_signal) override;

  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;

  void files_info () override;

  void kill () override;

  void create_inferior (const char *, const std::string &,
			char **, int) override;

  void mourn_inferior () override;

  bool thread_alive (ptid_t ptid) override;

  std::string pid_to_str (ptid_t) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  bool is_async_p () override
  { return m_event_pipe.is_open (); }

  int async_wait_fd () override
  { return m_event_pipe.event_fd (); }

  /* Helper routine used from SIGCHLD handlers to signal the async
     event pipe.  */
  static void async_file_mark_if_open ()
  {
    if (m_event_pipe.is_open ())
      m_event_pipe.mark ();
  }

protected:
  /* Helper routines for interacting with the async event pipe.  */
  bool async_file_open ()
  { return m_event_pipe.open_pipe (); }
  void async_file_close ()
  { m_event_pipe.close_pipe (); }
  void async_file_flush ()
  { m_event_pipe.flush (); }
  void async_file_mark ()
  { m_event_pipe.mark (); }

  /* Cleanup the inferior after a successful ptrace detach.  */
  void detach_success (inferior *inf);

  /* Some targets don't allow us to request notification of inferior events
     such as fork and vfork immediately after the inferior is created.
     (This is because of how gdb creates inferiors via invoking a shell to
     do it.  In such a scenario, if the shell init file has commands in it,
     the shell will fork and exec for each of those commands, and we will
     see each such fork event.  Very bad.)

     Such targets will supply an appropriate definition for this
     function.  */
  virtual void post_startup_inferior (ptid_t ptid) = 0;

private:
  static event_pipe m_event_pipe;
};

#ifndef __NetBSD__
/* Return which PID to pass to ptrace in order to observe/control the
   tracee identified by PTID.

   Unlike most other Operating Systems, NetBSD tracks both pid and lwp
   and avoids this function.  */

extern pid_t get_ptrace_pid (ptid_t);
#endif

#endif
