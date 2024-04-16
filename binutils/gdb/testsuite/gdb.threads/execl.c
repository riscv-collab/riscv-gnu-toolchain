/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

/* Test handling thread control across an execl.  */

/* The original image loads a thread library and has several threads,
   while the new image does not load a thread library.  */

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static pthread_barrier_t threads_started_barrier;

void *
thread_function (void *arg)
{
  pthread_barrier_wait (&threads_started_barrier);

  while (1)
    sleep (100);
  return NULL;
}

int
main (int argc, char* argv[])
{
  pthread_t thread1;
  pthread_t thread2;
  char *new_image;

  pthread_barrier_init (&threads_started_barrier, NULL, 3);

  pthread_create (&thread1, NULL, thread_function, NULL);
  pthread_create (&thread2, NULL, thread_function, NULL);

  pthread_barrier_wait (&threads_started_barrier);

  new_image = malloc (strlen (argv[0]) + 2);
  strcpy (new_image, argv[0]);
  strcat (new_image, "1");

  if (execl (new_image, new_image, NULL) == -1) /* set breakpoint here */
    return 1;

  return 0;
}
