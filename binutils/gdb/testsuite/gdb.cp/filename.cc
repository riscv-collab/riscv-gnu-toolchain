/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

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

#include "includefile"

class C {
public:
  int includefile[1];

  C() {
    includefile[0] = 23;
  }

  void m() {
    /* stop inside C */
  }
};

class D {
public:
  int includefile();

  void m() {
    /* stop inside D */
  }
};

int D::includefile() {
  return 24;
}

int main() {
  C c;
  C* pc = &c;
  c.m();

  D d;
  D* pd = &d;
  d.m();

  return 0; /* stop outside */
}
