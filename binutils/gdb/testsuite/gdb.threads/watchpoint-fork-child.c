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

#include "watchpoint-fork.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <stdio.h>

/* `pid_t' may not be available.  */

static volatile int usr1_got;

static void
handler_usr1 (int signo)
{
  usr1_got++;
}

void
forkoff (int nr)
{
  int child, save_parent = getpid ();
  int i;
  struct sigaction act, oldact;
#ifdef THREAD
  void *thread_result;
#endif

  memset (&act, 0, sizeof act);
  act.sa_flags = SA_RESTART;
  act.sa_handler = handler_usr1;
  sigemptyset (&act.sa_mask);
  i = sigaction (SIGUSR1, &act, &oldact);
  assert (i == 0);

  child = fork ();
  switch (child)
    {
    case -1:
      assert (0);
    default:
#if DEBUG
      printf ("parent%d: %d\n", nr, (int) child);
#endif

      /* Sleep for a while to possibly get incorrectly ATTACH_THREADed by GDB
	 tracing the child fork with no longer valid thread/lwp entries of the
	 parent.  */

      i = sleep (2);
      assert (i == 0);

      /* We must not get caught here (against a forgotten breakpoint).  */

      var++;
      marker ();

#ifdef THREAD
      /* And neither got caught our thread.  */

      step = 99;
      i = pthread_join (thread, &thread_result);
      assert (i == 0);
      assert (thread_result == (void *) 99UL);
#endif

      /* Be sure our child knows we did not get caught above.  */

      i = kill (child, SIGUSR1);
      assert (i == 0);

      /* Sleep for a while to check GDB's `info threads' no longer tracks us in
	 the child fork.  */

      i = sleep (2);
      assert (i == 0);

      _exit (0);
    case 0:
#if DEBUG
      printf ("child%d: %d\n", nr, (int) getpid ());
#endif

      /* Let the parent signal us about its success.  Be careful of races.  */

      for (;;)
	{
	  /* Parent either died (and USR1_GOT is zero) or it succeeded.  */
	  if (getppid () != save_parent)
	    break;
	  if (kill (getppid (), 0) != 0)
	    break;
	  /* Parent succeeded?  */
	  if (usr1_got)
	    break;

#ifdef THREAD
	  i = pthread_yield ();
	  assert (i == 0);
#endif
	}
      assert (usr1_got);

      /* We must get caught here (against a false watchpoint removal).  */

      marker ();
    }

  i = sigaction (SIGUSR1, &oldact, NULL);
  assert (i == 0);
}
