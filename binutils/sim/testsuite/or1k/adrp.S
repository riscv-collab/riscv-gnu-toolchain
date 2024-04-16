/* Tests the load page address instruction.

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
# output: report(0x00002064);\n
# output: report(0x0001a008);\n
# output: report(0x00002000);\n
# output: report(0x0001a000);\n
# output: report(0x00002000);\n
# output: report(0x00014000);\n
# output: report(0x00000000);\n
# output: exit(0)\n

#include "or1k-asm-test-helpers.h"

	STANDARD_TEST_ENVIRONMENT

	.section .data
	.org 0x10000
	.align 4
pad:	.quad 	0
	.type   pi, @object
	.size   pi, 4
pi:
	.float	3.14159

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Print out the PC.  To compare with that loaded by l.adrp.  */
	l.jal	capture_pc
	 l.nop
capture_pc:
	REPORT_REG_TO_CONSOLE r9

	/* Print out our data address to compared with l.adrp offset.  */
	l.movhi	r11, ha(pi)
	l.addi	r11, r11, lo(pi)
	REPORT_REG_TO_CONSOLE r11

	/* Test l.adrp with symbols, loads page of symbol to register.  */
	l.adrp	r4, start_tests
	REPORT_REG_TO_CONSOLE r4

	l.adrp	r4, pi
	REPORT_REG_TO_CONSOLE r4

	/* Test l.adrp with immediate, immediate is the page offset.  */
	l.adrp	r4, 0x0
	REPORT_REG_TO_CONSOLE r4

	l.adrp	r4, 0x12000
	REPORT_REG_TO_CONSOLE r4

	l.adrp	r4, -0x2000
	REPORT_REG_TO_CONSOLE r4

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
