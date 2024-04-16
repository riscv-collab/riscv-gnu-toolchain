/* Tests the l.ext{b,h}{s,z} instructions.

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
# output: report(0x0000007f);\n
# output: report(0x0000007f);\n
# output: report(0x00000053);\n
# output: report(0x00000053);\n
# output: report(0x0000ff53);\n
# output: report(0x00000053);\n
# output: report(0x00001234);\n
# output: report(0x00000034);\n
# output: report(0x000000ff);\n
# output: report(0xffffffff);\n
# output: report(0x00000080);\n
# output: report(0xffffff80);\n
# output: report(0x0000ff80);\n
# output: report(0xffffff80);\n
# output: report(0x00007f80);\n
# output: report(0xffffff80);\n
# output: report(0x00007fff);\n
# output: report(0xffffffff);\n
# output: report(0x0000007f);\n
# output: report(0x0000007f);\n
# output: report(0x00000053);\n
# output: report(0x00000053);\n
# output: report(0x0000ff53);\n
# output: report(0x00000053);\n
# output: report(0x00001234);\n
# output: report(0x00000034);\n
# output: report(0x000000ff);\n
# output: report(0x000000ff);\n
# output: report(0x00000080);\n
# output: report(0x00000080);\n
# output: report(0x0000ff80);\n
# output: report(0x00000080);\n
# output: report(0x00007f80);\n
# output: report(0x00000080);\n
# output: report(0x00007fff);\n
# output: report(0x000000ff);\n
# output: report(0x00007fff);\n
# output: report(0x00007fff);\n
# output: report(0x00005233);\n
# output: report(0x00005233);\n
# output: report(0xffff2f53);\n
# output: report(0x00002f53);\n
# output: report(0x12345678);\n
# output: report(0x00005678);\n
# output: report(0x0000ffff);\n
# output: report(0xffffffff);\n
# output: report(0x00008000);\n
# output: report(0xffff8000);\n
# output: report(0x0000ff80);\n
# output: report(0xffffff80);\n
# output: report(0x80008000);\n
# output: report(0xffff8000);\n
# output: report(0x7fffffff);\n
# output: report(0xffffffff);\n
# output: report(0x00007fff);\n
# output: report(0x00007fff);\n
# output: report(0x00005233);\n
# output: report(0x00005233);\n
# output: report(0xffff2f53);\n
# output: report(0x00002f53);\n
# output: report(0x12345678);\n
# output: report(0x00005678);\n
# output: report(0x0000ffff);\n
# output: report(0x0000ffff);\n
# output: report(0x00008000);\n
# output: report(0x00008000);\n
# output: report(0x0000ff80);\n
# output: report(0x0000ff80);\n
# output: report(0x80008000);\n
# output: report(0x00008000);\n
# output: report(0x7fffffff);\n
# output: report(0x0000ffff);\n
# output: report(0xffffffff);\n
# output: report(0xffffffff);\n
# output: report(0x7fffffff);\n
# output: report(0x7fffffff);\n
# output: report(0x7fff7fff);\n
# output: report(0x7fff7fff);\n
# output: report(0xffff7f7f);\n
# output: report(0xffff7f7f);\n
# output: report(0xffffff7f);\n
# output: report(0xffffff7f);\n
# output: report(0xffff7fff);\n
# output: report(0xffff7fff);\n
# output: report(0x7fff7f7f);\n
# output: report(0x7fff7f7f);\n
# output: report(0x12345678);\n
# output: report(0x12345678);\n
# output: report(0xffffffff);\n
# output: report(0xffffffff);\n
# output: report(0x7fffffff);\n
# output: report(0x7fffffff);\n
# output: report(0x7fff7fff);\n
# output: report(0x7fff7fff);\n
# output: report(0xffff7f7f);\n
# output: report(0xffff7f7f);\n
# output: report(0xffffff7f);\n
# output: report(0xffffff7f);\n
# output: report(0xffff7fff);\n
# output: report(0xffff7fff);\n
# output: report(0x7fff7f7f);\n
# output: report(0x7fff7f7f);\n
# output: report(0x12345678);\n
# output: report(0x12345678);\n
# output: exit(0)\n

#include "or1k-asm-test-env.h"

	.macro CHECK_EXT insn, val, mask, high_mask
	LOAD_IMMEDIATE r4, \val
	REPORT_REG_TO_CONSOLE r4
	\insn   r5, r4
	REPORT_REG_TO_CONSOLE r5
	LOAD_IMMEDIATE r6, \mask
	l.xori  r7, r6, -1
	l.and   r8, r4, r6
	l.and   r9, r5, r6
	l.sfne  r8, r9
	OR1K_DELAYED_NOP (l.bf    ext_fail)
	l.and   r8, r5, r7
	LOAD_IMMEDIATE r7, \high_mask
	l.sfne  r8, r7
	OR1K_DELAYED_NOP (l.bf    ext_fail)
	.endm

#define CHECK_HIGH3_CLEAR(insn, val) CHECK_EXT insn, val, 0x000000ff, 0
#define CHECK_HIGH3_SET(val) CHECK_EXT l.extbs, val, 0x000000ff, 0xffffff00
#define CHECK_HIGH2_CLEAR(insn, val) CHECK_EXT insn, val, 0x0000ffff, 0
#define CHECK_HIGH2_SET(val) CHECK_EXT l.exths, val, 0x0000ffff, 0xffff0000

	.macro CHECK_MOVE insn, val
	LOAD_IMMEDIATE r4, \val
	REPORT_REG_TO_CONSOLE r4
	\insn   r5, r4
	REPORT_REG_TO_CONSOLE r5
	l.sfne  r5, r4
	OR1K_DELAYED_NOP (l.bf    ext_fail)
	.endm

	STANDARD_TEST_ENVIRONMENT

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Test l.extbs  */
	CHECK_HIGH3_CLEAR ( l.extbs, 0x7f   )
	CHECK_HIGH3_CLEAR ( l.extbs, 0x53   )
	CHECK_HIGH3_CLEAR ( l.extbs, 0xff53 )
	CHECK_HIGH3_CLEAR ( l.extbs, 0x1234 )

	CHECK_HIGH3_SET (0xff)
	CHECK_HIGH3_SET (0x80)
	CHECK_HIGH3_SET (0xff80)
	CHECK_HIGH3_SET (0x7f80)
	CHECK_HIGH3_SET (0x7fff)

	/* Test l.extbz  */
	CHECK_HIGH3_CLEAR (l.extbz, 0x7f)
	CHECK_HIGH3_CLEAR (l.extbz, 0x53)
	CHECK_HIGH3_CLEAR (l.extbz, 0xff53)
	CHECK_HIGH3_CLEAR (l.extbz, 0x1234)

	CHECK_HIGH3_CLEAR (l.extbz, 0xff)
	CHECK_HIGH3_CLEAR (l.extbz, 0x80)
	CHECK_HIGH3_CLEAR (l.extbz, 0xff80)
	CHECK_HIGH3_CLEAR (l.extbz, 0x7f80)
	CHECK_HIGH3_CLEAR (l.extbz, 0x7fff)

	/* Test l.exths  */
	CHECK_HIGH2_CLEAR (l.exths, 0x7fff)
	CHECK_HIGH2_CLEAR (l.exths, 0x5233)
	CHECK_HIGH2_CLEAR (l.exths, 0xffff2f53)
	CHECK_HIGH2_CLEAR (l.exths, 0x12345678)

	CHECK_HIGH2_SET (0xffff)
	CHECK_HIGH2_SET (0x8000)
	CHECK_HIGH2_SET (0xff80)
	CHECK_HIGH2_SET (0x80008000)
	CHECK_HIGH2_SET (0x7fffffff)

	/* Test l.exthz  */
	CHECK_HIGH2_CLEAR (l.exthz, 0x7fff)
	CHECK_HIGH2_CLEAR (l.exthz, 0x5233)
	CHECK_HIGH2_CLEAR (l.exthz, 0xffff2f53)
	CHECK_HIGH2_CLEAR (l.exthz, 0x12345678)

	CHECK_HIGH2_CLEAR (l.exthz, 0xffff)
	CHECK_HIGH2_CLEAR (l.exthz, 0x8000)
	CHECK_HIGH2_CLEAR (l.exthz, 0xff80)
	CHECK_HIGH2_CLEAR (l.exthz, 0x80008000)
	CHECK_HIGH2_CLEAR (l.exthz, 0x7fffffff)

	/* Test l.extws  */
	CHECK_MOVE l.extws, 0xffffffff
	CHECK_MOVE l.extws, 0x7fffffff
	CHECK_MOVE l.extws, 0x7fff7fff
	CHECK_MOVE l.extws, 0xffff7f7f
	CHECK_MOVE l.extws, 0xffffff7f
	CHECK_MOVE l.extws, 0xffff7fff
	CHECK_MOVE l.extws, 0x7fff7f7f
	CHECK_MOVE l.extws, 0x12345678

	/* Test l.extwz  */
	CHECK_MOVE l.extwz, 0xffffffff
	CHECK_MOVE l.extwz, 0x7fffffff
	CHECK_MOVE l.extwz, 0x7fff7fff
	CHECK_MOVE l.extwz, 0xffff7f7f
	CHECK_MOVE l.extwz, 0xffffff7f
	CHECK_MOVE l.extwz, 0xffff7fff
	CHECK_MOVE l.extwz, 0x7fff7f7f
	CHECK_MOVE l.extwz, 0x12345678

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9

ext_fail:
	EXIT_SIMULATION_WITH_IMMEDIATE_EXIT_CODE SEC_GENERIC_ERROR
