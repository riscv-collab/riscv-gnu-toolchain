/* Self tests for parallel_for_each

   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

/* This file is divided in two parts:
   - FOR_EACH-undefined, and
   - FOR_EACH-defined.
   The former includes the latter, more than once, with different values for
   FOR_EACH.  The FOR_EACH-defined part reads like a regular function.  */
#ifndef FOR_EACH

#include "defs.h"
#include "gdbsupport/selftest.h"
#include "gdbsupport/parallel-for.h"

#if CXX_STD_THREAD

#include "gdbsupport/thread-pool.h"

namespace selftests {
namespace parallel_for {

struct save_restore_n_threads
{
  save_restore_n_threads ()
    : n_threads (gdb::thread_pool::g_thread_pool->thread_count ())
  {
  }

  ~save_restore_n_threads ()
  {
    gdb::thread_pool::g_thread_pool->set_thread_count (n_threads);
  }

  int n_threads;
};

/* Define test_par using TEST in the FOR_EACH-defined part.  */
#define TEST test_par
#define FOR_EACH gdb::parallel_for_each
#include "parallel-for-selftests.c"
#undef FOR_EACH
#undef TEST

/* Define test_seq using TEST in the FOR_EACH-defined part.  */
#define TEST test_seq
#define FOR_EACH gdb::sequential_for_each
#include "parallel-for-selftests.c"
#undef FOR_EACH
#undef TEST

static void
test (int n_threads)
{
  test_par (n_threads);
  test_seq (n_threads);
}

static void
test_n_threads ()
{
  test (0);
  test (1);
  test (3);
}

}
}

#endif /* CXX_STD_THREAD */

void _initialize_parallel_for_selftests ();
void
_initialize_parallel_for_selftests ()
{
#ifdef CXX_STD_THREAD
  selftests::register_test ("parallel_for",
			    selftests::parallel_for::test_n_threads);
#endif /* CXX_STD_THREAD */
}

#else /* FOR_EACH */

static void
TEST (int n_threads)
{
  save_restore_n_threads saver;
  gdb::thread_pool::g_thread_pool->set_thread_count (n_threads);

#define NUMBER 10000

  std::atomic<int> counter (0);
  FOR_EACH (1, 0, NUMBER,
	    [&] (int start, int end)
	    {
	      counter += end - start;
	    });
  SELF_CHECK (counter == NUMBER);

  counter = 0;
  FOR_EACH (1, 0, 0,
	    [&] (int start, int end)
	    {
	      counter += end - start;
	    });
  SELF_CHECK (counter == 0);

#undef NUMBER

  /* Check that if there are fewer tasks than threads, then we won't
     end up with a null result.  */
  std::vector<std::unique_ptr<int>> intresults;
  std::atomic<bool> any_empty_tasks (false);

  FOR_EACH (1, 0, 1,
	    [&] (int start, int end)
	      {
		if (start == end)
		  any_empty_tasks = true;
		return std::make_unique<int> (end - start);
	      });
  SELF_CHECK (!any_empty_tasks);
  SELF_CHECK (std::all_of (intresults.begin (),
			   intresults.end (),
			   [] (const std::unique_ptr<int> &entry)
			     {
			       return entry != nullptr;
			     }));
}

#endif /* FOR_EACH */
