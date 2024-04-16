/* Tests some basic fpu instructions.

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
# output: report(0x00000000);\n
# output: report(0x00001234);\n
# output: \n
# output: report(0x40b23400);\n
# output: report(0x00000000);\n
# output: report(0x40b23400);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x40177081);\n
# output: report(0xc2e33eff);\n
# output: report(0x400921f9);\n
# output: report(0xf01b866e);\n
# output: \n
# output: report(0x40211456);\n
# output: report(0x587dfabf);\n
# output: report(0x400921f9);\n
# output: report(0xf01b866d);\n
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

	.type   large, @object
	.size   large, 8
large:
	.long  0
	.long  0x1234

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Test lf.itof.d int to double conversion.  Setting up:
	 *  r11      pointer to data
	 *  r12,r13  pi as double
	 *  r14,r15  e  as double
	 *  r16,r17  a long long
	 */
	l.movhi	r11, ha(anchor)
	l.addi	r11, r11, lo(anchor)
	l.lwz	r12, 0(r11)
	l.lwz	r13, 4(r11)

	l.lwz	r14, 8(r11)
	l.lwz	r15, 12(r11)

	l.lwz	r16, 16(r11)
	l.lwz	r18, 20(r11)

	/* Output to ensure we loaded it correctly.  */
	REPORT_REG_TO_CONSOLE r12
	REPORT_REG_TO_CONSOLE r13

	REPORT_REG_TO_CONSOLE r14
	REPORT_REG_TO_CONSOLE r15

	REPORT_REG_TO_CONSOLE r16
	REPORT_REG_TO_CONSOLE r18
	PRINT_NEWLINE_TO_CONSOLE

	/* Convert the big long to a double. */
	lf.itof.d r16,r18, r16,r18
	REPORT_REG_TO_CONSOLE r16
	REPORT_REG_TO_CONSOLE r18

	/* Convert the double back to a long, it should match before. */
	lf.ftoi.d r16,r18, r16,r18
	lf.itof.d r16,r18, r16,r18

	REPORT_REG_TO_CONSOLE r16
	REPORT_REG_TO_CONSOLE r18

	PRINT_NEWLINE_TO_CONSOLE

	/* Add and subtract some double values.  */
	lf.add.d r12,r13, r12,r13, r14,r15
	REPORT_REG_TO_CONSOLE r12
	REPORT_REG_TO_CONSOLE r13

	lf.sub.d r12,r13, r12,r13, r14,r15
	REPORT_REG_TO_CONSOLE r12
	REPORT_REG_TO_CONSOLE r13
	PRINT_NEWLINE_TO_CONSOLE

	/* Multiply and divide double values.  */
	lf.mul.d r12,r13, r12,r13, r14,r15
	REPORT_REG_TO_CONSOLE r12
	REPORT_REG_TO_CONSOLE r13

	lf.div.d r12,r13, r12,r13, r14,r15
	REPORT_REG_TO_CONSOLE r12
	REPORT_REG_TO_CONSOLE r13
	PRINT_NEWLINE_TO_CONSOLE

	/* Test lf.sfge.s set flag if r6 >= r10.  */
	lf.sfge.d r12,r13, r14,r15
	MOVE_FROM_SPR r2, SPR_SR
	REPORT_BIT_TO_CONSOLE r2, SPR_SR_F
	PRINT_NEWLINE_TO_CONSOLE

	/* Test raising an exception by dividing by 0.  */
	MOVE_FROM_SPR r2, SPR_FPCSR
	l.ori	r2, r2, 0x1
	MOVE_TO_SPR SPR_FPCSR, r2
div0:	lf.div.d r2,r3, r12,r13, r0,r1
	REPORT_EXCEPTION div0
	PRINT_NEWLINE_TO_CONSOLE

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
