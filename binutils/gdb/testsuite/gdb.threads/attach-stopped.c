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

/* This program is intended to be started outside of gdb, then
   manually stopped via a signal.  */

#include <stddef.h>
#include <unistd.h>
#ifdef USE_THREADS
#include <pthread.h>
#endif

static void *func (void *arg)
{
  sleep (10000);  /* Ridiculous time, but we will eventually kill it.  */
  sleep (10000);  /* Second sleep.  */
  return NULL;
}

int main ()
{

#ifndef USE_THREADS

  func (NULL);

#else

  pthread_t th;
  pthread_create (&th, NULL, func, NULL);
  pthread_join (th, NULL);

#endif

  return 0;
}
