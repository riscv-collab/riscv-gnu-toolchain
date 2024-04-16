/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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
#include <assert.h>

static volatile int a, b, c;

static void
marker_exit (void)
{
  a = 1;
}

static void *
start (void *arg)
{
  b = 2;
  c = 3;

  return NULL;
}

int
main (void)
{
  pthread_t thread;
  int i;

  i = pthread_create (&thread, NULL, start, NULL);
  assert (i == 0);
  i = pthread_join (thread, NULL);
  assert (i == 0);

  marker_exit ();
  return 0;
}
