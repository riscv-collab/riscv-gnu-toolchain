//  test RND setting AZ
# mach: bfin

.include "testutils.inc"
	start


//  result is zero with overflow ==> AZ, therefore, is not set
	R0.L = 0x8000;
	R0 = R0.L (X);
	R1.L = R0 (RND);
	CC = AZ;	R7 = CC;
	DBGA(R1.L, 0);
	DBGA ( R7.L , 0x1 );

// No Overflow, result is zero, AZ is set
	R0 = 1 (X);
	R1.L = r0 (RND);
	CC = AZ;	R7 = CC;
	DBGA(R1.L, 0);
	DBGA ( R7.L , 0x1 );

// result should be 1
	R0.L = 0x8000;
	R0.H = 0;
	R1.L = R0 (RND);
	CC = AZ;	R7 = CC;
	DBGA(R1.L, 1);
	DBGA ( R7.L , 0x0 );

// Result should be non-zero
	R0.H = 0x7ff0;
	R0.L = 0x8000;
	R1.L = R0 (RND);
	CC = AZ;	R7 = CC;
	DBGA(R1.L, 0x7ff1);
	DBGA ( R7.L , 0x0 );

	pass
