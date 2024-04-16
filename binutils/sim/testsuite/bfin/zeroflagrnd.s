# mach: bfin

.include "testutils.inc"
	start

	init_r_regs 0;
	ASTAT=R0;

	R0.L = -32768;
	R0.H = -1;
	R0.L = R0 (RND);
	DBGA ( R0.L , 0 );

	_DBG R0;
//R0 = ASTAT;
//DBG R0;
//DBGA ( R0.L , 0x1 );
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
