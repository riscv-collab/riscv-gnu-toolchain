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

#include <assert.h>
#include <pthread.h>
#include <unistd.h>

/* Number of threads.  Each thread continuously steps over a
   breakpoint.  */
#define NTHREADS 10

pthread_t threads[NTHREADS];

pthread_barrier_t barrier;

/* Used to create a conditional breakpoint that always fails.  */
volatile int zero;

static void *
thread_func (void *arg)
{
  pthread_barrier_wait (&barrier);

  while (1)
    {
      usleep (1); /* set break here */
    }

  return NULL;
}

int
main (void)
{
  int ret;
  int i;

  /* Don't run forever.  */
  alarm (180);

  pthread_barrier_init (&barrier, NULL, NTHREADS + 1);

  /* Start the threads that constantly hits a conditional breakpoint
     that needs to be stepped over.  */
  for (i = 0; i < NTHREADS; i++)
    {
      ret = pthread_create (&threads[i], NULL, thread_func, NULL);
      assert (ret == 0);
    }

  /* Wait until all threads are up and running.  */
  pthread_barrier_wait (&barrier);

  /* Let them start hitting the breakpoint.  */
  usleep (100);

  /* Exit abruptly.  */
  return 0;
}
