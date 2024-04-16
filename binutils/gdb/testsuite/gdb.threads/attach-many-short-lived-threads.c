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

#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

pthread_t main_thread;
pthread_attr_t detached_attr;
pthread_attr_t joinable_attr;

/* Number of threads we'll create of each variant
   (joinable/detached).  */
int n_threads = 50;

/* Mutex used to hold creating detached threads.  */
pthread_mutex_t dthrds_create_mutex;

/* Wrapper for pthread_create.   */

void
create_thread (pthread_attr_t *attr,
	       void *(*start_routine) (void *), void *arg)
{
  pthread_t child;
  int rc;

  while ((rc = pthread_create (&child, attr, start_routine, arg)) != 0)
    {
      fprintf (stderr, "unexpected error from pthread_create: %s (%d)\n",
	       strerror (rc), rc);
      sleep (1);
    }
}

void
break_fn (void)
{
}

/* Data passed to joinable threads on creation.  This is allocated on
   the heap and ownership transferred from parent to child.  (We do
   this because it's not portable to cast pthread_t to pointer.)  */

struct thread_arg
{
  pthread_t parent;
};

/* Entry point for joinable threads.  These threads first join their
   parent before spawning a new child (and exiting).  The parent's tid
   is passed as pthread_create argument, encapsulated in a struct
   thread_arg object.  */

void *
joinable_fn (void *arg)
{
  struct thread_arg *p = arg;

  pthread_setname_np (pthread_self (), "joinable");

  if (p->parent != main_thread)
    assert (pthread_join (p->parent, NULL) == 0);

  p->parent = pthread_self ();

  create_thread (&joinable_attr, joinable_fn, p);

  break_fn ();

  return NULL;
}

/* Entry point for detached threads.  */

void *
detached_fn (void *arg)
{
  pthread_setname_np (pthread_self (), "detached");

  /* This should throttle threads a bit in case we manage to spawn
     threads faster than they exit.  */
  pthread_mutex_lock (&dthrds_create_mutex);

  create_thread (&detached_attr, detached_fn, NULL);

  /* Note this is called before the mutex is unlocked otherwise in
     non-stop mode, when the breakpoint is hit we'd keep spawning more
     threads forever while the old threads stay alive (stopped in the
     breakpoint).  */
  break_fn ();

  pthread_mutex_unlock (&dthrds_create_mutex);

  return NULL;
}

/* Allow for as much timeout as DejaGnu wants, plus a bit of
   slack.  */
#define SECONDS (TIMEOUT + 20)

/* We'll exit after this many seconds.  */
unsigned int seconds_left = SECONDS;

/* GDB sets this whenever it's about to start a new detach/attach
   sequence.  We react by resetting the seconds left counter.  */
volatile int again = 0;

int
main (int argc, char *argv[])
{
  int i;

  if (argc > 1)
    n_threads = atoi (argv[1]);

  pthread_mutex_init (&dthrds_create_mutex, NULL);

  pthread_attr_init (&detached_attr);
  pthread_attr_setdetachstate (&detached_attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_init (&joinable_attr);
  pthread_attr_setdetachstate (&joinable_attr, PTHREAD_CREATE_JOINABLE);

  main_thread = pthread_self ();

  /* Spawn the initial set of test threads.  Some threads are
     joinable, others are detached.  This exercises different code
     paths in the runtime.  */
  for (i = 0; i < n_threads; ++i)
    {
      struct thread_arg *p;

      p = malloc (sizeof *p);
      p->parent = main_thread;
      create_thread (&joinable_attr, joinable_fn, p);

      create_thread (&detached_attr, detached_fn, NULL);
    }

  /* Exit after a while if GDB is gone/crashes.  But wait long enough
     for one attach/detach sequence done by the .exp file.  */
  while (--seconds_left > 0)
    {
      sleep (1);

      if (again)
	{
	  /* GDB should be reattaching soon.  Restart the timer.  */
	  again = 0;
	  seconds_left = SECONDS;
	}
    }

  printf ("timeout, exiting\n");
  return 0;
}
