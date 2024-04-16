# Blackfin testcase for SEQSTAT register
# mach: bfin

	.include "testutils.inc"

	.macro seqstat_test val:req
	imm32 R0, \val
	SEQSTAT = R0;
	R1 = SEQSTAT;
	CC = R7 == R1;
	IF !CC JUMP 1f;
	.endm

	start

	# Writes to SEQSTAT should be ignored
	R7 = SEQSTAT;

	seqstat_test 0
	seqstat_test 0x1
	seqstat_test -1
	seqstat_test 0xab11cd22

	pass
1:	fail
