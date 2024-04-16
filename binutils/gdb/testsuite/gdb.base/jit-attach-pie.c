/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2024 Free Software Foundation, Inc.

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
#include <stdint.h>
#include <pthread.h>

#include "jit-protocol.h"

static void *
thread_proc (void *arg)
{
  sleep (60);
  return arg;
}

int
main (void)
{
  pthread_t thread;

  pthread_create (&thread, NULL, thread_proc, 0);
  pthread_join (thread, NULL);
  return 0;
}
