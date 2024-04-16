/* Test program for non-stop debugging.
   Copyright 1996-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int exit_first_thread = 0;

void break_at_me (int id, int i)
{
}

void *
worker (void *arg)
{
  int id = *(int *)arg;
  int i = 0;
  
  /* When gdb is running, it sets hidden breakpoints in the thread
     library.  The signals caused by these hidden breakpoints can
     cause system calls such as 'sleep' to return early.  Pay attention
     to the return value from 'sleep' to get the full sleep.  */
  for (;;++i)
    {
      int unslept = 1;
      while (unslept > 0)
	unslept = sleep (unslept);

      if (exit_first_thread && id == 0)
	return NULL;

      break_at_me (id, i);
    }
}

pthread_t
create_thread (int id)
{
  pthread_t tid;
  /* This memory will be leaked, we don't care for a test.  */
  int *id2 = malloc (sizeof (int));
  *id2 = id;

  if (pthread_create (&tid, NULL, worker, (void *) id2))
    {
      perror ("pthread_create 1");
      exit (1);
    }
  return tid;
}

int
main (int argc, char *argv[])
{
  pthread_t tid;
  create_thread (0);
  sleep (1);
  tid = create_thread (1);
  pthread_join (tid, NULL);

  return 0;
}

