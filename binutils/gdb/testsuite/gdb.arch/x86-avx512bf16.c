/* Test program for bfloat16 of AVX 512 registers.

   Copyright 2020-2024 Free Software Foundation, Inc.

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

typedef struct
{
  float f[4];
} v4sd_t;

typedef struct
{
  float f[8];
} v8sd_t;

typedef struct
{
  float f[16];
} v16sd_t;

v4sd_t xmm_data[] =
{
  { {  0.0,  0.125,  0.25,  0.375 } },
  { {  0.5,  0.625,  0.75,  0.875 } },
  { {  1.0,  1.125,  1.25,  1.375 } },
  { {  1.5,  1.625,  1.75,  1.875 } },
  { {  2.0,  2.125,  2.25,  2.375 } },
  { {  2.5,  2.625,  2.75,  2.875 } },
  { {  3.0,  3.125,  3.25,  3.375 } },
  { {  3.5,  3.625,  3.75,  3.875 } },
};

v8sd_t ymm_data[] =
{
  { {  8.0,  8.25,  8.5,  8.75,  9.0,  9.25,  9.5,  9.75 } },
  { { 10.0, 10.25, 10.5, 10.75, 11.0, 11.25, 11.5, 11.75 } },
  { { 12.0, 12.25, 12.5, 12.75, 13.0, 13.25, 13.5, 13.75 } },
  { { 14.0, 14.25, 14.5, 14.75, 15.0, 15.25, 15.5, 15.75 } },
  { { 16.0, 16.25, 16.5, 16.75, 17.0, 17.25, 17.5, 17.75 } },
  { { 18.0, 18.25, 18.5, 18.75, 19.0, 19.25, 19.5, 19.75 } },
  { { 20.0, 20.25, 20.5, 20.75, 21.0, 21.25, 21.5, 21.75 } },
  { { 22.0, 22.25, 22.5, 22.75, 23.0, 23.25, 23.5, 23.75 } },
};

v16sd_t zmm_data[] =
{
  { { 20.0,  20.5,  21.0,  21.5,  22.0,  22.5,  23.0,  23.5,  24.0,  24.5,
      25.0,  25.5,  26.0,  26.5,  27.0,  27.5 } },
  { { 28.0,  28.5,  29.0,  29.5,  30.0,  30.5,  31.0,  31.5,  32.0,  32.5,
      33.0,  33.5,  34.0,  34.5,  35.0,  35.5 } },
  { { 36.0,  36.5,  37.0,  37.5,  38.0,  38.5,  39.0,  39.5,  40.0,  40.5,
      41.0,  41.5,  42.0,  42.5,  43.0,  43.5 } },
  { { 44.0,  44.5,  45.0,  45.5,  46.0,  46.5,  47.0,  47.5,  48.0,  48.5,
      49.0,  49.5,  50.0,  50.5,  51.0,  51.5 } },
  { { 52.0,  52.5,  53.0,  53.5,  54.0,  54.5,  55.0,  55.5,  56.0,  56.5,
      57.0,  57.5,  58.0,  58.5,  59.0,  59.5 } },
  { { 60.0,  60.5,  61.0,  61.5,  62.0,  62.5,  63.0,  63.5,  64.0,  64.5,
      65.0,  65.5,  66.0,  66.5,  67.0,  67.5 } },
  { { 68.0,  68.5,  69.0,  69.5,  70.0,  70.5,  71.0,  71.5,  72.0,  72.5,
      73.0,  73.5,  74.0,  74.5,  75.0,  75.5 } },
  { { 76.0,  76.5,  77.0,  77.5,  78.0,  78.5,  79.0,  79.5,  80.0,  80.5,
      81.0,  81.5,  82.0,  82.5,  83.0,  83.5 } },
};

void
move_data_to_xmm_reg (void)
{
  asm ("vmovups 0(%0), %%xmm0 \n\t"
       "vmovups 16(%0), %%xmm1 \n\t"
       "vmovups 32(%0), %%xmm2 \n\t"
       "vmovups 48(%0), %%xmm3 \n\t"
       "vmovups 64(%0), %%xmm4 \n\t"
       "vmovups 80(%0), %%xmm5 \n\t"
       "vmovups 96(%0), %%xmm6 \n\t"
       "vmovups 112(%0), %%xmm7 \n\t"
       : /* no output operands  */
       : "r" (xmm_data));
}

void
move_data_to_ymm_reg (void)
{
  asm ("vmovups 0(%0), %%ymm0 \n\t"
       "vmovups 32(%0), %%ymm1 \n\t"
       "vmovups 64(%0), %%ymm2 \n\t"
       "vmovups 96(%0), %%ymm3 \n\t"
       "vmovups 128(%0), %%ymm4 \n\t"
       "vmovups 160(%0), %%ymm5 \n\t"
       "vmovups 192(%0), %%ymm6 \n\t"
       "vmovups 224(%0), %%ymm7 \n\t"
       : /* no output operands  */
       : "r" (ymm_data));
}

void
move_data_to_zmm_reg (void)
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
}

void
convert_xmm_from_float_to_bfloat16 (void)
{
  asm("vcvtne2ps2bf16 %xmm0, %xmm1, %xmm0");
  asm("vcvtne2ps2bf16 %xmm6, %xmm7, %xmm6");
}

void
convert_ymm_from_float_to_bfloat16 (void)
{
  asm("vcvtne2ps2bf16 %ymm0, %ymm1, %ymm0");
  asm("vcvtne2ps2bf16 %ymm6, %ymm7, %ymm6");
}

void
convert_zmm_from_float_to_bfloat16 (void)
{
  asm("vcvtne2ps2bf16 %zmm0, %zmm1, %zmm0");
  asm("vcvtne2ps2bf16 %zmm6, %zmm7, %zmm6");
}

int
main (int argc, char **argv)
{
  /* Move initial values from array to registers and read from XMM regs.  */
  move_data_to_xmm_reg ();
  convert_xmm_from_float_to_bfloat16 ();
  asm ("nop"); /* first breakpoint here  */

  /* Move initial values from array to registers and read from YMM regs.  */
  move_data_to_ymm_reg ();
  convert_ymm_from_float_to_bfloat16 ();
  asm ("nop"); /* second breakpoint here  */

  /* Move initial values from array to registers and read from ZMM regs.  */
  move_data_to_zmm_reg ();
  convert_zmm_from_float_to_bfloat16 ();
  asm ("nop"); /* third breakpoint here  */

  return 0;
}
