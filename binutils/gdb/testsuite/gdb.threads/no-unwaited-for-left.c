/* Copyright 2007-2024 Free Software Foundation, Inc.

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
#include <assert.h>
#include <unistd.h>

static volatile pthread_t main_thread;
pthread_barrier_t barrier;

static void *
thread_a (void *arg)
{
  int i;

  return 0; /* break-here */
}

static void *
thread_b (void *arg)
{
  int i;

  pthread_barrier_wait (&barrier);

  i = pthread_join (main_thread, NULL);
  assert (i == 0);

  return arg;
}

int
main (void)
{
  pthread_t thread;
  int i;

  /* First test resuming only `thread_a', which exits.  */
  i = pthread_create (&thread, NULL, thread_a, NULL);
  assert (i == 0);
  pthread_join (thread, NULL);

  /* Then test resuming only the leader, which also exits.  */
  main_thread = pthread_self ();

  pthread_barrier_init (&barrier, NULL, 2);

  i = pthread_create (&thread, NULL, thread_b, NULL);
  assert (i == 0);

  pthread_barrier_wait (&barrier);

  pthread_exit (NULL); /* break-here-2 */
  /* NOTREACHED */
  return 0;
}
