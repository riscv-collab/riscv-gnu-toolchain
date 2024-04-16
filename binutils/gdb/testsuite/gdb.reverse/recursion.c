/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023-2024 Free Software Foundation, Inc.

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

/* Test GDB's ability to handle recursive functions when executing
   in reverse.  */

/* The recursive foo call must be the first line of the recursive
   function, to test that we're not stepping too much and skipping
   multiple calls when we should skip only one.  */
int
foo (int x)
{
  if (x) return foo (x-1);
  return 0;
}

int
bar (int x)
{
  int r = foo (x);
  return 2*r;
}

int
main ()
{
  int i = bar (5);
  int j = foo (5);
  return 0;			/* END OF MAIN */
}
