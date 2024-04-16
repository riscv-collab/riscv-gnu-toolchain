/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

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

static int global;

static void
break_function (void)
{
  global = 42; /* set break here */
}

static void *
thread_function (void *arg)
{
  break_function ();

  return arg;
}

int
main (void)
{
  pthread_t th;

  pthread_create (&th, NULL, thread_function, NULL);

  break_function ();

  pthread_join (th, NULL);

  return 0;
}
