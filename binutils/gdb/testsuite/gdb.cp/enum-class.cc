/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

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

enum class E1 {
  HI = 7, THERE
};

enum class E2 {
  HI = 23, THERE
};

// overload1(E1::HI) is ok.
// overload1(77) is ok.
int overload1 (int v) { return 0; }
int overload1 (E1 v) { return static_cast<int> (v); }
int overload1 (E2 v) { return - static_cast<int> (v); }

// overload2(E1::HI) is ok.
// overload1(77) fails.
int overload2 (E1 v) { return static_cast<int> (v); }
int overload2 (E2 v) { return - static_cast<int> (v); }

// overload3(E1::HI) fails.
// overload1(77) is ok.
int overload3 (int v) { return 0; }
int overload3 (E2 v) { return static_cast<int> (v); }

int
main (void)
{
  return 0;
}
