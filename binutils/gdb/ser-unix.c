/* Serial interface for local (hardwired) serial ports on Un*x like systems

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
#include "ser-unix.h"

#include <fcntl.h>
#include <sys/types.h>
#include "terminal.h"
#include <sys/socket.h>
#include "gdbsupport/gdb_sys_time.h"

#include "gdbsupport/gdb_select.h"
#include "gdbcmd.h"
#include "gdbsupport/filestuff.h"
#include <termios.h>
#include "gdbsupport/scoped_ignore_sigttou.h"

struct hardwire_ttystate
  {
    struct termios termios;
  };

#ifdef CRTSCTS
/* Boolean to explicitly enable or disable h/w flow control.  */
static bool serial_hwflow;
static void
show_serial_hwflow (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Hardware flow control is %s.\n"), value);
}
#endif

static void hardwire_raw (struct serial *scb);
static int rate_to_code (int rate);
static void hardwire_setbaudrate (struct serial *scb, int rate);
static int hardwire_setparity (struct serial *scb, int parity);
static void hardwire_close (struct serial *scb);
static int get_tty_state (struct serial *scb,
			  struct hardwire_ttystate * state);
static int set_tty_state (struct serial *scb,
			  struct hardwire_ttystate * state);
static serial_ttystate hardwire_get_tty_state (struct serial *scb);
static int hardwire_set_tty_state (struct serial *scb, serial_ttystate state);
static void hardwire_print_tty_state (struct serial *, serial_ttystate,
				      struct ui_file *);
static int hardwire_drain_output (struct serial *);
static int hardwire_flush_output (struct serial *);
static int hardwire_flush_input (struct serial *);
static void hardwire_send_break (struct serial *);
static int hardwire_setstopbits (struct serial *, int);

/* Open up a real live device for serial I/O.  */

static void
hardwire_open (struct serial *scb, const char *name)
{
  scb->fd = gdb_open_cloexec (name, O_RDWR, 0).release ();
  if (scb->fd < 0)
    perror_with_name ("could not open device");
}

static int
get_tty_state (struct serial *scb, struct hardwire_ttystate *state)
{
  if (tcgetattr (scb->fd, &state->termios) < 0)
    return -1;

  return 0;
}

static int
set_tty_state (struct serial *scb, struct hardwire_ttystate *state)
{
  if (tcsetattr (scb->fd, TCSANOW, &state->termios) < 0)
    return -1;

  return 0;
}

static serial_ttystate
hardwire_get_tty_state (struct serial *scb)
{
  struct hardwire_ttystate *state = XNEW (struct hardwire_ttystate);

  if (get_tty_state (scb, state))
    {
      xfree (state);
      return NULL;
    }

  return (serial_ttystate) state;
}

static serial_ttystate
hardwire_copy_tty_state (struct serial *scb, serial_ttystate ttystate)
{
  struct hardwire_ttystate *state = XNEW (struct hardwire_ttystate);

  *state = *(struct hardwire_ttystate *) ttystate;

  return (serial_ttystate) state;
}

static int
hardwire_set_tty_state (struct serial *scb, serial_ttystate ttystate)
{
  struct hardwire_ttystate *state;

  state = (struct hardwire_ttystate *) ttystate;

  return set_tty_state (scb, state);
}

static void
hardwire_print_tty_state (struct serial *scb,
			  serial_ttystate ttystate,
			  struct ui_file *stream)
{
  struct hardwire_ttystate *state = (struct hardwire_ttystate *) ttystate;
  int i;

  gdb_printf (stream, "c_iflag = 0x%x, c_oflag = 0x%x,\n",
	      (int) state->termios.c_iflag,
	      (int) state->termios.c_oflag);
  gdb_printf (stream, "c_cflag = 0x%x, c_lflag = 0x%x\n",
	      (int) state->termios.c_cflag,
	      (int) state->termios.c_lflag);
#if 0
  /* This not in POSIX, and is not really documented by those systems
     which have it (at least not Sun).  */
  gdb_printf (stream, "c_line = 0x%x.\n", state->termios.c_line);
#endif
  gdb_printf (stream, "c_cc: ");
  for (i = 0; i < NCCS; i += 1)
    gdb_printf (stream, "0x%x ", state->termios.c_cc[i]);
  gdb_printf (stream, "\n");
}

/* Wait for the output to drain away, as opposed to flushing
   (discarding) it.  */

