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

#include <pthread.h>
#include <assert.h>
#include <unistd.h>

static void *
thread_func (void *arg)
{
#if !SCHEDLOCK
  while (1)
    sleep (1);
#endif

  return NULL;
}

int
main (void)
{
  pthread_t thread;
  int ret;

  ret = pthread_create (&thread, NULL, thread_func, NULL); /* set break 1 here */
  assert (ret == 0);

#if SCHEDLOCK
  /* When testing with schedlock enabled, the new thread won't run, so
     we can't join it, as that would hang forever.  Instead, sleep for
     a bit, enough that if the spawned thread is scheduled, it hits
     the thread_func breakpoint before the main thread reaches the
     "return 0" line below.  */
  sleep (3);
#else
  pthread_join (thread, NULL);
#endif

  return 0; /* set break 2 here */
}
