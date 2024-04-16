/* This testcase is part of GDB, the GNU debugger.

   Copyright 2018-2024 Free Software Foundation, Inc.

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

/* This file contains a library that can be preloaded into GDB on Linux
   using the LD_PRELOAD technique.

   The library intercepts calls to WAITPID and SIGSUSPEND in order to
   simulate the behaviour of a heavily loaded kernel.

   When GDB wants to stop all threads in an inferior each thread is sent a
   SIGSTOP, GDB will then wait for the signal to be received by the thread
   with a waitpid call.

   If the kernel is slow in either delivering the signal, or making the
   result available to the waitpid call then GDB will enter a sigsuspend
   call in order to wait for the inferior threads to change state, this is
   signalled to GDB with a SIGCHLD.

   A bug in GDB meant that in some cases we would deadlock during this
   process.  This was rarely seen as the kernel is usually quick at
   delivering signals and making the results available to waitpid, so quick
   that GDB would gather the statuses from all inferior threads in the
   original pass.

   The idea in this library is to rate limit calls to waitpid (where pid is
   -1 and the WNOHANG option is set) so that only 1 per second can return
   an answer.  Any additional calls will report that no threads are
   currently ready.  This should match the behaviour we see on a slow
   kernel.

   However, given that usually when using this library, the kernel does
   have the waitpid result ready this means that the kernel will never send
   GDB a SIGCHLD.  This means that when GDB enters sigsuspend it will block
   forever.  Alternatively, if GDB enters its polling loop the lack of
   SIGCHLD means that we will never see an event on the child threads.  To
   resolve these problems the library intercepts calls to sigsuspend and
   forces the call to exit if there is a pending waitpid result.  Also,
   when we know that there's a waitpid result that we've ignored, we create
   a new thread which, after a short delay, will send GDB a SIGCHLD.  */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

/* Logging.  */

static void
log_msg (const char *fmt, ...)
{
#ifdef LOGGING
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
#endif /* LOGGING */
}

/* Error handling, message and exit.  */

static void
error (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);

  exit (EXIT_FAILURE);
}

/* Cache the result of a waitpid call that has not been reported back to
   GDB yet.  We only ever cache a single result.  Once we have a result
   cached then later calls to waitpid with the WNOHANG option will return a
   result of 0.  */

static struct
{
  /* Flag to indicate when we have a result cached.  */
  int cached_p;

  /* The cached result fields from a waitpid call.  */
  pid_t pid;
  int wstatus;
} cached_wait_status;

/* Lock to hold when modifying SIGNAL_THREAD_ACTIVE_P.  */

static pthread_mutex_t thread_creation_lock_obj = PTHREAD_MUTEX_INITIALIZER;
#define thread_creation_lock (&thread_creation_lock_obj)

/* This flag is only modified while holding the THREAD_CREATION_LOCK mutex.
   When this flag is true then there is a signal thread alive that will be
   sending a SIGCHLD at some point in the future.  */

static int signal_thread_active_p;

/* When we last allowed a waitpid to complete.  */

static struct timeval last_waitpid_time = { 0, 0 };

/* The number of seconds that must elapse between calls to waitpid where
   the pid is -1 and the WNOHANG option is set.  If calls occur faster than
   this then we force a result of 0 to be returned from waitpid.  */

#define WAITPID_MIN_TIME (1)

/* Return true (non-zero) if we should skip this call to waitpid, or false
   (zero) if this waitpid call should be handled with a call to the "real"
   waitpid function.  Allows 1 waitpid call per second.  */

static int
should_skip_waitpid (void)
{
  struct timeval *tv = &last_waitpid_time;
  if (tv->tv_sec == 0)
    {
      if (gettimeofday (tv, NULL) < 0)
	error ("error: gettimeofday failed\n");
      return 0; /* Don't skip.  */
    }
  else
    {
      struct timeval new_tv;

      if (gettimeofday (&new_tv, NULL) < 0)
	error ("error: gettimeofday failed\n");

      if ((new_tv.tv_sec - tv->tv_sec) < WAITPID_MIN_TIME)
	return 1; /* Skip.  */

      *tv = new_tv;
    }

  /* Don't skip.  */
  return 0;
}

/* Perform a real waitpid call.  */

static pid_t
real_waitpid (pid_t pid, int *wstatus, int options)
{
  typedef pid_t (*fptr_t) (pid_t, int *, int);
  static fptr_t real_func = NULL;

  if (real_func == NULL)
    {
      real_func = dlsym (RTLD_NEXT, "waitpid");
      if (real_func == NULL)
	error ("error: failed to find real waitpid\n");
    }

  return (*real_func) (pid, wstatus, options);
}

/* Thread worker created when we cache a waitpid result.  Delays for a
   short period of time and then sends SIGCHLD to the GDB process.  This
   should trigger GDB to call waitpid again, at which point we will make
   the cached waitpid result available.  */

