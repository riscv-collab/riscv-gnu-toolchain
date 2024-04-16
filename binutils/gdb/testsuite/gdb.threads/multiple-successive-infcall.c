/* This testcase is part of GDB, the GNU debugger.

   Copyright 2018-2024 Free Software Foundation, Inc.

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
#include <pthread.h>

/* This defines the number of threads to spawn.  */
#define THREADCOUNT 4

/* Global barrier type to control synchronization between threads.  */
static pthread_barrier_t print_barrier;

/* Define global thread identifiers.  */
static pthread_t threads[THREADCOUNT];

/* Hold values for each thread at the index supplied to the thread
   on creation.  */
static int thread_ids[THREADCOUNT];

/* Find the value associated with the calling thread.  */
static int
get_value ()
{
  for (int tid = 0; tid < THREADCOUNT; ++tid)
    {
      if (pthread_equal (threads[tid], pthread_self ()))
	return thread_ids[tid];
    }
  /* Value for the main thread.  */
  return 1;
}

/* Return the nth Fibonacci number.  */
static unsigned long
fast_fib (unsigned int n)
{
  int a = 0;
  int b = 1;
  int t;
  for (unsigned int i = 0; i < n; ++i)
    {
      t = b;
      b = a + b;
      a = t;
    }
  return a;
}

/* Encapsulate the synchronization of the threads. Perform a barrier before
   and after the computation.  */
static void *
thread_function (void *args)
{
  int tid = *((int *) args);
  int status = pthread_barrier_wait (&print_barrier);
  if (status == PTHREAD_BARRIER_SERIAL_THREAD)
    printf ("All threads entering compute region\n");

  unsigned long result = fast_fib (100); /* testmarker01 */
  status = pthread_barrier_wait (&print_barrier);
  if (status == PTHREAD_BARRIER_SERIAL_THREAD)
    printf ("All threads outputting results\n");

  pthread_barrier_wait (&print_barrier);
  printf ("Thread %d Result: %lu\n", tid, result);
}

int
main (void)
{
  int err = pthread_barrier_init (&print_barrier, NULL, THREADCOUNT);
  if (err != 0)
    {
      fprintf (stderr, "Barrier creation failed\n");
      return EXIT_FAILURE;
    }
  /* Create the worker threads (main).  */
  printf ("Spawning worker threads\n");
  for (int tid = 0; tid < THREADCOUNT; ++tid)
    {
      /* Add 2 so the value maps to the debugger's thread identifiers.  */
      thread_ids[tid] = tid + 2; /* prethreadcreationmarker */
      err = pthread_create (&threads[tid], NULL, thread_function,
			    (void *) &thread_ids[tid]);
      if (err != 0)
	{
	  fprintf (stderr, "Thread creation failed\n");
	  return EXIT_FAILURE;
	}
    }
  /* Wait for the threads to complete then exit.  */
  for (int tid = 0; tid < THREADCOUNT; ++tid)
    pthread_join (threads[tid], NULL);

  return EXIT_SUCCESS;
}
