# Blackfin testcase for event processing
# mach: bfin

	.include "testutils.inc"

	start

	# Run enough instructions to trigger event processing
	# and thus cpu stopping/restarting

	R0 = 0;
	imm32 R1, 100000

3:
	R0 += 1;	# 1
	R0 += 1;
	R0 += 1;	# 3
	R0 += 1;
	R0 += 1;	# 5
	R0 += 1;
	R0 += 1;	# 7
	R0 += 1;
	R0 += 1;	# 9
	R0 += 1;
	R0 += 1;	# 11
	R0 += 1;
	R0 += 1;	# 13
	R0 += 1;
	R0 += 1;	# 15
	R0 += 1;
	R0 += 1;	# 17
	R0 += 1;
	R0 += 1;	# 19
	R0 += 1;

	CC = R0 < R1;
	IF CC JUMP 3b;

	CC = R0 == R1;
	IF !CC JUMP 1f;

	pass
1:
	fail
