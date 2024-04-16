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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

pthread_barrier_t barrier;

#define NUM_THREADS 2

void *
thread_function (void *arg)
{
  /* This ensures that the breakpoint is only hit after both threads
     are created, so the test can always switch to the non-event
     thread when the breakpoint triggers.  */
  pthread_barrier_wait (&barrier);

  while (1); /* break here */
}

int
main (void)
{
  int i;

  alarm (300);

  pthread_barrier_init (&barrier, NULL, NUM_THREADS);

  for (i = 0; i < NUM_THREADS; i++)
    {
      pthread_t thread;
      int res;

      res = pthread_create (&thread, NULL,
			    thread_function, NULL);
      assert (res == 0);
    }

  while (1)
    sleep (1);

  return 0;
}
