/* Test program exit in non-stop mode.
   Copyright 2009-2024 Free Software Foundation, Inc.

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
#include <stdio.h>

#define NTHREADS 4
void* thread_function (void*);

void *
thread_function (void *arg)
{
  int x = * (int *) arg;

  printf ("Thread <%d> executing\n", x);

  return NULL;
}

int
main ()
{
  pthread_t thread_id[NTHREADS];
  int args[NTHREADS];
  int i;

  for (i = 0; i < NTHREADS; ++i)
    {
      args[i] = i;
      pthread_create (&thread_id[i], NULL, thread_function, &args[i]);
    }

  for (i = 0; i < NTHREADS; ++i)
    {
      pthread_join (thread_id[i], NULL); 
    }

  return 0;
}
