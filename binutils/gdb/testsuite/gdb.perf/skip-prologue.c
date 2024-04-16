/* This testcase is part of GDB, the GNU debugger.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

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

/* GDB will analyze the prologue of functions f1 and f2.  */

struct foo
{
  int a[32];
};

static int
f1 (void)
{
  return 0;
}

static double
f2 (int a, long long b, double c, struct foo f)
{
  f.a[0] = a + (int) b + c;

  return c + 0.2;
}

int
main (void)
{
  struct foo f;

  f1 ();
  f2 (0, 0, 0.1, f);

  return 0;
}
