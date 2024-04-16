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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>

int fds[2] = { -1, -1 };

static void
grandparent_done (void)
{
}

/* The exp file overrides this in order to test both fork and
   vfork.  */
#ifndef FORK
#define FORK fork
#endif

int
main (void)
{
  int pid;
  int nbytes;
  const char string[] = "Hello, world!\n";
  char readbuffer[80];

  /* Don't run forever.  */
  alarm (300);

  /* Create a pipe.  The write side will be inherited all the way to
     the grandchild.  The grandparent will read this, expecting to see
     EOF (meaning the grandchild closed the pipe).  */
  pipe (fds);

  pid = FORK ();
  if (pid < 0)
    {
      perror ("fork");
      exit (1);
    }
  else if (pid == 0)
    {
      /* Close input side of pipe.  */
      close (fds[0]);

      pid = FORK ();
      if (pid == 0)
	{
	  printf ("I'm the grandchild!\n");

	  /* Don't explicitly close the pipe.  If GDB fails to kill
	     this process, then the grandparent will hang in the pipe
	     read below.  */
#if 0
	  close (fds[1]);
#endif
	  while (1)
	    sleep (1);
	}
      else
	{
	  close (fds[1]);
	  printf ("I'm the proud parent of child #%d!\n", pid);
	  wait (NULL);
	}
    }
  else if (pid > 0)
    {
      close (fds[1]);
      printf ("I'm the proud parent of child #%d!\n", pid);
      nbytes = read (fds[0], readbuffer, sizeof (readbuffer));
      assert (nbytes == 0);
      printf ("read returned nbytes=%d\n", nbytes);
      wait (NULL);

      grandparent_done ();
    }

  return 0;
}
