/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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

#include <stdio.h>

static size_t total_bytes = 0;

int
main ()
{
  int i = 0;

  /* When testing with "target remote |", stdout is a pipe, and thus
     block buffered by default.  Force it to be line buffered.  */
  setvbuf (stdout, NULL, _IOLBF, 0);

  /* This outputs > 70 KB which is larger than the default pipe buffer
     size on most systems (typically 16 KB or 64 KB).  */
  for (i = 0; i < 3000; i++)
    total_bytes += printf ("this is line number %d\n", i);

  printf ("total bytes written = %u\n", (unsigned) total_bytes);
  return 0; /* printing done */
}
