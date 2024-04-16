/*  ALU test program.
 *  Test instructions
 *  (r7,r6) = +/- (r0,r1);
 *  (r7,r6) = +/- (r0,r1)s;
 */
# mach: bfin

.include "testutils.inc"
	start


// test positive overflow
	R0.L = 0xffff;
	R0.H = 0x7fff;
	R1.L = 0x0001;
	R1.H = 0x0000;
	R7 = 0;
	ASTAT = R7;
	R6 = R0 + R1, R7 = R0 - R1 (NS);
	DBGA ( R6.L , 0x0000 );
	DBGA ( R6.H , 0x8000 );
	DBGA ( R7.L , 0xfffe );
	DBGA ( R7.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// test positive overflow
	R0.L = 0xffff;
	R0.H = 0x7fff;
	R1.L = 0x0001;
	R1.H = 0x0000;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 + R1, R6 = R0 - R1 (NS);
	DBGA ( R6.L , 0xfffe );
	DBGA ( R6.H , 0x7fff );
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// test positive sat
	R0.L = 0xffff;
	R0.H = 0x7fff;
	R1.L = 0x0001;
	R1.H = 0x0000;
	R7 = 0;
	ASTAT = R7;
	R6 = R0 + R1, R7 = R0 - R1 (S);
	DBGA ( R6.L , 0xffff );
	DBGA ( R6.H , 0x7fff );
	DBGA ( R7.L , 0xfffe );
	DBGA ( R7.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// test positive sat
	R0.L = 0xffff;
	R0.H = 0x7fff;
	R1.L = 0x0001;
	R1.H = 0x0000;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 + R1, R6 = R0 - R1 (S);
	DBGA ( R6.L , 0xfffe );
	DBGA ( R6.H , 0x7fff );
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

	pass
