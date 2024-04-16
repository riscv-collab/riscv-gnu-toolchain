/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

#define ARRAY_SIZE 10

struct SimpleStruct
{
  int a;
  double d;
};

union SimpleUnion
{
  int i;
  char c;
  float f;
  double d;
};

typedef struct SimpleStruct SS;

struct ComplexStruct
{
  struct SimpleStruct s;
  union SimpleUnion u;
  SS sa[ARRAY_SIZE];
};

union ComplexUnion
{
  SS s;
  struct SimpleStruct sa[ARRAY_SIZE];
};

int
main (void)
{
  struct SimpleStruct ss;
  struct SimpleStruct* ss_ptr = &ss;
  SS ss_t;

  union SimpleUnion su;
  struct ComplexStruct cs;
  struct ComplexStruct* cs_ptr = &cs;
  union ComplexUnion cu;
  int i;
  double darray[5] = {0.1, 0.2, 0.3, 0.4, 0.5};
  double *darray_ref = darray;

  ss.a = 10;
  ss.d = 100.01;
  ss_t = ss;
  
  su.d = 100.1;

  cs.s = ss;
  cs.u = su;
  for (i = 0; i < ARRAY_SIZE; i++)
    {
      cs.sa[i].a = i;
      cs.sa[i].d = 10.10 + i;
      cu.sa[i].a = i;
      cu.sa[i].d = 100.10 + i;
    }

  return 0; /* Break here. */
}
