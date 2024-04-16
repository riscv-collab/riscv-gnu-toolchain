/*  SHIFT test program.
 *  Test r0, r1, A0 <<= BITMUX;
 */
# mach: bfin

.include "testutils.inc"
	start

	init_r_regs 0;
	ASTAT = R0;

// load r0=0x90000001
// load r1=0x90000002
// load r2=0x00000000
// load r3=0x00000000
// load r4=0x20000002
// load r5=0x00000000
	loadsym P1, data0;

// insert two bits, both equal to 1
// A0: 00 0000 0000 -> 00 0000 0003
// r0:    9000 0001 ->    2000 0002
// r1:    9000 0002 ->    2000 0004
	R0 = [ P1 + 0 ];
	R1 = [ P1 + 4 ];
	A0.w = R2;
	A0.x = R3.L;
	BITMUX( R0 , R1, A0) (ASL);
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0x0003 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x0000 );
	DBGA ( R0.L , 0x0002 );
	DBGA ( R0.H , 0x2000 );
	DBGA ( R1.L , 0x0004 );
	DBGA ( R1.H , 0x2000 );

// insert two bits, one equal to 1, other to 0
// A0: 00 0000 0000 -> 00 0000 0001
// r0:    9000 0001 ->    2000 0002
// r4:    2000 0002 ->    4000 0004
	R0 = [ P1 + 0 ];
	R4 = [ P1 + 16 ];
	A0.w = R2;
	A0.x = R3.L;
	BITMUX( R0 , R4, A0) (ASL);
	R6 = A0.w;
	R7.L = A0.x;
	DBGA ( R6.L , 0x0001 );
	DBGA ( R6.H , 0x0000 );
	DBGA ( R7.L , 0x0000 );
	DBGA ( R0.L , 0x0002 );
	DBGA ( R0.H , 0x2000 );
	DBGA ( R4.L , 0x0004 );
	DBGA ( R4.H , 0x4000 );

	pass

	.data
data0:
	.dw 0x0001
	.dw 0x9000

	.dw 0x0002
	.dw 0x9000

	.dw 0x0000
	.dw 0x0000

	.dw 0x0000
	.dw 0x0000

	.dw 0x0002
	.dw 0x2000

	.dw 0x0000
	.dw 0x0000
