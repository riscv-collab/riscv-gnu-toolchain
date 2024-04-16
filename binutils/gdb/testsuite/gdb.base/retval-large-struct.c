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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

struct big_struct_t
{
  int int_array[5];
  double double_array[5];
  char char_array[5];
};

struct big_struct_t big_struct =
{
  {1, 2, 3, 4, 5},
  {3.25, 5.0, 6.25, 1.325, -1.95},
  "abcde"
};

struct big_struct_t return_large_struct (void)
{
  return big_struct;
}

int
main (int argc, char **argv)
{
  struct big_struct_t test_struct;

  test_struct = return_large_struct ();

  return 0;
}
