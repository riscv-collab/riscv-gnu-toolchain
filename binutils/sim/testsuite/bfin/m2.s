//  MAC test program.
//  Test basic edge values
//  SIGNED FRACTIONAL mode
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

// 0x7fff * 0x7fff = 0x007ffe0002
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1 += R0.L * R1.L, A0 += R0.L * R1.L;
	R6 = A1.w;
	_DBG ASTAT;
	_DBG A0;
	R7.L = A1.x;
	_DBG ASTAT;
	DBGA ( R6.L , 0x0002 );
	DBGA ( R6.H , 0x7ffe );
	DBGA ( R7.L , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// 0x8000 * 0x7fff = 0xff80010000
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1 += R0.H * R1.L, A0 += R0.H * R1.L;
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x8001 );
	DBGA ( R7.L , 0xffff );
	_DBG ASTAT;
	R7 = ASTAT;
	DBGA (R7.H, 0x0);
	DBGA (R7.L, 0x0);

// 0x8000 * 0x8000 = 0x007fffffff
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1 += R0.H * R1.H, A0 += R0.H * R1.H;
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x8000 );
	DBGA ( R7.L , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// saturate positive by first loading large value into accums
// expected value is 0x7fffffffff
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1.w = R2;
	A1.x = R3.L;
	A0.w = R2;
	A0.x = R3.L;
	A1 += R0.L * R1.L, A0 += R0.L * R1.L;
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0xffff );
	DBGA ( R7.L , 0x007f );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x1 );

// saturate negative
// expected value is 0x8000000000
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1.x = R4.L;
	A0.x = R4.L;
	A1 += R0.L * R1.H, A0 += R0.L * R1.H;
	R6 = A1.w;
	_DBG ASTAT;
	R7.L = A1.x;
	_DBG ASTAT;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0xff80 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x1 );

// saturate positive with "-="
// expected value is 0x7fffffffff
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1.w = R2;
	A1.x = R3.L;
	A0.w = R2;
	A0.x = R3.L;
	A1 -= R0.H * R1.L, A0 -= R0.H * R1.L;
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0xffff );
	DBGA ( R7.L , 0x007f );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x1 );

// saturate negative with "-="
// expected value is 0x8000000000
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1.x = R4.L;
	A0.x = R4.L;
	A1 -= R0.L * R1.L, A0 -= R0.L * R1.L;
	R6 = A1.w;
	_DBG ASTAT;
	R7.L = A1.x;
	_DBG ASTAT;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0xff80 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x1 );

// 0x8000 * 0x8000 = 0xff80000001 with "-="
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1 -= R0.H * R1.H, A0 -= R0.H * R1.H;
	R6 = A1.w;
	_DBG ASTAT;
	R7.L = A1.x;
	_DBG ASTAT;

	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x8000 );
	DBGA ( R7.L , 0xffff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// 0x7fff * 0x7fff = 0x007ffe0002 with "="
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1 += R0.L * R1.L, A0 += R0.L * R1.L;
	A1 = R0.L * R1.L, A0 = R0.L * R1.L;
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0002 );
	DBGA ( R6.H , 0x7ffe );
	DBGA ( R7.L , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// 0x7fff * 0x7fff = 0x007ffe0002 with "NOP"
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1 += R0.L * R1.L;
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0002 );
	DBGA ( R6.H , 0x7ffe );
	DBGA ( R7.L , 0x0000 );
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// 0x8000 * 0x8000 = 0x007fffffff with "NOP"
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1 += R0.H * R1.H;
	_DBG A1;
	R6 = A1.w;
	R7.L = A1.x;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x8000 );
	DBGA ( R7.L , 0x0000 );

	R6 = A0.w;
	_DBG ASTAT;
	R7.L = A0.x;
	_DBG ASTAT;
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x0000 );
	R7 = ASTAT;	_dbg astat;
//AV1 AV1S should be 0.
	DBGA ( R7.H , 0x0000 );
	DBGA ( R7.L , 0x0000 );

	_DBG ASTAT;
	A1 = A0 = 0;
	_DBG A1;
	_DBG R0;	_DBG R1;
	A1 += R0.L * R1.L;	// make sure overflow flag is not set to zero
	_DBG A1;
	_DBG ASTAT;
	R7 = ASTAT;
//AV1S should be 0.
	DBGA ( R7.H, 0x0000 );
	DBGA ( R7.L, 0x0000 );

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
