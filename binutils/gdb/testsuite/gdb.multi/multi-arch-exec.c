/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>

#define NUM_THREADS 1

static pthread_barrier_t barrier;

static void *
thread_start (void *arg)
{
  pthread_barrier_wait (&barrier);
  pthread_barrier_wait (&barrier);
  pthread_barrier_wait (&barrier);

  while (1)
    sleep (1);
  return NULL;
}

static void
all_started (void)
{
}

int
main (int argc, char ** argv)
{
  char prog[PATH_MAX];
  pthread_t thread;
  int len;

  strcpy (prog, argv[0]);
  len = strlen (prog);
  /* Replace "multi-arch-exec" with "multi-arch-exec-hello".  */
  memcpy (prog + len - 15, "multi-arch-exec-hello", 21);
  prog[len + 6] = 0;

  pthread_barrier_init (&barrier, NULL, NUM_THREADS + 1);
  pthread_create (&thread, NULL, thread_start, NULL);

  pthread_barrier_wait (&barrier);
  all_started ();

  /* Avoid races with GDB ptrace-resuming the threads and the exec: ensure
     both threads were resumed by GDB before going into the exec.  */
  pthread_barrier_wait (&barrier);
  pthread_barrier_wait (&barrier);

  execl (prog,
         prog,
         (char *) NULL);
  perror ("execl failed");
  exit (1);
}
