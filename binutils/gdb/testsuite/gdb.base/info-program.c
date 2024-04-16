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

#ifdef USE_THREADS

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

pthread_barrier_t barrier;
pthread_t child_thread;

void *
child_function (void *arg)
{
  pthread_barrier_wait (&barrier);

  while (1)
    usleep (100);

  pthread_exit (NULL);
}

#endif /* USE_THREADS */

static void
done (void)
{
}

int
main (void)
{
#ifdef USE_THREADS
  int res;

  alarm (300);

  pthread_barrier_init (&barrier, NULL, 2);

  res = pthread_create (&child_thread, NULL, child_function, NULL);
  pthread_barrier_wait (&barrier);
#endif /* USE_THREADS */

  done ();

#ifdef USE_THREADS
  pthread_join (child_thread, NULL);
#endif /* USE_THREADS */

  return 0;
}
