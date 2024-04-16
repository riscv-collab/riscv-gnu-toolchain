# mach: bfin

.include "testutils.inc"
	start


// issue 118

	R0 = 1;
	R1 = 0;
	A0.x = R1;
	A0.w = R0;

	A0 = - A0;

	_DBG A0;
	_DBG ASTAT;

//R0 = ASTAT;
//DBGA ( R0.L , 0x2 );

	cc = az;
	r0 = cc;
	dbga( r0.l, 0);
	cc = an;
	r0 = cc;
	dbga( r0.l, 1);
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
