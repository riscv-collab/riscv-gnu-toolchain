/* Test program for AVX 512 registers.

   Copyright 2014-2024 Free Software Foundation, Inc.

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

#include "x86-cpuid.h"

typedef struct
{
  double f[8];
} v8sd_t;

short k_data[] =
{
  0x1211,
  0x2221,
  0x3231,
  0x4241,
  0x5251,
  0x6261,
  0x7271
};

v8sd_t zmm_data[] =
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
  { { 16.0, 16.125, 16.25, 16.375, 16.50, 16.625, 16.75, 16.875 } },
  { { 17.0, 17.125, 17.25, 17.375, 17.50, 17.625, 17.75, 17.875 } },
  { { 18.0, 18.125, 18.25, 18.375, 18.50, 18.625, 18.75, 18.875 } },
  { { 19.0, 19.125, 19.25, 19.375, 19.50, 19.625, 19.75, 19.875 } },
  { { 20.0, 20.125, 20.25, 20.375, 20.50, 20.625, 20.75, 20.875 } },
  { { 21.0, 21.125, 21.25, 21.375, 21.50, 21.625, 21.75, 21.875 } },
  { { 22.0, 22.125, 22.25, 22.375, 22.50, 22.625, 22.75, 22.875 } },
  { { 23.0, 23.125, 23.25, 23.375, 23.50, 23.625, 23.75, 23.875 } },
  { { 24.0, 24.125, 24.25, 24.375, 24.50, 24.625, 24.75, 24.875 } },
  { { 25.0, 25.125, 25.25, 25.375, 25.50, 25.625, 25.75, 25.875 } },
  { { 26.0, 26.125, 26.25, 26.375, 26.50, 26.625, 26.75, 26.875 } },
  { { 27.0, 27.125, 27.25, 27.375, 27.50, 27.625, 27.75, 27.875 } },
  { { 28.0, 28.125, 28.25, 28.375, 28.50, 28.625, 28.75, 28.875 } },
  { { 29.0, 29.125, 29.25, 29.375, 29.50, 29.625, 29.75, 29.875 } },
  { { 30.0, 30.125, 30.25, 30.375, 30.50, 30.625, 30.75, 30.875 } },
  { { 31.0, 31.125, 31.25, 31.375, 31.50, 31.625, 31.75, 31.875 } },
  { { 32.0, 32.125, 32.25, 32.375, 32.50, 32.625, 32.75, 32.875 } },
#endif
};

int
have_avx512 (void)
{
  unsigned int eax, ebx, ecx, edx, max_level, vendor, has_osxsave, has_avx512f;

  max_level = __get_cpuid_max (0, &vendor);
  __cpuid (1, eax, ebx, ecx, edx);

  has_osxsave = ecx & bit_OSXSAVE;

  if (max_level >= 7)
    {
      __cpuid_count (7, 0, eax, ebx, ecx, edx);
      has_avx512f = ebx & bit_AVX512F;
    }

  if (has_osxsave && has_avx512f)
    return 1;
  else
    return 0;
}

void
move_k_data_to_reg (void)
{
  asm ("kmovw 0(%0), %%k1\n\t"
       "kmovw 2(%0), %%k2\n\t"
       "kmovw 4(%0), %%k3\n\t"
       "kmovw 6(%0), %%k4\n\t"
       "kmovw 8(%0), %%k5\n\t"
       "kmovw 10(%0), %%k6\n\t"
       "kmovw 12(%0), %%k7\n\t"
       : /* no output operands  */
       : "r" (k_data));
}

void
move_k_data_to_memory (void)
{
  asm ("kmovw %%k1, 0(%0) \n\t"
       "kmovw %%k2, 2(%0) \n\t"
       "kmovw %%k3, 4(%0) \n\t"
       "kmovw %%k4, 6(%0) \n\t"
       "kmovw %%k5, 8(%0) \n\t"
       "kmovw %%k6, 10(%0) \n\t"
       "kmovw %%k7, 12(%0) \n\t"
       : /* no output operands  */
       : "r" (k_data));
}

