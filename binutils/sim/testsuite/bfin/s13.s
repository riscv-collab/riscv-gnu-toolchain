//  Test  rl3 = ashift (rh0 by r5;
//  Test  rl3 = lshift (rh0 by r5);
# mach: bfin

.include "testutils.inc"
	start

	init_r_regs 0;

	R0 = 0;
	ASTAT = R0;
	R0.L = 0x1;
	R0.H = 0x1;
	R5.L = 4;
	R7.L = ASHIFT R0.L BY R5.L;
	DBGA ( R7.L , 0x0010 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0 = 0;
	ASTAT = R0;
	R0.L = 0x8000;
	R0.H = 0x1;
	R5.L = -4;
	R5.H = 0;
	R7.L = ASHIFT R0.L BY R5.L;
	DBGA ( R7.L , 0xf800 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0 = 0;
	ASTAT = R0;
	R0.L = 0x0;
	R0.H = 0x1;
	R5.L = 0;
	R5.H = 0;
	R7.L = ASHIFT R0.L BY R5.L;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0 = 0;
	ASTAT = R0;
	R7 = 0;
	R0.L = 0x1;
	R0.H = 0x8000;
	R5.L = -4;
	R5.H = 0;
	R7.H = ASHIFT R0.H BY R5.L;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0xf800 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0 = 0;
	ASTAT = R0;
	R7 = 0;
	R0.L = 0x1;
	R0.H = 0x8000;
	R5.L = -4;
	R5.H = 0;
	R7.L = ASHIFT R0.H BY R5.L;
	DBGA ( R7.L , 0xf800 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0 = 0;
	ASTAT = R0;
	R7 = 0;
	R0.L = 0x1;
	R0.H = 0xffff;
	R5.L = 31;	// should accept   mag of +31
	R5.H = 0;
	R7.L = ASHIFT R0.H BY R5.L;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0 = 0;
	ASTAT = R0;
	R7 = 0;
	R0.L = 0x1;
	R0.H = 0x0100;
	R5.L = 63;	//  mag of 63 will appear as -1 since 6 bits are masked
	R5.H = 0;
	R7.L = ASHIFT R0.H BY R5.L;
	DBGA ( R7.L , 0x0080 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// logic shifts
	R0 = 0;
	ASTAT = R0;
	R7 = 0;
	R0.L = 0x1;
	R0.H = 0x8000;
	R5.L = -4;
	R5.H = 0;
	R7.L = LSHIFT R0.H BY R5.L;
	DBGA ( R7.L , 0x0800 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0 = 0;
	ASTAT = R0;
	R7 = 0;
	R0.L = 0x1;
	R0.H = 0x1;
	R5.L = 4;
	R5.H = 0;
	R7.H = LSHIFT R0.L BY R5.L;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0010 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0 = 0;
	ASTAT = R0;
	R7 = 1;
	R0.L = 0x0;
	R0.H = 0x0;
	R5.L = 0;
	R5.H = 0;
	R7.L = LSHIFT R0.L BY R5.L;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0 = 0;
	ASTAT = R0;
	R7 = 1;
	R0.L = 0x1;
	R0.H = 0x0;
	R5.L = 15;
	R5.H = 0;
	R7.L = LSHIFT R0.L BY R5.L;
	DBGA ( R7.L , 0x8000 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0 = 0;
	ASTAT = R0;
	R7 = 1;
	R0.L = 0x0100;
	R0.H = 0x0;
	R5.L = 63;	//  mag of 63 will appear as -1 since 6 bits are masked
	R5.H = 0;
	R7.L = LSHIFT R0.L BY R5.L;
	DBGA ( R7.L , 0x0080 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0 = 0;
	ASTAT = R0;
	R7 = 1;
	R0.L = 0x0100;
	R0.H = 0x0;
	R5.L = 31;	// should accept   mag of +31
	R5.H = 0;
	R7.L = LSHIFT R0.L BY R5.L;
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	pass
