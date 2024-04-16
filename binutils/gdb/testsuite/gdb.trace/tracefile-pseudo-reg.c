/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2024 Free Software Foundation, Inc.

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

/*
 * Test program for reading target description from tfile: collects AVX
 * registers on x86_64.
 */

#include <immintrin.h>

void
dummy (void)
{
}

static void
end (void)
{
}

int
main (void)
{
  /* Strictly speaking, it should be ymm15 (xmm15 is 128-bit), but gcc older
     than 4.9 doesn't recognize "ymm15" as a valid register name.  */
  register __v8si a asm("xmm15") = {
    0x12340001,
    0x12340002,
    0x12340003,
    0x12340004,
    0x12340005,
    0x12340006,
    0x12340007,
    0x12340008,
  };
  asm volatile ("traceme: call dummy" : : "x" (a));
  end ();
  return 0;
}
