/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

/* A place to record the thread.  */
pthread_t the_thread;

/* The worker thread just spins forever.  */
void*
thread_worker (void *payload)
{
  while (1)
    {
      sleep (1);
    }

  return NULL;
}

/* Create a worker thread.  */
int
spawn_thread ()
{
  if (pthread_create (&the_thread, NULL, thread_worker, NULL))
    {
      fprintf (stderr, "Unable to create thread.\n");
      return 0;
    }
  return 1;
}

/* A place for GDB to place a breakpoint.   */
void __attribute__((used))
breakpt ()
{
  /* Nothing.  */
}

/* Create a worker thread that just spins forever, then enter a loop
   periodically calling the BREAKPT function.  */
int
main()
{
  /* Ensure we stop if GDB crashes and DejaGNU fails to kill us.  */
  alarm (10);

  spawn_thread ();

  while (1)
    {
      sleep (1);

      breakpt ();
    }

  return 0;
}
