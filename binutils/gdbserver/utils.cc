/* General utility routines for the remote server for GDB.
   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#include "server.h"

#ifdef IN_PROCESS_AGENT
#  define PREFIX "ipa: "
#  define TOOLNAME "GDBserver in-process agent"
#else
#  define PREFIX "gdbserver: "
#  define TOOLNAME "GDBserver"
#endif

/* Generally useful subroutines used throughout the program.  */

/* If in release mode, just exit.  This avoids potentially littering
   the filesystem of small embedded targets with core files.  If in
   development mode however, abort, producing core files to help with
   debugging GDBserver.  */
static void ATTRIBUTE_NORETURN
abort_or_exit ()
{
#ifdef DEVELOPMENT
  abort ();
#else
  exit (1);
#endif
}

void
malloc_failure (long size)
{
  fprintf (stderr,
	   PREFIX "ran out of memory while trying to allocate %lu bytes\n",
	   (unsigned long) size);
  abort_or_exit ();
}

/* Print an error message and return to top level.  */

void
verror (const char *string, va_list args)
{
#ifdef IN_PROCESS_AGENT
  fflush (stdout);
  vfprintf (stderr, string, args);
  fprintf (stderr, "\n");
  exit (1);
#else
  throw_verror (GENERIC_ERROR, string, args);
#endif
}

void
vwarning (const char *string, va_list args)
{
  fprintf (stderr, PREFIX);
  vfprintf (stderr, string, args);
  fprintf (stderr, "\n");
}

/* Report a problem internal to GDBserver, and abort/exit.  */

void
internal_verror (const char *file, int line, const char *fmt, va_list args)
{
  fprintf (stderr,  "\
%s:%d: A problem internal to " TOOLNAME " has been detected.\n", file, line);
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "\n");
  abort_or_exit ();
}

/* Report a problem internal to GDBserver.  */

void
internal_vwarning (const char *file, int line, const char *fmt, va_list args)
{
  fprintf (stderr,  "\
%s:%d: A problem internal to " TOOLNAME " has been detected.\n", file, line);
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "\n");
}

/* Convert a CORE_ADDR into a HEX string, like %lx.
   The result is stored in a circular static buffer, NUMCELLS deep.  */

char *
paddress (CORE_ADDR addr)
{
  return phex_nz (addr, sizeof (CORE_ADDR));
}
