/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2024 Free Software Foundation, Inc.

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

int next (int i);

int
main (void)
{
  int result = -1;

  result = next (result);
  return result;
}

/* The following function's implementation starts by including a file
   (break-include.inc) which contains a copyright header followed by
   a single C statement.  When we place a breakpoint on the line where
   the function name is declared, we expect GDB to skip the function's
   prologue, and insert the breakpoint on the first line of "user" code
   for that function, which we have set up to be that single statement
   break-include.inc provides.

   The purpose of this testcase is to verify that, when we insert
   that breakpoint, GDB reports the location as being in that include
   file, but also using the correct line number inside that include
   file -- NOT the line number we originally used to insert the
   breakpoint, nor the location where the file is included from.
   In order to verify that GDB shows the right line number, we must
   be careful that this first statement located in break-include.inc
   and our function are not on the same line number.  Otherwise,
   we could potentially have a false PASS.

   This is why we implement the following function as far away
   from the start of this file as possible, as we know that
   break-include.inc is a fairly short file (copyright header
   and single statement only).  */

int
next (int i)  /* break here */
{
#include "break-include.inc"
  return i;
}
