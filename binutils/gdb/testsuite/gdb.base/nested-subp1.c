/* This test program is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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
foo (int i1)
{
  int
  nested (int i2)
  {
    /* Here with i1 and i2, we can test that GDB can fetch both a local and a
       non-local variable in the most simple nested function situation: the
       parent block instance is accessible as the directly upper frame.  */
    return i1 * i2; /* STOP */
  }

  return nested (i1 + 1);
}

int
main ()
{
  return !foo (1);
}
