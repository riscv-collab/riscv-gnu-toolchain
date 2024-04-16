//  ALU test program.
//  Test dual 16 bit MAX, MIN, ABS instructions
# mach: bfin

.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;
// MAX
// first operand is larger, so AN=0
	R0.L = 0x0001;
	R0.H = 0x0002;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7 = MAX ( R0 , R1 ) (V);
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0002 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// second operand is larger
	R0.L = 0x0000;
	R0.H = 0x0000;
	R1.L = 0x0001;
	R1.H = 0x0022;
	R7 = MAX ( R0 , R1 ) (V);
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0022 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// one operand larger, one smaller.
	R0.L = 0x000a;
	R0.H = 0x0000;
	R1.L = 0x0001;
	R1.H = 0x0022;
	R7 = MAX ( R0 , R1 ) (V);
	DBGA ( R7.L , 0x000a );
	DBGA ( R7.H , 0x0022 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0x8001;
	R0.H = 0xffff;
	R1.L = 0x8000;
	R1.H = 0x0022;
	R7 = MAX ( R0 , R1 ) (V);
	DBGA ( R7.L , 0x8001 );
	DBGA ( R7.H , 0x0022 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0x8000;
	R0.H = 0xffff;
	R1.L = 0x8000;
	R1.H = 0x0022;
	R7 = MAX ( R0 , R1 ) (V);
	DBGA ( R7.L , 0x8000 );
	DBGA ( R7.H , 0x0022 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// MIN
// second operand is smaller
	R0.L = 0x0001;
	R0.H = 0x0004;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7 = MIN ( R0 , R1 ) (V);
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// first operand is smaller
	R0.L = 0xffff;
	R0.H = 0x8001;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7 = MIN ( R0 , R1 ) (V);
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x8001 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// one of each
	R0.L = 0xffff;
	R0.H = 0x0034;
	R1.L = 0x0999;
	R1.H = 0x0010;
	R7 = MIN ( R0 , R1 ) (V);
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x0010 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0xffff;
	R0.H = 0x0010;
	R1.L = 0x0999;
	R1.H = 0x0010;
	R7 = MIN ( R0 , R1 ) (V);
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x0010 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// ABS
	R0.L = 0x0001;
	R0.H = 0x8001;
	R7 = ABS R0 (V);
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x7fff );
	_DBG ASTAT;
	R6 = ASTAT;
	_DBG R6;

	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  VS;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0x0001;
	R0.H = 0x8000;
	R7 = ABS R0 (V);
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  VS;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0x0000;
	R0.H = 0xffff;
	R7 = ABS R0 (V);
	_DBG R7;
	_DBG ASTAT;
	R6 = ASTAT;
	_DBG R6;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0001 );
	CC =  VS;	R6 = CC; DBGA ( R6.L, 0x1 );
	CC =  AZ;	R6 = CC; DBGA ( R6.L, 0x1 );

	pass
