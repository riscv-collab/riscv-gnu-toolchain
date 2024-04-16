/* This testcase is part of GDB, the GNU debugger.

   Copyright 2018-2024 Free Software Foundation, Inc.

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

static int
a_very_unique_name (int a, int b)
{
  return a + b;
}

static double
a_very_unique_name (double a, double b)
{
  return a + b;
}

int
main (void)
{
  int i = a_very_unique_name (3, 4);
  double d = a_very_unique_name (3.0, 4.0);
  return 1;
}
