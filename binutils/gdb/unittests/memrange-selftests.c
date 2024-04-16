/* Self tests for mem ranges for GDB, the GNU debugger.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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
#include "memrange.h"

namespace selftests {
namespace memrange_tests {

static void
normalize_mem_ranges_tests ()
{
  /* Empty vector.  */
  {
    std::vector<mem_range> ranges;

    normalize_mem_ranges (&ranges);

    SELF_CHECK (ranges.size () == 0);
  }

  /* With one range.  */
  {
    std::vector<mem_range> ranges;

    ranges.emplace_back (10, 20);

    normalize_mem_ranges (&ranges);

    SELF_CHECK (ranges.size () == 1);
    SELF_CHECK (ranges[0] == mem_range (10, 20));
  }

  /* Completely disjoint ranges.  */
  {
    std::vector<mem_range> ranges;

    ranges.emplace_back (20, 1);
    ranges.emplace_back (10, 1);

    normalize_mem_ranges (&ranges);

    SELF_CHECK (ranges.size () == 2);
    SELF_CHECK (ranges[0] == mem_range (10, 1));
    SELF_CHECK (ranges[1] == mem_range (20, 1));
  }

  /* Overlapping and contiguous ranges.  */
  {
    std::vector<mem_range> ranges;

    ranges.emplace_back (5, 10);
    ranges.emplace_back (10, 10);
    ranges.emplace_back (15, 10);

    normalize_mem_ranges (&ranges);

    SELF_CHECK (ranges.size () == 1);
    SELF_CHECK (ranges[0] == mem_range (5, 20));
  }

  /* Duplicate ranges.  */
  {
    std::vector<mem_range> ranges;

    ranges.emplace_back (10, 10);
    ranges.emplace_back (10, 10);

    normalize_mem_ranges (&ranges);

    SELF_CHECK (ranges.size () == 1);
    SELF_CHECK (ranges[0] == mem_range (10, 10));
  }

  /* Range completely inside another.  */
  {
    std::vector<mem_range> ranges;

    ranges.emplace_back (14, 2);
    ranges.emplace_back (10, 10);

    normalize_mem_ranges (&ranges);

    SELF_CHECK (ranges.size () == 1);
    SELF_CHECK (ranges[0] == mem_range (10, 10));
  }
}

} /* namespace memrange_tests */
} /* namespace selftests */

void _initialize_memrange_selftests ();
void
_initialize_memrange_selftests ()
{
  selftests::register_test
    ("normalize_mem_ranges",
     selftests::memrange_tests::normalize_mem_ranges_tests);
}
