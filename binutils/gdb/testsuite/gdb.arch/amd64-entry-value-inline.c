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

static volatile int v;

static __attribute__((noinline, noclone)) void
fn1 (int x)
{
  v++;
}

static int
fn2 (int x, int y)
{
  if (y)
    {
      fn1 (x);
      y = -2 + x;	/* break-here */
      y = y * y * y + y;
      fn1 (x + y);
    }
  return x;
}

__attribute__((noinline, noclone)) int
fn3 (int x, int y)
{
  return fn2 (x, y);
}

int
main ()
{
  fn3 (6, 25);
  return 0;
}
