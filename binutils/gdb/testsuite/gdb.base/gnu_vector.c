/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Contributed by Ken Werner <ken.werner@de.ibm.com>  */

#include <stdarg.h>

#define VECTOR(n, type)					\
  type __attribute__ ((vector_size (n * sizeof(type))))

typedef VECTOR (8, int) int8;

typedef VECTOR (4, int) int4;
typedef VECTOR (4, unsigned int) uint4;
typedef VECTOR (4, char) char4;
typedef VECTOR (4, float) float4;

typedef VECTOR (2, int) int2;
typedef VECTOR (2, long long) longlong2;
typedef VECTOR (2, float) float2;
typedef VECTOR (2, double) double2;

typedef VECTOR (1, char) char1;
typedef VECTOR (1, int) int1;
typedef VECTOR (1, double) double1;

int ia = 2;
int ib = 1;
float fa = 2;
float fb = 1;
long long lla __attribute__ ((mode(DI))) = 0x0000000100000001ll;
char4 c4 = {1, 2, 3, 4};
int4 i4a = {2, 4, 8, 16};
int4 i4b = {1, 2, 8, 4};
float4 f4a = {2, 4, 8, 16};
float4 f4b = {1, 2, 8, 4};
uint4 ui4 = {2, 4, 8, 16};
int2 i2 = {1, 2};
longlong2 ll2 = {1, 2};
float2 f2 = {1, 2};
double2 d2 = {1, 2};

union
{
  int i;
  VECTOR (sizeof(int), char) cv;
} union_with_vector_1;

struct
{
  int i;
  VECTOR (sizeof(int), char) cv;
  float4 f4;
} struct_with_vector_1;

struct just_int2
{
  int2 i;
};

struct two_int2
{
  int2 i, j;
};


/* Simple vector-valued function with a few 16-byte vector
   arguments.  */

int4
add_some_intvecs (int4 a, int4 b, int4 c)
{
  return a + b + c;
}

/* Many small vector arguments, 4 bytes each.  */

char4
add_many_charvecs (char4 a, char4 b, char4 c, char4 d, char4 e,
		   char4 f, char4 g, char4 h, char4 i, char4 j)
{
  return (a + b + c + d + e + f + g + h + i + j);
}

/* Varargs: One fixed and N-1 variable vector arguments.  */

float4
add_various_floatvecs (int n, float4 a, ...)
{
  int i;
  va_list argp;

  va_start (argp, a);
  for (i = 1; i < n; i++)
    a += va_arg (argp, float4);
  va_end (argp);

  return a;
}

/* Struct-wrapped vectors (might be passed as if not wrapped).  */

struct just_int2
add_structvecs (int2 a, struct just_int2 b, struct two_int2 c)
{
  struct just_int2 res;

  res.i = a + b.i + c.i + c.j;
  return res;
}

/* Single-element vectors (might be treated like scalars).  */

double1
add_singlevecs (char1 a, int1 b, double1 c)
{
  return (double1) {a[0] + b[0] + c[0]};
}


int
main ()
{
  int4 res;

  res = add_some_intvecs (i4a, i4a + i4b, i4b);

  res = add_some_intvecs (i4a, i4a + i4b, i4b);

  add_some_intvecs (i4a, i4a + i4b, i4b);

  return 0;
}
