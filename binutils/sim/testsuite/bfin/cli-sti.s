# Blackfin testcase for cli/sti instructions
# mach: bfin
# sim: --environment operating

	.include "testutils.inc"

	start

	# Make sure we can't mask <=EVT4
	R0 = 0;
	sti R0;
	cli R1;
	R2 = 0x1f;
	CC = R1 == R2;
	IF !CC JUMP 1f;

	# Make sure we can mask >=EVT5
	R0 = 0xff;
	sti R0;
	cli R1;
	CC = R0 == R1;
	IF !CC JUMP 1f;

	dbg_pass
1:	dbg_fail
