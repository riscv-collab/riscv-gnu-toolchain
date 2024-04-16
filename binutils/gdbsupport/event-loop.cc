/* Event loop machinery for GDB, the GNU debugger.
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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/event-loop.h"

#include <chrono>

#ifdef HAVE_POLL
#if defined (HAVE_POLL_H)
#include <poll.h>
#elif defined (HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif
#endif

#include <sys/types.h>
#include "gdbsupport/gdb_sys_time.h"
#include "gdbsupport/gdb_select.h"
#include <optional>
#include "gdbsupport/scope-exit.h"

/* See event-loop.h.  */

debug_event_loop_kind debug_event_loop;

/* Tell create_file_handler what events we are interested in.
   This is used by the select version of the event loop.  */

#define GDB_READABLE	(1<<1)
#define GDB_WRITABLE	(1<<2)
#define GDB_EXCEPTION	(1<<3)

/* Information about each file descriptor we register with the event
   loop.  */

struct file_handler
{
  /* File descriptor.  */
  int fd;

  /* Events we want to monitor: POLLIN, etc.  */
  int mask;

  /* Events that have been seen since the last time.  */
  int ready_mask;

  /* Procedure to call when fd is ready.  */
  handler_func *proc;

  /* Argument to pass to proc.  */
  gdb_client_data client_data;

  /* User-friendly name of this handler.  */
  std::string name;

  /* If set, this file descriptor is used for a user interface.  */
  bool is_ui;

  /* Was an error detected on this fd?  */
  int error;

  /* Next registered file descriptor.  */
  struct file_handler *next_file;
};

#ifdef HAVE_POLL
/* Do we use poll or select?  Some systems have poll, but then it's
   not useable with all kinds of files.  We probe that whenever a new
   file handler is added.  */
static bool use_poll = true;
#endif

#ifdef USE_WIN32API
#include <windows.h>
#include <io.h>
#endif

/* Gdb_notifier is just a list of file descriptors gdb is interested in.
   These are the input file descriptor, and the target file
   descriptor.  We have two flavors of the notifier, one for platforms
   that have the POLL function, the other for those that don't, and
   only support SELECT.  Each of the elements in the gdb_notifier list is
   basically a description of what kind of events gdb is interested
   in, for each fd.  */

static struct
  {
    /* Ptr to head of file handler list.  */
    file_handler *first_file_handler;

    /* Next file handler to handle, for the select variant.  To level
       the fairness across event sources, we serve file handlers in a
       round-robin-like fashion.  The number and order of the polled
       file handlers may change between invocations, but this is good
       enough.  */
    file_handler *next_file_handler;

#ifdef HAVE_POLL
    /* Ptr to array of pollfd structures.  */
    struct pollfd *poll_fds;

    /* Next file descriptor to handle, for the poll variant.  To level
       the fairness across event sources, we poll the file descriptors
       in a round-robin-like fashion.  The number and order of the
       polled file descriptors may change between invocations, but
       this is good enough.  */
    int next_poll_fds_index;

    /* Timeout in milliseconds for calls to poll().  */
    int poll_timeout;
#endif

    /* Masks to be used in the next call to select.
       Bits are set in response to calls to create_file_handler.  */
    fd_set check_masks[3];

    /* What file descriptors were found ready by select.  */
    fd_set ready_masks[3];

    /* Number of file descriptors to monitor (for poll).  */
    /* Number of valid bits (highest fd value + 1) (for select).  */
    int num_fds;

    /* Time structure for calls to select().  */
    struct timeval select_timeout;

    /* Flag to tell whether the timeout should be used.  */
    int timeout_valid;
  }
gdb_notifier;

/* Structure associated with a timer.  PROC will be executed at the
   first occasion after WHEN.  */
struct gdb_timer
  {
    std::chrono::steady_clock::time_point when;
    int timer_id;
    struct gdb_timer *next;
    timer_handler_func *proc;	    /* Function to call to do the work.  */
    gdb_client_data client_data;    /* Argument to async_handler_func.  */
  };

