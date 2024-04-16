/* This testcase is part of GDB, the GNU debugger.

   Copyright 2020-2024 Free Software Foundation, Inc.

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
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

/* The number of threads to create.  */
int thread_count = 3;

/* Counter accessed from threads to ensure that all threads have been
   started.  Is initialised to THREAD_COUNT and each thread decrements it
   upon startup.  */
volatile int counter;

/* Lock guarding COUNTER. */
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Is initialised with our pid, GDB will read this.  */
pid_t global_pid;

/* Just somewhere to put a breakpoint.  */
static void
breakpt ()
{
  /* Nothing.  */
}

/* Thread safe decrement of the COUNTER global.  */
static void
decrement_counter ()
{
  if (pthread_mutex_lock (&counter_mutex) != 0)
    abort ();
  --counter;
  if (pthread_mutex_unlock (&counter_mutex) != 0)
    abort ();
}

/* Thread safe read of the COUNTER global.  */
static int
read_counter ()
{
  int val;

  if (pthread_mutex_lock (&counter_mutex) != 0)
    abort ();
  val = counter;
  if (pthread_mutex_unlock (&counter_mutex) != 0)
    abort ();

  return val;
}

#if defined DO_EXIT_TEST

/* Thread entry point.  ARG is a pointer to a single integer, the ID for
   this thread numbered 1 to THREAD_COUNT (a global).  */
static void *
thread_worker_exiting (void *arg)
{
  int id;

  id = *((int *) arg);

  decrement_counter ();

  if (id != thread_count)
    {
      int i;

      /* All threads except the last one will wait here while the test is
	 carried out.  Don't wait forever though, just in case the test
	 goes wrong.  */
      for (i = 0; i < 60; ++i)
	sleep (1);
    }
  else
    {
      /* The last thread waits here until all other threads have been
	 created.  */
      while (read_counter () > 0)
	sleep (1);

      /* Hit the breakpoint so GDB can stop.  */
      breakpt ();

      /* And exit all threads.  */
      exit (0);
    }

  return NULL;
}

#define thread_worker thread_worker_exiting

#elif defined DO_SIGNAL_TEST

/* Thread entry point.  ARG is a pointer to a single integer, the ID for
   this thread numbered 1 to THREAD_COUNT (a global).  */
static void *
thread_worker_signalling (void *arg)
{
  int i, id;

  id = *((int *) arg);

  decrement_counter ();

  if (id == thread_count)
    {
      /* The last thread waits here until all other threads have been
	 created.  */
      while (read_counter () > 0)
	sleep (1);

      /* Hit the breakpoint so GDB can stop.  */
      breakpt ();
    }

  /* All threads wait here while the testsuite sends us a signal.  Don't
     block forever though, just in case the test goes wrong.  */
  for (i = 0; i < 60; ++i)
    sleep (1);

  return NULL;
}

#define thread_worker thread_worker_signalling

#else

#error "Compile with DO_EXIT_TEST or DO_SIGNAL_TEST defined"

#endif

struct thread_info
{
  pthread_t thread;
  int id;
};

int
main ()
{
  int i, max = thread_count;

  /* Put the pid somewhere easy for GDB to read.  */
  global_pid = getpid ();

  /* Space to hold all of the thread_info objects.  */
  struct thread_info *info = malloc (sizeof (struct thread_info) * max);
  if (info == NULL)
    abort ();

  /* Initialise the counter.  Don't do this under lock as we only have the
     main thread at this point.  */
  counter = thread_count;

  /* Create all of the threads.  */
  for (i = 0; i < max; ++i)
    {
      struct thread_info *thr = &info[i];
      thr->id = i + 1;
      if (pthread_create (&thr->thread, NULL, thread_worker, &thr->id) != 0)
	abort ();
    }

  /* Gather in all of the threads.  This never completes, as the
     final thread created will exit the process, and all of the other
     threads block forever.  Still, it gives the main thread something to
     do.  */
  for (i = 0; i < max; ++i)
    {
      struct thread_info *thr = &info[i];
      if (pthread_join (thr->thread, NULL) != 0)
	abort ();
    }

  free (info);

  /* Return non-zero.  We should never get here, but if we do make sure we
     indicate something has gone wrong.  */
  return 1;
}
