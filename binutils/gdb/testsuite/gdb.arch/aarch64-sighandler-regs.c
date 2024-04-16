/* This file is part of GDB, the GNU debugger.

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

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#define OVERWRITE_GP_REGS \
		    "ldr x1, [x0]\n\t" \
		    "ldr x2, [x0]\n\t" \
		    "ldr x3, [x0]\n\t" \
		    "ldr x4, [x0]\n\t" \
		    "ldr x5, [x0]\n\t" \
		    "ldr x6, [x0]\n\t" \
		    "ldr x7, [x0]\n\t" \
		    "ldr x8, [x0]\n\t" \
		    "ldr x9, [x0]\n\t" \
		    "ldr x10, [x0]\n\t" \
		    "ldr x11, [x0]\n\t" \
		    "ldr x12, [x0]\n\t" \
		    "ldr x13, [x0]\n\t" \
		    "ldr x14, [x0]\n\t" \
		    "ldr x15, [x0]\n\t" \
		    "ldr x16, [x0]\n\t" \
		    "ldr x17, [x0]\n\t" \
		    "ldr x18, [x0]\n\t" \
		    "ldr x19, [x0]\n\t" \
		    "ldr x20, [x0]\n\t" \
		    "ldr x21, [x0]\n\t" \
		    "ldr x22, [x0]\n\t" \
		    "ldr x23, [x0]\n\t" \
		    "ldr x24, [x0]\n\t" \
		    "ldr x25, [x0]\n\t" \
		    "ldr x26, [x0]\n\t" \
		    "ldr x27, [x0]\n\t" \
		    "ldr x28, [x0]\n\t"

#ifdef SVE
#define OVERWRITE_FP_REGS \
		    "ptrue p3.s\n\t" \
		    "ld1w z0.s, p3/z, [x0]\n\t" \
		    "ld1w z1.s, p3/z, [x0]\n\t" \
		    "ld1w z2.s, p3/z, [x0]\n\t" \
		    "ld1w z3.s, p3/z, [x0]\n\t" \
		    "ld1w z4.s, p3/z, [x0]\n\t" \
		    "ld1w z5.s, p3/z, [x0]\n\t" \
		    "ld1w z6.s, p3/z, [x0]\n\t" \
		    "ld1w z7.s, p3/z, [x0]\n\t" \
		    "ld1w z8.s, p3/z, [x0]\n\t" \
		    "ld1w z9.s, p3/z, [x0]\n\t" \
		    "ld1w z10.s, p3/z, [x0]\n\t" \
		    "ld1w z11.s, p3/z, [x0]\n\t" \
		    "ld1w z12.s, p3/z, [x0]\n\t" \
		    "ld1w z13.s, p3/z, [x0]\n\t" \
		    "ld1w z14.s, p3/z, [x0]\n\t" \
		    "ld1w z15.s, p3/z, [x0]\n\t" \
		    "ld1w z16.s, p3/z, [x0]\n\t" \
		    "ld1w z17.s, p3/z, [x0]\n\t" \
		    "ld1w z18.s, p3/z, [x0]\n\t" \
		    "ld1w z19.s, p3/z, [x0]\n\t" \
		    "ld1w z20.s, p3/z, [x0]\n\t" \
		    "ld1w z21.s, p3/z, [x0]\n\t" \
		    "ld1w z22.s, p3/z, [x0]\n\t" \
		    "ld1w z23.s, p3/z, [x0]\n\t" \
		    "ld1w z24.s, p3/z, [x0]\n\t" \
		    "ld1w z25.s, p3/z, [x0]\n\t" \
		    "ld1w z26.s, p3/z, [x0]\n\t" \
		    "ld1w z27.s, p3/z, [x0]\n\t" \
		    "ld1w z28.s, p3/z, [x0]\n\t" \
		    "ld1w z29.s, p3/z, [x0]\n\t" \
		    "ld1w z30.s, p3/z, [x0]\n\t" \
		    "ld1w z31.s, p3/z, [x0]\n\t"
#else
#define OVERWRITE_FP_REGS \
		    "ldr q0, [x0]\n\t" \
		    "ldr q1, [x0]\n\t" \
		    "ldr q2, [x0]\n\t" \
		    "ldr q3, [x0]\n\t" \
		    "ldr q4, [x0]\n\t" \
		    "ldr q5, [x0]\n\t" \
		    "ldr q6, [x0]\n\t" \
		    "ldr q7, [x0]\n\t" \
		    "ldr q8, [x0]\n\t" \
		    "ldr q9, [x0]\n\t" \
		    "ldr q10, [x0]\n\t" \
		    "ldr q11, [x0]\n\t" \
		    "ldr q12, [x0]\n\t" \
		    "ldr q13, [x0]\n\t" \
		    "ldr q14, [x0]\n\t" \
		    "ldr q15, [x0]\n\t" \
		    "ldr q16, [x0]\n\t" \
		    "ldr q17, [x0]\n\t" \
		    "ldr q18, [x0]\n\t" \
		    "ldr q19, [x0]\n\t" \
		    "ldr q20, [x0]\n\t" \
		    "ldr q21, [x0]\n\t" \
		    "ldr q22, [x0]\n\t" \
		    "ldr q23, [x0]\n\t" \
		    "ldr q24, [x0]\n\t" \
		    "ldr q25, [x0]\n\t" \
		    "ldr q26, [x0]\n\t" \
		    "ldr q27, [x0]\n\t" \
		    "ldr q28, [x0]\n\t" \
		    "ldr q29, [x0]\n\t" \
		    "ldr q30, [x0]\n\t" \
		    "ldr q31, [x0]\n\t"
#endif

#ifdef SVE
#define OVERWRITE_P_REGS(pattern) \
		    "ptrue p0.s, " #pattern "\n\t" \
		    "ptrue p1.s, " #pattern "\n\t" \
		    "ptrue p2.s, " #pattern "\n\t" \
		    "ptrue p3.s, " #pattern "\n\t" \
		    "ptrue p4.s, " #pattern "\n\t" \
		    "ptrue p5.s, " #pattern "\n\t" \
		    "ptrue p6.s, " #pattern "\n\t" \
		    "ptrue p7.s, " #pattern "\n\t" \
		    "ptrue p8.s, " #pattern "\n\t" \
		    "ptrue p9.s, " #pattern "\n\t" \
		    "ptrue p10.s, " #pattern "\n\t" \
		    "ptrue p11.s, " #pattern "\n\t" \
		    "ptrue p12.s, " #pattern "\n\t" \
		    "ptrue p13.s, " #pattern "\n\t" \
		    "ptrue p14.s, " #pattern "\n\t" \
		    "ptrue p15.s, " #pattern "\n\t"
#else
#define OVERWRITE_P_REGS(pattern)
#endif


void
handler (int sig)
{
  char buf_handler[] = {0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
			0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f};

  __asm __volatile ("mov x0, %0\n\t" \
		    OVERWRITE_GP_REGS \
		    OVERWRITE_FP_REGS \
		    OVERWRITE_P_REGS(MUL3) \
		    : : "r" (buf_handler));

  exit(0);
}



int
main ()
{
  /* Ensure all the signals aren't blocked.  */
  sigset_t newset;
  sigemptyset (&newset);
  sigprocmask (SIG_SETMASK, &newset, NULL);

  signal (SIGILL, handler);

  char buf_main[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		     0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

  /* 0x06000000 : Cause an illegal instruction. Value undefined as per ARM
     Architecture Reference Manual ARMv8, Section C4.1.  */

  __asm __volatile ("mov x0, %0\n\t" \
		    OVERWRITE_GP_REGS \
		    OVERWRITE_FP_REGS \
		    OVERWRITE_P_REGS(VL1) \
		    ".inst 0x06000000"
		    : : "r" (buf_main));

  return 0;
}
