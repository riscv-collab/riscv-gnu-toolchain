/* Tests the find instructions.

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
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x80000000);\n
# output: report(0x00000020);\n
# output: \n
# output: report(0x55555555);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0xaaaaaaaa);\n
# output: report(0x00000002);\n
# output: \n
# output: report(0x00018000);\n
# output: report(0x00000010);\n
# output: \n
# output: report(0xc0000000);\n
# output: report(0x0000001f);\n
# output: \n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x80000000);\n
# output: report(0x00000020);\n
# output: \n
# output: report(0x55555555);\n
# output: report(0x0000001f);\n
# output: \n
# output: report(0xaaaaaaaa);\n
# output: report(0x00000020);\n
# output: \n
# output: report(0x00018000);\n
# output: report(0x00000011);\n
# output: \n
# output: report(0xc0000000);\n
# output: report(0x00000020);\n
# output: \n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: exit(0)\n

#include "or1k-asm-test-helpers.h"

	.macro TEST_FIND opcode, operand
	LOAD_IMMEDIATE r5, \operand
	REPORT_REG_TO_CONSOLE r5
	\opcode r4, r5
	REPORT_REG_TO_CONSOLE r4
	PRINT_NEWLINE_TO_CONSOLE
	.endm

	STANDARD_TEST_ENVIRONMENT

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Test l.ff1  */

	TEST_FIND l.ff1, 0x00000001
	TEST_FIND l.ff1, 0x80000000
	TEST_FIND l.ff1, 0x55555555
	TEST_FIND l.ff1, 0xaaaaaaaa
	TEST_FIND l.ff1, 0x00018000
	TEST_FIND l.ff1, 0xc0000000
	TEST_FIND l.ff1, 0x00000000

	/* Test l.fl1  */

	TEST_FIND l.fl1, 0x00000001
	TEST_FIND l.fl1, 0x80000000
	TEST_FIND l.fl1, 0x55555555
	TEST_FIND l.fl1, 0xaaaaaaaa
	TEST_FIND l.fl1, 0x00018000
	TEST_FIND l.fl1, 0xc0000000
	TEST_FIND l.fl1, 0x00000000

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
