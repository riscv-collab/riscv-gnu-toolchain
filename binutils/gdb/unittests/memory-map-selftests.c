/* Self tests for memory-map for GDB, the GNU debugger.

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
#include "memory-map.h"

#if defined(HAVE_LIBEXPAT)

namespace selftests {
namespace memory_map_tests {

/* A simple valid test input for parse_memory_map.  */

static const char valid_mem_map[] = R"(<?xml version="1.0"?>
<!DOCTYPE memory-map
	  PUBLIC "+//IDN gnu.org//DTD GDB Memory Map V1.0//EN"
		 "http://sourceware.org/gdb/gdb-memory-map.dtd">
<memory-map>
  <memory type="ram" start="0" length="4096" />
  <memory type="rom" start="65536" length="256" />
  <memory type="flash" start="131072" length="65536">
    <property name="blocksize">1024</property>
  </memory>
</memory-map>
)";

/* Validate memory region R against some expected values (the other
   parameters).  */

static void
check_mem_region (const mem_region &r, CORE_ADDR lo, CORE_ADDR hi,
		  mem_access_mode mode, int blocksize)
{
  SELF_CHECK (r.lo == lo);
  SELF_CHECK (r.hi == hi);
  SELF_CHECK (r.enabled_p);

  SELF_CHECK (r.attrib.mode == mode);
  SELF_CHECK (r.attrib.blocksize == blocksize);

}

/* Test the parse_memory_map function.  */

static void
parse_memory_map_tests ()
{
  std::vector<mem_region> regions = parse_memory_map (valid_mem_map);

  SELF_CHECK (regions.size () == 3);

  check_mem_region (regions[0], 0, 0 + 4096, MEM_RW, -1);
  check_mem_region (regions[1], 65536, 65536 + 256, MEM_RO, -1);
  check_mem_region (regions[2], 131072, 131072 + 65536, MEM_FLASH, 1024);
}

} /* namespace memory_map_tests */
} /* namespace selftests */

#endif /* HAVE_LIBEXPAT */

void _initialize_memory_map_selftests ();
void
_initialize_memory_map_selftests ()
{
#if defined(HAVE_LIBEXPAT)
  selftests::register_test
    ("parse_memory_map",
     selftests::memory_map_tests::parse_memory_map_tests);
#endif
}
