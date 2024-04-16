/* Thread pool

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "gdbsupport/thread-pool.h"

#if CXX_STD_THREAD

#include "gdbsupport/alt-stack.h"
#include "gdbsupport/block-signals.h"
#include <algorithm>
#include <system_error>

/* On the off chance that we have the pthread library on a Windows
   host, but std::thread is not using it, avoid calling
   pthread_setname_np on Windows.  */
#ifndef _WIN32
#ifdef HAVE_PTHREAD_SETNAME_NP
#define USE_PTHREAD_SETNAME_NP
#endif
#endif

#ifdef USE_PTHREAD_SETNAME_NP

#include <pthread.h>

/* Handle platform discrepancies in pthread_setname_np: macOS uses a
   single-argument form, while Linux uses a two-argument form.  NetBSD
   takes a printf-style format and an argument.  This wrapper handles the
   difference.  */

ATTRIBUTE_UNUSED static void
do_set_thread_name (int (*set_name) (pthread_t, const char *, void *),
		    const char *name)
{
  set_name (pthread_self (), "%s", const_cast<char *> (name));
}

ATTRIBUTE_UNUSED static void
do_set_thread_name (int (*set_name) (pthread_t, const char *),
		    const char *name)
{
  set_name (pthread_self (), name);
}

/* The macOS man page says that pthread_setname_np returns "void", but
   the headers actually declare it returning "int".  */
ATTRIBUTE_UNUSED static void
do_set_thread_name (int (*set_name) (const char *), const char *name)
{
  set_name (name);
}

static void
set_thread_name (const char *name)
{
  do_set_thread_name (pthread_setname_np, name);
}

#elif defined (USE_WIN32API)

#include <windows.h>

typedef HRESULT WINAPI (SetThreadDescription_ftype) (HANDLE, PCWSTR);
static SetThreadDescription_ftype *dyn_SetThreadDescription;
static bool initialized;

static void
init_windows ()
{
  initialized = true;

  HMODULE hm = LoadLibrary (TEXT ("kernel32.dll"));
  if (hm)
    dyn_SetThreadDescription
      = (SetThreadDescription_ftype *) GetProcAddress (hm,
						       "SetThreadDescription");

  /* On some versions of Windows, this function is only available in
     KernelBase.dll, not kernel32.dll.  */
  if (dyn_SetThreadDescription == nullptr)
    {
      hm = LoadLibrary (TEXT ("KernelBase.dll"));
      if (hm)
	dyn_SetThreadDescription
	  = (SetThreadDescription_ftype *) GetProcAddress (hm,
							   "SetThreadDescription");
    }
}

static void
do_set_thread_name (const wchar_t *name)
{
  if (!initialized)
    init_windows ();

  if (dyn_SetThreadDescription != nullptr)
    dyn_SetThreadDescription (GetCurrentThread (), name);
}

#define set_thread_name(NAME) do_set_thread_name (L ## NAME)

#else /* USE_WIN32API */

static void
set_thread_name (const char *name)
{
}

#endif

#endif /* CXX_STD_THREAD */

namespace gdb
{

/* The thread pool detach()s its threads, so that the threads will not
   prevent the process from exiting.  However, it was discovered that
   if any detached threads were still waiting on a condition variable,
   then the condition variable's destructor would wait for the threads
   to exit -- defeating the purpose.

   Allocating the thread pool on the heap and simply "leaking" it
   avoids this problem.
*/
thread_pool *thread_pool::g_thread_pool = new thread_pool ();

thread_pool::~thread_pool ()
{
  /* Because this is a singleton, we don't need to clean up.  The
     threads are detached so that they won't prevent process exit.
     And, cleaning up here would be actively harmful in at least one
     case -- see the comment by the definition of g_thread_pool.  */
}

void
thread_pool::set_thread_count (size_t num_threads)
{
#if CXX_STD_THREAD
  std::lock_guard<std::mutex> guard (m_tasks_mutex);
  m_sized_at_least_once = true;

  /* If the new size is larger, start some new threads.  */
  if (m_thread_count < num_threads)
    {
      /* Ensure that signals used by gdb are blocked in the new
	 threads.  */
      block_signals blocker;
      for (size_t i = m_thread_count; i < num_threads; ++i)
	{
	  try
	    {
	      std::thread thread (&thread_pool::thread_function, this);
	      thread.detach ();
	    }
	  catch (const std::system_error &)
	    {
	      /* libstdc++ may not implement std::thread, and will
		 throw an exception on use.  It seems fine to ignore
		 this, and any other sort of startup failure here.  */
	      num_threads = i;
	      break;
	    }
	}
    }
  /* If the new size is smaller, terminate some existing threads.  */
  if (num_threads < m_thread_count)
    {
      for (size_t i = num_threads; i < m_thread_count; ++i)
	m_tasks.emplace ();
      m_tasks_cv.notify_all ();
    }

  m_thread_count = num_threads;
#else
  /* No threads available, simply ignore the request.  */
#endif /* CXX_STD_THREAD */
}

#if CXX_STD_THREAD

void
thread_pool::do_post_task (std::packaged_task<void ()> &&func)
{
  /* This assert is here to check that no tasks are posted to the pool between
     its initialization and sizing.  */
  gdb_assert (m_sized_at_least_once);
  std::packaged_task<void ()> t (std::move (func));

  if (m_thread_count != 0)
    {
      std::lock_guard<std::mutex> guard (m_tasks_mutex);
      m_tasks.emplace (std::move (t));
      m_tasks_cv.notify_one ();
    }
  else
    {
      /* Just execute it now.  */
      t ();
    }
}

void
thread_pool::thread_function ()
{
  /* This must be done here, because on macOS one can only set the
     name of the current thread.  */
  set_thread_name ("gdb worker");

  /* Ensure that SIGSEGV is delivered to an alternate signal
     stack.  */
  gdb::alternate_signal_stack signal_stack;

  while (true)
    {
      std::optional<task_t> t;

      {
	/* We want to hold the lock while examining the task list, but
	   not while invoking the task function.  */
	std::unique_lock<std::mutex> guard (m_tasks_mutex);
	while (m_tasks.empty ())
	  m_tasks_cv.wait (guard);
	t = std::move (m_tasks.front());
	m_tasks.pop ();
      }

      if (!t.has_value ())
	break;
      (*t) ();
    }
}

#endif /* CXX_STD_THREAD */

} /* namespace gdb */
