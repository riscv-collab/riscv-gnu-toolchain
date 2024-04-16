//  Test  rl3 = ashift (rh0 by 7);
//  Test  rl3 = lshift (rh0 by 7);
# mach: bfin

.include "testutils.inc"
	start

	init_r_regs 0;

	R0 = 0;
	ASTAT = R0;
	R0.L = 0x1;
	R0.H = 0x1;
	R7.L = R0.L << 4;
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
	R7.L = R0.L >>> 4;
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
	R7.L = R0.L << 0;
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
	R7.H = R0.H >>> 4;
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
	R7.L = R0.H >>> 4;
	DBGA ( R7.L , 0xf800 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

// logic shifts
	R0 = 0;
	ASTAT = R0;
	R7 = 0;
	R0.L = 0x1;
	R0.H = 0x8000;
	R7.L = R0.H >> 4;
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
	R7.H = R0.L << 4;
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
	R7.L = R0.L << 0;
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
	R7.L = R0.L << 15;
	DBGA ( R7.L , 0x8000 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AC0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV0;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AV1;	R7 = CC; DBGA ( R7.L , 0x0 );

	pass
