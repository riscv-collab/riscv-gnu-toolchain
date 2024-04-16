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

   Check that watchpoints get propagated to all existing threads when the
   watchpoint is created.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#ifndef NR_THREADS
#define NR_THREADS 4
#endif

#ifndef X_INCR_COUNT
#define X_INCR_COUNT 10
#endif

void *thread_function (void *arg); /* Function executed by each thread.  */

pthread_mutex_t x_mutex;
int x;

/* Used to hold threads back until watchthreads2.exp is ready.  */
int test_ready = 0;

int
main ()
{
  int res;
  pthread_t threads[NR_THREADS];
  int i;

  pthread_mutex_init (&x_mutex, NULL);

  for (i = 0; i < NR_THREADS; ++i)
    {
      res = pthread_create (&threads[i],
			    NULL,
			    thread_function,
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
   watchthreads2.exp uses this to track when all threads are running
   instead of, for example, the program keeping track
   because we don't need the program to know when all threads are running,
   instead we need gdb to know when all threads are running.
   There is a delay between when a thread has started and when the thread
   has been registered with gdb.  */

void
thread_started ()
{
}

void *
thread_function (void *arg)
{
  int i;

  thread_started ();

  /* Don't start incrementing X until watchthreads2.exp is ready.  */
  while (! test_ready)
    usleep (1);

  for (i = 0; i < X_INCR_COUNT; ++i)
    {
      pthread_mutex_lock (&x_mutex);
      /* For debugging.  */
      printf ("Thread %ld changing x %d -> %d\n", (long) arg, x, x + 1);
      /* The call to usleep is so that when the watchpoint triggers,
	 the pc is still on the same line.  */
      ++x; usleep (1);  /* X increment.  */
      pthread_mutex_unlock (&x_mutex);
    }

  pthread_exit (NULL);
}
