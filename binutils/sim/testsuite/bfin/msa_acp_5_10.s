# mach: bfin

.include "testutils.inc"
	start


	r1.l = 0x0;
	r1.h = 0x8000;
	A0.w=r1;
	_dbg a1;
	a0 = a0 << 8;
	_dbg a0;
	_dbg astat;

	A0 = - A0;
	_dbg astat;
	_dbg a0;
	r7 = astat;

	cc = az;
	r7 = cc;
	dbga(r7.l, 0);
	cc = an;
	r7 = cc;
	dbga(r7.l, 0);
	cc = av0;
	r7 = cc;
	dbga(r7.l, 1);
	cc = av0s;
	r7 = cc;
	dbga(r7.l, 1);
	cc = av1;
	r7 = cc;
	dbga(r7.l, 0);
	cc = av1s;
	r7 = cc;
	dbga(r7.l, 0);

	r1.l = 0x0;
	r1.h = 0x8000;
	A1.w=r1;
	_dbg a0;
	a1 = a1 << 8;
	_dbg a1;
	_dbg astat;

	A1 = - A1;
	cc = az;
	r7 = cc;
	dbga(r7.l, 0);
	cc = an;
	r7 = cc;
	dbga(r7.l, 0);
	cc = av0;
	r7 = cc;
	dbga(r7.l, 1);
	cc = av0s;
	r7 = cc;
	dbga(r7.l, 1);
	cc = av1;
	r7 = cc;
	dbga(r7.l, 1);
	cc = av1s;
	r7 = cc;
	dbga(r7.l, 1);

	_dbg astat;
	_dbg a1;
	pass
