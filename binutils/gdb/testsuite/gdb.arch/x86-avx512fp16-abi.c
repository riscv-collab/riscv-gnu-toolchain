/* Test program for _Float16 parameters and return values.

   Copyright 2021-2024 Free Software Foundation, Inc.

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
#include <complex.h>

_Float16
square (_Float16 num) {
  return num * num; /* BP1.  */
}

_Float16 _Complex
plus (_Float16 _Complex num) {
  return num + (2.5 + 0.5I); /* BP2.  */
}

int
main ()
{
  _Float16 a = square (1.25);
  _Float16 _Complex b = 6.25 + I;
  _Float16 _Complex ret = plus (b); /* BP3.  */
  return 0;
}
