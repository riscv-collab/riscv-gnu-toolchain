/* Self tests for packed for GDB, the GNU debugger.

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
#include "gdbsupport/packed.h"

namespace selftests {
namespace packed_tests {

enum test_enum
{
  TE_A = 1,
  TE_B = 2,
  TE_C = 3,
  TE_D = 4,
};

static_assert (sizeof (packed<test_enum, 1>) == 1);
static_assert (sizeof (packed<test_enum, 2>) == 2);
static_assert (sizeof (packed<test_enum, 3>) == 3);
static_assert (sizeof (packed<test_enum, 4>) == 4);

static_assert (alignof (packed<test_enum, 1>) == 1);
static_assert (alignof (packed<test_enum, 2>) == 1);
static_assert (alignof (packed<test_enum, 3>) == 1);
static_assert (alignof (packed<test_enum, 4>) == 1);

/* Triviality checks.  */
#define CHECK_TRAIT(TRAIT)			\
  static_assert (std::TRAIT<packed<test_enum, 1>>::value, "")

#if HAVE_IS_TRIVIALLY_COPYABLE

CHECK_TRAIT (is_trivially_copyable);
CHECK_TRAIT (is_trivially_copy_constructible);
CHECK_TRAIT (is_trivially_move_constructible);
CHECK_TRAIT (is_trivially_copy_assignable);
CHECK_TRAIT (is_trivially_move_assignable);

#endif

#undef CHECK_TRAIT

/* Entry point.  */

static void
run_tests ()
{
  typedef packed<unsigned int, 2> packed_2;

  packed_2 p1;
  packed_2 p2 (0x0102);
  p1 = 0x0102;

  SELF_CHECK (p1 == p1);
  SELF_CHECK (p1 == p2);
  SELF_CHECK (p1 == 0x0102);
  SELF_CHECK (0x0102 == p1);

  SELF_CHECK (p1 != 0);
  SELF_CHECK (0 != p1);

  SELF_CHECK (p1 != 0x0103);
  SELF_CHECK (0x0103 != p1);

  SELF_CHECK (p1 != 0x01020102);
  SELF_CHECK (0x01020102 != p1);

  SELF_CHECK (p1 != 0x01020000);
  SELF_CHECK (0x01020000 != p1);

  /* Check truncation.  */
  p1 = 0x030102;
  SELF_CHECK (p1 == 0x0102);
  SELF_CHECK (p1 != 0x030102);

  /* Check that the custom std::atomic/packed/T relational operators
     work as intended.  No need for fully comprehensive tests, as all
     operators are defined in the same way, via a macro.  We just want
     to make sure that we can compare atomic-wrapped packed, with
     packed, and with the packed underlying type.  */

  std::atomic<packed<unsigned int, 2>> atomic_packed_2 (0x0102);

  SELF_CHECK (atomic_packed_2 == atomic_packed_2);
  SELF_CHECK (atomic_packed_2 == p1);
  SELF_CHECK (p1 == atomic_packed_2);
  SELF_CHECK (atomic_packed_2 == 0x0102u);
  SELF_CHECK (0x0102u == atomic_packed_2);

  SELF_CHECK (atomic_packed_2 >= 0x0102u);
  SELF_CHECK (atomic_packed_2 <= 0x0102u);
  SELF_CHECK (atomic_packed_2 > 0u);
  SELF_CHECK (atomic_packed_2 < 0x0103u);
  SELF_CHECK (atomic_packed_2 >= 0u);
  SELF_CHECK (atomic_packed_2 <= 0x0102u);
  SELF_CHECK (!(atomic_packed_2 > 0x0102u));
  SELF_CHECK (!(atomic_packed_2 < 0x0102u));

  /* Check std::atomic<packed> truncation behaves the same as without
     std::atomic.  */
  atomic_packed_2 = 0x030102;
  SELF_CHECK (atomic_packed_2 == 0x0102u);
  SELF_CHECK (atomic_packed_2 != 0x030102u);
}

} /* namespace packed_tests */
} /* namespace selftests */

void _initialize_packed_selftests ();
void
_initialize_packed_selftests ()
{
  selftests::register_test ("packed", selftests::packed_tests::run_tests);
}
