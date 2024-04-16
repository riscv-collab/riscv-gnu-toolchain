/* Tests the set flag (l.sf*) instructions.

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
# output: exit(0)\n

#include "or1k-asm-test-helpers.h"

#define INT_MAX		2147483647 /* 0x7fffffff  */
#define INT_MAX_MIN1	2147483646 /* 0x7ffffffe  */
#define NEG_INT_MAX	-2147483648 /* 0x80000000  */
#define NEG_INT_MAX_PL1	-2147483647 /* 0x80000001  */
#define MIN1		-1 /* 0xffffffff  */

#define SHRT_MIN	(-32768)
#define SHRT_MAX	32767

#define UINT_MAX	4294967295 /* 0xffffffff  */
#define UINT_MAX_MIN1	4294967294 /* 0xfffffffe  */

#define USHRT_MAX	65535

	.macro MOVE_TO_R4_R5_AND_REPORT a, b
	LOAD_IMMEDIATE r4, \a
	LOAD_IMMEDIATE r5, \b

	/* During development, add REPORT_xxx statements here to see the
	   operands.  */
	.endm

	.macro MOVE_TO_R4_AND_REPORT_I a, b
	LOAD_IMMEDIATE r4, \a

	/* During development, add REPORT_xxx statements here to see the
	   operands.  */
	.endm

	.macro SHOULD_BE_SET
	OR1K_DELAYED_NOP (l.bnf  failed)
	.endm

	.macro SHOULDNT_BE_SET
	OR1K_DELAYED_NOP (l.bf    failed)
	.endm

	.macro SHOULD_BE_LESS_THAN_SIGNED a, b
	MOVE_TO_R4_R5_AND_REPORT \a , \b

	l.sfeq  r4, r5
	SHOULDNT_BE_SET
	l.sfne  r4, r5
	SHOULD_BE_SET
	l.sfgts r4, r5
	SHOULDNT_BE_SET
	l.sfges r4, r5
	SHOULDNT_BE_SET
	l.sfles r4, r5
	SHOULD_BE_SET
	l.sflts r4, r5
	SHOULD_BE_SET
	.endm

	.macro SHOULD_BE_GREATER_THAN_SIGNED a, b
	MOVE_TO_R4_R5_AND_REPORT \a , \b

	l.sfeq  r4, r5
	SHOULDNT_BE_SET
	l.sfne  r4, r5
	SHOULD_BE_SET
	l.sfgts r4, r5
	SHOULD_BE_SET
	l.sfges r4, r5
	SHOULD_BE_SET
	l.sfles r4, r5
	SHOULDNT_BE_SET
	l.sflts r4, r5
	SHOULDNT_BE_SET
	.endm

	.macro SHOULD_BE_LESS_THAN_UNSIGNED a, b
	MOVE_TO_R4_R5_AND_REPORT \a , \b

	l.sfeq  r4, r5
	SHOULDNT_BE_SET
	l.sfne  r4, r5
	SHOULD_BE_SET
	l.sfgtu r4, r5
	SHOULDNT_BE_SET
	l.sfgeu r4, r5
	SHOULDNT_BE_SET
	l.sfleu r4, r5
	SHOULD_BE_SET
	l.sfltu r4, r5
	SHOULD_BE_SET
	.endm

	.macro SHOULD_BE_GREATER_THAN_UNSIGNED a, b
	MOVE_TO_R4_R5_AND_REPORT \a , \b

	l.sfeq  r4, r5
	SHOULDNT_BE_SET
	l.sfne  r4, r5
	SHOULD_BE_SET
	l.sfgtu r4, r5
	SHOULD_BE_SET
	l.sfgeu r4, r5
	SHOULD_BE_SET
	l.sfleu r4, r5
	SHOULDNT_BE_SET
	l.sfltu r4, r5
	SHOULDNT_BE_SET
	.endm

	.macro SHOULD_BE_EQUAL a, b
	MOVE_TO_R4_R5_AND_REPORT \a , \b

	l.sfeq  r4, r5
	SHOULD_BE_SET
	l.sfne  r4, r5
	SHOULDNT_BE_SET

	/* Signed tests.  */
	l.sfgts r4, r5
	SHOULDNT_BE_SET
	l.sfges r4, r5
	SHOULD_BE_SET
	l.sfles r4, r5
	SHOULD_BE_SET
	l.sflts r4, r5
	SHOULDNT_BE_SET

	/* Unsigned tests.  */
	l.sfgtu r4, r5
	SHOULDNT_BE_SET
	l.sfgeu r4, r5
	SHOULD_BE_SET
	l.sfleu r4, r5
	SHOULD_BE_SET
	l.sfltu r4, r5
	SHOULDNT_BE_SET
	.endm

	.macro SHOULDNT_BE_EQUAL a, b
	MOVE_TO_R4_R5_AND_REPORT \a , \b

	l.sfeq  r4, r5
	SHOULDNT_BE_SET
	l.sfne  r4, r5
	SHOULD_BE_SET
	.endm

	.macro SHOULD_BE_EQUAL_I a, b
	MOVE_TO_R4_AND_REPORT_I \a, \b

	l.sfeqi  r4, \b
	SHOULD_BE_SET
	l.sfnei  r4, \b
	SHOULDNT_BE_SET

	/* Signed tests.  */
	l.sfgtsi r4, \b
	SHOULDNT_BE_SET
	l.sfgesi r4, \b
	SHOULD_BE_SET
	l.sflesi r4, \b
	SHOULD_BE_SET
	l.sfltsi r4, \b
	SHOULDNT_BE_SET

	/* Unsigned tests.  */
	l.sfgtui r4, \b
	SHOULDNT_BE_SET
	l.sfgeui r4, \b
	SHOULD_BE_SET
	l.sfleui r4, \b
	SHOULD_BE_SET
	l.sfltui r4, \b
	SHOULDNT_BE_SET
	.endm

	.macro SHOULDNT_BE_EQUAL_I a, b
	MOVE_TO_R4_AND_REPORT_I \a, \b

	l.sfeqi  r4, \b
	SHOULDNT_BE_SET
	l.sfnei  r4, \b
	SHOULD_BE_SET
	.endm

	.macro SHOULD_BE_LESS_THAN_SIGNED_I a, b
	MOVE_TO_R4_AND_REPORT_I \a, \b

	l.sfeqi  r4, \b
	SHOULDNT_BE_SET
	l.sfnei  r4, \b
	SHOULD_BE_SET
	l.sfgtsi r4, \b
	SHOULDNT_BE_SET
	l.sfgesi r4, \b
	SHOULDNT_BE_SET
	l.sflesi r4, \b
	SHOULD_BE_SET
	l.sfltsi r4, \b
	SHOULD_BE_SET
	.endm

	.macro SHOULD_BE_GREATER_THAN_SIGNED_I a, b
	MOVE_TO_R4_AND_REPORT_I \a, \b

	l.sfeqi  r4, \b
	SHOULDNT_BE_SET
	l.sfnei  r4, \b
	SHOULD_BE_SET
	l.sfgtsi r4, \b
	SHOULD_BE_SET
	l.sfgesi r4, \b
	SHOULD_BE_SET
	l.sflesi r4, \b
	SHOULDNT_BE_SET
	l.sfltsi r4, \b
	SHOULDNT_BE_SET
	.endm

	.macro SHOULD_BE_LESS_THAN_UNSIGNED_I a, b
	MOVE_TO_R4_AND_REPORT_I \a, \b

	l.sfeqi  r4, \b
	SHOULDNT_BE_SET
	l.sfnei  r4, \b
	SHOULD_BE_SET
	l.sfgtui r4, \b
	SHOULDNT_BE_SET
	l.sfgeui r4, \b
	SHOULDNT_BE_SET
	l.sfleui r4, \b
	SHOULD_BE_SET
	l.sfltui r4, \b
	SHOULD_BE_SET
	.endm

	.macro SHOULD_BE_GREATER_THAN_UNSIGNED_I a, b
	MOVE_TO_R4_AND_REPORT_I \a, \b

	l.sfeqi  r4, \b
	SHOULDNT_BE_SET
	l.sfnei  r4, \b
	SHOULD_BE_SET
	l.sfgtui r4, \b
	SHOULD_BE_SET
	l.sfgeui r4, \b
	SHOULD_BE_SET
	l.sfleui r4, \b
	SHOULDNT_BE_SET
	l.sfltui r4, \b
	SHOULDNT_BE_SET
	.endm

	STANDARD_TEST_ENVIRONMENT

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Signed tests  */

	SHOULD_BE_LESS_THAN_SIGNED 0, 1
	SHOULD_BE_LESS_THAN_SIGNED MIN1, 0
	SHOULD_BE_LESS_THAN_SIGNED INT_MAX_MIN1, INT_MAX
	SHOULD_BE_LESS_THAN_SIGNED NEG_INT_MAX, INT_MAX
	SHOULD_BE_LESS_THAN_SIGNED NEG_INT_MAX, INT_MAX_MIN1
	SHOULD_BE_LESS_THAN_SIGNED NEG_INT_MAX_PL1, INT_MAX
	SHOULD_BE_LESS_THAN_SIGNED NEG_INT_MAX_PL1, INT_MAX_MIN1
	SHOULD_BE_LESS_THAN_SIGNED -7, -6
	SHOULD_BE_LESS_THAN_SIGNED NEG_INT_MAX, NEG_INT_MAX_PL1
	SHOULD_BE_LESS_THAN_SIGNED NEG_INT_MAX, MIN1
	SHOULD_BE_LESS_THAN_SIGNED NEG_INT_MAX, 0

	SHOULD_BE_GREATER_THAN_SIGNED 1, 0
	SHOULD_BE_GREATER_THAN_SIGNED 0, MIN1
	SHOULD_BE_GREATER_THAN_SIGNED INT_MAX, INT_MAX_MIN1
	SHOULD_BE_GREATER_THAN_SIGNED INT_MAX, NEG_INT_MAX
	SHOULD_BE_GREATER_THAN_SIGNED INT_MAX_MIN1, NEG_INT_MAX
	SHOULD_BE_GREATER_THAN_SIGNED INT_MAX, NEG_INT_MAX_PL1
	SHOULD_BE_GREATER_THAN_SIGNED INT_MAX_MIN1, NEG_INT_MAX_PL1
	SHOULD_BE_GREATER_THAN_SIGNED -6, -7
	SHOULD_BE_GREATER_THAN_SIGNED NEG_INT_MAX_PL1, NEG_INT_MAX
	SHOULD_BE_GREATER_THAN_SIGNED MIN1, NEG_INT_MAX
	SHOULD_BE_GREATER_THAN_SIGNED 0, NEG_INT_MAX

	/* See the immediate tests below.  */
	SHOULD_BE_LESS_THAN_SIGNED 0xFFFF7FFF, 0xFFFF8000
	/* See the immediate tests below.  */
	SHOULD_BE_GREATER_THAN_SIGNED 0xFFFF8001, 0xFFFF8000

	/* Signed tests, immediate  */

	SHOULD_BE_LESS_THAN_SIGNED_I	0,  1
	SHOULD_BE_LESS_THAN_SIGNED_I  -1,  0
	SHOULD_BE_LESS_THAN_SIGNED_I  -7, -6

	SHOULD_BE_GREATER_THAN_SIGNED_I 0x00008000, 0x7FFF
	SHOULD_BE_LESS_THAN_SIGNED_I 0xFFFFFFFF, 0x7FFF
	/* 0x8000 gets sign-extended to 0xFFFF8000.  */
	SHOULD_BE_LESS_THAN_SIGNED_I 0xFFFF7FFF, 0x8000
	/* 0x8000 gets sign-extended to 0xFFFF8000.  */
	SHOULD_BE_GREATER_THAN_SIGNED_I 0xFFFF8001, 0x8000
	/* 0x8000 gets sign-extended to 0xFFFF8000.  */
	SHOULD_BE_GREATER_THAN_SIGNED_I 0x00008000, 0x8000

	/* Unsigned tests  */

	SHOULD_BE_LESS_THAN_UNSIGNED 0, 1
	SHOULD_BE_LESS_THAN_UNSIGNED UINT_MAX_MIN1, UINT_MAX
	SHOULD_BE_GREATER_THAN_UNSIGNED 1, 0
	SHOULD_BE_GREATER_THAN_UNSIGNED UINT_MAX, UINT_MAX_MIN1
	SHOULD_BE_GREATER_THAN_UNSIGNED UINT_MAX, 0
	SHOULD_BE_GREATER_THAN_UNSIGNED 0x80000001,  0x80000000
	SHOULD_BE_LESS_THAN_UNSIGNED 0x80000000,  0x80000001
	SHOULD_BE_GREATER_THAN_UNSIGNED 0x80000000, 0x7fffffff
	SHOULD_BE_LESS_THAN_UNSIGNED 0x7fffffff,  0x80000000
	SHOULD_BE_GREATER_THAN_UNSIGNED 0x7fffffff,  0x7ffffffe
	SHOULD_BE_LESS_THAN_UNSIGNED 0x7ffffffe,  0x7fffffff
	SHOULD_BE_LESS_THAN_UNSIGNED 0x2024fae0,  0xfef03220

	/* Unsigned tests, immediate  */

	SHOULD_BE_LESS_THAN_UNSIGNED_I 0, 1
	SHOULD_BE_GREATER_THAN_UNSIGNED_I 1, 0
	SHOULD_BE_LESS_THAN_UNSIGNED_I	 SHRT_MAX - 1, SHRT_MAX
	SHOULD_BE_GREATER_THAN_UNSIGNED_I SHRT_MAX	 , SHRT_MAX - 1

	/* The sign extension produces unexpected results here.  */

	/* 0xFFFF gets sign-extended to 0xFFFFFFFF.  */
	SHOULD_BE_LESS_THAN_UNSIGNED_I 0xFFFFFFFF - 1, 0xFFFF
	/* 0x8000 gets sign-extended to 0xFFFF8000.  */
	SHOULD_BE_LESS_THAN_UNSIGNED_I 0xFFFF7FFF, 0x8000

	/* Equal tests.  */

	SHOULD_BE_EQUAL 0, 0
	SHOULD_BE_EQUAL UINT_MAX, UINT_MAX
	SHOULD_BE_EQUAL MIN1, UINT_MAX
	SHOULD_BE_EQUAL INT_MAX, INT_MAX
	SHOULD_BE_EQUAL NEG_INT_MAX, NEG_INT_MAX

	/* Equal tests, immediate.  Test the 16-to-32-bit sign extension.  */

	SHOULD_BE_EQUAL_I 0, 0
	SHOULD_BE_EQUAL_I 0x00007FFF, 0x7FFF
	SHOULD_BE_EQUAL_I 0xFFFF8000, 0x8000
	SHOULD_BE_EQUAL_I 0xFFFFFFFF, 0xFFFF

	/* Non-equal tests.  */

	SHOULDNT_BE_EQUAL 0, 1
	SHOULDNT_BE_EQUAL UINT_MAX, INT_MAX
	SHOULDNT_BE_EQUAL UINT_MAX, NEG_INT_MAX
	SHOULDNT_BE_EQUAL MIN1, NEG_INT_MAX_PL1
	SHOULDNT_BE_EQUAL INT_MAX, NEG_INT_MAX
	SHOULDNT_BE_EQUAL NEG_INT_MAX_PL1, UINT_MAX_MIN1

	/* Non-equal tests, immediate.  Test the 16-to-32-bit sign
	   extension.  */

	SHOULDNT_BE_EQUAL_I 0x00008000, 0x8000

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9

failed:
	EXIT_SIMULATION_WITH_IMMEDIATE_EXIT_CODE SEC_GENERIC_ERROR