/* List of currently active timers.  It is sorted in order of
   increasing timers.  */
static struct
  {
    /* Pointer to first in timer list.  */
    struct gdb_timer *first_timer;

    /* Id of the last timer created.  */
    int num_timers;
  }
timer_list;

static void create_file_handler (int fd, int mask, handler_func *proc,
				 gdb_client_data client_data,
				 std::string &&name, bool is_ui);
static int gdb_wait_for_event (int);
static int update_wait_timeout (void);
static int poll_timers (void);

/* Process one high level event.  If nothing is ready at this time,
   wait at most MSTIMEOUT milliseconds for something to happen (via
   gdb_wait_for_event), then process it.  Returns >0 if something was
   done, <0 if there are no event sources to wait for, =0 if timeout occurred.
   A timeout of 0 allows to serve an already pending event, but does not
   wait if none found.
   Setting the timeout to a negative value disables it.
   The timeout is never used by gdb itself, it is however needed to
   integrate gdb event handling within Insight's GUI event loop. */

int
gdb_do_one_event (int mstimeout)
{
  static int event_source_head = 0;
  const int number_of_sources = 3;
  int current = 0;

  /* First let's see if there are any asynchronous signal handlers
     that are ready.  These would be the result of invoking any of the
     signal handlers.  */
  if (invoke_async_signal_handlers ())
    return 1;

  /* To level the fairness across event sources, we poll them in a
     round-robin fashion.  */
  for (current = 0; current < number_of_sources; current++)
    {
      int res;

      switch (event_source_head)
	{
	case 0:
	  /* Are any timers that are ready?  */
	  res = poll_timers ();
	  break;
	case 1:
	  /* Are there events already waiting to be collected on the
	     monitored file descriptors?  */
	  res = gdb_wait_for_event (0);
	  break;
	case 2:
	  /* Are there any asynchronous event handlers ready?  */
	  res = check_async_event_handlers ();
	  break;
	default:
	  internal_error ("unexpected event_source_head %d",
			  event_source_head);
	}

      event_source_head++;
      if (event_source_head == number_of_sources)
	event_source_head = 0;

      if (res > 0)
	return 1;
    }

  if (mstimeout == 0)
    return 0;	/* 0ms timeout: do not wait for an event. */

  /* Block waiting for a new event.  If gdb_wait_for_event returns -1,
     we should get out because this means that there are no event
     sources left.  This will make the event loop stop, and the
     application exit.
     If a timeout has been given, a new timer is set accordingly
     to abort event wait.  It is deleted upon gdb_wait_for_event
     termination and thus should never be triggered.
     When the timeout is reached, events are not monitored again:
     they already have been checked in the loop above. */

  std::optional<int> timer_id;

  SCOPE_EXIT 
    {
      if (timer_id.has_value ())
	delete_timer (*timer_id);
    };

  if (mstimeout > 0)
    timer_id = create_timer (mstimeout,
			     [] (gdb_client_data arg)
			     {
			       ((std::optional<int> *) arg)->reset ();
			     },
			     &timer_id);
  return gdb_wait_for_event (1);
}

/* See event-loop.h  */

void
add_file_handler (int fd, handler_func *proc, gdb_client_data client_data,
		  std::string &&name, bool is_ui)
{
#ifdef HAVE_POLL
  if (use_poll)
    {
      struct pollfd fds;

      /* Check to see if poll () is usable.  If not, we'll switch to
	 use select.  This can happen on systems like
	 m68k-motorola-sys, `poll' cannot be used to wait for `stdin'.
	 On m68k-motorola-sysv, tty's are not stream-based and not
	 `poll'able.  */
      fds.fd = fd;
      fds.events = POLLIN;
      if (poll (&fds, 1, 0) == 1 && (fds.revents & POLLNVAL))
	use_poll = false;
    }
  if (use_poll)
    {
      create_file_handler (fd, POLLIN, proc, client_data, std::move (name),
			   is_ui);
    }
  else
#endif /* HAVE_POLL */
    create_file_handler (fd, GDB_READABLE | GDB_EXCEPTION,
			 proc, client_data, std::move (name), is_ui);
}

