/* Tests the multiply instructions.

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
# output: report(0x00000002);\n
# output: report(0x00000003);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00008001);\n
# output: report(0x0000fffe);\n
# output: report(0x7ffffffe);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00008000);\n
# output: report(0x00010000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00010000);\n
# output: report(0x00010000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffffffe);\n
# output: report(0xfffffffd);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffff7fff);\n
# output: report(0xffff0002);\n
# output: report(0x7ffffffe);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffff7fff);\n
# output: report(0xffff0000);\n
# output: report(0x80010000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffff0000);\n
# output: report(0xfffeffff);\n
# output: report(0x00010000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000002);\n
# output: report(0xfffffffd);\n
# output: report(0xfffffffa);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffff8000);\n
# output: report(0x00010000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffff7fff);\n
# output: report(0x00010000);\n
# output: report(0x7fff0000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x80000000);\n
# output: report(0x00000001);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00008000);\n
# output: report(0x00010000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x00000002);\n
# output: report(0xfffffffd);\n
# output: report(0xfffffffa);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffff7fff);\n
# output: report(0xffff0000);\n
# output: report(0x80010000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x00000002);\n
# output: report(0x00000003);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00010002);\n
# output: report(0x00007fff);\n
# output: report(0x7ffffffe);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00020000);\n
# output: report(0x00004000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00040000);\n
# output: report(0x00004000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffffffe);\n
# output: report(0x0000fffd);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffefffe);\n
# output: report(0x00008001);\n
# output: report(0x7ffffffe);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffe0000);\n
# output: report(0x0000bfff);\n
# output: report(0x80020000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffdfffe);\n
# output: report(0x00008000);\n
# output: report(0x00010000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000002);\n
# output: report(0x0000fffd);\n
# output: report(0xfffffffa);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00010000);\n
# output: report(0x00008000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffdfffc);\n
# output: report(0x00004000);\n
# output: report(0x7fff0000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x80000000);\n
# output: report(0x00000001);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00020000);\n
# output: report(0x00004000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0xfffffffe);\n
# output: report(0x0000fffd);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffdfffe);\n
# output: report(0x00008000);\n
# output: report(0x00010000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x00000002);\n
# output: report(0x00000003);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00008001);\n
# output: report(0x0000fffe);\n
# output: report(0x7ffffffe);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00008000);\n
# output: report(0x00010000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00010000);\n
# output: report(0x00010000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffffffe);\n
# output: report(0xfffffffd);\n
# output: report(0x00000006);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffff7fff);\n
# output: report(0xffff0002);\n
# output: report(0x7ffffffe);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffff7fff);\n
# output: report(0xffff0000);\n
# output: report(0x80010000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffff0000);\n
# output: report(0xfffeffff);\n
# output: report(0x00010000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000002);\n
# output: report(0xfffffffd);\n
# output: report(0xfffffffa);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffff8000);\n
# output: report(0x00010000);\n
# output: report(0x80000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffff7fff);\n
# output: report(0x00010000);\n
# output: report(0x7fff0000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x80000000);\n
# output: report(0x00000001);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00008000);\n
# output: report(0x00010000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000002);\n
# output: report(0xfffffffd);\n
# output: report(0xfffffffa);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0xffff7fff);\n
# output: report(0xffff0000);\n
# output: report(0x80010000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: \n
# output: exit(0)\n

#include "or1k-asm-test-helpers.h"

	STANDARD_TEST_ENVIRONMENT

	.section .exception_vectors

	/* Range exception.  */
	.org	0xb00

	/* The handling is a bit dubious at present.  We just patch the
	   instruction with l.nop and restart.  This will go wrong in branch
	   delay slots, but we are not testing that here.  */
	l.addi r1, r1, -EXCEPTION_STACK_SKIP_SIZE
	PUSH r2
	PUSH r3
	/* Save the address of the instruction that caused the problem.  */
	MOVE_FROM_SPR r2, SPR_EPCR_BASE
	LOAD_IMMEDIATE r3, 0x15000000 /* Opcode for l.nop  */
	l.sw	0(r2), r3
	POP r3
	POP r2
	l.addi r1, r1, EXCEPTION_STACK_SKIP_SIZE
	l.rfe

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Test l.mul  */

	/* Multiply two small positive numbers.  This should set no flags.
	   */
	TEST_INST_I32_I32 l.mul, 0x00000002, 0x00000003

	/* Multiply two quite large positive numbers.  This should set no
	   flags  */
	TEST_INST_I32_I32 l.mul, 0x00008001, 0x0000fffe

	/* Multiply two slightly too large positive numbers.  This should
	   set the overflow, but not the carry flag .  */
	TEST_INST_I32_I32 l.mul, 0x00008000, 0x00010000

	/* Multiply two large positive numbers.  This should set the
	   overflow flags (even though the result is not a negative
	   number.  */
	TEST_INST_I32_I32 l.mul, 0x00010000, 0x00010000

	/* Multiply two small negative numbers.  This will set no flags.  */
	TEST_INST_I32_I32 l.mul, 0xfffffffe, 0xfffffffd

	/* Multiply two quite large negative numbers.  This will no flags.  */
	TEST_INST_I32_I32 l.mul, 0xffff7fff, 0xffff0002

	/* Multiply two slightly too large negative numbers.  This should
	   set the overflow flag.  */
	TEST_INST_I32_I32 l.mul, 0xffff7fff, 0xffff0000

	/* Multiply two large negative numbers.  This should set the
	   both the carry and overflow flags (even though the result is a
	   positive number.  */
	TEST_INST_I32_I32 l.mul, 0xffff0000, 0xfffeffff

	/* Multiply one small negative number and one small positive
	   number.  This will set the no flags.  */
	TEST_INST_I32_I32 l.mul, 0x00000002, 0xfffffffd

	/* Multiply one quite large negative number and one quite large
	   positive number.  This will set no flags.  */
	TEST_INST_I32_I32 l.mul, 0xffff8000, 0x00010000

	/* Multiply one slightly too large negative number and one slightly
	   too large positive number.  This should set the overflow flag.  */
	TEST_INST_I32_I32 l.mul, 0xffff7fff, 0x00010000

	/* Multiply the largest negative number by positive unity.  This
	   should set neither carry, nor overflow flag.  */
	TEST_INST_I32_I32 l.mul, 0x80000000, 0x00000001

	/* Check that range exceptions are triggered.  */

	SET_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Check that an overflow alone causes a RANGE Exception.  */
	TEST_INST_I32_I32 l.mul, 0x00008000, 0x00010000

	/* Check multiply of a negative and positive does not cause a RANGE
	   Exception.  */
	TEST_INST_I32_I32 l.mul, 0x00000002, 0xfffffffd

	/* Check that negative overflow causes a RANGE exception.  */
	TEST_INST_I32_I32 l.mul, 0xffff7fff, 0xffff0000

	CLEAR_SPR_SR_FLAGS SPR_SR_OVE, r2, r3


	/* Test l.muli  */

	/* Multiply two small positive numbers.  This should set no flags.  */
	TEST_INST_I32_I16 l.muli, 0x00000002, 0x0003

	/* Multiply two quite large positive numbers.  This should set no
	   flags */
	TEST_INST_I32_I16 l.muli, 0x00010002, 0x7fff

	/* Multiply two slightly too large positive numbers.  This should
	   set the overflow, but not the carry flag.  */
	TEST_INST_I32_I16 l.muli, 0x00020000, 0x4000

	/* Multiply two large positive numbers.  This should set the
	   overflow flag, even though the result is not a negative number.  */
	TEST_INST_I32_I16 l.muli, 0x00040000, 0x4000

	/* Multiply two small negative numbers.  This should set no flags.  */
	TEST_INST_I32_I16 l.muli, 0xfffffffe, 0xfffd

	/* Multiply two quite large negative numbers.  This will set no
	   flags.  */
	TEST_INST_I32_I16 l.muli, 0xfffefffe, 0x8001

	/* Multiply two slightly too large negative numbers.  This should
	   set the overflow flag.  */
	TEST_INST_I32_I16 l.muli, 0xfffe0000, 0xbfff

	/* Multiply two large negative numbers.  This should set the
	   overflow flag, even though the result is a positive number.  */
	TEST_INST_I32_I16 l.muli, 0xfffdfffe, 0x8000

	/* Multiply one small negative number and one small positive
	   number.  This will set no flags.  */
	TEST_INST_I32_I16 l.muli, 0x00000002, 0xfffd

	/* Multiply one quite large negative number and one quite large
	   positive number.  This will set no flags.  */
	TEST_INST_I32_I16 l.muli, 0x00010000, 0x8000

	/* Multiply one slightly too large negative number and one slightly
	   too large positive number.  This will set the overflow flag.  */
	TEST_INST_I32_I16 l.muli, 0xfffdfffc, 0x4000

	/* Multiply the largest negative number by positive unity.  Should
	   set neither carry, nor overflow flag.  */
	TEST_INST_I32_I16 l.muli, 0x80000000, 0x0001

	/* Check that range exceptions are triggered.  */

	SET_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Check that an overflow alone causes a RANGE Exception.  */
	TEST_INST_I32_I16 l.muli, 0x00020000, 0x4000

	/* Check that two negatives will not cause a RANGE Exception.  */
	TEST_INST_I32_I16 l.muli, 0xfffffffe, 0xfffd

	/* Check that multiply of larget negative and positive numbers causes
	   a RANGE exception and overflow.  */
	TEST_INST_I32_I16 l.muli, 0xfffdfffe, 0x8000

	CLEAR_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Test l.mulu  */

	/* Multiply two small positive numbers.  This should set no flags.  */
	TEST_INST_I32_I32 l.mulu, 0x00000002, 0x00000003

	/* Multiply two quite large positive numbers.  This should set no
	   flags.  */
	TEST_INST_I32_I32 l.mulu, 0x00008001, 0x0000fffe

	/* Multiply two slightly too large positive numbers.  This will set
	   no flags.  */
	TEST_INST_I32_I32 l.mulu, 0x00008000, 0x00010000

	/* Multiply two large positive numbers.  This will set the overflow
	   flag.  */
	TEST_INST_I32_I32 l.mulu, 0x00010000, 0x00010000

	/* Multiply two small negative numbers.  This will set the
	   carry flag, but not the overflow flag.  */
	TEST_INST_I32_I32 l.mulu, 0xfffffffe, 0xfffffffd

	/* Multiply two quite large negative numbers.  This will set the
	   carry flag, but not the overflow flag.  */
	TEST_INST_I32_I32 l.mulu, 0xffff7fff, 0xffff0002

	/* Multiply two slightly too large negative numbers.  This will set
	   the carry flag, and not the overflow flag  */
	TEST_INST_I32_I32 l.mulu, 0xffff7fff, 0xffff0000

	/* Multiply two large negative numbers.  This will set the both the
	   carry flag (even though the result is a positive number.)  */
	TEST_INST_I32_I32 l.mulu, 0xffff0000, 0xfffeffff

	/* Multiply one small negative number and one small positive
	   number.  This will set the carry flag, but not the overflow
	   flag.  */
	TEST_INST_I32_I32 l.mulu, 0x00000002, 0xfffffffd

	/* Multiply one quite large negative number and one quite large
	   positive number.  This will set the carry flag, but not the
	   overflow flag.  */
	TEST_INST_I32_I32 l.mulu, 0xffff8000, 0x00010000

	/* Multiply one slightly too large negative number and one slightly
	   too large positive number.  This will set the carry flag, but
	   not the overflow flag.  */
	TEST_INST_I32_I32 l.mulu, 0xffff7fff, 0x00010000

	/* Multiply the largest negative number by positive unity.  Should
	   set neither carry, nor overflow flag.  */
	TEST_INST_I32_I32 l.mulu, 0x80000000, 0x00000001

	/* Check that range exceptions are never triggered.  */

	SET_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Check that what would cause an overflow alone in 2's complement
	   does not cause a RANGE Exception.  */
	TEST_INST_I32_I32 l.mulu, 0x00008000, 0x00010000

	/* Check that a carry causes a RANGE Exception.  */
	TEST_INST_I32_I32 l.mulu, 0x00000002, 0xfffffffd

	/* Check that what would cause an overflow and carry in 2's
	   complement causes a RANGE Exception.  */
	TEST_INST_I32_I32 l.mulu, 0xffff7fff, 0xffff0000

	CLEAR_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
