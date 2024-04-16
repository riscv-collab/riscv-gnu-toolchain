# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	r0 = 0;
	ASTAT = R0;

	r0.h =0x8000;
	r1 = 0;
	r1.h =0x8000;
	cc = r1==r0;
	_dbg astat;
	r7=astat;
	CHECKREG R7, (_AC0|_AC0_COPY|_CC|_AZ);

	r7=0;
	astat=r7;
	r0.l = 0xffff;
	r0.h =0x7fff;
	r1.l = 0xffff;
	r1.h =0x7fff;
	cc = r1==r0;
	_dbg astat;
	r7=astat;
	CHECKREG R7, (_AC0|_AC0_COPY|_CC|_AZ);

	r7=0;
	astat=r7;
	r0.l = 0;
	r0.h =0x8000;
	r1.l = 0xffff;
	r1.h =0x7fff;
	cc = r1==r0;
	_dbg astat;
	r7=astat;
	CHECKREG R7, (_UNSET);

	pass;
