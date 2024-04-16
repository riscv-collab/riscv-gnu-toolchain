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

struct s
{
  int a;
  int b;
};

struct SS
{
  union { int x; char y; };
  union { int a; char b; };
};

typedef struct s TS;
TS ts;

int aligncheck;

union UU
{
  int i;
  float f;
  int a[5];
};

#ifdef __cplusplus
struct C
{
  int c;
  int d;

  int
  a_method (int x, char y)
    {
      return x + y;
    }

  int
  a_const_method (int x, char y) const
    {
      return x + y;
    }

  static int
  a_static_method (int x, char y)
    {
      return x + y;
    }
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

unsigned char global_unsigned_char;
char global_char;
signed char global_signed_char;

unsigned int global_unsigned_int;
int global_int;
signed int global_signed_int;

enum E
{ v1, v2, v3
};

struct s vec_data_1 = {1, 1};
struct s vec_data_2 = {1, 2};

static int
a_function (int x, char y)
{
  return x + y;
}

int
main ()
{
  int ar[2] = {1,2};
  struct s st;
  struct SS ss;
  union UU uu;
#ifdef __cplusplus
  C c;
  c.c = 1;
  c.d = 2;
  D d;
  d.e = 3;
  d.f = 4;

  c.a_method (0, 1);
  c.a_const_method (0, 1);
  C::a_static_method (0, 1);
#endif
  enum E e;

  st.a = 3;
  st.b = 5;

  e = v2;

  ss.x = 100;

  a_function (0, 1);

  return 0;      /* break to inspect struct and array.  */
}
