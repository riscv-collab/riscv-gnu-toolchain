# mach: bfin

.include "testutils.inc"
	start


	R0 = 1;
	DBGA ( R0.L , 1 );

	R1.L = 2;
	DBGA ( R1.L , 2 );

	R2 = 3;
	A0.x = R2;
	R0 = A0.x;
	DBGA ( R0.L , 3 );

	P0 = 4;
	R0 = P0;
	DBGA ( R0.L , 4 );

	R0 = 45;
	R1 = 22;
	A1 = R0.L * R1.L, A0 = R0.H * R1.H;
	_DBG A1;

	loadsym I2, foo;
	P0 = I2;
	R0 = 0x0333 (X);
	R3 = 0x0444 (X);

	R3.L = ( A0 = R0.L * R0.L ) || [ I2 ++ ] = R3 || NOP;
	DBGA ( R3.L , 0x14 );
	R0 = [ P0 ];
	DBGA ( R0.L , 0x0444 );

	pass

	.data
foo:
	.space (0x10);
