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
#include <assert.h>

#ifndef WITH_THREADS
# error "WITH_THREADS must be defined."
#endif

#if WITH_THREADS
# include <pthread.h>

static pthread_barrier_t barrier;

static void *
thread_func (void *p)
{
  pthread_barrier_wait (&barrier);

  for (int i = 0; i < 30; i++)
    sleep (1);

  return NULL;
}

#endif /* WITH_THREADS */

static void
all_started (void)
{}

int
main (void)
{
  alarm (30);

#if WITH_THREADS
  int ret = pthread_barrier_init (&barrier, NULL, 2);
  assert (ret == 0);

  pthread_t thread;
  ret = pthread_create (&thread, NULL, thread_func, NULL);
  assert (ret == 0);

  pthread_barrier_wait (&barrier);
#endif /* WITH_THREADS */

  all_started ();

  for (int i = 0; i < 30; i++)
    sleep (1);

  return 0;
}
