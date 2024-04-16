/* Testsuite architecture macros for OpenRISC.

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

#ifndef OR1K_ASM_TEST_H
#define OR1K_ASM_TEST_H

#include "spr-defs.h"

/* Register definitions */

/* The "jump and link" instructions store the return address in R9.  */
#define LINK_REGISTER_R9 r9

/* These register definitions match the ABI.  */
#define ZERO_R0          r0
#define STACK_POINTER_R1 r1
#define FRAME_POINTER_R2 r2
#define RETURN_VALUE_R11 r11

	/* Load/move/clear helpers  */

	.macro LOAD_IMMEDIATE  reg, val
	l.movhi \reg,       hi ( \val )
	l.ori   \reg, \reg, lo ( \val )
	.endm

	.macro MOVE_REG  dest_reg, src_reg
	.ifnes "\dest_reg","\src_reg"
	l.ori \dest_reg, \src_reg, 0
	.endif
	.endm

	.macro CLEAR_REG reg
	l.movhi \reg, 0
	.endm

	.macro MOVE_FROM_SPR  reg, spr_reg
	l.mfspr \reg, ZERO_R0, \spr_reg
	.endm

	.macro MOVE_TO_SPR  spr_reg, reg
	l.mtspr ZERO_R0, \reg, \spr_reg
	.endm

	.macro SET_SPR_SR_FLAGS flag_mask, scratch_reg_1, scratch_reg_2
	/* We cannot use PUSH and POP here because some flags like Carry
	   would get overwritten.  */

	/* We could optimise this routine, as instruction l.mtspr already
	   does a logical OR.  */
	MOVE_FROM_SPR \scratch_reg_2, SPR_SR
	LOAD_IMMEDIATE \scratch_reg_1, \flag_mask
	l.or \scratch_reg_2, \scratch_reg_2, \scratch_reg_1
	MOVE_TO_SPR SPR_SR, \scratch_reg_2
	.endm

	.macro CLEAR_SPR_SR_FLAGS flag_mask, scratch_reg_1, scratch_reg_2
	/* We cannot use PUSH and POP here because some flags like Carry
	   would get overwritten.  */

	MOVE_FROM_SPR \scratch_reg_2, SPR_SR
	LOAD_IMMEDIATE \scratch_reg_1, ~\flag_mask
	l.and \scratch_reg_2, \scratch_reg_2, \scratch_reg_1
	MOVE_TO_SPR SPR_SR, \scratch_reg_2

	.endm

	/* Stack helpers  */

/* This value is defined in the OpenRISC 1000 specification.  */
#define EXCEPTION_STACK_SKIP_SIZE  128

	/* WARNING: Functions without prolog cannot use these PUSH or POP
	   macros.

	   PERFORMANCE WARNING: These PUSH/POP macros are convenient, but
	   can lead to slow code.  If you need to PUSH or POP several
	   registers, it's faster to use non-zero offsets when
	   loading/storing and then increment/decrement the stack pointer
	   just once.  */

	.macro PUSH reg
	l.addi STACK_POINTER_R1, STACK_POINTER_R1, -4
	l.sw   0(STACK_POINTER_R1), \reg
	.endm

	/* WARNING: see the warnings for PUSH.  */
	.macro POP reg
	l.lwz  \reg, 0(STACK_POINTER_R1)
	l.addi STACK_POINTER_R1, STACK_POINTER_R1, 4
	.endm

/* l.nop definitions for simulation control and console output.  */

/* Register definitions for the simulation l.nop codes.  */
#define NOP_REPORT_R3 r3
#define NOP_EXIT_R3   r3

