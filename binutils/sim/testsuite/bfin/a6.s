//  ALU test program.
//  Test instructions
//  r7 = +/+ (r0,r1);
//  r7 = +/+ (r0,r1) s;
//  r7 = +/+ (r0,r1) sx;
# mach: bfin

.include "testutils.inc"
	start


// one result overflows positive
	R0.L = 0x0001;
	R0.H = 0x0010;
	R1.L = 0x7fff;
	R1.H = 0x0010;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 +|+ R1;
	DBGA ( R7.L , 0x8000 );
	DBGA ( R7.H , 0x0020 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );

// one result overflows negative
	R0.L = 0xffff;
	R0.H = 0x0010;
	R1.L = 0x8000;
	R1.H = 0x0010;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 +|+ R1;
	DBGA ( R7.L , 0x7fff );
	DBGA ( R7.H , 0x0020 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// one result zero
	R0.L = 0x0001;
	R0.H = 0xffff;
	R1.L = 0x0001;
	R1.H = 0x0001;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 +|+ R1;
	DBGA ( R7.L , 0x0002 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R5 = CC; DBGA ( R5.L , 0x1 );
	CC = AN;	R5 = CC; DBGA ( R5.L , 0x0 );
	CC =  V;	R5 = CC; DBGA ( R5.L , 0x0 );
	CC = AC0;	R5 = CC; DBGA ( R5.L , 0x0 );

// one result saturates positive
	R0.L = 0x0001;
	R0.H = 0x0010;
	R1.L = 0x7fff;
	R1.H = 0x0010;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 +|+ R1 (S);
	DBGA ( R7.L , 0x7fff );
	DBGA ( R7.H , 0x0020 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );

// one result saturates negative
	R0.L = 0xffff;
	R0.H = 0x0010;
	R1.L = 0x8000;
	R1.H = 0x0010;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 +|+ R1 (S);
	DBGA ( R7.L , 0x8000 );
	DBGA ( R7.H , 0x0020 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// two results saturates negative
	R0.L = 0xffff;
	R0.H = 0xfff0;
	R1.L = 0x8000;
	R1.H = 0x8000;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 +|+ R1 (S);
	DBGA ( R7.L , 0x8000 );
	DBGA ( R7.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// one result overflows positive and cross
	R0.L = 0x0001;
	R0.H = 0x0010;
	R1.L = 0x7fff;
	R1.H = 0x0010;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 +|+ R1 (CO);
	DBGA ( R7.L , 0x0020 );
	DBGA ( R7.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );

// one result saturates negative and cross
	R0.L = 0xffff;
	R0.H = 0x0010;
	R1.L = 0x8000;
	R1.H = 0x0010;
	R7 = 0;
	ASTAT = R7;
	R7 = R0 +|+ R1 (SCO);
	DBGA ( R7.L , 0x0020 );
	DBGA ( R7.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

	pass
