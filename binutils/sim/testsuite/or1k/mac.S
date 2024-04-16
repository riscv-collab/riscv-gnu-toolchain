/* Tests the MAC instructions.

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
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x0000000c);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x40000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0xffffffff);\n
# output: report(0x00000006);\n
# output: report(0x80000000);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x7ffffffe);\n
# output: report(0x00000000);\n
# output: report(0x80000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x7fffffff);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x7ffffffd);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x0000000c);\n
# output: report(0x00000000);\n
# output: report(0x00000005);\n
# output: report(0xffffffff);\n
# output: report(0xfffffffa);\n
# output: report(0x00000000);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0xffffffff);\n
# output: report(0x7fffffff);\n
# output: report(0xfffffff9);\n
# output: report(0xffffffff);\n
# output: report(0xfffffff9);\n
# output: report(0xfffffffe);\n
# output: report(0xffffffff);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0xffffffff);\n
# output: report(0x80000000);\n
# output: report(0xffffffff);\n
# output: report(0x80000006);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x7fffffff);\n
# output: report(0x7fffffff);\n
# output: report(0xffffffff);\n
# output: report(0x7fffffff);\n
# output: report(0xfffffffe);\n
# output: report(0xffffffff);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x0000000c);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x40000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0xffffffff);\n
# output: report(0x00000006);\n
# output: report(0x80000000);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x7ffffffe);\n
# output: report(0x00000000);\n
# output: report(0x80000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x7fffffff);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x7ffffffd);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x0000000c);\n
# output: report(0x00000000);\n
# output: report(0x00000005);\n
# output: report(0xffffffff);\n
# output: report(0xfffffffa);\n
# output: report(0x00000000);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0xffffffff);\n
# output: report(0x7fffffff);\n
# output: report(0xfffffff9);\n
# output: report(0xffffffff);\n
# output: report(0xfffffff9);\n
# output: report(0xfffffffe);\n
# output: report(0xffffffff);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0xffffffff);\n
# output: report(0x80000000);\n
# output: report(0xffffffff);\n
# output: report(0x80000006);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x7fffffff);\n
# output: report(0x7fffffff);\n
# output: report(0xffffffff);\n
# output: report(0x7fffffff);\n
# output: report(0xfffffffe);\n
# output: report(0xffffffff);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000006);\n
# output: report(0x0000000c);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000006);\n
# output: report(0x00000006);\n
# output: report(0x7ffffffe);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0xffffffff);\n
# output: report(0x7ffffffd);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000006);\n
# output: report(0x0000000c);\n
# output: report(0x00000005);\n
# output: report(0xfffffffa);\n
# output: report(0x00000006);\n
# output: report(0xffffffff);\n
# output: report(0xfffffff9);\n
# output: report(0xfffffff9);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x80000000);\n
# output: report(0x80000006);\n
# output: report(0x00000000);\n
# output: report(0x7fffffff);\n
# output: report(0x7fffffff);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0xffffffff);\n
# output: report(0xfffffffa);\n
# output: report(0x00000000);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0xfffffffa);\n
# output: report(0x3fffffff);\n
# output: report(0xfffffffa);\n
# output: report(0xffffffff);\n
# output: report(0xfffffff4);\n
# output: report(0xfffffffe);\n
# output: report(0xffffffff);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0xffffffff);\n
# output: report(0x80000002);\n
# output: report(0xffffffff);\n
# output: report(0x80000004);\n
# output: report(0x00000000);\n
# output: report(0x00000004);\n
# output: report(0x7ffffffe);\n
# output: report(0xffffffff);\n
# output: report(0xffffffff);\n
# output: report(0x80000001);\n
# output: report(0xffffffff);\n
# output: report(0x00000004);\n
# output: report(0xfffffffe);\n
# output: report(0x00000004);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000006);\n
# output: report(0xffffffff);\n
# output: report(0xfffffff9);\n
# output: report(0x00000000);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x0000000c);\n
# output: report(0x00000001);\n
# output: report(0x00000005);\n
# output: report(0x7fffffff);\n
# output: report(0xffffffff);\n
# output: report(0xffffffff);\n
# output: report(0xffffffff);\n
# output: report(0xffffffff);\n
# output: report(0x00000005);\n
# output: report(0x80000000);\n
# output: report(0x00000006);\n
# output: report(0x00000000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x80000006);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x7fffffff);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x7fffffff);\n
# output: report(0xffffffff);\n
# output: report(0x7fffffff);\n
# output: report(0x80000000);\n
# output: report(0x80000000);\n
# output: exit(0)\n

#include "or1k-asm-test-helpers.h"

	.macro TEST_MACRC mac_hi, mac_lo, op1, op2
	LOAD_IMMEDIATE r2, \mac_hi
	MOVE_TO_SPR SPR_MACHI, r2
	LOAD_IMMEDIATE r2, \mac_lo
	MOVE_TO_SPR SPR_MACLO, r2
	LOAD_IMMEDIATE r5, \op1
	LOAD_IMMEDIATE r6, \op2
	l.mac   r5, r6
	l.macrc r3
	REPORT_REG_TO_CONSOLE r3
	.endm

	.macro TEST_MAC mac_hi, mac_lo, op1, op2
	LOAD_IMMEDIATE r2, \mac_hi
	MOVE_TO_SPR SPR_MACHI, r2
	LOAD_IMMEDIATE r2, \mac_lo
	MOVE_TO_SPR SPR_MACLO, r2
	LOAD_IMMEDIATE r5, \op1
	LOAD_IMMEDIATE r6, \op2
	l.mac r5, r6
	MOVE_FROM_SPR r3, SPR_MACHI
	REPORT_REG_TO_CONSOLE r3
	MOVE_FROM_SPR r3, SPR_MACLO
	REPORT_REG_TO_CONSOLE r3
	.endm

	.macro TEST_MACI mac_hi, mac_lo, op1, op2_immediate
	LOAD_IMMEDIATE r2, \mac_hi
	MOVE_TO_SPR SPR_MACHI, r2
	LOAD_IMMEDIATE r2, \mac_lo
	MOVE_TO_SPR SPR_MACLO, r2
	LOAD_IMMEDIATE r5, \op1
	l.maci r5, \op2_immediate
	MOVE_FROM_SPR r3, SPR_MACHI
	REPORT_REG_TO_CONSOLE r3
	MOVE_FROM_SPR r3, SPR_MACLO
	REPORT_REG_TO_CONSOLE r3
	.endm

	.macro TEST_MSB mac_hi, mac_lo, op1, op2
	LOAD_IMMEDIATE r2, \mac_hi
	MOVE_TO_SPR SPR_MACHI, r2
	LOAD_IMMEDIATE r2, \mac_lo
	MOVE_TO_SPR SPR_MACLO, r2
	LOAD_IMMEDIATE r5, \op1
	LOAD_IMMEDIATE r6, \op2
	l.msb r5, r6
	MOVE_FROM_SPR r3, SPR_MACHI
	REPORT_REG_TO_CONSOLE r3
	MOVE_FROM_SPR r3, SPR_MACLO
	REPORT_REG_TO_CONSOLE r3
	.endm

	STANDARD_TEST_ENVIRONMENT

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Test the l.mac instruction.  */

	/* two small positive numbers  */

	/* MAC two small positive numbers on a zero total  */
	TEST_MAC 0x00000000, 0x00000000, 0x00000002, 0x00000003

	/* MAC two small positive numbers on a small positive total  */
	TEST_MAC 0x00000000, 0x00000006, 0x00000002, 0x00000003,

	/* MAC two small positive numbers on a moderate positive total  */
	TEST_MAC 0x00000000, 0xfffffffa, 0x00000002, 0x00000003

	/* MAC two small positive numbers on a large positive total  */
	TEST_MAC 0x3fffffff, 0xfffffffa, 0x00000002, 0x00000003

	/* MAC two small positive numbers on a small negative total  */
	TEST_MAC 0xffffffff, 0xfffffffa, 0x00000002, 0x00000003

	/* MAC two small positive numbers on a moderate negative total  */
	TEST_MAC 0xffffffff, 0x00000000, 0x00000002, 0x00000003

	/* MAC two small positive numbers on a large negative total  */
	TEST_MAC 0x80000000, 0x00000000, 0x00000002, 0x00000003

	/* two moderate positive numbers  */

	/* MAC two moderate positive numbers on a zero total  */
	TEST_MAC 0x00000000, 0x00000000, 0x00008001, 0x0000fffe

	/* MAC two moderate positive numbers on a small positive total  */
	TEST_MAC 0x00000000, 0x00000002, 0x00008001, 0x0000fffe

	/* MAC two moderate positive numbers on a moderate positive total  */
	TEST_MAC 0x00000000, 0x80000002, 0x00008001, 0x0000fffe

	/* MAC two moderate positive numbers on a large positive total  */
	TEST_MAC 0x7fffffff, 0x80000001, 0x00008001, 0x0000fffe

	/* MAC two moderate positive numbers on a small negative total  */
	TEST_MAC 0xffffffff, 0xffffffff, 0x00008001, 0x0000fffe

	/* MAC two moderate positive numbers on a moderate negative total  */
	TEST_MAC 0xffffffff, 0x80000002, 0x00008001, 0x0000fffe

	/* MAC two moderate positive numbers on a large negative total  */
	TEST_MAC 0xfffffffe, 0x80000002, 0x00008001, 0x0000fffe

	/* two small negative numbers  */

	/* MAC two small negative numbers on a zero total  */
	TEST_MAC 0x00000000, 0x00000000, 0xfffffffe, 0xfffffffd

	/* MAC two small negative numbers on a small positive total  */
	TEST_MAC 0x00000000, 0x00000006, 0xfffffffe, 0xfffffffd

	/* MAC two small negative numbers on a small negative total  */
	TEST_MAC 0xffffffff, 0xffffffff, 0xfffffffe, 0xfffffffd

	/* one small positive and one small negative  */

	/* MAC one small positive and one small negative number on a zero
	   total  */
	TEST_MAC 0x00000000, 0x00000000, 0x00000002, 0xfffffffd

	/* MAC one small positive and one small negative number on a small
	   positive total  */
	TEST_MAC 0x00000000, 0x0000000c, 0x00000002, 0xfffffffd

	/* MAC one small positive and one small negative number on a
	   moderate positive total  */
	TEST_MAC 0x00000001, 0x00000005, 0x00000002, 0xfffffffd

	/* MAC one small positive and one small negative number on a large
	   positive total  */
	TEST_MAC 0x7fffffff, 0xffffffff, 0x00000002, 0xfffffffd

	/* MAC one small positive and one small negative number on a small
	   negative total  */
	TEST_MAC 0xffffffff, 0xffffffff, 0x00000002, 0xfffffffd

	/* MAC one small positive and one small negative number on a
	   moderate negative total  */
	TEST_MAC 0xffffffff, 0x00000005, 0x00000002, 0xfffffffd

	/* MAC one small positive and one small negative number on a large
	   negative total  */
	TEST_MAC 0x80000000, 0x00000006, 0x00000002, 0xfffffffd

	/* one moderate positive and one moderate negative number  */

	/* MAC one moderate positive and one moderate negative number on a
	   zero total  */
	TEST_MAC 0x00000000, 0x00000000, 0x00008000, 0xffff0000

	/* MAC one moderate positive and one moderate negative number on a
	   small positive total  */
	TEST_MAC 0x00000000, 0x00000006, 0x00008000, 0xffff0000

	/* MAC one moderate positive and one moderate negative number on a
	   moderate positive total  */
	TEST_MAC 0x00000000, 0x80000000, 0x00008000, 0xffff0000

	/* MAC one moderate positive and one moderate negative number on a
	   large positive total  */
	TEST_MAC 0x7fffffff, 0xffffffff, 0x00008000, 0xffff0000

	/* MAC one moderate positive and one moderate negative number on a
	   small negative total  */
	TEST_MAC 0xffffffff, 0xffffffff, 0x00008000, 0xffff0000

	/* MAC one moderate positive and one moderate negative number on a
	   moderate negative total  */
	TEST_MAC 0xffffffff, 0x7fffffff, 0x00008000, 0xffff0000

	/* MAC one moderate positive and one moderate negative number on a
	   large negative total  */
	TEST_MAC 0x80000000, 0x80000000, 0x00008000, 0xffff0000

	/* Test the l.maci instruction.  */

	/* two small positive numbers  */

	/* MAC two small positive numbers on a zero total  */
	TEST_MACI 0x00000000, 0x00000000, 0x00000002, 0x0003

	/* MAC two small positive numbers on a small positive total  */
	TEST_MACI 0x00000000, 0x00000006, 0x00000002, 0x0003

	/* MAC two small positive numbers on a moderate positive total  */
	TEST_MACI 0x00000000, 0xfffffffa, 0x00000002, 0x0003

	/* MAC two small positive numbers on a large positive total  */
	TEST_MACI 0x3fffffff, 0xfffffffa, 0x00000002, 0x0003

	/* MAC two small positive numbers on a small negative total  */
	TEST_MACI 0xffffffff, 0xfffffffa, 0x00000002, 0x0003

	/* MAC two small positive numbers on a moderate negative total  */
	TEST_MACI 0xffffffff, 0x00000000, 0x00000002, 0x0003

	/* MAC two small positive numbers on a large negative total  */
	TEST_MACI 0x80000000, 0x00000000, 0x00000002, 0x0003

	/* two moderate positive numbers  */

	/* MAC two moderate positive numbers on a zero total  */
	TEST_MACI 0x00000000, 0x00000000, 0x00010002, 0x7fff

	/* MAC two moderate positive numbers on a small positive total  */
	TEST_MACI 0x00000000, 0x00000002, 0x00010002, 0x7fff

	/* MAC two moderate positive numbers on a moderate positive total  */
	TEST_MACI 0x00000000, 0x80000002, 0x00010002, 0x7fff

	/* MAC two moderate positive numbers on a large positive total  */
	TEST_MACI 0x7fffffff, 0x80000001, 0x00010002, 0x7fff

	/* MAC two moderate positive numbers on a small negative total  */
	TEST_MACI 0xffffffff, 0xffffffff, 0x00010002, 0x7fff

	/* MAC two moderate positive numbers on a moderate negative total  */
	TEST_MACI 0xffffffff, 0x80000002, 0x00010002, 0x7fff

	/* MAC two moderate positive numbers on a large negative total  */
	TEST_MACI 0xfffffffe, 0x80000002, 0x00010002, 0x7fff

	/* two small negative numbers  */

	/* MAC two small negative numbers on a zero total  */
	TEST_MACI 0x00000000, 0x00000000, 0xfffffffe, 0xfffd

	/* MAC two small negative numbers on a small positive total  */
	TEST_MACI 0x00000000, 0x00000006, 0xfffffffe, 0xfffd

	/* MAC two small negative numbers on a small negative total  */
	TEST_MACI 0xffffffff, 0xffffffff, 0xfffffffe, 0xfffd

	/* one small positive and one small negative  */

	/* MAC one small positive and one small negative number on a zero
	   total  */
	TEST_MACI 0x00000000, 0x00000000, 0x00000002, 0xfffd

	/* MAC one small positive and one small negative number on a small
	   positive total  */
	TEST_MACI 0x00000000, 0x0000000c, 0x00000002, 0xfffd

	/* MAC one small positive and one small negative number on a
	   moderate positive total  */
	TEST_MACI 0x00000001, 0x00000005, 0x00000002, 0xfffd

	/* MAC one small positive and one small negative number on a large
	   positive total  */
	TEST_MACI 0x7fffffff, 0xffffffff, 0x00000002, 0xfffd

	/* MAC one small positive and one small negative number on a small
	   negative total  */
	TEST_MACI 0xffffffff, 0xffffffff, 0x00000002, 0xfffd

	/* MAC one small positive and one small negative number on a
	   moderate negative total  */
	TEST_MACI 0xffffffff, 0x00000005, 0x00000002, 0xfffd

	/* MAC one small positive and one small negative number on a large
	   negative total  */
	TEST_MACI 0x80000000, 0x00000006, 0x00000002, 0xfffd

	/* one moderate positive and one moderate negative  */

	/* MAC one moderate positive and one moderate negative number on a
	   zero total  */
	TEST_MACI 0x00000000, 0x00000000, 0x00010000, 0x8000

	/* MAC one moderate positive and one moderate negative number on a
	   small positive total  */
	TEST_MACI 0x00000000, 0x00000006, 0x00010000, 0x8000

	/* MAC one moderate positive and one moderate negative number on a
	   moderate positive total  */
	TEST_MACI 0x00000000, 0x80000000, 0x00010000, 0x8000

	/* MAC one moderate positive and one moderate negative number on a
	   large positive total  */
	TEST_MACI 0x7fffffff, 0xffffffff, 0x00010000, 0x8000

	/* MAC one moderate positive and one moderate negative number on a
	   small negative total  */
	TEST_MACI 0xffffffff, 0xffffffff, 0x00010000, 0x8000

	/* MAC one moderate positive and one moderate negative number on a
	   moderate negative total  */
	TEST_MACI 0xffffffff, 0x7fffffff, 0x00010000, 0x8000

	/* MAC one moderate positive and one moderate negative number on a
	   large negative total  */
	TEST_MACI 0x80000000, 0x80000000, 0x00010000, 0x8000

	/* Test the l.macrc instruction.

	   Note that these tests use the same input data as the ones for
	   l.mac above.  The results are the same, but only the low 32-bits
	   are compared.  */

	/* two small positive numbers  */

	/* MAC two small positive numbers on a zero total  */
	TEST_MACRC 0x00000000, 0x00000000, 0x00000002, 0x00000003

	/* MAC two small positive numbers on a small positive total  */
	TEST_MACRC 0x00000000, 0x00000006, 0x00000002, 0x00000003

	/* MAC two small positive numbers on a moderate positive total  */
	TEST_MACRC 0x00000000, 0xfffffffa, 0x00000002, 0x00000003

	/* MAC two small positive numbers on a large positive total  */
	TEST_MACRC 0x3fffffff, 0xfffffffa, 0x00000002, 0x00000003

	/* MAC two small positive numbers on a small negative total  */
	TEST_MACRC 0xffffffff, 0xfffffffa, 0x00000002, 0x00000003

	/* MAC two small positive numbers on a moderate negative total  */
	TEST_MACRC 0xffffffff, 0x00000000, 0x00000002, 0x00000003

	/* MAC two small positive numbers on a large negative total  */
	TEST_MACRC 0x80000000, 0x00000000, 0x00000002, 0x00000003

	/* two moderate positive numbers  */

	/* MAC two moderate positive numbers on a zero total  */
	TEST_MACRC 0x00000000, 0x00000000, 0x00008001, 0x0000fffe

	/* MAC two moderate positive numbers on a small positive total  */
	TEST_MACRC 0x00000000, 0x00000002, 0x00008001, 0x0000fffe

	/* MAC two moderate positive numbers on a moderate positive total  */
	TEST_MACRC 0x00000000, 0x80000002, 0x00008001, 0x0000fffe

	/* MAC two moderate positive numbers on a large positive total  */
	TEST_MACRC 0x7fffffff, 0x80000001, 0x00008001, 0x0000fffe

	/* MAC two moderate positive numbers on a small negative total  */
	TEST_MACRC 0xffffffff, 0xffffffff, 0x00008001, 0x0000fffe

	/* MAC two moderate positive numbers on a moderate negative total  */
	TEST_MACRC 0xffffffff, 0x80000002, 0x00008001, 0x0000fffe

	/* MAC two moderate positive numbers on a large negative total  */
	TEST_MACRC 0xfffffffe, 0x80000002, 0x00008001, 0x0000fffe

	/* two small negative numbers  */

	/* MAC two small negative numbers on a zero total  */
	TEST_MACRC 0x00000000, 0x00000000, 0xfffffffe, 0xfffffffd

	/* MAC two small negative numbers on a small positive total  */
	TEST_MACRC 0x00000000, 0x00000006, 0xfffffffe, 0xfffffffd

	/* MAC two small negative numbers on a small negative total  */
	TEST_MACRC 0xffffffff, 0xffffffff, 0xfffffffe, 0xfffffffd

	/* one small positive and one small negative number  */

	/* MAC one small positive and one small negative number on a zero
	   total  */
	TEST_MACRC 0x00000000, 0x00000000, 0x00000002, 0xfffffffd

	/* MAC one small positive and one small negative number on a small
	   positive total  */
	TEST_MACRC 0x00000000, 0x0000000c, 0x00000002, 0xfffffffd

	/* MAC one small positive and one small negative number on a
	   moderate positive total  */
	TEST_MACRC 0x00000001, 0x00000005, 0x00000002, 0xfffffffd

	/* MAC one small positive and one small negative number on a large
	   positive total  */
	TEST_MACRC 0x7fffffff, 0xffffffff, 0x00000002, 0xfffffffd

	/* MAC one small positive and one small negative number on a small
	   negative total  */
	TEST_MACRC 0xffffffff, 0xffffffff, 0x00000002, 0xfffffffd

	/* MAC one small positive and one small negative number on a
	   moderate negative total  */
	TEST_MACRC 0xffffffff, 0x00000005, 0x00000002, 0xfffffffd

	/* MAC one small positive and one small negative number on a large
	   negative total  */
	TEST_MACRC 0x80000000, 0x00000006, 0x00000002, 0xfffffffd

	/* one moderate positive and one moderate negative  */

	/* MAC one moderate positive and one moderate negative number on a
	   zero total  */
	TEST_MACRC 0x00000000, 0x00000000, 0x00008000, 0xffff0000

	/* MAC one moderate positive and one moderate negative number on a
	   small positive total  */
	TEST_MACRC 0x00000000, 0x00000006, 0x00008000, 0xffff0000

	/* MAC one moderate positive and one moderate negative number on a
	   moderate positive total  */
	TEST_MACRC 0x00000000, 0x80000000, 0x00008000, 0xffff0000

	/* MAC one moderate positive and one moderate negative number on a
	   large positive total  */
	TEST_MACRC 0x7fffffff, 0xffffffff, 0x00008000, 0xffff0000

	/* MAC one moderate positive and one moderate negative number on a
	   small negative total  */
	TEST_MACRC 0xffffffff, 0xffffffff, 0x00008000, 0xffff0000

	/* MAC one moderate positive and one moderate negative number on a
	   moderate negative total  */
	TEST_MACRC 0xffffffff, 0x7fffffff, 0x00008000, 0xffff0000

	/* MAC one moderate positive and one moderate negative number on a
	   large negative total  */
	TEST_MACRC 0x80000000, 0x80000000, 0x00008000, 0xffff0000

	/* Test the l.msb instruction.  */

	/* MSB two small positive numbers on a zero total  */
	TEST_MSB 0x00000000, 0x00000000, 0x00000002, 0x00000003

	/* MSB two small positive numbers on a small positive total  */
	TEST_MSB 0x00000000, 0x0000000c, 0x00000002, 0x00000003

	/* MSB two small positive numbers on a moderate positive total  */
	TEST_MSB 0x00000001, 0x00000000, 0x00000002, 0x00000003

	/* MSB two small positive numbers on a large positive total  */
	TEST_MSB 0x40000000, 0x00000000, 0x00000002, 0x00000003

	/* MSB two small positive numbers on a small negative total  */
	TEST_MSB 0xffffffff, 0xfffffffa, 0x00000002, 0x00000003

	/* MSB two small positive numbers on a moderate negative total  */
	TEST_MSB 0xffffffff, 0x00000005, 0x00000002, 0x00000003

	/* MSB two small positive numbers on a large negative total  */
	TEST_MSB 0x80000000, 0x00000006, 0x00000002, 0x00000003

	/* two moderate positive numbers  */

	/* MSB two moderate positive numbers on a zero total  */
	TEST_MSB 0x00000000, 0x00000000, 0x00008001, 0x0000fffe

	/* MSB two moderate positive numbers on a small positive total  */
	TEST_MSB 0x00000000, 0x00000002, 0x00008001, 0x0000fffe

	/* MSB two moderate positive numbers on a moderate positive total  */
	TEST_MSB 0x00000000, 0x80000002, 0x00008001, 0x0000fffe

	/* MSB two moderate positive numbers on a large positive total  */
	TEST_MSB 0x7fffffff, 0x7ffffffd, 0x00008001, 0x0000fffe

	/* MSB two moderate positive numbers on a small negative total  */
	TEST_MSB 0xffffffff, 0xffffffff, 0x00008001, 0x0000fffe

	/* MSB two moderate positive numbers on a moderate negative total  */
	TEST_MSB 0xffffffff, 0x80000002, 0x00008001, 0x0000fffe

	/* MSB two moderate positive numbers on a large negative total  */
	TEST_MSB 0xfffffffe, 0x80000002, 0x00008001, 0x0000fffe

	/* two small negative numbers  */

	/* MSB two small negative numbers on a zero total  */
	TEST_MSB 0x00000000, 0x00000006, 0xfffffffe, 0xfffffffd

	/* MSB two small negative numbers on a small positive total  */
	TEST_MSB 0x00000000, 0x0000000c, 0xfffffffe, 0xfffffffd

	/* MSB two small negative numbers on a small negative total  */
	TEST_MSB 0xffffffff, 0xffffffff, 0xfffffffe, 0xfffffffd

	/* one small positive and one small negative number  */

	/* MSB one small positive and one small negative number on a zero
	   total  */
	TEST_MSB 0x00000000, 0x00000000, 0x00000002, 0xfffffffd

	/* MSB one small positive and one small negative number on a small
	   positive total  */
	TEST_MSB 0x00000000, 0x00000006, 0x00000002, 0xfffffffd

	/* MSB one small positive and one small negative number on a
	   moderate positive total  */
	TEST_MSB 0x00000000, 0xffffffff, 0x00000002, 0xfffffffd

	/* MSB one small positive and one small negative number on a large
	   positive total  */
	TEST_MSB 0x7fffffff, 0xfffffff9, 0x00000002, 0xfffffffd

	/* MSB one small positive and one small negative number on a small
	   negative total  */
	TEST_MSB 0xffffffff, 0xfffffff9, 0x00000002, 0xfffffffd

	/* MSB one small positive and one small negative number on a
	   moderate negative total  */
	TEST_MSB 0xfffffffe, 0xffffffff, 0x00000002, 0xfffffffd

	/* MSB one small positive and one small negative number on a large
	   negative total  */
	TEST_MSB 0x80000000, 0x00000000, 0x00000002, 0xfffffffd

	/* one moderate positive and one moderate negative number  */

	/* MSB one moderate positive and one moderate negative number on a
	   zero total  */
	TEST_MSB 0x00000000, 0x00000000, 0x00008000, 0xffff0000

	/* MSB one moderate positive and one moderate negative number on a
	   small positive total  */
	TEST_MSB 0x00000000, 0x00000006, 0x00008000, 0xffff0000

	/* MSB one moderate positive and one moderate negative number on a
	   moderate positive total  */
	TEST_MSB 0x00000000, 0x80000000, 0x00008000, 0xffff0000

	/* MSB one moderate positive and one moderate negative number on a
	   large positive total  */
	TEST_MSB 0x7fffffff, 0x7fffffff, 0x00008000, 0xffff0000

	/* MSB one moderate positive and one moderate negative number on a
	   small negative total  */
	TEST_MSB 0xffffffff, 0xffffffff, 0x00008000, 0xffff0000

	/* MSB one moderate positive and one moderate negative number on a
	   moderate negative total  */
	TEST_MSB 0xfffffffe, 0xffffffff, 0x00008000, 0xffff0000

	/* MSB one moderate positive and one moderate negative number on a
	   large negative total  */
	TEST_MSB 0x80000000, 0x00000000, 0x00008000, 0xffff0000

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