/* Helper for add_file_handler.

   For the poll case, MASK is a combination (OR) of POLLIN,
   POLLRDNORM, POLLRDBAND, POLLPRI, POLLOUT, POLLWRNORM, POLLWRBAND:
   these are the events we are interested in.  If any of them occurs,
   proc should be called.

   For the select case, MASK is a combination of READABLE, WRITABLE,
   EXCEPTION.  PROC is the procedure that will be called when an event
   occurs for FD.  CLIENT_DATA is the argument to pass to PROC.  */

static void
create_file_handler (int fd, int mask, handler_func * proc,
		     gdb_client_data client_data, std::string &&name,
		     bool is_ui)
{
  file_handler *file_ptr;

  /* Do we already have a file handler for this file?  (We may be
     changing its associated procedure).  */
  for (file_ptr = gdb_notifier.first_file_handler; file_ptr != NULL;
       file_ptr = file_ptr->next_file)
    {
      if (file_ptr->fd == fd)
	break;
    }

  /* It is a new file descriptor.  Add it to the list.  Otherwise, just
     change the data associated with it.  */
  if (file_ptr == NULL)
    {
      file_ptr = new file_handler;
      file_ptr->fd = fd;
      file_ptr->ready_mask = 0;
      file_ptr->next_file = gdb_notifier.first_file_handler;
      gdb_notifier.first_file_handler = file_ptr;

#ifdef HAVE_POLL
      if (use_poll)
	{
	  gdb_notifier.num_fds++;
	  if (gdb_notifier.poll_fds)
	    gdb_notifier.poll_fds =
	      (struct pollfd *) xrealloc (gdb_notifier.poll_fds,
					  (gdb_notifier.num_fds
					   * sizeof (struct pollfd)));
	  else
	    gdb_notifier.poll_fds =
	      XNEW (struct pollfd);
	  (gdb_notifier.poll_fds + gdb_notifier.num_fds - 1)->fd = fd;
	  (gdb_notifier.poll_fds + gdb_notifier.num_fds - 1)->events = mask;
	  (gdb_notifier.poll_fds + gdb_notifier.num_fds - 1)->revents = 0;
	}
      else
#endif /* HAVE_POLL */
	{
	  if (mask & GDB_READABLE)
	    FD_SET (fd, &gdb_notifier.check_masks[0]);
	  else
	    FD_CLR (fd, &gdb_notifier.check_masks[0]);

	  if (mask & GDB_WRITABLE)
	    FD_SET (fd, &gdb_notifier.check_masks[1]);
	  else
	    FD_CLR (fd, &gdb_notifier.check_masks[1]);

	  if (mask & GDB_EXCEPTION)
	    FD_SET (fd, &gdb_notifier.check_masks[2]);
	  else
	    FD_CLR (fd, &gdb_notifier.check_masks[2]);

	  if (gdb_notifier.num_fds <= fd)
	    gdb_notifier.num_fds = fd + 1;
	}
    }

  file_ptr->proc = proc;
  file_ptr->client_data = client_data;
  file_ptr->mask = mask;
  file_ptr->name = std::move (name);
  file_ptr->is_ui = is_ui;
}

/* Return the next file handler to handle, and advance to the next
   file handler, wrapping around if the end of the list is
   reached.  */

