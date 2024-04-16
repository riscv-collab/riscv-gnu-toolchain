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

#define NUM_THREADS 2

static pthread_barrier_t barrier;

static void *
thread_func (void *v)
{
  int i;

  pthread_barrier_wait (&barrier);

  for (i = 0; i < 100; i++)
    sleep (1);
}

static void
all_threads_created (void)
{
}

int
main (void)
{
  int i;
  pthread_t threads[NUM_THREADS];

  /* +1 to account for the main thread */
  pthread_barrier_init (&barrier, NULL, NUM_THREADS + 1);

  for (i = 0; i < NUM_THREADS; i++)
    pthread_create (&threads[i], NULL, thread_func, NULL);

  pthread_barrier_wait (&barrier);

  all_threads_created ();

  for (i = 0; i < 100; i++)
    sleep (1);

  return 0;
}
