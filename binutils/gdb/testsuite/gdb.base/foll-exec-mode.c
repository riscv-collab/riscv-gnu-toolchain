/* This test program is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

int global_i = 100;

int main (int argc, char ** argv)
{
  int local_j = global_i + 1;
  int local_k = local_j + 1;
  char prog[PATH_MAX];
  int len;

  strcpy (prog, argv[0]);
  len = strlen (prog);
  /* Replace "foll-exec-mode" with "execd-prog".  */
  memcpy (prog + len - 14, "execd-prog", 10);
  prog[len - 4] = 0;

  printf ("foll-exec is about to execlp(execd-prog)...\n");

  execlp (prog,     /* Set breakpoint here.  */
	  "/execd-prog",
	  "execlp arg1 from foll-exec",
	  (char *) 0);
}
