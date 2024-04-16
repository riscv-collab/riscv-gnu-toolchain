/* This testcase is part of GDB, the GNU debugger.

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

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

int save_parent;

/* Variable set by GDB.  If true, then a fork child (or parent) exits
   if its parent (or child) exits.  Otherwise the process waits
   forever until either GDB or the alarm kills it.  */
volatile int exit_if_relative_exits = 0;

/* The fork child.  Just runs forever.  */

static int
fork_child (void)
{
  /* Don't run forever.  */
  alarm (180);

  while (1)
    {
      if (exit_if_relative_exits)
	{
	  sleep (1);

	  /* Exit if GDB kills the parent.  */
	  if (getppid () != save_parent)
	    break;
	  if (kill (getppid (), 0) != 0)
	    break;
	}
      else
	pause ();
    }

  return 0;
}

/* The fork parent.  Just runs forever.  */

static int
fork_parent (void)
{
  /* Don't run forever.  */
  alarm (180);

  while (1)
    {
      if (exit_if_relative_exits)
	{
	  int res = wait (NULL);
	  if (res == -1 && errno == EINTR)
	    continue;
	  else if (res == -1)
	    {
	      perror ("wait");
	      return 1;
	    }
	  else
	    return 0;
	}
      else
	pause ();
    }

  return 0;
}

int
main (void)
{
  pid_t pid;

  save_parent = getpid ();

  /* The parent and child should basically run forever without
     tripping on any debug event.  We want to check that GDB updates
     the parent and child running states correctly right after the
     fork.  */
  pid = fork ();
  if (pid > 0)
    return fork_parent ();
  else if (pid == 0)
    return fork_child ();
  else
    {
      perror ("fork");
      return 1;
    }
}
