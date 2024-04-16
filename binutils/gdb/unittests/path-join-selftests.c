/* Self tests for path_join for GDB, the GNU debugger.

   Copyright (C) 2022-2024 Free Software Foundation, Inc.

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
namespace path_join {

template <typename ...Args>
static void
test_one (const char *expected, Args... paths)
{
  std::string actual = ::path_join (paths...);

  SELF_CHECK (actual == expected);
}

/* Test path_join.  */

static void
test ()
{
  test_one ("/foo/bar", "/foo", "bar");
  test_one ("/bar", "/", "bar");
  test_one ("foo/bar/", "foo", "bar", "");
  test_one ("foo", "", "foo");
  test_one ("foo/bar", "foo", "", "bar");
  test_one ("foo/", "foo", "");
  test_one ("foo/", "foo/", "");

  test_one ("D:/foo/bar", "D:/foo", "bar");
  test_one ("D:/foo/bar", "D:/foo/", "bar");

#if defined(_WIN32)
  /* The current implementation doesn't recognize backslashes as directory
     separators on Unix-like systems, so only run these tests on Windows.  If
     we ever switch our implementation to use std::filesystem::path, they
     should work anywhere, in theory.  */
  test_one ("D:\\foo/bar", "D:\\foo", "bar");
  test_one ("D:\\foo\\bar", "D:\\foo\\", "bar");
  test_one ("\\\\server\\dir\\file", "\\\\server\\dir\\", "file");
  test_one ("\\\\server\\dir/file", "\\\\server\\dir", "file");
#endif /* _WIN32 */
}

}
}

void _initialize_path_join_selftests ();
void
_initialize_path_join_selftests ()
{
  selftests::register_test ("path_join",
			    selftests::path_join::test);
}
