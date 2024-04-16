/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

#define NUM_THREADS 2

static volatile int unblock_main[NUM_THREADS];

static void
child_sub_function (int child_idx)
{
  volatile int dummy = 0;

  unblock_main[child_idx] = 1;

  while (1)
    /* Dummy loop body to allow setting breakpoint.  */
    dummy = !dummy; /* thread loop line */
}

static void *
child_function (void *args)
{
  int child_idx = (int) (uintptr_t) args;

  child_sub_function (child_idx); /* thread caller line */

  return NULL;
}

int
main (void)
{
  int i = 0;
  pthread_t threads[NUM_THREADS];

  /* Make the test exit eventually.  */
  alarm (20);

  for (i = 0; i < NUM_THREADS; i++)
    pthread_create (&threads[i], NULL, child_function, (void *) (uintptr_t) i);

  /* Wait for child threads to reach child_sub_function.  */
  for (i = 0; i < NUM_THREADS; i++)
    while (!unblock_main[i])
      ;

  volatile int dummy = 0;
  while (1)
    /* Dummy loop body to allow setting breakpoint.  */
    dummy = !dummy; /* main break line */

  return 0;
}
