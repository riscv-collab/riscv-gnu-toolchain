//  ALU test program.
//  Test 32 bit MAX, MIN, ABS instructions
# mach: bfin

.include "testutils.inc"
	start


// MAX
// first operand is larger, so AN=0
	R0.L = 0x0001;
	R0.H = 0x0000;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7 = MAX ( R0 , R1 );
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// second operand is larger, so AN=1
	R0.L = 0x0000;
	R0.H = 0x0000;
	R1.L = 0x0001;
	R1.H = 0x0000;
	R7 = MAX ( R0 , R1 );
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// first operand is larger, check correct output with overflow
	R0.L = 0xffff;
	R0.H = 0x7fff;
	R1.L = 0xffff;
	R1.H = 0xffff;
	R7 = MAX ( R0 , R1 );
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// second operand is larger, no overflow here
	R0.L = 0xffff;
	R0.H = 0xffff;
	R1.L = 0xffff;
	R1.H = 0x7fff;
	R7 = MAX ( R0 , R1 );
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// second operand is larger, overflow
	R0.L = 0xffff;
	R0.H = 0x800f;
	R1.L = 0xffff;
	R1.H = 0x7fff;
	R7 = MAX ( R0 , R1 );
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0S;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1S;	R7 = CC; DBGA ( R7.L , 0x0 );

// both operands equal
	R0.L = 0x0080;
	R0.H = 0x8000;
	R1.L = 0x0080;
	R1.H = 0x8000;
	R7 = MAX ( R0 , R1 );
	DBGA ( R7.L , 0x0080 );
	DBGA ( R7.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// MIN
// second operand is smaller
	R0.L = 0x0001;
	R0.H = 0x0000;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7 = MIN ( R0 , R1 );
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// first operand is smaller
	R0.L = 0x0001;
	R0.H = 0x8000;
	R1.L = 0x0000;
	R1.H = 0x0000;
	R7 = MIN ( R0 , R1 );
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// first operand is smaller, overflow
	R0.L = 0x0001;
	R0.H = 0x8000;
	R1.L = 0x0000;
	R1.H = 0x0ff0;
	R7 = MIN ( R0 , R1 );
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// equal operands
	R0.L = 0x0001;
	R0.H = 0x8000;
	R1.L = 0x0001;
	R1.H = 0x8000;
	R7 = MIN ( R0 , R1 );
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// ABS
	R0.L = 0x0001;
	R0.H = 0x8000;
	R7 = ABS R0;
	_DBG R7;
	_DBG ASTAT;
	R6 = ASTAT;

	_DBG R6;
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x7fff );
//CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
//CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
//CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
//CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
//CC =  VS;	R7 = CC; DBGA ( R7.L , 0x1 );
//CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0x0001;
	R0.H = 0x0000;
	R7 = ABS R0;
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0x0000;
	R0.H = 0x8000;
	R7 = ABS R0;
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  VS;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0xffff;
	R0.H = 0xffff;
	R7 = ABS R0;
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0x0000;
	R0.H = 0x0000;
	R7 = ABS R0;
	_DBG R7;
	_DBG ASTAT;
	R6 = ASTAT;
	_DBG R6;

	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0000 );
	CC = VS;	R6 = CC; DBGA (R6.L, 0x1);
	CC = AZ;	R6 = CC; DBGA (R6.L, 0x1);

	pass
