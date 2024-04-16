# mach: bfin
.include "testutils.inc"
	start

	R0 = 0;
	CC = R0 == R0;

	IF CC JUMP 4;
	JUMP.S LL1;
	pass
LL1:
	fail
