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
#include <unistd.h>
#include <stdlib.h>

/* Always zero, used in breakpoint condition.  */
volatile int global_zero;

static pthread_barrier_t threads_started_barrier;

void *
child_function (void *arg)
{
  pthread_barrier_wait (&threads_started_barrier);

  while (1)
    {
      usleep (1); /* set breakpoint child here */
    }

  pthread_exit (NULL);
}

int
main (void)
{
  pthread_t child_thread;
  int res;

  pthread_barrier_init (&threads_started_barrier, NULL, 2);

  res = pthread_create (&child_thread, NULL, child_function, NULL);

  pthread_barrier_wait (&threads_started_barrier);

  sleep (2); /* set wait-thread breakpoint here */
  exit (EXIT_SUCCESS);
}
