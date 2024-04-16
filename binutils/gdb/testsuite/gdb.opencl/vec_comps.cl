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
#define CREATE_VEC(TYPE, NAME)\
  TYPE NAME =\
  (TYPE)  (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

  CREATE_VEC(char16, c16)
  CREATE_VEC(uchar16, uc16)
  CREATE_VEC(short16, s16)
  CREATE_VEC(ushort16, us16)
  CREATE_VEC(int16, i16)
  CREATE_VEC(uint16, ui16)
  CREATE_VEC(long16, l16)
  CREATE_VEC(ulong16, ul16)
#ifdef cl_khr_fp16
  CREATE_VEC(half16, h16)
#endif
  CREATE_VEC(float16, f16)
#ifdef cl_khr_fp64
  CREATE_VEC(double16, d16)
#endif

  /* marker! */

  data[get_global_id(0)] = 1;
}
