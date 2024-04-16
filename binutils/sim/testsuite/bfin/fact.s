# Blackfin testcase for factorial
# mach: bfin

	.include "testutils.inc"

	start

	.macro factorial num:req answer:req
	R0 = \num (Z);
	CALL _fact;
	imm32 r1, \answer;
	CC = R1 == R0;
	if CC JUMP 1f;
	fail
1:
	.endm

_test:
	factorial 1 1
	factorial 2 2
	factorial 3 6
	factorial 4 24
	factorial 5 120
	factorial 6 720
	factorial 7 5040
	factorial 8 40320
	factorial 9 362880
	factorial 10 3628800
	factorial 11 39916800
	factorial 12 479001600
# This is the real answer, but it overflows 32bits.  Since gas itself
# likes to choke on 64bit values when compiled for 32bit systems, just
# specify the truncated 32bit value since that's what the Blackfin will
# come up with too.
#	factorial 13 6227020800
	factorial 13 1932053504
	pass

_fact:
	LINK 0;
	[ -- SP ] = R7;
	CC = R0 < 2;
	IF CC JUMP 1f;
	R7 = R0;
	R0 += -1;
	CALL _fact;
	R0 *= R7;
1:
	R7 = [ SP ++ ];
	UNLINK;
	RTS;
