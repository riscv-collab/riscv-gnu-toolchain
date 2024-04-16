/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

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

#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#define NUM_THREADS 4

static pthread_barrier_t barrier;

static void *
thread_function (void *arg)
{
  pthread_barrier_wait (&barrier);

  for (int i = 0; i < 30; i++)
    sleep (1);

  return NULL;
}

static void
all_threads_started (void)
{}

int
main (void)
{
  pthread_t threads[NUM_THREADS];

  pthread_barrier_init (&barrier, NULL, NUM_THREADS + 1);

  for (int i = 0; i < NUM_THREADS; i++)
    {
      int res = pthread_create (&threads[i], NULL, thread_function, NULL);
      assert (res == 0);
    }

  pthread_barrier_wait (&barrier);
  all_threads_started ();

  for (int i = 0; i < NUM_THREADS; i++)
    {
      int res = pthread_join (threads[i], NULL);
      assert (res == 0);
    }

  return 0;
}

