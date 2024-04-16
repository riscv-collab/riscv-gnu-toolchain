/* Tests some basic unordered fpu compare instructions.

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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
# output: report(0x400921f9);\n
# output: report(0xf01b866e);\n
# output: report(0x4005bf09);\n
# output: report(0x95aaf790);\n
# output: report(0x7ff80000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000001);\n
# output: \n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000001);\n
# output: \n
# output: exit(0)\n

#include "or1k-asm-test-helpers.h"

	STANDARD_TEST_ENVIRONMENT

	.section .data
	.align 4
	.type   pi, @object
	.size   pi, 8
anchor:
pi:
	.double  3.14159

	.type   e, @object
	.size   e, 8
e:
	.double  2.71828

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Test unordered double comparisons.  Setting up:
	 *  r11      pointer to data
	 *  r12,r13  pi  as double
	 *  r14,r15  e   as double
	 *  r16,r17  nan as double
	 */
	l.movhi	r11, ha(anchor)
	l.addi	r11, r11, lo(anchor)
	l.lwz	r12, 0(r11)
	l.lwz	r13, 4(r11)

	l.lwz	r14, 8(r11)
	l.lwz	r15, 12(r11)

	/* Make a NaN.  */
	lf.sub.d r16,r18, r12,r13, r12,r13
	lf.div.d r16,r18, r16,r18, r16,r18

	/* Output to ensure we loaded it correctly.  */
	REPORT_REG_TO_CONSOLE r12
	REPORT_REG_TO_CONSOLE r13

	REPORT_REG_TO_CONSOLE r14
	REPORT_REG_TO_CONSOLE r15

	REPORT_REG_TO_CONSOLE r16
	REPORT_REG_TO_CONSOLE r18
	PRINT_NEWLINE_TO_CONSOLE

	lf.sfuge.d r12,r13, r14,r15
	MOVE_FROM_SPR r2, SPR_SR
	REPORT_BIT_TO_CONSOLE r2, SPR_SR_F
	PRINT_NEWLINE_TO_CONSOLE

	lf.sfun.d r12,r13, r14,r15
	MOVE_FROM_SPR r2, SPR_SR
	REPORT_BIT_TO_CONSOLE r2, SPR_SR_F
	PRINT_NEWLINE_TO_CONSOLE

	lf.sfun.d r12,r13, r16,r18
	MOVE_FROM_SPR r2, SPR_SR
	REPORT_BIT_TO_CONSOLE r2, SPR_SR_F
	PRINT_NEWLINE_TO_CONSOLE

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
