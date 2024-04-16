/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

#include <pthread.h>
#include <unistd.h>
#include <memory.h>

#define NTHR 3
#define NBOGUSTHR 2

int thr_data[NTHR];

/* Thread handles for each thread plus some "bogus" threads.  */
pthread_t thrs[NTHR + NBOGUSTHR];

/* The thread children will meet at this barrier. */
pthread_barrier_t c_barrier;

/* The main thread and child thread will meet at this barrier. */
pthread_barrier_t mc_barrier;

void
do_something (int n)
{
}

void *
do_work (void *data)
{
  int num = * (int *) data;

  /* As the child threads are created, they'll meet the main thread
     at this barrier.  We do this to ensure that threads end up in
     GDB's thread list in the order in which they were created.  Having
     this ordering makes it easier to write the test.  */
  pthread_barrier_wait (&mc_barrier);

  /* All of the child threads will meet at this barrier before proceeding.
     This ensures that all threads will be active (not exited) and in
     roughly the same state when the first one hits the breakpoint in
     do_something().  */
  pthread_barrier_wait (&c_barrier);

  do_something (num);

  pthread_exit (NULL);
}

void
after_mc_barrier (void)
{
}

int
main (int argc, char **argv)
{
  int i;

  pthread_barrier_init (&c_barrier, NULL, NTHR - 1);
  pthread_barrier_init (&mc_barrier, NULL, 2);

  thrs[0] = pthread_self ();
  thr_data[0] = 1;

  /* Create two bogus thread handles.  */
  memset (&thrs[NTHR], 0, sizeof (pthread_t));
  memset (&thrs[NTHR + 1], 0xaa, sizeof (pthread_t));

  for (i = 1; i < NTHR; i++)
    {
      thr_data[i] = i + 1;

      pthread_create (&thrs[i], NULL, do_work, &thr_data[i]);
      pthread_barrier_wait (&mc_barrier);
      after_mc_barrier ();
    }

  for (i = 1; i < NTHR; i++)
    pthread_join (thrs[i], NULL);
}
