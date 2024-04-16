/* Self tests for scoped_restore for GDB, the GNU debugger.

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
#include "gdbsupport/scoped_restore.h"

namespace selftests {
namespace scoped_restore_tests {

struct Base {};
struct Derived : Base {};

static int global;

/* Check that we can return a scoped_restore from a function.  Below
   we'll make sure this does the right thing.  */
static scoped_restore_tmpl<int>
make_scoped_restore_global (int value)
{
  return make_scoped_restore (&global, value);
}

static void
run_tests ()
{
  /* Test that single-argument make_scoped_restore restores the
     original value on scope exit.  */
  {
    int integer = 0;
    {
      scoped_restore restore = make_scoped_restore (&integer);
      SELF_CHECK (integer == 0);
      integer = 1;
    }
    SELF_CHECK (integer == 0);
  }

  /* Same, with two-argument make_scoped_restore.  */
  {
    int integer = 0;
    {
      scoped_restore restore = make_scoped_restore (&integer, 1);
      SELF_CHECK (integer == 1);
    }
    SELF_CHECK (integer == 0);
  }

  /* Test releasing a scoped_restore.  */
  {
    int integer = 0;
    {
      scoped_restore restore = make_scoped_restore (&integer, 1);
      SELF_CHECK (integer == 1);
      restore.release ();
    }
    /* The overridden value should persist.  */
    SELF_CHECK (integer == 1);
  }

  /* Test creating a scoped_restore with a value of a type convertible
     to T.  */
  {
    Base *base = nullptr;
    Derived derived;
    {
      scoped_restore restore = make_scoped_restore (&base, &derived);

      SELF_CHECK (base == &derived);
    }
    SELF_CHECK (base == nullptr);
  }

  /* Test calling a function that returns a scoped restore.  Makes
     sure that if the compiler emits a call to the copy ctor, that we
     still do the right thing.  */
  {
    {
      SELF_CHECK (global == 0);
      scoped_restore restore = make_scoped_restore_global (1);
      SELF_CHECK (global == 1);
    }
    SELF_CHECK (global == 0);
  }
}

} /* namespace scoped_restore_tests */
} /* namespace selftests */

void _initialize_scoped_restore_selftests ();
void
_initialize_scoped_restore_selftests ()
{
  selftests::register_test ("scoped_restore",
			    selftests::scoped_restore_tests::run_tests);
}
