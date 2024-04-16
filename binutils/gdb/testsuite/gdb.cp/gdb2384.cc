/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

#include "gdb2384-base.h"

class derived1 : public base
{
 public:
  derived1 (int);
};

derived1::derived1 (int _x)
  : base (_x)
{
}

class derived2 : public derived
{
 public:
  derived2 (int);
};

derived2::derived2 (int _x)
  : derived (_x)
{
}

int g;

int
main ()
{
  derived1 d1 (42);
  derived2 d2 (24);
  g = d1.meth (); // First breakpoint
  g = d2.meth (); // Second breakpoint
  return 0;
}