static void*
send_sigchld_thread (void *arg)
{
  /* Delay one second longer than WAITPID_MIN_TIME so that there can be no
     chance that a call to SHOULD_SKIP_WAITPID will return true once the
     SIGCHLD is delivered and handled.  */
  sleep (WAITPID_MIN_TIME + 1);

  pthread_mutex_lock (thread_creation_lock);
  signal_thread_active_p = 0;

  if (cached_wait_status.cached_p)
    {
      log_msg ("signal-thread: sending SIGCHLD\n");
      kill (getpid (), SIGCHLD);
    }

  pthread_mutex_unlock (thread_creation_lock);
  return NULL;
}

/* The waitpid entry point function.  */

pid_t
waitpid (pid_t pid, int *wstatus, int options)
{
  log_msg ("waitpid: waitpid (%d, %p, 0x%x)\n", pid, wstatus, options);

  if ((options & WNOHANG) != 0
      && pid == -1
      && should_skip_waitpid ())
    {
      if (!cached_wait_status.cached_p)
	{
	  /* Do the waitpid call, but hold the result back.  */
	  pid_t tmp_pid;
	  int tmp_wstatus;

	  tmp_pid = real_waitpid (-1, &tmp_wstatus, options);
	  if (tmp_pid > 0)
	    {
	      log_msg ("waitpid: delaying waitpid result (pid = %d)\n",
		       tmp_pid);

	      /* Cache the result.  */
	      cached_wait_status.pid = tmp_pid;
	      cached_wait_status.wstatus = tmp_wstatus;
	      cached_wait_status.cached_p = 1;

	      /* Is there a thread around that will be sending a signal in
		 the near future?  The prevents us from creating one
		 thread per call to waitpid when the calls occur in a
		 sequence.  */
	      pthread_mutex_lock (thread_creation_lock);
	      if (!signal_thread_active_p)
		{
		  sigset_t old_ss, new_ss;
		  pthread_t thread_id;
		  pthread_attr_t attr;

		  /* Create the new signal sending thread in detached
		     state.  This means that the thread doesn't need to be
		     pthread_join'ed.  Which is fine as there's no result
		     we care about.  */
		  pthread_attr_init (&attr);
		  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

		  /* Ensure the signal sending thread has all signals
		     blocked.  We don't want any signals to GDB to be
		     handled in that thread.  */
		  sigfillset (&new_ss);
		  sigprocmask (SIG_BLOCK, &new_ss, &old_ss);

		  log_msg ("waitpid: spawn thread to signal us\n");
		  if (pthread_create (&thread_id, &attr,
				      send_sigchld_thread, NULL) != 0)
		    error ("error: pthread_create failed\n");

		  signal_thread_active_p = 1;
		  sigprocmask (SIG_SETMASK, &old_ss, NULL);
		  pthread_attr_destroy (&attr);
		}

	      pthread_mutex_unlock (thread_creation_lock);
	    }
	}

      log_msg ("waitpid: skipping\n");
      return 0;
    }

  /* If we have a cached result that is a suitable reply for this call to
     waitpid then send that cached result back now.  */
  if (cached_wait_status.cached_p
      && (pid == -1 || pid == cached_wait_status.pid))
    {
      pid_t pid;

      pid = cached_wait_status.pid;
      log_msg ("waitpid: return cached result (%d)\n", pid);
      *wstatus = cached_wait_status.wstatus;
      cached_wait_status.cached_p = 0;
      return pid;
    }

  log_msg ("waitpid: real waitpid call\n");
  return real_waitpid (pid, wstatus, options);
}

/* Perform a real sigsuspend call.  */

static int
real_sigsuspend (const sigset_t *mask)
{
  typedef int (*fptr_t) (const sigset_t *);
  static fptr_t real_func = NULL;

  if (real_func == NULL)
    {
      real_func = dlsym (RTLD_NEXT, "sigsuspend");
      if (real_func == NULL)
	error ("error: failed to find real sigsuspend\n");
    }

  return (*real_func) (mask);
}

/* The sigsuspend entry point function.  */

int
sigsuspend (const sigset_t *mask)
{
  log_msg ("sigsuspend: sigsuspend (0x%p)\n", ((void *) mask));

  /* If SIGCHLD is _not_ in MASK, and is therefore deliverable, then if we
     have a pending wait status pretend that a signal arrived.  We will
     have a thread alive that is going to deliver a signal but doing this
     will boost the speed as we don't have to wait for a signal.  If the
     signal ends up being delivered then it should be harmless, we'll just
     perform an additional waitpid call.   */
  if (!sigismember (mask, SIGCHLD))
    {
      if (cached_wait_status.cached_p)
	{
	  log_msg ("sigsuspend: interrupt for cached waitstatus\n");
	  last_waitpid_time.tv_sec = 0;
	  last_waitpid_time.tv_usec = 0;
	  errno = EINTR;
	  return -1;
	}
    }

  log_msg ("sigsuspend: real sigsuspend call\n");
  return real_sigsuspend (mask);
}
