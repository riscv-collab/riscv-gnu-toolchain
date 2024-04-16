# mach: bfin

#include "test.h"
.include "testutils.inc"
	start


	init_r_regs 0;
	ASTAT = r0;

	r0 = -1;
	A0 = r0;
	_dbg astat;
	r0 +=1;
	_dbg astat;
	A0 = A0 (S);
	r7 = astat;
	checkreg r7, (_AC0|_AC0_COPY|_AN)
	_dbg a0;
	_dbg astat;
	A0 = 0;
	A0 = A0 (S);
	r7 = astat;
	checkreg r7, (_AC0|_AC0_COPY|_AZ)
	_dbg a0;
	_dbg astat;

	r0 = -1;
	A1 = r0;
	_dbg astat;
	r0 +=1;
	r7 = astat;
	checkreg r7, (_AC0|_AC0_COPY|_AZ);
	_dbg astat;
	A1 = A1 (S);
	r7 = astat;
	_dbg astat;
	checkreg r7, (_AC0|_AC0_COPY|_AN)
	_dbg a1;
	_dbg astat;
	A1 = 0;
	A1 = A1 (S);
	r7 = astat;
	checkreg r7, (_AC0|_AC0_COPY|_AZ)
	_dbg a1;
	_dbg astat;

	r1.l = 0x0;
	r1.h = 0x8000;
	a1 = r1;
	a1 = a1 << 8;
	_dbg a1;
	r7 = astat;
	checkreg r7, (_AC0|_AC0_COPY|_AN)
	a1 = a1(s);
	_dbg astat;
	_dbg a1;
	r7 = astat;
	checkreg r7, (_AV1S|_AV1|_AC0|_AC0_COPY|_AN)

	r1.l = 0x0;
	r1.h = 0x8000;
	a0 = r1;
	a0 = a0 << 8;
	_dbg a0;
	a0 = a0(s);
	_dbg astat;
	_dbg a0;
	r7 = astat;
	checkreg r7, (_AV1S|_AV1|_AV0S|_AV0|_AC0|_AC0_COPY|_AN)
	pass
