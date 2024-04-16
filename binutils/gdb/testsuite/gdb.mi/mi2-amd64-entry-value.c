/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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

static void __attribute__((noinline, noclone))
e (int i, double j)
{
  v = 0;
}

static int __attribute__((noinline, noclone))
data (void)
{
  return 10;
}

static int __attribute__((noinline, noclone))
data2 (void)
{
  return 20;
}

static int __attribute__((noinline, noclone))
different (int val)
{
  val++;
  e (val, val);
asm ("breakhere_different:");
  return val;
}

static int __attribute__((noinline, noclone))
validity (int lost, int born)
{
  lost = data ();
  e (0, 0.0);
asm ("breakhere_validity:");
  return born;
}

static void __attribute__((noinline, noclone))
invalid (int inv)
{
  e (0, 0.0);
asm ("breakhere_invalid:");
}

int
main ()
{
  different (5);
  validity (5, data ());
  invalid (data2 ());
  return 0;
}
