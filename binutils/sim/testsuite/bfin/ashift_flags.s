# mach: bfin

.include "testutils.inc"
        start

// load r1=0x7fffffff
// load r2=0x80000000
// load r3=0x000000ff
// load r4=0x00000000
	loadsym p0, data0;
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

	_dbg r0;
	_dbg r1;
	_dbg r2;
	_dbg r3;
	_dbg r4;

	R7 = 0;
	ASTAT = R7;
	r5 = r1 << 0x4 (s);
	_DBG ASTAT;
	r7=astat;
	dbga (r5.h, 0x7fff);
	dbga (r5.l, 0xffff);
	dbga (r7.h, 0x0300);	// V=1, VS=1
	dbga (r7.l, 0x8);

	R7 = 0;
	ASTAT = R7;
	r5.h = r1.h << 0x4 (s);
	_DBG ASTAT;
	r7=astat;
	dbga (r5.h, 0x7fff);
	dbga (r7.h, 0x0300);	// V=1, VS=1
	dbga (r7.l, 0x8);

	A0 = 0;
	A0.w = r1;
	A0.x = r0.l;
	r6 = 0x3;
	_dbg r6;
	_dbg A0;
	R7 = 0;
	ASTAT = R7;
	A0 = ASHIFT A0 BY R6.L;
	_DBG ASTAT;
	_DBG A0;
	r7 = astat;
	dbga (r7.h, 0x0);	// AV0=0, AV0S=0
	dbga (r7.l, 0x2);	// AN = 1

	A1 = 0;
	A1 = r1;
	A1.x = r0.l;
	r6 = 0x3;
	_dbg A1;
	R7 = 0;
	ASTAT = R7;
	A1 = ASHIFT A1 BY R6.L;
	_DBG ASTAT;
	_DBG A1;
	r7 = astat;
	dbga (r7.h, 0x0);	// AV1=0, AV1S=0
	dbga (r7.l, 0x2);	// AN = 1

	pass

	.data 0x1000;
data0:
	.dw 0x1111
	.dw 0x1111
	.dw 0xffff
	.dw 0x7fff
	.dw 0x0000
	.dw 0x8000
	.dw 0x00ff
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
