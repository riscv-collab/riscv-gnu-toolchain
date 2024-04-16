/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

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
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>

pthread_barrier_t barrier;

volatile int second_child;

void
breakpoint_function (void)
{
  /* GDB sets a breakpoint in this function.  */
}

void *
child_function_0 (void *arg)
{
  volatile unsigned int counter = 1;

  pthread_barrier_wait (&barrier);

  while (counter > 0)
    {
      counter++;
      usleep (1);

      if (second_child)
	continue;

      breakpoint_function ();
    }

  pthread_exit (NULL);
}

void *
child_function_1 (void *arg)
{
  volatile unsigned int counter = 1;

  pthread_barrier_wait (&barrier);

  while (counter > 0)
    {
      counter++;
      usleep (1);

      if (!second_child)
	continue;

      breakpoint_function ();
    }

  pthread_exit (NULL);
}

static int
wait_threads (void)
{
  return 1; /* in wait_threads */
}

int
main (void)
{
  pthread_t child_thread[2];
  int res;

  pthread_barrier_init (&barrier, NULL, 3);

  res = pthread_create (&child_thread[0], NULL, child_function_0, NULL);
  assert (res == 0);
  res = pthread_create (&child_thread[1], NULL, child_function_1, NULL);
  assert (res == 0);

  pthread_barrier_wait (&barrier);
  wait_threads (); /* set wait-thread breakpoint here */

  pthread_join (child_thread[0], NULL);
  pthread_join (child_thread[1], NULL);

  exit (EXIT_SUCCESS);
}
