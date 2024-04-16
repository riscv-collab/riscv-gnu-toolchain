# Blackfin testcase for branching out of the middle of a hardware loop
# mach: bfin

	.include "testutils.inc"

	.macro test_prep lc:req, sym:req
	imm32 P0, \lc
	loadsym P1, \sym
	R5 = 0;
	R6 = 0;
	R7 = 0;
	LSETUP (1f, 2f) LC0 = P0;
	.endm

	.macro test_check exp5:req, exp6:req, exp7:req, expLC
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

	start
	mnop;

test_jump_s:
	.macro test_jump_s lc:req
	test_prep \lc, 3f
1:	JUMP (P1);
	R5 += 1;
2:	R6 += 1;
	fail
3:	R7 += 1;
	test_check 0, 0, 1, \lc
	.endm
	test_jump_s 0
	test_jump_s 1
	test_jump_s 2
	test_jump_s 10

test_jump_m:
	.macro test_jump_m lc:req
	test_prep \lc, 3f
1:	R5 += 1;
	JUMP (P1);
2:	R6 += 1;
	fail
3:	R7 += 1;
	test_check 1, 0, 1, \lc
	.endm
	test_jump_m 0
	test_jump_m 1
	test_jump_m 2
	test_jump_m 10

test_jump_e:
	.macro test_jump_e lc:req, lcend:req
	test_prep \lc, 3f
1:	R5 += 1;
	R6 += 1;
2:	JUMP (P1);
	fail
3:	R7 += 1;
	test_check 1, 1, 1, \lcend
	.endm
	test_jump_e 0, 0
	test_jump_e 1, 0
	test_jump_e 2, 1
	test_jump_e 10, 9

test_call_s:
	.macro test_call_s lc:req, exp5:req, exp6:req, exp7:req
	test_prep \lc, __ret
1:	CALL (P1);
	R5 += 1;
2:	R6 += 1;
3:	R7 += 1;
	test_check \exp5, \exp6, \exp7, 0
	.endm
	test_call_s 0, 1, 1, 2
	test_call_s 1, 1, 1, 2
	test_call_s 2, 2, 2, 3
	test_call_s 10, 10, 10, 11

test_call_m:
	.macro test_call_m lc:req, exp5:req, exp6:req, exp7:req
	test_prep \lc, __ret
1:	R5 += 1;
	CALL (P1);
2:	R6 += 1;
3:	R7 += 1;
	test_check \exp5, \exp6, \exp7, 0
	.endm
	test_call_m 0, 1, 1, 2
	test_call_m 1, 1, 1, 2
	test_call_m 2, 2, 2, 3
	test_call_m 10, 10, 10, 11

test_call_e:
	.macro test_call_e lc:req, exp5:req, exp6:req, exp7:req
	test_prep \lc, __ret
1:	R5 += 1;
	R6 += 1;
2:	CALL (P1);
3:	R7 += 1;
	test_check \exp5, \exp6, \exp7, 0
	.endm
	test_call_e 0, 1, 1, 2
	test_call_e 1, 1, 1, 2
	test_call_e 2, 2, 2, 3
	test_call_e 10, 10, 10, 11

	pass

__ret:
	nop;nop;nop;
	R7 += 1;
	rts;
