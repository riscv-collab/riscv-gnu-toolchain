/* Template argument tests.

   Copyright 2010-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Please email any bugs, comments, and/or additions to this file to:
   bug-gdb@gnu.org  */

int a_global;

struct S
{
  int f;
  void somefunc() { }
};

template<typename T, int I, int *P, int S::*MP>
struct Base
{
  template<typename Z>
  struct Inner
  {
    void inner_m ()
    {
      // Breakpoint 2.
    }
  };

  void base_m ()
  {
    // Breakpoint 1.
  }

  template<typename Q>
  void templ_m ()
  {
    // Breakpoint 4.
  }
};

template<typename T, int I, int *P, int S::*MP>
void func ()
{
  // Breakpoint 3.
}

template<void (S::*F) ()>
struct K2
{
  void k2_m ()
  {
    // Breakpoint 5.
  }
};

// GCC PR debug/49546
struct S3
{
  static void m (int x) {}
};
template <void (*F) (int)>
// or: template <void (F) (int)>
struct K3
{
  void k3_m ()
  {
    F (0);	// Breakpoint 6.
  }
};

namespace pr24470
{
// From PR c++/24470
// This caused a gdb crash during startup.

template <int a> struct b {};
template <typename, typename> struct c {
  template <long d> using e = b<d>;
  void k(e<0>);
};
template <typename, template <typename, typename> class, unsigned long...>
struct m;
template <typename g, template <typename, typename> class h, unsigned long i>
struct m<g, h, i> {
  using j = b<i>;
};
struct n {
  template <typename g> using f = typename m<g, c, 0>::j;
};

n::f<int> l;
}

int main ()
{
  Base<double, 23, &a_global, &S::f> base;
  // Note that instantiating with P==0 does not work with g++.
  // That would be worth testing, once g++ is fixed.
  Base<long, 47, &a_global, &S::f>::Inner<float> inner;
  K2<&S::somefunc> k2;
  K3<&S3::m> k3;
// or: K3<S3::m> k3;

  base.base_m ();
  inner.inner_m ();
  func<unsigned char, 91, &a_global, &S::f> ();
  base.templ_m<short> ();
  k2.k2_m ();
  k3.k3_m ();

  return 0;
}
