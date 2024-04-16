/* Host support routines for MinGW, for GDB, the GNU debugger.

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
#include "main.h"
#include "serial.h"
#include "gdbsupport/event-loop.h"
#include "gdbsupport/gdb_select.h"
#include "inferior.h"

#include <windows.h>
#include <signal.h>

/* Return an absolute file name of the running GDB, if possible, or
   ARGV0 if not.  The return value is in malloc'ed storage.  */

char *
windows_get_absolute_argv0 (const char *argv0)
{
  char full_name[PATH_MAX];

  if (GetModuleFileName (NULL, full_name, PATH_MAX))
    return xstrdup (full_name);
  return xstrdup (argv0);
}

/* Wrapper for select.  On Windows systems, where the select interface
   only works for sockets, this uses the GDB serial abstraction to
   handle sockets, consoles, pipes, and serial ports.

   The arguments to this function are the same as the traditional
   arguments to select on POSIX platforms.  */

int
gdb_select (int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	    struct timeval *timeout)
{
  static HANDLE never_handle;
  HANDLE handles[MAXIMUM_WAIT_OBJECTS];
  HANDLE h;
  DWORD event;
  DWORD num_handles;
  /* SCBS contains serial control objects corresponding to file
     descriptors in READFDS and WRITEFDS.  */
  struct serial *scbs[MAXIMUM_WAIT_OBJECTS];
  /* The number of valid entries in SCBS.  */
  size_t num_scbs;
  int fd;
  int num_ready;
  size_t indx;

  if (n == 0)
    {
      /* The MS API says that the first argument to
	 WaitForMultipleObjects cannot be zero.  That's why we just
	 use a regular Sleep here.  */
      if (timeout != NULL)
	Sleep (timeout->tv_sec * 1000 + timeout->tv_usec / 1000);

      return 0;
    }

  num_ready = 0;
  num_handles = 0;
  num_scbs = 0;
  for (fd = 0; fd < n; ++fd)
    {
      HANDLE read = NULL, except = NULL;
      struct serial *scb;

      /* There is no support yet for WRITEFDS.  At present, this isn't
	 used by GDB -- but we do not want to silently ignore WRITEFDS
	 if something starts using it.  */
      gdb_assert (!writefds || !FD_ISSET (fd, writefds));

      if ((!readfds || !FD_ISSET (fd, readfds))
	  && (!exceptfds || !FD_ISSET (fd, exceptfds)))
	continue;

      scb = serial_for_fd (fd);
      if (scb)
	{
	  serial_wait_handle (scb, &read, &except);
	  scbs[num_scbs++] = scb;
	}

      if (read == NULL)
	read = (HANDLE) _get_osfhandle (fd);
      if (except == NULL)
	{
	  if (!never_handle)
	    never_handle = CreateEvent (0, FALSE, FALSE, 0);

	  except = never_handle;
	}

      if (readfds && FD_ISSET (fd, readfds))
	{
	  gdb_assert (num_handles < MAXIMUM_WAIT_OBJECTS);
	  handles[num_handles++] = read;
	}

      if (exceptfds && FD_ISSET (fd, exceptfds))
	{
	  gdb_assert (num_handles < MAXIMUM_WAIT_OBJECTS);
	  handles[num_handles++] = except;
	}
    }

  gdb_assert (num_handles <= MAXIMUM_WAIT_OBJECTS);

  event = WaitForMultipleObjects (num_handles,
				  handles,
				  FALSE,
				  timeout
				  ? (timeout->tv_sec * 1000
				     + timeout->tv_usec / 1000)
				  : INFINITE);
  /* EVENT can only be a value in the WAIT_ABANDONED_0 range if the
     HANDLES included an abandoned mutex.  Since GDB doesn't use
     mutexes, that should never occur.  */
  gdb_assert (!(WAIT_ABANDONED_0 <= event
		&& event < WAIT_ABANDONED_0 + num_handles));
  /* We no longer need the helper threads to check for activity.  */
  for (indx = 0; indx < num_scbs; ++indx)
    serial_done_wait_handle (scbs[indx]);
  if (event == WAIT_FAILED)
    return -1;
  if (event == WAIT_TIMEOUT)
    return 0;
  /* Run through the READFDS, clearing bits corresponding to descriptors
     for which input is unavailable.  */
  h = handles[event - WAIT_OBJECT_0];
  for (fd = 0, indx = 0; fd < n; ++fd)
    {
      HANDLE fd_h;

      if ((!readfds || !FD_ISSET (fd, readfds))
	  && (!exceptfds || !FD_ISSET (fd, exceptfds)))
	continue;

      if (readfds && FD_ISSET (fd, readfds))
	{
	  fd_h = handles[indx++];
	  /* This handle might be ready, even though it wasn't the handle
	     returned by WaitForMultipleObjects.  */
	  if (fd_h != h && WaitForSingleObject (fd_h, 0) != WAIT_OBJECT_0)
	    FD_CLR (fd, readfds);
	  else
	    num_ready++;
	}

      if (exceptfds && FD_ISSET (fd, exceptfds))
	{
	  fd_h = handles[indx++];
	  /* This handle might be ready, even though it wasn't the handle
	     returned by WaitForMultipleObjects.  */
	  if (fd_h != h && WaitForSingleObject (fd_h, 0) != WAIT_OBJECT_0)
	    FD_CLR (fd, exceptfds);
	  else
	    num_ready++;
	}
    }

  return num_ready;
}

