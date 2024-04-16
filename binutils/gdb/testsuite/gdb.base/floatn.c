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

_Float32 f32;
_Float64 f64;
_Float128 f128;
_Float32x f32x;
_Float64x f64x;

_Complex _Float32 c32;
_Complex _Float64 c64;
_Complex _Float128 c128;
_Complex _Float32x c32x;
_Complex _Float64x c64x;

int main()
{
  f32 = 1.5f32;
  f64 = 2.25f64;
  f128 = 3.375f128;
  f32x = 10.5f32x;
  f64x = 20.25f64x;

  c32 = 1.5f32 + 1.0if;
  c64 = 2.25f64 + 1.0if;
  c128 = 3.375f128 + 1.0if;
  c32x = 10.5f32x + 1.0if;
  c64x = 20.25f64x + 1.0if;

  return 0;
}
