# mach: bfin

.include "testutils.inc"
	start

	R0 = 0;
	R1 = 0;
	R2 = 0;
	R3 = 0;
	R0.L = -32768;
	R0.H = 32767;
	R1.L = 32767;
	R1.H = -32768;
	R2.H = (A1 = R0.L * R1.H) (M), R2.L = (A0 = R0.L * R1.L) (TFU);

	_DBG R2;
	DBGA ( R2.L , 0x3fff );
	DBGA ( R2.H , 0xc000 );

	R3 = ( A1 = R0.L * R1.H ) (M),  R2 = ( A0 = R0.L * R1.L )  (FU);

	_DBG R3;
	DBGA ( R3.L , 0 );
	DBGA ( R3.H , 0xc000 );

	pass
