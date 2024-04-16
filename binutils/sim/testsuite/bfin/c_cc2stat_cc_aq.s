//Original:/testcases/core/c_cc2stat_cc_aq/c_cc2stat_cc_aq.dsp
// Spec Reference: cc2stat cc aq
# mach: bfin

.include "testutils.inc"
	start



imm32 r0, 0x00000000;
imm32 r1, 0x00000000;
imm32 r2, 0x00000000;
imm32 r3, 0x00000000;
imm32 r4, 0x00000000;
imm32 r5, 0x00000000;
imm32 r6, 0x00000000;
imm32 r7, 0x00000000;

// test CC = AQ 0-0, 0-1, 1-0, 1-1
R7 = 0x00;
ASTAT = R7;	// cc = 0, AQ = 0
CC = AQ;	//
R0 = CC;	//

R7 = 0x40 (X);
ASTAT = R7;	// cc = 0, AQ = 1
CC = AQ;	//
R1 = CC;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AQ = 0
CC = AQ;	//
R2 = CC;	//

R7 = 0x60 (X);
ASTAT = R7;	// cc = 1, AQ = 1
CC = AQ;	//
R3 = CC;	//

// test cc |= AQ (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AQ = 0
CC |= AQ;	//
R4 = CC;	//

R7 = 0x40 (X);
ASTAT = R7;	// cc = 0, AQ = 1
CC |= AQ;	//
R5 = CC;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AQ = 0
CC |= AQ;	//
R6 = CC;	//

R7 = 0x60 (X);
ASTAT = R7;	// cc = 1, AQ = 1
CC |= AQ;	//
R7 = CC;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000001;

// test CC &= AQ (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AQ = 0
CC &= AQ;	//
R4 = CC;	//

R7 = 0x40 (X);
ASTAT = R7;	// cc = 0, AQ = 1
CC &= AQ;	//
R5 = CC;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AQ = 0
CC &= AQ;	//
R6 = CC;	//

R7 = 0x60 (X);
ASTAT = R7;	// cc = 1, AQ = 1
CC &= AQ;	//
R7 = CC;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;

// test CC ^= AQ (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AQ = 0
CC ^= AQ;	//
R4 = CC;	//

R7 = 0x40 (X);
ASTAT = R7;	// cc = 0, AQ = 1
CC ^= AQ;	//
R5 = CC;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AQ = 0
CC ^= AQ;	//
R6 = CC;	//

R7 = 0x60 (X);
ASTAT = R7;	// cc = 1, AQ = 1
CC ^= AQ;	//
R7 = CC;	//


CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;

// test AQ = CC 0-0, 0-1, 1-0, 1-1
R7 = 0x00;
ASTAT = R7;	// cc = 0, AQ = 0
AQ = CC;	//
R0 = ASTAT;	//

R7 = 0x40 (X);
ASTAT = R7;	// cc = 0, AQ = 1
AQ = CC;	//
R1 = ASTAT;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AQ = 0
AQ = CC;	//
R2 = ASTAT;	//

R7 = 0x60 (X);
ASTAT = R7;	// cc = 1, AQ = 1
AQ = CC;	//
R3 = ASTAT;	//

// test AQ |= CC (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AQ = 0
AQ |= CC;	//
R4 = ASTAT;	//

R7 = 0x40 (X);
ASTAT = R7;	// cc = 0, AQ = 1
AQ |= CC;	//
R5 = ASTAT;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AQ = 0
AQ |= CC;	//
R6 = ASTAT;	//

R7 = 0x60 (X);
ASTAT = R7;	// cc = 1, AQ = 1
AQ |= CC;	//
R7 = ASTAT;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000060;
CHECKREG r3, 0x00000060;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000040;
CHECKREG r6, 0x00000060;
CHECKREG r7, 0x00000060;

// test AQ &= CC (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AQ = 0
AQ &= CC;	//
R4 = ASTAT;	//

R7 = 0x40 (X);
ASTAT = R7;	// cc = 0, AQ = 1
AQ &= CC;	//
R5 = ASTAT;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AQ = 0
AQ &= CC;	//
R6 = ASTAT;	//

R7 = 0x60 (X);
ASTAT = R7;	// cc = 1, AQ = 1
AQ &= CC;	//
R7 = ASTAT;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000060;
CHECKREG r3, 0x00000060;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000020;
CHECKREG r7, 0x00000060;

// test AQ ^= CC (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AQ = 0
AQ ^= CC;	//
R4 = ASTAT;	//

R7 = 0x40 (X);
ASTAT = R7;	// cc = 0, AQ = 1
AQ ^= CC;	//
R5 = ASTAT;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AQ = 0
AQ ^= CC;	//
R6 = ASTAT;	//

R7 = 0x60 (X);
ASTAT = R7;	// cc = 1, AQ = 1
AQ ^= CC;	//
R7 = ASTAT;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000060;
CHECKREG r3, 0x00000060;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000040;
CHECKREG r6, 0x00000060;
CHECKREG r7, 0x00000020;


pass
