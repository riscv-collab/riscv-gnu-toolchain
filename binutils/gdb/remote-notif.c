/* Remote notification in GDB protocol

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

/* Remote async notification is sent from remote target over RSP.
   Each type of notification is represented by an object of
   'struct notif', which has a field 'pending_reply'.  It is not
   NULL when GDB receives a notification from GDBserver, but hasn't
   acknowledge yet.  Before GDB acknowledges the notification,
   GDBserver shouldn't send notification again (see the header comments
   in gdbserver/notif.c).

   Notifications are processed in an almost-unified approach for both
   all-stop mode and non-stop mode, except the timing to process them.
   In non-stop mode, notifications are processed in
   remote_async_get_pending_events_handler, while in all-stop mode,
   they are processed in remote_resume.  */

#include "defs.h"
#include "remote.h"
#include "remote-notif.h"
#include "observable.h"
#include "gdbsupport/event-loop.h"
#include "target.h"
#include "inferior.h"
#include "infrun.h"
#include "gdbcmd.h"
#include "async-event.h"

bool notif_debug = false;

/* Supported clients of notifications.  */

static const notif_client *const notifs[] =
{
  &notif_client_stop,
};

static_assert (ARRAY_SIZE (notifs) == REMOTE_NOTIF_LAST);

/* Parse the BUF for the expected notification NC, and send packet to
   acknowledge.  */

void
remote_notif_ack (remote_target *remote,
		  const notif_client *nc, const char *buf)
{
  notif_event_up event = nc->alloc_event ();

  if (notif_debug)
    gdb_printf (gdb_stdlog, "notif: ack '%s'\n",
		nc->ack_command);

  nc->parse (remote, nc, buf, event.get ());
  nc->ack (remote, nc, buf, std::move (event));
}

/* Parse the BUF for the expected notification NC.  */

notif_event_up
remote_notif_parse (remote_target *remote,
		    const notif_client *nc, const char *buf)
{
  notif_event_up event = nc->alloc_event ();

  if (notif_debug)
    gdb_printf (gdb_stdlog, "notif: parse '%s'\n", nc->name);

  nc->parse (remote, nc, buf, event.get ());

  return event;
}

/* Process notifications in STATE's notification queue one by one.
   EXCEPT is not expected in the queue.  */

void
remote_notif_process (struct remote_notif_state *state,
		      const notif_client *except)
{
  while (!state->notif_queue.empty ())
    {
      const notif_client *nc = state->notif_queue.front ();
      state->notif_queue.pop_front ();

      gdb_assert (nc != except);

      if (nc->can_get_pending_events (state->remote, nc))
	remote_notif_get_pending_events (state->remote, nc);
    }
}

static void
remote_async_get_pending_events_handler (gdb_client_data data)
{
  remote_notif_state *notif_state = (remote_notif_state *) data;
  clear_async_event_handler (notif_state->get_pending_events_token);
  gdb_assert (remote_target_is_non_stop_p (notif_state->remote));
  remote_notif_process (notif_state, NULL);
}

/* Remote notification handler.  Parse BUF, queue notification and
   update STATE.  */

void
handle_notification (struct remote_notif_state *state, const char *buf)
{
  const notif_client *nc;
  size_t i;

  for (i = 0; i < ARRAY_SIZE (notifs); i++)
    {
      const char *name = notifs[i]->name;

      if (startswith (buf, name)
	  && buf[strlen (name)] == ':')
	break;
    }

  /* We ignore notifications we don't recognize, for compatibility
     with newer stubs.  */
  if (i == ARRAY_SIZE (notifs))
    return;

  nc =  notifs[i];

  if (state->pending_event[nc->id] != NULL)
    {
      /* We've already parsed the in-flight reply, but the stub for some
	 reason thought we didn't, possibly due to timeout on its side.
	 Just ignore it.  */
      if (notif_debug)
	gdb_printf (gdb_stdlog,
		    "notif: ignoring resent notification\n");
    }
  else
    {
      notif_event_up event
	= remote_notif_parse (state->remote, nc, buf + strlen (nc->name) + 1);

      /* Be careful to only set it after parsing, since an error
	 may be thrown then.  */
      state->pending_event[nc->id] = std::move (event);

      /* Notify the event loop there's a stop reply to acknowledge
	 and that there may be more events to fetch.  */
      state->notif_queue.push_back (nc);
      if (target_is_non_stop_p ())
	{
	  /* In non-stop, We mark REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN
	     in order to go on what we were doing and postpone
	     querying notification events to some point safe to do so.
	     See details in the function comment of
	     remote.c:remote_notif_get_pending_events.

	     In all-stop, GDB may be blocked to wait for the reply, we
	     shouldn't return to event loop until the expected reply
	     arrives.  For example:

	     1.1) --> vCont;c
	       GDB expects getting stop reply 'T05 thread:2'.
	     1.2) <-- %Notif
	       <GDB marks the REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN>

	     After step #1.2, we return to the event loop, which
	     notices there is a new event on the
	     REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN and calls the
	     handler, which will send 'vNotif' packet.
	     1.3) --> vNotif
	     It is not safe to start a new sequence, because target
	     is still running and GDB is expecting the stop reply
	     from stub.

	     To solve this, whenever we parse a notification
	     successfully, we don't mark the
	     REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN and let GDB blocked
	     there as before to get the sequence done.

	     2.1) --> vCont;c
	       GDB expects getting stop reply 'T05 thread:2'
	     2.2) <-- %Notif
	       <Don't mark the REMOTE_ASYNC_GET_PENDING_EVENTS_TOKEN>
	     2.3) <-- T05 thread:2

	     These pending notifications can be processed later.  */
	  mark_async_event_handler (state->get_pending_events_token);
	}

      if (notif_debug)
	gdb_printf (gdb_stdlog,
		    "notif: Notification '%s' captured\n",
		    nc->name);
    }
}

/* Return an allocated remote_notif_state.  */

struct remote_notif_state *
remote_notif_state_allocate (remote_target *remote)
{
  struct remote_notif_state *notif_state = new struct remote_notif_state;

  notif_state->remote = remote;

  /* Register async_event_handler for notification.  */

  notif_state->get_pending_events_token
    = create_async_event_handler (remote_async_get_pending_events_handler,
				  notif_state, "remote-notif");

  return notif_state;
}

/* Free STATE and its fields.  */

remote_notif_state::~remote_notif_state ()
{
  /* Unregister async_event_handler for notification.  */
  if (get_pending_events_token != NULL)
    delete_async_event_handler (&get_pending_events_token);
}

void _initialize_notif ();
void
_initialize_notif ()
{
  add_setshow_boolean_cmd ("notification", no_class, &notif_debug,
			   _("\
Set debugging of async remote notification."), _("\
Show debugging of async remote notification."), _("\
When non-zero, debugging output about async remote notifications"
" is enabled."),
			   NULL,
			   NULL,
			   &setdebuglist, &showdebuglist);
}
