/* Fork a Unix child process, and set up to debug it, for GDBserver.
   Copyright (C) 1989-2024 Free Software Foundation, Inc.

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
#include "gdbsupport/job-control.h"
#include "gdbsupport/scoped_restore.h"
#include "nat/fork-inferior.h"
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef SIGTTOU
/* A file descriptor for the controlling terminal.  */
static int terminal_fd;

/* TERMINAL_FD's original foreground group.  */
static pid_t old_foreground_pgrp;

/* Hand back terminal ownership to the original foreground group.  */

static void
restore_old_foreground_pgrp (void)
{
  tcsetpgrp (terminal_fd, old_foreground_pgrp);
}
#endif

/* See nat/fork-inferior.h.  */

void
prefork_hook (const char *args)
{
  client_state &cs = get_client_state ();
  threads_debug_printf ("args: %s", args);

#ifdef SIGTTOU
  signal (SIGTTOU, SIG_DFL);
  signal (SIGTTIN, SIG_DFL);
#endif

  /* Clear this so the backend doesn't get confused, thinking
     CONT_THREAD died, and it needs to resume all threads.  */
  cs.cont_thread = null_ptid;
}

/* See nat/fork-inferior.h.  */

void
postfork_hook (pid_t pid)
{
}

/* See nat/fork-inferior.h.  */

void
postfork_child_hook ()
{
  /* This is set to the result of setpgrp, which if vforked, will be
     visible to you in the parent process.  It's only used by humans
     for debugging.  */
  static int debug_setpgrp = 657473;

  debug_setpgrp = gdb_setpgid ();
  if (debug_setpgrp == -1)
    perror (_("setpgrp failed in child"));
}

/* See nat/fork-inferior.h.  */

void
gdb_flush_out_err ()
{
  fflush (stdout);
  fflush (stderr);
}

/* See server.h.  */

void
post_fork_inferior (int pid, const char *program)
{
  client_state &cs = get_client_state ();
#ifdef SIGTTOU
  signal (SIGTTOU, SIG_IGN);
  signal (SIGTTIN, SIG_IGN);
  terminal_fd = fileno (stderr);
  old_foreground_pgrp = tcgetpgrp (terminal_fd);
  tcsetpgrp (terminal_fd, pid);
  atexit (restore_old_foreground_pgrp);
#endif

  process_info *proc = find_process_pid (pid);

  /* If the inferior fails to start, startup_inferior mourns the
     process (which deletes it), and then throws an error.  This means
     that on exception return, we don't need or want to clear this
     flag back, as PROC won't exist anymore.  Thus, we don't use a
     scoped_restore.  */
  proc->starting_up = true;

  startup_inferior (the_target, pid,
		    START_INFERIOR_TRAPS_EXPECTED,
		    &cs.last_status, &cs.last_ptid);

  /* If we get here, the process was successfully started.  */
  proc->starting_up = false;

  current_thread->last_resume_kind = resume_stop;
  current_thread->last_status = cs.last_status;
  signal_pid = pid;
  target_post_create_inferior ();
  fprintf (stderr, "Process %s created; pid = %d\n", program, pid);
  fflush (stderr);
}