static int
hardwire_drain_output (struct serial *scb)
{
  /* Ignore SIGTTOU which may occur during the drain.  */
  scoped_ignore_sigttou ignore_sigttou;

  return tcdrain (scb->fd);
}

static int
hardwire_flush_output (struct serial *scb)
{
  return tcflush (scb->fd, TCOFLUSH);
}

static int
hardwire_flush_input (struct serial *scb)
{
  ser_base_flush_input (scb);

  return tcflush (scb->fd, TCIFLUSH);
}

static void
hardwire_send_break (struct serial *scb)
{
  if (tcsendbreak (scb->fd, 0) == -1)
    perror_with_name ("sending break");
}

static void
hardwire_raw (struct serial *scb)
{
  struct hardwire_ttystate state;

  if (get_tty_state (scb, &state))
    gdb_printf (gdb_stderr, "get_tty_state failed: %s\n",
		safe_strerror (errno));

  state.termios.c_iflag = 0;
  state.termios.c_oflag = 0;
  state.termios.c_lflag = 0;
  state.termios.c_cflag &= ~CSIZE;
  state.termios.c_cflag |= CLOCAL | CS8;
#ifdef CRTSCTS
  /* h/w flow control.  */
  if (serial_hwflow)
    state.termios.c_cflag |= CRTSCTS;
  else
    state.termios.c_cflag &= ~CRTSCTS;
#ifdef CRTS_IFLOW
  if (serial_hwflow)
    state.termios.c_cflag |= CRTS_IFLOW;
  else
    state.termios.c_cflag &= ~CRTS_IFLOW;
#endif
#endif
  state.termios.c_cc[VMIN] = 0;
  state.termios.c_cc[VTIME] = 0;

  if (set_tty_state (scb, &state))
    gdb_printf (gdb_stderr, "set_tty_state failed: %s\n",
		safe_strerror (errno));
}

#ifndef B19200
#define B19200 EXTA
#endif

#ifndef B38400
#define B38400 EXTB
#endif

/* Translate baud rates from integers to damn B_codes.  Unix should
   have outgrown this crap years ago, but even POSIX wouldn't buck it.  */

static struct
{
  int rate;
  int code;
}
baudtab[] =
{
  {
    50, B50
  }
  ,
  {
    75, B75
  }
  ,
  {
    110, B110
  }
  ,
  {
    134, B134
  }
  ,
  {
    150, B150
  }
  ,
  {
    200, B200
  }
  ,
  {
    300, B300
  }
  ,
  {
    600, B600
  }
  ,
  {
    1200, B1200
  }
  ,
  {
    1800, B1800
  }
  ,
  {
    2400, B2400
  }
  ,
  {
    4800, B4800
  }
  ,
  {
    9600, B9600
  }
  ,
  {
    19200, B19200
  }
  ,
  {
    38400, B38400
  }
  ,
#ifdef B57600
  {
    57600, B57600
  }
  ,
#endif
#ifdef B115200
  {
    115200, B115200
  }
  ,
#endif
#ifdef B230400
  {
    230400, B230400
  }
  ,
#endif
#ifdef B460800
  {
    460800, B460800
  }
  ,
#endif
#ifdef B500000
  {
    500000, B500000
  }
  ,
#endif
#ifdef B576000
  {
    576000, B576000
  }
  ,
#endif
#ifdef B921600
  {
    921600, B921600
  }
  ,
#endif
#ifdef B1000000
  {
    1000000, B1000000
  }
  ,
#endif
#ifdef B1152000
  {
    1152000, B1152000
  }
  ,
#endif
#ifdef B1500000
  {
    1500000, B1500000
  }
  ,
#endif
#ifdef B2000000
  {
    2000000, B2000000
  }
  ,
#endif
#ifdef B2500000
  {
    2500000, B2500000
  }
  ,
#endif
#ifdef B3000000
  {
    3000000, B3000000
  }
  ,
#endif
#ifdef B3500000
  {
    3500000, B3500000
  }
  ,
#endif
#ifdef B4000000
  {
    4000000, B4000000
  }
  ,
#endif
  {
    -1, -1
  }
  ,
};

