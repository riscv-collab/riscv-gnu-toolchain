/* Pthreads test program.
   Copyright 1996-2024 Free Software Foundation, Inc.

   Written by Fred Fish of Cygnus Support
   Contributed by Cygnus Support

   This file is part of GDB.

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
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static int verbose = 0;

static void
common_routine (int arg)
{
  static int from_thread1;
  static int from_thread2;
  static int from_main;
  static int hits;
  static int full_coverage;

  if (verbose)
    printf ("common_routine (%d)\n", arg);
  hits++;
  switch (arg)
    {
    case 0:
      from_main++;
      break;
    case 1:
      from_thread1++;
      break;
    case 2:
      from_thread2++;
      break;
    }
  if (from_main && from_thread1 && from_thread2)
    full_coverage = 1;
}

static void *
thread1 (void *arg)
{
  int i;
  int z = 0;

  if (verbose)
    printf ("thread1 (%0lx) ; pid = %d\n", (long) arg, getpid ());
  for (i = 1; i <= 10000000; i++)
    {
      if (verbose)
	printf ("thread1 %ld\n", (long) pthread_self ());
      z += i;
      common_routine (1);
      sleep (1);
    }
  return (void *) 0;
}

static void *
thread2 (void * arg)
{
  int i;
  int k = 0;

  if (verbose)
    printf ("thread2 (%0lx) ; pid = %d\n", (long) arg, getpid ());
  for (i = 1; i <= 10000000; i++)
    {
      if (verbose)
	printf ("thread2 %ld\n", (long) pthread_self ());
      k += i;
      common_routine (2);
      sleep (1);
    }
  sleep (100);
  return (void *) 0;
}

void
foo (int a, int b, int c)
{
  int d, e, f;

  if (verbose)
    printf ("a=%d\n", a);
}

/* Similar to perror, but use ERR instead of errno.  */

static void
print_error (const char *ctx, int err)
{
  fprintf (stderr, "%s: %s (%d)\n", ctx, strerror (err), err);
}

int
main (int argc, char **argv)
{
  pthread_t tid1, tid2;
  int j;
  int t = 0;
  void (*xxx) ();
  pthread_attr_t attr;
  int res;

  if (verbose)
    printf ("pid = %d\n", getpid ());

  foo (1, 2, 3);

  res = pthread_attr_init (&attr);
  if (res != 0)
    {
      print_error ("pthread_attr_init 1", res);
      exit (1);
    }

#ifdef PTHREAD_SCOPE_SYSTEM
  res = pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);
  if (res != 0 && res != ENOTSUP)
    {
      print_error ("pthread_attr_setscope 1", res);
      exit (1);
    }
#endif

  res = pthread_create (&tid1, &attr, thread1, (void *) 0xfeedface);
  if (res != 0)
    {
      print_error ("pthread_create 1", res);
      exit (1);
    }
  if (verbose)
    printf ("Made thread %ld\n", (long) tid1);
  sleep (1);

  res = pthread_create (&tid2, NULL, thread2, (void *) 0xdeadbeef);
  if (res != 0)
    {
      print_error ("pthread_create 2", res);
      exit (1);
    }
  if (verbose)
    printf ("Made thread %ld\n", (long) tid2);

  sleep (1);

  for (j = 1; j <= 10000000; j++)
    {
      if (verbose)
	printf ("top %ld\n", (long) pthread_self ());
      common_routine (0);
      sleep (1);
      t += j;
    }
  
  exit (0);
}
