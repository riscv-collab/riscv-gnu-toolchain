/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

/* Test rvalue references in python.  Based on py-value-cc.cc.  */

#include <utility>

class A
{
public:
  int operator+ (const int a1);

 public:
  int a;
};

int
A::operator+ (const int a1)
{
  return a + a1;
}

class B : public A
{
 public:
  char a;
};

typedef int *int_ptr;

int
main ()
{
  int val = 10;
  int &&int_rref = std::move (val);
  int_ptr ptr = &val;
  int_ptr &&int_ptr_rref = std::move (ptr);

  B b;
  b.a = 'b';
  (&b)->A::a = 100;
  B &&b_rref = std::move (b);

  return 0; /* Break here.  */
}
