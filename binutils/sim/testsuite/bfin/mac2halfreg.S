# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	init_r_regs 0;
	ASTAT = R0;

	r3.h=0x8000;
	r2.h=0x8000;
	r7=0;
	r6.l=0x7f;
	a0.x=r6.l;
	r6.h=0x7fff;
	r6.l=0xffff;
	a0.w=r6;
	_dbg a0;
	r3.l=(a0+=r3.h*r2.h);
	r7=ASTAT;
	_dbg A0;
	_dbg r3;
	_dbg ASTAT;
//AV0 does not overflow
	checkreg r7, (_VS|_V|_V_COPY);

	pass
