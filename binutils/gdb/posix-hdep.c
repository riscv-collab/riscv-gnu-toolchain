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
#include "gdbsupport/event-loop.h"
#include "gdbsupport/gdb_select.h"
#include "inferior.h"
#include <signal.h>

/* Wrapper for select.  Nothing special needed on POSIX platforms.  */

int
gdb_select (int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	    struct timeval *timeout)
{
  return select (n, readfds, writefds, exceptfds, timeout);
}

/* Host-dependent console fputs method.  POSIX platforms always return
   zero, to use the default C 'fputs'.  */
int
gdb_console_fputs (const char *buf, FILE *f)
{
  return 0;
}

/* See inferior.h.  */

tribool
sharing_input_terminal (int pid)
{
  /* Using host-dependent code here is fine, because the
     child_terminal_foo functions are meant to be used by child/native
     targets.  */
#if defined (__linux__) || defined (__sun__)
  char buf[100];

  xsnprintf (buf, sizeof (buf), "/proc/%d/fd/0", pid);
  return is_gdb_terminal (buf);
#else
  return TRIBOOL_UNKNOWN;
#endif
}

/* Current C-c handler.  */
static c_c_handler_ftype *current_handler;

/* A wrapper that reinstalls the current signal handler.  */
static void
handler_wrapper (int num)
{
  signal (num, handler_wrapper);
  if (current_handler != SIG_IGN)
    current_handler (num);
}

/* See inferior.h.  */

c_c_handler_ftype *
install_sigint_handler (c_c_handler_ftype *fn)
{
  signal (SIGINT, handler_wrapper);
  c_c_handler_ftype *result = current_handler;
  current_handler = fn;
  return result;
}
