/* This testcase is part of GDB, the GNU debugger.

   Copyright 2022-2024 Free Software Foundation, Inc.

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
#include <sys/syscall.h>

#define NUM_THREADS 32

pthread_barrier_t barrier;

static void
do_exit (int exitcode)
{
  /* Synchronize all threads up to here so that they all exit at
     roughly the same time.  */
  pthread_barrier_wait (&barrier);

  /* All threads exit with SYS_exit, even the main thread, to avoid
     exiting with a group-exit syscall, as that syscall changes the
     exit status of all still-alive threads, thus potentially masking
     a bug.  */
  syscall (SYS_exit, exitcode);
}

static void *
start (void *arg)
{
  int thread_return_value = *(int *) arg;

  do_exit (thread_return_value);
}

int
main(void)
{
  pthread_t threads[NUM_THREADS];
  int thread_return_val[NUM_THREADS];
  int i;

  pthread_barrier_init (&barrier, NULL, NUM_THREADS + 1);

  for (i = 0; i < NUM_THREADS; ++i)
    {
      thread_return_val[i] = i + 2;
      pthread_create (&threads[i], NULL, start, &thread_return_val[i]);
    }

  do_exit (1);
}
