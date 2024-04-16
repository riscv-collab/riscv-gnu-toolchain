/* Test program for SSE registers.

   Copyright 2004-2024 Free Software Foundation, Inc.

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

/* Align sufficient to be able to use movaps.  */
#define ALIGN 16

typedef struct {
  _Alignas (ALIGN) float f[4];
} v4sf_t;


v4sf_t data_orig[] =
  {
    { {  0.0,  0.25,  0.50,  0.75 } },
    { {  1.0,  1.25,  1.50,  1.75 } },
    { {  2.0,  2.25,  2.50,  2.75 } },
    { {  3.0,  3.25,  3.50,  3.75 } },
    { {  4.0,  4.25,  4.50,  4.75 } },
    { {  5.0,  5.25,  5.50,  5.75 } },
    { {  6.0,  6.25,  6.50,  6.75 } },
    { {  7.0,  7.25,  7.50,  7.75 } },
#ifdef __x86_64__
    { {  8.0,  8.25,  8.50,  8.75 } },
    { {  9.0,  9.25,  9.50,  9.75 } },
    { { 10.0, 10.25, 10.50, 10.75 } },
    { { 11.0, 11.25, 11.50, 11.75 } },
    { { 12.0, 12.25, 12.50, 12.75 } },
    { { 13.0, 13.25, 13.50, 13.75 } },
    { { 14.0, 14.25, 14.50, 14.75 } },
    { { 15.0, 15.25, 15.50, 15.75 } },
#endif
  };


int
have_sse (void)
{
  unsigned int edx;

  if (!x86_cpuid (1, NULL, NULL, NULL, &edx))
    return 0;

  if (edx & bit_SSE)
    return 1;
  else
    return 0;
}

#include "precise-aligned-alloc.c"

int
main (int argc, char **argv)
{
  void *allocated_ptr;
  v4sf_t *data
    = precise_aligned_dup (ALIGN, sizeof (data_orig), &allocated_ptr,
			   data_orig);

  if (have_sse ())
    {
      asm ("movaps 0(%0), %%xmm0\n\t"
           "movaps 16(%0), %%xmm1\n\t"
           "movaps 32(%0), %%xmm2\n\t"
           "movaps 48(%0), %%xmm3\n\t"
           "movaps 64(%0), %%xmm4\n\t"
           "movaps 80(%0), %%xmm5\n\t"
           "movaps 96(%0), %%xmm6\n\t"
           "movaps 112(%0), %%xmm7\n\t"
           : /* no output operands */
           : "r" (data) 
           : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7");
#ifdef __x86_64__
      asm ("movaps 128(%0), %%xmm8\n\t"
           "movaps 144(%0), %%xmm9\n\t"
           "movaps 160(%0), %%xmm10\n\t"
           "movaps 176(%0), %%xmm11\n\t"
           "movaps 192(%0), %%xmm12\n\t"
           "movaps 208(%0), %%xmm13\n\t"
           "movaps 224(%0), %%xmm14\n\t"
           "movaps 240(%0), %%xmm15\n\t"
           : /* no output operands */
           : "r" (data) 
           : "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15");
#endif

      asm ("nop"); /* first breakpoint here */

      asm (
           "movaps %%xmm0, 0(%0)\n\t"
           "movaps %%xmm1, 16(%0)\n\t"
           "movaps %%xmm2, 32(%0)\n\t"
           "movaps %%xmm3, 48(%0)\n\t"
           "movaps %%xmm4, 64(%0)\n\t"
           "movaps %%xmm5, 80(%0)\n\t"
           "movaps %%xmm6, 96(%0)\n\t"
           "movaps %%xmm7, 112(%0)\n\t"
           : /* no output operands */
           : "r" (data) 
           : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7");
#ifdef __x86_64__
      asm (
           "movaps %%xmm8, 128(%0)\n\t"
           "movaps %%xmm9, 144(%0)\n\t"
           "movaps %%xmm10, 160(%0)\n\t"
           "movaps %%xmm11, 176(%0)\n\t"
           "movaps %%xmm12, 192(%0)\n\t"
           "movaps %%xmm13, 208(%0)\n\t"
           "movaps %%xmm14, 224(%0)\n\t"
           "movaps %%xmm15, 240(%0)\n\t"
           : /* no output operands */
           : "r" (data) 
           : "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15");
#endif

      puts ("Bye!"); /* second breakpoint here */
    }

  free (allocated_ptr);

  return 0;
}
