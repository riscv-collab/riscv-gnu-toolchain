/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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

#include <unistd.h>
#include <pthread.h>

pthread_t child_thread[2];

void *
thread_function2 (void *arg)
{
  while (1)
    sleep (1);

  return NULL;
}

void *
thread_function1 (void *arg)
{
  pthread_create (&child_thread[1], NULL, thread_function2, NULL);

  while (1)
    sleep (1);

  return NULL;
}

int
main (void)
{
  int i;

  alarm (300);

  pthread_create (&child_thread[0], NULL, thread_function1, NULL);

  for (i = 0; i < 2; i++)
    pthread_join (child_thread[i], NULL);

  return 0;
}
