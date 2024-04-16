/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUMTHREADS 10

volatile int done = 0;
static pthread_barrier_t barrier;

static void *
start (void *arg)
{
  while (!done)
    usleep (100);
  assert (0);
  return arg;
}

void *
thread_function (void *arg)
{
  int x = * (int *) arg;

  printf ("Thread <%d> executing\n", x);

  pthread_barrier_wait (&barrier);

  while (!done)
    usleep (100);

  return NULL;
}

void *
thread_forker (void *arg)
{
  int x = * (int *) arg;
  pid_t pid;
  int rv;
  int i;
  pthread_t thread;

  printf ("Thread forker <%d> executing\n", x);

  pthread_barrier_wait (&barrier);

  switch ((pid = fork ()))
    {
    case -1:
      assert (0);
    default:
      wait (&rv);
      done = 1;
      break;
    case 0:
      i = pthread_create (&thread, NULL, start, NULL);
      assert (i == 0);
      i = pthread_join (thread, NULL);
      assert (i == 0);

      assert (0);
    }

  return NULL;
}

int
main (void)
{
  pthread_t threads[NUMTHREADS];
  int args[NUMTHREADS];
  int i, j;

  alarm (600);

  i = pthread_barrier_init (&barrier, NULL, NUMTHREADS);
  assert (i == 0);

  /* Create a few threads that do mostly nothing, and then one that
     forks.  */
  for (j = 0; j < NUMTHREADS - 1; ++j)
    {
      args[j] = j;
      pthread_create (&threads[j], NULL, thread_function, &args[j]);
    }

  args[j] = j;
  pthread_create (&threads[j], NULL, thread_forker, &args[j]);

  for (j = 0; j < NUMTHREADS; ++j)
    {
      pthread_join (threads[j], NULL);
    }

  return 0;
}
