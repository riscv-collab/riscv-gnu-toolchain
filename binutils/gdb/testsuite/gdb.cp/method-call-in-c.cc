/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

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

struct baz_type
{
  int a = 0;
  int b = 1;
  int c = 2;
};

struct foo_type
{
  int func (baz_type b, float f)
  {
    return var++;
  }

  foo_type &operator+= (const baz_type &rhs)
  {
    var += (rhs.a + rhs.b + rhs.c);
    return *this;
  }

  static int static_method (float f, baz_type b)
  {
    return b.a + b.b + b.c + (int) f;
  }

  int var = 120;
};

volatile int global_var;

int
main (void)
{
  baz_type b = {};
  float f = 1.0;

  foo_type foo;

  foo += b;

  global_var = foo.static_method (f, b);

  return foo.func (b, f);	/* Break here.  */
}
