/* This testcase is part of GDB, the GNU debugger.

   Copyright 2018-2024 Free Software Foundation, Inc.

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
#include <sys/types.h>
#include <unistd.h>

/* This is set to 1 by the debugger post attach to continue to the
 watchpoint trigger.  */
volatile int should_continue = 0;
/* The variable to place a watchpoint on.  */
volatile int watched_variable = 0;

int
main (void)
{
  unsigned int counter = 1;
  int mypid = getpid ();

  /* Wait for the debugger to attach, but not indefinitely so this
     test program is not left hanging around.  */
  for (counter = 0; !should_continue && counter < 100; counter++)
    sleep (1);			/* pidacquired */

  /* Trigger a watchpoint.  */
  watched_variable = 4;
  printf ("My variable is %d\n", watched_variable);
  return 0;
}
