/* This testcase is part of GDB, the GNU debugger.

   Copyright 2002-2024 Free Software Foundation, Inc.

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

/* Rvalue references casting tests, based on casts.cc.  */

#include <utility>

struct A
{
  int a;
  A (int aa): a (aa) {}
};

struct B: public A
{
  int b;
  B (int aa, int bb): A (aa), b (bb) {}
};


struct Alpha
{
  virtual void x () { }
};

struct Gamma
{
};

struct Derived : public Alpha
{
};

struct VirtuallyDerived : public virtual Alpha
{
};

struct DoublyDerived : public VirtuallyDerived,
		       public virtual Alpha,
		       public Gamma
{
};

int
main (int argc, char **argv)
{
  A *a = new B (42, 1729);
  B *b = (B *) a;
  A &ar = *b;
  B &br = (B&)ar;
  A &&arr = std::move (A (42));
  B &&brr = std::move (B (42, 1729));

  Derived derived;
  DoublyDerived doublyderived;

  Alpha *ad = &derived;
  Alpha *add = &doublyderived;

  return 0;  /* breakpoint spot: rvalue-ref-casts.exp: 1 */
}
