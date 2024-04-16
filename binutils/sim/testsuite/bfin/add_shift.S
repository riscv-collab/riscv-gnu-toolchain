// ACP 5.6 Flags for dreg=(dreg+dreg)<<1,2
# mach: bfin

#include "test.h"
.include "testutils.inc"
	start


	r1=0;
	ASTAT = R1;
	r2=0;
	r2.h=0x4000;
	r2=(r2+r1)<<2;
	dbga (r2.l,0x0);
	dbga (r2.h,0x0);
	_dbg ASTAT;
	r7=ASTAT;
	CHECKREG R7, (_VS|_V|_V_COPY|_AZ)

	r2=0;
	r2.h=0x4000;
	r2=(r2+r1)<<1;
	dbga (r2.l,0x0);
	dbga (r2.h,0x8000);
	_dbg ASTAT;
	r7=ASTAT;
	CHECKREG R7, (_VS|_V|_V_COPY|_AN)

	r1=0;
	r1.h=0xd300;
	r2=0;
	r2.h=0xb700;
	r2=(r2+r1)<<1;
	dbga (r2.l,0x0);
	dbga (r2.h,0x1400);
	_dbg ASTAT;
	r7=ASTAT;
	CHECKREG R7, (_VS|_V|_V_COPY)

	r0 = 1;
	r0 <<= 31;	// r0 should be 0x80000000
	r7 = 0;
	ASTAT = r7;
	_dbg r0;
	r1 = r0;
	_dbg r1;
	r1 = (r1 + r0) << 1;	// add overflows to zero, no shift overflow
	_dbg r1;
	_dbg ASTAT;
	r7 = ASTAT;
	CHECKREG R7, (_VS|_V|_V_COPY|_AZ);

	pass
