/* Generic serial interface functions.

   Copyright (C) 1992-2024 Free Software Foundation, Inc.

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
#include "serial.h"
#include "ser-base.h"
#include "gdbsupport/event-loop.h"

#include "gdbsupport/gdb_select.h"
#include "gdbsupport/gdb_sys_time.h"
#ifdef USE_WIN32API
#include <winsock2.h>
#endif


static timer_handler_func push_event;
static handler_func fd_event;

/* Event handling for ASYNC serial code.

   At any time the SERIAL device either: has an empty FIFO and is
   waiting on a FD event; or has a non-empty FIFO/error condition and
   is constantly scheduling timer events.

   ASYNC only stops pestering its client when it is de-async'ed or it
   is told to go away.  */

/* Value of scb->async_state: */
enum {
  /* When >= 0, this contains the ID of the currently scheduled timer event.
     This state is rarely encountered.  Timer events are one-off so as soon as
     the event is delivered the state is changed to NOTHING_SCHEDULED.  */

  /* The fd_event() handler is scheduled.  It is called when ever the
     file descriptor becomes ready.  */
  FD_SCHEDULED = -1,

  /* Either no task is scheduled (just going into ASYNC mode) or a
     timer event has just gone off and the current state has been
     forced into nothing scheduled.  */
  NOTHING_SCHEDULED = -2
};

/* Identify and schedule the next ASYNC task based on scb->async_state
   and scb->buf* (the input FIFO).  A state machine is used to avoid
   the need to make redundant calls into the event-loop - the next
   scheduled task is only changed when needed.  */

static void
reschedule (struct serial *scb)
{
  if (serial_is_async_p (scb))
    {
      int next_state;

      switch (scb->async_state)
	{
	case FD_SCHEDULED:
	  if (scb->bufcnt == 0)
	    next_state = FD_SCHEDULED;
	  else
	    {
	      delete_file_handler (scb->fd);
	      next_state = create_timer (0, push_event, scb);
	    }
	  break;
	case NOTHING_SCHEDULED:
	  if (scb->bufcnt == 0)
	    {
	      add_file_handler (scb->fd, fd_event, scb, "serial");
	      next_state = FD_SCHEDULED;
	    }
	  else
	    {
	      next_state = create_timer (0, push_event, scb);
	    }
	  break;
	default: /* TIMER SCHEDULED */
	  if (scb->bufcnt == 0)
	    {
	      delete_timer (scb->async_state);
	      add_file_handler (scb->fd, fd_event, scb, "serial");
	      next_state = FD_SCHEDULED;
	    }
	  else
	    next_state = scb->async_state;
	  break;
	}
      if (serial_debug_p (scb))
	{
	  switch (next_state)
	    {
	    case FD_SCHEDULED:
	      if (scb->async_state != FD_SCHEDULED)
		gdb_printf (gdb_stdlog, "[fd%d->fd-scheduled]\n",
			    scb->fd);
	      break;
	    default: /* TIMER SCHEDULED */
	      if (scb->async_state == FD_SCHEDULED)
		gdb_printf (gdb_stdlog, "[fd%d->timer-scheduled]\n",
			    scb->fd);
	      break;
	    }
	}
      scb->async_state = next_state;
    }
}

/* Run the SCB's async handle, and reschedule, if the handler doesn't
   close SCB.  */

static void
run_async_handler_and_reschedule (struct serial *scb)
{
  int is_open;

  /* Take a reference, so a serial_close call within the handler
     doesn't make SCB a dangling pointer.  */
  serial_ref (scb);

  /* Run the handler.  */
  scb->async_handler (scb, scb->async_context);

  is_open = serial_is_open (scb);
  serial_unref (scb);

  /* Get ready for more, if not already closed.  */
  if (is_open)
    reschedule (scb);
}

/* FD_EVENT: This is scheduled when the input FIFO is empty (and there
   is no pending error).  As soon as data arrives, it is read into the
   input FIFO and the client notified.  The client should then drain
   the FIFO using readchar().  If the FIFO isn't immediatly emptied,
   push_event() is used to nag the client until it is.  */

