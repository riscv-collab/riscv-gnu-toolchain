/* Serial interface for a pipe to a separate program
   Copyright (C) 1999-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions.

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
#include "serial.h"
#include "ser-base.h"
#include "ser-unix.h"

#include "gdb_vfork.h"

#include <sys/types.h>
#include <sys/socket.h>
#include "gdbsupport/gdb_sys_time.h"
#include <fcntl.h>
#include "gdbsupport/filestuff.h"
#include "gdbsupport/pathstuff.h"

#include <signal.h>

static void pipe_close (struct serial *scb);

struct pipe_state
  {
    int pid;
  };

/* Open up a raw pipe.  */

static void
pipe_open (struct serial *scb, const char *name)
{
#if !HAVE_SOCKETPAIR
  return -1;
#else
  struct pipe_state *state;
  /* This chunk: */
  /* Copyright (c) 1988, 1993
   *      The Regents of the University of California.  All rights reserved.
   *
   * This code is derived from software written by Ken Arnold and
   * published in UNIX Review, Vol. 6, No. 8.
   */
  int pdes[2];
  int err_pdes[2];
  int pid;

  if (*name == '|')
    {
      name++;
      name = skip_spaces (name);
    }

  if (gdb_socketpair_cloexec (AF_UNIX, SOCK_STREAM, 0, pdes) < 0)
    perror_with_name (_("could not open socket pair"));
  if (gdb_socketpair_cloexec (AF_UNIX, SOCK_STREAM, 0, err_pdes) < 0)
    {
      int save = errno;
      close (pdes[0]);
      close (pdes[1]);
      perror_with_name (_("could not open socket pair"), save);
    }

  /* Create the child process to run the command in.  Note that the
     apparent call to vfork() below *might* actually be a call to
     fork() due to the fact that autoconf will ``#define vfork fork''
     on certain platforms.  */
  pid = vfork ();
  
  /* Error.  */
  if (pid == -1)
    {
      int save = errno;
      close (pdes[0]);
      close (pdes[1]);
      close (err_pdes[0]);
      close (err_pdes[1]);
      perror_with_name (_("could not vfork"), save);
    }

  if (fcntl (err_pdes[0], F_SETFL, O_NONBLOCK) == -1)
    {
      close (err_pdes[0]);
      close (err_pdes[1]);
      err_pdes[0] = err_pdes[1] = -1;
    }

  /* Child.  */
  if (pid == 0)
    {
      /* We don't want ^c to kill the connection.  */
#ifdef HAVE_SETSID
      pid_t sid = setsid ();
      if (sid == -1)
	signal (SIGINT, SIG_IGN);
#else
      signal (SIGINT, SIG_IGN);
#endif

      /* Re-wire pdes[1] to stdin/stdout.  */
      close (pdes[0]);
      if (pdes[1] != STDOUT_FILENO)
	{
	  dup2 (pdes[1], STDOUT_FILENO);
	  close (pdes[1]);
	}
      dup2 (STDOUT_FILENO, STDIN_FILENO);

      if (err_pdes[0] != -1)
	{
	  close (err_pdes[0]);
	  dup2 (err_pdes[1], STDERR_FILENO);
	  close (err_pdes[1]);
	}

      close_most_fds ();

      const char *shellfile = get_shell ();
      execl (shellfile, shellfile, "-c", name, (char *) 0);
      _exit (127);
    }

  /* Parent.  */
  close (pdes[1]);
  if (err_pdes[1] != -1)
    close (err_pdes[1]);
  /* :end chunk */
  state = XNEW (struct pipe_state);
  state->pid = pid;
  scb->fd = pdes[0];
  scb->error_fd = err_pdes[0];
  scb->state = state;

  /* If we don't do this, GDB simply exits when the remote side dies.  */
  signal (SIGPIPE, SIG_IGN);
#endif
}

static void
pipe_close (struct serial *scb)
{
  struct pipe_state *state = (struct pipe_state *) scb->state;

  close (scb->fd);
  scb->fd = -1;

  if (state != NULL)
    {
      int wait_result, status;

      /* Don't kill the task right away, give it a chance to shut down cleanly.
	 But don't wait forever though.  */
#define PIPE_CLOSE_TIMEOUT 5

      /* Assume the program will exit after SIGTERM.  Might be
	 useful to print any remaining stderr output from
	 scb->error_fd while waiting.  */
#define SIGTERM_TIMEOUT INT_MAX

      wait_result = -1;
#ifdef HAVE_WAITPID
      wait_result = wait_to_die_with_timeout (state->pid, &status,
					      PIPE_CLOSE_TIMEOUT);
#endif
      if (wait_result == -1)
	{
	  kill (state->pid, SIGTERM);
#ifdef HAVE_WAITPID
	  wait_to_die_with_timeout (state->pid, &status, SIGTERM_TIMEOUT);
#endif
	}

      if (scb->error_fd != -1)
	close (scb->error_fd);
      scb->error_fd = -1;
      xfree (state);
      scb->state = NULL;
    }
}

int
gdb_pipe (int pdes[2])
{
#if !HAVE_SOCKETPAIR
  errno = ENOSYS;
  return -1;
#else

  if (gdb_socketpair_cloexec (AF_UNIX, SOCK_STREAM, 0, pdes) < 0)
    return -1;

  /* If we don't do this, GDB simply exits when the remote side
     dies.  */
  signal (SIGPIPE, SIG_IGN);
  return 0;
#endif
}

static const struct serial_ops pipe_ops =
{
  "pipe",
  pipe_open,
  pipe_close,
  NULL,
  ser_base_readchar,
  ser_base_write,
  ser_base_flush_output,
  ser_base_flush_input,
  ser_base_send_break,
  ser_base_raw,
  ser_base_get_tty_state,
  ser_base_copy_tty_state,
  ser_base_set_tty_state,
  ser_base_print_tty_state,
  ser_base_setbaudrate,
  ser_base_setstopbits,
  ser_base_setparity,
  ser_base_drain_output,
  ser_base_async,
  ser_unix_read_prim,
  ser_unix_write_prim
};

void _initialize_ser_pipe ();
void
_initialize_ser_pipe ()
{
  serial_add_interface (&pipe_ops);
}
