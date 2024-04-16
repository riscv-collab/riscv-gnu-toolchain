/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2024 Free Software Foundation, Inc.

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
#include <unistd.h>
#include <stdlib.h>

#define NUM_THREADS 20
const int num_threads = NUM_THREADS;

static pthread_t child_thread[NUM_THREADS];

static pthread_barrier_t threads_started_barrier;

volatile int always_zero;
volatile unsigned int dummy;

static void
infinite_loop (void)
{
  while (1)
    {
      dummy++; /* set breakpoint here */
    }
}

static void *
child_function (void *arg)
{
  pthread_barrier_wait (&threads_started_barrier);

  infinite_loop ();

  return NULL;
}

void
all_started (void)
{
}

int
main (void)
{
  int res;
  int i;

  alarm (300);

  pthread_barrier_init (&threads_started_barrier, NULL, NUM_THREADS + 1);

  for (i = 0; i < NUM_THREADS; i++)
    {
      res = pthread_create (&child_thread[i], NULL, child_function, NULL);
    }

  /* Wait until all threads have been scheduled.  */
  pthread_barrier_wait (&threads_started_barrier);

  all_started ();

  infinite_loop ();
}
