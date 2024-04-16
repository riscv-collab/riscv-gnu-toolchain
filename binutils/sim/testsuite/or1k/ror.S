/* Tests instructions l.ror and l.rori.

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
# output: report(0xd9c787c1);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000004);\n
# output: report(0x3b38f0f8);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000010);\n
# output: report(0x0f83b38f);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x0000001f);\n
# output: report(0x671e1f07);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000021);\n
# output: report(0xd9c787c1);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00002224);\n
# output: report(0x3b38f0f8);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00f789f0);\n
# output: report(0x0f83b38f);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0xffffffff);\n
# output: report(0x671e1f07);\n
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
# output: report(0x3b38f0f8);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000010);\n
# output: report(0x0f83b38f);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x0000001f);\n
# output: report(0x671e1f07);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000021);\n
# output: report(0xd9c787c1);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000024);\n
# output: report(0x3b38f0f8);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x00000030);\n
# output: report(0x0f83b38f);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x0000003f);\n
# output: report(0x671e1f07);\n
# output: \n
# output: exit(0)\n

#include "or1k-asm-test-env.h"

	.macro TEST_ROR op1, op2, res
	/* Note that 'res' is not used here.  We could stop using the
	   .TestResults file and use 'res' here instead.  */
	LOAD_IMMEDIATE r5, \op1
	LOAD_IMMEDIATE r6, \op2
	REPORT_REG_TO_CONSOLE r5
	REPORT_REG_TO_CONSOLE r6
	l.ror r4, r5, r6
	REPORT_REG_TO_CONSOLE r4
	PRINT_NEWLINE_TO_CONSOLE
	.endm

	.macro TEST_RORI op1, op2, res
	/* Note that 'res' is not used here.  We could stop using the
	   .TestResults file and use 'res' here instead.  */
	LOAD_IMMEDIATE r5, \op1
	REPORT_REG_TO_CONSOLE r5
	REPORT_IMMEDIATE_TO_CONSOLE \op2
	l.rori r4, r5, \op2
	REPORT_REG_TO_CONSOLE r4
	PRINT_NEWLINE_TO_CONSOLE
	.endm

	STANDARD_TEST_ENVIRONMENT

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Test the l.ror instruction.  */

	/* Rotate by zero  */
	TEST_ROR 0xb38f0f83, 0x00000000, 0xb38f0f83

	/* Rotate by amounts in the 1 - 31 range.  */
	TEST_ROR 0xb38f0f83, 0x00000001, 0xd9c787c1
	TEST_ROR 0xb38f0f83, 0x00000004, 0x3b38f0f8
	TEST_ROR 0xb38f0f83, 0x00000010, 0x0f83b38f
	TEST_ROR 0xb38f0f83, 0x0000001f, 0x671e1f07

	/* Rotate by larger amounts - should be masked.  */
	TEST_ROR 0xb38f0f83, 0x00000021, 0xd9c787c1
	TEST_ROR 0xb38f0f83, 0x00002224, 0x3b38f0f8
	TEST_ROR 0xb38f0f83, 0x00f789f0, 0x0f83b38f
	TEST_ROR 0xb38f0f83, 0xffffffff, 0x671e1f07

	/* Test the l.rori instruction.  */

	/* Rotate by zero */
	TEST_RORI 0xb38f0f83, 0x00000000, 0xb38f0f83

	/* Rotate by amounts in the 1 - 31 range.  */
	TEST_RORI 0xb38f0f83, 0x01, 0xd9c787c1
	TEST_RORI 0xb38f0f83, 0x04, 0x3b38f0f8
	TEST_RORI 0xb38f0f83, 0x10, 0x0f83b38f
	TEST_RORI 0xb38f0f83, 0x1f, 0x671e1f07

	/* Rotate by larger amounts (32 - 63) - should be masked.  */
	TEST_RORI 0xb38f0f83, 0x21, 0xd9c787c1
	TEST_RORI 0xb38f0f83, 0x24, 0x3b38f0f8
	TEST_RORI 0xb38f0f83, 0x30, 0x0f83b38f
	TEST_RORI 0xb38f0f83, 0x3f, 0x671e1f07

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
