/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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

class C {
public:
  C() {}
  ~C() {}
};

typedef struct {
  C m;
} t;

t v;

namespace X {
  class C2 {
  public:
    C2() {}
  };

  typedef struct {
    C2 m;
  } t2;

  t2 v2;
}

template<class T>
class C3 {
public:
  ~C3() {}
};

typedef struct {
  C3<int> m;
} t3;

t3 v3;

int main()
{
  return 0;
}
