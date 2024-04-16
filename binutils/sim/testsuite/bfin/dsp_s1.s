/*  SHIFT test program.
 *  Test r0, r1, A0 >>= BITMUX;
 */
# mach: bfin

.include "testutils.inc"
	start

	init_r_regs 0;
	ASTAT = r0;

// load r0=0x80000009
// load r1=0x10000009
// load r2=0x0000000f
// load r3=0x00000000
// load r4=0x80000008
// load r5=0x00000000
	loadsym P0, data0;
	loadsym P1, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];
	R5 = [ P0 ++ ];

// insert two bits, both equal to 1
// A0: 00 0000 000f -> c0 0000 0003
// r0:    8000 0009 ->    4000 0004
// r1:    1000 0009 ->    0800 0004
	R0 = [ P1 + 0 ];
	R1 = [ P1 + 4 ];
	A0.w = R2;
	A0.x = R3.L;
	BITMUX( R0 , R1, A0) (ASR);
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0x0003 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0xffc0 );
	DBGA ( R0.L , 0x0004 );
	DBGA ( R0.H , 0x4000 );
	DBGA ( R1.L , 0x0004 );
	DBGA ( R1.H , 0x0800 );

// insert two bits, one equal to 1, other to 0
// A0: 00 0000 000f -> 40 0000 0003
// r0:    8000 0009 ->    4000 0004
// r4:    8000 0008 ->    4000 0004
	R0 = [ P1 + 0 ];
	R4 = [ P1 + 16 ];
	A0.w = R2;
	A0.x = R3.L;
	BITMUX( R0 , R4, A0) (ASR);
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0x0003 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x0040 );
	DBGA ( R0.L , 0x0004 );
	DBGA ( R0.H , 0x4000 );
	DBGA ( R4.L , 0x0004 );
	DBGA ( R4.H , 0x4000 );

	pass

	.data
data0:
	.dw 0x0009
	.dw 0x8000

	.dw 0x0009
	.dw 0x1000

	.dw 0x000f
	.dw 0x0000

	.dw 0x0000
	.dw 0x0000

	.dw 0x0008
	.dw 0x8000

	.dw 0x0000
	.dw 0x0000
