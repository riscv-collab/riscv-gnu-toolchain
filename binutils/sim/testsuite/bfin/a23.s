//  Test ALU  ABS accumulators
# mach: bfin

.include "testutils.inc"
	start


	R0 = 0x00000000;
	A0.w = R0;
	R0 = 0x80 (X);
	A0.x = R0;

	A0 = ABS A0;
	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0xffff );	DBGA ( R4.L , 0xffff );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x007f );

	R0 = 0x00000001;
	A0.w = R0;
	R0 = 0x80 (X);
	A0.x = R0;

	A0 = ABS A0;
	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0xffff );	DBGA ( R4.L , 0xffff );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x007f );

	R0 = 0xffffffff;
	A0.w = R0;
	R0 = 0xff (X);
	A0.x = R0;

	A0 = ABS A0;
	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x0000 );	DBGA ( R4.L , 0x0001 );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x0000 );

	R0 = 0xfffffff0;
	A0.w = R0;
	R0 = 0x7f (X);
	A0.x = R0;

	A0 = ABS A0;
	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0xffff );	DBGA ( R4.L , 0xfff0 );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x007f );

	R0 = 0x00000000;
	A0.w = R0;
	R0 = 0x80 (X);
	A0.x = R0;

	A1 = ABS A0;
	R4 = A1.w;
	R5 = A1.x;
	DBGA ( R4.H , 0xffff );	DBGA ( R4.L , 0xffff );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x007f );

	R0 = 0x00000000;
	A0.w = R0;
	R0 = 0x80 (X);
	A0.x = R0;

	R0 = 0x00000002;
	A1.w = R0;
	R0 = 0x80 (X);
	A1.x = R0;

	A1 = ABS A1, A0 = ABS A0;
	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0xffff );	DBGA ( R4.L , 0xffff );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x007f );

	R4 = A1.w;
	R5 = A1.x;
	DBGA ( R4.H , 0xffff );	DBGA ( R4.L , 0xfffe );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x007f );

	pass
