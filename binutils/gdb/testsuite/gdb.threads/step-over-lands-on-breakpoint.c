/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

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

pthread_barrier_t barrier;
pthread_t child_thread;

volatile unsigned int counter = 1;

void *
child_function (void *arg)
{
  pthread_barrier_wait (&barrier);

  while (counter > 0)
    {
      counter++;

      asm ("	nop"); /* set breakpoint child here */
      asm ("	nop"); /* set breakpoint after step-over here */
      usleep (1);
    }

  pthread_exit (NULL);
}

int
main ()
{
  int res;
  long i;

  alarm (300);

  pthread_barrier_init (&barrier, NULL, 2);

  res = pthread_create (&child_thread, NULL, child_function, NULL);
  pthread_barrier_wait (&barrier);

  /* Use an infinite loop with no function calls so that "step" over
     this line never finishes before the breakpoint in the other
     thread triggers.  That can happen if the step-over of thread 2 is
     done with displaced stepping on a target that is always in
     non-stop mode, as in that case GDB runs both threads
     simultaneously.  */
  while (1); /* set wait-thread breakpoint here */

  pthread_join (child_thread, NULL);

  exit (EXIT_SUCCESS);
}
