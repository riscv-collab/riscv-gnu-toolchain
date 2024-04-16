/* This testcase is part of GDB, the GNU debugger.

   Copyright 2022-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
pthread_t thread2_id;
pthread_t thread3_id;

void* do_thread (void* d)
{
  if (d != NULL)
    {
      pthread_barrier_t *barrier = (pthread_barrier_t *) d;
      pthread_barrier_wait (barrier);
    }

  return NULL;			/* In thread */
}

int main (void)
{
  /* We want the threads to exit in a known order.  Use a barrier to ensure
     the second thread doesn't exit until the first has been joined.  */
  pthread_barrier_t barrier;
  pthread_barrier_init (&barrier, NULL, 2);
  pthread_create (&thread2_id, NULL, do_thread, NULL);
  pthread_create (&thread3_id, NULL, do_thread, &barrier);
  pthread_join (thread2_id, NULL);
  pthread_barrier_wait (&barrier);
  pthread_join (thread3_id, NULL);
  return 12;			/* Done */
}
