/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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
#include <pthread.h>
#include <assert.h>

/* NOP instruction: must have the same size as the breakpoint
   instruction for the test to be effective.  */

#if defined (__s390__) || defined (__s390x__)
# define NOP asm ("nopr 0")
#else
# define NOP asm ("nop")
#endif

void *
thread_function (void *arg)
{
  NOP; /* set breakpoint here */
  while (1);
}

int
main (void)
{
  int res;
  pthread_t thread;

  alarm (300);

  res = pthread_create (&thread,
		       NULL,
		       thread_function,
		       NULL);
  assert (res == 0);

  pthread_join (thread, NULL);
  return 0;
}
