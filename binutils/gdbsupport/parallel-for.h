/* Parallel for loops

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

#ifndef GDBSUPPORT_PARALLEL_FOR_H
#define GDBSUPPORT_PARALLEL_FOR_H

#include <algorithm>
#include <type_traits>
#include "gdbsupport/thread-pool.h"
#include "gdbsupport/function-view.h"

namespace gdb
{

/* A very simple "parallel for".  This splits the range of iterators
   into subranges, and then passes each subrange to the callback.  The
   work may or may not be done in separate threads.

   This approach was chosen over having the callback work on single
   items because it makes it simple for the caller to do
   once-per-subrange initialization and destruction.

   The parameter N says how batching ought to be done -- there will be
   at least N elements processed per thread.  Setting N to 0 is not
   allowed.  */

template<class RandomIt, class RangeFunction>
void
parallel_for_each (unsigned n, RandomIt first, RandomIt last,
		   RangeFunction callback)
{
  /* If enabled, print debug info about how the work is distributed across
     the threads.  */
  const bool parallel_for_each_debug = false;

  size_t n_worker_threads = thread_pool::g_thread_pool->thread_count ();
  size_t n_threads = n_worker_threads;
  size_t n_elements = last - first;
  size_t elts_per_thread = 0;
  size_t elts_left_over = 0;

  if (n_threads > 1)
    {
      /* Require that there should be at least N elements in a
	 thread.  */
      gdb_assert (n > 0);
      if (n_elements / n_threads < n)
	n_threads = std::max (n_elements / n, (size_t) 1);
      elts_per_thread = n_elements / n_threads;
      elts_left_over = n_elements % n_threads;
      /* n_elements == n_threads * elts_per_thread + elts_left_over. */
    }

  size_t count = n_threads == 0 ? 0 : n_threads - 1;
  std::vector<gdb::future<void>> results;

  if (parallel_for_each_debug)
    {
      debug_printf (_("Parallel for: n_elements: %zu\n"), n_elements);
      debug_printf (_("Parallel for: minimum elements per thread: %u\n"), n);
      debug_printf (_("Parallel for: elts_per_thread: %zu\n"), elts_per_thread);
    }

  for (int i = 0; i < count; ++i)
    {
      RandomIt end;
      end = first + elts_per_thread;
      if (i < elts_left_over)
	/* Distribute the leftovers over the worker threads, to avoid having
	   to handle all of them in a single thread.  */
	end++;

      /* This case means we don't have enough elements to really
	 distribute them.  Rather than ever submit a task that does
	 nothing, we short-circuit here.  */
      if (first == end)
	end = last;

      if (end == last)
	{
	  /* We're about to dispatch the last batch of elements, which
	     we normally process in the main thread.  So just truncate
	     the result list here.  This avoids submitting empty tasks
	     to the thread pool.  */
	  count = i;
	  break;
	}

      if (parallel_for_each_debug)
	{
	  debug_printf (_("Parallel for: elements on worker thread %i\t: %zu"),
			i, (size_t)(end - first));
	  debug_printf (_("\n"));
	}
      results.push_back (gdb::thread_pool::g_thread_pool->post_task ([=] ()
        {
	  return callback (first, end);
	}));
      first = end;
    }

  for (int i = count; i < n_worker_threads; ++i)
    if (parallel_for_each_debug)
      {
	debug_printf (_("Parallel for: elements on worker thread %i\t: 0"), i);
	debug_printf (_("\n"));
      }

  /* Process all the remaining elements in the main thread.  */
  if (parallel_for_each_debug)
    {
      debug_printf (_("Parallel for: elements on main thread\t\t: %zu"),
		    (size_t)(last - first));
      debug_printf (_("\n"));
    }
  callback (first, last);

  for (auto &fut : results)
    fut.get ();
}

/* A sequential drop-in replacement of parallel_for_each.  This can be useful
   when debugging multi-threading behaviour, and you want to limit
   multi-threading in a fine-grained way.  */

template<class RandomIt, class RangeFunction>
void
sequential_for_each (unsigned n, RandomIt first, RandomIt last,
		     RangeFunction callback)
{
  callback (first, last);
}

}

#endif /* GDBSUPPORT_PARALLEL_FOR_H */
