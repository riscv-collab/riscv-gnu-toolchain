//  reg-based SHIFT test program.
# mach: bfin

.include "testutils.inc"
	start


//  Test FDEP with no sign extension

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x0c08;	// pos=12 len=8
	R1.H = 0x00ff;
	R7 = DEPOSIT( R0, R1 );
	DBGA ( R7.L , 0xfead );
	DBGA ( R7.H , 0x123f );

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x0c04;	// pos=12 len=4
	R1.H = 0x00ff;
	R7 = DEPOSIT( R0, R1 );
	DBGA ( R7.L , 0xfead );
	DBGA ( R7.H , 0x1234 );

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x0c05;	// pos=12 len=5
	R1.H = 0x00ff;
	R7 = DEPOSIT( R0, R1 );
	DBGA ( R7.L , 0xfead );
	DBGA ( R7.H , 0x1235 );

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x0010;	// pos=0 len=16
	R1.H = 0xffff;
	R7 = DEPOSIT( R0, R1 );
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x1234 );

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x0011;	// pos=0 len=17
	R1.H = 0xffff;
	R7 = DEPOSIT( R0, R1 );
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x1234 );

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x0114;	// pos=1 len=20
	R1.H = 0xffff;
	R7 = DEPOSIT( R0, R1 );
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x1235 );

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x001f;	// pos=0 len=31
	R1.H = 0xffff;
	R7 = DEPOSIT( R0, R1 );
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x1234 );

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x1c04;	// pos=28 len=4
	R1.H = 0xffff;
	R7 = DEPOSIT( R0, R1 );
	DBGA ( R7.L , 0xdead );
	DBGA ( R7.H , 0xf234 );

	R0.L = 0xdead;
	R0.H = 0x0234;
	R1.L = 0x1d04;	// pos=29 len=4
	R1.H = 0xffff;
	R7 = DEPOSIT( R0, R1 );
	DBGA ( R7.L , 0xdead );
	DBGA ( R7.H , 0xe234 );

	R0.L = 0xdead;
	R0.H = 0x0234;
	R1.L = 0x1f04;	// pos=31 len=4
	R1.H = 0xffff;
	R7 = DEPOSIT( R0, R1 );
	DBGA ( R7.L , 0xdead );
	DBGA ( R7.H , 0x8234 );

	R0.L = 0xdead;
	R0.H = 0x0234;
	R1.L = 0x2004;	// pos=32 len=4, same as pos=0 len=4
	R1.H = 0xffff;
	R7 = DEPOSIT( R0, R1 );
	DBGA ( R7.L , 0xdeaf );
	DBGA ( R7.H , 0x0234 );

//  Test FDEP with sign extension

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x0c08;	// pos=12 len=8
	R1.H = 0x00ff;
	R7 = DEPOSIT( R0, R1 ) (X);
	DBGA ( R7.L , 0xfead );
	DBGA ( R7.H , 0xffff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x0c08;	// pos=12 len=8
	R1.H = 0x007f;
	R7 = DEPOSIT( R0, R1 ) (X);
	DBGA ( R7.L , 0xfead );
	DBGA ( R7.H , 0x0007 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0xdea0;
	R0.H = 0x1234;
	R1.L = 0x0110;	// pos=1 len=16
	R1.H = 0xffff;
	R7 = DEPOSIT( R0, R1 ) (X);
	DBGA ( R7.L , 0xfffe );
	DBGA ( R7.H , 0xffff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );

	R0.L = 0xdea0;
	R0.H = 0x1234;
	R1.L = 0x0101;	// pos=1 len=1
	R1.H = 0xffff;
	R7 = DEPOSIT( R0, R1 ) (X);
	DBGA ( R7.L , 0xfffe );
	DBGA ( R7.H , 0xffff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );

	R0.L = 0xdea0;
	R0.H = 0x1234;
	R1.L = 0x0102;	// pos=1 len=2
	R1.H = 0x0001;
	R7 = DEPOSIT( R0, R1 ) (X);
	DBGA ( R7.L , 0x0002 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0xdea0;
	R0.H = 0x1234;
	R1.L = 0x0002;	// pos=0 len=2
	R1.H = 0x0001;
	R7 = DEPOSIT( R0, R1 ) (X);
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0xdea0;
	R0.H = 0x1234;
	R1.L = 0x0000;	// pos=0 len=0
	R1.H = 0x000f;
	R7 = DEPOSIT( R0, R1 ) (X);
	DBGA ( R7.L , 0x0000 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );

	pass
