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
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

struct mytype
{
  char *x;
};

void
rec (int i)
{
  if (i <= 0)
    return;
  rec (i-1);
}

int
f ()
{
  rec (100);
  return 2;
}

void
g (struct mytype mt, int depth)
{
  if (depth <= 0)
    return; /* TAG: final frame */
  g (mt, depth - 1); /* TAG: first frame */
}

int
main ()
{
  struct mytype mt;
  mt.x = "hello world";
  g (mt, 10); /* TAG: outside the frame */
  return 0;
}
