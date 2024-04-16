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


/* Number of times the main process forks.  */
#define NFORKS 10

/* Number of threads by each fork child.  */
#define NTHREADS 10

static void *
thread_func (void *arg)
{
  /* Empty.  */
  return NULL;
}

static void
fork_child (void)
{
  pthread_t threads[NTHREADS];
  int i;
  int ret;

  for (i = 0; i < NTHREADS; i++)
    {
      ret = pthread_create (&threads[i], NULL, thread_func, NULL);
      assert (ret == 0);
    }

  for (i = 0; i < NTHREADS; i++)
    {
      ret = pthread_join (threads[i], NULL);
      assert (ret == 0);
    }
}

int
main (void)
{
  pid_t childs[NFORKS];
  int i;
  int status;
  int num_exited = 0;

  /* Don't run forever if the wait loop below gets stuck.  */
  alarm (180);

  for (i = 0; i < NFORKS; i++)
    {
      pid_t pid;

      pid = fork ();

      if (pid > 0)
	{
	  /* Parent.  */
	  childs[i] = pid;
	}
      else if (pid == 0)
	{
	  /* Child.  */
	  fork_child ();
	  return 0;
	}
      else
	{
	  perror ("fork");
	  return 1;
	}
    }

  while (num_exited != NFORKS)
    {
      pid_t pid = wait (&status);

      if (pid == -1)
	{
	  perror ("wait");
	  return 1;
	}

      if (WIFEXITED (status))
	{
	  num_exited++;
	}
      else
	{
	  printf ("Hmm, unexpected wait status 0x%x from child %d\n", status,
		  pid);
	}
    }

  return 0;
}
