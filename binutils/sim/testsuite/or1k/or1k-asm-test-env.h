/* Testsuite macros for OpenRISC.

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

#ifndef OR1K_ASM_TEST_ENV_H
#define OR1K_ASM_TEST_ENV_H

#include "or1k-asm.h"
#include "or1k-asm-test.h"

	.macro STANDARD_TEST_HEADER
	/* Without the "a" (allocatable) flag, this section gets some
	   default flags and is discarded by objcopy when flattening to
	   the binary file.  */
	.section .exception_vectors, "ax"
	.org    0x100
	.global _start
_start:
	/* Clear R0 on start-up. There is no guarantee that R0 is hardwired
	   to zero and indeed it is not when simulating.  */
	CLEAR_REG r0
	OR1K_DELAYED_NOP(l.j test_startup)
	.section .text
test_startup:
	.endm

	.macro STANDARD_TEST_BODY
	LOAD_IMMEDIATE STACK_POINTER_R1, stack_begin
	CLEAR_BSS r3, r4
	CALL r3, start_tests
	EXIT_SIMULATION_WITH_IMMEDIATE_EXIT_CODE SEC_SUCCESS
	.section .stack
	.space 4096 /* We need more than EXCEPTION_STACK_SKIP_SIZE bytes.  */
stack_begin:
	.endm

	.macro STANDARD_TEST_ENVIRONMENT
	/* One of the test cases needs to do some tests before setting up
	   the stack and so on.  That's the reason this macro is split into
	   2 parts allowing the caller to inject code between the 2
	   initialisation phases.  */
	STANDARD_TEST_HEADER
	STANDARD_TEST_BODY
	.endm

#endif /* OR1K_ASM_TEST_ENV_H */
