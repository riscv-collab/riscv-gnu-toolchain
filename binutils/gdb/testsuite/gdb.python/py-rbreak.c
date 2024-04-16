/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

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
func1 ()
{
  return 1;
}

int
func2 ()
{
  return 2;
}

int
func3 ()
{
  return 3;
}

int
func4 ()
{
  return 4;
}

int
func5 ()
{
  return 5;
}

void
func6 ()
{
  return;
}

void
outside_scope ()
{
  return;
}

int
main()
{
  func1 (); /* Break func1.  */
  func2 ();
  func3 ();
  func4 ();
  func5 ();
  func6 ();
  outside_scope ();

  return 0;
}
