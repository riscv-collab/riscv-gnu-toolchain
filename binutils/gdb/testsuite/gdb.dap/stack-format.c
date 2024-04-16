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

int function (int x, char y)
{
  return y - x - 1;			/* BREAK */
}

int z_1 (int x, char y)
{
  return function (x, y);
}

int z_2 (int x, char y)
{
  return z_1 (x, y);
}

int z_3 (int x, char y)
{
  return z_2 (x, y);
}

int main ()
{
  return z_3 (64, 'A');
}
