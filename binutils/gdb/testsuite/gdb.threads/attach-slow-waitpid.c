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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define NUM_THREADS 4

/* Crude spin lock.  Threads all spin until this is set to 0.  */
int go = 1;

/* Thread function, just spin until GO is set to 0.  */
void *
perform_work (void *argument)
{
  /* Cast to volatile to ensure that ARGUMENT is loaded each time around
     the loop.  */
  while (*((volatile int*) argument))
    {
      /* Nothing.  */
    }
  return NULL;
}

/* The spin loop for the main thread.  */
void
function (void)
{
  (void) perform_work (&go);
  printf ("Finished from function\n");
}

/* Main program, create some threads which all spin waiting for GO to be
   set to 0.  */
int
main (void)
{
  pthread_t threads[NUM_THREADS];
  int result_code;
  unsigned index;

  /* Create some threads.  */
  for (index = 0; index < NUM_THREADS; ++index)
    {
      printf ("In main: creating thread %d\n", index);
      result_code = pthread_create (&threads[index], NULL, perform_work, &go);
      assert (!result_code);
    }

  function ();

  /* Wait for each thread to complete.  */
  for (index = 0; index < NUM_THREADS; ++index)
    {
      /* Block until thread INDEX completes.  */
      result_code = pthread_join (threads[index], NULL);
      assert (!result_code);
      printf ("In main: thread %d has completed\n", index);
    }
  printf ("In main: All threads completed successfully\n");
  return 0;
}
