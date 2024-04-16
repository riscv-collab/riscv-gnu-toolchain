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

/* Globals used for vector tests.  */
static vector unsigned long vec_xa, vec_xb, vec_xt;
static unsigned long ra, rb, rs;

int
main ()
{

  /* This test is used to verify the recording of the MMA instructions.  The
     names of the MMA instructions pmxbf16ger*, pmxvf32ger*,pmxvf64ger*,
     pmxvi4ger8*, pmxvi8ger4* pmxvi16ger2* instructions were officially changed
     to pmdmxbf16ger*, pmdmxvf32ger*, pmdmxvf64ger*, pmdmxvi4ger8*,
     pmdmxvi8ger4*, pmdmxvi16ger* respectively.  The old mnemonics are used in
     this test for backward compatibity.   */
  ra = 0xABCDEF012;
  rb = 0;
  rs = 0x012345678;

  /* 9.0, 16.0, 25.0, 36.0 */
  vec_xb = (vector unsigned long){0x4110000041800000, 0x41c8000042100000};

  vec_xt = (vector unsigned long){0xFF00FF00FF00FF00, 0xAA00AA00AA00AA00};

  /* Test 1, ISA 3.1 word instructions. Load source into r1, result of brh
     put in r0.  */
  ra = 0xABCDEF012;                     /* stop 1 */
  __asm__ __volatile__ ("pld 1, %0" :: "r" (ra ));
  __asm__ __volatile__ ("brh 0, 1" );
  ra = 0;                               /* stop 2 */

  /* Test 2, ISA 3.1 MMA instructions with results in various ACC entries
     xxsetaccz    - ACC[3]
     xvi4ger8     - ACC[4]
     xvf16ger2pn  - ACC[5]
     pmxvi8ger4   - ACC[6]
     pmxvf32gerpp - ACC[7] and fpscr */
  /* Need to initialize the vs registers to a non zero value.  */
  ra = (unsigned long) & vec_xb;
  __asm__ __volatile__ ("lxvd2x 12, %0, %1" :: "r" (ra ), "r" (rb));
  __asm__ __volatile__ ("lxvd2x 13, %0, %1" :: "r" (ra ), "r" (rb));
  __asm__ __volatile__ ("lxvd2x 14, %0, %1" :: "r" (ra ), "r" (rb));
  __asm__ __volatile__ ("lxvd2x 15, %0, %1" :: "r" (ra ), "r" (rb));
  vec_xa = (vector unsigned long){0x333134343987601, 0x9994bbbc9983307};
  vec_xb = (vector unsigned long){0x411234041898760, 0x41c833042103400};
  __asm__ __volatile__ ("lxvd2x 16, %0, %1" :: "r" (ra ), "r" (rb));
  vec_xb = (vector unsigned long){0x123456789987650, 0x235676546989807};
  __asm__ __volatile__ ("lxvd2x 17, %0, %1" :: "r" (ra ), "r" (rb));
  vec_xb = (vector unsigned long){0x878363439823470, 0x413434c99839870};
  __asm__ __volatile__ ("lxvd2x 18, %0, %1" :: "r" (ra ), "r" (rb));
  vec_xb = (vector unsigned long){0x043765434398760, 0x419876555558850};
  __asm__ __volatile__ ("lxvd2x 19, %0, %1" :: "r" (ra ), "r" (rb));
  vec_xb = (vector unsigned long){0x33313434398760, 0x9994bbbc99899330};
  __asm__ __volatile__ ("lxvd2x 20, %0, %1" :: "r" (ra ), "r" (rb));
  __asm__ __volatile__ ("lxvd2x 21, %0, %1" :: "r" (ra ), "r" (rb));
  __asm__ __volatile__ ("lxvd2x 22, %0, %1" :: "r" (ra ), "r" (rb));
  __asm__ __volatile__ ("lxvd2x 23, %0, %1" :: "r" (ra ), "r" (rb));
  __asm__ __volatile__ ("lxvd2x 24, %0, %1" :: "r" (ra ), "r" (rb));
  __asm__ __volatile__ ("lxvd2x 25, %0, %1" :: "r" (ra ), "r" (rb));
  __asm__ __volatile__ ("lxvd2x 26, %0, %1" :: "r" (ra ), "r" (rb));
  __asm__ __volatile__ ("lxvd2x 27, %0, %1" :: "r" (ra ), "r" (rb));
  vec_xa = (vector unsigned long){0x33313434398760, 0x9994bbbc998330};
  vec_xb = (vector unsigned long){0x4110000041800000, 0x41c8000042100000};
  __asm__ __volatile__ ("lxvd2x 28, %0, %1" :: "r" (ra ), "r" (rb));
  vec_xb = (vector unsigned long){0x4567000046800000, 0x4458000048700000};
  __asm__ __volatile__ ("lxvd2x 29, %0, %1" :: "r" (ra ), "r" (rb));
  vec_xb = (vector unsigned long){0x41dd000041e00000, 0x41c8000046544400};
  __asm__ __volatile__ ("lxvd2x 30, %0, %1" :: "r" (ra ), "r" (rb));

  /* SNAN */
  vec_xb = (vector unsigned long){0x7F8F00007F8F0000, 0x7F8F00007F8F0000};

  __asm__ __volatile__ ("lxvd2x 31, %0, %1" :: "r" (ra ), "r" (rb));

  ra = 0xAB;                            /* stop 3 */
  __asm__ __volatile__ ("xxsetaccz 3");
  __asm__ __volatile__ ("xvi4ger8 4, %x0, %x1" :: "wa" (vec_xa), \
			"wa" (vec_xb) );
  __asm__ __volatile__ ("xvf16ger2pn 5, %x0, %x1" :: "wa" (vec_xa),\
			"wa" (vec_xb) );
  /* Use the older instruction name for backward compatibility */
  __asm__ __volatile__ ("pmxvi8ger4spp  6, %x0, %x1, 11, 13, 5"
                                :: "wa" (vec_xa), "wa" (vec_xb) );
  __asm__ __volatile__ ("pmxvf32gerpp  7, %x0, %x1, 11, 13"
                                :: "wa" (vec_xa), "wa" (vec_xb) );
  ra = 0;                               /* stop 4 */
}
