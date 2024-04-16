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

#ifndef SER_EVENT_H
#define SER_EVENT_H

/* This is used to be able to signal the event loop (or any other
   select/poll) of events, in a race-free manner.

   For example, a signal handler can defer non-async-signal-safe work
   to the event loop, by having the signal handler set a struct
   serial_event object, and having the event loop wait for that same
   object to the readable.  Once readable, the event loop breaks out
   of select/poll and calls a registered callback that does the
   deferred work.  */

struct serial_event;

/* Make a new serial_event object.  */
struct serial_event *make_serial_event (void);

/* Return the FD that can be used by select/poll to wait for the
   event.  The only valid operation on this object is to wait until it
   is readable.  */
extern int serial_event_fd (struct serial_event *event);

/* Set the event.  This signals the file descriptor returned by
   serial_event_fd as readable.  */
extern void serial_event_set (struct serial_event *event);

/* Clear the event.  The file descriptor returned by serial_event_fd
   is not longer readable after this, until a new serial_event_set
   call is made.  */
extern void serial_event_clear (struct serial_event *event);

#endif
