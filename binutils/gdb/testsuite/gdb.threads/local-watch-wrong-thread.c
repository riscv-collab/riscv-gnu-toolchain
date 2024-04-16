/* This testcase is part of GDB, the GNU debugger.

   Copyright 2002-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

unsigned int args[2];
int trigger = 0;

void *
thread_function0 (void *arg)
{
  int my_number =  (long) arg;
  volatile int *myp = (volatile int *) &args[my_number];

  while (*myp > 0)
    {
      (*myp) ++;
      usleep (1);  /* Loop increment 1.  */
    }

  return NULL;
}

void *
thread_function0_1 (void *arg)
{
  void *ret = thread_function0 (arg);

  return ret; /* set breakpoint here */
}

void *
thread_function1 (void *arg)
{
  int my_number =  (long) arg;

  volatile int *myp = (volatile int *) &args[my_number];

  while (*myp > 0)
    {
      (*myp) ++;
      usleep (1);  /* Loop increment 2.  */
    }

  return NULL;
}

int
main ()
{
  int res;
  pthread_t threads[2];
  void *thread_result;
  long i = 0;

  args[i] = 1; /* Init value.  */
  res = pthread_create (&threads[i], NULL,
			thread_function0_1,
			(void *) i);

  i++;
  args[i] = 1; /* Init value.  */
  res = pthread_create(&threads[i], NULL,
		       thread_function1,
		       (void *) i);

  pthread_join (threads[0], &thread_result);
  pthread_join (threads[1], &thread_result);
  exit(EXIT_SUCCESS);
}
