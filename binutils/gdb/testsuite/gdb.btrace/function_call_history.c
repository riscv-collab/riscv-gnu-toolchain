/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <christian.himpel@intel.com>

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

int
inc (int i)
{
  return i+1;
}

int
fib (int n)
{
  if (n <= 1)
    return n;

  return fib(n-2) + fib(n-1);
}

int
main (void)
{
  int i, j;

  for (i = 0; i < 10; i++)
    j += inc(i);

  j += fib(3); /* bp.1 */
  return j; /* bp.2 */
}
