/* This test script is part of GDB, the GNU debugger.

   Copyright 2006-2024 Free Software Foundation, Inc.

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

#include <stdarg.h>

int func(int x)
{
   return x;
}

struct Foo {
  Foo() : x_(1) { }
  int func() const { return x_; }
 private:
  int x_;
};

typedef Foo *FooHandle;

extern "C" {
  int foo(int);
}

int sum_vararg_int (int count, ...)
{
  va_list va;
  int sum = 0;

  va_start (va, count);
  for (int i = 0; i < count; i++)
    sum += va_arg (va, int);
  va_end (va);

  return sum;
}

int vararg_func (int a, ...)
{
  return 1;
}

int vararg_func (int a, int b, ...)
{
  return 2;
}

int main()
{
    Foo f;
    Foo *pf = &f;
    Foo* &rf = pf;
    FooHandle handle = pf;
    rf->func(); /* set breakpoint here */
    foo(0);
    sum_vararg_int (1, 5);
    return func(0);
}
