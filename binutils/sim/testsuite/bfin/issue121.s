# mach: bfin

.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;
	R0.L = 32767;
	R0.H = 32767;
	R1.L = -32768;
	R1.H = -32768;
	R0.L = R0 + R1 (RND12);

	_DBG R0;
	_DBG ASTAT;
//R1 = ASTAT;
//_DBG R1;

//DBGA ( R1.H , 0x0 );
//DBGA ( R1.L , 0x0001 );
	cc = az;
	r0 = cc;
	dbga( r0.l, 1);
	cc = an;
	r0 = cc;
	dbga( r0.l, 0);
	cc = av0;
	r0 = cc;
	dbga( r0.l, 0);
	cc = av0s;
	r0 = cc;
	dbga( r0.l, 0);
	cc = av1;
	r0 = cc;
	dbga( r0.l, 0);
	cc = av1s;
	r0 = cc;
	dbga( r0.l, 0);

	pass
