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

#include <stdlib.h>
#include <unistd.h>

/* GDB reads this to figure out the child's PID.  */
pid_t child_pid;

void
marker (void)
{
}

int
main ()
{
  /* Don't let the test case run forever.  */
  alarm (60);

  child_pid = fork ();

  switch (child_pid)
    {
    case -1:
      return 1;

    case 0:
      break;

    default:
      /* In parent process.  */
      while (1)
	{
	  marker ();
	  usleep (1000);
	}
    }

  /* In child process.  */

  /* Alarms are not inherited by child processes. Set the alarm again to stop
     the test case running forever.  */
  alarm (60);

  /* Detach from controlling terminal.  */
  if (setsid () == (pid_t) -1)
    return 1;

  while (1)
    usleep (1000);

  return 0;
}