static void
fd_event (int error, void *context)
{
  struct serial *scb = (struct serial *) context;
  if (error != 0)
    {
      scb->bufcnt = SERIAL_ERROR;
    }
  else if (scb->bufcnt == 0)
    {
      /* Prime the input FIFO.  The readchar() function is used to
	 pull characters out of the buffer.  See also
	 generic_readchar().  */
      int nr;

      do
	{
	  nr = scb->ops->read_prim (scb, BUFSIZ);
	}
      while (nr < 0 && errno == EINTR);

      if (nr == 0)
	{
	  scb->bufcnt = SERIAL_EOF;
	}
      else if (nr > 0)
	{
	  scb->bufcnt = nr;
	  scb->bufp = scb->buf;
	}
      else
	{
	  scb->bufcnt = SERIAL_ERROR;
	}
    }
  run_async_handler_and_reschedule (scb);
}

/* PUSH_EVENT: The input FIFO is non-empty (or there is a pending
   error).  Nag the client until all the data has been read.  In the
   case of errors, the client will need to close or de-async the
   device before nagging stops.  */

static void
push_event (void *context)
{
  struct serial *scb = (struct serial *) context;

  scb->async_state = NOTHING_SCHEDULED; /* Timers are one-off */
  run_async_handler_and_reschedule (scb);
}

/* Wait for input on scb, with timeout seconds.  Returns 0 on success,
   otherwise SERIAL_TIMEOUT or SERIAL_ERROR.  */

/* NOTE: Some of the code below is dead.  The only possible values of
   the TIMEOUT parameter are ONE and ZERO.  OTOH, we should probably
   get rid of the deprecated_ui_loop_hook call in do_ser_base_readchar
   instead and support infinite time outs here.  */

static int
ser_base_wait_for (struct serial *scb, int timeout)
{
  while (1)
    {
      int numfds;
      struct timeval tv;
      fd_set readfds, exceptfds;
      int nfds;

      /* NOTE: Some OS's can scramble the READFDS when the select()
	 call fails (ex the kernel with Red Hat 5.2).  Initialize all
	 arguments before each call.  */

      tv.tv_sec = timeout;
      tv.tv_usec = 0;

      FD_ZERO (&readfds);
      FD_ZERO (&exceptfds);
      FD_SET (scb->fd, &readfds);
      FD_SET (scb->fd, &exceptfds);

      QUIT;

      nfds = scb->fd + 1;
      if (timeout >= 0)
	numfds = interruptible_select (nfds, &readfds, 0, &exceptfds, &tv);
      else
	numfds = interruptible_select (nfds, &readfds, 0, &exceptfds, 0);

      if (numfds <= 0)
	{
	  if (numfds == 0)
	    return SERIAL_TIMEOUT;
	  else if (errno == EINTR)
	    continue;
	  else
	    return SERIAL_ERROR;	/* Got an error from select or
					   poll.  */
	}

      return 0;
    }
}

/* Read any error output we might have.  */

static void
ser_base_read_error_fd (struct serial *scb, int close_fd)
{
  if (scb->error_fd != -1)
    {
      ssize_t s;
      char buf[GDB_MI_MSG_WIDTH + 1];

      for (;;)
	{
	  char *current;
	  char *newline;
	  int to_read = GDB_MI_MSG_WIDTH;
	  int num_bytes = -1;

	  if (scb->ops->avail)
	    num_bytes = (scb->ops->avail)(scb, scb->error_fd);

	  if (num_bytes != -1)
	    to_read = (num_bytes < to_read) ? num_bytes : to_read;

	  if (to_read == 0)
	    break;

	  s = read (scb->error_fd, &buf, to_read);
	  if ((s == -1) || (s == 0 && !close_fd))
	    break;

	  if (s == 0 && close_fd)
	    {
	      /* End of file.  */
	      if (serial_is_async_p (scb))
		delete_file_handler (scb->error_fd);
	      close (scb->error_fd);
	      scb->error_fd = -1;
	      break;
	    }

	  /* In theory, embedded newlines are not a problem.
	     But for MI, we want each output line to have just
	     one newline for legibility.  So output things
	     in newline chunks.  */
	  gdb_assert (s > 0 && s <= GDB_MI_MSG_WIDTH);
	  buf[s] = '\0';
	  current = buf;
	  while ((newline = strstr (current, "\n")) != NULL)
	    {
	      *newline = '\0';
	      gdb_puts (current, gdb_stderr);
	      gdb_puts ("\n", gdb_stderr);
	      current = newline + 1;
	    }

	  gdb_puts (current, gdb_stderr);
       }
    }
}

