/* This testcase is part of GDB, the GNU debugger.

   Copyright 2020-2024 Free Software Foundation, Inc.

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

pid_t pid;

static pthread_barrier_t threads_started_barrier;

static void
all_started (void)
{
}

static void *
fun (void *dummy)
{
  int i;

  pthread_barrier_wait (&threads_started_barrier);

  for (i = 0; i < 180; i++)
    sleep (1);

  pthread_exit (NULL);
}

int
main (void)
{
  int i;
  pthread_t thread;

  pid = getpid ();

  pthread_barrier_init (&threads_started_barrier, NULL, 2);

  pthread_create (&thread, NULL, fun, NULL);

  pthread_barrier_wait (&threads_started_barrier);

  all_started ();

  for (i = 0; i < 180; i++)
    sleep (1);

  exit (EXIT_SUCCESS);
}
