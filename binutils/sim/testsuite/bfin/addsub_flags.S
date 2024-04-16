// ACP 5.17 Dual ALU ops
// AZ, AN, AC0, AC1, V and VS are affected
// AV0, AV0S, AV1, AV1S are unaffected
# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	init_r_regs 0;
	ASTAT = R0;
	A0 = A1 = 0;

	r0=0;
	r0.h=0x7fff;
	r2=0;
	r2.h=0x7000;
	r1=r0+r2,r3=r0-r2;
	r7=astat;
	_dbg r1;
	_dbg r3;
	_dbg astat;
	CHECKREG R7, (_VS|_V|_V_COPY|_AC0|_AC0_COPY|_AN);

	a1=r2;
	a0=r0;
	r1=a0+a1, r3=a0-a1;
	r7=astat;
	_dbg a0;
	_dbg a1;
	_dbg r1;
	_dbg r3;
	_dbg astat;
	CHECKREG R7, (_VS|_V|_V_COPY|_AC0|_AC0_COPY|_AN);

	a0=r2;
	a1=r0;
	r1=a1+a0, r3=a1-a0;
	r7=astat;
	_dbg a0;
	_dbg a1;
	_dbg r1;
	_dbg r3;
	_dbg astat;
	CHECKREG R7, (_VS|_V|_V_COPY|_AC0|_AC0_COPY|_AN);

	r0.h=0xafff;
	r2.h=0xa000;
	a1=r2;
	a0=r0;
	r1=a0+a1, r3=a0-a1;
	r7=astat;
	_dbg a0;
	_dbg a1;
	_dbg r1;
	_dbg r3;
	_dbg astat;
	CHECKREG R7, (_VS|_V|_V_COPY|_AC0|_AC0_COPY|_AC1);

	r1=a0+a1, r3=a0-a1 (s);
	r7=astat;
	_dbg a0;
	_dbg a1;
	_dbg r1;
	_dbg r3;
	_dbg astat;
	CHECKREG R7, (_VS|_V|_V_COPY|_AC0|_AC0_COPY|_AC1|_AN);

	r0.h=0xafff;
	r2.h=0xa000;
	a0=r2;
	a1=r0;
	r1=a1+a0, r3=a1-a0;
	r7=astat;
	_dbg a0;
	_dbg a1;
	_dbg r1;
	_dbg r3;
	_dbg astat;
	CHECKREG R7, (_VS|_V|_V_COPY|_AC0|_AC0_COPY|_AC1);

	r1=a1+a0, r3=a1-a0 (s);
	r7=astat;
	_dbg a0;
	_dbg a1;
	_dbg r1;
	_dbg r3;
	_dbg astat;
	CHECKREG R7, (_VS|_V|_V_COPY|_AC0|_AC0_COPY|_AC1|_AN);

	r2.h=0x8001;
	r1=r0+r2,r3=r0-r2;
	_dbg r1;
	_dbg r3;
	_dbg astat;
	r7=astat;
	CHECKREG R7, (_VS|_V|_V_COPY|_AC0|_AC0_COPY|_AC1);

	r2.h=0x8000;
	r1=r0+r2,r3=r0-r2;
	r7=astat;
	_dbg r1;
	_dbg r3;
	_dbg astat;
	CHECKREG R7, (_VS|_V|_V_COPY|_AC0|_AC0_COPY|_AC1);

	pass;
