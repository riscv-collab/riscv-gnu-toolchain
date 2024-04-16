/*  ALU test program.
 *  Test instructions
 *  r7 = +/- (r0,r1);
 *  r7 = -/+ (r0,r1);
 *  r7 = -/- (r0,r1);
 */

# mach: bfin

.include "testutils.inc"
	start

// test subtraction
	R0.L = 0x000f;
	R0.H = 0x0010;
	R1.L = 0x000f;
	R1.H = 0x0010;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 +|- R1;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0020 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// test overflow negative on subtraction
	R0.L = 0x8000;
	R0.H = 0x0010;
	R1.L = 0x0001;
	R1.H = 0x0010;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 +|- R1;
	DBGA ( R7.L , 0x7fff );
	DBGA ( R7.H , 0x0020 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// test saturate negative on subtraction +/-
	R0.L = 0x8000;
	R0.H = 0x0010;
	R1.L = 0x0001;
	R1.H = 0x0010;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 +|- R1 (S);
	DBGA ( R7.L , 0x8000 );
	DBGA ( R7.H , 0x0020 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// test saturate negative on subtraction -/+
	R0.L = 0x8000;
	R0.H = 0x8000;
	R1.L = 0x0001;
	R1.H = 0x0001;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 -|+ R1 (S);
	DBGA ( R7.L , 0x8001 );
	DBGA ( R7.H , 0x8000 );
	CC = AZ;	R5 = CC; DBGA ( R5.L , 0x0 );
	CC = AN;	R5 = CC; DBGA ( R5.L , 0x1 );
	CC =  V;	R5 = CC; DBGA ( R5.L , 0x1 );
	CC = AC0;	R5 = CC; DBGA ( R5.L , 0x0 );

// test saturate negative on subtraction -/-
	R0.L = 0x8000;
	R0.H = 0x8000;
	R1.L = 0x0001;
	R1.H = 0x0001;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 -|- R1 (S);
	DBGA ( R7.L , 0x8000 );
	DBGA ( R7.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// test saturate positive on subtraction -/+
	R0.L = 0x7fff;
	R0.H = 0x7fff;
	R1.L = 0xffff;
	R1.H = 0xffff;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 -|+ R1 (S);
	DBGA ( R7.L , 0x7ffe );
	DBGA ( R7.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

	pass
