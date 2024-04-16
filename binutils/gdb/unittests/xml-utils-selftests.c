/* Unit tests for the xml-utils.c file.

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
#include "gdbsupport/xml-utils.h"
#include "gdbsupport/selftest.h"

namespace selftests {
namespace xml_utils {

static void test_xml_escape_text ()
{
  const char *input = "<this isn't=\"xml\"> &";
  const char *expected_output = "&lt;this isn&apos;t=&quot;xml&quot;&gt; &amp;";
  std::string actual_output = xml_escape_text (input);

  SELF_CHECK (actual_output == expected_output);
}

static void test_xml_escape_text_append ()
{
  /* Make sure that we do indeed append.  */
  std::string actual_output = "foo<xml>";
  const char *input = "<this isn't=\"xml\"> &";
  const char *expected_output
    = "foo<xml>&lt;this isn&apos;t=&quot;xml&quot;&gt; &amp;";
  xml_escape_text_append (actual_output, input);

  SELF_CHECK (actual_output == expected_output);
}

}
}

void _initialize_xml_utils ();
void
_initialize_xml_utils ()
{
  selftests::register_test ("xml_escape_text",
			    selftests::xml_utils::test_xml_escape_text);
  selftests::register_test ("xml_escape_text_append",
			    selftests::xml_utils::test_xml_escape_text_append);
}
