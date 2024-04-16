//  When the RND12 instruction produces large negative results, the AV0 flag is
//  should not be set.
# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	init_r_regs 0;
	ASTAT = R0;

	R0.H = 0xcef4;
	R0.L = 0x3ed6;
	R1.H = 0x56f4;
	R1.L = 0x417a;
	R2.H = R0 - R1 (RND12);

	_DBG ASTAT;
	R0 = ASTAT;
	CHECKREG R0, (_VS|_V|_V_COPY|_AN);
	CHECKREG R2, 0x80000000;

	pass