/* Map COLOR's RGB triplet, with 8 bits per component, into 16 Windows
   console colors, where each component has just 1 bit, plus a single
   intensity bit which affects all 3 components.  */
static int
rgb_to_16colors (const ui_file_style::color &color)
{
  uint8_t rgb[3];
  color.get_rgb (rgb);

  int retval = 0;
  for (int i = 0; i < 3; i++)
    {
      /* Subdivide 256 possible values of each RGB component into 3
	 regions: no color, normal color, bright color.  256 / 3 = 85,
	 but ui-style.c follows xterm and uses 92 for R and G
	 components of the bright-blue color, so we bias the divisor a
	 bit to have the bright colors between 9 and 15 identical to
	 what ui-style.c expects.  */
      int bits = rgb[i] / 93;
      retval |= ((bits > 0) << (2 - i)) | ((bits > 1) << 3);
    }

  return retval;
}

/* Zero if not yet initialized, 1 if stdout is a console device, else -1.  */
static int mingw_console_initialized;

/* Handle to stdout . */
static HANDLE hstdout = INVALID_HANDLE_VALUE;

/* Text attribute to use for normal text (the "none" pseudo-color).  */
static SHORT  norm_attr;

/* The most recently applied style.  */
static ui_file_style last_style;

/* Alternative for the libc 'fputs' which handles embedded SGR
   sequences in support of styling.  */

