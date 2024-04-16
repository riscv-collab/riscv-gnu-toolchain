/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>

#define MAX_LOOP_ITER 10000

static char *argv0;

static void*
worker_a (void *pArg)
{
  int iter = 0;
  char *args[] = {argv0, "self-call", NULL };

  while (iter++ < MAX_LOOP_ITER)
    {
      pid_t pid = FORK_FUNC ();
      if (pid == 0)
	{
	  /* child */
	  if (execvp (args[0], args) == -1)
	    {
	      fprintf (stderr, "execvp error: %d\n", errno);
	      exit (1);
	    }
	}

      waitpid (pid, NULL, 0);
      usleep (5);
    }
}

static void*
worker_b (void *pArg)
{
  int iter = 0;
  while (iter++ < MAX_LOOP_ITER)  /* for loop */
    {
      usleep (5);  /* break here */
      usleep (5);  /* other line */
    }
}

int
main (int argc, char **argv)
{
  pthread_t wa_pid;
  pthread_t wb_pid;

  argv0 = argv[0];

  if (argc > 1 && strcmp (argv[1], "self-call") == 0)
    exit (0);

  pthread_create (&wa_pid, NULL, worker_a, NULL);
  pthread_create (&wb_pid, NULL, worker_b, NULL);
  pthread_join (wa_pid, NULL);

  exit (0);
}
