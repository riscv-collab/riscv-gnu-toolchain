# mach: bfin

.include "testutils.inc"
	start

	init_r_regs 0;
	ASTAT = R0;

	R0.L = 0x1234;
	R0.H = 0x7765;
	DBGA ( R0.L , 0x1234 );
	DBGA ( R0.H , 0x7765 );
	R0.L = -1;
	DBGA ( R0.H , 0x7765 );
	DBGA ( R0.L , 0xffff );

	R0.L = 0x5555;
	R0.H = 0xAAAA;
	DBGA ( R0.H , 0xAAAA );
	DBGA ( R0.L , 0x5555 );

	I0.L = 0x1234;
	I0.H = 0x256;
	R0 = I0;
	DBGA ( R0.L , 0x1234 );
	DBGA ( R0.H , 0x256 );

	R0 = -50;
	R1 = -77 (X);
	R2 = -99 (X);
	R3 = 32767 (X);
	R4 = -32768 (X);
	R5 = 256 (X);
	R6 = 128 (X);
	R7 = 1023 (X);
	DBGA ( R0.L , 0xffce );
	DBGA ( R1.L , 0xffb3 );
	DBGA ( R2.L , 0xff9d );
	DBGA ( R3.L , 0x7fff );
	DBGA ( R4.L , 0x8000 );
	DBGA ( R5.L , 256 );
	DBGA ( R6.L , 128 );
	DBGA ( R7.L , 1023 );

	R6 = -1;
	DBGA ( R6.L , 0xffff );

	R0.L = 0x5555;
	R1.L = 0xaaaa;

	DBGA ( R0.L , 0x5555 );
	DBGA ( R1.L , 0xaaaa );

	R0 = R0 + R1;
	DBGA ( R0.H , 0xfffe );

	pass
