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

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "../lib/my-syscalls.h"

#define NUM_THREADS 32

static void *
stepper_over_exit_thread (void *v)
{
  my_exit (0);

  /* my_exit above should exit the thread, we don't expect to reach
     here.  */
  abort ();
}

static void *
spawner_thread (void *v)
{
  for (;;)
    {
      pthread_t threads[NUM_THREADS];
      int i;

      for (i = 0; i < NUM_THREADS; i++)
	pthread_create (&threads[i], NULL, stepper_over_exit_thread, NULL);

      for (i = 0; i < NUM_THREADS; i++)
	pthread_join (threads[i], NULL);
    }
}

static void
break_here (void)
{
}

static void *
breakpoint_hitter_thread (void *v)
{
  for (;;)
    break_here ();
}

int
main ()
{
  pthread_t breakpoint_hitter;
  pthread_t spawner;

  alarm (60);

  pthread_create (&spawner, NULL, spawner_thread, NULL);
  pthread_create (&breakpoint_hitter, NULL, breakpoint_hitter_thread, NULL);

  pthread_join (spawner, NULL);

  return 0;
}
