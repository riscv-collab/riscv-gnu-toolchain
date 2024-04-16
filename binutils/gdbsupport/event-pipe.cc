/* Event pipe for GDB, the GNU debugger.

   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/event-pipe.h"
#include "gdbsupport/filestuff.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

event_pipe::~event_pipe ()
{
  if (is_open ())
    close_pipe ();
}

/* See event-pipe.h.  */

bool
event_pipe::open_pipe ()
{
  if (is_open ())
    return false;

  if (gdb_pipe_cloexec (m_fds) == -1)
    return false;

  if (fcntl (m_fds[0], F_SETFL, O_NONBLOCK) == -1
      || fcntl (m_fds[1], F_SETFL, O_NONBLOCK) == -1)
    {
      close_pipe ();
      return false;
    }

  return true;
}

/* See event-pipe.h.  */

void
event_pipe::close_pipe ()
{
  ::close (m_fds[0]);
  ::close (m_fds[1]);
  m_fds[0] = -1;
  m_fds[1] = -1;
}

/* See event-pipe.h.  */

void
event_pipe::flush ()
{
  int ret;
  char buf;

  do
    {
      ret = read (m_fds[0], &buf, 1);
    }
  while (ret >= 0 || (ret == -1 && errno == EINTR));
}

/* See event-pipe.h.  */

void
event_pipe::mark ()
{
  int ret;

  /* It doesn't really matter what the pipe contains, as long we end
     up with something in it.  Might as well flush the previous
     left-overs.  */
  flush ();

  do
    {
      ret = write (m_fds[1], "+", 1);
    }
  while (ret == -1 && errno == EINTR);

  /* Ignore EAGAIN.  If the pipe is full, the event loop will already
     be awakened anyway.  */
}
