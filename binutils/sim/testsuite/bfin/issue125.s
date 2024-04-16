# mach: bfin

.include "testutils.inc"
	start

	A0 = 0;
	A1 = 0;
	R0 = -1;
	R1 = 0;
	R1.L = 0x007f;
	A0.w = R0;
	A0.x = R1;
	A1.w = R0;
	A1.x = R1;
	_DBG A0;
	_DBG A1;
	_DBG astat;
	A0 += A1;

	_DBG A0;
//	    _DBG ASTAT;
//	    R0 = ASTAT;
//	    _DBG R0;
//	    DBGA ( R0.L , 0x0 );
//	    DBGA ( R0.H , 0x3 );
	cc = az;
	r0 = cc;
	dbga( r0.l, 0);
	cc = an;
	r0 = cc;
	dbga( r0.l, 0);
	cc = av0;
	r0 = cc;
	dbga( r0.l, 1);
	cc = av0s;
	r0 = cc;
	dbga( r0.l, 1);
	cc = av1;
	r0 = cc;
	dbga( r0.l, 0);
	cc = av1s;
	r0 = cc;
	dbga( r0.l, 0);

	A1 = 0;
	_DBG A0;
	A0 += A1;

	_DBG A0;
//	    _DBG ASTAT;
//	    R0 = ASTAT;
//	    _DBG R0;

//	    DBGA ( R0.L , 0 );
//	    DBGA ( R0.H , 2 );
	cc = az;
	r0 = cc;
	dbga( r0.l, 0);
	cc = an;
	r0 = cc;
	dbga( r0.l, 0);
	cc = av0;
	r0 = cc;
	dbga( r0.l, 0);
	cc = av0s;
	r0 = cc;
	dbga( r0.l, 1);
	cc = av1;
	r0 = cc;
	dbga( r0.l, 0);
	cc = av1s;
	r0 = cc;
	dbga( r0.l, 0);

	pass
