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

int table_1_data[] = {1, 3, 1, 2, 3};
int *table_1_ptr = &table_1_data[2];

int table_2_data[] = {2, 6, 5, 8, 13, 21, 34};
int *table_2_ptr = &table_2_data[2];

int
main (void)
{
  *table_1_ptr = 2;
  *table_2_ptr = 3;
  return 0;
}
