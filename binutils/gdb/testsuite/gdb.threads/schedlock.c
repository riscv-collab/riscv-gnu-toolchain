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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

void *thread_function(void *arg); /* Pointer to function executed by each thread */

#define NUM 1

unsigned long long int args[NUM+1];

int main() {
    int res;
    pthread_t threads[NUM];
    void *thread_result;
    long i;

    alarm (30);

    for (i = 1; i <= NUM; i++)
      {
	args[i] = 1;
	res = pthread_create(&threads[i - 1],
		             NULL,
			     thread_function,
			     (void *) i);
      }

    /* schedlock.exp: last thread start.  */
    args[0] = 1;
    thread_function ((void *) 0);

    exit(EXIT_SUCCESS);
}

void some_function (void) {
  /* Sleep a bit to give the other threads a chance to run, if not
     locked.  This also ensure that even if the compiler optimizes out
     or inlines some_function, there's still be some function that
     needs to be stepped over.  */
  usleep (1);
}

/* When testing "next", this is set to have the loop call
   some_function, which GDB should step over.  When testing "step",
   that would step into the function, which is not what we want.  */
volatile int call_function = 0;

/* Call some_function if CALL_FUNCTION is set.  This is wrapped in a
   macro so that it's a single source line in the main loop.  */
#define MAYBE_CALL_SOME_FUNCTION()		\
  do						\
    {						\
      if (call_function)			\
	some_function ();			\
    } while (0)

void *thread_function(void *arg) {
    int my_number =  (long) arg;
    unsigned long long int *myp = (unsigned long long int *) &args[my_number];
    volatile unsigned int one = 1;

    while (one)
      {
	/* schedlock.exp: main loop.  */
	MAYBE_CALL_SOME_FUNCTION(); (*myp) ++;
      }

    pthread_exit(NULL);
}

