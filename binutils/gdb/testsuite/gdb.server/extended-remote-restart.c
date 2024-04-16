/* This testcase is part of GDB, the GNU debugger.

   Copyright 2018-2024 Free Software Foundation, Inc.

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

#include <unistd.h>
#include <stdlib.h>

static void
breakpt ()
{
  asm ("" ::: "memory");
}

static void
go_child ()
{
  breakpt ();

  while (1)
    sleep (1);
}

static void
go_parent ()
{
  breakpt ();

  while (1)
    sleep (1);
}

int
main ()
{
  pid_t pid;

  pid = fork ();
  if (pid == -1)
    abort ();

  if (pid == 0)
    go_child ();
  else
    go_parent ();

  exit (EXIT_SUCCESS);
}
