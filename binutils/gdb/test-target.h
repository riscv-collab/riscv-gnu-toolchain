/* A mock process_stratum target_ops

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

#ifndef TEST_TARGET_H
#define TEST_TARGET_H

#include "process-stratum-target.h"

#if GDB_SELF_TEST
namespace selftests {

/* A mock process_stratum target_ops that doesn't read/write registers
   anywhere.  */

class test_target_ops : public process_stratum_target
{
public:
  test_target_ops () = default;

  const target_info &info () const override;

  bool has_registers () override
  {
    return true;
  }

  bool has_stack () override
  {
    return true;
  }

  bool has_memory () override
  {
    return true;
  }

  void prepare_to_store (regcache *regs) override
  {
  }

  void store_registers (regcache *regs, int regno) override
  {
  }
};

} // namespace selftests
#endif /* GDB_SELF_TEST */

#endif /* !defined (TEST_TARGET_H) */
