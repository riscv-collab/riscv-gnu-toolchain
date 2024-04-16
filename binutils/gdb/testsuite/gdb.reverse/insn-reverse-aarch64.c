/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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

#include <arm_neon.h>

static void
load (void)
{
  int buf[8];

  asm ("ld1 { v1.8b }, [%[buf]]\n"
       "ld1 { v2.8b, v3.8b }, [%[buf]]\n"
       "ld1 { v3.8b, v4.8b, v5.8b }, [%[buf]]\n"
       :
       : [buf] "r" (buf)
       : /* No clobbers */);
}

static void
move (void)
{
  float32x2_t b1_ = vdup_n_f32(123.0f);
  float32_t a1_ = 0;
  float64x1_t b2_ = vdup_n_f64(456.0f);
  float64_t a2_ = 0;

  asm ("ins %0.s[0], %w1\n"
       : "=w"(b1_)
       : "r"(a1_), "0"(b1_)
       : /* No clobbers */);

  asm ("ins %0.d[1], %x1\n"
       : "=w"(b2_)
       : "r"(a2_), "0"(b2_)
       : /* No clobbers */);
}

static void
adv_simd_mod_imm (void)
{
  float32x2_t a1 = {2.0, 4.0};

  asm ("bic %0.2s, #1\n"
       "bic %0.2s, #1, lsl #8\n"
       : "=w"(a1)
       : "0"(a1)
       : /* No clobbers */);
}

static void
adv_simd_scalar_index (void)
{
  float64x2_t b_ = {0.0, 0.0};
  float64_t a_ = 1.0;
  float64_t result;

  asm ("fmla %d0,%d1,%2.d[1]"
       : "=w"(result)
       : "w"(a_), "w"(b_)
       : /* No clobbers */);
}

static void
adv_simd_smlal (void)
{
  asm ("smlal v13.2d, v8.2s, v0.2s");
}

static void
adv_simd_vect_shift (void)
{
  asm ("fcvtzs s0, s0, #1");
}

/* Initialize arch-specific bits.  */

static void initialize (void)
{
  /* AArch64 doesn't currently use this function.  */
}

/* Functions testing instruction decodings.  GDB will test all of these.  */
static testcase_ftype testcases[] =
{
  load,
  move,
  adv_simd_mod_imm,
  adv_simd_scalar_index,
  adv_simd_smlal,
  adv_simd_vect_shift,
};