int
gdb_console_fputs (const char *linebuf, FILE *fstream)
{
  if (!mingw_console_initialized)
    {
      hstdout = (HANDLE)_get_osfhandle (fileno (fstream));
      DWORD cmode;
      CONSOLE_SCREEN_BUFFER_INFO csbi;

      if (hstdout != INVALID_HANDLE_VALUE
	  && GetConsoleMode (hstdout, &cmode) != 0
	  && GetConsoleScreenBufferInfo (hstdout, &csbi))
	{
	  norm_attr = csbi.wAttributes;
	  mingw_console_initialized = 1;
	}
      else if (hstdout != INVALID_HANDLE_VALUE)
	mingw_console_initialized = -1; /* valid, but not a console device */
    }
  /* If our stdout is not a console device, let the default 'fputs'
     handle the task. */
  if (mingw_console_initialized <= 0)
    return 0;

  /* Mapping between 8 ANSI colors and Windows console attributes.  */
  static int fg_color[] = {
    0,					/* black */
    FOREGROUND_RED,			/* red */
    FOREGROUND_GREEN,			/* green */
    FOREGROUND_GREEN | FOREGROUND_RED,	/* yellow */
    FOREGROUND_BLUE,			/* blue */
    FOREGROUND_BLUE | FOREGROUND_RED,	/* magenta */
    FOREGROUND_BLUE | FOREGROUND_GREEN, /* cyan */
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE /* gray */
  };
  static int bg_color[] = {
    0,					/* black */
    BACKGROUND_RED,			/* red */
    BACKGROUND_GREEN,			/* green */
    BACKGROUND_GREEN | BACKGROUND_RED,	/* yellow */
    BACKGROUND_BLUE,			/* blue */
    BACKGROUND_BLUE | BACKGROUND_RED,	/* magenta */
    BACKGROUND_BLUE | BACKGROUND_GREEN, /* cyan */
    BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE /* gray */
  };

  ui_file_style style = last_style;
  unsigned char c;
  size_t n_read;

  for ( ; (c = *linebuf) != 0; linebuf += n_read)
    {
      if (c == '\033')
	{
	  fflush (fstream);
	  bool parsed = style.parse (linebuf, &n_read);
	  if (n_read <= 0)	/* should never happen */
	    n_read = 1;
	  if (!parsed)
	    {
	      /* This means we silently swallow SGR sequences we
		 cannot parse.  */
	      continue;
	    }
	  /* Colors.  */
	  const ui_file_style::color &fg = style.get_foreground ();
	  const ui_file_style::color &bg = style.get_background ();
	  int fgcolor, bgcolor, bright, inverse;
	  if (fg.is_none ())
	    fgcolor = norm_attr & 15;
	  else if (fg.is_basic ())
	    fgcolor = fg_color[fg.get_value () & 15];
	  else
	    fgcolor = rgb_to_16colors (fg);
	  if (bg.is_none ())
	    bgcolor = norm_attr & (15 << 4);
	  else if (bg.is_basic ())
	    bgcolor = bg_color[bg.get_value () & 15];
	  else
	    bgcolor = rgb_to_16colors (bg) << 4;

	  /* Intensity.  */
	  switch (style.get_intensity ())
	    {
	    case ui_file_style::NORMAL:
	    case ui_file_style::DIM:
	      bright = 0;
	      break;
	    case ui_file_style::BOLD:
	      bright = 1;
	      break;
	    default:
	      gdb_assert_not_reached ("invalid intensity");
	    }

	  /* Inverse video.  */
	  if (style.is_reverse ())
	    inverse = 1;
	  else
	    inverse = 0;

	  /* Construct the attribute.  */
	  if (inverse)
	    {
	      int t = fgcolor;
	      fgcolor = (bgcolor >> 4);
	      bgcolor = (t << 4);
	    }
	  if (bright)
	    fgcolor |= FOREGROUND_INTENSITY;

	  SHORT attr = (bgcolor & (15 << 4)) | (fgcolor & 15);

	  /* Apply the attribute.  */
	  SetConsoleTextAttribute (hstdout, attr);
	}
      else
	{
	  /* When we are about to write newline, we need to clear to
	     EOL with the normal attribute, to avoid spilling the
	     colors to the next screen line.  We assume here that no
	     non-default attribute extends beyond the newline.  */
	  if (c == '\n')
	    {
	      DWORD nchars;
	      COORD start_pos;
	      DWORD written;
	      CONSOLE_SCREEN_BUFFER_INFO csbi;

	      fflush (fstream);
	      GetConsoleScreenBufferInfo (hstdout, &csbi);

	      if (csbi.wAttributes != norm_attr)
		{
		  start_pos = csbi.dwCursorPosition;
		  nchars = csbi.dwSize.X - start_pos.X;

		  FillConsoleOutputAttribute (hstdout, norm_attr, nchars,
					      start_pos, &written);
		  FillConsoleOutputCharacter (hstdout, ' ', nchars,
					      start_pos, &written);
		}
	    }
	  fputc (c, fstream);
	  n_read = 1;
	}
    }

  last_style = style;
  return 1;
}

/* See inferior.h.  */

tribool
sharing_input_terminal (int pid)
{
  std::vector<DWORD> results (10);
  DWORD len = 0;
  while (true)
    {
      len = GetConsoleProcessList (results.data (), results.size ());
      /* Note that LEN == 0 is a failure, but we can treat it the same
	 as a "no".  */
      if (len <= results.size ())
	break;

      results.resize (len);
    }
  /* In case the vector was too big.  */
  results.resize (len);
  if (std::find (results.begin (), results.end (), pid) != results.end ())
    {
      /* The pid is in the list sharing the console, so don't
	 interrupt the inferior -- it will get the signal itself.  */
      return TRIBOOL_TRUE;
    }

  return TRIBOOL_FALSE;
}

/* Current C-c handler.  */
static c_c_handler_ftype *current_handler;

/* The Windows callback that forwards requests to the C-c handler.  */
static BOOL WINAPI
ctrl_c_handler (DWORD event_type)
{
  if (event_type == CTRL_BREAK_EVENT || event_type == CTRL_C_EVENT)
    {
      if (current_handler != SIG_IGN)
	current_handler (SIGINT);
    }
  else
    return FALSE;
  return TRUE;
}

/* See inferior.h.  */

c_c_handler_ftype *
install_sigint_handler (c_c_handler_ftype *fn)
{
  /* We want to make sure the gdb handler always comes first, so that
     gdb gets to handle the C-c.  This is why the handler is always
     removed and reinstalled here.  Note that trying to remove the
     function without installing it first will cause a crash.  */
  static bool installed = false;
  if (installed)
    SetConsoleCtrlHandler (ctrl_c_handler, FALSE);
  SetConsoleCtrlHandler (ctrl_c_handler, TRUE);
  installed = true;

  c_c_handler_ftype *result = current_handler;
  current_handler = fn;
  return result;
}
