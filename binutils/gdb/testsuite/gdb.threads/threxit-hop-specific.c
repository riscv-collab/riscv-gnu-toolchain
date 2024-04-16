/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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
#include <stdlib.h>

void *
thread_function (void *arg)
{
  /* We'll next over this, with scheduler-locking off.  */
  pthread_exit (NULL);
}

void
hop_me (void)
{
}

int
main (int argc, char **argv)
{
  pthread_t thread;

  pthread_create (&thread, NULL, thread_function, NULL);
  pthread_join (thread, NULL); /* wait for exit */

  /* The main thread should be able to hop over the breakpoint set
     here...  */
  hop_me (); /* set thread specific breakpoint here */

  /* ... and reach here.  */
  exit (0); /* set exit breakpoint here */
}
