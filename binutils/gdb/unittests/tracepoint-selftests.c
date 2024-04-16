/* Self tests for tracepoint-related code for GDB, the GNU debugger.

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
#include "gdbsupport/selftest.h"
#include "tracepoint.h"

namespace selftests {
namespace tracepoint_tests {

static void
test_parse_static_tracepoint_marker_definition ()
{
  static_tracepoint_marker marker;
  const char def[] = ("1234:6d61726b657231:6578747261207374756666,"
		      "abba:6d61726b657232:,"
		      "cafe:6d61726b657233:6d6f72657374756666");
  const char *start = def;
  const char *end;

  parse_static_tracepoint_marker_definition (start, &end, &marker);

  SELF_CHECK (marker.address == 0x1234);
  SELF_CHECK (marker.str_id == "marker1");
  SELF_CHECK (marker.extra == "extra stuff");
  SELF_CHECK (end == strchr (start, ','));

  start = end + 1;
  parse_static_tracepoint_marker_definition (start, &end, &marker);

  SELF_CHECK (marker.address == 0xabba);
  SELF_CHECK (marker.str_id == "marker2");
  SELF_CHECK (marker.extra == "");
  SELF_CHECK (end == strchr (start, ','));

  start = end + 1;
  parse_static_tracepoint_marker_definition (start, &end, &marker);

  SELF_CHECK (marker.address == 0xcafe);
  SELF_CHECK (marker.str_id == "marker3");
  SELF_CHECK (marker.extra == "morestuff");
  SELF_CHECK (end == def + strlen (def));
}

} /* namespace tracepoint_tests */
} /* namespace selftests */

void _initialize_tracepoint_selftests ();
void
_initialize_tracepoint_selftests ()
{
  selftests::register_test
    ("parse_static_tracepoint_marker_definition",
     selftests::tracepoint_tests::test_parse_static_tracepoint_marker_definition);
}
