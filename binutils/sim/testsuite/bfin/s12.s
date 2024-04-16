//  Shifter test program.
//  Test instructions
//  RL5 = EXPADJ R4 BY RL2;
# mach: bfin

.include "testutils.inc"
	start


	R0.L = 30;	// large norm of small value
	R0.H = 1;	// make sure high half is not used
	R1.L = 0x0000;
	R1.H = 0x1000;	// small norm (2) of large value
	R7.L = EXPADJ( R1 , R0.L );
	DBGA ( R7.L , 0x0002 );

	R0.L = 3;	// small norm of large value
	R0.H = 1;	// make sure high half is not used
	R1.L = 0x0000;
	R1.H = 0xff00;	// small norm (2) of large value
	R7.L = EXPADJ( R1 , R0.L );
	DBGA ( R7.L , 0x0003 );

	R0.L = 3;
	R0.H = 1;
	R1.L = 0xffff;
	R1.H = 0xffff;
	R7.L = EXPADJ( R1 , R0.L );
	DBGA ( R7.L , 0x0003 );

	R0.L = 31;
	R0.H = 1;
	R1.L = 0x0000;	// norm=0
	R1.H = 0x8000;
	R7.L = EXPADJ( R1 , R0.L );
	DBGA ( R7.L , 0x0000 );

// RL5 = EXPADJ/EXPADJ R4 BY RL2;

	R0.L = 15;
	R1.L = 0x0800;
	R1.H = 0x1000;
	R7.L = EXPADJ( R1 , R0.L ) (V);
	DBGA ( R7.L , 0x0002 );

	R0.L = 15;
	R1.L = 0x1000;
	R1.H = 0x0800;
	R7.L = EXPADJ( R1 , R0.L ) (V);
	DBGA ( R7.L , 0x0002 );

	R0.L = 1;
	R1.L = 0x0800;
	R1.H = 0x1000;
	R7.L = EXPADJ( R1 , R0.L ) (V);
	DBGA ( R7.L , 0x0001 );

	R0.L = 14;
	R1.L = 0xff00;
	R1.H = 0xfff0;
	R7.L = EXPADJ( R1 , R0.L ) (V);
	DBGA ( R7.L , 0x0007 );

// RL5 = EXPADJ RL4 BY RL2;

	R0.L = 14;
	R1.L = 0xff00;
	R1.H = 0x1000;
	R7.L = EXPADJ( R1.L , R0.L );
	DBGA ( R7.L , 0x0007 );

	R0.L = 3;
	R1.L = 0xff00;
	R1.H = 0x1000;
	R7.L = EXPADJ( R1.L , R0.L );
	DBGA ( R7.L , 0x0003 );

	R0.L = 14;
	R1.L = 0x1000;
	R1.H = 0xff00;
	R7.L = EXPADJ( R1.H , R0.L );
	DBGA ( R7.L , 0x0007 );

	pass
