//Original:/testcases/core/c_cc2stat_cc_an/c_cc2stat_cc_an.dsp
// Spec Reference: cc2stat cc an
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

// test CC = AN 0-0, 0-1, 1-0, 1-1
R7 = 0x00;
ASTAT = R7;	// cc = 0, AN = 0
CC = AN;	//
R0 = CC;	//

R7 = 0x02;
ASTAT = R7;	// cc = 0, AN = 1
CC = AN;	//
R1 = CC;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AN = 0
CC = AN;	//
R2 = CC;	//

R7 = 0x22;
ASTAT = R7;	// cc = 1, AN = 1
CC = AN;	//
R3 = CC;	//

// test cc |= AN (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AN = 0
CC |= AN;	//
R4 = CC;	//

R7 = 0x02;
ASTAT = R7;	// cc = 0, AN = 1
CC |= AN;	//
R5 = CC;	//

R7 = 0x22;
ASTAT = R7;	// cc = 1, AN = 0
CC |= AN;	//
R6 = CC;	//

R7 = 0x22;
ASTAT = R7;	// cc = 1, AN = 1
CC |= AN;	//
R7 = CC;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000001;

// test CC &= AN (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AN = 0
CC &= AN;	//
R4 = CC;	//

R7 = 0x02;
ASTAT = R7;	// cc = 0, AN = 1
CC &= AN;	//
R5 = CC;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AN = 0
CC &= AN;	//
R6 = CC;	//

R7 = 0x22;
ASTAT = R7;	// cc = 1, AN = 1
CC &= AN;	//
R7 = CC;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;

// test CC ^= AN (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AN = 0
CC ^= AN;	//
R4 = CC;	//

R7 = 0x02;
ASTAT = R7;	// cc = 0, AN = 1
CC ^= AN;	//
R5 = CC;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AN = 0
CC ^= AN;	//
R6 = CC;	//

R7 = 0x22;
ASTAT = R7;	// cc = 1, AN = 1
CC ^= AN;	//
R7 = CC;	//


CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;

// test AN = CC 0-0, 0-1, 1-0, 1-1
R7 = 0x00;
ASTAT = R7;	// cc = 0, AN = 0
AN = CC;	//
R0 = ASTAT;	//

R7 = 0x02;
ASTAT = R7;	// cc = 0, AN = 1
AN = CC;	//
R1 = ASTAT;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AN = 0
AN = CC;	//
R2 = ASTAT;	//

R7 = 0x22;
ASTAT = R7;	// cc = 1, AN = 1
AN = CC;	//
R3 = ASTAT;	//

// test AN |= CC (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AN = 0
AN |= CC;	//
R4 = ASTAT;	//

R7 = 0x02;
ASTAT = R7;	// cc = 0, AN = 1
AN |= CC;	//
R5 = ASTAT;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AN = 0
AN |= CC;	//
R6 = ASTAT;	//

R7 = 0x22;
ASTAT = R7;	// cc = 1, AN = 1
AN |= CC;	//
R7 = ASTAT;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000022;
CHECKREG r3, 0x00000022;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000002;
CHECKREG r6, 0x00000022;
CHECKREG r7, 0x00000022;

// test AN &= CC (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AN = 0
AN &= CC;	//
R4 = ASTAT;	//

R7 = 0x02;
ASTAT = R7;	// cc = 0, AN = 1
AN &= CC;	//
R5 = ASTAT;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AN = 0
AN &= CC;	//
R6 = ASTAT;	//

R7 = 0x22;
ASTAT = R7;	// cc = 1, AN = 1
AN &= CC;	//
R7 = ASTAT;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000022;
CHECKREG r3, 0x00000022;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000020;
CHECKREG r7, 0x00000022;

// test AN ^= CC (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AN = 0
AN ^= CC;	//
R4 = ASTAT;	//

R7 = 0x02;
ASTAT = R7;	// cc = 0, AN = 1
AN ^= CC;	//
R5 = ASTAT;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AN = 0
AN ^= CC;	//
R6 = ASTAT;	//

R7 = 0x22;
ASTAT = R7;	// cc = 1, AN = 1
AN ^= CC;	//
R7 = ASTAT;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000022;
CHECKREG r3, 0x00000022;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000002;
CHECKREG r6, 0x00000022;
CHECKREG r7, 0x00000020;


pass
