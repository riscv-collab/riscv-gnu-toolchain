/* Self tests for unpack_field_as_long

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
#include "selftest-arch.h"
#include "value.h"
#include "gdbtypes.h"
#include "arch-utils.h"

namespace selftests {
namespace unpack {

static void
unpack_field_as_long_tests (struct gdbarch *arch)
{
  gdb_byte buffer[8];
  const struct builtin_type *bt = builtin_type (arch);
  struct type *struct_type = arch_composite_type (arch, "<<selftest>>",
						  TYPE_CODE_STRUCT);

  append_composite_type_field (struct_type, "field0", bt->builtin_int8);
  append_composite_type_field_aligned (struct_type, "field1",
				       bt->builtin_uint32, 4);

  memset (buffer, 0, sizeof (buffer));
  buffer[0] = 255;
  if (gdbarch_byte_order (arch) == BFD_ENDIAN_BIG)
    buffer[7] = 23;
  else
    buffer[4] = 23;

  SELF_CHECK (unpack_field_as_long (struct_type, buffer, 0) == -1);
  SELF_CHECK (unpack_field_as_long (struct_type, buffer, 1) == 23);
}

}
}

void _initialize_unpack_selftests ();
void
_initialize_unpack_selftests ()
{
  selftests::register_test_foreach_arch
    ("unpack_field_as_long", selftests::unpack::unpack_field_as_long_tests);
}
