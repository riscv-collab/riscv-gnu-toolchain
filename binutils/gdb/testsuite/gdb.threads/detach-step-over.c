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

#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

/* Number of threads we'll create.  */
int n_threads = 10;

int mypid;

static void
setup_done (void)
{
}

/* Entry point for threads.  Loops forever.  */

void *
thread_func (void *arg)
{
  /* Avoid setting the breakpoint at an instruction that wouldn't
     require a fixup phase, like a branch/jump.  In such a case, even
     if GDB manages to detach the inferior with an incomplete
     displaced step, GDB inferior may still not crash.  A breakpoint
     at a line that increments a variable is good bet that we end up
     setting a breakpoint at an instruction that will require a fixup
     phase to move the PC from the scratch pad to the instruction
     after the breakpoint.  */
  volatile unsigned counter = 0;

  while (1)
    {
      counter++; /* Set breakpoint here.  */
      counter++;
      counter++;
    }

  return NULL;
}

/* Allow for as much timeout as DejaGnu wants, plus a bit of
   slack.  */
#define SECONDS (TIMEOUT + 20)

/* We'll exit after this many seconds.  */
unsigned int seconds_left = SECONDS;

/* GDB sets this whenever it's about to start a new detach/attach
   sequence.  We react by resetting the seconds-left counter.  */
volatile int again = 0;

int
main (int argc, char **argv)
{
  int i;

  signal (SIGUSR1, SIG_IGN);

  mypid = getpid ();
  setup_done ();

  if (argc > 1)
    n_threads = atoi (argv[1]);

  /* Spawn the test threads.  */
  for (i = 0; i < n_threads; ++i)
    {
      pthread_t child;
      int rc;

      rc = pthread_create (&child, NULL, thread_func, NULL);
      assert (rc == 0);
    }

  /* Exit after a while if GDB is gone/crashes.  But wait long enough
     for one attach/detach sequence done by the .exp file.  */
  while (--seconds_left > 0)
    {
      sleep (1);

      if (again)
	{
	  /* GDB should be reattaching soon.  Restart the timer.  */
	  again = 0;
	  seconds_left = SECONDS;
	}
    }

  printf ("timeout, exiting\n");
  return 0;
}
