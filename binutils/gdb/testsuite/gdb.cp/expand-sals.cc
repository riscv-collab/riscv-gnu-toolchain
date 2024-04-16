/* This testcase is part of GDB, the GNU debugger.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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
func ()
{
  return 42;	/* func-line */
}

volatile int global_x;

class A
{
public:
  A ()
    {
      global_x = func ();	/* caller-line */
    }
};

/* class B is here just to make the `func' calling line above having multiple
   instances - multiple locations.  Template cannot be used as its instances
   would have different function names which get discarded by GDB
   expand_line_sal_maybe.  */

class B : public A
{
};

int
main (void)
{
  A a;
  B b;

  return 0;
}
