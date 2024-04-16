/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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

#include <stdlib.h>

struct s
{
  int a;
  int b;
};

typedef struct s TS;
TS ts;

#ifdef __cplusplus
struct C
{
  int c;
  int d;
};

struct D : C
{
  int e;
  int f;
};

template<typename T, int I, int C::*MP>
struct Temargs
{
};

Temargs<D, 23, &C::c> temvar;

#endif

enum E
{ v1, v2, v3
};

struct s vec_data_1 = {1, 1};
struct s vec_data_2 = {1, 2};

struct flex_member
{
  int n;
  int items[];
};

int
main ()
{
  int ar[2] = {1,2};
  struct s st;
#ifdef __cplusplus
  C c;
  c.c = 1;
  c.d = 2;
  D d;
  d.e = 3;
  d.f = 4;
#endif
  enum E e;
  
  st.a = 3;
  st.b = 5;

  e = v2;

  struct flex_member *f = (struct flex_member *) malloc (100);
  f->items[0] = 111;
  f->items[1] = 222;
  
  return 0;      /* break to inspect struct and array.  */
}
