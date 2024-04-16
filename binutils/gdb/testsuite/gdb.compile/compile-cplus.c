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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdbool.h>
#include <iostream>

#define SOME_MACRO 23
#define ARG_MACRO(X, Y) ((X) + (Y) - 1)


enum enum_type {
  ONE = 1,
  TWO = 2
};

typedef int v4 __attribute__ ((vector_size (16)));

union union_type;

struct struct_type {
  char charfield;
  unsigned char ucharfield;
  short shortfield;
  unsigned short ushortfield;
  int intfield;
  unsigned int uintfield;
  unsigned int bitfield : 3;
  long longfield;
  unsigned long ulongfield;
  enum enum_type enumfield;
  float floatfield;
  double doublefield;
  const union union_type *ptrfield;
  struct struct_type *selffield;
  int arrayfield[5];
  _Complex double complexfield;
  _Bool boolfield;
  v4 vectorfield;
};

typedef int inttypedef;

union union_type {
  int intfield;
  inttypedef typedeffield;
};

/* volatile provides some coverage of the conversion code.  */
volatile struct struct_type struct_object;

union union_type union_object;


enum ulonger_enum_type {
  REALLY_MINUS_1 = -1UL,
};

enum ulonger_enum_type ulonger;

enum longer_enum_type {
  MINUS_1 = -1,
  FORCE_TO_LONG = 1L << ((8 * sizeof (long)) - 2)
};

enum longer_enum_type longer;

int globalvar = 10;

static void
func_static (int addend)
{
  globalvar += addend;
}

void
func_global (int subtrahend)
{
  globalvar -= subtrahend;
}

void
no_args_or_locals ()
{
  /* no_args_or_locals breakpoint */
}

int *intptr;
int globalshadow = 10;
static int staticshadow = 20;
int externed = 7;

class Base
{
  virtual int pure_virt () = 0;
 public:
  int return_value () {return a;}
 private:
  int a = 1;
  int b = 2;
};

class Base2
{
  virtual int non_pure () {return 84;}
 public:
  int return_value () {return b;}
 private:
  int a = 3;
  int b = 4;
};

class Base3
{
 public:
  int return_value () {return b;}
 private:
  int b = 5;
};


class Multiple : public Base, public Base2
{
  int pure_virt ()
  {
    int a = Base::return_value ();
    return a + 42;
  }
};
//struct foo { foo(); virtual ~foo(); }; struct bar : virtual foo { bar(); ~bar(); }; struct baz : bar {}; bar::bar() {} bar::~bar() {} bar t; baz u;
struct VirtualOnly
{
  VirtualOnly();
  virtual ~VirtualOnly()=0;
};

VirtualOnly::VirtualOnly ()
{
}

VirtualOnly::~VirtualOnly ()
{
}

struct VirtualBase : virtual VirtualOnly
{
  int z = 22;
  VirtualBase ();
  ~VirtualBase ();
};

struct VirtualBase2 : VirtualBase {};

VirtualBase::VirtualBase ()
{
  z = 24;
}

VirtualBase::~VirtualBase ()
{
  z = 22;
}

class Foo
{
  int var;
  static const int public_static_var = 12;

 private:
  int private_var = 0;
  int private_method ();

 public:
  int public_var = 0;
  int public_method ();
  void set_private_var (int);
};

void Foo::set_private_var (int i)
{
  private_var = i;
}

int Foo::private_method ()
{
  return private_var;
}

int Foo::public_method ()
{
  return public_var;
}

int
main ()
{
  int localvar = 50;
  int shadowed = 51;
  int bound = 3;
  int unresolved = 10;
  int globalshadow = 100;
  int staticshadow = 200;
  int externed = 9;
  int f = 0;
  int var = 0;
  Foo foovar;
  Multiple *multivar = new Multiple;
  VirtualBase vbase;
  VirtualBase2 vbase2;
  static int static_local = 77000;

  foovar.public_var = 42;
  foovar.set_private_var (42);
  multivar->Base2::return_value();
  {
    int another_local = 7;
    int shadowed = 52;
    extern int unresolved;
    extern int externed;

    int vla[bound];

    func_static (0); /* break-here */
    no_args_or_locals ();
  }

 return 0;
}
