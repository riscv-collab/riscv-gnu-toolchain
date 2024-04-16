/* Tests the divide instructions.

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
# output: report(0x0000000c);\n
# output: report(0x00000003);\n
# output: report(0x00000004);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x0000000b);\n
# output: report(0x00000003);\n
# output: report(0x00000003);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffffff4);\n
# output: report(0xfffffffd);\n
# output: report(0x00000004);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffffff5);\n
# output: report(0xfffffffd);\n
# output: report(0x00000003);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffffff4);\n
# output: report(0x00000003);\n
# output: report(0xfffffffc);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffffff5);\n
# output: report(0x00000003);\n
# output: report(0xfffffffd);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x0000000c);\n
# output: report(0xfffffffd);\n
# output: report(0xfffffffc);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x0000000b);\n
# output: report(0xfffffffd);\n
# output: report(0xfffffffd);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x0000000c);\n
# output: report(0x00000000);\n
# output: report(0xfffffffd);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffffff4);\n
# output: report(0x00000000);\n
# output: report(0xfffffffd);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x0000000c);\n
# output: report(0x00000000);\n
# output: report(0xfffffffd);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0xfffffff4);\n
# output: report(0x00000000);\n
# output: report(0xfffffffd);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x0000000c);\n
# output: report(0x00000003);\n
# output: report(0x00000004);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x0000000b);\n
# output: report(0x00000003);\n
# output: report(0x00000003);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffffff4);\n
# output: report(0xfffffffd);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffffff5);\n
# output: report(0xfffffffd);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffffff4);\n
# output: report(0x00000003);\n
# output: report(0x55555551);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffffff5);\n
# output: report(0x00000003);\n
# output: report(0x55555551);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x0000000c);\n
# output: report(0xfffffffd);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x0000000b);\n
# output: report(0xfffffffd);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x0000000c);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xfffffff4);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x0000000c);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0xfffffff4);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
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

	/* Test l.div  */

	/* Divide two positive numbers and check rounding.  Should set no
	   flags.  */
	TEST_INST_I32_I32 l.div, 0x0000000c, 0x00000003 /* 12 / 3 = 4  */
	TEST_INST_I32_I32 l.div, 0x0000000b, 0x00000003 /* 11 / 3 = 3  */

	/* Divide two negative numbers and check rounding.  Should set no
	   flags.  */
	TEST_INST_I32_I32 l.div, 0xfffffff4, 0xfffffffd
	TEST_INST_I32_I32 l.div, 0xfffffff5, 0xfffffffd

	/* Divide a negative number by a positive number and check
	   rounding.  Should set no flags.  */
	TEST_INST_I32_I32 l.div, 0xfffffff4, 0x00000003
	TEST_INST_I32_I32 l.div, 0xfffffff5, 0x00000003

	/* Divide a positive number by a negative number and check
	   rounding.  Should set no flags.  */
	TEST_INST_I32_I32 l.div, 0x0000000c, 0xfffffffd
	TEST_INST_I32_I32 l.div, 0x0000000b, 0xfffffffd

	/* Divide by zero.  This will set the overflow flag.  */
	TEST_INST_I32_I32 l.div, 0x0000000c, 0x00000000
	TEST_INST_I32_I32 l.div, 0xfffffff4, 0x00000000

	/* Check that range exceptions are triggered.  */

	SET_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Divide by zero.  This will set the overflow flag and trigger an
	   exception.  */
	TEST_INST_I32_I32 l.div, 0x0000000c, 0x00000000
	TEST_INST_I32_I32 l.div, 0xfffffff4, 0x00000000

	CLEAR_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Test l.divu  */

	/* Divide two positive numbers and check rounding.  Should set no
	   flags.  */
	TEST_INST_I32_I32 l.divu, 0x0000000c, 0x00000003
	TEST_INST_I32_I32 l.divu, 0x0000000b, 0x00000003

	/* Divide two numbers that would be negative under 2's complement
	   and check rounding.  Should set no flags.  */
	TEST_INST_I32_I32 l.divu, 0xfffffff4, 0xfffffffd
	TEST_INST_I32_I32 l.divu, 0xfffffff5, 0xfffffffd

	/* Divide a number that would be negative under 2's complement by a
	   number that would be positive under 2's complement and check
	   rounding.  This should set no flags.  */
	TEST_INST_I32_I32 l.divu, 0xfffffff4, 0x00000003
	TEST_INST_I32_I32 l.divu, 0xfffffff5, 0x00000003

	/* Divide a number that would be positive under 2's complement by a
	   number that would be negative under 2's complement and check
	   rounding.  This should set no flags.  */
	TEST_INST_I32_I32 l.divu, 0x0000000c, 0xfffffffd
	TEST_INST_I32_I32 l.divu, 0x0000000b, 0xfffffffd

	/* Divide by zero.  This will set the carry flag.  */
	TEST_INST_I32_I32 l.divu, 0x0000000c, 0x00000000
	TEST_INST_I32_I32 l.divu, 0xfffffff4, 0x00000000

	/* Check that range exceptions are triggered.  */

	SET_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Divide by zero.  This will set the carry flag and trigger an
	   exception.  */
	TEST_INST_I32_I32 l.divu, 0x0000000c, 0x00000000
	TEST_INST_I32_I32 l.divu, 0xfffffff4, 0x00000000

	CLEAR_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
