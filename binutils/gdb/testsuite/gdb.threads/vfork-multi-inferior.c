/* This testcase is part of GDB, the GNU debugger.

   Copyright 2022-2024 Free Software Foundation, Inc.

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
#include <sys/wait.h>
#include <assert.h>

static void
should_break_here (void)
{
}

int
main (void)
{
  int i;

  for (i = 0; i < NR_LOOPS; i++)
    {
      int pid = vfork ();

      if (pid != 0)
	{
	  /* Parent */
	  int stat;
	  int ret = waitpid (pid, &stat, 0);
	  assert (ret == pid);
	  assert (WIFEXITED (stat));
	  assert (WEXITSTATUS (stat) == 12);

	  should_break_here ();
	}
      else
	{
	  /* Child */
	  _exit (12);
	}
    }

  return 0;
}
