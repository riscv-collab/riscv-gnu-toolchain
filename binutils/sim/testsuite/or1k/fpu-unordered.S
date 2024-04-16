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
# output: report(0x40490fd0);\n
# output: report(0x402df84d);\n
# output: report(0x7fc00000);\n
# output: \n
# output: report(0x00000001);\n
# output: \n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000001);\n
# output: \n
# output: report(0x00000001);\n
# output: \n
# output: exit(0)\n

#include "or1k-asm-test-helpers.h"

	STANDARD_TEST_ENVIRONMENT

	.section .data
	.align 4
	.type   pi, @object
	.size   pi, 4
anchor:
pi:
	.float  3.14159

	.type   e, @object
	.size   e, 4
e:
	.float  2.71828

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Test unordered float comparisons.  Setting up:
	 *  r11      pointer to data
	 *  r12  pi  as float
	 *  r13  e   as float
	 *  r16  nan as float
	 */
	l.movhi	r11, ha(anchor)
	l.addi	r11, r11, lo(anchor)
	l.lwz	r12, 0(r11)

	l.lwz	r13, 4(r11)

	/* Make a NaN.  */
	lf.sub.s r16, r13, r13
	lf.div.s r16, r16, r16

	/* Output to ensure we loaded it correctly.  */
	REPORT_REG_TO_CONSOLE r12
	REPORT_REG_TO_CONSOLE r13
	REPORT_REG_TO_CONSOLE r16
	PRINT_NEWLINE_TO_CONSOLE

	lf.sfuge.s r12, r13
	MOVE_FROM_SPR r2, SPR_SR
	REPORT_BIT_TO_CONSOLE r2, SPR_SR_F
	PRINT_NEWLINE_TO_CONSOLE

	lf.sfun.s r12, r13
	MOVE_FROM_SPR r2, SPR_SR
	REPORT_BIT_TO_CONSOLE r2, SPR_SR_F
	PRINT_NEWLINE_TO_CONSOLE

	lf.sfun.s r12, r16
	MOVE_FROM_SPR r2, SPR_SR
	REPORT_BIT_TO_CONSOLE r2, SPR_SR_F
	PRINT_NEWLINE_TO_CONSOLE

	lf.sfueq.s r12, r12
	MOVE_FROM_SPR r2, SPR_SR
	REPORT_BIT_TO_CONSOLE r2, SPR_SR_F
	PRINT_NEWLINE_TO_CONSOLE

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
