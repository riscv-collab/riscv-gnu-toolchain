/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023-2024 Free Software Foundation, Inc.

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

#include <assert.h>
#include <pthread.h>

/* Test can create at most this number of extra threads.  */
#define MAX_THREADS 3

/* For convenience.  */
#define FALSE 0
#define TRUE (!FALSE)

/* Controls a thread created by this test.  */
struct thread_descriptor
{
  /* The pthread handle.  Not valid unless STARTED is true.  */
  pthread_t thr;

  /* This field is set to TRUE when a thread has been created, otherwise,
     is false.  */
  int started;

  /* A condition variable and mutex, used for synchronising between the
     worker thread and the main thread.  */
  pthread_cond_t cond;
  pthread_mutex_t mutex;
};

/* Keep track of worker threads.  */
struct thread_descriptor threads[MAX_THREADS];

/* Worker thread function.  Doesn't do much.  Synchronise with the main
   thread, mark the thread as started, and then block waiting for the main
   thread.  Once the main thread wakes us, this thread exits.

   ARG is a thread_descriptor shared with the main thread.  */

void *
thread_function (void *arg)
{
  int res;
  struct thread_descriptor *thread = (struct thread_descriptor *) arg;

  /* Acquire the thread's lock.  Initially the main thread holds this lock,
     but releases it when the main thread enters a pthread_cond_wait.  */
  res = pthread_mutex_lock (&thread->mutex);
  assert (res == 0);

  /* Mark the thread as started.  */
  thread->started = TRUE;

  /* Signal the main thread to tell it we are started.  The main thread
     will still be blocked though as we hold the thread's lock.  */
  res = pthread_cond_signal (&thread->cond);
  assert (res == 0);

  /* Now wait until the main thread tells us to exit.  By entering this
     pthread_cond_wait we release the lock, which allows the main thread to
     resume.  */
  res = pthread_cond_wait (&thread->cond, &thread->mutex);
  assert (res == 0);

  /* The main thread woke us up.  We reacquired the thread lock as we left
     the pthread_cond_wait, so release the lock now.  */
  res = pthread_mutex_unlock (&thread->mutex);
  assert (res == 0);

  return NULL;
}

/* Start a new thread within the global THREADS array.  Return true if a
   new thread was started, otherwise return false.  */

int
start_thread ()
{
  int idx, res;

  for (idx = 0; idx < MAX_THREADS; ++idx)
    if (!threads[idx].started)
      break;

  if (idx == MAX_THREADS)
    return FALSE;

  /* Acquire the thread lock before starting the new thread.  */
  res = pthread_mutex_lock (&threads[idx].mutex);
  assert (res == 0);

  /* Start the new thread.  */
  res = pthread_create (&threads[idx].thr, NULL,
			thread_function, &threads[idx]);
  assert (res == 0);

  /* Unlock and wait.  The thread signals us once it is ready.  */
  res = pthread_cond_wait (&threads[idx].cond, &threads[idx].mutex);
  assert (res == 0);

  /* The worker thread is now blocked in a pthread_cond_wait and we
     reacquired the lock as we left our own pthread_cond_wait above.  */
  res = pthread_mutex_unlock (&threads[idx].mutex);
  assert (res == 0);

  return TRUE;
}

/* Stop a thread from within the global THREADS array.  Return true if a
   thread was stopped, otherwise return false.  */
int
stop_thread ()
{
  /* Look for a thread that is started.  */
  for (int idx = 0; idx < MAX_THREADS; ++idx)
    if (threads[idx].started)
      {
	int res;

	/* Grab the thread lock.  */
	res = pthread_mutex_lock (&threads[idx].mutex);
	assert (res == 0);

	/* Signal the worker thread, this wakes it up, but it can't exit
	   until it acquires the thread lock, which we currently hold.  */
	res = pthread_cond_signal (&threads[idx].cond);
	assert (res == 0);

	/* Release the thread lock, this allows the worker thread to exit.  */
	res = pthread_mutex_unlock (&threads[idx].mutex);
	assert (res == 0);

	/* Now wait for the thread to exit.  */
	void *retval;
	res = pthread_join (threads[idx].thr, &retval);
	assert (res == 0);
	assert (retval == NULL);

	/* Now the thread has exited, mark it as no longer started.  */
	assert (threads[idx].started);
	threads[idx].started = FALSE;

	return TRUE;
      }

  return FALSE;
}

void
init_descriptor_array ()
{
  for (int i = 0; i < MAX_THREADS; ++i)
    {
      int res;

      threads[i].started = FALSE;
      res = pthread_cond_init (&threads[i].cond, NULL);
      assert (res == 0);
      res = pthread_mutex_init (&threads[i].mutex, NULL);
      assert (res == 0);
    }
}

void
breakpt ()
{
  /* Nothing.  */
}

int
main ()
{
  init_descriptor_array ();
  breakpt ();
  start_thread ();
  stop_thread ();
  breakpt ();
  return 0;
}
