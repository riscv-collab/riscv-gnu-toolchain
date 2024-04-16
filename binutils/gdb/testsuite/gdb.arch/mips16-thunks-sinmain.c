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

double sinfrob (double d);
double sinfrob16 (double d);

double sinblah (double d);
double sinblah16 (double d);

double sinhelper (double);
long lsinhelper (double);

double (*sinfunc) (double) = sinfrob;
double (*sinfunc16) (double) = sinfrob16;

double f = 1.0;
long i = 1;

int
main (void)
{
  double d = f;
  long l = i;

  d = sinfrob16 (d);
  d = sinfrob (d);
  d = sinhelper (d);

  sinfunc = sinblah;
  sinfunc16 = sinblah16;

  d = sinblah (d);
  d = sinblah16 (d);
  l = lsinhelper (d);

  return l + i;
}
