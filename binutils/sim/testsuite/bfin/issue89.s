# mach: bfin

.include "testutils.inc"
	start

	init_r_regs 0;
	ASTAT = R0;

	R2.L = 0x000f;
	R2.H = 0x038c;
	_DBG R2;

	R7.L = 0x007c;
	R7.H = 0x0718;
	A0 = 0;
	A0.w = R7;
	_DBG A0;

	A0 = ROT A0 BY R2.L;

	_DBG A0;

	R4 = A0.w;
	R5 = A0.x;
	DBGA ( R4.H , 0x003e );
	DBGA ( R4.L , 0x0001 );
	DBGA ( R5.H , 0xffff );
	DBGA ( R5.L , 0xff8c );

	pass
