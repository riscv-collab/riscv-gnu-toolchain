# mach: bfin

.include "testutils.inc"
	start

// load acc with values;
	R0.L = 0x5d8c;
	R0.H = 0x90c4;
	A0.w = R0;
	R0.L = 0x8308;
	A0.x = R0;
	R0.L = 0x32da;
	R0.H = 0xa6ec;
	A1.w = R0;
	R0.L = 0x1772;
	A1.x = R0;
// load regs with values;
	R0.L = 0x83de;
	R0.H = 0x7070;
	R1.L = 0x8b86;
	R1.H = 0x85ac;
	R2.L = 0x2398;
	R2.H = 0x3adc;
	R3.L = 0x1480;
	R3.H = 0x7f90;
// end load regs and acc;
	SAA ( R1:0 , R3:2 ) (R);

	_DBG A0;
	_DBG A1;

	R0 = A0.x;	DBGA ( R0.L , 0 );
	R0 = A1.x;	DBGA ( R0.L , 0 );

	pass
