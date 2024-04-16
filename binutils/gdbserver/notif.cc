/* Notification to GDB.
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

/* Async notifications to GDB.  When the state of remote target is
   changed or something interesting to GDB happened, async
   notifications are used to tell GDB.

   Each type of notification is represented by an object
   'struct notif_server', in which there is a queue for events to GDB
   represented by 'struct notif_event'.  GDBserver writes (by means of
   'write' field) each event in the queue into the buffer and send the
   contents in buffer to GDB.  The contents in buffer is specified in
   RSP.  See more in the comments to field 'queue' of
   'struct notif_server'.

   Here is the workflow of sending events and managing queue:
   1.  At any time, when something interesting FOO happens, a object
   of 'struct notif_event' or its sub-class EVENT is created for FOO.

   2.  Enque EVENT to the 'queue' field of 'struct notif_server' for
   FOO and send corresponding notification packet to GDB if EVENT is
   the first one.
   #1 and #2 are done by function 'notif_push'.

   3.  EVENT is not deque'ed until the ack of FOO from GDB arrives.
   Before ack of FOO arrives, FOO happens again, a new object of
   EVENT is created and enque EVENT silently.
   Once GDB has a chance to ack to FOO, it sends an ack to GDBserver,
   and GDBserver repeatedly sends events to GDB and gets ack of FOO,
   until queue is empty.  Then, GDBserver sends 'OK' to GDB that all
   queued notification events are done.

   # 3 is done by function 'handle_notif_ack'.  */

#include "server.h"
#include "notif.h"

static struct notif_server *notifs[] =
{
  &notif_stop,
};

/* Write another event or an OK, if there are no more left, to
   OWN_BUF.  */

void
notif_write_event (struct notif_server *notif, char *own_buf)
{
  if (!notif->queue.empty ())
    {
      struct notif_event *event = notif->queue.front ();

      notif->write (event, own_buf);
    }
  else
    write_ok (own_buf);
}

/* Handle the ack in buffer OWN_BUF,and packet length is PACKET_LEN.
   Return 1 if the ack is handled, and return 0 if the contents
   in OWN_BUF is not a ack.  */

int
handle_notif_ack (char *own_buf, int packet_len)
{
  size_t i;
  struct notif_server *np;

  for (i = 0; i < ARRAY_SIZE (notifs); i++)
    {
      const char *ack_name = notifs[i]->ack_name;

      if (startswith (own_buf, ack_name)
	  && packet_len == strlen (ack_name))
	break;
    }

  if (i == ARRAY_SIZE (notifs))
    return 0;

  np = notifs[i];

  /* If we're waiting for GDB to acknowledge a pending event,
     consider that done.  */
  if (!np->queue.empty ())
    {
      struct notif_event *head = np->queue.front ();
      np->queue.pop_front ();

      remote_debug_printf ("%s: acking %d", np->ack_name,
			   (int) np->queue.size ());

      delete head;
    }

  notif_write_event (np, own_buf);

  return 1;
}

/* Put EVENT to the queue of NOTIF.  */

void
notif_event_enque (struct notif_server *notif,
		   struct notif_event *event)
{
  notif->queue.push_back (event);

  remote_debug_printf ("pending events: %s %d", notif->notif_name,
		       (int) notif->queue.size ());

}

/* Push one event NEW_EVENT of notification NP into NP->queue.  */

void
notif_push (struct notif_server *np, struct notif_event *new_event)
{
  bool is_first_event = np->queue.empty ();

  /* Something interesting.  Tell GDB about it.  */
  notif_event_enque (np, new_event);

  /* If this is the first stop reply in the queue, then inform GDB
     about it, by sending a corresponding notification.  */
  if (is_first_event)
    {
      char buf[PBUFSIZ];
      char *p = buf;

      xsnprintf (p, PBUFSIZ, "%s:", np->notif_name);
      p += strlen (p);

      np->write (new_event, p);
      putpkt_notif (buf);
    }
}
