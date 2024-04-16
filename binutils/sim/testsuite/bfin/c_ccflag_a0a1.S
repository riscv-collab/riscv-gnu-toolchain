//Original:/testcases/core/c_ccflag_a0a1/c_ccflag_a0a1.dsp
// Spec Reference: ccflag a0-a1 (==, <, <=)
# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	imm32 r0, 0x12345778;
	imm32 r1, 0x12345678;
	imm32 r2, 0x056789ab;
	imm32 r3, 0x80231345;

	imm32 r4, 0x00770088;
	imm32 r5, 0x009900aa;
	imm32 r6, 0x00bb00cc;
	imm32 r7, _UNSET;

	ASTAT = R7;
	R4 = ASTAT;
	A0 = R0;
	A1 = R0;

// positive a0 EQUAL to a1
	CC = A0 == A1;
	R5 = ASTAT;
	CC = A0 < A1;
	R6 = ASTAT;
	CHECKREG r4, _UNSET;
	CHECKREG r5, (_AC0|_CC|_AC0_COPY|_AZ);
	CHECKREG r6, (_AC0|_AC0_COPY|_AZ);
	CC = A0 <= A1;
	R5 = ASTAT;
	CC = A0 < A1;
	R6 = ASTAT;
	CC = A0 <= A1;
	R7 = ASTAT;
	CHECKREG r5, (_AC0|_CC|_AC0_COPY|_AZ);
	CHECKREG r6, (_AC0|_AC0_COPY|_AZ);
	CHECKREG r7, (_AC0|_CC|_AC0_COPY|_AZ);

// positive a0 GREATER than to positive a1
	A1 = R1;
	CC = A0 == A1;
	R5 = ASTAT;
	CC = A0 < A1;
	R6 = ASTAT;
	CC = A0 <= A1;
	R7 = ASTAT;
	CHECKREG r5, (_AC0|_AC0_COPY);	// carry
	CHECKREG r6, (_AC0|_AC0_COPY);
	CHECKREG r7, (_AC0|_AC0_COPY);

// positive a0 LESS  than to positive a1
	A1 = R2;
	CC = A0 == A1;
	R5 = ASTAT;
	CC = A0 < A1;
	R6 = ASTAT;
	CC = A0 <= A1;
	R7 = ASTAT;
	CHECKREG r5, (_AC0|_AC0_COPY);
	CHECKREG r6, (_AC0|_AC0_COPY);
	CHECKREG r7, (_AC0|_AC0_COPY);

// positive a0 GREATER than to neg a1
	A1 = R3;
	CC = A0 == A1;
	R5 = ASTAT;
	CC = A0 < A1;
	R6 = ASTAT;
	CC = A0 <= A1;
	R7 = ASTAT;
	CHECKREG r5, _UNSET;
	CHECKREG r6, _UNSET;
	CHECKREG r7, _UNSET;

// negative a0  and positive a1
	imm32 r0, -1;
	imm32 r1,  2;
	imm32 r2, -3;
	imm32 r3, -4;
	A0 = R0;
	A1 = R1;

	R7 = 0;
	ASTAT = R7;
	R4 = ASTAT;

	CC = A0 == A1;
	R5 = ASTAT;
	CC = A0 < A1;
	R6 = ASTAT;
	CC = A0 <= A1;
	R7 = ASTAT;
	CHECKREG r4, _UNSET;
	CHECKREG r5, (_AC0|_AC0_COPY|_AN);
	CHECKREG r6, (_AC0|_AC0_COPY|_CC|_AN);
	CHECKREG r7, (_AC0|_AC0_COPY|_CC|_AN);

//  negative a0 LESS than neg a1
	A0 = R3;
	A1 = R4;
	CC = A0 == A1;
	R5 = ASTAT;
	CC = A0 < A1;
	R6 = ASTAT;
	CC = A0 <= A1;
	R7 = ASTAT;
	CHECKREG r4, _UNSET;
	CHECKREG r5, (_AC0|_AC0_COPY|_AN);
	CHECKREG r6, (_AC0|_AC0_COPY|_CC|_AN);
	CHECKREG r7, (_AC0|_AC0_COPY|_CC|_AN);

// negative a0 GREATER neg a1
	A0 = R0;
	A1 = R3;
	CC = A0 == A1;
	R5 = ASTAT;
	CC = A0 < A1;
	R6 = ASTAT;
	CC = A0 <= A1;
	R7 = ASTAT;
	CHECKREG r4, _UNSET;
	CHECKREG r5, (_AC0|_AC0_COPY);
	CHECKREG r6, (_AC0|_AC0_COPY);
	CHECKREG r7, (_AC0|_AC0_COPY);

// negative a0  EQUAL neg imm3
	A0 = R3;
	A1 = R3;
	CC = A0 == A1;
	R5 = ASTAT;
	CC = A0 < A1;
	R6 = ASTAT;
	CC = A0 <= A1;
	R7 = ASTAT;
	CHECKREG r4, _UNSET;
	CHECKREG r5, (_AC0|_CC|_AC0_COPY|_AZ);
	CHECKREG r6, (_AC0|_AC0_COPY|_AZ);
	CHECKREG r7, (_AC0|_CC|_AC0_COPY|_AZ);

	pass
