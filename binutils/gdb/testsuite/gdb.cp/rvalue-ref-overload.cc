/* This testcase is part of GDB, the GNU debugger.

   Copyright 1998-2024 Free Software Foundation, Inc.

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

/* Rvalue references overload tests for GDB, based on overload.cc.  */

#include <stddef.h>
#include <utility>

class foo;

typedef foo &foo_lval_ref;
typedef foo &&foo_rval_ref;

class foo
{
public:
  foo ();
  foo (foo_lval_ref);
  foo (foo_rval_ref);
  ~foo ();

  int overload1arg (foo_lval_ref);
  int overload1arg (foo_rval_ref);
  int overloadConst (const foo &);
  int overloadConst (const foo &&);
};

void
marker1 ()
{
}

static int
f (int &x)
{
  return 1;
}

static int
f (const int &x)
{
  return 2;
}

static int
f (int &&x)
{
  return 3;
}

static int
g (int &&x)
{
  return x;
}

int
main ()
{
  foo foo_rr_instance1;
  foo arg;
  int i = 0;
  const int ci = 0;

  // result = 1 + 2 + 3 + 3 = 9
  int result = f (i) + f (ci) + f (0) + f (std::move (i));

  /* Overload resolution below requires both a CV-conversion
     and reference conversion.  */
  int test_const // = 3
    = foo_rr_instance1.overloadConst (arg);

  /* The statement below is illegal: cannot bind rvalue reference of
     type 'int&&' to lvalue of type 'int'.

     result = g (i); */
  result = g (5); // this is OK

  marker1 (); // marker1-returns-here
  return result;
}

foo::foo  ()                       {}
foo::foo  (foo_lval_ref afoo)      {}
foo::foo  (foo_rval_ref afoo)      {}
foo::~foo ()                       {}

/* Some functions to test overloading by varying one argument type. */

int foo::overload1arg (foo_lval_ref arg)           { return 1; }
int foo::overload1arg (foo_rval_ref arg)           { return 2; }
int foo::overloadConst (const foo &arg)            { return 3; }
int foo::overloadConst (const foo &&arg)           { return 4; }
