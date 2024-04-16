/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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

/* Some overloaded functions to test breakpoints with multiple
   locations.  */

class foo
{
public:
  static void overload (void);
  static void overload (char);
  static void overload (int);
  static void overload (double);
};

void
foo::overload ()
{
}

void
foo::overload (char arg)
{
}

void
foo::overload (int arg)
{
}

void
foo::overload (double arg)
{
}

void
marker ()
{
}

int
main ()
{
  foo::overload ();
  foo::overload (111);
  foo::overload ('h');
  foo::overload (3.14);

  marker ();

  return 0;
}