static file_handler *
get_next_file_handler_to_handle_and_advance (void)
{
  file_handler *curr_next;

  /* The first time around, this is still NULL.  */
  if (gdb_notifier.next_file_handler == NULL)
    gdb_notifier.next_file_handler = gdb_notifier.first_file_handler;

  curr_next = gdb_notifier.next_file_handler;
  gdb_assert (curr_next != NULL);

  /* Advance.  */
  gdb_notifier.next_file_handler = curr_next->next_file;
  /* Wrap around, if necessary.  */
  if (gdb_notifier.next_file_handler == NULL)
    gdb_notifier.next_file_handler = gdb_notifier.first_file_handler;

  return curr_next;
}

/* Remove the file descriptor FD from the list of monitored fd's: 
   i.e. we don't care anymore about events on the FD.  */
void
delete_file_handler (int fd)
{
  file_handler *file_ptr, *prev_ptr = NULL;
  int i;

  /* Find the entry for the given file.  */

  for (file_ptr = gdb_notifier.first_file_handler; file_ptr != NULL;
       file_ptr = file_ptr->next_file)
    {
      if (file_ptr->fd == fd)
	break;
    }

  if (file_ptr == NULL)
    return;

#ifdef HAVE_POLL
  if (use_poll)
    {
      int j;
      struct pollfd *new_poll_fds;

      /* Create a new poll_fds array by copying every fd's information
	 but the one we want to get rid of.  */

      new_poll_fds = (struct pollfd *) 
	xmalloc ((gdb_notifier.num_fds - 1) * sizeof (struct pollfd));

      for (i = 0, j = 0; i < gdb_notifier.num_fds; i++)
	{
	  if ((gdb_notifier.poll_fds + i)->fd != fd)
	    {
	      (new_poll_fds + j)->fd = (gdb_notifier.poll_fds + i)->fd;
	      (new_poll_fds + j)->events = (gdb_notifier.poll_fds + i)->events;
	      (new_poll_fds + j)->revents
		= (gdb_notifier.poll_fds + i)->revents;
	      j++;
	    }
	}
      xfree (gdb_notifier.poll_fds);
      gdb_notifier.poll_fds = new_poll_fds;
      gdb_notifier.num_fds--;
    }
  else
#endif /* HAVE_POLL */
    {
      if (file_ptr->mask & GDB_READABLE)
	FD_CLR (fd, &gdb_notifier.check_masks[0]);
      if (file_ptr->mask & GDB_WRITABLE)
	FD_CLR (fd, &gdb_notifier.check_masks[1]);
      if (file_ptr->mask & GDB_EXCEPTION)
	FD_CLR (fd, &gdb_notifier.check_masks[2]);

      /* Find current max fd.  */

      if ((fd + 1) == gdb_notifier.num_fds)
	{
	  gdb_notifier.num_fds--;
	  for (i = gdb_notifier.num_fds; i; i--)
	    {
	      if (FD_ISSET (i - 1, &gdb_notifier.check_masks[0])
		  || FD_ISSET (i - 1, &gdb_notifier.check_masks[1])
		  || FD_ISSET (i - 1, &gdb_notifier.check_masks[2]))
		break;
	    }
	  gdb_notifier.num_fds = i;
	}
    }

  /* Deactivate the file descriptor, by clearing its mask, 
     so that it will not fire again.  */

  file_ptr->mask = 0;

  /* If this file handler was going to be the next one to be handled,
     advance to the next's next, if any.  */
  if (gdb_notifier.next_file_handler == file_ptr)
    {
      if (file_ptr->next_file == NULL
	  && file_ptr == gdb_notifier.first_file_handler)
	gdb_notifier.next_file_handler = NULL;
      else
	get_next_file_handler_to_handle_and_advance ();
    }

  /* Get rid of the file handler in the file handler list.  */
  if (file_ptr == gdb_notifier.first_file_handler)
    gdb_notifier.first_file_handler = file_ptr->next_file;
  else
    {
      for (prev_ptr = gdb_notifier.first_file_handler;
	   prev_ptr->next_file != file_ptr;
	   prev_ptr = prev_ptr->next_file)
	;
      prev_ptr->next_file = file_ptr->next_file;
    }

  delete file_ptr;
}

