/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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

#define NTHREADS 3

static void *
test (void *arg)
{
  /* Let's hope this is long enough for GDB to enable tracing and check that
     everything is working as expected.  */
  int unslept = 10;
  while (unslept > 0)
    unslept = sleep (unslept);

  return arg;
}

int
main (void)
{
  pthread_t th[NTHREADS];
  int i;

  for (i = 0; i < NTHREADS; ++i)
    pthread_create (&th[i], NULL, test, NULL);

  test (NULL); /* bp.1 */

  for (i = 0; i < NTHREADS; ++i)
    pthread_join (th[i], NULL);

  return 0;
}
