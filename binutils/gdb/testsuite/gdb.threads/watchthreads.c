/* This testcase is part of GDB, the GNU debugger.

   Copyright 2002-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
   This file is copied from schedlock.c.  */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

void *thread_function (void *arg); /* Function executed by each thread.  */

#define NUM 5

unsigned int args[NUM+1];

int
main ()
{
    int res;
    pthread_t threads[NUM];
    void *thread_result;
    long i;

    /* To keep the test determinative, initialize args first,
       then start all the threads.  Otherwise, the way watchthreads.exp
       is written, we have to worry about things like threads[0] getting
       to 29 hits of args[0] before args[1] gets changed.  */

    for (i = 0; i < NUM; i++)
      {
	/* The call to usleep is so that when the watchpoint triggers,
	   the pc is still on the same line.  */
	args[i] = 1; usleep (1); /* Init value.  */
      }

    for (i = 0; i < NUM; i++)
      {
	res = pthread_create (&threads[i],
			      NULL,
			      thread_function,
			      (void *) i);
      }

    args[i] = 1;
    thread_function ((void *) i);

    exit (EXIT_SUCCESS);
}

void *
thread_function (void *arg)
{
    int my_number =  (long) arg;
    int *myp = (int *) &args[my_number];

    /* Don't run forever.  Run just short of it :)  */
    while (*myp > 0)
      {
	(*myp) ++; usleep (1);  /* Loop increment.  */
      }

    pthread_exit (NULL);
}

