/* Tests instructions l.add, l.addc, l.addi and l.addic.

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
# output: report(0x00000002);\n
# output: report(0x00000003);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000001);\n
# output: report(0x00000002);\n
# output: report(0x00000003);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffffffff);\n
# output: report(0xfffffffe);\n
# output: report(0xfffffffd);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x40000000);\n
# output: report(0x3fffffff);\n
# output: report(0x7fffffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x40000000);\n
# output: report(0x40000000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xc0000000);\n
# output: report(0xc0000000);\n
# output: report(0x80000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xbfffffff);\n
# output: report(0xbfffffff);\n
# output: report(0x7ffffffe);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x40000000);\n
# output: report(0x40000000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0xffffffff);\n
# output: report(0xfffffffe);\n
# output: report(0xfffffffd);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xbfffffff);\n
# output: report(0xbfffffff);\n
# output: report(0x7ffffffe);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x00000001);\n
# output: report(0x00000002);\n
# output: report(0x00000003);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffffffff);\n
# output: report(0xfffffffe);\n
# output: report(0xfffffffd);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x40000000);\n
# output: report(0x3fffffff);\n
# output: report(0x7fffffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x40000000);\n
# output: report(0x3fffffff);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x40000000);\n
# output: report(0x40000000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000000);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xc0000000);\n
# output: report(0xc0000000);\n
# output: report(0x80000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xc0000000);\n
# output: report(0xbfffffff);\n
# output: report(0x80000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xbfffffff);\n
# output: report(0xbfffffff);\n
# output: report(0x7ffffffe);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x40000000);\n
# output: report(0x40000000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x40000000);\n
# output: report(0x3fffffff);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0xffffffff);\n
# output: report(0xfffffffe);\n
# output: report(0xfffffffd);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000000);\n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xbfffffff);\n
# output: report(0xbfffffff);\n
# output: report(0x7ffffffe);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x00000001);\n
# output: report(0x00000002);\n
# output: report(0x00000003);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000001);\n
# output: report(0x00000002);\n
# output: report(0x00000003);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffffffff);\n
# output: report(0x0000fffe);\n
# output: report(0xfffffffd);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x7fff8000);\n
# output: report(0x00007fff);\n
# output: report(0x7fffffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x7fffc000);\n
# output: report(0x00004000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x80008000);\n
# output: report(0x00008000);\n
# output: report(0x80000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x80007fff);\n
# output: report(0x00008000);\n
# output: report(0x7fffffff);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x7fffc000);\n
# output: report(0x00004000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0xffffffff);\n
# output: report(0x0000fffe);\n
# output: report(0xfffffffd);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x80007fff);\n
# output: report(0x00008000);\n
# output: report(0x7fffffff);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x00000001);\n
# output: report(0x00000002);\n
# output: report(0x00000003);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffffffff);\n
# output: report(0x0000fffe);\n
# output: report(0xfffffffd);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x7fff8000);\n
# output: report(0x00007fff);\n
# output: report(0x7fffffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x7fff8000);\n
# output: report(0x00007fff);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x7fffc000);\n
# output: report(0x00004000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0xffffffff);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000000);\n
# output: report(0x0000ffff);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x80008000);\n
# output: report(0x00008000);\n
# output: report(0x80000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x80007fff);\n
# output: report(0x00008000);\n
# output: report(0x80000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x80007fff);\n
# output: report(0x00008000);\n
# output: report(0x7fffffff);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x7fffc000);\n
# output: report(0x00004000);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0x7fffc000);\n
# output: report(0x00003fff);\n
# output: report(0x80000000);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
# output: \n
# output: report(0xffffffff);\n
# output: report(0x0000fffe);\n
# output: report(0xfffffffd);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x00000000);\n
# output: report(0x0000ffff);\n
# output: report(0x00000000);\n
# output: report(0x00000001);\n
# output: report(0x00000000);\n
# output: report(0x00000000);\n
# output: \n
# output: report(0x80007fff);\n
# output: report(0x00008000);\n
# output: report(0x7fffffff);\n
# output: report(0x00000001);\n
# output: report(0x00000001);\n
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
	   delay slots.  But we don't have those in this test.  */
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

	/* Test l.add  */

	/* Add two small positive numbers  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.add, 1, 2

	/* The carry flag should be ignored.  */
	TEST_INST_FF_I32_I32 SPR_SR_CY, SPR_SR_OV, l.add, 1, 2

	/* Add two small negative numbers, which should set the carry flag
	   but not the overflow flag.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.add, -1, -2

	/* Add two quite large positive numbers.  Should set neither the
	   overflow nor the carry flag.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.add, 0x40000000, \
	  0x3fffffff

	/* Add two large positive numbers.  Should set the overflow, but
	   not the carry flag.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.add, 0x40000000, \
	  0x40000000

	/* Add two quite large negative numbers.  Should set the carry, but
	   not the overflow flag.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.add, -1073741824, \
	  -1073741824 /* -1073741824 = 0xC0000000  */

	/* Add two large negative numbers.  Should set both the overflow
	   and carry flags.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.add, 0xbfffffff, \
	  0xbfffffff

	/* Check that range exceptions are triggered.  */

	SET_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Check that an overflow alone causes a RANGE Exception.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.add, 0x40000000, \
	  0x40000000

	/* Check that a carry alone does not cause a RANGE Exception.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.add, 0xffffffff, \
	  0xfffffffe

	/* Check that carry and overflow together cause an exception.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.add, 0xbfffffff, \
	  0xbfffffff

	CLEAR_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Test l.addc  */

	/* Add two small positive numbers  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV,  l.addc, 1, 2

	/* Add two small negative numbers.  Sets the carry flag but not the
	   overflow flag.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.addc, -1, -2

	/* Add two quite large positive numbers.  Should set neither the
	   overflow nor the carry flag.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.addc, 0x40000000, \
	  0x3fffffff

	/* Add two quite large positive numbers with a carry in.  Should
	   set the overflow but not the carry flag.  */
	TEST_INST_FF_I32_I32 SPR_SR_CY, SPR_SR_OV, l.addc, 0x40000000, \
	  0x3fffffff

	/* Add two large positive numbers.  Should set the overflow, but
	   not the carry flag.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.addc, 0x40000000, \
	  0x40000000

	/* Add the largest unsigned value to zero with a carry.  This
	   potentially can break a simplistic test for carry that does not
	   consider the carry flag properly.  Do it both ways around.  */
	TEST_INST_FF_I32_I32 SPR_SR_CY, SPR_SR_OV, l.addc, -1,  0
	TEST_INST_FF_I32_I32 SPR_SR_CY, SPR_SR_OV, l.addc,  0, -1

	/* Add two quite large negative numbers.  Should set the carry, but
	   not the overflow flag.  Here -1073741824 is 0xC0000000.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.addc, -1073741824, \
	  -1073741824

	/* Add two quite large negative numbers that would overflow, with a
	   carry that just avoids the overflow.  Should set the carry, but
	   not the overflow flag.  Here -1073741824 is 0xC0000000 and
	   -1073741825 is 0xBFFFFFFF.  */
	TEST_INST_FF_I32_I32 SPR_SR_CY, SPR_SR_OV, l.addc, -1073741824, \
	  -1073741825

	/* Add two large negative numbers.  Should set both the overflow
	   and carry flags.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.addc, -1073741825, \
	  -1073741825

	/* Check that range exceptions are triggered.  */

	SET_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Check that an overflow alone causes a RANGE Exception, even when
	   it is the carry that causes the overflow.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.addc, 0x40000000, \
	  0x40000000
	TEST_INST_FF_I32_I32 SPR_SR_CY, SPR_SR_OV, l.addc, 0x40000000, \
	  0x3fffffff

	/* Check that a carry alone does not cause a RANGE Exception, even
	   when it is the carry that causes the overflow.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.addc, 0xffffffff, \
	  0xfffffffe
	TEST_INST_FF_I32_I32 SPR_SR_CY, SPR_SR_OV, l.addc, 0x00000000, \
	  0xffffffff

	/* Check that carry and overflow together cause an exception.  */
	TEST_INST_FF_I32_I32 0, SPR_SR_CY | SPR_SR_OV, l.addc, 0xbfffffff, \
	  0xbfffffff

	CLEAR_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Test l.addi  */

	/* Add two small positive numbers  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addi, 1, 2

	/* Check carry in is ignored.  */
	TEST_INST_FF_I32_I16 SPR_SR_CY, SPR_SR_OV, l.addi, 1, 2

	/* Add two small negative numbers.  Sets the carry flag but not the
	   overflow flag.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addi, 0xffffffff, \
	  0xfffe

	/* Add two quite large positive numbers.  Should set neither the
	   overflow nor the carry flag.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addi, 0x7fff8000, \
	  0x7fff

	/* Add two large positive numbers.  Should set the overflow, but
	   not the carry flag.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addi, 0x7fffc000, \
	  0x4000

	/* Add two quite large negative numbers.  Should set the carry, but
	   not the overflow flag.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addi, 0x80008000, \
	  0x8000

	/* Add two large negative numbers.  Should set both the overflow
	   and carry flags.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addi, 0x80007fff, \
	  0x8000

	/* Check that range exceptions are triggered.  */

	SET_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Check that an overflow alone causes a RANGE Exception.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addi, 0x7fffc000, \
	  0x4000

	/* Check that a carry alone does not cause a RANGE Exception.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addi, 0xffffffff, \
	  0xfffe

	/* Check that carry and overflow together cause an exception.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addi, 0x80007fff, \
	  0x8000

	CLEAR_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Test l.addi */

	/* Add two small positive numbers  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addic, 1, 2

	/* Add two small negative numbers.  Sets the carry flag but not the
	   overflow flag.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addic, 0xffffffff, \
	  0xfffe

	/* Add two quite large positive numbers.  Should set neither the
	   overflow nor the carry flag.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addic, 0x7fff8000, \
	  0x7fff

	/* Add two quite large positive numbers with a carry in.  Should
	   set the overflow but not the carry flag.  */
	TEST_INST_FF_I32_I16 SPR_SR_CY, SPR_SR_OV, l.addic, 0x7fff8000, 0x7fff

	/* Add two large positive numbers.  Should set the overflow, but
	   not the carry flag.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addic, 0x7fffc000, \
	  0x4000

	/* Add the largest unsigned value to zero with a carry.  This
	   potentially can break a simplistic test for carry that does not
	   consider the carry flag properly.  Do it both ways around.  */
	TEST_INST_FF_I32_I16 SPR_SR_CY, SPR_SR_OV, l.addic, 0xffffffff, 0x0000
	TEST_INST_FF_I32_I16 SPR_SR_CY, SPR_SR_OV, l.addic, 0x00000000, 0xffff

	/* Add two quite large negative numbers.  Should set the carry, but
	   not the overflow flag.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addic, 0x80008000, \
	  0x8000

	/* Add two quite large negative numbers that would overflow, with a
	   carry that just avoids the overflow.  This should set the carry,
	   but not the overflow flag.  */
	TEST_INST_FF_I32_I16 SPR_SR_CY, SPR_SR_OV, l.addic, 0x80007fff, 0x8000

	/* Add two large negative numbers.  Should set both the overflow
	   and carry flags.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addic, 0x80007fff, \
	  0x8000

	/* Check that range exceptions are triggered.  */

	SET_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	/* Check that an overflow alone causes a RANGE Exception, even when
	   it is the carry that causes the overflow.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addic, 0x7fffc000, \
	  0x4000
	TEST_INST_FF_I32_I16 SPR_SR_CY, SPR_SR_OV, l.addic, 0x7fffc000, 0x3fff

	/* Check that a carry alone does not cause a RANGE Exception, even
	   when it is the carry that causes the overflow.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addic, 0xffffffff, \
	  0xfffe
	TEST_INST_FF_I32_I16 SPR_SR_CY, SPR_SR_OV, l.addic, 0x00000000, 0xffff

	/* Check that carry and overflow together cause an exception.  */
	TEST_INST_FF_I32_I16 0, SPR_SR_CY | SPR_SR_OV, l.addic, 0x80007fff, \
	  0x8000

	CLEAR_SPR_SR_FLAGS SPR_SR_OVE, r2, r3

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9
