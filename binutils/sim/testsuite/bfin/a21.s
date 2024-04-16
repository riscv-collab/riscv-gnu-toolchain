//  Test ALU   RND RND12 RND20
# mach: bfin

.include "testutils.inc"
	start


// positive saturation
	R0 = 0xffffffff;
	A0.w = R0;
	A1.w = R0;
	R0 = 0x7f (X);
	A0.x = R0;
	A1.x = R0;
	R3 = A1 + A0, R4 = A1 - A0 (S);
	DBGA ( R3.H , 0x7fff );	DBGA ( R3.L , 0xffff );
	DBGA ( R4.H , 0x0000 );	DBGA ( R4.L , 0x0000 );

// neg saturation
	R0 = 0;
	A0.w = R0;
	A1.w = R0;
	R0 = 0x80 (X);
	A0.x = R0;
	A1.x = R0;
	R3 = A1 + A0, R4 = A1 - A0 (S);
	DBGA ( R3.H , 0x8000 );	DBGA ( R3.L , 0x0000 );
	DBGA ( R4.H , 0x0000 );	DBGA ( R4.L , 0x0000 );

// positive saturation
	R0 = 0xfffffff0;
	A0.w = R0;
	A1.w = R0;
	R0 = 0x01;
	A0.x = R0;
	A1.x = R0;
	R3 = A1 + A0, R4 = A1 - A0 (S);
	DBGA ( R3.H , 0x7fff );	DBGA ( R3.L , 0xffff );
	DBGA ( R4.H , 0x0000 );	DBGA ( R4.L , 0x0000 );

// no sat
	R0 = 0xfffffff0;
	A0.w = R0;
	A1.w = R0;
	R0 = 0x01;
	A0.x = R0;
	A1.x = R0;
	R3 = A1 + A0, R4 = A1 - A0 (NS);
	DBGA ( R3.H , 0xffff );	DBGA ( R3.L , 0xffe0 );
	DBGA ( R4.H , 0x0000 );	DBGA ( R4.L , 0x0000 );

// add and sub +1 -1
	R0 = 0x00000001;
	A0.w = R0;
	R0 = 0xffffffff;
	A1.w = R0;
	R0 = 0;
	A0.x = R0;
	R0 = 0xff (X);
	A1.x = R0;
	R3 = A1 + A0, R4 = A1 - A0 (NS);
	DBGA ( R3.H , 0x0000 );	DBGA ( R3.L , 0x0000 ); // 0
	DBGA ( R4.H , 0xffff );	DBGA ( R4.L , 0xfffe ); // -2

// should get the same with saturation
	R3 = A1 + A0, R4 = A1 - A0 (S);
	DBGA ( R3.H , 0x0000 );	DBGA ( R3.L , 0x0000 ); // 0
	DBGA ( R4.H , 0xffff );	DBGA ( R4.L , 0xfffe ); // -2

// add and sub -1 +1 but with reverse order of A0 A1
	R0 = 0x00000001;
	A0.w = R0;
	R0 = 0xffffffff;
	A1.w = R0;
	R0 = 0;
	A0.x = R0;
	R0 = 0xff (X);
	A1.x = R0;
	R3 = A0 + A1, R4 = A0 - A1 (NS);
	DBGA ( R3.H , 0x0000 );	DBGA ( R3.L , 0x0000 );
	DBGA ( R4.H , 0x0000 );	DBGA ( R4.L , 0x0002 );

	pass
