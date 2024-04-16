# Blackfin testcase for overflow
# mach: bfin

	.include "testutils.inc"

	start

	# add 0x80000000 + 0x80000000
	R1 = 1;
	R1 <<= 31;
	R0 = R1;
	R0 = R0 + R1;
	CC =  V;    // check to see if av0 and ac get set
	CC &= AC0;
	IF !CC JUMP art;
	R1 = 0;
	R1 += 0;
	CC = AZ;
	IF !CC JUMP art;
	pass

art:
	R0 = CC;
	R1 = 1 (Z);

	CC = R1 == R0
	if CC jump 1f;
	fail
1:
	pass
