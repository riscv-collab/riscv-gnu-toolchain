//  ALU test program.
//  Test instructions   reg = (A0+=A1)

#include "test.h"
.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;

	loadsym P0, data0;

	R0 = [ P0 ++ ];
	R1 = [ P0 ++ ];
	R2 = [ P0 ++ ];
	R3 = [ P0 ++ ];
	R4 = [ P0 ++ ];

// add accums and transfer result
	A1 = A0 = 0;
	A1.w = R0;
	A0.w = R0;
	R6 = ( A0 += A1 );
	CHECKREG R6, 0x22222222;
	R6 = A0.w;
	CHECKREG R6, 0x22222222;
	R7 = A0.x;
	CHECKREG R7, 0;
	R6 = A1.w;
	CHECKREG R6, 0x11111111;
	R7 = A1.x;
	CHECKREG R7, 0;

// add accums and transfer result (saturate positive)
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1.w = R1;
	A0.w = R1;
	R6 = ( A0 += A1 );
	CHECKREG R6, 0x7fffffff;
	R6 = A0.w;
	CHECKREG R6, 0xfffffffe;
	R7 = A0.x;
	CHECKREG R7, 0;
	R6 = A1.w;
	CHECKREG R6, 0x7fffffff;
	_DBG ASTAT;
	R7 = A1.x;
	_DBG ASTAT;
	CHECKREG R7, 0;
	R7 = ASTAT;
	CHECKREG R7, (_VS|_V|_V_COPY);

// add accums and transfer result (saturate negative)
	R7 = 0;
	ASTAT = R7;
	A1 = A0 = 0;
	A1.w = R2;
	A0.w = R2;
	A1.x = R3.L;
	A0.x = R3.L;
	R6 = ( A0 += A1 );
	CHECKREG R6, 0x80000000;
	R6 = A0.w;
	CHECKREG R6, 0x00000000;
	R7 = A0.x;
	CHECKREG R6, 0;
	R6 = A1.w;
	CHECKREG R6, 0x80000000;
	R7 = A1.x;
	CHECKREG R7, 0xffffffff;
	R7 = ASTAT;
	_DBG ASTAT;
	CHECKREG R7, (_VS|_V|_V_COPY|_AC0|_AC0_COPY|_AN);

	pass

	.data
data0:
	.dw 0x1111
	.dw 0x1111
	.dw 0xffff
	.dw 0x7fff
	.dw 0x0000
	.dw 0x8000
	.dw 0x00ff
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
