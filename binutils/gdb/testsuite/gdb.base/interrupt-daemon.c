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

#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

static void
daemon_main (void)
{
}

int
main ()
{
  pid_t child;

  alarm (60);

  /* Normally we're a progress group leader at this point, so can't
     create a session.  Fork so the child can create a new
     session.  */
  child = fork ();
  if (child == -1)
    return 1;
  else if (child != 0)
    return 0;
  else
    {
      /* In child.  Switch to a new session.  */
      pid_t session = setsid ();
      if (session == -1)
	return 1;

      /* Fork again, so that the grand child (what we want to debug)
	 can't accidentally acquire a controlling terminal, because
	 it's not a session leader.  We're not opening any file here,
	 but this is representative of what daemons do.  */
      child = fork ();
      if (child == -1)
	return 1;
      else if (child != 0)
	return 0;

      /* In grandchild.  */
      daemon_main ();

      while (1)
	sleep (1);
    }

  return 0;
}
