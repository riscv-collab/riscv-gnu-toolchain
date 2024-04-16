//  Shifter test program.
//  Test instructions
//  RL0 = SIGNBITS R1;
//  RL0 = SIGNBITS RL1;
//  RL0 = SIGNBITS RH1;

# mach: bfin

.include "testutils.inc"
	start


// on 32-b word

	R1.L = 0xffff;
	R1.H = 0x7fff;
	R0.L = SIGNBITS R1;
	DBGA ( R0.L , 0x0000 );

	R1.L = 0xffff;
	R1.H = 0x30ff;
	R0.L = SIGNBITS R1;
	DBGA ( R0.L , 0x0001 );

	R1.L = 0xff0f;
	R1.H = 0x10ff;
	R0.L = SIGNBITS R1;
	DBGA ( R0.L , 0x0002 );

	R1.L = 0xff0f;
	R1.H = 0xe0ff;
	R0.L = SIGNBITS R1;
	DBGA ( R0.L , 0x0002 );

	R1.L = 0x0001;
	R1.H = 0x0000;
	R0.L = SIGNBITS R1;
	DBGA ( R0.L , 0x0001e );

	R1.L = 0xfffe;
	R1.H = 0xffff;
	R0.L = SIGNBITS R1;
	DBGA ( R0.L , 0x0001e );

	R1.L = 0xffff;	// return largest norm for -1
	R1.H = 0xffff;
	R0.L = SIGNBITS R1;
	DBGA ( R0.L , 0x0001f );

	R1.L = 0;	// return largest norm for zero
	R1.H = 0;
	R0.L = SIGNBITS R1;
	DBGA ( R0.L , 0x001f );

// on 16-b word

	R1.L = 0x7fff;
	R1.H = 0xffff;
	R0.L = SIGNBITS R1.L;
	DBGA ( R0.L , 0x0000 );

	R1.L = 0x0fff;
	R1.H = 0x0001;
	R0.L = SIGNBITS R1.H;
	DBGA ( R0.L , 0x000e );

	R1.L = 0x0fff;
	R1.H = 0xffff;
	R0.L = SIGNBITS R1.H;
	DBGA ( R0.L , 0x000f );

	R1.L = 0x0fff;
	R1.H = 0xfffe;
	R0.L = SIGNBITS R1.H;
	DBGA ( R0.L , 0x000e );

	pass
