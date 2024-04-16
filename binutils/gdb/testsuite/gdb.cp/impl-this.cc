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

#ifdef DEBUG
#include <stdio.h>
#endif

template <typename T>
class A
{
public:
  T i;
  T z;
  A () : i (1), z (10) {}
};

template <typename T>
class B : public virtual A<T>
{
public:
  T i;
  T common;
  B () : i (2), common (200) {}
};

typedef B<int> Bint;

class C : public virtual A<int>
{
public:
  int i;
  int c;
  int common;
  C () : i (3), c (30), common (300) {}
};

class BB : public A<int>
{
public:
  int i;
  BB () : i (20) {}
};

class CC : public A<int>
{
public:
  int i;
  CC () : i (30) {}
};

class Ambig : public BB, public CC
{
public:
  int i;
  Ambig () : i (1000) {}
};

class D : public Bint, public C
{
public:
  int i;
  int x;
  Ambig am;
  D () : i (4), x (40) {}

#ifdef DEBUG
#define SUM(X)					\
  do						\
    {						\
      sum += (X);				\
      printf ("" #X " = %d\n", (X));		\
    }						\
  while (0)
#else
#define SUM(X) sum += (X)
#endif

int
f (void)
  {
    int sum = 0;

    SUM (i);
    SUM (D::i);
    SUM (D::B<int>::i);
    SUM (B<int>::i);
    SUM (D::C::i);
    SUM (C::i);
    SUM (D::B<int>::A<int>::i);
    SUM (B<int>::A<int>::i);
    SUM (A<int>::i);
    SUM (D::C::A<int>::i);
    SUM (C::A<int>::i);
    SUM (D::x);
    SUM (x);
    SUM (D::C::c);
    SUM (C::c);
    SUM (c);
    SUM (D::A<int>::i);
    SUM (Bint::i);
    //SUM (D::Bint::i);
    //SUM (D::Bint::A<int>::i);
    SUM (Bint::A<int>::i);
    // ambiguous: SUM (common);
    SUM (B<int>::common);
    SUM (C::common);
    SUM (am.i);
    // ambiguous: SUM (am.A<int>::i);

    return sum;
  }
};

int
main (void)
{
  Bint b;
  D d;

  return d.f () + b.i;
}
