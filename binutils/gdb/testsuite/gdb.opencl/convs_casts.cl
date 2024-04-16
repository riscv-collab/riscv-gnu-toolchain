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
  char c = 123;
  uchar uc = 123;
  short s = 123;
  ushort us = 123;
  int i = 123;
  uint ui = 123;
  long l = 123;
  ulong ul = 123;
#ifdef cl_khr_fp16
  half h = 123.0;
#endif
  float f = 123.0;
#ifdef cl_khr_fp64
  double d = 123.0;
#endif

  /* marker! */

  data[get_global_id(0)] = 1;
}
