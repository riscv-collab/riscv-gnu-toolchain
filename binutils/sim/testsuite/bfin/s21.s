//  Test  A0 = ROT    (A0 by imm6);
# mach: bfin

.include "testutils.inc"
	start

	init_r_regs 0;
	ASTAT = R0;
	A0 = A1 = 0;

// rot
// left  by 1
// 00 8000 0001 -> 01 0000 0002 cc=0
	R0.L = 0x0001;
	R0.H = 0x8000;
	R7 = 0;
	CC = R7;
	A1 = A0 = 0;
	A0.w = R0;
	A0 = ROT A0 BY 1;
	R1 = A0.w;
	DBGA ( R1.L , 0x0002 );
	DBGA ( R1.H , 0x0000 );
	R1.L = A0.x;
	DBGA ( R1.L , 0x0001 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
// left  by 1
// 80 0000 0001 -> 00 0000 0002 cc=1
	R7 = 0;
	CC = R7;
	R0.L = 0x0001;
	R0.H = 0x0000;
	R1.L = 0x0080;
	A1 = A0 = 0;
	A0.w = R0;
	A0.x = R1.L;
	A0 = ROT A0 BY 1;
	R1 = A0.w;
	DBGA ( R1.L , 0x0002 );
	DBGA ( R1.H , 0x0000 );
	R1.L = A0.x;
	DBGA ( R1.L , 0x0000 );
	R7 = CC;
	DBGA ( R7.L , 0x0001 );

// rot
// left  by 1 with cc=1
// 80 8000 0001 -> 01 0000 0003 cc=1
	R7 = 1;
	CC = R7;
	R0.L = 0x0001;
	R0.H = 0x8000;
	R1.L = 0x0080;
	A1 = A0 = 0;
	A0.w = R0;
	A0.x = R1.L;
	A0 = ROT A0 BY 1;
	R1 = A0.w;
	DBGA ( R1.L , 0x0003 );
	DBGA ( R1.H , 0x0000 );
	R1.L = A0.x;
	DBGA ( R1.L , 0x0001 );
	R7 = CC;
	DBGA ( R7.L , 0x0001 );

// rot
// left  by 2 with cc=1
// 80 0000 0001 -> 00 0000 0007 cc=0
	R7 = 1;
	CC = R7;
	R0.L = 0x0001;
	R0.H = 0x0000;
	R1.L = 0x0080;
	A1 = A0 = 0;
	A0.w = R0;
	A0.x = R1.L;
	A0 = ROT A0 BY 2;
	R1 = A0.w;
	DBGA ( R1.L , 0x0007 );
	DBGA ( R1.H , 0x0000 );
	R1.L = A0.x;
	DBGA ( R1.L , 0x0000 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
// left  by 3 with cc=0
	R7 = 0;
	CC = R7;
	R0.L = 0x0001;
	R0.H = 0x0000;
	R1.L = 0x0080;
	A1 = A0 = 0;
	A0.w = R0;
	A0.x = R1.L;
	A0 = ROT A0 BY 3;
	R1 = A0.w;
	DBGA ( R1.L , 0x000a );
	DBGA ( R1.H , 0x0000 );
	R1.L = A0.x;
	DBGA ( R1.L , 0x0000 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
// left by largest positive magnitude of 31
// 80 0000 0001 -> 00 a000 0000 cc=0
	R7 = 0;
	CC = R7;
	R0.L = 0x0001;
	R0.H = 0x0000;
	R1.L = 0x0080;
	A1 = A0 = 0;
	A0.w = R0;
	A0.x = R1.L;
	A0 = ROT A0 BY 31;
	R1 = A0.w;
	DBGA ( R1.L , 0x0000 );
	DBGA ( R1.H , 0xa000 );
	R1.L = A0.x;
	DBGA ( R1.L , 0x0000 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
// right  by 1
// 80 0000 0001 -> 40 0000 0000 cc=1
	R7 = 0;
	CC = R7;
	R0.L = 0x0001;
	R0.H = 0x0000;
	R1.L = 0x0080;
	A1 = A0 = 0;
	A0.w = R0;
	A0.x = R1.L;
	A0 = ROT A0 BY -1;
	R1 = A0.w;
	DBGA ( R1.L , 0x0000 );
	DBGA ( R1.H , 0x0000 );
	R1.L = A0.x;
	DBGA ( R1.L , 0x0040 );
	R7 = CC;
	DBGA ( R7.L , 0x0001 );

// rot
// right  by 1
// 80 0000 0001 -> c0 0000 0000 cc=1
	R7 = 1;
	CC = R7;
	R0.L = 0x0001;
	R0.H = 0x0000;
	R1.L = 0x0080;
	A1 = A0 = 0;
	A0.w = R0;
	A0.x = R1.L;
	A0 = ROT A0 BY -1;
	R1 = A0.w;
	DBGA ( R1.L , 0x0000 );
	DBGA ( R1.H , 0x0000 );
	R1.L = A0.x;
	DBGA ( R1.L , 0xffc0 );
	R7 = CC;
	DBGA ( R7.L , 0x0001 );

// rot
// right  by 2
// 80 0000 0001 -> e0 0000 0000 cc=0
	R7 = 1;
	CC = R7;
	R0.L = 0x0001;
	R0.H = 0x0000;
	R1.L = 0x0080;
	A1 = A0 = 0;
	A0.w = R0;
	A0.x = R1.L;
	A0 = ROT A0 BY -2;
	R1 = A0.w;
	DBGA ( R1.L , 0x0000 );
	DBGA ( R1.H , 0x0000 );
	R1.L = A0.x;
	DBGA ( R1.L , 0xffe0 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
// right  by 9
// 80 0000 0001 -> 01 c000 0000 cc=0
	R7 = 1;
	CC = R7;
	R0.L = 0x0001;
	R0.H = 0x0000;
	R1.L = 0x0080;
	A1 = A0 = 0;
	A0.w = R0;
	A0.x = R1.L;
	A0 = ROT A0 BY -9;
	R1 = A0.w;
	DBGA ( R1.L , 0x0000 );
	DBGA ( R1.H , 0xc000 );
	R1.L = A0.x;
	DBGA ( R1.L , 0x0001 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot
// right  by 9 with reg
// 80 0000 0001 -> 01 c000 0000 cc=0
	R7 = 1;
	CC = R7;
	R0.L = 0x0001;
	R0.H = 0x0000;
	R1.L = 0x0080;
	A1 = A0 = 0;
	A0.w = R0;
	A0.x = R1.L;
	R5 = -9;
	A0 = ROT A0 BY R5.L;
	R1 = A0.w;
	DBGA ( R1.L , 0x0000 );
	DBGA ( R1.H , 0xc000 );
	R1.L = A0.x;
	DBGA ( R1.L , 0x0001 );
	R7 = CC;
	DBGA ( R7.L , 0x0000 );

// rot left by 4 with cc=1
	R0.L = 0x789a;
	R0.H = 0x3456;
	A0.w = R0;
	R0.L = 0x12;
	A0.x = R0;

	R0 = 1;
	CC = R0;

	A0 = ROT A0 BY 4;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x4567 );	DBGA ( R4.L , 0x89a8 );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x0023 );

// rot left by 28 with cc=1
	R0.L = 0x789a;
	R0.H = 0x3456;
	A0.w = R0;
	R0.L = 0x12;
	A0.x = R0;

	R0 = 1;
	CC = R0;

	A0 = ROT A0 BY 28;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0xa891 );	DBGA ( R4.L , 0xa2b3 );
	DBGA ( R5.H , 0xffff );	DBGA ( R5.L , 0xff89 );

// rot right by 4 with cc=1
	R0.L = 0x789a;
	R0.H = 0x3456;
	A0.w = R0;
	R0.L = 0x12;
	A0.x = R0;

	R0 = 1;
	CC = R0;

	A0 = ROT A0 BY -4;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x2345 );	DBGA ( R4.L , 0x6789 );
	DBGA ( R5.H , 0x0000 );	DBGA ( R5.L , 0x0051 );

// rot right by 8 with cc=1
	R0.L = 0x789a;
	R0.H = 0x3456;
	A0.w = R0;
	R0.L = 0x12;
	A0.x = R0;

	R0 = 1;
	CC = R0;

	A0 = ROT A0 BY -28;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0xcf13 );	DBGA ( R4.L , 0x5123 );
	DBGA ( R5.H , 0xffff );	DBGA ( R5.L , 0xff8a );

	pass
