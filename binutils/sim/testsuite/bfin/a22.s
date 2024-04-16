//  Test ALU  NEG accumulators
# mach: bfin

.include "testutils.inc"
	start


	R0 = 0xffffffff;
	A0.w = R0;
	R0 = 0x7f (X);
	A0.x = R0;
	A0 = - A0;
	_DBG A0;
	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x0000 );	DBGA ( R4.L , 0x0001 );
	DBGA ( R5.H , 0xffff );	DBGA ( R5.L , 0xff80 );

	R0 = 0x1;
	A0.w = R0;
	R0 = 0x0;
	A0.x = R0;
	A0 = - A0;
	R4 = A0.w;
	R5 = A0.x;
	_DBG A0;
	DBGA ( R4.H , 0xffff );	DBGA ( R4.L , 0xffff );
	DBGA ( R5.H , 0xffff );	DBGA ( R5.L , 0xffff );

	R0 = 0xffffffff;
	A0.w = R0;
	R0 = 0xff (X);
	A0.x = R0;
	A0 = - A0;
	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x0000 );	DBGA ( R4.L , 0x0001 );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x0000 );

	R0 = 0x00000000;
	A0.w = R0;
	R0 = 0x80 (X);
	A0.x = R0;
	A0 = - A0;
	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0xffff );	DBGA ( R4.L , 0xffff );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x007f );

//  NEG NEG
	R0 = 0x00000000;
	A0.w = R0;
	R0 = 0x80 (X);
	A0.x = R0;

	R0 = 0xffffffff;
	A1.w = R0;
	R0 = 0x7f (X);
	A1.x = R0;

	A1 = - A1, A0 = - A0;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0xffff );	DBGA ( R4.L , 0xffff );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x007f );

	R4 = A1.w;
	R5 = A1.x;
	_DBG A1;
	DBGA ( R4.H , 0x0000 );	DBGA ( R4.L , 0x0001 );
	DBGA ( R5.H , 0xffff );	DBGA ( R5.L , 0xff80 );

//  NEG NEG register
	R0.L = 0x0001;
	R0.H = 0x8000;

	R3 = - R0 (V);
	DBGA ( R3.H , 0x7fff );	DBGA ( R3.L , 0xffff );

	_DBG ASTAT;

	pass
