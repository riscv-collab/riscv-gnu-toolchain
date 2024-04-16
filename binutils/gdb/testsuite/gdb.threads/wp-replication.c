/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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

   Check that hardware watchpoints get correctly replicated to all
   existing threads when hardware watchpoints are created.  This test
   creates one hardware watchpoint per thread until a maximum is
   reached.  It originally addresses a deficiency seen on embedded
   powerpc targets with slotted hardware *point designs.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#ifndef NR_THREADS
#define NR_THREADS 4 /* Set by the testcase.  */
#endif

#ifndef X_INCR_COUNT
#define X_INCR_COUNT 10 /* Set by the testcase.  */
#endif

void *thread_function (void *arg); /* Function executed by each thread.  */

/* Used to hold threads back until wp-replication.exp is ready.  */
int test_ready = 0;

/* Used to hold threads back until every thread has had a chance of causing
   a watchpoint trigger.  This prevents a situation in GDB where it may miss
   watchpoint triggers when threads exit while other threads are causing
   watchpoint triggers.  */
int can_terminate = 0;

/* Number of watchpoints GDB is capable of using (this is provided
   by GDB during the test run).  */
int hw_watch_count = 0;

/* Array with elements we can create watchpoints for.  */
static int watched_data[NR_THREADS];
pthread_mutex_t data_mutex;

int
main ()
{
  int res;
  pthread_t threads[NR_THREADS];
  int i;

  pthread_mutex_init (&data_mutex, NULL);

  for (i = 0; i < NR_THREADS; i++)
    {
      res = pthread_create (&threads[i],
			    NULL, thread_function,
			    (void *) (intptr_t) i);
      if (res != 0)
	{
	  fprintf (stderr, "error in thread %d create\n", i);
	  abort ();
	}
    }

  for (i = 0; i < NR_THREADS; ++i)
    {
      res = pthread_join (threads[i], NULL);
      if (res != 0)
	{
	  fprintf (stderr, "error in thread %d join\n", i);
	  abort ();
	}
    }

  exit (EXIT_SUCCESS);
}

/* Easy place for a breakpoint.
   wp-replication.exp uses this to track when all threads are running
   instead of, for example, the program keeping track
   because we don't need the program to know when all threads are running,
   instead we need gdb to know when all threads are running.
   There is a delay between when a thread has started and when the thread
   has been registered with gdb.  */

void
thread_started (void)
{
}

void *
thread_function (void *arg)
{
  int i, j;
  long thread_number = (long) arg;

  thread_started ();

  /* Don't start incrementing X until wp-replication.exp is ready.  */
  while (!test_ready)
    usleep (1);

  pthread_mutex_lock (&data_mutex);

  for (i = 0; i < NR_TRIGGERS_PER_THREAD; i++)
    {
      for (j = 0; j < hw_watch_count; j++)
	{
	  /* For debugging.  */
	  printf ("Thread %ld changing watch_thread[%d] data"
	          " from %d -> %d\n", thread_number, j,
	          watched_data[j], watched_data[j] + 1);
	  /* Increment the watched data field.  */
	  watched_data[j]++;
	}
    }

  pthread_mutex_unlock (&data_mutex);

  /* Hold the threads here to work around a problem GDB has evaluating
     watchpoints right when a DSO event shows up (PR breakpoints/10116).
     Sleep a little longer (than, say, 1, 5 or 10) to avoid consuming
     lots of cycles while the other threads are trying to execute the
     loop.  */
  while (!can_terminate)
    usleep (100);

  pthread_exit (NULL);
}
