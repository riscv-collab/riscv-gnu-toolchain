/* Unit tests for the utils.c file.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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
#include "utils.h"
#include "gdbsupport/selftest.h"

namespace selftests {
namespace utils {

static void
test_substitute_path_component ()
{
  auto test = [] (std::string s, const char *from, const char *to,
		  const char *expected)
    {
      char *temp = xstrdup (s.c_str ());
      substitute_path_component (&temp, from, to);
      SELF_CHECK (strcmp (temp, expected) == 0);
      xfree (temp);
    };

  test ("/abc/$def/g", "abc", "xyz", "/xyz/$def/g");
  test ("abc/$def/g", "abc", "xyz", "xyz/$def/g");
  test ("/abc/$def/g", "$def", "xyz", "/abc/xyz/g");
  test ("/abc/$def/g", "g", "xyz", "/abc/$def/xyz");
  test ("/abc/$def/g", "ab", "xyz", "/abc/$def/g");
  test ("/abc/$def/g", "def", "xyz", "/abc/$def/g");
  test ("/abc/$def/g", "abc", "abc", "/abc/$def/g");
  test ("/abc/$def/g", "abc", "", "//$def/g");
  test ("/abc/$def/g", "abc/$def", "xyz", "/xyz/g");
  test ("/abc/$def/abc", "abc", "xyz", "/xyz/$def/xyz");
}

}
}

void _initialize_utils_selftests ();
void
_initialize_utils_selftests ()
{
  selftests::register_test ("substitute_path_component",
			    selftests::utils::test_substitute_path_component);
}
