//  MAC test program.
//  Test basic edge values
//  UNSIGNED FRACTIONAL mode U
//  test ops: "+=" "-="
# mach: bfin

.include "testutils.inc"
	start


// load r0=0x80007fff
// load r1=0x80007fff
// load r2=0xf0000000
// load r3=0x0000007f
// load r4=0x00000080
// load r5=0xffffffff
	loadsym P0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];
	R5 = [ P0 ++ ];

	dbga(r0.h, 0x8000);
	dbga(r0.l, 0x7fff);
	dbga(r1.h, 0x8000);
	dbga(r1.l, 0x7fff);
	dbga(r2.h, 0xf000);
	dbga(r2.l, 0);

// 0x8000 * 0x7fff = 0x003fff8000
	A1 = A0 = 0;
	A1 += R0.H * R1.L, A0 += R0.H * R1.L (FU);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x8000 );
	DBGA ( R6.H , 0x3fff );
	DBGA ( R7.L , 0x0000 );
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0x8000 );
	DBGA ( R6.H , 0x3fff );
	DBGA ( R7.L , 0x0000 );

// 0x8000 * 0x8000 = 0x0040000000
	A1 = A0 = 0;
	A1 += R0.H * R1.H, A0 += R0.H * R1.H (FU);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x4000 );
	DBGA ( R7.L , 0x0000 );
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x4000 );
	DBGA ( R7.L , 0x0000 );

// 0xffff * 0xffff = 0x00fffe0001
	A1 = A0 = 0;
	A1 += R5.H * R5.H, A0 += R5.H * R5.H (FU);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0001 );
	DBGA ( R6.H , 0xfffe );
	DBGA ( R7.L , 0x0000 );
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0x0001 );
	DBGA ( R6.H , 0xfffe );
	DBGA ( R7.L , 0x0000 );

// saturate high by first loading large value into accums
// expected value is 0xffffffffff
	A1 = A0 = 0;
	A1.w = R5;
	A1.x = R5.L;
	A0.w = R5;
	A0.x = R5.L;
	A1 += R5.H * R5.H, A0 += R5.H * R5.H (FU);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0xffff );
	DBGA ( R7.L , 0xffff );
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0xffff );
	DBGA ( R7.L , 0xffff );

// saturate low with "-="
// expected value is 0x0000000000
	A1 = A0 = 0;
	A1 -= R4.L * R4.L, A0 -= R4.L * R4.L (FU);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x0000 );
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x0000 );

// saturate low with "-="
// expected value is 0x0000000000
	A1 = A0 = 0;
	A1 -= R1.H * R0.H, A0 -= R1.H * R0.H (FU);
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x0000 );
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x0000 );

	pass

	.data
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
	.dw 0xffff
	.dw 0xffff
