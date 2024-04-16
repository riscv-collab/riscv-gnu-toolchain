//  ALU test program.
//  Test instructions:
//	dreg = -dreg (ns);
//	dreg = -dreg (s);
// dspalu32 negate instruction
# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;

// CHECK MULTI ISSUE
	r1=0x5;
	loadsym i0, data0;
	r2 = -r1 (ns) || r3=[i0++];
	checkreg r2, 0xfffffffb;
	r3 = astat
	checkreg r3, (_AN);

	r1.h = 0x8000;
	r1.l = 0x0;
	r2 = -r1 (s);
	checkreg r2, 0x7fffffff;

	r3 = astat;
	_dbg astat;
	checkreg r3, (_VS|_V|_V_COPY);

	pass

	.data
data0:
	.space (0x10);
