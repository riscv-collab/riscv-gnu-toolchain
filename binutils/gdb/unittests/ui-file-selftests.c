/* Self tests for ui_file

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
#include "gdbsupport/selftest.h"
#include "ui-file.h"

namespace selftests {
namespace file {

static void
check_one (const char *str, int quoter, const char *result)
{
  string_file out;
  out.putstr (str, quoter);
  SELF_CHECK (out.string () == result);
}

static void
run_tests ()
{
  check_one ("basic stuff: \\", '\\',
	     "basic stuff: \\\\");
  check_one ("more basic stuff: \\Q", 'Q',
	     "more basic stuff: \\\\\\Q");
  check_one ("more basic stuff: \\Q", '\0',
	     "more basic stuff: \\Q");

  check_one ("weird stuff: \x1f\x90\n\b\t\f\r\033\007", '\\',
	     "weird stuff: \\037\\220\\n\\b\\t\\f\\r\\e\\a");

  scoped_restore save_7 = make_scoped_restore (&sevenbit_strings, true);
  check_one ("more weird stuff: \xa5", '\\',
	     "more weird stuff: \\245");
}

} /* namespace file*/
} /* namespace selftests */

void _initialize_ui_file_selftest ();
void
_initialize_ui_file_selftest ()
{
  selftests::register_test ("ui-file",
			    selftests::file::run_tests);
}
