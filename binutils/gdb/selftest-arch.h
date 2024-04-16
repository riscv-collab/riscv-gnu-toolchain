/* GDB self-test for each gdbarch.
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

#ifndef SELFTEST_ARCH_H
#define SELFTEST_ARCH_H

typedef void self_test_foreach_arch_function (struct gdbarch *);

namespace selftests
{

/* Register a selftest running FUNCTION for each arch supported by GDB. */

extern void
  register_test_foreach_arch (const std::string &name,
			      self_test_foreach_arch_function *function);
}

#endif /* SELFTEST_ARCH_H */
