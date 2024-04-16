/* This test is part of GDB, the GNU debugger.

   Copyright 2018-2024 Free Software Foundation, Inc.

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

#include <stdint.h>

int __attribute__ ((nomips16))
main (void)
{
  /* We need to use a complex type to avoid hitting GCC's limit of
     the number of `asm' operands:

     mips-fpregset-core.f: In function 'main':
     mips-fpregset-core.f:66:3: error: more than 30 operands in 'asm'
        asm (
        ^~~

     and still have complete 32 FGR coverage with FP64 ABIs.  Using
     a complex type breaks o32 GCC though, so use plain `double' with
     FP32.  */
#if _MIPS_FPSET == 32
  typedef double _Complex float_t;
#else
  typedef double float_t;
#endif
  union
    {
      uint64_t i[32];
      float_t f[256 / sizeof (float_t)];
    }
  u =
    { .i =
      {
	0x0112233445566778, 0x899aabbccddeeff0,
	0x0213243546576879, 0x8a9bacbdcedfe0f1,
	0x031425364758697a, 0x8b9cadbecfd0e1f2,
	0x0415263748596a7b, 0x8c9daebfc0d1e2f3,
	0x05162738495a6b7c, 0x8d9eafb0c1d2e3f4,
	0x061728394a5b6c7d, 0x8e9fa0b1c2d3e4f5,
	0x0718293a4b5c6d7e, 0x8f90a1b2c3d4e5f6,
	0x08192a3b4c5d6e7f, 0x8091a2b3c4d5e6f7,
	0x091a2b3c4d5e6f70, 0x8192a3b4c5d6e7f8,
	0x0a1b2c3d4e5f6071, 0x8293a4b5c6d7e8f9,
	0x0b1c2d3e4f506172, 0x8394a5b6c7d8e9fa,
	0x0c1d2e3f40516273, 0x8495a6b7c8d9eafb,
	0x0d1e2f3041526374, 0x8596a7b8c9daebfc,
	0x0e1f203142536475, 0x8697a8b9cadbecfd,
	0x0f10213243546576, 0x8798a9bacbdcedfe,
	0x0011223344556677, 0x8899aabbccddeeff
      }
    };

  asm (
    ".globl\tbreak_here\n\t"
    ".aent\tbreak_here\n"
    "break_here:\n\t"
    "lb\t$0,0($0)\n"
    :
    : [f0] "f" (u.f[0]), [f2] "f" (u.f[1]),
      [f4] "f" (u.f[2]), [f6] "f" (u.f[3]),
      [f8] "f" (u.f[4]), [f10] "f" (u.f[5]),
      [f12] "f" (u.f[6]), [f14] "f" (u.f[7]),
      [f16] "f" (u.f[8]), [f18] "f" (u.f[9]),
      [f20] "f" (u.f[10]), [f22] "f" (u.f[11]),
      [f24] "f" (u.f[12]), [f26] "f" (u.f[13]),
      [f28] "f" (u.f[14]), [f30] "f" (u.f[15]));

  return 0;
}
