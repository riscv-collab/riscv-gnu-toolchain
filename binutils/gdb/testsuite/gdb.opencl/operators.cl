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

__kernel void testkernel (__global int *data)
{
  char ca = 2;
  char cb = 1;
  uchar uca = 2;
  uchar ucb = 1;
  char4 c4a = (char4) (2, 4, 8, 16);
  char4 c4b = (char4) (1, 2, 8, 4);
  uchar4 uc4a = (uchar4) (2, 4, 8, 16);
  uchar4 uc4b = (uchar4) (1, 2, 8, 4);

  short sa = 2;
  short sb = 1;
  ushort usa = 2;
  ushort usb = 1;
  short4 s4a = (short4) (2, 4, 8, 16);
  short4 s4b = (short4) (1, 2, 8, 4);
  ushort4 us4a = (ushort4) (2, 4, 8, 16);
  ushort4 us4b = (ushort4) (1, 2, 8, 4);

  int ia = 2;
  int ib = 1;
  uint uia = 2;
  uint uib = 1;
  int4 i4a = (int4) (2, 4, 8, 16);
  int4 i4b = (int4) (1, 2, 8, 4);
  uint4 ui4a = (uint4) (2, 4, 8, 16);
  uint4 ui4b = (uint4) (1, 2, 8, 4);

  long la = 2;
  long lb = 1;
  ulong ula = 2;
  ulong ulb = 1;
  long4 l4a = (long4) (2, 4, 8, 16);
  long4 l4b = (long4) (1, 2, 8, 4);
  ulong4 ul4a = (ulong4) (2, 4, 8, 16);
  ulong4 ul4b = (ulong4) (1, 2, 8, 4);

#ifdef cl_khr_fp16
  half ha = 2;
  half hb = 1;
  half4 h4a = (half4) (2, 4, 8, 16);
  half4 h4b = (half4) (1, 2, 8, 4);
#endif

  float fa = 2;
  float fb = 1;
  float4 f4a = (float4) (2, 4, 8, 16);
  float4 f4b = (float4) (1, 2, 8, 4);

#ifdef cl_khr_fp64
  double da = 2;
  double db = 1;
  double4 d4a = (double4) (2, 4, 8, 16);
  double4 d4b = (double4) (1, 2, 8, 4);
#endif

  uint4 ui4 = (uint4) (2, 4, 8, 16);
  int2 i2 = (int2) (1, 2);
  long2 l2 = (long2) (1, 2);
#ifdef cl_khr_fp16
  half2 h2 = (half2) (1, 2);
#endif
  float2 f2 = (float2) (1, 2);
#ifdef cl_khr_fp64
  double2 d2 = (double2) (1, 2);
#endif

  /* marker! */

  data[get_global_id(0)] = 1;
}
