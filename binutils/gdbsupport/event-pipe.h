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

#ifndef COMMON_EVENT_PIPE_H
#define COMMON_EVENT_PIPE_H

/* An event pipe used as a waitable file in the event loop in place of
   some other event associated with a signal.  The handler for the
   signal marks the event pipe to force a wakeup in the event loop.
   This uses the well-known self-pipe trick.  */

class event_pipe
{
public:
  event_pipe() = default;
  ~event_pipe();

  DISABLE_COPY_AND_ASSIGN (event_pipe);

  /* Create a new pipe.  */
  bool open_pipe ();

  /* Close the pipe.  */
  void close_pipe ();

  /* True if the event pipe has been opened.  */
  bool is_open () const
  { return m_fds[0] != -1; }

  /* The file descriptor of the waitable file to use in the event
     loop.  */
  int event_fd () const
  { return m_fds[0]; }

  /* Flush the event pipe.  */
  void flush ();

  /* Put something in the pipe, so the event loop wakes up.  */
  void mark ();
private:
  int m_fds[2] = { -1, -1 };
};

#endif /* COMMON_EVENT_PIPE_H */