/* Handle the given event by calling the procedure associated to the
   corresponding file handler.  */

static void
handle_file_event (file_handler *file_ptr, int ready_mask)
{
  int mask;

  /* See if the desired events (mask) match the received events
     (ready_mask).  */

#ifdef HAVE_POLL
  if (use_poll)
    {
      int error_mask;

      /* With poll, the ready_mask could have any of three events set
	 to 1: POLLHUP, POLLERR, POLLNVAL.  These events cannot be
	 used in the requested event mask (events), but they can be
	 returned in the return mask (revents).  We need to check for
	 those event too, and add them to the mask which will be
	 passed to the handler.  */

      /* POLLHUP means EOF, but can be combined with POLLIN to
	 signal more data to read.  */
      error_mask = POLLHUP | POLLERR | POLLNVAL;
      mask = ready_mask & (file_ptr->mask | error_mask);

      if ((mask & (POLLERR | POLLNVAL)) != 0)
	{
	  /* Work in progress.  We may need to tell somebody
	     what kind of error we had.  */
	  if (mask & POLLERR)
	    warning (_("Error detected on fd %d"), file_ptr->fd);
	  if (mask & POLLNVAL)
	    warning (_("Invalid or non-`poll'able fd %d"),
		     file_ptr->fd);
	  file_ptr->error = 1;
	}
      else
	file_ptr->error = 0;
    }
  else
#endif /* HAVE_POLL */
    {
      if (ready_mask & GDB_EXCEPTION)
	{
	  warning (_("Exception condition detected on fd %d"),
		   file_ptr->fd);
	  file_ptr->error = 1;
	}
      else
	file_ptr->error = 0;
      mask = ready_mask & file_ptr->mask;
    }

  /* If there was a match, then call the handler.  */
  if (mask != 0)
    {
      event_loop_ui_debug_printf (file_ptr->is_ui,
				  "invoking fd file handler `%s`",
				  file_ptr->name.c_str ());
      file_ptr->proc (file_ptr->error, file_ptr->client_data);
    }
}

/* Wait for new events on the monitored file descriptors.  Run the
   event handler if the first descriptor that is detected by the poll.
   If BLOCK and if there are no events, this function will block in
   the call to poll.  Return 1 if an event was handled.  Return -1 if
   there are no file descriptors to monitor.  Return 1 if an event was
   handled, otherwise returns 0.  */

