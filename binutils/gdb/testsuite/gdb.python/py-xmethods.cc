/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

namespace dop
{

class A
{
public:
  int a;
  int array [10];
  virtual ~A ();
  int operator+ (const A &obj);
  int operator- (const A &obj);
  virtual int geta (void);
};

A::~A () { }

int a_plus_a = 0;

int
A::operator+ (const A &obj)
{
  a_plus_a++;
  return a + obj.a;
}

int a_minus_a = 0;

int
A::operator- (const A &obj)
{
  a_minus_a++;
  return a - obj.a;
}

int a_geta = 0;

int
A::geta (void)
{
  a_geta++;
  return a;
}

class B : public A
{
public:
  virtual int geta (void);
};

int b_geta = 0;

int
B::geta (void)
{
  b_geta++;
  return 2 * a;
}

typedef B Bt;

typedef Bt Btt;

class E : public A
{
public:
  /* This class has a member named 'a', while the base class also has a
     member named 'a'.  When one invokes A::geta(), A::a should be
     returned and not E::a as the 'geta' method is defined on class 'A'.
     This class tests this aspect of debug methods.  */
  int a;
};

template <typename T>
class G
{
 public:
  template <typename T1>
  int size_diff ();

  template <int M>
  int size_mul ();

  template <typename T1>
  T mul(const T1 t1);

 public:
  T t;
};

int g_size_diff = 0;

template <typename T>
template <typename T1>
int
G<T>::size_diff ()
{
  g_size_diff++;
  return sizeof (T1) - sizeof (T);
}

int g_size_mul = 0;

template <typename T>
template <int M>
int
G<T>::size_mul ()
{
  g_size_mul++;
  return M * sizeof (T);
}

int g_mul = 0;

template <typename T>
template <typename T1>
T
G<T>::mul (const T1 t1)
{
  g_mul++;
  return t1 * t;
}

}  // namespace dop

using namespace dop;

int main(void)
{
  A a1, a2;
  a1.a = 5;
  a2.a = 10;

  B b1;
  b1.a = 30;
  A *a_ptr = &b1;

  Bt bt;
  bt.a = 40;

  Btt btt;
  btt.a = -5;

  G<int> g, *g_ptr;
  g.t = 5;
  g_ptr = &g;

  E e;
  E &e_ref = e;
  E *e_ptr = &e;
  e.a = 1000;
  e.A::a = 100;

  int diff = g.size_diff<float> ();
  int smul = g.size_mul<2> ();
  int mul = g.mul (1.0);

  for (int i = 0; i < 10; i++)
    {
      a1.array[i] = a2.array[i] = b1.array[i] = i;
    }

  return 0; /* Break here.  */
}
