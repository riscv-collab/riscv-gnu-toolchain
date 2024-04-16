/* This testcase is part of GDB, the GNU debugger.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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

#include "unbuffer_output.c"

static int g;

void
foo (int arg)
{
  g += arg;
  g *= 2; /* set dprintf 1 here */
  g /= 2.5; /* set breakpoint 1 here */
}

int
main (int argc, char *argv[])
{
  int loc = 1234;

  gdb_unbuffer_output ();

  /* Ensure these functions are available.  */
  printf ("kickoff %d\n", loc);
  fprintf (stderr, "also to stderr %d\n", loc);

  foo (loc++);
  foo (loc++);
  foo (loc++);
  return g;
}

#include <stdlib.h>
/* Make sure function 'malloc' is linked into program.  One some bare-metal
   port, if we don't use 'malloc', it will not be linked in program.  'malloc'
   is needed, otherwise we'll see such error message

   evaluation of this expression requires the program to have a function
   "malloc".  */
void
bar (void)
{
  void *p = malloc (16);

  free (p);
}
