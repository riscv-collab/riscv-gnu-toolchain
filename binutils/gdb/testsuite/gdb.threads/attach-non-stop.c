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

#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

/* Number of threads we'll create.  */
static const int n_threads = 10;

/* Entry point for threads.  Loops forever.  */

void *
thread_func (void *arg)
{
  while (1)
    sleep (1);

  return NULL;
}

int
main (int argc, char **argv)
{
  int i;

  alarm (30);

  /* Spawn the test threads.  */
  for (i = 0; i < n_threads; ++i)
    {
      pthread_t child;
      int rc;

      rc = pthread_create (&child, NULL, thread_func, NULL);
      assert (rc == 0);
    }

  while (1)
    sleep (1);

  return 0;
}
