/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

void
do_nothing (void)
{
}

void
handle (int sig)
{
  do_nothing (); /* handle marker */
}

int
main ()
{
  int i;
  signal (SIGHUP, handle);

  raise (SIGHUP);		/* first HUP */

  signal (SIGCHLD, handle);
  for (i = 0; i < 3; i++)	/* fork loop */
    {
      switch (fork())
	{
	case -1:
	  perror ("fork");
	  exit (1);
	case 0:
	  exit (0);
	}
      wait (NULL);
    }

  raise (SIGHUP);		/* second HUP */

  raise (SIGHUP);		/* third HUP */
}

