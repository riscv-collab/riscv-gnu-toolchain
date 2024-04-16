/* Serial interface for local (hardwired) serial ports on Windows systems

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

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
#include "ser-tcp.h"

#include <windows.h>
#include <conio.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "command.h"
#include "gdbsupport/buildargv.h"

struct ser_windows_state
{
  int in_progress;
  OVERLAPPED ov;
  DWORD lastCommMask;
  HANDLE except_event;
};

/* CancelIo is not available for Windows 95 OS, so we need to use
   LoadLibrary/GetProcAddress to avoid a startup failure.  */
#define CancelIo dyn_CancelIo
typedef BOOL WINAPI (CancelIo_ftype) (HANDLE);
static CancelIo_ftype *CancelIo;

/* Open up a real live device for serial I/O.  */

static void
ser_windows_open (struct serial *scb, const char *name)
{
  HANDLE h;
  struct ser_windows_state *state;
  COMMTIMEOUTS timeouts;

  h = CreateFile (name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		  OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
  if (h == INVALID_HANDLE_VALUE)
    {
      std::string msg = string_printf(_("could not open file: %s"),
				      name);
      throw_winerror_with_name (msg.c_str (), GetLastError ());
    }

  scb->fd = _open_osfhandle ((intptr_t) h, O_RDWR);
  if (scb->fd < 0)
    error (_("could not get underlying file descriptor"));

  if (!SetCommMask (h, EV_RXCHAR))
    throw_winerror_with_name (_("error calling SetCommMask"),
			      GetLastError ());

  timeouts.ReadIntervalTimeout = MAXDWORD;
  timeouts.ReadTotalTimeoutConstant = 0;
  timeouts.ReadTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = 0;
  timeouts.WriteTotalTimeoutMultiplier = 0;
  if (!SetCommTimeouts (h, &timeouts))
    throw_winerror_with_name (_("error calling SetCommTimeouts"),
			      GetLastError ());

  state = XCNEW (struct ser_windows_state);
  scb->state = state;

  /* Create a manual reset event to watch the input buffer.  */
  state->ov.hEvent = CreateEvent (0, TRUE, FALSE, 0);

  /* Create a (currently unused) handle to record exceptions.  */
  state->except_event = CreateEvent (0, TRUE, FALSE, 0);
}

/* Wait for the output to drain away, as opposed to flushing (discarding)
   it.  */

static int
ser_windows_drain_output (struct serial *scb)
{
  HANDLE h = (HANDLE) _get_osfhandle (scb->fd);

  return (FlushFileBuffers (h) != 0) ? 0 : -1;
}

static int
ser_windows_flush_output (struct serial *scb)
{
  HANDLE h = (HANDLE) _get_osfhandle (scb->fd);

  return (PurgeComm (h, PURGE_TXCLEAR) != 0) ? 0 : -1;
}

static int
ser_windows_flush_input (struct serial *scb)
{
  HANDLE h = (HANDLE) _get_osfhandle (scb->fd);

  return (PurgeComm (h, PURGE_RXCLEAR) != 0) ? 0 : -1;
}

static void
ser_windows_send_break (struct serial *scb)
{
  HANDLE h = (HANDLE) _get_osfhandle (scb->fd);

  if (SetCommBreak (h) == 0)
    throw_winerror_with_name ("error calling SetCommBreak",
			      GetLastError ());

  /* Delay for 250 milliseconds.  */
  Sleep (250);

  if (ClearCommBreak (h) == 0)
    throw_winerror_with_name ("error calling ClearCommBreak",
			      GetLastError ());
}

static void
ser_windows_raw (struct serial *scb)
{
  HANDLE h = (HANDLE) _get_osfhandle (scb->fd);
  DCB state;

  if (GetCommState (h, &state) == 0)
    return;

  state.fOutxCtsFlow = FALSE;
  state.fOutxDsrFlow = FALSE;
  state.fDtrControl = DTR_CONTROL_ENABLE;
  state.fDsrSensitivity = FALSE;
  state.fOutX = FALSE;
  state.fInX = FALSE;
  state.fNull = FALSE;
  state.fAbortOnError = FALSE;
  state.ByteSize = 8;

  if (SetCommState (h, &state) == 0)
    warning (_("SetCommState failed"));
}

static int
ser_windows_setstopbits (struct serial *scb, int num)
{
  HANDLE h = (HANDLE) _get_osfhandle (scb->fd);
  DCB state;

  if (GetCommState (h, &state) == 0)
    return -1;

  switch (num)
    {
    case SERIAL_1_STOPBITS:
      state.StopBits = ONESTOPBIT;
      break;
    case SERIAL_1_AND_A_HALF_STOPBITS:
      state.StopBits = ONE5STOPBITS;
      break;
    case SERIAL_2_STOPBITS:
      state.StopBits = TWOSTOPBITS;
      break;
    default:
      return 1;
    }

  return (SetCommState (h, &state) != 0) ? 0 : -1;
}

/* Implement the "setparity" serial_ops callback.  */

static int
ser_windows_setparity (struct serial *scb, int parity)
{
  HANDLE h = (HANDLE) _get_osfhandle (scb->fd);
  DCB state;

  if (GetCommState (h, &state) == 0)
    return -1;

  switch (parity)
    {
    case GDBPARITY_NONE:
      state.Parity = NOPARITY;
      state.fParity = FALSE;
      break;
    case GDBPARITY_ODD:
      state.Parity = ODDPARITY;
      state.fParity = TRUE;
      break;
    case GDBPARITY_EVEN:
      state.Parity = EVENPARITY;
      state.fParity = TRUE;
      break;
    default:
      internal_warning ("Incorrect parity value: %d", parity);
      return -1;
    }

  return (SetCommState (h, &state) != 0) ? 0 : -1;
}

static void
ser_windows_setbaudrate (struct serial *scb, int rate)
{
  HANDLE h = (HANDLE) _get_osfhandle (scb->fd);
  DCB state;

  if (GetCommState (h, &state) == 0)
    throw_winerror_with_name ("call to GetCommState failed", GetLastError ());

  state.BaudRate = rate;

  if (SetCommState (h, &state) == 0)
    throw_winerror_with_name ("call to SetCommState failed", GetLastError ());
}

static void
ser_windows_close (struct serial *scb)
{
  struct ser_windows_state *state;

  /* Stop any pending selects.  On Windows 95 OS, CancelIo function does
     not exist.  In that case, it can be replaced by a call to CloseHandle,
     but this is not necessary here as we do close the Windows handle
     by calling close (scb->fd) below.  */
  if (CancelIo)
    CancelIo ((HANDLE) _get_osfhandle (scb->fd));
  state = (struct ser_windows_state *) scb->state;
  CloseHandle (state->ov.hEvent);
  CloseHandle (state->except_event);

  if (scb->fd < 0)
    return;

  close (scb->fd);
  scb->fd = -1;

  xfree (scb->state);
}

static void
ser_windows_wait_handle (struct serial *scb, HANDLE *read, HANDLE *except)
{
  struct ser_windows_state *state;
  COMSTAT status;
  DWORD errors;
  HANDLE h = (HANDLE) _get_osfhandle (scb->fd);

  state = (struct ser_windows_state *) scb->state;

  *except = state->except_event;
  *read = state->ov.hEvent;

  if (state->in_progress)
    return;

  /* Reset the mask - we are only interested in any characters which
     arrive after this point, not characters which might have arrived
     and already been read.  */

  /* This really, really shouldn't be necessary - just the second one.
     But otherwise an internal flag for EV_RXCHAR does not get
     cleared, and we get a duplicated event, if the last batch
     of characters included at least two arriving close together.  */
  if (!SetCommMask (h, 0))
    warning (_("ser_windows_wait_handle: reseting mask failed"));

  if (!SetCommMask (h, EV_RXCHAR))
    warning (_("ser_windows_wait_handle: reseting mask failed (2)"));

  /* There's a potential race condition here; we must check cbInQue
     and not wait if that's nonzero.  */

  ClearCommError (h, &errors, &status);
  if (status.cbInQue > 0)
    {
      SetEvent (state->ov.hEvent);
      return;
    }

  state->in_progress = 1;
  ResetEvent (state->ov.hEvent);
  state->lastCommMask = -2;
  if (WaitCommEvent (h, &state->lastCommMask, &state->ov))
    {
      gdb_assert (state->lastCommMask & EV_RXCHAR);
      SetEvent (state->ov.hEvent);
    }
  else
    gdb_assert (GetLastError () == ERROR_IO_PENDING);
}

static int
ser_windows_read_prim (struct serial *scb, size_t count)
{
  struct ser_windows_state *state;
  OVERLAPPED ov;
  DWORD bytes_read;
  HANDLE h;

  state = (struct ser_windows_state *) scb->state;
  if (state->in_progress)
    {
      WaitForSingleObject (state->ov.hEvent, INFINITE);
      state->in_progress = 0;
      ResetEvent (state->ov.hEvent);
    }

  memset (&ov, 0, sizeof (OVERLAPPED));
  ov.hEvent = CreateEvent (0, FALSE, FALSE, 0);
  h = (HANDLE) _get_osfhandle (scb->fd);

  if (!ReadFile (h, scb->buf, /* count */ 1, &bytes_read, &ov))
    {
      if (GetLastError () != ERROR_IO_PENDING
	  || !GetOverlappedResult (h, &ov, &bytes_read, TRUE))
	{
	  ULONGEST err = GetLastError ();
	  CloseHandle (ov.hEvent);
	  throw_winerror_with_name (_("error while reading"), err);
	}
    }

  CloseHandle (ov.hEvent);
  return bytes_read;
}

static int
ser_windows_write_prim (struct serial *scb, const void *buf, size_t len)
{
  OVERLAPPED ov;
  DWORD bytes_written;
  HANDLE h;

  memset (&ov, 0, sizeof (OVERLAPPED));
  ov.hEvent = CreateEvent (0, FALSE, FALSE, 0);
  h = (HANDLE) _get_osfhandle (scb->fd);
  if (!WriteFile (h, buf, len, &bytes_written, &ov))
    {
      if (GetLastError () != ERROR_IO_PENDING
	  || !GetOverlappedResult (h, &ov, &bytes_written, TRUE))
	throw_winerror_with_name ("error while writing", GetLastError ());
    }

  CloseHandle (ov.hEvent);
  return bytes_written;
}

/* On Windows, gdb_select is implemented using WaitForMulpleObjects.
   A "select thread" is created for each file descriptor.  These
   threads looks for activity on the corresponding descriptor, using
   whatever techniques are appropriate for the descriptor type.  When
   that activity occurs, the thread signals an appropriate event,
   which wakes up WaitForMultipleObjects.

   Each select thread is in one of two states: stopped or started.
   Select threads begin in the stopped state.  When gdb_select is
   called, threads corresponding to the descriptors of interest are
   started by calling a wait_handle function.  Each thread that
   notices activity signals the appropriate event and then reenters
   the stopped state.  Before gdb_select returns it calls the
   wait_handle_done functions, which return the threads to the stopped
   state.  */

enum select_thread_state {
  STS_STARTED,
  STS_STOPPED
};

struct ser_console_state
{
  /* Signaled by the select thread to indicate that data is available
     on the file descriptor.  */
  HANDLE read_event;
  /* Signaled by the select thread to indicate that an exception has
     occurred on the file descriptor.  */
  HANDLE except_event;
  /* Signaled by the select thread to indicate that it has entered the
     started state.  HAVE_STARTED and HAVE_STOPPED are never signaled
     simultaneously.  */
  HANDLE have_started;
  /* Signaled by the select thread to indicate that it has stopped,
     either because data is available (and READ_EVENT is signaled),
     because an exception has occurred (and EXCEPT_EVENT is signaled),
     or because STOP_SELECT was signaled.  */
  HANDLE have_stopped;

  /* Signaled by the main program to tell the select thread to enter
     the started state.  */
  HANDLE start_select;
  /* Signaled by the main program to tell the select thread to enter
     the stopped state.  */
  HANDLE stop_select;
  /* Signaled by the main program to tell the select thread to
     exit.  */
  HANDLE exit_select;

  /* The handle for the select thread.  */
  HANDLE thread;
  /* The state of the select thread.  This field is only accessed in
     the main program, never by the select thread itself.  */
  enum select_thread_state thread_state;
};

/* Called by a select thread to enter the stopped state.  This
   function does not return until the thread has re-entered the
   started state.  */
static void
select_thread_wait (struct ser_console_state *state)
{
  HANDLE wait_events[2];

  /* There are two things that can wake us up: a request that we enter
     the started state, or that we exit this thread.  */
  wait_events[0] = state->start_select;
  wait_events[1] = state->exit_select;
  if (WaitForMultipleObjects (2, wait_events, FALSE, INFINITE) 
      != WAIT_OBJECT_0)
    /* Either the EXIT_SELECT event was signaled (requesting that the
       thread exit) or an error has occurred.  In either case, we exit
       the thread.  */
    ExitThread (0);
  
  /* We are now in the started state.  */
  SetEvent (state->have_started);
}

typedef DWORD WINAPI (*thread_fn_type)(void *);

/* Create a new select thread for SCB executing THREAD_FN.  The STATE
   will be filled in by this function before return.  */
static void
create_select_thread (thread_fn_type thread_fn,
		      struct serial *scb,
		      struct ser_console_state *state)
{
  DWORD threadId;

  /* Create all of the events.  These are all auto-reset events.  */
  state->read_event = CreateEvent (NULL, FALSE, FALSE, NULL);
  state->except_event = CreateEvent (NULL, FALSE, FALSE, NULL);
  state->have_started = CreateEvent (NULL, FALSE, FALSE, NULL);
  state->have_stopped = CreateEvent (NULL, FALSE, FALSE, NULL);
  state->start_select = CreateEvent (NULL, FALSE, FALSE, NULL);
  state->stop_select = CreateEvent (NULL, FALSE, FALSE, NULL);
  state->exit_select = CreateEvent (NULL, FALSE, FALSE, NULL);

  state->thread = CreateThread (NULL, 0, thread_fn, scb, 0, &threadId);
  /* The thread begins in the stopped state.  */
  state->thread_state = STS_STOPPED;
}

/* Destroy the select thread indicated by STATE.  */
static void
destroy_select_thread (struct ser_console_state *state)
{
  /* Ask the thread to exit.  */
  SetEvent (state->exit_select);
  /* Wait until it does.  */
  WaitForSingleObject (state->thread, INFINITE);

  /* Destroy the events.  */
  CloseHandle (state->read_event);
  CloseHandle (state->except_event);
  CloseHandle (state->have_started);
  CloseHandle (state->have_stopped);
  CloseHandle (state->start_select);
  CloseHandle (state->stop_select);
  CloseHandle (state->exit_select);
}

/* Called by gdb_select to start the select thread indicated by STATE.
   This function does not return until the thread has started.  */
static void
start_select_thread (struct ser_console_state *state)
{
  /* Ask the thread to start.  */
  SetEvent (state->start_select);
  /* Wait until it does.  */
  WaitForSingleObject (state->have_started, INFINITE);
  /* The thread is now started.  */
  state->thread_state = STS_STARTED;
}

/* Called by gdb_select to stop the select thread indicated by STATE.
   This function does not return until the thread has stopped.  */
static void
stop_select_thread (struct ser_console_state *state)
{
  /* If the thread is already in the stopped state, we have nothing to
     do.  Some of the wait_handle functions avoid calling
     start_select_thread if they notice activity on the relevant file
     descriptors.  The wait_handle_done functions still call
     stop_select_thread -- but it is already stopped.  */
  if (state->thread_state != STS_STARTED)
    return;
  /* Ask the thread to stop.  */
  SetEvent (state->stop_select);
  /* Wait until it does.  */
  WaitForSingleObject (state->have_stopped, INFINITE);
  /* The thread is now stopped.  */
  state->thread_state = STS_STOPPED;
}

static DWORD WINAPI
console_select_thread (void *arg)
{
  struct serial *scb = (struct serial *) arg;
  struct ser_console_state *state;
  int event_index;
  HANDLE h;

  state = (struct ser_console_state *) scb->state;
  h = (HANDLE) _get_osfhandle (scb->fd);

  while (1)
    {
      HANDLE wait_events[2];
      INPUT_RECORD record;
      DWORD n_records;

      select_thread_wait (state);

      while (1)
	{
	  wait_events[0] = state->stop_select;
	  wait_events[1] = h;

	  event_index = WaitForMultipleObjects (2, wait_events,
						FALSE, INFINITE);

	  if (event_index == WAIT_OBJECT_0
	      || WaitForSingleObject (state->stop_select, 0) == WAIT_OBJECT_0)
	    break;

	  if (event_index != WAIT_OBJECT_0 + 1)
	    {
	      /* Wait must have failed; assume an error has occurred, e.g.
		 the handle has been closed.  */
	      SetEvent (state->except_event);
	      break;
	    }

	  /* We've got a pending event on the console.  See if it's
	     of interest.  */
	  if (!PeekConsoleInput (h, &record, 1, &n_records) || n_records != 1)
	    {
	      /* Something went wrong.  Maybe the console is gone.  */
	      SetEvent (state->except_event);
	      break;
	    }

	  if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
	    {
	      WORD keycode = record.Event.KeyEvent.wVirtualKeyCode;

	      /* Ignore events containing only control keys.  We must
		 recognize "enhanced" keys which we are interested in
		 reading via getch, if they do not map to ASCII.  But we
		 do not want to report input available for e.g. the
		 control key alone.  */

	      if (record.Event.KeyEvent.uChar.AsciiChar != 0
		  || keycode == VK_PRIOR
		  || keycode == VK_NEXT
		  || keycode == VK_END
		  || keycode == VK_HOME
		  || keycode == VK_LEFT
		  || keycode == VK_UP
		  || keycode == VK_RIGHT
		  || keycode == VK_DOWN
		  || keycode == VK_INSERT
		  || keycode == VK_DELETE)
		{
		  /* This is really a keypress.  */
		  SetEvent (state->read_event);
		  break;
		}
	    }
	  else if (record.EventType == MOUSE_EVENT)
	    {
	      SetEvent (state->read_event);
	      break;
	    }

	  /* Otherwise discard it and wait again.  */
	  ReadConsoleInput (h, &record, 1, &n_records);
	}

      SetEvent(state->have_stopped);
    }
  return 0;
}

static int
fd_is_pipe (int fd)
{
  if (PeekNamedPipe ((HANDLE) _get_osfhandle (fd), NULL, 0, NULL, NULL, NULL))
    return 1;
  else
    return 0;
}

static int
fd_is_file (int fd)
{
  if (GetFileType ((HANDLE) _get_osfhandle (fd)) == FILE_TYPE_DISK)
    return 1;
  else
    return 0;
}

static DWORD WINAPI
pipe_select_thread (void *arg)
{
  struct serial *scb = (struct serial *) arg;
  struct ser_console_state *state;
  HANDLE h;

  state = (struct ser_console_state *) scb->state;
  h = (HANDLE) _get_osfhandle (scb->fd);

  while (1)
    {
      DWORD n_avail;

      select_thread_wait (state);

      /* Wait for something to happen on the pipe.  */
      while (1)
	{
	  if (!PeekNamedPipe (h, NULL, 0, NULL, &n_avail, NULL))
	    {
	      SetEvent (state->except_event);
	      break;
	    }

	  if (n_avail > 0)
	    {
	      SetEvent (state->read_event);
	      break;
	    }

	  /* Delay 10ms before checking again, but allow the stop
	     event to wake us.  */
	  if (WaitForSingleObject (state->stop_select, 10) == WAIT_OBJECT_0)
	    break;
	}

      SetEvent (state->have_stopped);
    }
  return 0;
}

static DWORD WINAPI
file_select_thread (void *arg)
{
  struct serial *scb = (struct serial *) arg;
  struct ser_console_state *state;
  HANDLE h;

  state = (struct ser_console_state *) scb->state;
  h = (HANDLE) _get_osfhandle (scb->fd);

  while (1)
    {
      select_thread_wait (state);

      if (SetFilePointer (h, 0, NULL, FILE_CURRENT)
	  == INVALID_SET_FILE_POINTER)
	SetEvent (state->except_event);
      else
	SetEvent (state->read_event);

      SetEvent (state->have_stopped);
    }
  return 0;
}

static void
ser_console_wait_handle (struct serial *scb, HANDLE *read, HANDLE *except)
{
  struct ser_console_state *state = (struct ser_console_state *) scb->state;

  if (state == NULL)
    {
      thread_fn_type thread_fn;
      int is_tty;

      is_tty = isatty (scb->fd);
      if (!is_tty && !fd_is_file (scb->fd) && !fd_is_pipe (scb->fd))
	{
	  *read = NULL;
	  *except = NULL;
	  return;
	}

      state = XCNEW (struct ser_console_state);
      scb->state = state;

      if (is_tty)
	thread_fn = console_select_thread;
      else if (fd_is_pipe (scb->fd))
	thread_fn = pipe_select_thread;
      else
	thread_fn = file_select_thread;

      create_select_thread (thread_fn, scb, state);
    }

  *read = state->read_event;
  *except = state->except_event;

  /* Start from a blank state.  */
  ResetEvent (state->read_event);
  ResetEvent (state->except_event);
  ResetEvent (state->stop_select);

  /* First check for a key already in the buffer.  If there is one,
     we don't need a thread.  This also catches the second key of
     multi-character returns from getch, for instance for arrow
     keys.  The second half is in a C library internal buffer,
     and PeekConsoleInput will not find it.  */
  if (_kbhit ())
    {
      SetEvent (state->read_event);
      return;
    }

  /* Otherwise, start the select thread.  */
  start_select_thread (state);
}

static void
ser_console_done_wait_handle (struct serial *scb)
{
  struct ser_console_state *state = (struct ser_console_state *) scb->state;

  if (state == NULL)
    return;

  stop_select_thread (state);
}

static void
ser_console_close (struct serial *scb)
{
  struct ser_console_state *state = (struct ser_console_state *) scb->state;

  if (scb->state)
    {
      destroy_select_thread (state);
      xfree (scb->state);
    }
}

struct ser_console_ttystate
{
  int is_a_tty;
};

static serial_ttystate
ser_console_get_tty_state (struct serial *scb)
{
  if (isatty (scb->fd))
    {
      struct ser_console_ttystate *state;

      state = XNEW (struct ser_console_ttystate);
      state->is_a_tty = 1;
      return state;
    }
  else
    return NULL;
}

struct pipe_state
{
  /* Since we use the pipe_select_thread for our select emulation,
     we need to place the state structure it requires at the front
     of our state.  */
  struct ser_console_state wait;

  /* The pex obj for our (one-stage) pipeline.  */
  struct pex_obj *pex;

  /* Streams for the pipeline's input and output.  */
  FILE *input, *output;
};

static struct pipe_state *
make_pipe_state (void)
{
  struct pipe_state *ps = XCNEW (struct pipe_state);

  ps->wait.read_event = INVALID_HANDLE_VALUE;
  ps->wait.except_event = INVALID_HANDLE_VALUE;
  ps->wait.start_select = INVALID_HANDLE_VALUE;
  ps->wait.stop_select = INVALID_HANDLE_VALUE;

  return ps;
}

static void
free_pipe_state (struct pipe_state *ps)
{
  int saved_errno = errno;

  if (ps->wait.read_event != INVALID_HANDLE_VALUE)
    destroy_select_thread (&ps->wait);

  /* Close the pipe to the child.  We must close the pipe before
     calling pex_free because pex_free will wait for the child to exit
     and the child will not exit until the pipe is closed.  */
  if (ps->input)
    fclose (ps->input);
  if (ps->pex)
    {
      pex_free (ps->pex);
      /* pex_free closes ps->output.  */
    }
  else if (ps->output)
    fclose (ps->output);

  xfree (ps);

  errno = saved_errno;
}

struct pipe_state_destroyer
{
  void operator() (pipe_state *ps) const
  {
    free_pipe_state (ps);
  }
};

typedef std::unique_ptr<pipe_state, pipe_state_destroyer> pipe_state_up;

static void
pipe_windows_open (struct serial *scb, const char *name)
{
  FILE *pex_stderr;

  if (name == NULL)
    error_no_arg (_("child command"));

  if (*name == '|')
    {
      name++;
      name = skip_spaces (name);
    }

  gdb_argv argv (name);

  if (! argv[0] || argv[0][0] == '\0')
    error (_("missing child command"));

  pipe_state_up ps (make_pipe_state ());

  ps->pex = pex_init (PEX_USE_PIPES, "target remote pipe", NULL);
  if (! ps->pex)
    error (_("could not start pipeline"));
  ps->input = pex_input_pipe (ps->pex, 1);
  if (! ps->input)
    error (_("could not find input pipe"));

  {
    int err;
    const char *err_msg
      = pex_run (ps->pex, PEX_SEARCH | PEX_BINARY_INPUT | PEX_BINARY_OUTPUT
		 | PEX_STDERR_TO_PIPE,
		 argv[0], argv.get (), NULL, NULL,
		 &err);

    if (err_msg)
      {
	/* Our caller expects us to return -1, but all they'll do with
	   it generally is print the message based on errno.  We have
	   all the same information here, plus err_msg provided by
	   pex_run, so we just raise the error here.  */
	if (err)
	  error (_("error starting child process '%s': %s: %s"),
		 name, err_msg, safe_strerror (err));
	else
	  error (_("error starting child process '%s': %s"),
		 name, err_msg);
      }
  }

  ps->output = pex_read_output (ps->pex, 1);
  if (! ps->output)
    error (_("could not find output pipe"));
  scb->fd = fileno (ps->output);

  pex_stderr = pex_read_err (ps->pex, 1);
  if (! pex_stderr)
    error (_("could not find error pipe"));
  scb->error_fd = fileno (pex_stderr);

  scb->state = ps.release ();
}

static int
pipe_windows_fdopen (struct serial *scb, int fd)
{
  struct pipe_state *ps;

  ps = make_pipe_state ();

  ps->input = fdopen (fd, "r+");
  if (! ps->input)
    goto fail;

  ps->output = fdopen (fd, "r+");
  if (! ps->output)
    goto fail;

  scb->fd = fd;
  scb->state = (void *) ps;

  return 0;

 fail:
  free_pipe_state (ps);
  return -1;
}

static void
pipe_windows_close (struct serial *scb)
{
  struct pipe_state *ps = (struct pipe_state *) scb->state;

  /* In theory, we should try to kill the subprocess here, but the pex
     interface doesn't give us enough information to do that.  Usually
     closing the input pipe will get the message across.  */

  free_pipe_state (ps);
}


static int
pipe_windows_read (struct serial *scb, size_t count)
{
  HANDLE pipeline_out = (HANDLE) _get_osfhandle (scb->fd);
  DWORD available;
  DWORD bytes_read;

  if (pipeline_out == INVALID_HANDLE_VALUE)
    error (_("could not find file number for pipe"));

  if (! PeekNamedPipe (pipeline_out, NULL, 0, NULL, &available, NULL))
    throw_winerror_with_name (_("could not peek into pipe"), GetLastError ());

  if (count > available)
    count = available;

  if (! ReadFile (pipeline_out, scb->buf, count, &bytes_read, NULL))
    throw_winerror_with_name (_("could not read from pipe"), GetLastError ());

  return bytes_read;
}


static int
pipe_windows_write (struct serial *scb, const void *buf, size_t count)
{
  struct pipe_state *ps = (struct pipe_state *) scb->state;
  HANDLE pipeline_in;
  DWORD written;

  int pipeline_in_fd = fileno (ps->input);
  if (pipeline_in_fd < 0)
    error (_("could not find file number for pipe"));

  pipeline_in = (HANDLE) _get_osfhandle (pipeline_in_fd);
  if (pipeline_in == INVALID_HANDLE_VALUE)
    error (_("could not find handle for pipe"));

  if (! WriteFile (pipeline_in, buf, count, &written, NULL))
    throw_winerror_with_name (_("could not write to pipe"), GetLastError ());

  return written;
}


static void
pipe_wait_handle (struct serial *scb, HANDLE *read, HANDLE *except)
{
  struct pipe_state *ps = (struct pipe_state *) scb->state;

  /* Have we allocated our events yet?  */
  if (ps->wait.read_event == INVALID_HANDLE_VALUE)
    /* Start the thread.  */
    create_select_thread (pipe_select_thread, scb, &ps->wait);

  *read = ps->wait.read_event;
  *except = ps->wait.except_event;

  /* Start from a blank state.  */
  ResetEvent (ps->wait.read_event);
  ResetEvent (ps->wait.except_event);
  ResetEvent (ps->wait.stop_select);

  start_select_thread (&ps->wait);
}

static void
pipe_done_wait_handle (struct serial *scb)
{
  struct pipe_state *ps = (struct pipe_state *) scb->state;

  /* Have we allocated our events yet?  */
  if (ps->wait.read_event == INVALID_HANDLE_VALUE)
    return;

  stop_select_thread (&ps->wait);
}

static int
pipe_avail (struct serial *scb, int fd)
{
  HANDLE h = (HANDLE) _get_osfhandle (fd);
  DWORD numBytes;
  BOOL r = PeekNamedPipe (h, NULL, 0, NULL, &numBytes, NULL);

  if (r == FALSE)
    numBytes = 0;
  return numBytes;
}

int
gdb_pipe (int pdes[2])
{
  if (_pipe (pdes, 512, _O_BINARY | _O_NOINHERIT) == -1)
    return -1;
  return 0;
}

struct net_windows_state
{
  struct ser_console_state base;
  
  HANDLE sock_event;
};

/* Check whether the socket has any pending data to be read.  If so,
   set the select thread's read event.  On error, set the select
   thread's except event.  If any event was set, return true,
   otherwise return false.  */

static int
net_windows_socket_check_pending (struct serial *scb)
{
  struct net_windows_state *state = (struct net_windows_state *) scb->state;
  unsigned long available;

  if (ioctlsocket (scb->fd, FIONREAD, &available) != 0)
    {
      /* The socket closed, or some other error.  */
      SetEvent (state->base.except_event);
      return 1;
    }
  else if (available > 0)
    {
      SetEvent (state->base.read_event);
      return 1;
    }

  return 0;
}

static DWORD WINAPI
net_windows_select_thread (void *arg)
{
  struct serial *scb = (struct serial *) arg;
  struct net_windows_state *state;
  int event_index;

  state = (struct net_windows_state *) scb->state;

  while (1)
    {
      HANDLE wait_events[2];
      WSANETWORKEVENTS events;

      select_thread_wait (&state->base);

      wait_events[0] = state->base.stop_select;
      wait_events[1] = state->sock_event;

      /* Wait for something to happen on the socket.  */
      while (1)
	{
	  event_index = WaitForMultipleObjects (2, wait_events, FALSE, INFINITE);

	  if (event_index == WAIT_OBJECT_0
	      || WaitForSingleObject (state->base.stop_select, 0) == WAIT_OBJECT_0)
	    {
	      /* We have been requested to stop.  */
	      break;
	    }

	  if (event_index != WAIT_OBJECT_0 + 1)
	    {
	      /* Some error has occurred.  Assume that this is an error
		 condition.  */
	      SetEvent (state->base.except_event);
	      break;
	    }

	  /* Enumerate the internal network events, and reset the
	     object that signalled us to catch the next event.  */
	  if (WSAEnumNetworkEvents (scb->fd, state->sock_event, &events) != 0)
	    {
	      /* Something went wrong.  Maybe the socket is gone.  */
	      SetEvent (state->base.except_event);
	      break;
	    }

	  if (events.lNetworkEvents & FD_READ)
	    {
	      if (net_windows_socket_check_pending (scb))
		break;

	      /* Spurious wakeup.  That is, the socket's event was
		 signalled before we last called recv.  */
	    }

	  if (events.lNetworkEvents & FD_CLOSE)
	    {
	      SetEvent (state->base.except_event);
	      break;
	    }
	}

      SetEvent (state->base.have_stopped);
    }
  return 0;
}

static void
net_windows_wait_handle (struct serial *scb, HANDLE *read, HANDLE *except)
{
  struct net_windows_state *state = (struct net_windows_state *) scb->state;

  /* Start from a clean slate.  */
  ResetEvent (state->base.read_event);
  ResetEvent (state->base.except_event);
  ResetEvent (state->base.stop_select);

  *read = state->base.read_event;
  *except = state->base.except_event;

  /* Check any pending events.  Otherwise, start the select
     thread.  */
  if (!net_windows_socket_check_pending (scb))
    start_select_thread (&state->base);
}

static void
net_windows_done_wait_handle (struct serial *scb)
{
  struct net_windows_state *state = (struct net_windows_state *) scb->state;

  stop_select_thread (&state->base);
}

static void
net_windows_open (struct serial *scb, const char *name)
{
  struct net_windows_state *state;

  net_open (scb, name);

  state = XCNEW (struct net_windows_state);
  scb->state = state;

  /* Associate an event with the socket.  */
  state->sock_event = CreateEvent (0, TRUE, FALSE, 0);
  WSAEventSelect (scb->fd, state->sock_event, FD_READ | FD_CLOSE);

  /* Start the thread.  */
  create_select_thread (net_windows_select_thread, scb, &state->base);
}


static void
net_windows_close (struct serial *scb)
{
  struct net_windows_state *state = (struct net_windows_state *) scb->state;

  destroy_select_thread (&state->base);
  CloseHandle (state->sock_event);

  xfree (scb->state);

  net_close (scb);
}

/* The serial port driver.  */

static const struct serial_ops hardwire_ops =
{
  "hardwire",
  ser_windows_open,
  ser_windows_close,
  NULL,
  ser_base_readchar,
  ser_base_write,
  ser_windows_flush_output,
  ser_windows_flush_input,
  ser_windows_send_break,
  ser_windows_raw,
  /* These are only used for stdin; we do not need them for serial
     ports, so supply the standard dummies.  */
  ser_base_get_tty_state,
  ser_base_copy_tty_state,
  ser_base_set_tty_state,
  ser_base_print_tty_state,
  ser_windows_setbaudrate,
  ser_windows_setstopbits,
  ser_windows_setparity,
  ser_windows_drain_output,
  ser_base_async,
  ser_windows_read_prim,
  ser_windows_write_prim,
  NULL,
  ser_windows_wait_handle
};

/* The dummy serial driver used for terminals.  We only provide the
   TTY-related methods.  */

static const struct serial_ops tty_ops =
{
  "terminal",
  NULL,
  ser_console_close,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  ser_console_get_tty_state,
  ser_base_copy_tty_state,
  ser_base_set_tty_state,
  ser_base_print_tty_state,
  NULL,
  NULL,
  NULL,
  ser_base_drain_output,
  NULL,
  NULL,
  NULL,
  NULL,
  ser_console_wait_handle,
  ser_console_done_wait_handle
};

/* The pipe interface.  */

static const struct serial_ops pipe_ops =
{
  "pipe",
  pipe_windows_open,
  pipe_windows_close,
  pipe_windows_fdopen,
  ser_base_readchar,
  ser_base_write,
  ser_base_flush_output,
  ser_base_flush_input,
  ser_base_send_break,
  ser_base_raw,
  ser_base_get_tty_state,
  ser_base_copy_tty_state,
  ser_base_set_tty_state,
  ser_base_print_tty_state,
  ser_base_setbaudrate,
  ser_base_setstopbits,
  ser_base_setparity,
  ser_base_drain_output,
  ser_base_async,
  pipe_windows_read,
  pipe_windows_write,
  pipe_avail,
  pipe_wait_handle,
  pipe_done_wait_handle
};

/* The TCP/UDP socket driver.  */

static const struct serial_ops tcp_ops =
{
  "tcp",
  net_windows_open,
  net_windows_close,
  NULL,
  ser_base_readchar,
  ser_base_write,
  ser_base_flush_output,
  ser_base_flush_input,
  ser_tcp_send_break,
  ser_base_raw,
  ser_base_get_tty_state,
  ser_base_copy_tty_state,
  ser_base_set_tty_state,
  ser_base_print_tty_state,
  ser_base_setbaudrate,
  ser_base_setstopbits,
  ser_base_setparity,
  ser_base_drain_output,
  ser_base_async,
  net_read_prim,
  net_write_prim,
  NULL,
  net_windows_wait_handle,
  net_windows_done_wait_handle
};

void _initialize_ser_windows ();
void
_initialize_ser_windows ()
{
  WSADATA wsa_data;

  HMODULE hm = NULL;

  /* First find out if kernel32 exports CancelIo function.  */
  hm = LoadLibrary ("kernel32.dll");
  if (hm)
    {
      CancelIo = (CancelIo_ftype *) GetProcAddress (hm, "CancelIo");
      FreeLibrary (hm);
    }
  else
    CancelIo = NULL;

  serial_add_interface (&hardwire_ops);
  serial_add_interface (&tty_ops);
  serial_add_interface (&pipe_ops);

  /* If WinSock works, register the TCP/UDP socket driver.  */

  if (WSAStartup (MAKEWORD (1, 0), &wsa_data) != 0)
    /* WinSock is unavailable.  */
    return;

  serial_add_interface (&tcp_ops);
}
