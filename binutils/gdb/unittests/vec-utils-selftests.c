/* Self tests for vector utility routines for GDB, the GNU debugger.

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/selftest.h"

#include "gdbsupport/gdb_vecs.h"

namespace selftests {
namespace vector_utils_tests {

static void
unordered_remove_tests ()
{
  /* Create vector with a single object in, and then remove that object.
     This test was added after a bug was discovered where unordered_remove
     would cause a self move assign.  If the C++ standard library is
     compiled in debug mode (by passing -D_GLIBCXX_DEBUG=1) and the
     operator= is removed from struct obj this test used to fail with an
     error from the C++ standard library.  */
  struct obj
  {
    std::vector<void *> var;

    obj() = default;

    /* gcc complains if we provide an assignment operator but no copy
       constructor, so provide one even if don't really care for this test.  */
    obj(const obj &other)
    {
      this->var = other.var;
    }

    obj &operator= (const obj &other)
    {
      if (this == &other)
	error (_("detected self move assign"));
      this->var = other.var;
      return *this;
    }
  };

  std::vector<obj> v;
  v.emplace_back ();
  auto it = v.end () - 1;
  unordered_remove (v, it);
  SELF_CHECK (v.empty ());
}

} /* namespace vector_utils_tests */
} /* namespace selftests */

void _initialize_vec_utils_selftests ();
void
_initialize_vec_utils_selftests ()
{
  selftests::register_test
    ("unordered_remove",
     selftests::vector_utils_tests::unordered_remove_tests);
}
