/* Copyright 2023-2024 Free Software Foundation, Inc.
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

enum enum1 {};

enum class enum2 : unsigned char {};

void
breakpt (enum1 arg1, enum2 arg2)
{
  /* Nothing.  */
}

int
main ()
{
  breakpt ((enum1) 8, (enum2) 4);

  return 0;
}
