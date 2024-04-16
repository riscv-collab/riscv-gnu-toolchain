/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023-2024 Free Software Foundation, Inc.

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
#include <unistd.h>

/* This is set from GDB to allow the main thread to exit.  */

volatile int dont_exit_just_yet = 1;

/* Somewhere to place a breakpoint.  */

void
breakpt ()
{
  /* Just spin.  When the test is started under GDB we never enter the spin
     loop, but when we attach, the worker thread will be spinning here.  */
  while (dont_exit_just_yet)
    sleep (1);
}

/* Thread function, doesn't do anything but hit a breakpoint.  */

void *
thread_worker (void *arg)
{
  breakpt ();
  return NULL;
}

int
main ()
{
  pthread_t thr;

  alarm (300);

  /* Create a thread.  */
  pthread_create (&thr, NULL, thread_worker, NULL);
  pthread_detach (thr);

  /* Spin until GDB releases us.  */
  while (dont_exit_just_yet)
    sleep (1);

  _exit (0);
}
