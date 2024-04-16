/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

/* globals used for vector tests */
static vector unsigned long vec_xb;
static unsigned long ra, rb, rs;

int
main ()
{
  ra = 0xABCDEF012;
  rb = 0;
  rs = 0x012345678;

  /* 9.0, 16.0, 25.0, 36.0 */
  vec_xb = (vector unsigned long){0x4110000041800000, 0x41c8000042100000};

  /* Test ISA 2.06 instructions.  Load source into vs1, result of sqrt
     put into vs0.  */
  ra = (unsigned long) & vec_xb;        /* stop 1 */
  __asm__ __volatile__ ("lxvd2x 1, %0, %1" :: "r" (ra ), "r" (rb));
  __asm__ __volatile__ ("xvsqrtsp 0, 1");
  ra = 0;                               /* stop 2 */
}

