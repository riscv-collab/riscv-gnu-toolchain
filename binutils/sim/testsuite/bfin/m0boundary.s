# mach: bfin

.include "testutils.inc"
	start

// setup a circular buffer calculation based on illegal register values
	I0 = 0xf2ef (Z);
	I0.H = 0xff88;

	L0 = 0xbd5f (Z);
	L0.H = 0xea9b;

	M0 = 0x0000 (Z);
	M0.H = 0x8000;

	B0 = 0x3fb9 (Z);
	B0.H = 0xff80;

op1:
	I0 -= M0;

	R0 = I0;
	DBGA ( R0.H , 0x7f88 );
	DBGA ( R0.L , 0xf2ef );

// setup a circular buffer calculation based on illegal register values
	I0 = 0xf2ef (Z);
	I0.H = 0xff88;

	L0 = 0xbd5f (Z);
	L0.H = 0xea9b;

	M0 = 0x0001 (Z);
	M0.H = 0x8000;

	B0 = 0x3fb9 (Z);
	B0.H = 0xff80;

op2:
	I0 -= M0;

	R0 = I0;
	DBGA ( R0.H , 0x7f88 );
	DBGA ( R0.L , 0xf2ee );

	pass
