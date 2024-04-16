/* Self tests for general utility routines for GDB, the GNU debugger.

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/selftest.h"

namespace selftests {

/* Type of both 'string_printf' and the 'format' function below.  Used
   to run the same tests against both string_printf and
   string_vprintf.  */
typedef std::string (format_func) (const char *fmt, ...)
  ATTRIBUTE_PRINTF (1, 2);

static void
test_format_func (format_func *func)
{
  /* Basic smoke tests.  */
  SELF_CHECK (func ("%s", "") == "");
  SELF_CHECK (func ("%s", "test") == "test");
  SELF_CHECK (func ("%d", 23) == "23");
  SELF_CHECK (func ("%s %d %s", "test", 23, "done") == "test 23 done");
  SELF_CHECK (func ("nothing") == "nothing");
  SELF_CHECK (func ("%d comes before 2", 1) == "1 comes before 2");
  SELF_CHECK (func ("hello %s", "world") == "hello world");

  /* Check that we don't mishandle very large strings.  (An earlier
     non-public implementation of string_printf mishandled this).  */
#define X10 "0123456789"
#define X100 X10 X10 X10 X10 X10 X10 X10 X10 X10 X10
#define X1000 X100 X100 X100 X100 X100 X100 X100 X100 X100 X100
#define X10000 X1000 X1000 X1000 X1000 X1000 X1000 X1000 X1000 X1000 X1000
#define X100000 X10000 X10000 X10000 X10000 X10000 X10000 X10000 X10000 X10000 X10000
  SELF_CHECK (func ("%s", X10) == X10);
  SELF_CHECK (func ("%s", X100) == X100);
  SELF_CHECK (func ("%s", X1000) == X1000);
  SELF_CHECK (func ("%s", X10000) == X10000);
  SELF_CHECK (func ("%s", X100000) == X100000);
}

static void
string_printf_tests ()
{
  test_format_func (string_printf);
}

static std::string ATTRIBUTE_PRINTF (1, 2)
format (const char *fmt, ...)
{
  va_list vp;

  va_start (vp, fmt);
  std::string result = string_vprintf (fmt, vp);
  va_end (vp);
  return result;
}

static void
string_vprintf_tests ()
{
  test_format_func (format);
}

/* Type of both 'string_appendf' and the 'string_vappendf_wrapper'
   function below.  Used to run the same tests against both
   string_appendf and string_vappendf.  */
typedef std::string &(string_appendf_func) (std::string &str, const char *fmt,
					    ...)
  ATTRIBUTE_PRINTF (2, 3);

static void
test_appendf_func (string_appendf_func *func)
{
  std::string str;

  func (str, "%s", "");
  SELF_CHECK (str == "");

  func (str, "%s", "test");
  SELF_CHECK (str == "test");

  func (str, "%d", 23);
  SELF_CHECK (str == "test23");

  func (str, "%s %d %s", "foo", 45, "bar");
  SELF_CHECK (str == "test23foo 45 bar");
}

static std::string & ATTRIBUTE_PRINTF (2, 3)
string_vappendf_wrapper (std::string &str, const char *fmt, ...)
{
  va_list vp;

  va_start (vp, fmt);
  string_vappendf (str, fmt, vp);
  va_end (vp);

  return str;
}

static void
string_appendf_tests ()
{
  test_appendf_func (string_appendf);
}

static void
string_vappendf_tests ()
{
  test_appendf_func (string_vappendf_wrapper);
}

} /* namespace selftests */

void _initialize_common_utils_selftests ();
void
_initialize_common_utils_selftests ()
{
  selftests::register_test ("string_printf", selftests::string_printf_tests);
  selftests::register_test ("string_vprintf", selftests::string_vprintf_tests);
  selftests::register_test ("string_appendf", selftests::string_appendf_tests);
  selftests::register_test ("string_vappendf",
			    selftests::string_vappendf_tests);
}
