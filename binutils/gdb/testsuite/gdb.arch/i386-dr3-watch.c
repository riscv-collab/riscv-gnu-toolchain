/* Copyright 2011-2024 Free Software Foundation, Inc.

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

int i1;
char gap1[32];

int i2;
char gap2[32];

int i3;
char gap3[32];

int i4;

void
trigger (void)
{
  i1 = 1;
  i2 = 2;
  i3 = 3;
  i4 = 4;
}

int
main ()
{
  trigger ();
  return 0;
}
