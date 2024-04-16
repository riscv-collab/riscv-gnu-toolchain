/* Tests the jump instructions.

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
# output: report(0x48000000);\n
# output: report(0x00000005);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x48000000);\n
# output: report(0x00000009);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x44000000);\n
# output: report(0x00000005);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x44000000);\n
# output: report(0x00000009);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: exit(0)\n

#include "or1k-asm-test-helpers.h"

/* Tests a jump instruction using a register destination.
   Checks whether the jump succeeds, or whether an exception is triggered
   (but not if the right exception was triggered yet).

   We manually construct the opcode, to allow us to force R9 into the
   destination field, to test exception handling.  Usually the assembler
   would prevent this.

   Do not specify R31 as the register to use for the jump, as it's used
   internally.  */

	.macro TEST_JUMP  opcode_value  dest_register_number  alignment_offset
	REPORT_IMMEDIATE_TO_CONSOLE \opcode_value
	REPORT_IMMEDIATE_TO_CONSOLE \dest_register_number
	REPORT_IMMEDIATE_TO_CONSOLE \alignment_offset
	LOAD_IMMEDIATE r\dest_register_number, 51f + \alignment_offset
	/* Generate the jump opcode.  */
\@1$:	OR1K_DELAYED_NOP \
	  (.word  ( \opcode_value | (\dest_register_number << 11) ))
	/* If the jump failed, we land here.  */
	REPORT_IMMEDIATE_TO_CONSOLE 1
	OR1K_DELAYED_NOP (l.j 52f)
	/* If the jump succeeds, we land here.  */
51:	REPORT_IMMEDIATE_TO_CONSOLE 0
52:	REPORT_EXCEPTION \@1$
	PRINT_NEWLINE_TO_CONSOLE
	.endm

	STANDARD_TEST_ENVIRONMENT

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Test l.jalr (jump and link register)  */
	TEST_JUMP 0x48000000, 5, 0
	/* TODO: The sim does not support unaligned memory access yet.
	TEST_JUMP 0x48000000, 5, 1
	TEST_JUMP 0x48000000, 5, 2
	TEST_JUMP 0x48000000, 5, 3
	*/

	/* Test with link register as the destination.  This is not
	   allowed.  */
	TEST_JUMP 0x48000000, 9, 0

	/* Test l.jr (jump register)  */
	TEST_JUMP 0x44000000, 5, 0
	/* TODO: The sim does not support unaligned memory access yet.
	TEST_JUMP 0x44000000, 5, 1
	TEST_JUMP 0x44000000, 5, 2
	TEST_JUMP 0x44000000, 5, 3
	*/

	/* Test with link register as the destination.   */
	TEST_JUMP 0x44000000, 9, 0

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
