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
#include <signal.h>
#include <unistd.h>

static pthread_t main_thread;

static void *
start (void *arg)
{
  /* A signal whose default action is ignore.  */
  pthread_kill (main_thread, SIGCHLD);

  while (1)
    sleep (1); /* set break here */
  return NULL;
}

int
main (void)
{
  unsigned int counter = 1;
  pthread_t thread;

  main_thread = pthread_self ();
  pthread_create (&thread, NULL, start, NULL);

  while (counter != 0)
    counter++; /* set break 2 here */

  return 0;
}
