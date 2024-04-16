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

struct some_struct
{
  int x;
  int y;

  static int z;
};

int some_struct::z = 37;

void
func ()
{
  some_struct aggregate { 91, 87 };

  int value = 23;

  int *ptr = &value;
  int &ref = value;

  return;			/* BREAK */
}

int
main (int argc, char *argv[])
{
  func ();
  return 0;
}
