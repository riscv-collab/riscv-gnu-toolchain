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

#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

/* How many threads fit in the target's thread number space.  */
long tid_max = -1;

/* Number of threads spawned.  */
unsigned long thread_counter;

/* How long it takes to spawn as many threads as fits in the thread
   number space.  On systems where thread IDs are just monotonically
   incremented, this is enough for the tid numbers to wrap around.  On
   targets that randomize thread IDs, this is enough time to give each
   number in the thread number space some chance of reuse.  It'll be
   capped to a lower value if we can't compute it.  REUSE_TIME_CAP
   is the max value, and the default value if ever the program
   has problem to compute it.  */
#define REUSE_TIME_CAP 60
unsigned int reuse_time = REUSE_TIME_CAP;

void *
do_nothing_thread_func (void *arg)
{
  usleep (1);
  return NULL;
}

static void
check_rc (int rc, const char *what)
{
  if (rc != 0)
    {
      fprintf (stderr, "unexpected error from %s: %s (%d)\n",
	       what, strerror (rc), rc);
      assert (0);
    }
}

void *
spawner_thread_func (void *arg)
{
  while (1)
    {
      pthread_t child;
      int rc;

      thread_counter++;

      rc = pthread_create (&child, NULL, do_nothing_thread_func, NULL);
      check_rc (rc, "pthread_create");

      rc = pthread_join (child, NULL);
      check_rc (rc, "pthread_join");
    }

  return NULL;
}

/* Called after the program is done counting number of spawned threads
   for a period, to compute REUSE_TIME.  */

void
after_count (void)
{
}

/* Called after enough time has passed for TID reuse to occur.  */

void
after_reuse_time (void)
{
}

#ifdef __linux__

/* Get the running system's configured pid_max.  */

static int
linux_proc_get_pid_max (void)
{
  static const char filename[]  ="/proc/sys/kernel/pid_max";
  FILE *file;
  char buf[100];
  int retval = -1;

  file = fopen (filename, "r");
  if (file == NULL)
    {
      fprintf (stderr, "unable to open %s\n", filename);
      return -1;
    }

  if (fgets (buf, sizeof (buf), file) != NULL)
    retval = strtol (buf, NULL, 10);

  fclose (file);
  return retval;
}

#endif

int
main (int argc, char *argv[])
{
  pthread_t child;
  int rc;
  unsigned int reuse_time_raw = 0;

  rc = pthread_create (&child, NULL, spawner_thread_func, NULL);
  check_rc (rc, "pthread_create spawner_thread");

#define COUNT_TIME 2
  sleep (COUNT_TIME);

#ifdef __linux__
  tid_max = linux_proc_get_pid_max ();
#endif
  /* If we don't know how many threads it would take to use the whole
     number space on this system, just run the test for a bit.  */
  if (tid_max > 0)
    {
      reuse_time_raw = tid_max / ((float) thread_counter / COUNT_TIME) + 0.5;

      /* Give it a bit more, just in case.  */
      reuse_time = reuse_time_raw + 3;
    }

  /* 4 seconds were sufficient on the machine this was first observed,
     an Intel i7-2620M @ 2.70GHz running Linux 3.18.7, with
     pid_max=32768.  Going forward, as machines get faster, this will
     need less time, unless pid_max is set to a very high number.  To
     avoid unreasonably long test time, cap to an upper bound.  */
  if (reuse_time > REUSE_TIME_CAP)
    reuse_time = REUSE_TIME_CAP;
  printf ("thread_counter=%lu, tid_max = %ld, reuse_time_raw=%u, reuse_time=%u\n",
	  thread_counter, tid_max, reuse_time_raw, reuse_time);
  after_count ();

  sleep (reuse_time);

  after_reuse_time ();
  return 0;
}
