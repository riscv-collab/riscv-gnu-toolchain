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

class A {
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

union U {
  int a;
  char c;
};

class B : public A {
 public:
  char a;
};

struct X
{
  union { int x; char y; };
  union { int a; char b; };
};

union UU
{
  union { int x; char y; };
  union { int a; char b; };
};

typedef B Btd;
typedef int *int_ptr;
typedef X Xtd;

int
func (const A &a)
{
  int val = 10;
  int &int_ref = val;
  int_ptr ptr = &val;
  int_ptr &int_ptr_ref = ptr;

  B b;
  B b1;

  b.a = 'a';
  b.A::a = 10;

  B *b_obj = &b1;
  b_obj->a = 'b';
  b_obj->A::a = 100;

  B &b_ref = b1;
  Btd &b_td = b1;

  U u;
  u.a = 0x63636363;

  X x;
  x.x = 101;
  x.a = 102;

  UU uu;
  uu.x = 1000;

  X *x_ptr = &x;
  Xtd *xtd = &x;

  return 0; /* Break here.  */
}

int
main ()
{
  A obj;

  obj.a = 5;

  return func (obj);
}
