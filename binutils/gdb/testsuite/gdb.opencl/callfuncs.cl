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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Contributed by Ulrich Weigand <ulrich.weigand.ibm.com>  */

__constant int opencl_version = __OPENCL_VERSION__;

#ifdef HAVE_cl_khr_fp64
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__constant int have_cl_khr_fp64 = 1;
#else
__constant int have_cl_khr_fp64 = 0;
#endif

#ifdef HAVE_cl_khr_fp16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
__constant int have_cl_khr_fp16 = 1;
#else
__constant int have_cl_khr_fp16 = 0;
#endif

#define def_call_func(type) \
  type call_##type (type a, type b) { return a + b; }

#ifdef CL_VERSION_1_1
#define def_call_family(type) \
  def_call_func(type) \
  def_call_func(type##2) \
  def_call_func(type##3) \
  def_call_func(type##4) \
  def_call_func(type##8) \
  def_call_func(type##16)
#else
#define def_call_family(type) \
  def_call_func(type) \
  def_call_func(type##2) \
  def_call_func(type##4) \
  def_call_func(type##8) \
  def_call_func(type##16)
#endif

def_call_family(char)
def_call_family(uchar)
def_call_family(short)
def_call_family(ushort)
def_call_family(int)
def_call_family(uint)
def_call_family(long)
def_call_family(ulong)
#ifdef cl_khr_fp16
def_call_family(half)
#endif
def_call_family(float)
#ifdef cl_khr_fp64
def_call_family(double)
#endif

#define call_func(type, var) \
  var = call_##type (var, var);

#ifdef CL_VERSION_1_1
#define call_family(type, var) \
  call_func(type, var) \
  call_func(type##2, var##2) \
  call_func(type##3, var##3) \
  call_func(type##4, var##4) \
  call_func(type##8, var##8) \
  call_func(type##16, var##16)
#else
#define call_family(type, var) \
  call_func(type, var) \
  call_func(type##2, var##2) \
  call_func(type##4, var##4) \
  call_func(type##8, var##8) \
  call_func(type##16, var##16)
#endif

__kernel void testkernel (__global int *data)
{
  bool b = 0;

  char   c   = 1;
  char2  c2  = (char2) (1, 2);
#ifdef CL_VERSION_1_1
  char3  c3  = (char3) (1, 2, 3);
#endif
  char4  c4  = (char4) (1, 2, 3, 4);
  char8  c8  = (char8) (1, 2, 3, 4, 5, 6, 7, 8);
  char16 c16 = (char16)(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

  uchar   uc   = 1;
  uchar2  uc2  = (uchar2) (1, 2);
#ifdef CL_VERSION_1_1
  uchar3  uc3  = (uchar3) (1, 2, 3);
#endif
  uchar4  uc4  = (uchar4) (1, 2, 3, 4);
  uchar8  uc8  = (uchar8) (1, 2, 3, 4, 5, 6, 7, 8);
  uchar16 uc16 = (uchar16)(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

  short   s   = 1;
  short2  s2  = (short2) (1, 2);
#ifdef CL_VERSION_1_1
  short3  s3  = (short3) (1, 2, 3);
#endif
  short4  s4  = (short4) (1, 2, 3, 4);
  short8  s8  = (short8) (1, 2, 3, 4, 5, 6, 7, 8);
  short16 s16 = (short16)(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

  ushort   us   = 1;
  ushort2  us2  = (ushort2) (1, 2);
#ifdef CL_VERSION_1_1
  ushort3  us3  = (ushort3) (1, 2, 3);
#endif
  ushort4  us4  = (ushort4) (1, 2, 3, 4);
  ushort8  us8  = (ushort8) (1, 2, 3, 4, 5, 6, 7, 8);
  ushort16 us16 = (ushort16)(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

  int   i   = 1;
  int2  i2  = (int2) (1, 2);
#ifdef CL_VERSION_1_1
  int3  i3  = (int3) (1, 2, 3);
#endif
  int4  i4  = (int4) (1, 2, 3, 4);
  int8  i8  = (int8) (1, 2, 3, 4, 5, 6, 7, 8);
  int16 i16 = (int16)(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

  uint   ui   = 1;
  uint2  ui2  = (uint2) (1, 2);
#ifdef CL_VERSION_1_1
  uint3  ui3  = (uint3) (1, 2, 3);
#endif
  uint4  ui4  = (uint4) (1, 2, 3, 4);
  uint8  ui8  = (uint8) (1, 2, 3, 4, 5, 6, 7, 8);
  uint16 ui16 = (uint16)(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

  long   l   = 1;
  long2  l2  = (long2) (1, 2);
#ifdef CL_VERSION_1_1
  long3  l3  = (long3) (1, 2, 3);
#endif
  long4  l4  = (long4) (1, 2, 3, 4);
  long8  l8  = (long8) (1, 2, 3, 4, 5, 6, 7, 8);
  long16 l16 = (long16)(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

  ulong   ul   = 1;
  ulong2  ul2  = (ulong2) (1, 2);
#ifdef CL_VERSION_1_1
  ulong3  ul3  = (ulong3) (1, 2, 3);
#endif
  ulong4  ul4  = (ulong4) (1, 2, 3, 4);
  ulong8  ul8  = (ulong8) (1, 2, 3, 4, 5, 6, 7, 8);
  ulong16 ul16 = (ulong16)(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

#ifdef cl_khr_fp16
  half   h   = 1.0;
  half2  h2  = (half2) (1.0, 2.0);
#ifdef CL_VERSION_1_1
  half3  h3  = (half3) (1.0, 2.0, 3.0);
#endif
  half4  h4  = (half4) (1.0, 2.0, 3.0, 4.0);
  half8  h8  = (half8) (1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
  half16 h16 = (half16)(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0);
#endif

  float   f   = 1.0;
  float2  f2  = (float2) (1.0, 2.0);
#ifdef CL_VERSION_1_1
  float3  f3  = (float3) (1.0, 2.0, 3.0);
#endif
  float4  f4  = (float4) (1.0, 2.0, 3.0, 4.0);
  float8  f8  = (float8) (1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
  float16 f16 = (float16)(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0);

#ifdef cl_khr_fp64
  double   d   = 1.0;
  double2  d2  = (double2) (1.0, 2.0);
#ifdef CL_VERSION_1_1
  double3  d3  = (double3) (1.0, 2.0, 3.0);
#endif
  double4  d4  = (double4) (1.0, 2.0, 3.0, 4.0);
  double8  d8  = (double8) (1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
  double16 d16 = (double16)(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0);
#endif

  /* marker! */

  call_family (char, c);
  call_family (uchar, uc);
  call_family (short, s);
  call_family (ushort, us);
  call_family (int, i);
  call_family (uint, ui);
  call_family (long, l);
  call_family (ulong, ul);
#ifdef cl_khr_fp16
  call_family (half, h);
#endif
  call_family (float, f);
#ifdef cl_khr_fp64
  call_family (double, d);
#endif

  data[get_global_id(0)] = 1;
}