void
move_zmm_data_to_reg (void)
{
  asm ("vmovups 0(%0), %%zmm0 \n\t"
       "vmovups 64(%0), %%zmm1 \n\t"
       "vmovups 128(%0), %%zmm2 \n\t"
       "vmovups 192(%0), %%zmm3 \n\t"
       "vmovups 256(%0), %%zmm4 \n\t"
       "vmovups 320(%0), %%zmm5 \n\t"
       "vmovups 384(%0), %%zmm6 \n\t"
       "vmovups 448(%0), %%zmm7 \n\t"
       : /* no output operands  */
       : "r" (zmm_data));
#ifdef __x86_64__
  asm ("vmovups 512(%0), %%zmm8 \n\t"
       "vmovups 576(%0), %%zmm9 \n\t"
       "vmovups 640(%0), %%zmm10 \n\t"
       "vmovups 704(%0), %%zmm11 \n\t"
       "vmovups 768(%0), %%zmm12 \n\t"
       "vmovups 832(%0), %%zmm13 \n\t"
       "vmovups 896(%0), %%zmm14 \n\t"
       "vmovups 960(%0), %%zmm15 \n\t"
       : /* no output operands  */
       : "r" (zmm_data));

  asm ("vmovups 1024(%0), %%zmm16 \n\t"
       "vmovups 1088(%0), %%zmm17 \n\t"
       "vmovups 1152(%0), %%zmm18 \n\t"
       "vmovups 1216(%0), %%zmm19 \n\t"
       "vmovups 1280(%0), %%zmm20 \n\t"
       "vmovups 1344(%0), %%zmm21 \n\t"
       "vmovups 1408(%0), %%zmm22 \n\t"
       "vmovups 1472(%0), %%zmm23 \n\t"
       "vmovups 1536(%0), %%zmm24 \n\t"
       "vmovups 1600(%0), %%zmm25 \n\t"
       "vmovups 1664(%0), %%zmm26 \n\t"
       "vmovups 1728(%0), %%zmm27 \n\t"
       "vmovups 1792(%0), %%zmm28 \n\t"
       "vmovups 1856(%0), %%zmm29 \n\t"
       "vmovups 1920(%0), %%zmm30 \n\t"
       "vmovups 1984(%0), %%zmm31 \n\t"
       : /* no output operands  */
       : "r" (zmm_data));
#endif
}

void
move_zmm_data_to_memory (void)
{
  asm ("vmovups %%zmm0, 0(%0)\n\t"
       "vmovups %%zmm1, 64(%0)\n\t"
       "vmovups %%zmm2, 128(%0)\n\t"
       "vmovups %%zmm3, 192(%0)\n\t"
       "vmovups %%zmm4, 256(%0)\n\t"
       "vmovups %%zmm5, 320(%0)\n\t"
       "vmovups %%zmm6, 384(%0)\n\t"
       "vmovups %%zmm7, 448(%0)\n\t"
       : /* no output operands  */
       : "r" (zmm_data));
#ifdef __x86_64__
  asm ("vmovups %%zmm8, 512(%0)\n\t"
       "vmovups %%zmm9, 576(%0)\n\t"
       "vmovups %%zmm10, 640(%0)\n\t"
       "vmovups %%zmm11, 704(%0)\n\t"
       "vmovups %%zmm12, 768(%0)\n\t"
       "vmovups %%zmm13, 832(%0)\n\t"
       "vmovups %%zmm14, 896(%0)\n\t"
       "vmovups %%zmm15, 960(%0)\n\t"
       : /* no output operands  */
       : "r" (zmm_data));

  asm ("vmovups %%zmm16, 1024(%0)\n\t"
       "vmovups %%zmm17, 1088(%0)\n\t"
       "vmovups %%zmm18, 1152(%0)\n\t"
       "vmovups %%zmm19, 1216(%0)\n\t"
       "vmovups %%zmm20, 1280(%0)\n\t"
       "vmovups %%zmm21, 1344(%0)\n\t"
       "vmovups %%zmm22, 1408(%0)\n\t"
       "vmovups %%zmm23, 1472(%0)\n\t"
       "vmovups %%zmm24, 1536(%0)\n\t"
       "vmovups %%zmm25, 1600(%0)\n\t"
       "vmovups %%zmm26, 1664(%0)\n\t"
       "vmovups %%zmm27, 1728(%0)\n\t"
       "vmovups %%zmm28, 1792(%0)\n\t"
       "vmovups %%zmm29, 1856(%0)\n\t"
       "vmovups %%zmm30, 1920(%0)\n\t"
       "vmovups %%zmm31, 1984(%0)\n\t"
       : /* no output operands  */
       : "r" (zmm_data));
#endif
}

int
main (int argc, char **argv)
{
  if (have_avx512 ())
    {
      /* Test for K registers.  */
      move_k_data_to_reg ();
      asm ("nop"); /* first breakpoint here  */

      move_k_data_to_memory ();
      asm ("nop"); /* second breakpoint here  */

      /* Test for ZMM registers.  */
      /* Move initial values from array to registers and read from ZMM regs.  */
      move_zmm_data_to_reg ();
      asm ("nop"); /* third breakpoint here  */

      /* Test script incremented values,
	 move back to array and check values.  */
      move_zmm_data_to_memory ();
      asm ("nop"); /* fourth breakpoint here  */

      /* Test for YMM registers.  */
      /* Test script incremented YMM values,
	 move back to array and check values.  */
      move_zmm_data_to_memory ();
      asm ("nop"); /* fifth breakpoint here  */

      /* Test for XMM registers.  */
      /* Test script incremented XMM values,
	 move back to array and check values.  */
      move_zmm_data_to_memory ();
      asm ("nop"); /* sixth breakpoint here  */

      asm ("vpternlogd $0xff, %zmm0, %zmm0, %zmm0");
#ifdef __x86_64__
      asm ("vpternlogd $0xff, %zmm0, %zmm0, %zmm16");
#endif
      asm ("vzeroupper");
      asm ("nop"); /* seventh breakpoint here  */
    }

  return 0;
}
