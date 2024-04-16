/* Unit tests for the cli-utils.c file.

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
#include "cli/cli-utils.h"
#include "gdbsupport/selftest.h"

namespace selftests {
namespace cli_utils {

static void
test_number_or_range_parser ()
{
  /* Test parsing a simple integer.  */
  {
    number_or_range_parser one ("1");

    SELF_CHECK (!one.finished ());
    SELF_CHECK (one.get_number () == 1);
    SELF_CHECK (one.finished ());
    SELF_CHECK (strcmp (one.cur_tok (), "") == 0);
  }

  /* Test parsing an integer followed by a non integer.  */
  {
    number_or_range_parser one_after ("1 after");

    SELF_CHECK (!one_after.finished ());
    SELF_CHECK (one_after.get_number () == 1);
    SELF_CHECK (one_after.finished ());
    SELF_CHECK (strcmp (one_after.cur_tok (), "after") == 0);
  }

  /* Test parsing a range.  */
  {
    number_or_range_parser one_three ("1-3");

    for (int i = 1; i < 4; i++)
      {
	SELF_CHECK (!one_three.finished ());
	SELF_CHECK (one_three.get_number () == i);
      }
    SELF_CHECK (one_three.finished ());
    SELF_CHECK (strcmp (one_three.cur_tok (), "") == 0);
  }

  /* Test parsing a range followed by a non-integer.  */
  {
    number_or_range_parser one_three_after ("1-3 after");

    for (int i = 1; i < 4; i++)
      {
	SELF_CHECK (!one_three_after.finished ());
	SELF_CHECK (one_three_after.get_number () == i);
      }
    SELF_CHECK (one_three_after.finished ());
    SELF_CHECK (strcmp (one_three_after.cur_tok (), "after") == 0);
  }

  /* Test a negative integer gives an error.  */
  {
    number_or_range_parser minus_one ("-1");

    SELF_CHECK (!minus_one.finished ());
    try
      {
	minus_one.get_number ();
	SELF_CHECK (false);
      }
    catch (const gdb_exception_error &ex)
      {
	SELF_CHECK (ex.reason == RETURN_ERROR);
	SELF_CHECK (ex.error == GENERIC_ERROR);
	SELF_CHECK (strcmp (ex.what (), "negative value") == 0);
	SELF_CHECK (strcmp (minus_one.cur_tok (), "-1") == 0);
      }
  }

  /* Test that a - followed by not a number does not give an error.  */
  {
    number_or_range_parser nan ("-whatever");

    SELF_CHECK (nan.finished ());
    SELF_CHECK (strcmp (nan.cur_tok (), "-whatever") == 0);
  }
}

static void
test_cli_utils ()
{
  selftests::cli_utils::test_number_or_range_parser ();
}

}
}

void _initialize_cli_utils_selftests ();
void
_initialize_cli_utils_selftests ()
{
  selftests::register_test ("cli_utils",
			    selftests::cli_utils::test_cli_utils);
}
