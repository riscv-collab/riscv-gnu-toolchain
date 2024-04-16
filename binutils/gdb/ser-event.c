/* Serial interface for a selectable event.
   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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
#include "ser-event.h"
#include "serial.h"
#include "gdbsupport/filestuff.h"

/* On POSIX hosts, a serial_event is basically an abstraction for the
   classical self-pipe trick.

   On Windows, a serial_event is a wrapper around a native Windows
   event object.  Because we want to interface with gdb_select, which
   takes file descriptors, we need to wrap that Windows event object
   in a file descriptor.  As _open_osfhandle can not be used with
   event objects, we instead create a dummy file wrap that in a file
   descriptor with _open_osfhandle, and pass that as selectable
   descriptor to callers.  As Windows' gdb_select converts file
   descriptors back to Windows handles by calling serial->wait_handle,
   nothing ever actually waits on that file descriptor.  */

struct serial_event_state
  {
#ifdef USE_WIN32API
    /* The Windows event object, created with CreateEvent.  */
    HANDLE event;
#else
    /* The write side of the pipe.  The read side is in
       serial->fd.  */
    int write_fd;
#endif
  };

/* Open a new serial event.  */

static void
serial_event_open (struct serial *scb, const char *name)
{
  struct serial_event_state *state;

  state = XNEW (struct serial_event_state);
  scb->state = state;

#ifndef USE_WIN32API
  {
    int fds[2];

    if (gdb_pipe_cloexec (fds) == -1)
      internal_error ("creating serial event pipe failed.");

    fcntl (fds[0], F_SETFL, O_NONBLOCK);
    fcntl (fds[1], F_SETFL, O_NONBLOCK);

    scb->fd = fds[0];
    state->write_fd = fds[1];
  }
#else
  {
    /* A dummy file object that can be wrapped in a file descriptor.
       We don't need to store this handle because closing the file
       descriptor automatically closes this.  */
    HANDLE dummy_file;

    /* A manual-reset event.  */
    state->event = CreateEvent (0, TRUE, FALSE, 0);

    /* The dummy file handle.  Created just so we have something
       wrappable in a file descriptor.  */
    dummy_file = CreateFile ("nul", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    scb->fd = _open_osfhandle ((intptr_t) dummy_file, 0);
  }
#endif
}

static void
serial_event_close (struct serial *scb)
{
  struct serial_event_state *state = (struct serial_event_state *) scb->state;

  close (scb->fd);
#ifndef USE_WIN32API
  close (state->write_fd);
#else
  CloseHandle (state->event);
#endif

  scb->fd = -1;

  xfree (state);
  scb->state = NULL;
}

#ifdef USE_WIN32API

/* Implementation of the wait_handle method.  Returns the native
   Windows event object handle.  */

static void
serial_event_wait_handle (struct serial *scb, HANDLE *read, HANDLE *except)
{
  struct serial_event_state *state = (struct serial_event_state *) scb->state;

  *read = state->event;
}

#endif

/* The serial_ops for struct serial_event objects.  Note we never
   register this serial type with serial_add_interface, because this
   is internal implementation detail never to be used by remote
   targets for protocol transport.  */

static const struct serial_ops serial_event_ops =
{
  "event",
  serial_event_open,
  serial_event_close,
  NULL, /* fdopen */
  NULL, /* readchar */
  NULL, /* write */
  NULL, /* flush_output */
  NULL, /* flush_input */
  NULL, /* send_break */
  NULL, /* go_raw */
  NULL, /* get_tty_state */
  NULL, /* copy_tty_state */
  NULL, /* set_tty_state */
  NULL, /* print_tty_state */
  NULL, /* setbaudrate */
  NULL, /* setstopbits */
  NULL, /* setparity */
  NULL, /* drain_output */
  NULL, /* async */
  NULL, /* read_prim */
  NULL, /* write_prim */
  NULL, /* avail */
#ifdef USE_WIN32API
  serial_event_wait_handle,
#endif
};

/* See ser-event.h.  */

struct serial_event *
make_serial_event (void)
{
  return (struct serial_event *) serial_open_ops (&serial_event_ops);
}

/* See ser-event.h.  */

int
serial_event_fd (struct serial_event *event)
{
  struct serial *ser = (struct serial *) event;

  return ser->fd;
}

/* See ser-event.h.  */

void
serial_event_set (struct serial_event *event)
{
  struct serial *ser = (struct serial *) event;
  struct serial_event_state *state = (struct serial_event_state *) ser->state;
#ifndef USE_WIN32API
  int r;
  char c = '+';		/* Anything.  */

  do
    {
      r = write (state->write_fd, &c, 1);
    }
  while (r < 0 && errno == EINTR);
#else
  SetEvent (state->event);
#endif
}

/* See ser-event.h.  */

void
serial_event_clear (struct serial_event *event)
{
  struct serial *ser = (struct serial *) event;
#ifndef USE_WIN32API
  int r;

  do
    {
      char c;

      r = read (ser->fd, &c, 1);
    }
  while (r > 0 || (r < 0 && errno == EINTR));
#else
  struct serial_event_state *state = (struct serial_event_state *) ser->state;
  ResetEvent (state->event);
#endif
}
