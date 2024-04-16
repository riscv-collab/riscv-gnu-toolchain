/* Copyright 2012-2024 Free Software Foundation, Inc.

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

typedef float V __attribute__((vector_size(16)));

static V
foo (V a, V b)
{
  return a + b * a;
}

static __attribute__((noinline, noclone)) int
f (void)
{
  volatile V a = { 1, 2, 3, 4 };
  volatile V b;

  b = foo (a, a);
  return b[0];
}

static __attribute__((noinline, noclone)) int
test_g0 (void)
{
  return f ();
}

static __attribute__((noinline, noclone)) int
test_g1 (int p1)
{
  return f ();
}

static __attribute__((noinline, noclone)) int
test_g2 (int p1, int p2)
{
  return f ();
}

static __attribute__((noinline, noclone)) int
test_g3 (int p1, int p2, int p3)
{
  return f ();
}

static __attribute__((noinline, noclone)) int
test_g4 (int p1, int p2, int p3, int p4)
{
  return f ();
}

int
main (void)
{
  return (test_g0 () + test_g1 (1) + test_g2 (1, 2) + test_g3 (1, 2, 3)
	  + test_g4 (1, 2, 3, 4);
}
