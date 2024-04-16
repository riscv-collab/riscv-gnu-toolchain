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

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

/* Used to individually advance each thread to the desired stopping point.  */
int ready;

sig_atomic_t sigusr1_received;
sig_atomic_t sigusr2_received;
sig_atomic_t sigabrt_received;

/* Number of threads currently running.  */
int thread_count;
pthread_mutex_t thread_count_mutex;
pthread_cond_t thread_count_condvar;

static void
incr_thread_count (void)
{
  pthread_mutex_lock (&thread_count_mutex);
  ++thread_count;
  pthread_cond_signal (&thread_count_condvar);
  pthread_mutex_unlock (&thread_count_mutex);
}

static void
sigusr1_handler (int sig)
{
  sigusr1_received = 1;
}

static void
sigusr2_handler (int sig)
{
  sigusr2_received = 1;
}

static void
sigabrt_handler (int sig)
{
  sigabrt_received = 1;
}

static void *
sigusr1_thread_function (void *unused)
{
  incr_thread_count ();
  while (!ready)
    usleep (100);
  pthread_kill (pthread_self (), SIGUSR1);
}

static void *
sigusr2_thread_function (void *unused)
{
  incr_thread_count ();
  while (!ready)
    usleep (100);
  /* pthread_kill (pthread_self (), SIGUSR2); - manually injected by gdb */
}

/* Wait until all threads are at a point where a backtrace will
   show the thread entry point function.  */

static void
wait_all_threads_running (int nr_threads)
{
  pthread_mutex_lock (&thread_count_mutex);

  while (1)
    {
      if (thread_count == nr_threads)
	{
	  pthread_mutex_unlock (&thread_count_mutex);
	  return;
	}
      pthread_cond_wait (&thread_count_condvar, &thread_count_mutex);
    }
}

static void
all_threads_running (void)
{
  while (!ready)
    usleep (100);
}

static void
all_threads_done (void)
{
}

int
main ()
{
  pthread_t sigusr1_thread, sigusr2_thread;

  /* Protect against running forever.  */
  alarm (60);

  signal (SIGUSR1, sigusr1_handler);
  signal (SIGUSR2, sigusr2_handler);
  signal (SIGABRT, sigabrt_handler);

  /* Don't let any thread advance past initialization.  */
  ready = 0;

  pthread_mutex_init (&thread_count_mutex, NULL);
  pthread_cond_init (&thread_count_condvar, NULL);

#define NR_THREADS 2
  pthread_create (&sigusr1_thread, NULL, sigusr1_thread_function, NULL);
  pthread_create (&sigusr2_thread, NULL, sigusr2_thread_function, NULL);
  wait_all_threads_running (NR_THREADS);
  all_threads_running ();

  pthread_kill (pthread_self (), SIGABRT);

  pthread_join (sigusr1_thread, NULL);
  pthread_join (sigusr2_thread, NULL);
  all_threads_done ();

  return 0;
}
