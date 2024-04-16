/* Tests the load and store instructions.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

# mach: or1k
# output: report(0xdeadbeef);\n
# output: report(0x00000000);\n
# output: report(0x7fffffff);\n
# output: report(0x80000000);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x7fffffff);\n
# output: report(0x80000000);\n
# output: report(0xffffffff);\n
# output: report(0xdeadbeef);\n
# output: report(0x00000000);\n
# output: report(0x7fffffff);\n
# output: report(0x80000000);\n
# output: report(0xdeadbeef);\n
# output: report(0x00000000);\n
# output: report(0x7fffffff);\n
# output: report(0x80000000);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x7fffffff);\n
# output: report(0x80000000);\n
# output: report(0xffffffff);\n
# output: report(0xdeadbeef);\n
# output: report(0x00000000);\n
# output: report(0x7fffffff);\n
# output: report(0x80000000);\n
# output: report(0x000000de);\n
# output: report(0x000000ad);\n
# output: report(0x000000be);\n
# output: report(0x000000ef);\n
# output: report(0x000000ef);\n
# output: report(0x000000be);\n
# output: report(0x000000ad);\n
# output: report(0x000000de);\n
# output: report(0xffffffde);\n
# output: report(0xffffffad);\n
# output: report(0xffffffbe);\n
# output: report(0xffffffef);\n
# output: report(0xffffffef);\n
# output: report(0xffffffbe);\n
# output: report(0xffffffad);\n
# output: report(0xffffffde);\n
# output: report(0x0000dead);\n
# output: report(0x0000beef);\n
# output: report(0x0000beef);\n
# output: report(0x0000dead);\n
# output: report(0xffffdead);\n
# output: report(0xffffbeef);\n
# output: report(0xffffbeef);\n
# output: report(0xffffdead);\n
# output: report(0xa1a2a3a4);\n
# output: report(0xb4b3b2b1);\n
# output: report(0x81828384);\n
# output: report(0x53545152);\n
# output: report(0xa0b0c0d0);\n
# output: report(0xa1b1c1d1);\n
# output: report(0xa3b3c3d3);\n
# output: report(0xa2b2c2d2);\n
# output: exit(0)\n

#include "or1k-asm-test-helpers.h"

	.macro TEST_LW  opcode, label, offset
	LOAD_IMMEDIATE r5, \label
	\opcode  r4, \offset(r5)
	REPORT_REG_TO_CONSOLE r4
	.endm

	STANDARD_TEST_ENVIRONMENT

	.section .rodata
	.balign 4

50:     .word   0xdeadbeef
51:     .word   0x00000000
52:     .word   0x7fffffff
53:     .word   0x80000000
54:     .word   0xffffffff

	.section .data
	.balign 4

buffer1:  .word 0x00000000
buffer2:  .word 0x00000000
buffer3:  .word 0x00000000
buffer4:  .word 0x00000000
buffer5:

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Test instruction l.lws  */

	/* Load with zero offset.  */
	TEST_LW l.lws 50b, 0
	TEST_LW l.lws 51b, 0
	TEST_LW l.lws 52b, 0
	TEST_LW l.lws 53b, 0
	TEST_LW l.lws 54b, 0

	/* Load with positive offset.  */
	TEST_LW l.lws 50b,  4
	TEST_LW l.lws 50b,  8
	TEST_LW l.lws 50b, 12
	TEST_LW l.lws 50b, 16

	/* Load with negative offset.  */
	TEST_LW l.lws 54b, -16
	TEST_LW l.lws 54b, -12
	TEST_LW l.lws 54b,  -8
	TEST_LW l.lws 54b,  -4

	/* TODO: add here test cases to cover unaligned memory accesses
	   with l.lws.  */

	/* Test instruction l.lwz */

	/* Load with zero offset.  */
	TEST_LW l.lwz 50b, 0
	TEST_LW l.lwz 51b, 0
	TEST_LW l.lwz 52b, 0
	TEST_LW l.lwz 53b, 0
	TEST_LW l.lwz 54b, 0

	/* Load with positive offset.  */
	TEST_LW l.lwz 50b,  4
	TEST_LW l.lwz 50b,  8
	TEST_LW l.lwz 50b, 12
	TEST_LW l.lwz 50b, 16

	/* Load with negative offset.  */
	TEST_LW l.lwz 54b, -16
	TEST_LW l.lwz 54b, -12
	TEST_LW l.lwz 54b,  -8
	TEST_LW l.lwz 54b,  -4

	/* TODO: add here test cases to cover unaligned memory accesses
	   with l.lwz.  */

	/* Test instruction l.lbz  */

	/* Read data at label 50, forwards, byte by byte.  */
	LOAD_IMMEDIATE r5, 50b

	l.lbz r4, 0(r5)
	REPORT_REG_TO_CONSOLE r4

	l.lbz r4, 1(r5)
	REPORT_REG_TO_CONSOLE r4

	l.lbz r4, 2(r5)
	REPORT_REG_TO_CONSOLE r4

	l.lbz r4, 3(r5)
	REPORT_REG_TO_CONSOLE r4

	/* Read data at label 50, backwards, byte by byte.  */
	LOAD_IMMEDIATE r31, 51b

	l.lbz r3, -1(r31)
	REPORT_REG_TO_CONSOLE r3

	l.lbz r3, -2(r31)
	REPORT_REG_TO_CONSOLE r3

	l.lbz r3, -3(r31)
	REPORT_REG_TO_CONSOLE r3

	l.lbz r3, -4(r31)
	REPORT_REG_TO_CONSOLE r3

	/* Test instruction l.lbs  */

	/* Read data at label 50, forwards, byte by byte.  */
	LOAD_IMMEDIATE r5, 50b

	l.lbs r4, 0(r5)
	REPORT_REG_TO_CONSOLE r4

	l.lbs r4, 1(r5)
	REPORT_REG_TO_CONSOLE r4

	l.lbs r4, 2(r5)
	REPORT_REG_TO_CONSOLE r4

	l.lbs r4, 3(r5)
	REPORT_REG_TO_CONSOLE r4

	/* Read data at label 50, backwards, byte by byte.  */
	LOAD_IMMEDIATE r31, 51b

	l.lbs r3, -1(r31)
	REPORT_REG_TO_CONSOLE r3

	l.lbs r3, -2(r31)
	REPORT_REG_TO_CONSOLE r3

	l.lbs r3, -3(r31)
	REPORT_REG_TO_CONSOLE r3

	l.lbs r3, -4(r31)
	REPORT_REG_TO_CONSOLE r3

	/* Test instruction l.lhz  */

	/* Read data at label 50, forwards, half-word by half-word.  */
	LOAD_IMMEDIATE r5, 50b

	l.lhz r4, 0(r5)
	REPORT_REG_TO_CONSOLE r4

	l.lhz r4, 2(r5)
	REPORT_REG_TO_CONSOLE r4

	/* Read data at label 50, backwards, half-word by half-word.  */
	LOAD_IMMEDIATE r31, 51b

	l.lhz r3, -2(r31)
	REPORT_REG_TO_CONSOLE r3

	l.lhz r3, -4(r31)
	REPORT_REG_TO_CONSOLE r3

	/* TODO: add here test cases to cover unaligned memory accesses
	   with l.lhz.  */

	/* Test instruction l.lhs  */

	/* Read data at label 50, forwards, half-word by half-word.  */
	LOAD_IMMEDIATE r5, 50b

	l.lhs r4, 0(r5)
	REPORT_REG_TO_CONSOLE r4

	l.lhs r4, 2(r5)
	REPORT_REG_TO_CONSOLE r4

	/* Read data at label 50, backwards, half-word by half-word.  */
	LOAD_IMMEDIATE r31, 51b

	l.lhs r3, -2(r31)
	REPORT_REG_TO_CONSOLE r3

	l.lhs r3, -4(r31)
	REPORT_REG_TO_CONSOLE r3

	/* TODO: add here test cases to cover unaligned memory accesses
	   with l.lhs.  */

	/* Test instruction l.sb  */

	/* Write 32-bits forwards, byte-to-byte.  */
	LOAD_IMMEDIATE r5, buffer1

	LOAD_IMMEDIATE r10, 0xA1
	LOAD_IMMEDIATE r11, 0xA2
	LOAD_IMMEDIATE r12, 0xA3
	LOAD_IMMEDIATE r13, 0xA4

	l.sb 0(r5), r10
	l.sb 1(r5), r11
	l.sb 2(r5), r12
	l.sb 3(r5), r13

	l.lwz r3, 0(r5)
	REPORT_REG_TO_CONSOLE r3

	/* Write 32-bits backwards, byte-to-byte.  */
	LOAD_IMMEDIATE r6, buffer2

	LOAD_IMMEDIATE r10, 0xB1
	LOAD_IMMEDIATE r11, 0xB2
	LOAD_IMMEDIATE r12, 0xB3
	LOAD_IMMEDIATE r13, 0xB4

	l.sb -1(r6), r10
	l.sb -2(r6), r11
	l.sb -3(r6), r12
	l.sb -4(r6), r13

	l.lwz r3, 0(r5)
	REPORT_REG_TO_CONSOLE r3

	/* TODO: add here test cases to cover unaligned memory accesses
	   with l.sb.  */

	/* Test instruction l.sh  */

	/* Write 32-bits forwards, one half-word at a time.  */
	LOAD_IMMEDIATE r5, buffer1

	LOAD_IMMEDIATE r10, 0x8182
	LOAD_IMMEDIATE r11, 0x8384

	l.sh 0(r5), r10
	l.sh 2(r5), r11

	l.lwz r3, 0(r5)
	REPORT_REG_TO_CONSOLE r3

	/* Write 32-bits backwards, one half-word at a time.  */
	LOAD_IMMEDIATE r6, buffer2

	LOAD_IMMEDIATE r10, 0x5152
	LOAD_IMMEDIATE r11, 0x5354

	l.sh -2(r6), r10
	l.sh -4(r6), r11

	l.lwz r3, 0(r5)
	REPORT_REG_TO_CONSOLE r3

	/* TODO: add here test cases to cover unaligned memory accesses
	   with l.sh.  */

	/* Test instruction l.sw  */
	LOAD_IMMEDIATE r5, buffer1
	LOAD_IMMEDIATE r6, buffer5

	LOAD_IMMEDIATE r10, 0xA0B0C0D0
	LOAD_IMMEDIATE r11, 0xA1B1C1D1
	LOAD_IMMEDIATE r12, 0xA2B2C2D2
	LOAD_IMMEDIATE r13, 0xA3B3C3D3

	l.sw  0(r5), r10
	l.sw  4(r5), r11
	l.sw -4(r6), r12
	l.sw -8(r6), r13

	TEST_LW l.lwz buffer1, 0
	TEST_LW l.lwz buffer2, 0
	TEST_LW l.lwz buffer3, 0
	TEST_LW l.lwz buffer4, 0

	/* TODO: add here test cases to cover unaligned memory accesses
	   with l.sw.  */

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
