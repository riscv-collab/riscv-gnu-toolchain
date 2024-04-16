/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

/* 
   Purpose of this test:  to test breakpoints on consecutive instructions.
*/

int a[7] = {1, 2, 3, 4, 5, 6, 7};

/* assert: first line of foo has more than one instruction. */
int foo ()
{
  return a[0] + a[1] + a[2] + a[3] + a[4] + a[5] + a[6];
}

int
main()
{
  foo ();
} /* end of main */

