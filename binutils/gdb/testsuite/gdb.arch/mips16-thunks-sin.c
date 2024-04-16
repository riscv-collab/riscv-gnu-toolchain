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

double sinmips16 (double d);
long lsinmips16 (double d);

extern long i;

double
sinhelper (double d)
{
  i++;
  d = sin (d);
  d = sinfrob16 (d);
  d = sinfrob (d);
  d = sinmips16 (d);
  i++;
  return d;
}

long
lsinhelper (double d)
{
  long l;

  i++;
  d = sin (d);
  d = sinblah (d);
  d = sinblah16 (d);
  l = lsinmips16 (d);
  i++;
  return l;
}