static int
rate_to_code (int rate)
{
  int i;

  for (i = 0; baudtab[i].rate != -1; i++)
    {
      /* test for perfect macth.  */
      if (rate == baudtab[i].rate)
	return baudtab[i].code;
      else
	{
	  /* check if it is in between valid values.  */
	  if (rate < baudtab[i].rate)
	    {
	      if (i)
		{
		  error (_("Invalid baud rate %d.  "
			   "Closest values are %d and %d."),
			 rate, baudtab[i - 1].rate, baudtab[i].rate);
		}
	      else
		{
		  error (_("Invalid baud rate %d.  Minimum value is %d."),
			 rate, baudtab[0].rate);
		}
	    }
	}
    }
 
  /* The requested speed was too large.  */
  error (_("Invalid baud rate %d.  Maximum value is %d."),
	 rate, baudtab[i - 1].rate);
}

static void
hardwire_setbaudrate (struct serial *scb, int rate)
{
  struct hardwire_ttystate state;
  int baud_code = rate_to_code (rate);
  
  if (get_tty_state (scb, &state))
    perror_with_name ("could not get tty state");

  cfsetospeed (&state.termios, baud_code);
  cfsetispeed (&state.termios, baud_code);

  if (set_tty_state (scb, &state))
    perror_with_name ("could not set tty state");
}

static int
hardwire_setstopbits (struct serial *scb, int num)
{
  struct hardwire_ttystate state;
  int newbit;

  if (get_tty_state (scb, &state))
    return -1;

  switch (num)
    {
    case SERIAL_1_STOPBITS:
      newbit = 0;
      break;
    case SERIAL_1_AND_A_HALF_STOPBITS:
    case SERIAL_2_STOPBITS:
      newbit = 1;
      break;
    default:
      return 1;
    }

  if (!newbit)
    state.termios.c_cflag &= ~CSTOPB;
  else
    state.termios.c_cflag |= CSTOPB;	/* two bits */

  return set_tty_state (scb, &state);
}

/* Implement the "setparity" serial_ops callback.  */

static int
hardwire_setparity (struct serial *scb, int parity)
{
  struct hardwire_ttystate state;
  int newparity = 0;

  if (get_tty_state (scb, &state))
    return -1;

  switch (parity)
    {
    case GDBPARITY_NONE:
      newparity = 0;
      break;
    case GDBPARITY_ODD:
      newparity = PARENB | PARODD;
      break;
    case GDBPARITY_EVEN:
      newparity = PARENB;
      break;
    default:
      internal_warning ("Incorrect parity value: %d", parity);
      return -1;
    }

  state.termios.c_cflag &= ~(PARENB | PARODD);
  state.termios.c_cflag |= newparity;

  return set_tty_state (scb, &state);
}


static void
hardwire_close (struct serial *scb)
{
  if (scb->fd < 0)
    return;

  close (scb->fd);
  scb->fd = -1;
}



/* The hardwire ops.  */

static const struct serial_ops hardwire_ops =
{
  "hardwire",
  hardwire_open,
  hardwire_close,
  NULL,
  ser_base_readchar,
  ser_base_write,
  hardwire_flush_output,
  hardwire_flush_input,
  hardwire_send_break,
  hardwire_raw,
  hardwire_get_tty_state,
  hardwire_copy_tty_state,
  hardwire_set_tty_state,
  hardwire_print_tty_state,
  hardwire_setbaudrate,
  hardwire_setstopbits,
  hardwire_setparity,
  hardwire_drain_output,
  ser_base_async,
  ser_unix_read_prim,
  ser_unix_write_prim
};

void _initialize_ser_hardwire ();
void
_initialize_ser_hardwire ()
{
  serial_add_interface (&hardwire_ops);

#ifdef CRTSCTS
  add_setshow_boolean_cmd ("remoteflow", no_class,
			   &serial_hwflow, _("\
Set use of hardware flow control for remote serial I/O."), _("\
Show use of hardware flow control for remote serial I/O."), _("\
Enable or disable hardware flow control (RTS/CTS) on the serial port\n\
when debugging using remote targets."),
			   NULL,
			   show_serial_hwflow,
			   &setlist, &showlist);
#endif
}

int
ser_unix_read_prim (struct serial *scb, size_t count)
{
  int result = recv (scb->fd, scb->buf, count, 0);
  if (result == -1 && errno != EINTR)
    perror_with_name ("error while reading");
  return result;
}

int
ser_unix_write_prim (struct serial *scb, const void *buf, size_t len)
{
  int result = write (scb->fd, buf, len);
  if (result == -1 && errno != EINTR)
    perror_with_name ("error while writing");
  return result;
}
