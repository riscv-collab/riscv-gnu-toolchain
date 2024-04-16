/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

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
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_FORKING_THREADS 12

static pthread_barrier_t barrier;
static volatile int should_exit = 0;

static void
sigusr1_handler (int sig, siginfo_t *siginfo, void *context)
{
  should_exit = 1;
}

static void *
forking_thread (void *arg)
{
  /* Wait for all forking threads to have spawned before fork-spamming.  */
  pthread_barrier_wait (&barrier);

  while (!should_exit)
    {
      pid_t pid = fork ();
      if (pid == 0)
	{
	  /* Child */
	  exit (8);
	}
      else
	{
	  /* Parent */
	  int status;
	  int ret = waitpid (pid, &status, 0);
	  assert (ret == pid);
	  assert (WIFEXITED (status));
	  assert (WEXITSTATUS (status) == 8);
	}
    }

  return NULL;
}

static void
break_here_first (void)
{
}

static pid_t my_pid;

int
main (void)
{
  pthread_t forking_threads[NUM_FORKING_THREADS];
  int ret;
  struct sigaction sa;
  int i;

  /* Just to make sure we don't run for ever.  */
  alarm (30);

  my_pid = getpid ();

  break_here_first ();

  pthread_barrier_init (&barrier, NULL, NUM_FORKING_THREADS);

  memset (&sa, 0, sizeof (sa));
  sa.sa_sigaction = sigusr1_handler;
  ret = sigaction (SIGUSR1, &sa, NULL);
  assert (ret == 0);

  for (i = 0; i < NUM_FORKING_THREADS; ++i)
    {
      ret = pthread_create (&forking_threads[i], NULL, forking_thread, NULL);
      assert (ret == 0);
    }

  for (i = 0; i < NUM_FORKING_THREADS; ++i)
    {
      ret = pthread_join (forking_threads[i], NULL);
      assert (ret == 0);
    }

  FILE *f = fopen (TOUCH_FILE_PATH, "w");
  assert (f != NULL);
  ret = fclose (f);
  assert (ret == 0);

  return 0;
}
