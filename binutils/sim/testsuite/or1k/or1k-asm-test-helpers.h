/* Testsuite helpers for OpenRISC.

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

#ifndef OR1K_ASM_TEST_HELPERS_H
#define OR1K_ASM_TEST_HELPERS_H

#include "spr-defs.h"
#include "or1k-asm-test-env.h"

	/* During exception handling the instruction under test is
	   overwritten with a nop.  Here we check if that is the case and
	   report.  */

	.macro REPORT_EXCEPTION  instruction_addr
	PUSH r2
	PUSH r3
	LOAD_IMMEDIATE r3, \instruction_addr
	l.lws r2, 0(r3)
	LOAD_IMMEDIATE r3, 0x15000000 /* l.nop */
	l.sfeq r2, r3
	OR1K_DELAYED_NOP (l.bnf 1f)
	REPORT_IMMEDIATE_TO_CONSOLE 0x00000001
	OR1K_DELAYED_NOP (l.j 2f)
1:
	REPORT_IMMEDIATE_TO_CONSOLE 0x00000000
2:
	POP r3
	POP r2
	.endm

	/* Test that will set and clear sr flags, run instruction report
	   the result and whether or not there was an exception.

	   Arguments:
	     flags_to_set - sr flags to set
	     flags_to_clear - sr flags to clear
	     opcode - the instruction to execute
	     op1 - first argument to the instruction
	     op2 - second argument to the function

	   Reports:
	     report(0x00000001);\n op1
	     report(0x00000002);\n op1
	     report(0x00000003);\n result
	     report(0x00000000);\n 1 if carry
	     report(0x00000000);\n 1 if overflow
	     report(0x00000000);\n 1 if exception
	     \n */

	.macro TEST_INST_FF_I32_I32  flags_to_set, flags_to_clear, opcode, op1, op2
	LOAD_IMMEDIATE r5, \op1
	LOAD_IMMEDIATE r6, \op2
	REPORT_REG_TO_CONSOLE r5
	REPORT_REG_TO_CONSOLE r6
	/* Clear the last exception address.  */
	MOVE_TO_SPR SPR_EPCR_BASE, ZERO_R0
	SET_SPR_SR_FLAGS   \flags_to_set  , r2, r3
	CLEAR_SPR_SR_FLAGS \flags_to_clear, r2, r3
\@1$:	\opcode r4, r5, r6
	MOVE_FROM_SPR r2, SPR_SR /* Save the flags.  */
	REPORT_REG_TO_CONSOLE r4

	REPORT_BIT_TO_CONSOLE r2, SPR_SR_CY
	REPORT_BIT_TO_CONSOLE r2, SPR_SR_OV
	REPORT_EXCEPTION \@1$
	PRINT_NEWLINE_TO_CONSOLE
	.endm

	.macro TEST_INST_FF_I32_I16  flags_to_set, flags_to_clear, opcode, op1, op2
	LOAD_IMMEDIATE r5, \op1
	REPORT_REG_TO_CONSOLE r5
	REPORT_IMMEDIATE_TO_CONSOLE \op2
	SET_SPR_SR_FLAGS   \flags_to_set  , r2, r3
	CLEAR_SPR_SR_FLAGS \flags_to_clear, r2, r3
	/* Clear the last exception address.  */
	MOVE_TO_SPR SPR_EPCR_BASE, ZERO_R0
\@1$:	\opcode r4, r5, \op2
	MOVE_FROM_SPR r2, SPR_SR /* Save the flags.  */
	REPORT_REG_TO_CONSOLE r4
	REPORT_BIT_TO_CONSOLE r2, SPR_SR_CY
	REPORT_BIT_TO_CONSOLE r2, SPR_SR_OV
	REPORT_EXCEPTION \@1$
	PRINT_NEWLINE_TO_CONSOLE
	.endm

	.macro TEST_INST_I32_I32  opcode, op1, op2
	TEST_INST_FF_I32_I32 0, 0, \opcode, \op1, \op2
	.endm

	.macro TEST_INST_I32_I16  opcode, op1, op2
	TEST_INST_FF_I32_I16 0, 0, \opcode, \op1, \op2
	.endm

	.macro CHECK_CARRY_AND_OVERFLOW_NOT_SET  overwritten_reg1, overwritten_reg2
	MOVE_FROM_SPR \overwritten_reg1, SPR_SR

	LOAD_IMMEDIATE \overwritten_reg2, SPR_SR_CY + SPR_SR_OV
	l.and   \overwritten_reg1, \overwritten_reg1, \overwritten_reg2
	l.sfne \overwritten_reg1, ZERO_R0

	OR1K_DELAYED_NOP (l.bnf \@2$)

	EXIT_SIMULATION_WITH_IMMEDIATE_EXIT_CODE  SEC_GENERIC_ERROR
\@2$:
	.endm

#endif /* OR1K_ASM_TEST_HELPERS_H */
