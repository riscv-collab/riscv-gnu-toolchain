/* Self tests for run_on_main_thread

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

#include "defs.h"
#include "gdbsupport/selftest.h"
#include "gdbsupport/block-signals.h"
#include "gdbsupport/scope-exit.h"
#include "run-on-main-thread.h"
#include "gdbsupport/event-loop.h"
#if CXX_STD_THREAD
#include <thread>
#endif

namespace selftests {
namespace main_thread_tests {

#if CXX_STD_THREAD

static bool done;

static void
set_done ()
{
  run_on_main_thread ([] ()
    {
      done = true;
    });
}

static void
run_tests ()
{
  std::thread thread;

  done = false;

  {
    gdb::block_signals blocker;

    SCOPE_EXIT
      {
	if (thread.joinable ())
	  thread.join ();
      };
    thread = std::thread (set_done);
  }

  while (!done && gdb_do_one_event () >= 0)
    ;

  /* Actually the test will just hang, but we want to test
     something.  */
  SELF_CHECK (done);
}

#endif

}
}

void _initialize_main_thread_selftests ();
void
_initialize_main_thread_selftests ()
{
#if CXX_STD_THREAD
  selftests::register_test ("run_on_main_thread",
			    selftests::main_thread_tests::run_tests);
#endif
}
