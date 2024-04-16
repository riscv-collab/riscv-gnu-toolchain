//  MAC test program.
//  Test basic edge values
//  SIGNED INTEGER mode
//  test ops: "+=" "-=" "=" "NOP"
# mach: bfin

.include "testutils.inc"
	start


// load r0=0x80007fff
// load r1=0x80007fff
// load r2=0xf0000000
// load r3=0x0000007f
// load r4=0x00000080
	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

// 0x7fff * 0x7fff = 0x003fff0001
	A1 = A0 = 0;
	A1 += R0.L * R1.L, A0 += R0.L * R1.L (IS);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0001 );
	DBGA ( R6.H , 0x3fff );
	DBGA ( R7.L , 0x0000 );

// 0x8000 * 0x7fff = 0xffc0008000
	A1 = A0 = 0;
	A1 += R0.H * R1.L, A0 += R0.H * R1.L (IS);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x8000 );
	DBGA ( R6.H , 0xc000 );
	DBGA ( R7.L , 0xffff );

// 0x8000 * 0x8000 = 0x0040000000
	A1 = A0 = 0;
	A1 += R0.H * R1.H, A0 += R0.H * R1.H (IS);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x4000 );
	DBGA ( R7.L , 0x0000 );

// saturate positive by first loading large value into accums
// expected value is 0x7fffffffff
	A1 = A0 = 0;
	A1.w = R2;
	A1.x = R3.L;
	A0.w = R2;
	A0.x = R3.L;
	A1 += R0.L * R1.L, A0 += R0.L * R1.L (IS);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0xffff );
	DBGA ( R7.L , 0x007f );

// saturate negative
// expected value is 0x8000000000
	A1 = A0 = 0;
	A1.x = R4.L;
	A0.x = R4.L;
	A1 += R0.L * R1.H, A0 += R0.L * R1.H (IS);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0xff80 );

// saturate positive with "-="
// expected value is 0x7fffffffff
	A1 = A0 = 0;
	A1.w = R2;
	A1.x = R3.L;
	A0.w = R2;
	A0.x = R3.L;
	A1 -= R0.H * R1.L, A0 -= R0.H * R1.L (IS);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0xffff );
	DBGA ( R7.L , 0x007f );

// saturate negative with "-="
// expected value is 0x8000000000
	A1 = A0 = 0;
	A1.x = R4.L;
	A0.x = R4.L;
	A1 -= R0.L * R1.L, A0 -= R0.L * R1.L (IS);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0xff80 );

// 0x8000 * 0x8000 = 0xffc0000000 with "-="
	A1 = A0 = 0;
	A1 -= R0.H * R1.H, A0 -= R0.H * R1.H (IS);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0xc000 );
	DBGA ( R7.L , 0xffff );

	pass

	.data 0x1000;
data0:
	.dw 0x7fff
	.dw 0x8000
	.dw 0x7fff
	.dw 0x8000
	.dw 0x0000
	.dw 0xf000
	.dw 0x007f
	.dw 0x0000
	.dw 0x0080
	.dw 0x0000
