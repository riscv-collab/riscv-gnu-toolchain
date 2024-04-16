/* Create threads from multiple threads in parallel.
   Copyright 2007-2024 Free Software Foundation, Inc.

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

#include <pthread.h>
#include <stdio.h>
#include <limits.h>

#define NUM_CREATE 1
#define NUM_THREAD 8

void *
thread_function (void *arg)
{
  int x = * (int *) arg;

  printf ("Thread <%d> executing\n", x);

  return NULL;
}

void *
create_function (void *arg)
{
  pthread_attr_t attr;
  pthread_t threads[NUM_THREAD];
  int args[NUM_THREAD];
  size_t stacksize;
  int i = * (int *) arg;
  int j;

  pthread_attr_init (&attr); /* set breakpoint 1 here.  */
  pthread_attr_getstacksize (&attr, &stacksize);
  pthread_attr_setstacksize (&attr, 2 * stacksize);

  /* Create a ton of quick-executing threads, then wait for them to
     complete.  */
  for (j = 0; j < NUM_THREAD; ++j)
    {
      args[j] = i * 1000 + j;
      pthread_create (&threads[j], &attr, thread_function, &args[j]);
    }

  for (j = 0; j < NUM_THREAD; ++j)
    pthread_join (threads[j], NULL);

  pthread_attr_destroy (&attr);

  return NULL;
}

int 
main (int argc, char **argv)
{
  pthread_attr_t attr;
  pthread_t threads[NUM_CREATE];
  int args[NUM_CREATE];
  size_t stacksize;
  int n, i;

  pthread_attr_init (&attr);
  pthread_attr_getstacksize (&attr, &stacksize);
  pthread_attr_setstacksize (&attr, 2 * stacksize);

  for (n = 0; n < 100; ++n)
    {
      for (i = 0; i < NUM_CREATE; i++)
	{
	  args[i] = i;
	  pthread_create (&threads[i], &attr, create_function, &args[i]);
	}

      create_function (&i);
      for (i = 0; i < NUM_CREATE; i++)
	pthread_join (threads[i], NULL);
    }

  pthread_attr_destroy (&attr);

  return 0;
}
