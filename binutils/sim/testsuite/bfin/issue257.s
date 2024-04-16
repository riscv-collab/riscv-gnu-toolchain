# mach: bfin

.include "testutils.inc"
	start


	R0 = 0;
	R1 = 0;
	R2 = 0;
	R0.H = 0xfffe;
	R0.L = 0x9be8;
	R1.L = 0xeb53;
	R2.H = R0 - R1 (RND20);

	_DBG R2;
	_DBG ASTAT;
	DBGA ( R2.H , 0 );

	R0 = ASTAT;
//DBGA ( R0.L , 1 );
	cc = az;
	r0 = cc;
	dbga( r0.l, 1);
	cc = an;
	r0 = cc;
	dbga( r0.l, 0);

	pass
