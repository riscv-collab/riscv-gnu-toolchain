/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2024 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <stdlib.h>

long double ld;
__float128 f128;

// Test largest IEEE-128 value.  This has to be supported since the
// __float128 data type by definition is encoded as IEEE-128.
__float128 large128 = 1.18973149535723176508575932662800702e+4932q;

int main()
{
  ld = 1.375l;
  f128 = 2.375q;

  return 0;
}
