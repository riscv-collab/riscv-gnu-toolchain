/* Copyright 2015-2024 Free Software Foundation, Inc.

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

#include <arm_neon.h>

#define DEF_FUNC1(N, TYPE, VALUE...)	\
  TYPE a##N = {VALUE};			\
  TYPE vec_func##N(TYPE a)		\
  { return a; }

/* 64-bit vector.  */

DEF_FUNC1(1, int8x8_t, -1, -2, 3, 4, 5, -6, 7, 8)
DEF_FUNC1(2, int16x4_t, 0, 2, -4, 6)
DEF_FUNC1(3, int32x2_t, -10, 12)
DEF_FUNC1(4, uint8x8_t, 1, 2, 3, 4, 5, 6, 7, 8)
DEF_FUNC1(5, uint16x4_t, 4, 3, 2, 1)
DEF_FUNC1(6, uint32x2_t, 100, 200)
DEF_FUNC1(7, float32x2_t, 1.0, 2.0)
DEF_FUNC1(8, poly8x8_t, 8, 10, 12, 14, 15, 16, 1, 0)
DEF_FUNC1(9, poly16x4_t, 32, 33, 34, 35)

/* 128-bit vector.  */

DEF_FUNC1(10, int8x16_t, -1, -2, 3, 4, 5, -6, 7, 8, 8, 10, 12, 14, 15, 16, 1, 0);
DEF_FUNC1(11, int16x8_t, 4, 10, -13, -16, 18, 1, 2, 4);
DEF_FUNC1(12, int32x4_t, 32, 33, -34, 35);
DEF_FUNC1(13, uint8x16_t, 1, 2, 3, 4, 5, 6, 7, 8, 8, 10, 12, 14, 15, 16, 1, 0);
DEF_FUNC1(14, uint16x8_t, 4, 10, 13, 16, 18, 1, 2, 4);
DEF_FUNC1(15, uint32x4_t, 16, 18, 1, 2);
DEF_FUNC1(16, float32x4_t, 2.0, 5.0, 4.0, 8.0);
DEF_FUNC1(17, poly8x16_t, 8, 10, 12, 14, 15, 16, 1, 0, 8, 10, 12, 14, 15, 16, 1, 0);
DEF_FUNC1(18, poly16x8_t, 8, 10, 12, 14, 15, 16, 1, 0);

/* Homogeneous Short Vector Aggregate.  */

struct hva1
{
  int8x8_t f1;
  int8x8_t f2;
  int8x8_t f3;
};

struct hva2
{
  int8x8_t f1;
  int16x4_t f2;
  int32x2_t f3;
};

struct hva3
{
  int8x8_t f1;
  int8x8_t f2;
  int8x8_t f3;
  int8x8_t f4;
  int16x4_t f5;
  int32x2_t f6;
};

struct hva1 hva1 = {{-1, -2, 3, 4, 5, -6, 7, 8},
		    {-1, -2, 3, 4, 5, -6, 7, 8},
		    {-1, -2, 3, 4, 5, -6, 7, 8}};

struct hva2 hva2 = {{-1, -2, 3, 4, 5, -6, 7, 8},
		    {0, 2, -4, 6},
		    {-10, 12}};

struct hva3 hva3 = {{-1, -2, 3, 4, 5, -6, 7, 8},
		    {-1, -2, 3, 4, 5, -6, 7, 8},
		    {-1, -2, 3, 4, 5, -6, 7, 8},
		    {-1, -2, 3, 4, 5, -6, 7, 8},
		    {0, 2, -4, 6},
		    {-10, 12}};

#define DEF_FUNC2(N)			  \
  struct hva##N hva_func##N(struct hva##N a) \
  { return a; }

DEF_FUNC2 (1)
DEF_FUNC2 (2)
DEF_FUNC2 (3)

int
main (void)
{
  return 0;
}
