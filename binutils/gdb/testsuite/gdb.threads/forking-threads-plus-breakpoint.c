/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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

#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>

/* Number of threads.  Each thread continuously spawns a fork and wait
   for it.  If we have another thread continuously start a step over,
   gdbserver should end up finding new forks while suspending
   threads.  */
#define NTHREADS 10

pthread_t threads[NTHREADS];

pthread_barrier_t barrier;

#define NFORKS 10

/* Used to create a conditional breakpoint that always fails.  */
volatile int zero;

static void *
thread_forks (void *arg)
{
  int i;

  pthread_barrier_wait (&barrier);

  for (i = 0; i < NFORKS; i++)
    {
      pid_t pid;

      do
	{
	  pid = fork ();
	}
      while (pid == -1 && errno == EINTR);

      if (pid > 0)
	{
	  int status;

	  /* Parent.  */
	  do
	    {
	      pid = waitpid (pid, &status, 0);
	    }
	  while (pid == -1 && errno == EINTR);

	  if (pid == -1)
	    {
	      perror ("wait");
	      exit (1);
	    }

	  if (!WIFEXITED (status))
	    {
	      printf ("Unexpected wait status 0x%x from child %d\n",
		      status, pid);
	    }
	}
      else if (pid == 0)
	{
	  /* Child.  */
	  exit (0);
	}
      else
	{
	  perror ("fork");
	  exit (1);
	}
    }

  return NULL;
}

/* Set this to tell the thread_breakpoint thread to exit.  */
volatile int break_out;

static void *
thread_breakpoint (void *arg)
{
  pthread_barrier_wait (&barrier);

  while (!break_out)
    {
      usleep (1); /* set break here */
    }

  return NULL;
}

pthread_barrier_t barrier;

int
main (void)
{
  int i;
  int ret;
  pthread_t bp_thread;

  /* Don't run forever.  */
  alarm (180);

  pthread_barrier_init (&barrier, NULL, NTHREADS + 1);

  /* Start the threads that constantly fork.  */
  for (i = 0; i < NTHREADS; i++)
    {
      ret = pthread_create (&threads[i], NULL, thread_forks, NULL);
      assert (ret == 0);
    }

  /* Start the thread that constantly hit a conditional breakpoint
     that needs to be stepped over.  */
  ret = pthread_create (&bp_thread, NULL, thread_breakpoint, NULL);
  assert (ret == 0);

  /* Wait for forking to stop.  */
  for (i = 0; i < NTHREADS; i++)
    {
      ret = pthread_join (threads[i], NULL);
      assert (ret == 0);
    }

  break_out = 1;
  pthread_join (bp_thread, NULL);
  assert (ret == 0);

  return 0;
}
