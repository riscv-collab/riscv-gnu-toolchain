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

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

/* Set this to non-zero from GDB to start a third worker thread.  */
volatile int start_third_thread = 0;

void *
thread_worker_2 (void *arg)
{
  int i;

  printf ("Hello from the third thread.\n");
  fflush (stdout);

  for (i = 0; i < 300; ++i)
    sleep (1);

  return NULL;
}

void *
thread_worker_1 (void *arg)
{
  int i;
  pthread_t thr;
  void *val;

  if (start_third_thread)
    pthread_create (&thr, NULL, thread_worker_2, NULL);

  printf ("Hello from the first thread.\n");
  fflush (stdout);

  for (i = 0; i < 300; ++i)
    sleep (1);

  if (start_third_thread)
    pthread_join (thr, &val);

  return NULL;
}

void *
thread_idle_loop (void *arg)
{
  int i;

  for (i = 0; i < 300; ++i)
    sleep (1);

  return NULL;
}

int
main ()
{
  pthread_t thr, thr_idle;
  void *val;

  if (getenv ("MAKE_EXTRA_THREAD") != NULL)
    pthread_create (&thr_idle, NULL, thread_idle_loop, NULL);

  pthread_create (&thr, NULL, thread_worker_1, NULL);
  pthread_join (thr, &val);

  if (getenv ("MAKE_EXTRA_THREAD") != NULL)
    pthread_join (thr_idle, &val);

  return 0;
}
