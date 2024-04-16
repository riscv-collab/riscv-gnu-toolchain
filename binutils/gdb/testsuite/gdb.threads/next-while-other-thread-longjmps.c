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
#include <setjmp.h>

/* Number of threads.  */
#define NTHREADS 10

/* When set, threads exit.  */
volatile int break_out;

pthread_barrier_t barrier;

/* Entry point for threads that setjmp/longjmp.  */

static void *
thread_longjmp (void *arg)
{
  jmp_buf env;

  pthread_barrier_wait (&barrier);

  while (!break_out)
    {
      if (setjmp (env) == 0)
	longjmp (env, 1);

      usleep (1);
    }
  return NULL;
}

/* Entry point for threads that try/catch.  */

static void *
thread_try_catch (void *arg)
{
  volatile unsigned int counter = 0;

  pthread_barrier_wait (&barrier);

  while (!break_out)
    {
      try
	{
	  throw 1;
	}
      catch (...)
	{
	  counter++;
	}

      usleep (1);
    }
  return NULL;
}

int
main (void)
{
  pthread_t threads[NTHREADS];
  int i;
  int ret;

  /* Don't run forever.  */
  alarm (180);

  pthread_barrier_init (&barrier, NULL, NTHREADS + 1);

  for (i = 0; i < NTHREADS; i++)
    {
      /* Half of the threads does setjmp/longjmp, the other half does
	 try/catch.  */
      if ((i % 2) == 0)
	ret = pthread_create (&threads[i], NULL, thread_longjmp , NULL);
      else
	ret = pthread_create (&threads[i], NULL, thread_try_catch , NULL);
      assert (ret == 0);
    }

  /* Wait until all threads are running.  */
  pthread_barrier_wait (&barrier);

#define LINE usleep (1)

  /* The other thread's setjmp/longjmp/try/catch should not disturb
     this thread's stepping over these lines.  */

  LINE; /* set break here */
  LINE; /* line 1 */
  LINE; /* line 2 */
  LINE; /* line 3 */
  LINE; /* line 4 */
  LINE; /* line 5 */
  LINE; /* line 6 */
  LINE; /* line 7 */
  LINE; /* line 8 */
  LINE; /* line 9 */
  LINE; /* line 10 */

  break_out = 1;

  for (i = 0; i < NTHREADS; i++)
    {
      ret = pthread_join (threads[i], NULL);
      assert (ret == 0);
    }

  return 0;
}
