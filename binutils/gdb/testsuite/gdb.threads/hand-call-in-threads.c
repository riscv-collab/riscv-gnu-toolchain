/* Test case for hand function calls in multi-threaded program.

   Copyright 2008-2024 Free Software Foundation, Inc.

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#ifndef NR_THREADS
#define NR_THREADS 4
#endif

int thread_count;

pthread_mutex_t thread_count_mutex;

pthread_cond_t thread_count_condvar;

void
incr_thread_count (void)
{
  pthread_mutex_lock (&thread_count_mutex);
  ++thread_count;
  if (thread_count == NR_THREADS)
    pthread_cond_signal (&thread_count_condvar);
  pthread_mutex_unlock (&thread_count_mutex);
}

void
cond_wait (pthread_cond_t *cond, pthread_mutex_t *mut)
{
  pthread_mutex_lock (mut);
  pthread_cond_wait (cond, mut);
  pthread_mutex_unlock (mut);
}

void
noreturn (void)
{
  pthread_mutex_t mut;
  pthread_cond_t cond;

  pthread_mutex_init (&mut, NULL);
  pthread_cond_init (&cond, NULL);

  /* Wait for a condition that will never be signaled, so we effectively
     block the thread here.  */
  cond_wait (&cond, &mut);
}

void *
forever_pthread (void *unused)
{
  incr_thread_count ();
  noreturn ();
}

void
hand_call (void)
{
}

/* Wait until all threads are running.  */

void
wait_all_threads_running (void)
{
  pthread_mutex_lock (&thread_count_mutex);
  if (thread_count == NR_THREADS)
    {
      pthread_mutex_unlock (&thread_count_mutex);
      return;
    }
  pthread_cond_wait (&thread_count_condvar, &thread_count_mutex);
  if (thread_count == NR_THREADS)
    {
      pthread_mutex_unlock (&thread_count_mutex);
      return;
    }
  pthread_mutex_unlock (&thread_count_mutex);
  printf ("failed waiting for all threads to start\n");
  abort ();
}

/* Called when all threads are running.
   Easy place for a breakpoint.  */

void
all_threads_running (void)
{
}

int
main (void)
{
  pthread_t forever[NR_THREADS];
  int i;

  pthread_mutex_init (&thread_count_mutex, NULL);
  pthread_cond_init (&thread_count_condvar, NULL);

  for (i = 0; i < NR_THREADS; ++i)
    pthread_create (&forever[i], NULL, forever_pthread, NULL);

  wait_all_threads_running ();
  all_threads_running ();

  return 0;
}

