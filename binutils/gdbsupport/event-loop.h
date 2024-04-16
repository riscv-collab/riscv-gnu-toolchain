/* Definitions used by the GDB event loop.
   Copyright (C) 1999-2024 Free Software Foundation, Inc.
   Written by Elena Zannoni <ezannoni@cygnus.com> of Cygnus Solutions.

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

#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

/* An event loop listens for events from multiple event sources.  When
   an event arrives, it is queued and processed by calling the
   appropriate event handler.  The event loop then continues to listen
   for more events.  An event loop completes when there are no event
   sources to listen on.  External event sources can be plugged into
   the loop.

   There are 4 main components:
   - a list of file descriptors to be monitored, GDB_NOTIFIER.
   - a list of asynchronous event sources to be monitored,
     ASYNC_EVENT_HANDLER_LIST.
   - a list of events that have occurred, EVENT_QUEUE.
   - a list of signal handling functions, SIGHANDLER_LIST.

   GDB_NOTIFIER keeps track of the file descriptor based event
   sources.  ASYNC_EVENT_HANDLER_LIST keeps track of asynchronous
   event sources that are signalled by some component of gdb, usually
   a target_ops instance.  Event sources for gdb are currently the UI
   and the target.  Gdb communicates with the command line user
   interface via the readline library and usually communicates with
   remote targets via a serial port.  Serial ports are represented in
   GDB as file descriptors and select/poll calls.  For native targets
   instead, the communication varies across operating system debug
   APIs, but usually consists of calls to ptrace and waits (via
   signals) or calls to poll/select (via file descriptors).  In the
   current gdb, the code handling events related to the target resides
   in wait_for_inferior for synchronous targets; or, for asynchronous
   capable targets, by having the target register either a target
   controlled file descriptor and/or an asynchronous event source in
   the event loop, with the fetch_inferior_event function as the event
   callback.  In both the synchronous and asynchronous cases, usually
   the target event is collected through the target_wait interface.
   The target is free to install other event sources in the event loop
   if it so requires.

   EVENT_QUEUE keeps track of the events that have happened during the
   last iteration of the event loop, and need to be processed.  An
   event is represented by a procedure to be invoked in order to
   process the event.  The queue is scanned head to tail.  If the
   event of interest is a change of state in a file descriptor, then a
   call to poll or select will be made to detect it.

   If the events generate signals, they are also queued by special
   functions that are invoked through traditional signal handlers.
   The actions to be taken is response to such events will be executed
   when the SIGHANDLER_LIST is scanned, the next time through the
   infinite loop.

   Corollary tasks are the creation and deletion of event sources.  */

typedef void *gdb_client_data;
typedef void (handler_func) (int, gdb_client_data);
typedef void (timer_handler_func) (gdb_client_data);

/* Exported functions from event-loop.c */

extern int gdb_do_one_event (int mstimeout = -1);
extern void delete_file_handler (int fd);

/* Add a file handler/descriptor to the list of descriptors we are
   interested in.

   FD is the file descriptor for the file/stream to be listened to.

   NAME is a user-friendly name for the handler.

   If IS_UI is set, this file descriptor is used for a user interface.  */

extern void add_file_handler (int fd, handler_func *proc,
			      gdb_client_data client_data,
			      std::string &&name, bool is_ui = false);

extern int create_timer (int milliseconds, 
			 timer_handler_func *proc, 
			 gdb_client_data client_data);
extern void delete_timer (int id);

/* Must be defined by client.  */

extern void handle_event_loop_exception (const gdb_exception &);

/* Must be defined by client.  Returns true if any signal handler was
   ready.  */

extern int invoke_async_signal_handlers ();

/* Must be defined by client.  Returns true if any event handler was
   ready.  */

extern int check_async_event_handlers ();

enum class debug_event_loop_kind
{
  OFF,

  /* Print all event-loop related messages, except events from user-interface
     event sources.  */
  ALL_EXCEPT_UI,

  /* Print all event-loop related messages.  */
  ALL,
};

/* True if we are printing event loop debug statements.  */
extern debug_event_loop_kind debug_event_loop;

/* Print an "event loop" debug statement.  */

#define event_loop_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (debug_event_loop != debug_event_loop_kind::OFF, \
			      "event-loop", fmt, ##__VA_ARGS__)

/* Print an "event loop" debug statement that is know to come from a UI-related
   event (e.g. calling the event handler for the fd of the CLI).  */

#define event_loop_ui_debug_printf(is_ui, fmt, ...) \
  do \
    { \
      if (debug_event_loop == debug_event_loop_kind::ALL \
	  || (debug_event_loop == debug_event_loop_kind::ALL_EXCEPT_UI \
	      && !is_ui)) \
	debug_prefixed_printf ("event-loop", __func__, fmt, ##__VA_ARGS__); \
    } \
  while (0)

#endif /* EVENT_LOOP_H */
