/* This testcase is part of GDB, the GNU debugger.

   Copyright 2003-2024 Free Software Foundation, Inc.

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

struct B
{
  static int b;
};

int B::b = 23;

struct C : public B
{
  static int x;

  struct inner
  {
    static int z;
  };

  int y;

  C ()
  {
    // First breakpoint here
    y = x + inner::z;
  }

  int m ()
  {
    // Second breakpoint here
    return x - y;
  }
};

int C::x = 23;
int C::inner::z = 0;

template<typename T>
struct Templ
{
  static int y;

  int m()
  {
    // Third breakpoint here
    return Templ::y;
  }
};

template<typename T> int Templ<T>::y = 23;

int main ()
{
  C c;
  Templ<int> t;

  return c.m() + t.m();
}
