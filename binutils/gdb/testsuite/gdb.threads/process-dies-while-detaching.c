/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2024 Free Software Foundation, Inc.

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
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

/* This barrier ensures we only reach the initial breakpoint after all
   threads have started.  */
pthread_barrier_t start_threads_barrier;

/* Many threads in order to be fairly sure the process exits while GDB
   is detaching from each thread in the process, on targets that need
   to detach from each thread individually.  */
#define NTHREADS 256

/* GDB sets a watchpoint here.  */
int globalvar = 1;

/* GDB reads this.  */
int mypid;

/* Threads' entry point.  */

void *
thread_function (void *arg)
{
  pthread_barrier_wait (&start_threads_barrier);
  _exit (0);
}

/* The fork child's entry point.  */

void
child_function (void)
{
  pthread_t threads[NTHREADS];
  int i;

  pthread_barrier_init (&start_threads_barrier, NULL, NTHREADS + 1);

  for (i = 0; i < NTHREADS; i++)
    pthread_create (&threads[i], NULL, thread_function, NULL);
  pthread_barrier_wait (&start_threads_barrier);

  exit (0);
}

/* This is defined by the .exp file if testing the multi-process
   variant.  */
#ifdef MULTIPROCESS

/* The fork parent's entry point.  */

void
parent_function (pid_t child)
{
  int status, ret;

  alarm (300);

  ret = waitpid (child, &status, 0);

  if (ret == -1)
    {
      printf ("waitpid, errno=%d (%s)\n", errno, strerror (errno));
      exit (1);
    }
  else if (WIFEXITED (status))
    {
      printf ("exited, status=%d\n", WEXITSTATUS (status));
      exit (0);
    }
  else if (WIFSIGNALED (status))
    {
      printf ("signaled, sig=%d\n", WTERMSIG (status));
      exit (2);
    }
  else
    {
      printf ("unexpected, status=%x\n", status);
      exit (3);
    }
}

#endif

int
main (void)
{
#ifdef MULTIPROCESS
  pid_t child;

  child = fork ();
  if (child == -1)
    return 1;
#endif

  mypid = getpid ();

#ifdef MULTIPROCESS
  if (child != 0)
    parent_function (child);
  else
#endif
    child_function ();

  /* Not reached.  */
  abort ();
}
