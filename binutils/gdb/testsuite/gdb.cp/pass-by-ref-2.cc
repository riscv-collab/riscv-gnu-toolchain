/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

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

class ByVal {
public:
  ByVal (void);

  int x;
};

ByVal::ByVal (void)
{
  x = 2;
}

class ByRef {
public:
  ByRef (void);

  ByRef (const ByRef &rhs);

  int x;
};

ByRef::ByRef (void)
{
  x = 2;
}

ByRef::ByRef (const ByRef &rhs)
{
  x = 3; /* ByRef-cctor */
}

class ArrayContainerByVal {
public:
  ByVal items[2];
};

int
cbvArrayContainerByVal (ArrayContainerByVal arg)
{
  arg.items[0].x += 4;  // intentionally modify
  return arg.items[0].x;
}

class ArrayContainerByRef {
public:
  ByRef items[2];
};

int
cbvArrayContainerByRef (ArrayContainerByRef arg)
{
  arg.items[0].x += 4;  // intentionally modify
  return arg.items[0].x;
}

class DynamicBase {
public:
  DynamicBase (void);

  virtual int get (void);

  int x;
};

DynamicBase::DynamicBase (void)
{
  x = 2;
}

int
DynamicBase::get (void)
{
  return 42;
}

class Dynamic : public DynamicBase {
public:
  virtual int get (void);
};

int
Dynamic::get (void)
{
  return 9999;
}

int
cbvDynamic (DynamicBase arg)
{
  arg.x += 4;  // intentionally modify
  return arg.x + arg.get ();
}

class Inlined {
public:
  Inlined (void);

  __attribute__((always_inline))
  Inlined (const Inlined &rhs)
  {
    x = 3;
  }

  int x;
};

Inlined::Inlined (void)
{
  x = 2;
}

int
cbvInlined (Inlined arg)
{
  arg.x += 4;  // intentionally modify
  return arg.x;
}

class DtorDel {
public:
  DtorDel (void);

  ~DtorDel (void) = delete;

  int x;
};

DtorDel::DtorDel (void)
{
  x = 2;
}

int
cbvDtorDel (DtorDel arg)
{
  // Calling this method should be rejected
  return arg.x;
}

class FourCCtor {
public:
  FourCCtor (void);

  FourCCtor (FourCCtor &rhs);
  FourCCtor (const FourCCtor &rhs);
  FourCCtor (volatile FourCCtor &rhs);
  FourCCtor (const volatile FourCCtor &rhs);

  int x;
};

FourCCtor::FourCCtor (void)
{
  x = 2;
}

FourCCtor::FourCCtor (FourCCtor &rhs)
{
  x = 3;
}

FourCCtor::FourCCtor (const FourCCtor &rhs)
{
  x = 4;
}

FourCCtor::FourCCtor (volatile FourCCtor &rhs)
{
  x = 5;
}

FourCCtor::FourCCtor (const volatile FourCCtor &rhs)
{
  x = 6;
}

int
cbvFourCCtor (FourCCtor arg)
{
  arg.x += 10;  // intentionally modify
  return arg.x;
}

class TwoMCtor {
public:
  TwoMCtor (void);

  /* Even though one move ctor is defaulted, the other
     is explicit.  */
  TwoMCtor (const TwoMCtor &&rhs);
  TwoMCtor (TwoMCtor &&rhs) = default;

  int x;
};

TwoMCtor::TwoMCtor (void)
{
  x = 2;
}

TwoMCtor::TwoMCtor (const TwoMCtor &&rhs)
{
  x = 3;
}

int
cbvTwoMCtor (TwoMCtor arg)
{
  arg.x += 10;  // intentionally modify
  return arg.x;
}

class TwoMCtorAndCCtor {
public:
  TwoMCtorAndCCtor (void);

  TwoMCtorAndCCtor (const TwoMCtorAndCCtor &rhs) = default;

  /* Even though one move ctor is defaulted, the other
     is explicit.  This makes the type pass-by-ref.  */
  TwoMCtorAndCCtor (const TwoMCtorAndCCtor &&rhs);
  TwoMCtorAndCCtor (TwoMCtorAndCCtor &&rhs) = default;

  int x;
};

TwoMCtorAndCCtor::TwoMCtorAndCCtor (void)
{
  x = 2;
}

TwoMCtorAndCCtor::TwoMCtorAndCCtor (const TwoMCtorAndCCtor &&rhs)
{
  x = 4;
}

int
cbvTwoMCtorAndCCtor (TwoMCtorAndCCtor arg)
{
  arg.x += 10;  // intentionally modify
  return arg.x;
}

ArrayContainerByVal arrayContainerByVal;
ArrayContainerByRef arrayContainerByRef;
Dynamic dynamic;
Inlined inlined;
// Cannot stack-allocate DtorDel
DtorDel *dtorDel;
FourCCtor fourCctor_c0v0;
const FourCCtor fourCctor_c1v0;
volatile FourCCtor fourCctor_c0v1;
const volatile FourCCtor fourCctor_c1v1;
TwoMCtor twoMctor;
TwoMCtorAndCCtor twoMctorAndCctor;

int
main (void)
{
  int v;
  dtorDel = new DtorDel;
  /* Explicitly call the cbv function to make sure the compiler
     will not omit any code in the binary.  */
  v = cbvArrayContainerByVal (arrayContainerByVal);
  v = cbvArrayContainerByRef (arrayContainerByRef);
  v = cbvDynamic (dynamic);
  v = cbvInlined (inlined);
  v = cbvFourCCtor (fourCctor_c0v0);
  v = cbvFourCCtor (fourCctor_c1v0);
  v = cbvFourCCtor (fourCctor_c0v1);
  v = cbvFourCCtor (fourCctor_c1v1);
  /* v = cbvTwoMCtor (twoMctor); */ // This is illegal, cctor is deleted
  v = cbvTwoMCtorAndCCtor (twoMctorAndCctor);

  /* stop here */

  return 0;
}
