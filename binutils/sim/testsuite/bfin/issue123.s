# mach: bfin

.include "testutils.inc"
	start

	R0.L = 0x7bb8;
	R0.H = 0x8d5e;
	R4.L = 0x7e1c;
	R4.H = 0x9e22;
// end load regs and acc;
	R6.H = R4.H * R0.L (M), R6.L = R4.L * R0.H (ISS2);

	_DBG R6;

	DBGA ( R6.L , 0x8000 );
	DBGA ( R6.H , 0x8000 );

//-------------

	pass
