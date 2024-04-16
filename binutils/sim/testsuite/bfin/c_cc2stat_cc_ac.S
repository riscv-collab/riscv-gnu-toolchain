//Original:/testcases/core/c_cc2stat_cc_ac/c_cc2stat_cc_ac.dsp
// Spec Reference: cc2stat cc ac
# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	imm32 r0, _UNSET;
	imm32 r1, _UNSET;
	imm32 r2, _UNSET;
	imm32 r3, _UNSET;
	imm32 r4, _UNSET;
	imm32 r5, _UNSET;
	imm32 r6, _UNSET;
	imm32 r7, _UNSET;

// test CC = AC      0-0, 0-1, 1-0, 1-1
	imm32 R7, 0x00;
	ASTAT = R7;	// cc = 0, AC0 = 0
	CC = AC0;	//
	R0 = CC;	//

	imm32 R7, _AC0;
	ASTAT = R7;	// cc = 0, AC0 = 1
	CC = AC0;	//
	R1 = CC;	//

	imm32 R7, _CC;
	ASTAT = R7;	// cc = 1, AC0 = 0
	CC = AC0;	//
	R2 = CC;	//

	imm32 R7, (_CC|_AC0);
	ASTAT = R7;	// cc = 1, AC0 = 1
	CC = AC0;	//
	R3 = CC;	//

// test cc |= AC (0-0,  0-1,  1-0,  1-1)
	imm32 R7, 0x00;
	ASTAT = R7;	// cc = 0, AC0 = 0
	CC |= AC0;	//
	R4 = CC;	//

	imm32 R7, _AC0;
	ASTAT = R7;	// cc = 0, AC0 = 1
	CC |= AC0;	//
	R5 = CC;	//

	imm32 R7, (_CC|_AC0);
	ASTAT = R7;	// cc = 1, AC0 = 0
	CC |= AC0;	//
	R6 = CC;	//

	imm32 R7, (_CC|_AC0);
	ASTAT = R7;	// cc = 1, AC0 = 1
	CC |= AC0;	//
	R7 = CC;	//

	CHECKREG r0, _UNSET;
	CHECKREG r1, _SET;
	CHECKREG r2, _UNSET;
	CHECKREG r3, _SET;
	CHECKREG r4, _UNSET;
	CHECKREG r5, _SET;
	CHECKREG r6, _SET;
	CHECKREG r7, _SET;

// test CC &= AC (0-0,  0-1,  1-0,  1-1)
	imm32 R7, 0x00;
	ASTAT = R7;	// cc = 0, AC0 = 0
	CC &= AC0;	//
	R4 = CC;	//

	imm32 R7, _AC0;
	ASTAT = R7;	// cc = 0, AC0 = 1
	CC &= AC0;	//
	R5 = CC;	//

	imm32 R7, _CC;
	ASTAT = R7;	// cc = 1, AC0 = 0
	CC &= AC0;	//
	R6 = CC;	//

	imm32 R7, (_CC|_AC0);
	ASTAT = R7;	// cc = 1, AC0 = 1
	CC &= AC0;	//
	R7 = CC;	//

	CHECKREG r0, _UNSET;
	CHECKREG r1, _SET;
	CHECKREG r2, _UNSET;
	CHECKREG r3, _SET;
	CHECKREG r4, _UNSET;
	CHECKREG r5, _UNSET;
	CHECKREG r6, _UNSET;
	CHECKREG r7, _SET;

// test CC ^= AC (0-0,  0-1,  1-0,  1-1)
	imm32 R7, 0x00;
	ASTAT = R7;	// cc = 0, AC0 = 0
	CC ^= AC0;	//
	R4 = CC;	//

	imm32 R7, _AC0;
	ASTAT = R7;	// cc = 0, AC0 = 1
	CC ^= AC0;	//
	R5 = CC;	//

	imm32 R7, _CC;
	ASTAT = R7;	// cc = 1, AC0 = 0
	CC ^= AC0;	//
	R6 = CC;	//

	imm32 R7, (_CC|_AC0);
	ASTAT = R7;	// cc = 1, AC0 = 1
	CC ^= AC0;	//
	R7 = CC;	//

	CHECKREG r0, _UNSET;
	CHECKREG r1, _SET;
	CHECKREG r2, _UNSET;
	CHECKREG r3, _SET;
	CHECKREG r4, _UNSET;
	CHECKREG r5, _SET;
	CHECKREG r6, _SET;
	CHECKREG r7, _UNSET;

