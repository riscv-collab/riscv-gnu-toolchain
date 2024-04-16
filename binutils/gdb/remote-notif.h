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

#ifndef REMOTE_NOTIF_H
#define REMOTE_NOTIF_H

#include <list>
#include <memory>

/* An event of a type of async remote notification.  */

struct notif_event
{
  virtual ~notif_event ()
  {
  }
};

/* A unique pointer holding a notif_event.  */

typedef std::unique_ptr<notif_event> notif_event_up;

/* ID of the notif_client.  */

enum REMOTE_NOTIF_ID
{
  REMOTE_NOTIF_STOP = 0,
  REMOTE_NOTIF_LAST,
};

struct remote_target;

/* A client to a sort of async remote notification.  */

struct notif_client
{
  /* The name of notification packet.  */
  const char *name;

  /* The packet to acknowledge a previous reply.  */
  const char *ack_command;

  /* Parse BUF to get the expected event and update EVENT.  This
     function may throw exception if contents in BUF is not the
     expected event.  */
  void (*parse) (remote_target *remote,
		 const notif_client *self, const char *buf,
		 struct notif_event *event);

  /* Send field <ack_command> to remote, and do some checking.  If
     something wrong, throw an exception.  */
  void (*ack) (remote_target *remote,
	       const notif_client *self, const char *buf,
	       notif_event_up event);

  /* Check this notification client can get pending events in
     'remote_notif_process'.  */
  int (*can_get_pending_events) (remote_target *remote,
				 const notif_client *self);

  /* Allocate an event.  */
  notif_event_up (*alloc_event) ();

  /* Id of this notif_client.  */
  const enum REMOTE_NOTIF_ID id;
};

/* State on remote async notification.  */

struct remote_notif_state
{
  remote_notif_state () = default;
  ~remote_notif_state ();

  DISABLE_COPY_AND_ASSIGN (remote_notif_state);

  /* The remote target.  */
  remote_target *remote;

  /* Notification queue.  */

  std::list<const notif_client *> notif_queue;

  /* Asynchronous signal handle registered as event loop source for when
     the remote sent us a notification.  The registered callback
     will do a ACK sequence to pull the rest of the events out of
     the remote side into our event queue.  */

  struct async_event_handler *get_pending_events_token;

  /* One pending event for each notification client.  This is where we
     keep it until it is acknowledged.  When there is a notification
     packet, parse it, and create an object of 'struct notif_event' to
     assign to it.  This field is unchanged until GDB starts to ack
     this notification (which is done by
     remote.c:remote_notif_pending_replies).  */

  notif_event_up pending_event[REMOTE_NOTIF_LAST];
};

void remote_notif_ack (remote_target *remote, const notif_client *nc,
		       const char *buf);
notif_event_up remote_notif_parse (remote_target *remote,
				   const notif_client *nc,
				   const char *buf);

void handle_notification (struct remote_notif_state *notif_state,
			  const char *buf);

void remote_notif_process (struct remote_notif_state *state,
			   const notif_client *except);
remote_notif_state *remote_notif_state_allocate (remote_target *remote);

extern const notif_client notif_client_stop;

extern bool notif_debug;

#endif /* REMOTE_NOTIF_H */
