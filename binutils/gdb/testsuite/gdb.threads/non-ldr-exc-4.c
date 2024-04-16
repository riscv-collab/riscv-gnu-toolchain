/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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
#include <stdio.h>
#include <string.h>
#include <assert.h>

static const char *image;
static pthread_barrier_t barrier;
static char *argv1 = "go away";

static void *
thread_execler (void *arg)
{
  int i;

  pthread_barrier_wait (&barrier);

  /* Exec ourselves again.  */
  if (execl (image, image, argv1, NULL) == -1) /* break-here */
    {
      perror ("execl");
      abort ();
    }

  return NULL;
}

static void *
just_loop (void *arg)
{
  unsigned int i;

  pthread_barrier_wait (&barrier);

  for (i = 1; i > 0; i++)
    usleep (1);

  return NULL;
}

#define THREADS 5

pthread_t loop_thread[THREADS];

int
main (int argc, char **argv)
{
  pthread_t thread;
  int i, t;

  image = argv[0];

  /* Pass "inf" as argument to keep re-execing ad infinitum, which can
     be useful for manual testing.  Passing any other argument exits
     immediately (and that's what the execl above does by
     default).  */
  if (argc == 2 && strcmp (argv[1], "inf") == 0)
    argv1 = argv[1];
  else if (argc > 1)
    exit (0);

  pthread_barrier_init (&barrier, NULL, 2 + THREADS);

  i = pthread_create (&thread, NULL, thread_execler, NULL);
  assert (i == 0);

  for (t = 0; t < THREADS; t++)
    {
      i = pthread_create (&loop_thread[t], NULL, just_loop, NULL);
      assert (i == 0);
    }

  pthread_barrier_wait (&barrier);
  pthread_join (thread, NULL);
  return 0;
}
