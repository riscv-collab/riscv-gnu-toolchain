/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

unsigned int args[2];

pthread_barrier_t barrier;
pthread_t child_thread_2, child_thread_3;

void
sigusr1_handler (int signo)
{
}

void
callme (void)
{
}

void *
child_function_3 (void *arg)
{
  int my_number =  (long) arg;
  volatile int *myp = (int *) &args[my_number];

  pthread_barrier_wait (&barrier);

  while (*myp > 0)
    {
      (*myp) ++;
      callme (); /* set breakpoint thread 3 here */
    }

  pthread_exit (NULL);
}

void *
child_function_2 (void *arg)
{
  int my_number =  (long) arg;
  volatile int *myp = (int *) &args[my_number];

  pthread_barrier_wait (&barrier);

  while (*myp > 0)
    {
      (*myp) ++;
      callme (); /* set breakpoint thread 2 here */
    }

  pthread_exit (NULL);
}

static int
wait_threads (void)
{
  return 1; /* in wait_threads */
}

int
main ()
{
  int res;
  long i;

  signal (SIGUSR1, sigusr1_handler);

  /* Call these early so that PLTs for these are resolved soon,
     instead of in the threads.  RTLD_NOW should work as well.  */
  usleep (0);
  pthread_barrier_init (&barrier, NULL, 1);
  pthread_barrier_wait (&barrier);

  pthread_barrier_init (&barrier, NULL, 2);

  i = 0;
  args[i] = 1;
  res = pthread_create (&child_thread_2,
			NULL, child_function_2, (void *) i);
  pthread_barrier_wait (&barrier);
  callme ();

  i = 1;
  args[i] = 1;
  res = pthread_create (&child_thread_3,
			NULL, child_function_3, (void *) i);
  pthread_barrier_wait (&barrier);
  wait_threads (); /* set wait-threads breakpoint here */

  pthread_join (child_thread_2, NULL);
  pthread_join (child_thread_3, NULL);

  exit(EXIT_SUCCESS);
}
