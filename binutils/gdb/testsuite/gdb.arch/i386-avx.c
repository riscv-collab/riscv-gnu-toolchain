/* Test program for AVX registers.

   Copyright 2010-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

#include <stdio.h>
#include "nat/x86-cpuid.h"

/* Align sufficient to be able to use vmovaps.  */
#define ALIGN 32

typedef struct {
  _Alignas (ALIGN) float f[8];
} v8sf_t;


v8sf_t data_orig[] =
  {
    { {  0.0,  0.125,  0.25,  0.375,  0.50,  0.625,  0.75,  0.875 } },
    { {  1.0,  1.125,  1.25,  1.375,  1.50,  1.625,  1.75,  1.875 } },
    { {  2.0,  2.125,  2.25,  2.375,  2.50,  2.625,  2.75,  2.875 } },
    { {  3.0,  3.125,  3.25,  3.375,  3.50,  3.625,  3.75,  3.875 } },
    { {  4.0,  4.125,  4.25,  4.375,  4.50,  4.625,  4.75,  4.875 } },
    { {  5.0,  5.125,  5.25,  5.375,  5.50,  5.625,  5.75,  5.875 } },
    { {  6.0,  6.125,  6.25,  6.375,  6.50,  6.625,  6.75,  6.875 } },
    { {  7.0,  7.125,  7.25,  7.375,  7.50,  7.625,  7.75,  7.875 } },
#ifdef __x86_64__
    { {  8.0,  8.125,  8.25,  8.375,  8.50,  8.625,  8.75,  8.875 } },
    { {  9.0,  9.125,  9.25,  9.375,  9.50,  9.625,  9.75,  9.875 } },
    { { 10.0, 10.125, 10.25, 10.375, 10.50, 10.625, 10.75, 10.875 } },
    { { 11.0, 11.125, 11.25, 11.375, 11.50, 11.625, 11.75, 11.875 } },
    { { 12.0, 12.125, 12.25, 12.375, 12.50, 12.625, 12.75, 12.875 } },
    { { 13.0, 13.125, 13.25, 13.375, 13.50, 13.625, 13.75, 13.875 } },
    { { 14.0, 14.125, 14.25, 14.375, 14.50, 14.625, 14.75, 14.875 } },
    { { 15.0, 15.125, 15.25, 15.375, 15.50, 15.625, 15.75, 15.875 } },
#endif
  };

#include "precise-aligned-alloc.c"

int
main (int argc, char **argv)
{
  void *allocated_ptr;
  v8sf_t *data
    = precise_aligned_dup (ALIGN, sizeof (data_orig), &allocated_ptr,
			   data_orig);

  asm ("vmovaps 0(%0), %%ymm0\n\t"
       "vmovaps 32(%0), %%ymm1\n\t"
       "vmovaps 64(%0), %%ymm2\n\t"
       "vmovaps 96(%0), %%ymm3\n\t"
       "vmovaps 128(%0), %%ymm4\n\t"
       "vmovaps 160(%0), %%ymm5\n\t"
       "vmovaps 192(%0), %%ymm6\n\t"
       "vmovaps 224(%0), %%ymm7\n\t"
       : /* no output operands */
       : "r" (data)
       : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7");
#ifdef __x86_64__
  asm ("vmovaps 256(%0), %%ymm8\n\t"
       "vmovaps 288(%0), %%ymm9\n\t"
       "vmovaps 320(%0), %%ymm10\n\t"
       "vmovaps 352(%0), %%ymm11\n\t"
       "vmovaps 384(%0), %%ymm12\n\t"
       "vmovaps 416(%0), %%ymm13\n\t"
       "vmovaps 448(%0), %%ymm14\n\t"
       "vmovaps 480(%0), %%ymm15\n\t"
       : /* no output operands */
       : "r" (data)
       : "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15");
#endif

  asm ("nop"); /* first breakpoint here */

  asm (
       "vmovaps %%ymm0, 0(%0)\n\t"
       "vmovaps %%ymm1, 32(%0)\n\t"
       "vmovaps %%ymm2, 64(%0)\n\t"
       "vmovaps %%ymm3, 96(%0)\n\t"
       "vmovaps %%ymm4, 128(%0)\n\t"
       "vmovaps %%ymm5, 160(%0)\n\t"
       "vmovaps %%ymm6, 192(%0)\n\t"
       "vmovaps %%ymm7, 224(%0)\n\t"
       : /* no output operands */
       : "r" (data)
       : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7");
#ifdef __x86_64__
  asm (
       "vmovaps %%ymm8, 256(%0)\n\t"
       "vmovaps %%ymm9, 288(%0)\n\t"
       "vmovaps %%ymm10, 320(%0)\n\t"
       "vmovaps %%ymm11, 352(%0)\n\t"
       "vmovaps %%ymm12, 384(%0)\n\t"
       "vmovaps %%ymm13, 416(%0)\n\t"
       "vmovaps %%ymm14, 448(%0)\n\t"
       "vmovaps %%ymm15, 480(%0)\n\t"
       : /* no output operands */
       : "r" (data)
       : "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15");
#endif

  puts ("Bye!"); /* second breakpoint here */

  free (allocated_ptr);

  return 0;
}
