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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

pthread_barrier_t barrier;
sig_atomic_t got_sigusr1;
sig_atomic_t got_sigusr2;

void
handler_sigusr1 (int sig)
{
  got_sigusr1 = 1;
}

void
handler_sigusr2 (int sig)
{
  got_sigusr2 = 1;
}

void *
thread_function (void *arg)
{
  volatile unsigned int count = 1;

  pthread_barrier_wait (&barrier);

  while (count++ != 0)
    {
      if (got_sigusr1 && got_sigusr2)
	break;
      usleep (1);
    }
}

void
all_threads_started (void)
{
}

void
all_threads_signalled (void)
{
}

void
end (void)
{
}

int
main (void)
{
  pthread_t child_thread[2];
  int i;

  signal (SIGUSR1, handler_sigusr1);
  signal (SIGUSR2, handler_sigusr2);

  for (i = 0; i < 2; i++)
    {
      pthread_barrier_init (&barrier, NULL, 2);
      pthread_create (&child_thread[i], NULL, thread_function, NULL);
      pthread_barrier_wait (&barrier);
      pthread_barrier_destroy (&barrier);
    }

  all_threads_started ();

  pthread_kill (child_thread[0], SIGUSR1);
  pthread_kill (child_thread[1], SIGUSR2);

  all_threads_signalled ();

  for (i = 0; i < 2; i++)
    pthread_join (child_thread[i], NULL);

  end ();
  return 0;
}
