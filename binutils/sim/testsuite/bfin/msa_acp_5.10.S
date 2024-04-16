# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	init_r_regs 0
	astat = r0;

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
	checkreg r7, (_AV0|_AV0S);

	r1.l = 0x0;
	r1.h = 0x8000;
	A1.w=r1;
	_dbg a0;
	a1 = a1 << 8;
	_dbg a1;
	_dbg astat;
	r7 = astat;
	checkreg r7, (_AV0|_AV0S|_AN);

	A1 = - A1;
	r7 = astat;
	checkreg r7, (_AV1|_AV1S|_AV0|_AV0S);
	_dbg astat;
	_dbg a1;
	pass;
