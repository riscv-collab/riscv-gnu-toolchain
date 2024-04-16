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

#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <asm/unistd.h>
#include <unistd.h>
#define gettid() syscall (__NR_gettid)

/* Non-atomic `var++' should not hurt as we synchronize the threads by the STEP
   variable.  Hit-comments need to be duplicated there to catch both at-stops
   and behind-stops, depending on the target.  */

volatile int var;

void
marker (void)
{
}

static void
empty (void)
{
}

static void
mark_exit (void)
{
}

pthread_t thread;
volatile int step;

static void *
start (void *arg)
{
  int i;

  if (step >= 3)
    goto step_3;

  while (step != 1)
    {
      i = sched_yield ();
      assert (i == 0);
    }

  var++;	/* validity-thread-B */
  empty ();	/* validity-thread-B */
  step = 2;
  while (step != 3)
    {
      if (step == 99)
	goto step_99;

      i = sched_yield ();
      assert (i == 0);
    }

step_3:
  if (step >= 5)
    goto step_5;

  var++;	/* after-fork1-B */
  empty ();	/* after-fork1-B */
  step = 4;
  while (step != 5)
    {
      if (step == 99)
	goto step_99;

      i = sched_yield ();
      assert (i == 0);
    }

step_5:
  var++;	/* after-fork2-B */
  empty ();	/* after-fork2-B */
  return (void *) 5UL;

step_99:
  /* We must not get caught here (against a forgotten breakpoint).  */
  var++;
  marker ();
  return (void *) 99UL;
}

int
main (void)
{
  int i;
  void *thread_result;

#if DEBUG
  setbuf (stdout, NULL);
  printf ("main: %d\n", (int) gettid ());
#endif

  /* General hardware breakpoints and watchpoints validity.  */
  marker ();
  var++;	/* validity-first */
  empty ();	/* validity-first */

  i = pthread_create (&thread, NULL, start, NULL);
  assert (i == 0);

  var++;	/* validity-thread-A */
  empty ();	/* validity-thread-A */
  step = 1;
  while (step != 2)
    {
      i = sched_yield ();
      assert (i == 0);
    }

  /* Hardware watchpoints got disarmed here.  */
  forkoff (1);

  var++;	/* after-fork1-A */
  empty ();	/* after-fork1-A */
  step = 3;
#ifdef FOLLOW_CHILD
  /* Spawn new thread as it was deleted in the child of FORK.  */
  i = pthread_create (&thread, NULL, start, NULL);
  assert (i == 0);
#endif
  while (step != 4)
    {
      i = sched_yield ();
      assert (i == 0);
    }

  /* A sanity check for double hardware watchpoints removal.  */
  forkoff (2);

  var++;	/* after-fork2-A */
  empty ();	/* after-fork2-A */
  step = 5;
#ifdef FOLLOW_CHILD
  /* Spawn new thread as it was deleted in the child of FORK.  */
  i = pthread_create (&thread, NULL, start, NULL);
  assert (i == 0);
#endif

  i = pthread_join (thread, &thread_result);
  assert (i == 0);
  assert (thread_result == (void *) 5UL);

  mark_exit ();
  return 0;
}