static int
gdb_wait_for_event (int block)
{
  file_handler *file_ptr;
  int num_found = 0;

  /* Make sure all output is done before getting another event.  */
  flush_streams ();

  if (gdb_notifier.num_fds == 0)
    return -1;

  if (block)
    update_wait_timeout ();

#ifdef HAVE_POLL
  if (use_poll)
    {
      int timeout;

      if (block)
	timeout = gdb_notifier.timeout_valid ? gdb_notifier.poll_timeout : -1;
      else
	timeout = 0;

      num_found = poll (gdb_notifier.poll_fds,
			(unsigned long) gdb_notifier.num_fds, timeout);

      /* Don't print anything if we get out of poll because of a
	 signal.  */
      if (num_found == -1 && errno != EINTR)
	perror_with_name (("poll"));
    }
  else
#endif /* HAVE_POLL */
    {
      struct timeval select_timeout;
      struct timeval *timeout_p;

      if (block)
	timeout_p = gdb_notifier.timeout_valid
	  ? &gdb_notifier.select_timeout : NULL;
      else
	{
	  memset (&select_timeout, 0, sizeof (select_timeout));
	  timeout_p = &select_timeout;
	}

      gdb_notifier.ready_masks[0] = gdb_notifier.check_masks[0];
      gdb_notifier.ready_masks[1] = gdb_notifier.check_masks[1];
      gdb_notifier.ready_masks[2] = gdb_notifier.check_masks[2];
      num_found = gdb_select (gdb_notifier.num_fds,
			      &gdb_notifier.ready_masks[0],
			      &gdb_notifier.ready_masks[1],
			      &gdb_notifier.ready_masks[2],
			      timeout_p);

      /* Clear the masks after an error from select.  */
      if (num_found == -1)
	{
	  FD_ZERO (&gdb_notifier.ready_masks[0]);
	  FD_ZERO (&gdb_notifier.ready_masks[1]);
	  FD_ZERO (&gdb_notifier.ready_masks[2]);

	  /* Dont print anything if we got a signal, let gdb handle
	     it.  */
	  if (errno != EINTR)
	    perror_with_name (("select"));
	}
    }

  /* Avoid looking at poll_fds[i]->revents if no event fired.  */
  if (num_found <= 0)
    return 0;

  /* Run event handlers.  We always run just one handler and go back
     to polling, in case a handler changes the notifier list.  Since
     events for sources we haven't consumed yet wake poll/select
     immediately, no event is lost.  */

  /* To level the fairness across event descriptors, we handle them in
     a round-robin-like fashion.  The number and order of descriptors
     may change between invocations, but this is good enough.  */
#ifdef HAVE_POLL
  if (use_poll)
    {
      int i;
      int mask;

      while (1)
	{
	  if (gdb_notifier.next_poll_fds_index >= gdb_notifier.num_fds)
	    gdb_notifier.next_poll_fds_index = 0;
	  i = gdb_notifier.next_poll_fds_index++;

	  gdb_assert (i < gdb_notifier.num_fds);
	  if ((gdb_notifier.poll_fds + i)->revents)
	    break;
	}

      for (file_ptr = gdb_notifier.first_file_handler;
	   file_ptr != NULL;
	   file_ptr = file_ptr->next_file)
	{
	  if (file_ptr->fd == (gdb_notifier.poll_fds + i)->fd)
	    break;
	}
      gdb_assert (file_ptr != NULL);

      mask = (gdb_notifier.poll_fds + i)->revents;
      handle_file_event (file_ptr, mask);
      return 1;
    }
  else
#endif /* HAVE_POLL */
    {
      /* See comment about even source fairness above.  */
      int mask = 0;

      do
	{
	  file_ptr = get_next_file_handler_to_handle_and_advance ();

	  if (FD_ISSET (file_ptr->fd, &gdb_notifier.ready_masks[0]))
	    mask |= GDB_READABLE;
	  if (FD_ISSET (file_ptr->fd, &gdb_notifier.ready_masks[1]))
	    mask |= GDB_WRITABLE;
	  if (FD_ISSET (file_ptr->fd, &gdb_notifier.ready_masks[2]))
	    mask |= GDB_EXCEPTION;
	}
      while (mask == 0);

      handle_file_event (file_ptr, mask);
      return 1;
    }
  return 0;
}

/* Create a timer that will expire in MS milliseconds from now.  When
   the timer is ready, PROC will be executed.  At creation, the timer
   is added to the timers queue.  This queue is kept sorted in order
   of increasing timers.  Return a handle to the timer struct.  */

int
create_timer (int ms, timer_handler_func *proc,
	      gdb_client_data client_data)
{
  using namespace std::chrono;
  struct gdb_timer *timer_ptr, *timer_index, *prev_timer;

  steady_clock::time_point time_now = steady_clock::now ();

  timer_ptr = new gdb_timer ();
  timer_ptr->when = time_now + milliseconds (ms);
  timer_ptr->proc = proc;
  timer_ptr->client_data = client_data;
  timer_list.num_timers++;
  timer_ptr->timer_id = timer_list.num_timers;

  /* Now add the timer to the timer queue, making sure it is sorted in
     increasing order of expiration.  */

  for (timer_index = timer_list.first_timer;
       timer_index != NULL;
       timer_index = timer_index->next)
    {
      if (timer_index->when > timer_ptr->when)
	break;
    }

  if (timer_index == timer_list.first_timer)
    {
      timer_ptr->next = timer_list.first_timer;
      timer_list.first_timer = timer_ptr;

    }
  else
    {
      for (prev_timer = timer_list.first_timer;
	   prev_timer->next != timer_index;
	   prev_timer = prev_timer->next)
	;

      prev_timer->next = timer_ptr;
      timer_ptr->next = timer_index;
    }

  gdb_notifier.timeout_valid = 0;
  return timer_ptr->timer_id;
}

