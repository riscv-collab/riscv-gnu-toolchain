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

extern void foo2 (); /* from derivation2.cc */

namespace N {
  typedef double value_type;
  struct Base { typedef int value_type; };
  struct Derived : public Base {
    void doit (void) const {
       int i = 3;

       while (i > 0)
         --i;
     }
  };
}

class A {
public:
    typedef int value_type;
    value_type a;
    value_type aa;

    A()
    {
        a=1;
        aa=2;
    }
    value_type afoo();
    value_type foo();
};



class B {
public:
    A::value_type b;
    A::value_type bb;

    B()
    {
        b=3;
        bb=4;
    }
    A::value_type bfoo();
    A::value_type foo();
    
};



class C {
public:
    int c;
    int cc;

    C()
    {
        c=5;
        cc=6;
    }
    int cfoo();
    int foo();
    
};



class D : private A, public B, protected C {
public:
    value_type d;
    value_type dd;

    D()
    {
        d =7;
        dd=8;
    }
    value_type dfoo();
    value_type foo();
    
};


class E : public A, B, protected C {
public:
    value_type e;
    value_type ee;

    E()
    {
        e =9;
        ee=10;
    }
    value_type efoo();
    value_type foo();
    
};


class F : A, public B, C {
public:
    value_type f;
    value_type ff;

    F()
    {
        f =11;
        ff=12;
    }
    value_type ffoo();
    value_type foo();
    
};

class G : private A, public B, protected C {
public:
    int g;
    int gg;
    int a;
    int b;
    int c;

    G()
    {
        g =13;
        gg =14;
        a=15;
        b=16;
        c=17;
        
    }
    int gfoo();
    int foo();
    
};

class Z : public A
{
public:
  typedef float value_type;
  value_type z;
};

class ZZ : public Z
{
public:
  value_type zz;
};

class V_base
{
public:
  virtual void m();
  int base;
};

void
V_base::m()
{
}

class V_inter : public virtual V_base
{
public:
  virtual void f();
  int inter;
};

void
V_inter::f()
{
}

class V_derived : public V_inter
{
public:
  double x;
};

V_derived vderived;

A::value_type A::afoo() {
    return 1;
}

A::value_type B::bfoo() {
    return 2;
}

A::value_type C::cfoo() {
    return 3;
}

D::value_type D::dfoo() {
    return 4;
}

E::value_type E::efoo() {
    return 5;
}

F::value_type F::ffoo() {
    return 6;
}

int G::gfoo() {
    return 77;
}

A::value_type A::foo()
{
    return 7;
    
}

A::value_type B::foo()
{
    return 8;
    
}

A::value_type C::foo()
{
    return 9;
    
}

D::value_type D::foo()
{
    return 10;
    
}

E::value_type E::foo()
{
    return 11;
    
}

F::value_type F::foo()
{
    return 12;
    
}

int G::foo()
{
    return 13;
    
}


void marker1()
{
}


int main(void)
{

    A a_instance;
    B b_instance;
    C c_instance;
    D d_instance;
    E e_instance;
    F f_instance;
    G g_instance;
    Z z_instance;
    ZZ zz_instance;

    marker1(); // marker1-returns-here
    
    a_instance.a = 20; // marker1-returns-here
    a_instance.aa = 21;
    b_instance.b = 22;
    b_instance.bb = 23;
    c_instance.c = 24;
    c_instance.cc = 25;
    d_instance.d = 26;
    d_instance.dd = 27;
    e_instance.e = 28;
    e_instance.ee =29;
    f_instance.f =30;
    f_instance.ff =31;
    g_instance.g = 32;
    g_instance.gg = 33;
    z_instance.z = 34.0;
    zz_instance.zz = 35.0;

    N::Derived dobj;
    N::Derived::value_type d = 1;
    N::value_type n = 3.0;
    dobj.doit ();
    foo2 ();
    return 0;
    
}
