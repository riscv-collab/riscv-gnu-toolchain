# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	r0 = 0;
	ASTAT = R0;

	r0 = -1;
	A0 = r0;
	A1 = 0;
	_dbg astat;
	r0 +=1;
	_dbg astat;
	A1 = A1(S), A0 = A0 (S);
	r7 = astat;
	checkreg r7, (_AC0|_AC0_COPY|_AN|_AZ);
	_dbg a0;
	_dbg astat;

	r0.l = 0x0;
	r0.h = 0x8000;
	r1 = 1;
	a0 = r0;
	a0 = a0 << 8;
	a1 = r1;
	r7 = astat;
	checkreg r7, (_AC0|_AC0_COPY|_AN);
	dbga(r7.l,0x1006);
	dbga(r7.h,0);

	A1 = A1(S), A0 = A0(S);
	_dbg a0;
	_dbg a1;
	_dbg astat;
	r7 = astat;
	checkreg r7, (_AV0S|_AV0|_AC0|_AC0_COPY|_AN);

	r0.l = 0x0;
	r0.h = 0x8000;
	r1 = 0;
	a1 = r0;
	a1 = a1 << 8;
	a0 = r1;
	r7 = astat;

	A1 = A1(S), A0 = A0(S);
	_dbg a0;
	_dbg a1;
	_dbg astat;
	r7 = astat;
	checkreg r7, (_AV1S|_AV1|_AV0S|_AC0|_AC0_COPY|_AN|_AZ);
	dbga(r7.l,0x1007);

	dbga(r7.h,0xe);

	pass;
