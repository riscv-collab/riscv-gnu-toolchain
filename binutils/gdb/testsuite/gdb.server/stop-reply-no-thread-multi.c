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

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

volatile int worker_blocked = 1;
volatile int main_blocked = 1;

void
unlock_worker (void)
{
  worker_blocked = 0;
}

void
unlock_main (void)
{
  main_blocked = 0;
}

void
breakpt (void)
{
  /* Nothing.  */
}

static void *
worker (void *data)
{
  unlock_main ();

  while (worker_blocked)
    ;

  breakpt ();

  return NULL;
}

int
main (void)
{
  pthread_t thr;
  void *retval;

  /* Ensure the test doesn't run forever.  */
  alarm (99);

  if (pthread_create (&thr, NULL, worker, NULL) != 0)
    abort ();

  while (main_blocked)
    ;

  unlock_worker ();

  if (pthread_join (thr, &retval) != 0)
    abort ();

  return 0;
}
