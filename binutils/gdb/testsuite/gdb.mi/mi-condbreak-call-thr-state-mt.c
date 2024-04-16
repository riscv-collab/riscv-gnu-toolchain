/* This testcase is part of GDB, the GNU debugger.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

/* This is the multi-threaded driver for the real test.  */

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

extern int test (void);

#define NTHREADS 5
pthread_barrier_t barrier;

void *
thread_func (void *arg)
{
  pthread_barrier_wait (&barrier);

  while (1)
    sleep (1);
}

void
create_thread (void)
{
  pthread_t tid;

  if (pthread_create (&tid, NULL, thread_func, NULL))
    {
      perror ("pthread_create");
      exit (1);
    }
}

int
main (void)
{
  int i;

  pthread_barrier_init (&barrier, NULL, NTHREADS + 1);

  for (i = 0; i < NTHREADS; i++)
    create_thread ();
  pthread_barrier_wait (&barrier);

  return test ();
}
