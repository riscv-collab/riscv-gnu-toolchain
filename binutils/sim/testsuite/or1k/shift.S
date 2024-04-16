/* Tests the shift instructions.

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
# output: report(0xb38f0f83);\n
# output: report(0x00000000);\n
# output: report(0xb38f0f83);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000001);\n
# output: report(0x671e1f06);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000004);\n
# output: report(0x38f0f830);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000010);\n
# output: report(0x0f830000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x0000001f);\n
# output: report(0x80000000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000021);\n
# output: report(0x671e1f06);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00002224);\n
# output: report(0x38f0f830);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00f789f0);\n
# output: report(0x0f830000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0xffffffff);\n
# output: report(0x80000000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000000);\n
# output: report(0xb38f0f83);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000001);\n
# output: report(0x671e1f06);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000004);\n
# output: report(0x38f0f830);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000010);\n
# output: report(0x0f830000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x0000001f);\n
# output: report(0x80000000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000021);\n
# output: report(0x671e1f06);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000024);\n
# output: report(0x38f0f830);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000030);\n
# output: report(0x0f830000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x0000003f);\n
# output: report(0x80000000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000000);\n
# output: report(0xb38f0f83);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000001);\n
# output: report(0xd9c787c1);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000004);\n
# output: report(0xfb38f0f8);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000010);\n
# output: report(0xffffb38f);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x0000001f);\n
# output: report(0xffffffff);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000001);\n
# output: report(0x2638783e);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000004);\n
# output: report(0x04c70f07);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000010);\n
# output: report(0x00004c70);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x0000001f);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000021);\n
# output: report(0xd9c787c1);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00002224);\n
# output: report(0xfb38f0f8);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00f789f0);\n
# output: report(0xffffb38f);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0xffffffff);\n
# output: report(0xffffffff);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000021);\n
# output: report(0x2638783e);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00002224);\n
# output: report(0x04c70f07);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00f789f0);\n
# output: report(0x00004c70);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000000);\n
# output: report(0xb38f0f83);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000001);\n
# output: report(0xd9c787c1);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000004);\n
# output: report(0xfb38f0f8);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000010);\n
# output: report(0xffffb38f);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x0000001f);\n
# output: report(0xffffffff);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000001);\n
# output: report(0x2638783e);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000004);\n
# output: report(0x04c70f07);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000010);\n
# output: report(0x00004c70);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x0000001f);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000021);\n
# output: report(0xd9c787c1);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000024);\n
# output: report(0xfb38f0f8);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000030);\n
# output: report(0xffffb38f);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x0000003f);\n
# output: report(0xffffffff);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000021);\n
# output: report(0x2638783e);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000024);\n
# output: report(0x04c70f07);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000030);\n
# output: report(0x00004c70);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x0000003f);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000000);\n
# output: report(0xb38f0f83);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000001);\n
# output: report(0x59c787c1);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000004);\n
# output: report(0x0b38f0f8);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000010);\n
# output: report(0x0000b38f);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x0000001f);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000001);\n
# output: report(0x2638783e);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000004);\n
# output: report(0x04c70f07);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000010);\n
# output: report(0x00004c70);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x0000001f);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000021);\n
# output: report(0x59c787c1);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00002224);\n
# output: report(0x0b38f0f8);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00f789f0);\n
# output: report(0x0000b38f);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0xffffffff);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000021);\n
# output: report(0x2638783e);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00002224);\n
# output: report(0x04c70f07);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00f789f0);\n
# output: report(0x00004c70);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000000);\n
# output: report(0xb38f0f83);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000001);\n
# output: report(0x59c787c1);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000004);\n
# output: report(0x0b38f0f8);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000010);\n
# output: report(0x0000b38f);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x0000001f);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000001);\n
# output: report(0x2638783e);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000004);\n
# output: report(0x04c70f07);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000010);\n
# output: report(0x00004c70);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x0000001f);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000021);\n
# output: report(0x59c787c1);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000024);\n
# output: report(0x0b38f0f8);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000030);\n
# output: report(0x0000b38f);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x0000003f);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000021);\n
# output: report(0x2638783e);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000024);\n
# output: report(0x04c70f07);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000030);\n
# output: report(0x00004c70);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x0000003f);\n
# output: report(0x00000000);\n
# output: \n
# output: exit(0)\n

#include "or1k-asm-test-helpers.h"

	.macro TEST_SHIFT opcode, op1, op2
	LOAD_IMMEDIATE r5, \op1
	LOAD_IMMEDIATE r6, \op2
	REPORT_REG_TO_CONSOLE r5
	REPORT_REG_TO_CONSOLE r6
	\opcode r4, r5, r6
	CHECK_CARRY_AND_OVERFLOW_NOT_SET r2, r3
	REPORT_REG_TO_CONSOLE r4
	PRINT_NEWLINE_TO_CONSOLE
	.endm

	.macro TEST_SHIFT_I opcode, op1, op2
	LOAD_IMMEDIATE r5, \op1
	REPORT_REG_TO_CONSOLE r5
	REPORT_IMMEDIATE_TO_CONSOLE \op2
	\opcode r4, r5, \op2
	CHECK_CARRY_AND_OVERFLOW_NOT_SET r2, r3
	REPORT_REG_TO_CONSOLE r4
	PRINT_NEWLINE_TO_CONSOLE
	.endm

	STANDARD_TEST_ENVIRONMENT

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Always set OVE.  We should never trigger an exception, even if
	   this bit is set.  */
	SET_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Test l.sll  */

	/* Shift left by zero.  */
	TEST_SHIFT l.sll, 0xb38f0f83, 0x00000000

	/* Shift left by amounts in the 1-31 range.  */
	TEST_SHIFT l.sll, 0xb38f0f83, 0x00000001
	TEST_SHIFT l.sll, 0xb38f0f83, 0x00000004
	TEST_SHIFT l.sll, 0xb38f0f83, 0x00000010
	TEST_SHIFT l.sll, 0xb38f0f83, 0x0000001f

	/* Shift left by larger amounts - should be masked.  */
	TEST_SHIFT l.sll, 0xb38f0f83, 0x00000021
	TEST_SHIFT l.sll, 0xb38f0f83, 0x00002224
	TEST_SHIFT l.sll, 0xb38f0f83, 0x00f789f0
	TEST_SHIFT l.sll, 0xb38f0f83, 0xffffffff

	/* Test l.slli  */

	/* Shift left by zero.  */
	TEST_SHIFT_I l.slli, 0xb38f0f83, 0x0000

	/* Shift left by amounts in the 1-31 range.  */
	TEST_SHIFT_I l.slli, 0xb38f0f83, 0x0001
	TEST_SHIFT_I l.slli, 0xb38f0f83, 0x0004
	TEST_SHIFT_I l.slli, 0xb38f0f83, 0x0010
	TEST_SHIFT_I l.slli, 0xb38f0f83, 0x001f

	/* Shift left by larger amounts - should be masked.  */
	TEST_SHIFT_I l.slli, 0xb38f0f83, 0x0021
	TEST_SHIFT_I l.slli, 0xb38f0f83, 0x0024
	TEST_SHIFT_I l.slli, 0xb38f0f83, 0x0030
	TEST_SHIFT_I l.slli, 0xb38f0f83, 0x003f

	/* Test l.sra  */

	/* Shift right by zero.  */
	TEST_SHIFT l.sra, 0xb38f0f83, 0x00000000

	/* Shift right by amounts in the 1-31 range.  */
	TEST_SHIFT l.sra, 0xb38f0f83, 0x00000001
	TEST_SHIFT l.sra, 0xb38f0f83, 0x00000004
	TEST_SHIFT l.sra, 0xb38f0f83, 0x00000010
	TEST_SHIFT l.sra, 0xb38f0f83, 0x0000001f

	TEST_SHIFT l.sra, 0x4c70f07c, 0x00000001
	TEST_SHIFT l.sra, 0x4c70f07c, 0x00000004
	TEST_SHIFT l.sra, 0x4c70f07c, 0x00000010
	TEST_SHIFT l.sra, 0x4c70f07c, 0x0000001f

	/* Shift right by larger amounts - should be masked.  */
	TEST_SHIFT l.sra, 0xb38f0f83, 0x00000021
	TEST_SHIFT l.sra, 0xb38f0f83, 0x00002224
	TEST_SHIFT l.sra, 0xb38f0f83, 0x00f789f0
	TEST_SHIFT l.sra, 0xb38f0f83, 0xffffffff

	TEST_SHIFT l.sra, 0x4c70f07c, 0x00000021
	TEST_SHIFT l.sra, 0x4c70f07c, 0x00002224
	TEST_SHIFT l.sra, 0x4c70f07c, 0x00f789f0
	TEST_SHIFT l.sra, 0x4c70f07c, 0xffffffff

	/* Test l.srai  */

	/* Shift right by zero.  */
	TEST_SHIFT_I l.srai, 0xb38f0f83, 0x0000

	/* Shift right by amounts in the 1-31 range.  */
	TEST_SHIFT_I l.srai, 0xb38f0f83, 0x0001
	TEST_SHIFT_I l.srai, 0xb38f0f83, 0x0004
	TEST_SHIFT_I l.srai, 0xb38f0f83, 0x0010
	TEST_SHIFT_I l.srai, 0xb38f0f83, 0x001f

	TEST_SHIFT_I l.srai, 0x4c70f07c, 0x0001
	TEST_SHIFT_I l.srai, 0x4c70f07c, 0x0004
	TEST_SHIFT_I l.srai, 0x4c70f07c, 0x0010
	TEST_SHIFT_I l.srai, 0x4c70f07c, 0x001f

	/* Shift right by larger amounts - should be masked.  */
	TEST_SHIFT_I l.srai, 0xb38f0f83, 0x0021
	TEST_SHIFT_I l.srai, 0xb38f0f83, 0x0024
	TEST_SHIFT_I l.srai, 0xb38f0f83, 0x0030
	TEST_SHIFT_I l.srai, 0xb38f0f83, 0x003f

	TEST_SHIFT_I l.srai, 0x4c70f07c, 0x0021
	TEST_SHIFT_I l.srai, 0x4c70f07c, 0x0024
	TEST_SHIFT_I l.srai, 0x4c70f07c, 0x0030
	TEST_SHIFT_I l.srai, 0x4c70f07c, 0x003f

	/* Test l.srl  */

	/* Shift right by zero.  */
	TEST_SHIFT l.srl, 0xb38f0f83, 0x00000000

	/* Shift right by amounts in the 1-31 range.  */
	TEST_SHIFT l.srl, 0xb38f0f83, 0x00000001
	TEST_SHIFT l.srl, 0xb38f0f83, 0x00000004
	TEST_SHIFT l.srl, 0xb38f0f83, 0x00000010
	TEST_SHIFT l.srl, 0xb38f0f83, 0x0000001f

	TEST_SHIFT l.srl, 0x4c70f07c, 0x00000001
	TEST_SHIFT l.srl, 0x4c70f07c, 0x00000004
	TEST_SHIFT l.srl, 0x4c70f07c, 0x00000010
	TEST_SHIFT l.srl, 0x4c70f07c, 0x0000001f

	/* Shift right by larger amounts - should be masked.  */
	TEST_SHIFT l.srl, 0xb38f0f83, 0x00000021
	TEST_SHIFT l.srl, 0xb38f0f83, 0x00002224
	TEST_SHIFT l.srl, 0xb38f0f83, 0x00f789f0
	TEST_SHIFT l.srl, 0xb38f0f83, 0xffffffff

	TEST_SHIFT l.srl, 0x4c70f07c, 0x00000021
	TEST_SHIFT l.srl, 0x4c70f07c, 0x00002224
	TEST_SHIFT l.srl, 0x4c70f07c, 0x00f789f0
	TEST_SHIFT l.srl, 0x4c70f07c, 0xffffffff

	/* Test l.srli  */

	/* Shift right by zero.  */
	TEST_SHIFT_I l.srli, 0xb38f0f83, 0x0000

	/* Shift right by amounts in the 1-31 range.  */
	TEST_SHIFT_I l.srli, 0xb38f0f83, 0x0001
	TEST_SHIFT_I l.srli, 0xb38f0f83, 0x0004
	TEST_SHIFT_I l.srli, 0xb38f0f83, 0x0010
	TEST_SHIFT_I l.srli, 0xb38f0f83, 0x001f

	TEST_SHIFT_I l.srli, 0x4c70f07c, 0x0001
	TEST_SHIFT_I l.srli, 0x4c70f07c, 0x0004
	TEST_SHIFT_I l.srli, 0x4c70f07c, 0x0010
	TEST_SHIFT_I l.srli, 0x4c70f07c, 0x001f

	/* Shift right by larger amounts - should be masked.  */
	TEST_SHIFT_I l.srli, 0xb38f0f83, 0x0021
	TEST_SHIFT_I l.srli, 0xb38f0f83, 0x0024
	TEST_SHIFT_I l.srli, 0xb38f0f83, 0x0030
	TEST_SHIFT_I l.srli, 0xb38f0f83, 0x003f

	TEST_SHIFT_I l.srli, 0x4c70f07c, 0x0021
	TEST_SHIFT_I l.srli, 0x4c70f07c, 0x0024
	TEST_SHIFT_I l.srli, 0x4c70f07c, 0x0030
	TEST_SHIFT_I l.srli, 0x4c70f07c, 0x003f

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
