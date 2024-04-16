/* Copyright 2015-2024 Free Software Foundation, Inc.

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

struct A
{
  A () : a_ (1) {}
  int do_it (int amount) { return a_ + amount; }

  int a_;
};

struct B
{
  B () : b_ (2) {}
  int do_it (int amount) { return b_ - amount; }

  int b_;
};

struct C
{
  C () : c_ (3) {}
  int do_it (int amount) { return c_ * amount; }

  int c_;
};

struct D : public A, B, C
{
  D () : d_ (4) {}

  int d_;
};

int
main ()
{
  D d;
  int var = 1234;

  var = d.A::do_it (1)
    + d.B::do_it (2)
    + d.C::do_it (3);		// break here

  return 0;
}
