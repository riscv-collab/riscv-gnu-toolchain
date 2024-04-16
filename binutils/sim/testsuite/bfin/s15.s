//  reg-based SHIFT test program.
# mach: bfin

.include "testutils.inc"
	start


//  Test FEXT with no sign extension

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x0810;	// pos=8 len=16
	R7 = EXTRACT( R0, R1.L ) (Z);
	DBGA ( R7.L , 0x34de );
	DBGA ( R7.H , 0 );

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x0814;	// pos=8 len=20
	R7 = EXTRACT( R0, R1.L ) (Z);
	DBGA ( R7.L , 0x34de );
	DBGA ( R7.H , 0x0002 );

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x0800;	// pos=8 len=0
	R7 = EXTRACT( R0, R1.L ) (Z);
	DBGA ( R7.L , 0 );
	DBGA ( R7.H , 0 );

	R0.L = 0xfff1;
	R0.H = 0xffff;
	R1.L = 0x0001;	// pos=0 len=1
	R7 = EXTRACT( R0, R1.L ) (Z);
	DBGA ( R7.L , 0x1 );
	DBGA ( R7.H , 0 );

	R0.L = 0xfff1;
	R0.H = 0xffff;
	R1.L = 0x0101;	// pos=1 len=1
	R7 = EXTRACT( R0, R1.L ) (Z);
	DBGA ( R7.L , 0 );
	DBGA ( R7.H , 0 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0xfff1;
	R0.H = 0xffff;
	R1.L = 0x1810;	// pos=24 len=16
	R7 = EXTRACT( R0, R1.L ) (Z);
	DBGA ( R7.L , 0x00ff );
	DBGA ( R7.H , 0 );

	R0.L = 0xfff1;
	R0.H = 0xffff;
	R1.L = 0x0020;	// pos=0 len=32 is like pos=0 len=0
	R7 = EXTRACT( R0, R1.L ) (Z);
	DBGA ( R7.L , 0x0 );
	DBGA ( R7.H , 0x0 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0xfff1;
	R0.H = 0xffff;
	R1.L = 0x2020;	// pos=32 len=32 is like pos=0 len=0
	R7 = EXTRACT( R0, R1.L ) (Z);
	DBGA ( R7.L , 0 );
	DBGA ( R7.H , 0 );

	R0.L = 0xfff1;
	R0.H = 0xffff;
	R1.L = 0x1f01;	// pos=31 len=1
	R7 = EXTRACT( R0, R1.L ) (Z);
	DBGA ( R7.L , 0x1 );
	DBGA ( R7.H , 0 );

	R0.L = 0xfff1;
	R0.H = 0xffff;
	R1.L = 0x1000;	// pos=16 len=0
	R7 = EXTRACT( R0, R1.L ) (Z);
	DBGA ( R7.L , 0 );
	DBGA ( R7.H , 0 );

//  Test FEXT with  sign extension

	R0.L = 0xdead;
	R0.H = 0x12f4;
	R1.L = 0x0810;	// pos=8 len=16
	R7 = EXTRACT( R0, R1.L ) (X);
	DBGA ( R7.L , 0xf4de );
	DBGA ( R7.H , 0xffff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );

	R0.L = 0xdead;
	R0.H = 0x1234;
	R1.L = 0x0810;	// pos=8 len=16
	R7 = EXTRACT( R0, R1.L ) (X);
	DBGA ( R7.L , 0x34de );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0xdead;
	R0.H = 0xf234;
	R1.L = 0x1f01;	// pos=31 len=1
	R7 = EXTRACT( R0, R1.L ) (X);
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0xffff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );

	R0.L = 0xdead;
	R0.H = 0xf234;
	R1.L = 0x1f02;	// pos=31 len=2
	R7 = EXTRACT( R0, R1.L ) (X);
	DBGA ( R7.L , 0x0001 );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0xffff;
	R0.H = 0xffff;
	R1.L = 0x101f;	// pos=16 len=31
	R7 = EXTRACT( R0, R1.L ) (X);
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0x0000 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );

	R0.L = 0xffff;
	R0.H = 0xffff;
	R1.L = 0x1001;	// pos=16 len=1
	R7 = EXTRACT( R0, R1.L ) (X);
	DBGA ( R7.L , 0xffff );
	DBGA ( R7.H , 0xffff );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x0 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x1 );

	R0.L = 0xffff;
	R0.H = 0xffff;
	R1.L = 0x1000;	// pos=16 len=0
	R7 = EXTRACT( R0, R1.L ) (X);
	DBGA ( R7.L , 0 );
	DBGA ( R7.H , 0 );
	CC = AZ;	R7 = CC; DBGA ( R7.L , 0x1 );
	CC = AN;	R7 = CC; DBGA ( R7.L , 0x0 );

	pass
