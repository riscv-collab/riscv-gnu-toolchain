/* Clean exit of the thread group leader should not break GDB.

   Copyright 2007-2024 Free Software Foundation, Inc.

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
#include <unistd.h>

static volatile pthread_t main_thread;

static void *
start (void *arg)
{
  int i;

  i = pthread_join (main_thread, NULL);
  assert (i == 0);

  sleep (10);  /* break-here */
  return arg;
}

int
main (void)
{
  pthread_t thread;
  int i;

  main_thread = pthread_self ();

  i = pthread_create (&thread, NULL, start, NULL);
  assert (i == 0);

  pthread_exit (NULL);
  /* NOTREACHED */
  return 0;
}
