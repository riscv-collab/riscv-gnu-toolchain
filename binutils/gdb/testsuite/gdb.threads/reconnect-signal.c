/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

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
#include <signal.h>
#include <unistd.h>

static pthread_t thread_2;
sig_atomic_t unlocked;

/* The test has three threads, and it's always thread 2 that gets the
   signal, to avoid spurious passes in case the remote side happens to
   always pick the first or the last thread in the list as the
   current/status thread on reconnection.  */

static void *
start2 (void *arg)
{
  unsigned int count;

  pthread_kill (thread_2, SIGUSR1);

  for (count = 1; !unlocked && count != 0; count++)
    usleep (1);
  return NULL;
}

static void *
start (void *arg)
{
  pthread_t thread;

  pthread_create (&thread, NULL, start2, NULL);
  pthread_join (thread, NULL);
  return NULL;
}

void
handle (int sig)
{
  unlocked = 1;
}

int
main ()
{
  signal (SIGUSR1, handle);

  pthread_create (&thread_2, NULL, start, NULL);
  pthread_join (thread_2, NULL);

  return 0;
}