/* SEC = Simulation Exit Code  */
#define SEC_SUCCESS            0
#define SEC_RETURNED_FROM_MAIN 1
#define SEC_GENERIC_ERROR      2

	/* When running under the simulator, this l.nop code terminates the
	   simulation.  */
	.macro EXIT_SIMULATION_WITH_IMMEDIATE_EXIT_CODE immediate_value
	LOAD_IMMEDIATE NOP_EXIT_R3, \immediate_value
	l.nop 1
	.endm

	.macro EXIT_SIMULATION_WITH_REG_EXIT_CODE reg
	MOVE_REG NOP_EXIT_R3, \reg
	l.nop 1
	.endm

	/* When running under the simulator, this l.nop code prints the
	   value of R3 to the console.  */
	.macro REPORT_TO_CONSOLE
	l.nop 2
	.endm

	/* NOTE: The stack must be set up, as this macro uses PUSH and POP.  */
	.macro REPORT_REG_TO_CONSOLE reg
	.ifeqs "\reg","r3"
	 /* Nothing more to do here, R3 is the register that gets printed.  */
	 REPORT_TO_CONSOLE
	.else
	 PUSH     NOP_REPORT_R3
	 MOVE_REG NOP_REPORT_R3, \reg
	 REPORT_TO_CONSOLE
	 POP      NOP_REPORT_R3
	.endif
	.endm

	/* NOTE: The stack must be set up, as this macro uses PUSH and POP.  */
	.macro REPORT_IMMEDIATE_TO_CONSOLE val
	PUSH     NOP_REPORT_R3
	LOAD_IMMEDIATE NOP_REPORT_R3, \val
	REPORT_TO_CONSOLE
	POP      NOP_REPORT_R3
	.endm

	.macro PRINT_NEWLINE_TO_CONSOLE
	PUSH  r3
	LOAD_IMMEDIATE r3, 0x0A
	l.nop 4
	POP   r3
	.endm

	/* If SR[F] is set, writes 0x00000001 to the console, otherwise it
	   writes 0x00000000.  */
	.macro REPORT_SRF_TO_CONSOLE
	OR1K_DELAYED_NOP (l.bnf \@1$)
	REPORT_IMMEDIATE_TO_CONSOLE 0x00000001
	OR1K_DELAYED_NOP (l.j \@2$)
\@1$:
	REPORT_IMMEDIATE_TO_CONSOLE 0x00000000
\@2$:
	.endm

	/* If the given register is 0, writes 0x00000000 to the console,
	   otherwise it writes 0x00000001.  */
	.macro REPORT_BOOL_TO_CONSOLE  reg
	l.sfne \reg, ZERO_R0
	REPORT_SRF_TO_CONSOLE
	.endm

	/* Writes to the console the value of the given register bit.  */
	.macro REPORT_BIT_TO_CONSOLE  reg, single_bit_mask
	PUSH r2
	PUSH r3
	PUSH r4
	MOVE_REG r2, \reg
	LOAD_IMMEDIATE r4, \single_bit_mask
	l.and   r3, r2, r4
	REPORT_BOOL_TO_CONSOLE r3
	POP r4
	POP r3
	POP r2
	.endm

	/* Jump helpers */

	.macro CALL overwritten_reg, subroutine_name
	LOAD_IMMEDIATE \overwritten_reg, \subroutine_name
	OR1K_DELAYED_NOP (l.jalr  \overwritten_reg)
	.endm

	.macro RETURN_TO_LINK_REGISTER_R9
	OR1K_DELAYED_NOP (l.jr LINK_REGISTER_R9)
	.endm

	/* Clear the BSS section on start-up */

	.macro CLEAR_BSS overwritten_reg1, overwritten_reg2
	LOAD_IMMEDIATE \overwritten_reg1, _bss_begin
	LOAD_IMMEDIATE \overwritten_reg2, _bss_end
	l.sfgeu \overwritten_reg1, \overwritten_reg2
	OR1K_DELAYED_NOP (l.bf    bss_is_empty)
bss_clear_loop:
	/* Possible optimisation to investigate:
	   move "l.sw 0(\overwritten_reg1), r0" to the jump delay slot as
	   "l.sw -4(\overwritten_reg1), r0" or similar. But keep in mind that
	   there are plans to remove the jump delay slot.  */
	l.sw    0(\overwritten_reg1), r0
	l.addi  \overwritten_reg1, \overwritten_reg1, 4
	l.sfgtu \overwritten_reg2, \overwritten_reg1
	OR1K_DELAYED_NOP (l.bf    bss_clear_loop)
bss_is_empty:
	.endm

#endif /* OR1K_ASM_TEST_H */
