/* Copyright 2023-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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
#include <stdlib.h>

#define NTHREAD 1

/* Barrier for synchronising threads.  */
static pthread_barrier_t barrier;

/* Wrapper around pthread_barrier_wait that aborts if the wait returns an
   error.  */
static void
barrier_wait (pthread_barrier_t *barrier)
{
  int res = pthread_barrier_wait (barrier);
  if (res != PTHREAD_BARRIER_SERIAL_THREAD && res != 0)
    abort ();
}

/* GDB can set this to 0 to force the 'spin' function to return.  */
volatile int do_spin = 1;

void
breakpt ()
{
  /* Nothing.  */
}

/* Spin for a reasonably long while (this lets GDB run some commands in
   async mode), but return early if/when do_spin is set to 0 (which is done
   by GDB).  */

void
spin ()
{
  int i;

  for (i = 0; i < 300 && do_spin; ++i)
    sleep (1);
}

void *
thread_worker (void *arg)
{
  barrier_wait (&barrier);
  return NULL;
}

int
main ()
{
  pthread_t thr[NTHREAD];
  int i;

  if (pthread_barrier_init (&barrier, NULL, NTHREAD + 1) != 0)
    abort ();

  for (i = 0; i < NTHREAD; ++i)
    pthread_create (&thr[i], NULL, thread_worker, NULL);

  breakpt ();	/* First breakpoint.  */

  barrier_wait (&barrier);

  for (i = 0; i < NTHREAD; ++i)
    pthread_join (thr[i], NULL);

  spin ();

  breakpt ();
}
