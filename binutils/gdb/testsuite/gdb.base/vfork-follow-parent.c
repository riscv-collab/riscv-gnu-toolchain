/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

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

#include <string.h>
#include <limits.h>
#include <stdio.h>

static volatile int unblock_parent = 0;

static void
break_parent (void)
{
}

int
main (int argc, char **argv)
{
  alarm (30);

  if (vfork () != 0)
    {
      /* We want to guarantee that GDB processes the child exit event before
	 the parent's breakpoint hit event.  Make the parent wait on this
	 variable that is eventually set by the test.  */
      while (!unblock_parent)
	usleep (1000);

      break_parent ();
    }
  else
    {
#if defined TEST_EXEC
      char prog[PATH_MAX];
      int len;

      strcpy (prog, argv[0]);
      len = strlen (prog);
      for (; len > 0; --len)
	{
	  if (prog[len - 1] == '/')
	    break;
	}
      strcpy (&prog[len], "vforked-prog");
      execlp (prog, prog, (char *) 0);
      perror ("exec failed");
      _exit (1);
#elif defined TEST_EXIT
      _exit (0);
#else
#error Define TEST_EXEC or TEST_EXIT
#endif
    }

  return 0;
}
