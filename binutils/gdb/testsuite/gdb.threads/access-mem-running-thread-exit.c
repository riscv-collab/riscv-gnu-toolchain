/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

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

#define THREADS 20

static volatile unsigned int global_var = 123;

/* Wrapper around pthread_create.   */

static void
create_thread (pthread_t *child,
	       void *(*start_routine) (void *), void *arg)
{
  int rc;

  while ((rc = pthread_create (child, NULL, start_routine, arg)) != 0)
    {
      fprintf (stderr, "unexpected error from pthread_create: %s (%d)\n",
	       strerror (rc), rc);
      sleep (1);
    }
}

/* Data passed to threads on creation.  This is allocated on the heap
   and ownership transferred from parent to child.  */

struct thread_arg
{
  /* The thread's parent.  */
  pthread_t parent;

  /* Whether to call pthread_join on the parent.  */
  int join_parent;
};

/* Entry point for threads.  */

static void *
thread_fn (void *arg)
{
  struct thread_arg *p = arg;

  /* Passing no argument makes the thread exit immediately.  */
  if (p == NULL)
    return NULL;

  if (p->join_parent)
    assert (pthread_join (p->parent, NULL) == 0);

  /* Spawn a number of threads that exit immediately, and then join
     them.  The idea is to maximize the time window when we mostly
     have threads exiting.  */
  {
    pthread_t child[THREADS];
    int i;

    /* Passing no argument makes the thread exit immediately.  */
    for (i = 0; i < THREADS; i++)
      create_thread (&child[i], thread_fn, NULL);

    for (i = 0; i < THREADS; i++)
      pthread_join (child[i], NULL);
  }

  /* Spawn a new thread that joins us, and exit.  The idea here is to
     not have any thread that stays around forever.  */
  {
    pthread_t child;

    p->parent = pthread_self ();
    p->join_parent = 1;
    create_thread (&child, thread_fn, p);
  }

  return NULL;
}

int
main (void)
{
  int i;

  for (i = 0; i < 4; i++)
    {
      struct thread_arg *p;
      pthread_t child;

      p = malloc (sizeof *p);
      p->parent = pthread_self ();
      /* Only join the parent once.  */
      if (i == 0)
	p->join_parent = 1;
      else
	p->join_parent = 0;
      create_thread (&child, thread_fn, p);
    }

  /* Exit the leader to make sure that we can access memory with the
     leader gone.  */
  pthread_exit (NULL);
}
