/* Copyright 2023-2024 Free Software Foundation, Inc.

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

int main ()
{
  typedef struct dei_struct
  {
    int x;
    int more[5];
  } dei_type;

  dei_type dei = { 2, { 3, 5, 7, 11, 13 } };

  static int scalar = 23;

  {
    const char *inner = "inner block";

    /* Make sure to use 'scalar'.  */
    return scalar - 23;			/* BREAK */
  }
}
