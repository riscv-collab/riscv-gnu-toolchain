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

#ifndef COMMON_SELFTEST_H
#define COMMON_SELFTEST_H

#include "gdbsupport/array-view.h"
#include "gdbsupport/function-view.h"
#include "gdbsupport/iterator-range.h"
#include <set>
#include <vector>

/* A test is just a function that does some checks and throws an
   exception if something has gone wrong.  */

namespace selftests
{

/* Selftests are registered under a unique name.  */

struct selftest
{
  selftest (std::string name, std::function<void (void)> test)
    : name { std::move (name) }, test { std::move (test) }
  { }
  bool operator< (const selftest &rhs) const
  { return name < rhs.name; }

  std::string name;
  std::function<void (void)> test;
};

/* Type of the container of all the registered selftests.  */
using selftests_registry = std::set<selftest>;
using selftests_range = iterator_range<selftests_registry::const_iterator>;

/* Create a range to iterate over all registered tests.  */

selftests_range all_selftests ();

/* True if selftest should run verbosely.  */

extern bool run_verbose ();

/* Register a new self-test.  */

extern void register_test (const std::string &name,
			   std::function<void(void)> function);

/* A selftest generator is a callback function used to delay the generation
   of selftests.  */

using selftests_generator = std::function<std::vector<selftest> (void)>;

/* Register a function which can lazily register selftests once GDB is fully
   initialized. */

extern void add_lazy_generator (selftests_generator generator);

/* Run all the self tests.  This print a message describing the number
   of test and the number of failures.

   If FILTERS is not empty, only run tests with names containing one of the
   element of FILTERS.  */

extern void run_tests (gdb::array_view<const char *const> filters,
		       bool verbose = false);

/* Reset GDB or GDBserver's internal state.  */
extern void reset ();
}

/* Check that VALUE is true, and, if not, throw an exception.  */

#define SELF_CHECK(VALUE)						\
  do {									\
    if (!(VALUE))							\
      error (_("self-test failed at %s:%d"), __FILE__, __LINE__);	\
  } while (0)

#endif /* COMMON_SELFTEST_H */
