/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

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

#define NUM 2

static pthread_barrier_t threads_started_barrier;

static void *
thread_function (void *arg)
{
  pthread_barrier_wait (&threads_started_barrier);

  while (1)
    {
      /* Sleep a bit to give the other threads a chance to run.  */
      usleep (1); /* set breakpoint here */
    }

  pthread_exit (NULL);
}

static void
all_started (void)
{
}

int
main ()
{
  pthread_t threads[NUM];
  long i;

  pthread_barrier_init (&threads_started_barrier, NULL, NUM + 1);

  for (i = 1; i <= NUM; i++)
    {
      int res;

      res = pthread_create (&threads[i - 1],
			    NULL,
			    thread_function, NULL);
    }

  pthread_barrier_wait (&threads_started_barrier);

  all_started ();

  sleep (180);

  exit (EXIT_SUCCESS);
}
