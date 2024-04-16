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

/* A copy of some classes in derivation.cc so that we can test symbol lookup
   in other CUs.  */

class A2 {
public:
    typedef int value_type;
    value_type a;

    A2()
    {
        a=1;
    }
};

class D2 : public A2 {
public:
    value_type d;

    D2()
    {
        d=7;
    }
};

void
foo2 ()
{
  D2 d2_instance;
  d2_instance.a = 42;
  d2_instance.d = 43;
}
