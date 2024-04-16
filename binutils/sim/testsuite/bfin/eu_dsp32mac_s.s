// Check MAC with scaling
# mach: bfin

.include "testutils.inc"
	start


	R0 = 0;
	R1 = 0;
	R2 = 0;
	A1 = A0 = 0;
// The result accumulated in A1, and stored to a reg half
	R0.L = 23229;
	R0.H = -23724;
	R1.L = -313;
	R1.H = -17732;
	R2.H = ( A1 = R1.L * R0.L ), A0 += R1.L * R0.L (S2RND);
	_DBG R2;
	DBGA ( R2.H , 0xfe44 );

	R0 = 0;
	ASTAT = R0;	// clear all flags
	A0 = 0;
	A1 = 0;
	R0.H = 0x8000;
	R0.L = 0x7fff;
	R1.H = 0x7fff;
	R1.L = 0x8000;
	A1 = R0.H * R1.H (M), R0.L = ( A0 -= R0.H * R1.H ) (ISS2);
	_DBG R0;
	DBGA ( R0.L , 0x7fff );

	R0 += 0;	// clear flags
	NOP;
	NOP;
	NOP;
	NOP;
	pass
