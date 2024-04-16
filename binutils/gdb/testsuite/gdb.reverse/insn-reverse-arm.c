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

static void
ext_reg_load (void)
{
  char in[8];

  asm ("vldr d0, [%0]" : : "r" (in));
  asm ("vldr s3, [%0]" : : "r" (in));

  asm ("vldm %0, {d3-d4}" : : "r" (in));
  asm ("vldm %0, {s9-s11}" : : "r" (in));
}

static void
ext_reg_mov (void)
{
  int i, j;
  double d;

  i = 1;
  j = 2;

  asm ("vmov s4, s5, %0, %1" : "=r" (i), "=r" (j): );
  asm ("vmov s7, s8, %0, %1" : "=r" (i), "=r" (j): );
  asm ("vmov %0, %1, s10, s11" : : "r" (i), "r" (j));
  asm ("vmov %0, %1, s1, s2" : : "r" (i), "r" (j));

  asm ("vmov %P2, %0, %1" : "=r" (i), "=r" (j): "w" (d));
  asm ("vmov %1, %2, %P0" : "=w" (d) : "r" (i), "r" (j));
}

static void
ext_reg_push_pop (void)
{
  double d;

  asm ("vpush {%P0}" : : "w" (d));
  asm ("vpop {%P0}" : : "w" (d));
}

/* Initialize arch-specific bits.  */

static void initialize (void)
{
  /* ARM doesn't currently use this function.  */
}

/* Functions testing instruction decodings.  GDB will test all of these.  */
static testcase_ftype testcases[] =
{
  ext_reg_load,
  ext_reg_mov,
  ext_reg_push_pop,
};
