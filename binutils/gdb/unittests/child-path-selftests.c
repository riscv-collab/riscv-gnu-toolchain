/* Self tests for child_path for GDB, the GNU debugger.

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
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/selftest.h"

namespace selftests {
namespace child_path {

/* Verify the result of a single child_path test.  */

static bool
child_path_check (const char *parent, const char *child, const char *expected)
{
  const char *result = ::child_path (parent, child);
  if (result == NULL || expected == NULL)
    return result == expected;
  return strcmp (result, expected) == 0;
}

/* Test child_path.  */

static void
test ()
{
  SELF_CHECK (child_path_check ("/one", "/two", NULL));
  SELF_CHECK (child_path_check ("/one", "/one", NULL));
  SELF_CHECK (child_path_check ("/one", "/one/", NULL));
  SELF_CHECK (child_path_check ("/one", "/one//", NULL));
  SELF_CHECK (child_path_check ("/one", "/one/two", "two"));
  SELF_CHECK (child_path_check ("/one/", "/two", NULL));
  SELF_CHECK (child_path_check ("/one/", "/one", NULL));
  SELF_CHECK (child_path_check ("/one/", "/one/", NULL));
  SELF_CHECK (child_path_check ("/one/", "/one//", NULL));
  SELF_CHECK (child_path_check ("/one/", "/one/two", "two"));
  SELF_CHECK (child_path_check ("/one/", "/one//two", "two"));
  SELF_CHECK (child_path_check ("/one/", "/one//two/", "two/"));
  SELF_CHECK (child_path_check ("/one", "/onetwo", NULL));
  SELF_CHECK (child_path_check ("/one", "/onetwo/three", NULL));
}

}
}

void _initialize_child_path_selftests ();
void
_initialize_child_path_selftests ()
{
  selftests::register_test ("child_path",
			    selftests::child_path::test);
}

