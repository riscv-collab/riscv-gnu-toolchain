/* Self tests for gdb::unique_xmalloc_ptr<char>.

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
#include "selftest-arch.h"
#include "gdbsupport/gdb_unique_ptr.h"

namespace selftests {
namespace unpack {

static void
unique_xmalloc_ptr_char ()
{
  gdb::unique_xmalloc_ptr<char> a = make_unique_xstrdup ("abc");
  gdb::unique_xmalloc_ptr<char> b = make_unique_xstrndup ("defghi", 3);

  SELF_CHECK (strcmp (a.get (), "abc") == 0);
  SELF_CHECK (strcmp (b.get (), "def") == 0);

  std::string str = "xxx";

  /* Check the operator+= overload.  */
  str += a;
  SELF_CHECK (str == "xxxabc");

  /* Check the operator+ overload.  */
  str = str + b;
  SELF_CHECK (str == "xxxabcdef");
}

}
}

void _initialize_unique_xmalloc_ptr_char ();
void
_initialize_unique_xmalloc_ptr_char ()
{
  selftests::register_test ("unique_xmalloc_ptr_char",
			    selftests::unpack::unique_xmalloc_ptr_char);
}
