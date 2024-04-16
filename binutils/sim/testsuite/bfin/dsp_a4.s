/*  ALU test program.
 *  Test instructions
 *  r3= + (r0,r0);
 *  r3= + (r0,r0) s;
 *  r3= - (r0,r0);
 *  r3= - (r0,r0) s;
 */
# mach: bfin

.include "testutils.inc"
	start


// overflow  positive
	R0.L = 0xffff;
	R0.H = 0x7fff;
	R7 = 0;
	ASTAT = R7;
	R3 = R0 + R0 (NS);
	DBGA ( R3.L , 0xfffe );
	DBGA ( R3.H , 0xffff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = VS;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );

// overflow  negative
	R0.L = 0x0000;
	R0.H = 0x8000;
	R7 = 0;
	ASTAT = R7;
	R3 = R0 + R0 (NS);
	DBGA ( R3.L , 0x0000 );
	DBGA ( R3.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// zero
	R0.L = 0xffff;
	R0.H = 0xffff;
	R1.L = 0x0001;
	R1.H = 0x0000;
	R7 = 0;
	ASTAT = R7;
	R3 = R1 + R0 (NS);
	DBGA ( R3.L , 0x0000 );
	DBGA ( R3.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// saturate positive
	R0.L = 0;
	R0.H = 0x7fff;
	R7 = 0;
	ASTAT = R7;
	R3 = R0 + R0 (S);
	DBGA ( R3.L , 0xffff );
	DBGA ( R3.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );

// saturate negative
	R0.L = 0;
	R0.H = 0x8000;
	R7 = 0;
	ASTAT = R7;
	R3 = R0 + R0 (S);
	DBGA ( R3.L , 0x0000 );
	DBGA ( R3.H , 0x8000 );

	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// saturate positive with subtraction
	R0.L = 0xffff;
	R0.H = 0xffff;
	R1.L = 0xffff;
	R1.H = 0x7fff;
	R7 = 0;
	ASTAT = R7;
	R3 = R1 - R0 (S);
	DBGA ( R3.L , 0xffff );
	DBGA ( R3.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );

// saturate negative with subtraction
	R0.L = 0x1;
	R0.H = 0x0;
	R1.L = 0x0000;
	R1.H = 0x8000;
	R7 = 0;
	ASTAT = R7;
	R3 = R1 - R0 (S);
	DBGA ( R3.L , 0x0000 );
	DBGA ( R3.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

	pass
