/* This testcase is part of GDB, the GNU debugger.

   Copyright 2003-2024 Free Software Foundation, Inc.

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

class A
{
public:
  enum E {X,Y,Z};
};

class B1 : public A
{
};

class B2 : public A
{
};

class C : public B1, public B2
{
public:
  void test(E e);
};

void C::test(E e)
{
  if (e == X)  // breakpoint 1
    {
    }
}

namespace N
{
  class A
  {
  public:
    enum E {X, Y, Z};
  };

  class B1 {};
  class B2 : public A {};

  class C : public B1, public B2
  {
  public:
    void test (E e);
  };

  void
  C::test (E e)
  {
    if (e == X) // breakpoint 2
      {
      }
  }
}

int main()
{
  C c;
  c.test(A::X);

  N::C nc;
  nc.test (N::A::X);
  return 0;
}

