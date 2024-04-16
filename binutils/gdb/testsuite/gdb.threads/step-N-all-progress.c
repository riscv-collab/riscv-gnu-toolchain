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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <pthread.h>
#include <unistd.h>
#include <stddef.h>

static pthread_barrier_t barrier;

static void *
thread_func (void *arg)
{
  pthread_barrier_wait (&barrier);
  return NULL;
}

int
main ()
{
  pthread_t thread;
  int ret;

  alarm (30);

  pthread_barrier_init (&barrier, NULL, 2);

  /* We run to this line below, and then issue "next 3".  That should
     step over the 3 lines below and land on the return statement.  If
     GDB prematurely stops the thread_func thread after the first of
     the 3 nexts (and never resumes it again), then the join won't
     ever return.  */
  pthread_create (&thread, NULL, thread_func, NULL); /* set break here */
  pthread_barrier_wait (&barrier);
  pthread_join (thread, NULL);

  return 0;
}
