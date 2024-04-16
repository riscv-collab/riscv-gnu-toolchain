/* Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

extern void opaque_routine (void);

int dyn_arr_size = 4;

int
main (void)
{
  int i;
  int my_table_size = dyn_arr_size - 1;
  int my_table [my_table_size];

  for (i = 0; i < my_table_size; i++)
    my_table[i] = i;

  opaque_routine ();
  return 0;
}
