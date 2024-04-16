/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

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

#include <stdlib.h>

int bar (void);
int baz (int);
void skip1_test_skip_file_and_function (void);
void test_skip_file_and_function (void);

__attribute__((__always_inline__)) static inline int
foo (void)
{
  return bar ();
}

int
main ()
{
  volatile int x;

  /* step immediately into the inlined code */
  baz (foo ());

  /* step first over non-inline code, this involves a different code path */
  x = 0; x = baz (foo ());

  test_skip_file_and_function ();

  return 0;
}

static void
test_skip (void)
{
}

static void
end_test_skip_file_and_function (void)
{
  abort ();
}

void
test_skip_file_and_function (void)
{
  test_skip ();
  skip1_test_skip_file_and_function ();
  end_test_skip_file_and_function ();
}
