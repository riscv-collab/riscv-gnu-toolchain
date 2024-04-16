/* Copyright 2014-2024 Free Software Foundation, Inc.

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

/* This C file provides some global variables laid out in a way
   that mimicks what the GNAT Ada compiler calls "fat pointers".
   These fat pointers are the memory representation used by
   the compiler to handle dynamic arrays.

   Debugging information on how to decode that data into an array
   will be generated separately by the testcase using that file.  */

struct fat_pointer
{
  int *data;
  int *bounds;
};

int table_1_data[] = {1, 2, 3};
int table_1_bounds[] = {1, 3};
struct fat_pointer table_1 = {table_1_data, table_1_bounds};

int table_2_data[] = {5, 8, 13, 21, 34};
int table_2_bounds[] = {2, 6};
struct fat_pointer table_2 = {table_2_data, table_2_bounds};

int
main (void)
{
  table_1.bounds[1] = 2;
  table_2.bounds[1] = 3;
  return 0;
}
