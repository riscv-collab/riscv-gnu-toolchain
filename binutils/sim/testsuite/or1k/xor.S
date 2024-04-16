/* Tests instructions l.xor, l.xori.

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
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffffffff);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xaaaaaaaa);\n
# output: report(0x00000000);\n
# output: report(0xaaaaaaaa);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xaaaaaaaa);\n
# output: report(0xaaaaaaaa);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x55555555);\n
# output: report(0x00000000);\n
# output: report(0x55555555);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x55555555);\n
# output: report(0x55555555);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xaaaaaaaa);\n
# output: report(0x55555555);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0xb38f0f83);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0xc4c70f07);\n
# output: report(0x88b7ff7b);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x38f0f83b);\n
# output: report(0x8b7ff7b8);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffffffff);\n
# output: report(0x0000ffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xaaaaaaaa);\n
# output: report(0x00000000);\n
# output: report(0xaaaaaaaa);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xaaaaaaaa);\n
# output: report(0x0000aaaa);\n
# output: report(0x55550000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x55555555);\n
# output: report(0x00000000);\n
# output: report(0x55555555);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x55555555);\n
# output: report(0x00005555);\n
# output: report(0x55550000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xaaaaaaaa);\n
# output: report(0x00005555);\n
# output: report(0xaaaaffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000f83);\n
# output: report(0x4c70ffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x4c70f07c);\n
# output: report(0x00000f07);\n
# output: report(0x4c70ff7b);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xb38f0f83);\n
# output: report(0x0000f83b);\n
# output: report(0x4c70f7b8);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: exit(0)\n

#include "or1k-asm-test-helpers.h"

	STANDARD_TEST_ENVIRONMENT

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Always set OVE.  We should never trigger an exception, even if
	   this bit is set.  */
	SET_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Test the l.xor instruction with a range of operands.  */

	TEST_INST_I32_I32 l.xor, 0x00000000, 0x00000000
	TEST_INST_I32_I32 l.xor, 0xffffffff, 0xffffffff
	TEST_INST_I32_I32 l.xor, 0xaaaaaaaa, 0x00000000
	TEST_INST_I32_I32 l.xor, 0xaaaaaaaa, 0xaaaaaaaa
	TEST_INST_I32_I32 l.xor, 0x55555555, 0x00000000
	TEST_INST_I32_I32 l.xor, 0x55555555, 0x55555555
	TEST_INST_I32_I32 l.xor, 0xaaaaaaaa, 0x55555555
	TEST_INST_I32_I32 l.xor, 0x4c70f07c, 0xb38f0f83
	TEST_INST_I32_I32 l.xor, 0x4c70f07c, 0xc4c70f07
	TEST_INST_I32_I32 l.xor, 0xb38f0f83, 0x38f0f83b

	/* Test the l.xori instruction with a range of operands.  */

	TEST_INST_I32_I16 l.xori, 0x00000000, 0x0000
	TEST_INST_I32_I16 l.xori, 0xffffffff, 0xffff
	TEST_INST_I32_I16 l.xori, 0xaaaaaaaa, 0x0000
	TEST_INST_I32_I16 l.xori, 0xaaaaaaaa, 0xaaaa
	TEST_INST_I32_I16 l.xori, 0x55555555, 0x0000
	TEST_INST_I32_I16 l.xori, 0x55555555, 0x5555
	TEST_INST_I32_I16 l.xori, 0xaaaaaaaa, 0x5555
	TEST_INST_I32_I16 l.xori, 0x4c70f07c, 0x0f83
	TEST_INST_I32_I16 l.xori, 0x4c70f07c, 0x0f07
	TEST_INST_I32_I16 l.xori, 0xb38f0f83, 0xf83b

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
