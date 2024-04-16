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

#include <pthread.h>
#include <assert.h>

struct type_1
{
  int m;
};

struct type_2
{
  int n;
};

__attribute__((used)) static int
called_from_pretty_printer (void)
{
  return 23;
}

static int
baz (struct type_1 z1, struct type_2 z2)
{
  return z1.m + z2.n;
}

static int
bar (struct type_1 y1, struct type_2 y2)
{
  return baz (y1, y2);
}

static int
foo (struct type_1 x1, struct type_2 x2)
{
  return bar (x1, x2);
}

static void *
thread_func (void *p)
{
  struct type_1 t1;
  struct type_2 t2;
  t1.m = 11;
  t2.n = 11;
  foo (t1, t2);

  return NULL;
}

int
main (void)
{
  pthread_t thread;
  int res;

  res = pthread_create (&thread, NULL, thread_func, NULL);
  assert (res == 0);

  res = pthread_join (thread, NULL);
  assert (res == 0);

  return 0;
}
