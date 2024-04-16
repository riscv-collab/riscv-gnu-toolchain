/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2024 Free Software Foundation, Inc.

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
#include <sys/types.h>
#include <sys/wait.h>

/* GDB sets watchpoint here.  */
static volatile int __attribute__ ((used)) globalvar;

/* Whether it's expected that the child exits with a signal, vs
   exiting normally.  GDB sets this.  */
static volatile int expect_signaled;

static void
marker (void)
{
}

static void
child_function (void)
{
}

int
main (void)
{
  pid_t child;

  child = fork ();
  if (child == -1)
    exit (1);
  else if (child != 0)
    {
      int status, ret;

      ret = waitpid (child, &status, 0);
      if (ret == -1)
	exit (2);
      else if (expect_signaled && !WIFSIGNALED (status))
	exit (3);
      else if (!expect_signaled && !WIFEXITED (status))
	exit (4);
      else
	marker ();
    }
  else
    child_function ();

  exit (0);
}
