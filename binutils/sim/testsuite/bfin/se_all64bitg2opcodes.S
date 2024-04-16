/*
 * Blackfin testcase for testing illegal/legal 64-bit opcodes (group 2)
 * from userspace.  we track all instructions which cause some sort of
 * exception when run from userspace, this is normally EXCAUSE :
 *  - 0x22 : illegal instruction combination
 * and walk every instruction from 0x0000 to 0xffff
 */

# mach: bfin
# sim: --environment operating

#include "test.h"
	.include "testutils.inc"

#define SE_ALL_BITS 16
#include "se_allopcodes.h"

.macro se_all_load_insn
	R2 = W[P5 + 6];
	R0 = R2;
.endm

.macro se_all_next_insn
	/* increment, and go again. */
	R0 = R2;

	R0 += 1;
	/* finish once we hit the 32bit limit */
	imm32 R1, 0x10000;
	CC = R1 == R0;
	IF CC JUMP pass_lvl;

	W[P5 + 6] = R0;
.endm

.macro se_all_insn_init
	MNOP || NOP || NOP;
.endm
.macro se_all_insn_table
	/* this table must be sorted, and end with zero */
	/* start	end		SEQSTAT */
	.dw 0x0001,	0x9bff,		0x22
	.dw 0x9c60,	0x9c7f,		0x22
	.dw 0x9ce0,	0x9cff,		0x22
	.dw 0x9d60,	0x9d7f,		0x22
	.dw 0x9e60,	0x9e7f,		0x22
	.dw 0x9ee0,	0x9eff,		0x22
	.dw 0x9f60,	0x9f7f,		0x22
	.dw 0xa000,	0xffff,		0x22
	.dw 0x0000,	0x0000,		0x00
.endm

	se_all_test