/* There is a chance that the creator of the timer wants to get rid of
   it before it expires.  */
void
delete_timer (int id)
{
  struct gdb_timer *timer_ptr, *prev_timer = NULL;

  /* Find the entry for the given timer.  */

  for (timer_ptr = timer_list.first_timer; timer_ptr != NULL;
       timer_ptr = timer_ptr->next)
    {
      if (timer_ptr->timer_id == id)
	break;
    }

  if (timer_ptr == NULL)
    return;
  /* Get rid of the timer in the timer list.  */
  if (timer_ptr == timer_list.first_timer)
    timer_list.first_timer = timer_ptr->next;
  else
    {
      for (prev_timer = timer_list.first_timer;
	   prev_timer->next != timer_ptr;
	   prev_timer = prev_timer->next)
	;
      prev_timer->next = timer_ptr->next;
    }
  delete timer_ptr;

  gdb_notifier.timeout_valid = 0;
}

/* Convert a std::chrono duration to a struct timeval.  */

template<typename Duration>
static struct timeval
duration_cast_timeval (const Duration &d)
{
  using namespace std::chrono;
  seconds sec = duration_cast<seconds> (d);
  microseconds msec = duration_cast<microseconds> (d - sec);

  struct timeval tv;
  tv.tv_sec = sec.count ();
  tv.tv_usec = msec.count ();
  return tv;
}

/* Update the timeout for the select() or poll().  Returns true if the
   timer has already expired, false otherwise.  */

static int
update_wait_timeout (void)
{
  if (timer_list.first_timer != NULL)
    {
      using namespace std::chrono;
      steady_clock::time_point time_now = steady_clock::now ();
      struct timeval timeout;

      if (timer_list.first_timer->when < time_now)
	{
	  /* It expired already.  */
	  timeout.tv_sec = 0;
	  timeout.tv_usec = 0;
	}
      else
	{
	  steady_clock::duration d = timer_list.first_timer->when - time_now;
	  timeout = duration_cast_timeval (d);
	}

      /* Update the timeout for select/ poll.  */
#ifdef HAVE_POLL
      if (use_poll)
	gdb_notifier.poll_timeout = timeout.tv_sec * 1000;
      else
#endif /* HAVE_POLL */
	{
	  gdb_notifier.select_timeout.tv_sec = timeout.tv_sec;
	  gdb_notifier.select_timeout.tv_usec = timeout.tv_usec;
	}
      gdb_notifier.timeout_valid = 1;

      if (timer_list.first_timer->when < time_now)
	return 1;
    }
  else
    gdb_notifier.timeout_valid = 0;

  return 0;
}

/* Check whether a timer in the timers queue is ready.  If a timer is
   ready, call its handler and return.  Update the timeout for the
   select() or poll() as well.  Return 1 if an event was handled,
   otherwise returns 0.*/

static int
poll_timers (void)
{
  if (update_wait_timeout ())
    {
      struct gdb_timer *timer_ptr = timer_list.first_timer;
      timer_handler_func *proc = timer_ptr->proc;
      gdb_client_data client_data = timer_ptr->client_data;

      /* Get rid of the timer from the beginning of the list.  */
      timer_list.first_timer = timer_ptr->next;

      /* Delete the timer before calling the callback, not after, in
	 case the callback itself decides to try deleting the timer
	 too.  */
      delete timer_ptr;

      /* Call the procedure associated with that timer.  */
      (proc) (client_data);

      return 1;
    }

  return 0;
}
