/* Tests some basic fpu instructions.

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
# output: report(0x00007ab7);\n
# output: report(0xffffd8f0);\n
# output: report(0x46f56e00);\n
# output: report(0xc61c4000);\n
# output: report(0x00007ab7);\n
# output: report(0xffffd8f0);\n
# output: \n
# output: report(0xc0490e56);\n
# output: report(0xfffffffd);\n
# output: \n
# output: report(0x4e6b4bbb);\n
# output: \n
# output: report(0xbdc0be40);\n
# output: \n
# output: report(0x00000001);\n
# output: \n
# output: WARNING: ignoring fpu error caught in fast mode.\n
# output: report(0x00000000);\n
# output: \n
# output: exit(0)\n

#include "or1k-asm-test-helpers.h"

	STANDARD_TEST_ENVIRONMENT

	.section .exception_vectors

	/* Floating point exception.  */
	.org	0xd00

	/* The handling is a bit dubious at present.  We just patch the
	   instruction with l.nop and restart.  This will go wrong in branch
	   delay slots.  But we don't have those in this test.  */
	l.addi r1, r1, -EXCEPTION_STACK_SKIP_SIZE
	PUSH r2
	PUSH r3
	/* Save the address of the instruction that caused the problem.  */
	MOVE_FROM_SPR r2, SPR_EPCR_BASE
	LOAD_IMMEDIATE r3, 0x15000000 /* Opcode for l.nop  */
	l.sw	-4(r2), r3
	POP r3
	POP r2
	l.addi r1, r1, EXCEPTION_STACK_SKIP_SIZE
	l.rfe

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Test lf.itof.s int to float conversion.  Setting up:
	 *  r10  31415.0f
	 *  r12	-10000.0f
	 */
	l.ori	r11, r0, 31415
	l.ori	r13, r0, -10000
	l.movhi	r15, 0xffff
	l.or	r13, r13, r15

	REPORT_REG_TO_CONSOLE r11
	REPORT_REG_TO_CONSOLE r13
	lf.itof.s r10, r11
	lf.itof.s r12, r13
	REPORT_REG_TO_CONSOLE r10
	REPORT_REG_TO_CONSOLE r12

	/* Test lf.ftoi.s float to int conversion.  */
	lf.ftoi.s r11, r10
	lf.ftoi.s r13, r12
	REPORT_REG_TO_CONSOLE r11
	REPORT_REG_TO_CONSOLE r13
	PRINT_NEWLINE_TO_CONSOLE

	/* Test lf.div.s divide 31415 by -1000 to get -pi.  Setting up:
	 *  r8	-3.1415f
	 */
	lf.div.s r8, r10, r12
	REPORT_REG_TO_CONSOLE r8

	lf.ftoi.s r11, r8
	REPORT_REG_TO_CONSOLE r11
	PRINT_NEWLINE_TO_CONSOLE

	/* Test lf.mul.s multiply -pi x -10000 x 31415.  Setting up:
	 *  r6	 986902225
	 */
	lf.mul.s r6, r8, r12
	lf.mul.s r6, r6, r10
	REPORT_REG_TO_CONSOLE r6
	PRINT_NEWLINE_TO_CONSOLE

	/* Test lf.rem.s remainder of 986902225 / -pi.  */
	lf.rem.s r2, r6, r8
	REPORT_REG_TO_CONSOLE r2
	PRINT_NEWLINE_TO_CONSOLE

	/* Test lf.sfge.s set flag if r6 >= r10.  */
	lf.sfge.s r6, r10
	MOVE_FROM_SPR r2, SPR_SR
	REPORT_BIT_TO_CONSOLE r2, SPR_SR_F
	PRINT_NEWLINE_TO_CONSOLE

	/* Test raising an exception by dividing by 0.  */
	MOVE_FROM_SPR r2, SPR_FPCSR
	l.ori	r2, r2, 0x1
	MOVE_TO_SPR SPR_FPCSR, r2
div0:	lf.div.s r2, r8, r0
	REPORT_EXCEPTION div0
	PRINT_NEWLINE_TO_CONSOLE

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
