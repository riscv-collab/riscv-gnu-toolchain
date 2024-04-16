# mach: bfin

.include "testutils.inc"
	start


	R0.H = 0x1234;
	R0.L = 0x5678;
	loadsym P0, data0;

	[ P0 ] = R0;
	P1 = [ P0 ];
	_DBG P1;
	R1 = [ P0 ];
	_DBG R1;
	CC = R0 == R1;
	IF !CC JUMP abrt;

	W [ P0 ] = R0;
	R1 = W [ P0 ] (Z);
	R2 = R0;
	R2 <<= 16;
	R2 >>= 16;
	_DBG R1;
	CC = R2 == R1;
	IF !CC JUMP abrt;

	B [ P0 ] = R0;
	R1 = B [ P0 ] (Z);
	R2 = R0;
	R2 <<= 24;
	R2 >>= 24;
	_DBG R1;
	CC = R2 == R1;
	IF !CC JUMP abrt;
	pass
abrt:
	fail;

	.data
data0:
	.dd 0xDEADBEAF;
