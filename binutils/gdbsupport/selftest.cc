/* GDB self-testing.
   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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
#include "common-exceptions.h"
#include "common-debug.h"
#include "selftest.h"
#include <functional>

namespace selftests
{
/* All the tests that have been registered.  Using an std::set allows keeping
   the order of tests stable and easily looking up whether a test name
   exists.  */

static selftests_registry tests;

/* Set of callback functions used to register selftests after GDB is fully
   initialized.  */

static std::vector<selftests_generator> lazy_generators;

/* See selftest.h.  */

void
register_test (const std::string &name,
	       std::function<void(void)> function)
{
  /* Check that no test with this name already exist.  */
  auto status = tests.emplace (name, std::move (function));
  if (!status.second)
    gdb_assert_not_reached ("Test already registered");
}

/* See selftest.h.  */

void
add_lazy_generator (selftests_generator generator)
{
  lazy_generators.push_back (std::move (generator));
}

/* See selftest.h.  */

static bool run_verbose_ = false;

/* See selftest.h.  */

bool
run_verbose ()
{
  return run_verbose_;
}

/* See selftest.h.  */

void
run_tests (gdb::array_view<const char *const> filters, bool verbose)
{
  int ran = 0;
  run_verbose_ = verbose;
  std::vector<const char *> failed;

  for (const auto &test : all_selftests ())
    {
      bool run = false;

      if (filters.empty ())
	run = true;
      else
	{
	  for (const char *filter : filters)
	    {
	      if (test.name.find (filter) != std::string::npos)
		run = true;
	    }
	}

      if (!run)
	continue;

      try
	{
	  debug_printf (_("Running selftest %s.\n"), test.name.c_str ());
	  ++ran;
	  test.test ();
	}
      catch (const gdb_exception_error &ex)
	{
	  debug_printf ("Self test failed: %s\n", ex.what ());
	  failed.push_back (test.name.c_str ());
	}

      reset ();
    }

  if (!failed.empty ())
    {
      debug_printf ("\nFailures:\n");

      for (const char *name : failed)
	debug_printf ("  %s\n", name);

      debug_printf ("\n");
    }

  debug_printf (_("Ran %d unit tests, %zu failed\n"),
		ran, failed.size ());
}

/* See selftest.h.  */

selftests_range
all_selftests ()
{
  /* Execute any function which might still want to register tests.  Once each
     function has been executed, clear lazy_generators to ensure that
     callback functions are only executed once.  */
  for (const auto &generator : lazy_generators)
    for (selftest &test : generator ())
      register_test (std::move (test.name), std::move (test.test));
  lazy_generators.clear ();

  return selftests_range (tests.cbegin (), tests.cend ());
}

} // namespace selftests
