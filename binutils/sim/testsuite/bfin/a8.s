# mach: bfin

.include "testutils.inc"
	start

// xh, h, xb, b
	R0.L = 32898;	R0.H = 1;
	R1.L = 49346;	R1.H = 3;
	R2.L = 6;	R2.H = -1;
	R3.L = 129;	R3.H = 7;
	R4.L = 4;	R4.H = 0;
	R5.L = 5;	R5.H = 0;
	R6.L = 6;	R6.H = 0;
	R7.L = 7;	R7.H = 0;
	R4 = R0.L (X);

//	_DBG ASTAT;	R7 = ASTAT;DBGA ( R7.L , 2 );
	cc = az;
	r7 = cc;
	dbga( r7.l, 0);
	cc = an;
	r7 = cc;
	dbga( r7.l, 1);
	cc = av0;
	r7 = cc;
	dbga( r7.l, 0);
	cc = av0s;
	r7 = cc;
	dbga( r7.l, 0);
	cc = av1;
	r7 = cc;
	dbga( r7.l, 0);
	cc = av1s;
	r7 = cc;
	dbga( r7.l, 0);

	R5 = R0.L;
	R6 = R1.B (X);
	R7 = R1.B;
	DBGA ( R4.l , 32898 );	DBGA ( R4.h , 0xffff);
	pass
