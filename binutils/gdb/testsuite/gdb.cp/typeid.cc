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

#include <typeinfo>

int i;
char *cp;
const char *ccp;
char ca[5];

struct Base
{
  virtual ~Base() { }
};

struct VB1 : public virtual Base
{
};

struct VB2 : public virtual Base
{
};

struct Derived : public VB1, VB2
{
};

Derived d;

Base *b = &d;
VB1 *vb1 = &d;
VB1 *vb2 = &d;

const Base *bv = &d;

int main ()
{
  const std::type_info &xi = typeid(i);
  const std::type_info &xcp = typeid(cp);
  const std::type_info &xccp = typeid(ccp);
  const std::type_info &xca = typeid(ca);
  const std::type_info &xd = typeid(d);
  const std::type_info &xb = typeid(b);

  return 0;
}