/* Event-loop callback for a serial's error_fd.  Flushes any error
   output we might have.  */

static void
handle_error_fd (int error, gdb_client_data client_data)
{
  serial *scb = (serial *) client_data;

  ser_base_read_error_fd (scb, 0);
}

/* Read a character with user-specified timeout.  TIMEOUT is number of
   seconds to wait, or -1 to wait forever.  Use timeout of 0 to effect
   a poll.  Returns char if successful.  Returns SERIAL_TIMEOUT if
   timeout expired, SERIAL_EOF if line dropped dead, or SERIAL_ERROR
   for any other error (see errno in that case).  */

static int
do_ser_base_readchar (struct serial *scb, int timeout)
{
  int status;
  int delta;

  /* We have to be able to keep the GUI alive here, so we break the
     original timeout into steps of 1 second, running the "keep the
     GUI alive" hook each time through the loop.

     Also, timeout = 0 means to poll, so we just set the delta to 0,
     so we will only go through the loop once.  */

  delta = (timeout == 0 ? 0 : 1);
  while (1)
    {
      /* N.B. The UI may destroy our world (for instance by calling
	 remote_stop,) in which case we want to get out of here as
	 quickly as possible.  It is not safe to touch scb, since
	 someone else might have freed it.  The
	 deprecated_ui_loop_hook signals that we should exit by
	 returning 1.  */

      if (deprecated_ui_loop_hook)
	{
	  if (deprecated_ui_loop_hook (0))
	    return SERIAL_TIMEOUT;
	}

      status = ser_base_wait_for (scb, delta);
      if (timeout > 0)
	timeout -= delta;

      /* If we got a character or an error back from wait_for, then we can 
	 break from the loop before the timeout is completed.  */
      if (status != SERIAL_TIMEOUT)
	break;

      /* If we have exhausted the original timeout, then generate
	 a SERIAL_TIMEOUT, and pass it out of the loop.  */
      else if (timeout == 0)
	{
	  status = SERIAL_TIMEOUT;
	  break;
	}

      /* We also need to check and consume the stderr because it could
	 come before the stdout for some stubs.  If we just sit and wait
	 for stdout, we would hit a deadlock for that case.  */
      ser_base_read_error_fd (scb, 0);
    }

  if (status < 0)
    return status;

  do
    {
      status = scb->ops->read_prim (scb, BUFSIZ);
    }
  while (status < 0 && errno == EINTR);

  if (status <= 0)
    {
      if (status == 0)
	return SERIAL_EOF;
      else
	/* Got an error from read.  */
	return SERIAL_ERROR;	
    }

  scb->bufcnt = status;
  scb->bufcnt--;
  scb->bufp = scb->buf;
  return *scb->bufp++;
}

/* Perform operations common to both old and new readchar.  */

/* Return the next character from the input FIFO.  If the FIFO is
   empty, call the SERIAL specific routine to try and read in more
   characters.

   Initially data from the input FIFO is returned (fd_event()
   pre-reads the input into that FIFO.  Once that has been emptied,
   further data is obtained by polling the input FD using the device
   specific readchar() function.  Note: reschedule() is called after
   every read.  This is because there is no guarentee that the lower
   level fd_event() poll_event() code (which also calls reschedule())
   will be called.  */

