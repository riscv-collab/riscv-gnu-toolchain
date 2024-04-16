# Blackfin testcase for branching into the middle of a hardware loop
# mach: bfin

	.include "testutils.inc"

	.macro test_prep lc:req
	loadsym P5, 1f;
	dmm32 LC0, \lc
	R5 = 0;
	R6 = 0;
	R7 = 0;
	.endm

	.macro test_check exp5:req, exp6:req, exp7:req, expLC:req
1:
	imm32 R4, \exp5;
	CC = R4 == R5;
	IF !CC JUMP 2f;
	imm32 R4, \exp6;
	CC = R4 == R6;
	IF !CC JUMP 2f;
	imm32 R4, \exp7;
	CC = R4 == R7;
	IF !CC JUMP 2f;
	R3 = LC0;
	imm32 R4, \expLC;
	CC = R4 == R3;
	IF !CC JUMP 2f;
	JUMP 3f;
2:	fail
3:
	.endm

	.macro test_rts entry:req, lc:req, exp5:req, exp6:req, exp7:req, expLC:req
	loadsym R1, \entry;
	RETS = R1;
	test_prep \lc
	RTS;
	test_check \exp5, \exp6, \exp7, \expLC
	.endm

	.macro test_jump entry:req, lc:req, exp5:req, exp6:req, exp7:req, expLC:req
	loadsym P1, \entry;
	test_prep \lc
	JUMP (P1);
	test_check \exp5, \exp6, \exp7, \expLC
	.endm

	start

	loadsym R1, hws;
	LT0 = R1;
	loadsym R1, hwe;
	LB0 = R1;

	test_rts hws, 0, 1, 1, 1, 0
	test_rts hws, 1, 1, 1, 1, 0
	test_rts hws, 2, 2, 2, 2, 0
	test_rts hws, 20, 20, 20, 20, 0

	test_rts hwm, 0, 0, 1, 1, 0
	test_rts hwm, 1, 0, 1, 1, 0
	test_rts hwm, 2, 1, 2, 2, 0
	test_rts hwm, 20, 19, 20, 20, 0

	test_rts hwe, 0, 0, 0, 1, 0
	test_rts hwe, 1, 0, 0, 1, 0
	test_rts hwe, 2, 1, 1, 2, 0
	test_rts hwe, 20, 19, 19, 20, 0

	test_rts hwp, 0, 0, 0, 0, 0
	test_rts hwp, 1, 0, 0, 0, 1
	test_rts hwp, 2, 0, 0, 0, 2

	test_jump hws, 0, 1, 1, 1, 0
	test_jump hws, 1, 1, 1, 1, 0
	test_jump hws, 2, 2, 2, 2, 0
	test_jump hws, 20, 20, 20, 20, 0

	test_jump hwm, 0, 0, 1, 1, 0
	test_jump hwm, 1, 0, 1, 1, 0
	test_jump hwm, 2, 1, 2, 2, 0
	test_jump hwm, 20, 19, 20, 20, 0

	test_jump hwe, 0, 0, 0, 1, 0
	test_jump hwe, 1, 0, 0, 1, 0
	test_jump hwe, 2, 1, 1, 2, 0
	test_jump hwe, 20, 19, 19, 20, 0

	test_jump hwp, 0, 0, 0, 0, 0
	test_jump hwp, 1, 0, 0, 0, 1
	test_jump hwp, 2, 0, 0, 0, 2

	pass

hws:	R5 += 1;
hwm:	R6 += 1;
hwe:	R7 += 1;
hwp:	JUMP (P5);
