/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

/*
  Test file to be compiled with -fstack-check, for testing "bt" against
  different stack checking prologue sequences.
 */

int i = 0;

void
small_frame ()
{
  i++; /* set breakpoint here */
}

void medium_frame ()
{
  char S [16384];
  small_frame ();
}

void big_frame ()
{
  char S [524188];
  small_frame ();
}

int
main ()
{
  small_frame ();
  medium_frame ();
  big_frame ();
  return 0;
}
