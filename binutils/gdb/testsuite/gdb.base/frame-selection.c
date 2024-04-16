/* Copyright 2018-2024 Free Software Foundation, Inc.

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

int
frame_2 (void)
{
  return 0;
}

int
frame_1 (void)
{
  return frame_2 ();
}

int
recursive (int arg)
{
  int v;

  if (arg < 2)
    v = recursive (arg + 1);
  else
    v = frame_2 ();

  return v;
}

int
main (void)
{
  int i, j;

  i = frame_1 ();
  j = recursive (0);

  return i + j;
}
