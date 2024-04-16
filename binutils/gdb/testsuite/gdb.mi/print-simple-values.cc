/* This test case is part of GDB, the GNU debugger.

   Copyright 2022-2024 Free Software Foundation, Inc.

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

/* Test program for PRINT_SIMPLE_VALUES.

   In the function f:

   * The arguments i, ir, and irr are ints or references to ints, which
     must be printed by PRINT_SIMPLE_VALUES.

   * The arguments a, s, and u are non-scalar values, which must not be
     printed by PRINT_SIMPLE_VALUES.

   * The arguments ar, arr, sr, srr, ur, and urr are references to
     non-scalar values, which must not be printed by
     PRINT_SIMPLE_VALUES.  */

struct s
{
  int v;
};

union u
{
  int v;
};

int
f (int i, int &ir, int &&irr,
   int a[1], int (&ar)[1], int (&&arr)[1],
   struct s s, struct s &sr, struct s &&srr,
   union u u, union u &ur, union u &&urr)
{
  return (i + ir + irr
	  + a[0] + ar[0] + arr[0]
	  + s.v + sr.v + srr.v
	  + u.v + ur.v + urr.v);
}

int
main (void)
{
  int i = 1, j = 2;
  int a[1] = { 4 }, b[1] = { 5 };
  struct s s = { 7 }, t = { 8 };
  union u u = { 10 }, v = { 11 };
  return f (i, j, 3, a, b, { 6 }, s, t, { 9 }, u, v, { 12 });
}
