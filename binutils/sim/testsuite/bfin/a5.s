//  ALU test program.
//  Test instructions
//   rL4= L+L (r2,r3);
//   rH4= L+H (r2,r3) S;
//   rL4= L-L (r2,r3);
//   rH4= L-H (r2,r3) S;
# mach: bfin

.include "testutils.inc"
	start

	init_r_regs 0;
	ASTAT = R0;

// overflow  positive
	R0.L = 0x0000;
	R0.H = 0x7fff;
	R1.L = 0x7fff;
	R1.H = 0x0000;
	R7 = 0;
	ASTAT = R7;
	R3.L = R0.H + R1.L (NS);
	DBGA ( R3.L , 0xfffe );
	DBGA ( R3.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );

// overflow  negative
	R0.L = 0xffff;
	R0.H = 0x0000;
	R1.L = 0x0000;
	R1.H = 0x8000;
	R3 = 0;
	R7 = 0;
	ASTAT = R7;
	R3.H = R0.L + R1.H (NS);
	DBGA ( R3.L , 0x0000 );
	DBGA ( R3.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// saturate  positive
	R0.L = 0x0000;
	R0.H = 0x7fff;
	R1.L = 0x7fff;
	R1.H = 0x0000;
	R3 = 0;
	R7 = 0;
	ASTAT = R7;
	R3.L = R0.H + R1.L (S);
	DBGA ( R3.L , 0x7fff );
	DBGA ( R3.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );

// saturate  negative
	R0.L = 0xffff;
	R0.H = 0x0000;
	R1.L = 0x0000;
	R1.H = 0x8000;
	R3 = 0;
	R7 = 0;
	ASTAT = R7;
	R3.L = R0.L + R1.H (S);
	DBGA ( R3.L , 0x8000 );
	DBGA ( R3.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// overflow  positive with subtraction
	R0.L = 0x0000;
	R0.H = 0x7fff;
	R1.L = 0xffff;
	R1.H = 0x0000;
	R7 = 0;
	ASTAT = R7;
	R3.L = R0.H - R1.L (NS);
	DBGA ( R3.L , 0x8000 );
	DBGA ( R3.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );

// overflow  negative with subtraction
	R0.L = 0x8000;
	R0.H = 0x0000;
	R1.L = 0x0000;
	R1.H = 0x0001;
	R3 = 0;
	R7 = 0;
	ASTAT = R7;
	R3.H = R0.L - R1.H (NS);
	DBGA ( R3.L , 0x0000 );
	DBGA ( R3.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

// saturate  positive with subtraction
	R0.L = 0x0000;
	R0.H = 0x7fff;
	R1.L = 0xffff;
	R1.H = 0x0000;
	R7 = 0;
	ASTAT = R7;
	R3.H = R0.H - R1.L (S);
	DBGA ( R3.L , 0x0000 );
	DBGA ( R3.H , 0x7fff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );

// saturate  negative with subtraction
	R0.L = 0x8000;
	R0.H = 0x0000;
	R1.L = 0x0000;
	R1.H = 0x0001;
	R3 = 0;
	R7 = 0;
	ASTAT = R7;
	R3.H = R0.L - R1.H (S);
	DBGA ( R3.L , 0x0000 );
	DBGA ( R3.H , 0x8000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC =  V;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x1 );

	pass
