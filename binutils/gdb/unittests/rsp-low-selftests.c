/* Unit tests for the rsp-low.c file.

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
#include "gdbsupport/rsp-low.h"

namespace selftests {
namespace rsp_low {

/* Test the variant of hex2bin that returns a byte_vector.  */

static void test_hex2bin_byte_vector ()
{
  gdb::byte_vector bv;

  /* Test an empty string.  */
  bv = hex2bin ("");
  SELF_CHECK (bv.size () == 0);

  /* Test a well-formatted hex string.  */
  bv = hex2bin ("abcd01");
  SELF_CHECK (bv.size () == 3);
  SELF_CHECK (bv[0] == 0xab);
  SELF_CHECK (bv[1] == 0xcd);
  SELF_CHECK (bv[2] == 0x01);

  /* Test an odd-length hex string.  */
  bv = hex2bin ("0123c");
  SELF_CHECK (bv.size () == 2);
  SELF_CHECK (bv[0] == 0x01);
  SELF_CHECK (bv[1] == 0x23);
}

static void test_hex2str ()
{
  SELF_CHECK (hex2str ("666f6f") == "foo");
  SELF_CHECK (hex2str ("666f6fa") == "foo");
  SELF_CHECK (hex2str ("666f6f", 2) == "fo");
  SELF_CHECK (hex2str ("666", 2) == "f");
  SELF_CHECK (hex2str ("666", 6) == "f");
  SELF_CHECK (hex2str ("") == "");
}

} /* namespace rsp_low */
} /* namespace selftests */

void _initialize_rsp_low_selftests ();
void
_initialize_rsp_low_selftests ()
{
  selftests::register_test ("hex2bin_byte_vector",
			    selftests::rsp_low::test_hex2bin_byte_vector);
  selftests::register_test ("hex2str",
			    selftests::rsp_low::test_hex2str);
}
