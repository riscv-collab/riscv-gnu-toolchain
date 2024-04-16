# mach: bfin

//  Shift test program.
//  Test instructions
//  RL0 = CC = BXOR (A0 AND R1) << 1;
//  RL0 = CC = BXOR  A0 AND R1;
//  A0 <<=1 (BXOR A0 AND A1 CC);
//  RL3 = CC = BXOR A0 AND A1 CC;

.include "testutils.inc"
	start

	init_r_regs 0;
	ASTAT = R0;

//  RL0 = CC = BXOR (A0 AND R1) << 1;
	R0.L = 0x1000;
	R0.H = 0x0000;
	A0.w = R0;
	R0.L = 0x0000;
	A0.x = R0.L;
	R1.L = 0xffff;
	R1.H = 0xffff;
	R2.L = CC = BXORSHIFT( A0 , R1 );
	R0 = A0.w;
	DBGA ( R0.L , 0x2000 );
	DBGA ( R0.H , 0x0000 );
	R0.L = A0.x;
	DBGA ( R0.L , 0x0000 );
	R0 = CC;
	DBGA ( R0.L , 0x0001 );
	DBGA ( R0.H , 0x0000 );
	DBGA ( R2.L , 0x0001 );

	R0.L = 0x1000;
	R0.H = 0x0001;
	A0.w = R0;
	R0.L = 0x0000;
	A0.x = R0.L;
	R1.L = 0xffff;
	R1.H = 0xffff;
	R2.L = CC = BXORSHIFT( A0 , R1 );
	R0 = A0.w;
	DBGA ( R0.L , 0x2000 );
	DBGA ( R0.H , 0x0002 );
	R0.L = A0.x;
	DBGA ( R0.L , 0x0000 );
	R0 = CC;
	DBGA ( R0.L , 0x0000 );
	DBGA ( R0.H , 0x0000 );
	DBGA ( R2.L , 0x0000 );

	R0.L = 0xffff;
	R0.H = 0xffff;
	A0.w = R0;
	R0.L = 0x00ff;
	A0.x = R0.L;
	R1.L = 0xffff;
	R1.H = 0xffff;
	R2.L = CC = BXORSHIFT( A0 , R1 );
	R0 = A0.w;
	DBGA ( R0.L , 0xfffe );
	DBGA ( R0.H , 0xffff );
	R0.L = A0.x;
	DBGA ( R0.L , 0xffff );
	R0 = CC;
	DBGA ( R0.L , 0x0001 );
	DBGA ( R0.H , 0x0000 );
	DBGA ( R2.L , 0x0001 );

// no
	R0.L = 0xffff;
	R0.H = 0xffff;
	A0.w = R0;
	R0.L = 0x00ff;
	A0.x = R0.L;
	R1.L = 0xffff;
	R1.H = 0xffff;
	R2.L = CC = BXOR( A0 , R1 );
	R0 = A0.w;
	DBGA ( R0.L , 0xffff );
	DBGA ( R0.H , 0xffff );
	R0.L = A0.x;
	DBGA ( R0.L , 0xffff );
	R0 = CC;
	DBGA ( R0.L , 0x0000 );
	DBGA ( R0.H , 0x0000 );
	DBGA ( R2.H , 0x0000 );

//  A0 <<=1 (BXOR A0 AND A1 CC);

	R0.L = 0x1000;
	R0.H = 0x0000;
	A0.w = R0;
	R0.L = 0x0000;
	A0.x = R0.L;
	R0.L = 0xffff;
	R0.H = 0xffff;
	A1.w = R0;
	R0.L = 0x00ff;
	A1.x = R0.L;
	R0.L = 0x0000;
	R0.H = 0x0000;
	CC = R0;
	A0 = BXORSHIFT( A0 , A1, CC );
	R0 = A0.w;
	DBGA ( R0.L , 0x2001 );
	DBGA ( R0.H , 0x0000 );
	R0.L = A0.x;
	DBGA ( R0.L , 0x0000 );

	R0.L = 0x1000;
	R0.H = 0x0000;
	A0.w = R0;
	R0.L = 0x0000;
	A0.x = R0.L;
	R0.L = 0x0fff;
	R0.H = 0xffff;
	A1.w = R0;
	R0.L = 0x00ff;
	A1.x = R0.L;
	R0.L = 0x0000;
	R0.H = 0x0000;
	CC = R0;
	A0 = BXORSHIFT( A0 , A1, CC );
	R0 = A0.w;
	DBGA ( R0.L , 0x2000 );
	DBGA ( R0.H , 0x0000 );
	R0.L = A0.x;
	DBGA ( R0.L , 0x0000 );

	R0.L = 0x1000;
	R0.H = 0x0000;
	A0.w = R0;
	R0.L = 0x0000;
	A0.x = R0.L;
	R0.L = 0xffff;
	R0.H = 0xffff;
	A1.w = R0;
	R0.L = 0x00ff;
	A1.x = R0.L;
	R0.L = 0x0001;
	R0.H = 0x0000;
	CC = R0;
	A0 = BXORSHIFT( A0 , A1, CC );
	R0 = A0.w;
	DBGA ( R0.L , 0x2000 );
	DBGA ( R0.H , 0x0000 );
	R0.L = A0.x;
	DBGA ( R0.L , 0x0000 );

// no

	R0.L = 0x1000;
	R0.H = 0x0000;
	A0.w = R0;
	R0.L = 0x0000;
	A0.x = R0.L;
	R0.L = 0xffff;
	R0.H = 0xffff;
	A1.w = R0;
	R0.L = 0x00ff;
	A1.x = R0.L;
	R0.L = 0x0000;
	R0.H = 0x0000;
	CC = R0;
	R2.L = CC = BXOR( A0 , A1, CC );
	R0 = A0.w;
	DBGA ( R0.L , 0x1000 );
	DBGA ( R0.H , 0x0000 );
	R0.L = A0.x;
	DBGA ( R0.L , 0x0000 );
	DBGA ( R2.L , 0x0001 );
	R0 = CC;
	DBGA ( R0.L , 0x0001 );

	pass
