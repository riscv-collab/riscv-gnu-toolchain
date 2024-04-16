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

#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <assert.h>
#include <unistd.h>

/* Count the number of tasks/threads in the PID thread group.  */

static int
count_tasks (pid_t pid)
{
  char path[100];
  int count;
  DIR *d;

  snprintf (path, sizeof (path), "/proc/%d/task/", (int) pid);
  d = opendir (path);
  if (d == NULL)
    return -1;

  for (count = 0; readdir (d) != NULL; count++)
    ;
  closedir (d);

  /* Account for '.' and '..'.  */
  assert (count > 2);
  return count - 2;
}

pthread_attr_t attr[CPU_SETSIZE];
pthread_t thr[CPU_SETSIZE];

static void *
mythread (void *_arg)
{
  return NULL;
}

int
main ()
{
  int i;

  for (i = 0; i < CPU_SETSIZE; i++)
    {
      cpu_set_t set;
      int ret;

      pthread_attr_init (&attr[i]);
      CPU_ZERO_S (sizeof (set), &set);
      CPU_SET_S (i, sizeof (set), &set);

      ret = pthread_attr_setaffinity_np (&attr[i], sizeof (set), &set);
      if (ret != 0)
	{
	  fprintf (stderr, "set_affinity: %d: %s\n", ret, strerror (ret));
	  exit (3);
	}
      ret = pthread_create (&thr[i], &attr[i], mythread, NULL);
      /* Should fail with EINVAL at some point.  */
      if (ret != 0)
	{
	  unsigned long t;

	  fprintf (stderr, "pthread_create: %d: %s\n", ret, strerror (ret));

	  /* Wait for all threads to exit.  pthread_create spawns a
	     clone thread even in the failing case, as it can only try
	     to set the affinity after creating the thread.  That new
	     thread is immediately canceled (because setting the
	     affinity fails), by killing it with a SIGCANCEL signal,
	     which may end up in pthread_cancel/unwind paths, which
	     may trigger a libgcc_s.so load, making the thread hit the
	     solib-event breakpoint.  Now, if we would let the program
	     exit without waiting, sometimes it would happen that the
	     inferior exits just while we're handling the solib-event,
	     resulting in errors being thrown due to failing ptrace
	     call fails (with ESCHR), breaking the test.  */
	  t = 16;
	  while (count_tasks (getpid ()) > 1)
	    {
	      usleep (t);

	      if (t < 256)
		t *= 2;
	    }

	  /* Normal exit, because this is what we are expecting.  */
	  exit (0);
	}
    }

  /* Should not normally be reached.  */
  exit (1);
}
