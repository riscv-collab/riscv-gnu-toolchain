/* Copyright 2022-2024 Free Software Foundation, Inc.

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

/* A function that segfaults (assuming that reads of address zero are
   prohibited), this is used from within a breakpoint condition.  */
int
func_segfault ()
{
  volatile int *p = 0;
  return *p;	/* Segfault here.  */
}

/* A function in which we will place a breakpoint.  This function is itself
   then used from within a breakpoint condition.  */
int
func_bp ()
{
  int res = 0;	/* Second breakpoint.  */
  return res;
}

int
foo ()
{
  return 0;	/* First breakpoint.  */
}

int
main ()
{
  int res = foo ();

  return res;
}
