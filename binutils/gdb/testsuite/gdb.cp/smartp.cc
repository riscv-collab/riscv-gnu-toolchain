/* This test script is part of GDB, the GNU debugger.

   Copyright 1999-2024 Free Software Foundation, Inc.

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

class Type1{
  public:
  int foo(){
    return 11;
  }
};

class Type2{
  public:
  int foo(){
    return 22;
  }
};

class Type3{
  public:
  int foo(int){
    return 33;
  }
  int foo(char){
    return 44;
  }
};

class Type4 {
  public:
    int a;
    int b;
};

int foo (Type3, float)
{
  return 55;
}

class MyPointer{
  Type1 *p;
 public:
  MyPointer(Type1 *pointer){
    p = pointer;
  }

  Type1 *operator->(){
    return p;
  }
};

template <typename T> class SmartPointer{
  T *p;
 public:
  SmartPointer(T *pointer){
    p = pointer;
  }

  T *operator->(){
    return p;
  }
};


class A {
 public:
  int inta;
  int foo() { return 66; }
};

class B {
 public:
  A a;
  A* operator->(){
    return &a;
  }
};

class C {
 public:
  B b;
  B& operator->(){
    return b;
  }
};

class C2 {
 public:
  B b;
  B operator->(){
    return b;
  }
};

int main(){
  Type1 mt1;
  Type2 mt2;
  Type3 mt3;

  Type4 mt4;
  mt4.a = 11;
  mt4.b = 12;

  MyPointer mp(&mt1);
  Type1 *mtp = &mt1;

  SmartPointer<Type1> sp1(&mt1);
  SmartPointer<Type2> sp2(&mt2);
  SmartPointer<Type3> sp3(&mt3);
  SmartPointer<Type4> sp4(&mt4);

  mp->foo();
  mtp->foo();

  sp1->foo();
  sp2->foo();

  sp3->foo(1);
  sp3->foo('a');

  (void) sp4->a;
  (void) sp4->b;

  Type4 *mt4p = &mt4;
  (void) mt4p->a;
  (void) mt4p->b;

  A a;
  B b;
  C c;
  C2 c2;

  a.inta = 77;
  b.a = a;
  c.b = b;
  c2.b = b;

  a.foo();
  b->foo();
  c->foo();

  b->inta = 77;
  c->inta = 77;
  c2->inta = 77;

  return 0; // end of main
}

