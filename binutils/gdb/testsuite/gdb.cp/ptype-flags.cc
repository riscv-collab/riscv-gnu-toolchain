/* Copyright 2012-2024 Free Software Foundation, Inc.

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

template<typename S>
class Simple
{
  S val;
};

template<typename T>
class Base
{
};

template<typename T>
class Holder : public Base<T>
{
public:
  Simple<T> t;
  Simple<T*> tstar;

  typedef Simple< Simple<T> > Z;

  Z z;

  double method(void) { return 23.0; }
};

namespace ns
{
  typedef double scoped_double;
}

typedef double global_double;

class TypedefHolder
{
public:
  double a;
  ns::scoped_double b;
  global_double c;

private:
  typedef double class_double;
  class_double d;

  double method1(ns::scoped_double) { return 24.0; }
  double method2(global_double) { return 24.0; }
};

Holder<int> value;
TypedefHolder value2;

int main()
{
  return 0;
}