int
generic_readchar (struct serial *scb, int timeout,
		  int (do_readchar) (struct serial *scb, int timeout))
{
  int ch;
  if (scb->bufcnt > 0)
    {
      ch = *scb->bufp;
      scb->bufcnt--;
      scb->bufp++;
    }
  else if (scb->bufcnt < 0)
    {
      /* Some errors/eof are are sticky.  */
      ch = scb->bufcnt;
    }
  else
    {
      ch = do_readchar (scb, timeout);
      if (ch < 0)
	{
	  switch ((enum serial_rc) ch)
	    {
	    case SERIAL_EOF:
	    case SERIAL_ERROR:
	      /* Make the error/eof stick.  */
	      scb->bufcnt = ch;
	      break;
	    case SERIAL_TIMEOUT:
	      scb->bufcnt = 0;
	      break;
	    }
	}
    }

  /* Read any error output we might have.  */
  ser_base_read_error_fd (scb, 1);

  reschedule (scb);
  return ch;
}

int
ser_base_readchar (struct serial *scb, int timeout)
{
  return generic_readchar (scb, timeout, do_ser_base_readchar);
}

void
ser_base_write (struct serial *scb, const void *buf, size_t count)
{
  const char *str = (const char *) buf;
  int cc;

  while (count > 0)
    {
      QUIT;

      cc = scb->ops->write_prim (scb, str, count);

      if (cc < 0)
	{
	  if (errno == EINTR)
	    continue;
	  perror_with_name ("error while writing");
	}
      count -= cc;
      str += cc;
    }
}

int
ser_base_flush_output (struct serial *scb)
{
  return 0;
}

int
ser_base_flush_input (struct serial *scb)
{
  if (scb->bufcnt >= 0)
    {
      scb->bufcnt = 0;
      scb->bufp = scb->buf;
      return 0;
    }
  else
    return SERIAL_ERROR;
}

void
ser_base_send_break (struct serial *scb)
{
}

int
ser_base_drain_output (struct serial *scb)
{
  return 0;
}

void
ser_base_raw (struct serial *scb)
{
  return;			/* Always in raw mode.  */
}

serial_ttystate
ser_base_get_tty_state (struct serial *scb)
{
  /* Allocate a dummy.  */
  return (serial_ttystate) XNEW (int);
}

serial_ttystate
ser_base_copy_tty_state (struct serial *scb, serial_ttystate ttystate)
{
  /* Allocate another dummy.  */
  return (serial_ttystate) XNEW (int);
}

int
ser_base_set_tty_state (struct serial *scb, serial_ttystate ttystate)
{
  return 0;
}

void
ser_base_print_tty_state (struct serial *scb, 
			  serial_ttystate ttystate,
			  struct ui_file *stream)
{
  /* Nothing to print.  */
  return;
}

void
ser_base_setbaudrate (struct serial *scb, int rate)
{
  /* Never fails!  */
}

int
ser_base_setstopbits (struct serial *scb, int num)
{
  return 0;			/* Never fails!  */
}

/* Implement the "setparity" serial_ops callback.  */

int
ser_base_setparity (struct serial *scb, int parity)
{
  return 0;			/* Never fails!  */
}

/* Put the SERIAL device into/out-of ASYNC mode.  */

void
ser_base_async (struct serial *scb,
		int async_p)
{
  if (async_p)
    {
      /* Force a re-schedule.  */
      scb->async_state = NOTHING_SCHEDULED;
      if (serial_debug_p (scb))
	gdb_printf (gdb_stdlog, "[fd%d->asynchronous]\n",
		    scb->fd);
      reschedule (scb);

      if (scb->error_fd != -1)
	add_file_handler (scb->error_fd, handle_error_fd, scb, "serial-error");
    }
  else
    {
      if (serial_debug_p (scb))
	gdb_printf (gdb_stdlog, "[fd%d->synchronous]\n",
		    scb->fd);
      /* De-schedule whatever tasks are currently scheduled.  */
      switch (scb->async_state)
	{
	case FD_SCHEDULED:
	  delete_file_handler (scb->fd);
	  break;
	case NOTHING_SCHEDULED:
	  break;
	default: /* TIMER SCHEDULED */
	  delete_timer (scb->async_state);
	  break;
	}

      if (scb->error_fd != -1)
	delete_file_handler (scb->error_fd);
    }
}