// test AC0 = CC      0-0, 0-1, 1-0, 1-1
	imm32 R7, 0x00;
	ASTAT = R7;	// cc = 0, AC0 = 0
	AC0 = CC;	//
	R0 = ASTAT;	//

	imm32 R7, _AC0;
	ASTAT = R7;	// cc = 0, AC0 = 1
	AC0 = CC;	//
	R1 = ASTAT;	//

	imm32 R7, _CC;
	ASTAT = R7;	// cc = 1, AC0 = 0
	AC0 = CC;	//
	R2 = ASTAT;	//

	imm32 R7, (_CC|_AC0);
	ASTAT = R7;	// cc = 1, AC0 = 1
	AC0 = CC;	//
	R3 = ASTAT;	//

// test AC0 |= CC (0-0,  0-1,  1-0,  1-1)
	imm32 R7, 0x00;
	ASTAT = R7;	// cc = 0, AC0 = 0
	AC0 |= CC;	//
	R4 = ASTAT;	//

	imm32 R7, _AC0;
	ASTAT = R7;	// cc = 0, AC0 = 1
	AC0 |= CC;	//
	R5 = ASTAT;	//

	imm32 R7, _CC;
	ASTAT = R7;	// cc = 1, AC0 = 0
	AC0 |= CC;	//
	R6 = ASTAT;	//

	imm32 R7, (_CC|_AC0);
	ASTAT = R7;	// cc = 1, AC0 = 1
	AC0 |= CC;	//
	R7 = ASTAT;	//

	CHECKREG r0, _UNSET;
	CHECKREG r1, _UNSET;
	CHECKREG r2, (_AC0|_CC);
	CHECKREG r3, (_CC|_AC0);
	CHECKREG r4, _UNSET;
	CHECKREG r5, (_AC0);
	CHECKREG r6, (_AC0|_CC);
	CHECKREG r7, (_CC|_AC0);

// test AC0 &= CC (0-0,  0-1,  1-0,  1-1)
	imm32 R7, 0x00;
	ASTAT = R7;	// cc = 0, AC0 = 0
	AC0 &= CC;	//
	R4 = ASTAT;	//

	imm32 R7, _AC0;
	ASTAT = R7;	// cc = 0, AC0 = 1
	AC0 &= CC;	//
	R5 = ASTAT;	//

	imm32 R7, _CC;
	ASTAT = R7;	// cc = 1, AC0 = 0
	AC0 &= CC;	//
	R6 = ASTAT;	//

	imm32 R7, (_CC|_AC0);
	ASTAT = R7;	// cc = 1, AC0 = 1
	AC0 &= CC;	//
	R7 = ASTAT;	//

	CHECKREG r0, _UNSET;
	CHECKREG r1, _UNSET;
	CHECKREG r2, (_CC|_AC0);
	CHECKREG r3, (_CC|_AC0);
	CHECKREG r4, _UNSET;
	CHECKREG r5, _UNSET;
	CHECKREG r6, _CC;
	CHECKREG r7, (_CC|_AC0);

// test AC0 ^= CC (0-0,  0-1,  1-0,  1-1)
	imm32 R7, 0x00;
	ASTAT = R7;	// cc = 0, AC0 = 0
	AC0 ^= CC;	//
	R4 = ASTAT;	//

	imm32 R7, _AC0;
	ASTAT = R7;	// cc = 0, AC0 = 1
	AC0 ^= CC;	//
	R5 = ASTAT;	//

	imm32 R7, _CC;
	ASTAT = R7;	// cc = 1, AC0 = 0
	AC0 ^= CC;	//
	R6 = ASTAT;	//

	imm32 R7, (_CC|_AC0);
	ASTAT = R7;	// cc = 1, AC0 = 1
	AC0 ^= CC;	//
	R7 = ASTAT;	//

	CHECKREG r0, _UNSET;
	CHECKREG r1, _UNSET;
	CHECKREG r2, (_CC|_AC0);
	CHECKREG r3, (_CC|_AC0);
	CHECKREG r4, _UNSET;
	CHECKREG r5, (_AC0);
	CHECKREG r6, (_CC|_AC0);
	CHECKREG r7, _CC;

	pass
