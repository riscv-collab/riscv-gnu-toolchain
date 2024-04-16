/* Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

/* Force output to unbuffered mode if not connected to a terminal.  */

#include <stdio.h>
#ifndef __MINGW32__
#include <unistd.h>
#endif

static void
gdb_unbuffer_output (void)
{
  /* Always force this for Windows testing.  To a native Windows
     program running under a Cygwin shell/ssh, stdin is really a
     Windows pipe, thus not a tty and its outputs ends up fully
     buffered.  */
#ifndef __MINGW32__
  if (!isatty (fileno (stdin)))
#endif
    {
      setvbuf (stdout, NULL, _IONBF, BUFSIZ);
      setvbuf (stderr, NULL, _IONBF, BUFSIZ);
    }
}
