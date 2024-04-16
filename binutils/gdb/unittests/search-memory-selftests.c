/* Self tests for simple_search_memory for GDB, the GNU debugger.

   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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
#include "gdbsupport/search.h"

namespace selftests {
namespace search_memory_tests {

static void
run_tests ()
{
  size_t size = 2 * SEARCH_CHUNK_SIZE + 1;

  std::vector<gdb_byte> data (size);
  data[size - 1] = 'x';

  bool read_fully = false;
  bool read_off_end = false;
  auto read_memory = [&] (CORE_ADDR from, gdb_byte *out, size_t len)
    {
      if (from + len > data.size ())
	read_off_end = true;
      else
	memcpy (out, &data[from], len);
      if (from + len == data.size ())
	read_fully = true;
      return true;
    };

  gdb_byte pattern = 'x';

  CORE_ADDR addr = 0;
  int result = simple_search_memory (read_memory, 0, data.size (),
				     &pattern, 1, &addr);
  /* In this case we don't care if read_fully was set or not.  */
  SELF_CHECK (result == 1);
  SELF_CHECK (!read_off_end);
  SELF_CHECK (addr == size - 1);

  addr = 0;
  read_fully = false;
  read_off_end = false;
  pattern = 'q';
  result = simple_search_memory (read_memory, 0, data.size (),
				 &pattern, 1, &addr);
  SELF_CHECK (result == 0);
  SELF_CHECK (!read_off_end);
  SELF_CHECK (read_fully);
  SELF_CHECK (addr == 0);

  /* Setup from PR gdb/17756.  */
  size = 0x7bb00;
  data = std::vector<gdb_byte> (size);
  const CORE_ADDR base_addr = 0x08370000;
  const gdb_byte wpattern[] = { 0x90, 0x8b, 0x98, 0x8 };
  const CORE_ADDR found_addr = 0x837bac8;
  memcpy (&data[found_addr - base_addr], wpattern, sizeof (wpattern));

  auto read_memory_2 = [&] (CORE_ADDR from, gdb_byte *out, size_t len)
    {
      memcpy (out, &data[from - base_addr], len);
      return true;
    };

  result = simple_search_memory (read_memory_2, base_addr, data.size (),
				 wpattern, sizeof (wpattern), &addr);
  SELF_CHECK (result == 1);
  SELF_CHECK (addr == found_addr);
}

} /* namespace search_memory_tests */
} /* namespace selftests */


void _initialize_search_memory_selftests ();
void
_initialize_search_memory_selftests ()
{
  selftests::register_test ("search_memory",
			    selftests::search_memory_tests::run_tests);
}
