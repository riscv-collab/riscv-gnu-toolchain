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

#include "defs.h"
#include "test-target.h"

#if GDB_SELF_TEST
namespace selftests {

static const target_info test_target_info = {
  "test",
  N_("unit tests target"),
  N_("You should never see this"),
};

const target_info &
test_target_ops::info () const
{
  return test_target_info;
}

} /* namespace selftests */
#endif /* GDB_SELF_TEST */
