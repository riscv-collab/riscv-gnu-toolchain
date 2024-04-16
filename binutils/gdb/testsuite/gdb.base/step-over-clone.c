/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2024 Free Software Foundation, Inc.

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

#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>

static void
marker ()
{}

#define STACK_SIZE 0x1000

/* These are used to signal that the threads have started correctly.  The
   GLOBAL_THREAD_COUNT is set to the number of threads in main, then
   decremented (under a lock) in each new thread.  */
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
int global_thread_count = 0;

static int
clone_fn (void *unused)
{
  /* Signal that this thread has started correctly.  */
  if (pthread_mutex_lock (&global_lock) != 0)
    abort ();
  global_thread_count--;
  if (pthread_mutex_unlock (&global_lock) != 0)
    abort ();

  return 0;
}

int
main (void)
{
  int i, pid;
  unsigned char *stack[6];

  /* Due to bug gdb/19675 the cloned thread _might_ try to reenter main
     (this depends on where the displaced instruction is placed for
     execution).  However, if we do reenter main then lets ensure we fail
     hard rather then just silently executing the code below.  */
  static int started = 0;
  if (!started)
    started = 1;
  else
    abort ();

  for (i = 0; i < (sizeof (stack) / sizeof (stack[0])); i++)
    stack[i] = malloc (STACK_SIZE);

  global_thread_count = (sizeof (stack) / sizeof (stack[0]));

  for (i = 0; i < (sizeof (stack) / sizeof (stack[0])); i++)
    {
      pid = clone (clone_fn, stack[i] + STACK_SIZE, CLONE_FILES | CLONE_VM,
		   NULL);
    }

  for (i = 0; i < (sizeof (stack) / sizeof (stack[0])); i++)
    free (stack[i]);

  /* Set an alarm so we don't end up stuck waiting for threads that might
     never start correctly.  */
  alarm (120);

  /* Now wait for all the threads to start up.  */
  while (global_thread_count != 0)
    {
      /* Force memory barrier so GLOBAL_THREAD_COUNT will be refetched.  */
      asm volatile ("" ::: "memory");
      sleep (1);
    }

  /* Call marker, this is what GDB is waiting for.  */
  marker ();
}
