/* Copyright 2015-2024 Free Software Foundation, Inc.

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

namespace {
  static enum {ABC = 1, DEF, GHI, JKL} anon_e = GHI;
  static union
  {
    char aa;
    int bb;
    float ff;
    double dd;
    void *pp;
  } anon_u = { 'a' };

  static struct
  {
    char *ptr;
    int len;
    struct
    {
      unsigned MAGIC;
    };
    union
    {
      int ua;
      char *ub;
    };
  } anon_s = {"abracadabra", 11, 0xdead, 0xbeef};

  struct A
  {
    A () : e (AA)
    {
      this->u.b = 0;
      this->s.ptr = "hello";
      this->s.len = 5;
    }

    enum {AA = 10, BB, CC, DD} e;
    union
    {
      char a;
      int b;
      float f;
      double d;
      void *p;
    } u;
    struct
    {
      char *ptr;
      int len;
    } s;
  };
};

int
main ()
{
  A a;
  int var = 1234;

  return a.u.b + a.s.len + static_cast<int> (a.e)
    + static_cast<int> (anon_e) + anon_u.bb + anon_s.len; // break here
}
