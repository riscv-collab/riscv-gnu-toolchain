# mach: bfin

.include "testutils.inc"
	start


	R0.H = -32768;
	R0.L = 0;
	R0 >>= 0x1;

	_DBG R0;
	R7 = ASTAT;
	_DBG R7;

//DBGA ( R7.H , 0x0000 );
//DBGA ( R7.L , 0x0000 );
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
	dbga( r0.l, 0);
	cc = av1;
	r0 = cc;
	dbga( r0.l, 0);
	cc = av1s;
	r0 = cc;
	dbga( r0.l, 0);

	R0.H = 0;
	R0.L = 1;
	R0 <<= 0x1f;

	_DBG R0;
	R7 = ASTAT;
	_DBG R7;
//DBGA ( R7.H , 0x0000 );
//DBGA ( R7.L , 0x0002 );
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

	R1.L = -1;
	R1.H = 32767;
	R0 = 31;
	R1 >>= R0;

	_DBG R1;
	R7 = ASTAT;
	_DBG R7;
//DBGA ( R7.H , 0x0000 );
//DBGA ( R7.L , 0x0001 );
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
