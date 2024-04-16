/* This testcase is part of GDB, the GNU debugger.

   Copyright 2004-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>

static volatile int done;
static volatile int dummy;
static volatile int no_handler;

static void
handler (int sig)
{
  /* This is more than one write so that the breakpoint location below
     is more than one instruction away.  */
  done = 1;
  done = 1;
  done = 1;
  done = 1; /* other handler location */
} /* handler */

struct itimerval itime;
struct sigaction action;

/* The enum is so that GDB can easily see these macro values.  */
enum {
  itimer_real = ITIMER_REAL,
  itimer_virtual = ITIMER_VIRTUAL
} itimer = ITIMER_VIRTUAL;

int
main ()
{
  int res;

  /* Set up the signal handler.  */
  memset (&action, 0, sizeof (action));
  action.sa_handler = no_handler ? SIG_IGN : handler;
  sigaction (SIGVTALRM, &action, NULL);
  sigaction (SIGALRM, &action, NULL);

  /* The values needed for the itimer.  This needs to be at least long
     enough for the setitimer() call to return.  */
  memset (&itime, 0, sizeof (itime));
  itime.it_value.tv_usec = 250 * 1000;

  /* Loop for ever, constantly taking an interrupt.  */
  while (1)
    {
      /* Set up a one-off timer.  A timer, rather than SIGSEGV, is
	 used as after a timer handler finishes the interrupted code
	 can safely resume.  */
      res = setitimer (itimer, &itime, NULL);
      if (res == -1)
	{
	  printf ("First call to setitimer failed, errno = %d\r\n",errno);
	  itimer = ITIMER_REAL;
	  res = setitimer (itimer, &itime, NULL);
	  if (res == -1)
	    {
	      printf ("Second call to setitimer failed, errno = %d\r\n",errno);
	      return 1;
	    }
	}
      /* Wait.  Issue a couple writes to a dummy volatile var to be
	 reasonably sure our simple "get-next-pc" logic doesn't
	 stumble on branches.  */
      dummy = 0; dummy = 0; while (!done);
      done = 0;
    }
  return 0;
}
