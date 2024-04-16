# Blackfin testcase for unlink insn with any immediate value
# mach: bfin
# sim: --environment operating

#include "test.h"
	.include "testutils.inc"

	start

	/* Set up exception handler */
	imm32 P4, EVT3;
	loadsym R1, _evx;
	[P4] = R1;

	/* Lower to the code we want to single step through */
	loadsym P1, _usr;
	RETI = P1;

	imm32 FP, 0x800000
	imm32 R0, 0x12345678;
	[FP] = R0;
	imm32 R0, 0x87654321;
	[FP + 4] = R0;

	RTI;

_usr:
	imm32 FP, 0x800000

	.byte 0x01, 0xe8
.Linsn:
	.byte 0, 0

	imm32 R0, 0x12345678;
	R1 = FP;
	CC = R0 == R1;
	IF !CC jump _fail;

	imm32 R0, 0x87654321;
	R1 = RETS;
	CC = R0 == R1;
	IF !CC jump _fail;

	imm32 R0, 0x800008;
	R1 = SP;
	CC = R0 == R1;
	IF !CC jump _fail;

	loadsym P0, .Linsn;
	R0 = W[P0];
	R0 += 1;
	W[P0] = R0;
	SSYNC;

	R0 = R0.L;
	CC = R0 == 0;
	IF CC jump _pass;
	jump _usr;

	.align 4;
_evx:
	dbg_fail;

_pass:
	dbg_pass;

_fail:
	dbg_fail;
