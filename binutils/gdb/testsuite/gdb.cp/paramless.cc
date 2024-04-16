/* Test case for template breakpoint test.

   Copyright 2022-2024 Free Software Foundation, Inc.

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

template<typename T>
struct outer
{
  template<typename Q = int>
  int fn (int x)
  {
    return x + 23;
  }
};

template<typename T = int>
int toplev (int y)
{
  return y;
}

outer<int> outer1;
outer<char> outer2;

int main ()
{
  int x1 = outer1.fn (0);
  int x2 = outer2.fn<short> (-46);
  int x3 = toplev<char> (0);
  int x4 = toplev (0);
  return x1 + x2;
}
