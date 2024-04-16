/* This testcase is part of GDB, the GNU debugger.

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
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

/* This test is compiled multiple times with FUNCTION_NAME defined to
   different strings, this means we should (hopefully) get the same code
   layout in memory, but with different strings for the function name.  */

int
FUNCTION_NAME (void)
{
  return 0;
}

int
main (void)
{
  return FUNCTION_NAME ();
}
