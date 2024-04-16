/* Copyright 2011-2024 Free Software Foundation, Inc.

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
#include <stdlib.h>
#include <assert.h>

pthread_t thread0;
pthread_t thread1;

static void *
start (void *arg)
{
  assert (pthread_self () == thread1);

  abort ();
}

int
main (void)
{
  int i;

  thread0 = pthread_self ();

  i = pthread_create (&thread1, NULL, start, NULL);
  assert (i == 0);

  i = pthread_join (thread1, NULL);
  assert (i == 0);

  return 0;
}
