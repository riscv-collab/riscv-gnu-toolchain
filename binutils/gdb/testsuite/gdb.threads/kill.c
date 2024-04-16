/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

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

#ifdef USE_THREADS

#include <unistd.h>
#include <pthread.h>

#define NUM 5

pthread_barrier_t barrier;

void *
thread_function (void *arg)
{
  volatile unsigned int counter = 1;

  pthread_barrier_wait (&barrier);

  while (counter > 0)
    {
      counter++;
      usleep (1);
    }

  pthread_exit (NULL);
}

#endif /* USE_THREADS */

void
setup (void)
{
#ifdef USE_THREADS
  pthread_t threads[NUM];
  int i;

  pthread_barrier_init (&barrier, NULL, NUM + 1);
  for (i = 0; i < NUM; i++)
    pthread_create (&threads[i], NULL, thread_function, NULL);
  pthread_barrier_wait (&barrier);
#endif /* USE_THREADS */
}

int
main (void)
{
  setup ();
  return 0; /* set break here */
}
