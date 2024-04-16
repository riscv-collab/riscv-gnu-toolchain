/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

#include <math.h>

double sinfrob (double d);
double sinfrob16 (double d);

double sinblah (double d);
double sinblah16 (double d);

extern double (*sinfunc) (double);
extern double (*sinfunc16) (double);

extern long i;

double
sinmips16 (double d)
{
  i++;
  d = sin (d);
  d = sinfrob16 (d);
  d = sinfrob (d);
  d = sinfunc16 (d);
  d = sinfunc (d);
  i++;
  return d;
}

long
lsinmips16 (double d)
{
  union
    {
      double d;
      long l[2];
    }
  u;

  i++;
  d = sin (d);
  d = sinblah (d);
  d = sinblah16 (d);
  d = sinfunc (d);
  u.d = sinfunc16 (d);
  i++;
  return u.l[0] == 0 && u.l[1] == 0;
}
