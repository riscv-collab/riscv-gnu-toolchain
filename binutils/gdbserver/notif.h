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

#ifndef GDBSERVER_NOTIF_H
#define GDBSERVER_NOTIF_H

#include "target.h"
#include <list>

/* Structure holding information related to a single event.  We
   keep a queue of these to push to GDB.  It can be extended if
   the event of given notification contains more information.  */

struct notif_event
{
  virtual ~notif_event ()
  {
  }

  /* No payload needed.  */
};

/* A type notification to GDB.  An object of 'struct notif_server'
   represents a type of notification.  */

typedef struct notif_server
{
  /* The name of ack packet, for example, 'vStopped'.  */
  const char *ack_name;

  /* The notification packet, for example, '%Stop'.  Note that '%' is
     not in 'notif_name'.  */
  const char *notif_name;

  /* A queue of events to GDB.  A new notif_event can be enque'ed
     into QUEUE at any appropriate time, and the notif_reply is
     deque'ed only when the ack from GDB arrives.  */
  std::list<notif_event *> queue;

  /* Write event EVENT to OWN_BUF.  */
  void (*write) (struct notif_event *event, char *own_buf);
} *notif_server_p;

extern struct notif_server notif_stop;

int handle_notif_ack (char *own_buf, int packet_len);
void notif_write_event (struct notif_server *notif, char *own_buf);

void notif_push (struct notif_server *np, struct notif_event *event);
void notif_event_enque (struct notif_server *notif,
			struct notif_event *event);

#endif /* GDBSERVER_NOTIF_H */
