/* Test case for forgotten hw-watchpoints after fork()-off of a process.

   Copyright 2012-2024 Free Software Foundation, Inc.

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
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <sys/wait.h>

#include "watchpoint-fork.h"

void
forkoff (int nr)
{
  pid_t child, pid_got;
  int exit_code = 42 + nr;
  int status, i;

  child = fork ();
  switch (child)
    {
    case -1:
      assert (0);
    case 0:
#if DEBUG
      printf ("child%d: %d\n", nr, (int) getpid ());
      /* Delay to get both the "child%d" and "parent%d" message printed without
	 a race breaking expect by its endless wait on `$gdb_prompt$':
	 Breakpoint 3, marker () at ../../../gdb/testsuite/gdb.threads/watchpoint-fork.c:33
	 33      }
	 (gdb) parent2: 14223  */
      i = sleep (1);
      assert (i == 0);
#endif

      /* We must not get caught here (against a forgotten breakpoint).  */
      var++;
      marker ();

      _exit (exit_code);
    default:
#if DEBUG
      printf ("parent%d: %d\n", nr, (int) child);
      /* Delay to get both the "child%d" and "parent%d" message printed, see
	 above.  */
      i = sleep (1);
      assert (i == 0);
#endif

      pid_got = wait (&status);
      assert (pid_got == child);
      assert (WIFEXITED (status));
      assert (WEXITSTATUS (status) == exit_code);

      /* We must get caught here (against a false watchpoint removal).  */
      marker ();
    }
}
