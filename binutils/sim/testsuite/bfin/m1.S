//  MAC test program.
//  Test for positive and negative saturation using
//  SIGNED FRACTIONAL  mode.
# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	init_r_regs 0;
	ASTAT = R0;

// load r0=0x00007fff
// load r1=0x00007fff
	loadsym p0, data0
	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];

	R5 = 0;
	R4 = 0;
	BITSET( R4 , 9 );
	A1 = A0 = 0;

LOOP1:
	A1 -= R0.L * R1.L, A0 += R0.L * R1.L;
//_DBG a1;
//_DBG a0;
	R4 += -1;
	CC = R4 == R5;
	IF !CC JUMP LOOP1;
R3 = ASTAT;
CHECKREG R3, (_AV1S|_AV1|_AV0S|_AV0|_AC0|_AC0_COPY|_CC|_AZ);

	_DBG A1;
	_DBG A0;

	R6 = A1.w;
	_DBG ASTAT;
	R7.L = A1.x;
	R3 = ASTAT;
	_DBG r3;
	CHECKREG R3, (_AV1S|_AV1|_AV0S|_AV0|_AC0|_AC0_COPY|_CC|_AZ);

	CHECKREG R6, 0;
	CHECKREG R7, 0x0000FF80;
	R6 = A0.w;
	R7.L = A0.x;
	CHECKREG R6, 0xffffffff;
	CHECKREG R7, 0x7f;

	pass

	.data 0x1000;
data0:
	.dw 0x7fff
	.dw 0x0000
	.dw 0x7fff
	.dw 0x0000
