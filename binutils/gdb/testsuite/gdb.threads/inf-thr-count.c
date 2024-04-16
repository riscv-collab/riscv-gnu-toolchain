/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023-2024 Free Software Foundation, Inc.

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

/* Set to 0 by GDB to cause the inferior to drop out of a spin loop.  */
volatile int spin = 1;

/* Set by the inferior to communicate to GDB what stage of the test we are
   in.  Initially 0, but set to 1 once a new thread has been created.  Then
   set to 2 once the extra thread has exited.  */
volatile int stage = 0;

/* New thread worker function.  Just spins until GDB tells it to exit.  */
void *
thread_func (void *arg)
{
  assert (arg == NULL);

  stage = 1;

  while (spin)
    sleep (1);

  return NULL;
}

/* Somewhere to place a breakpoint.  */
void
breakpt ()
{
  /* Nothing.  */
}

/* Create a new test that spins until GDB tells it to exit.  Then, once the
   new thread has exited, this thread spins until GDB tells us to exit.  */
int
main ()
{
  alarm (600);

  breakpt ();

  pthread_t thr;
  int res;

  res = pthread_create (&thr, NULL, thread_func, NULL);
  assert (res == 0);

  void *retval;
  res = pthread_join (thr, &retval);
  assert (res == 0);

  spin = 1;
  stage = 2;

  while (spin)
    sleep (1);

  breakpt ();

  return 0;
}

