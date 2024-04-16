//Original:/testcases/core/c_cc2stat_cc_az/c_cc2stat_cc_az.dsp
// Spec Reference: cc2stat cc az
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

// test CC = AZ 0-0, 0-1, 1-0, 1-1
R7 = 0x00;
ASTAT = R7;	// cc = 0, AZ = 0
CC = AZ;	//
R0 = CC;	//

R7 = 0x01;
ASTAT = R7;	// cc = 0, AZ = 1
CC = AZ;	//
R1 = CC;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AZ = 0
CC = AZ;	//
R2 = CC;	//

R7 = 0x21;
ASTAT = R7;	// cc = 1, AZ = 1
CC = AZ;	//
R3 = CC;	//

// test cc |= AZ (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AZ = 0
CC |= AZ;	//
R4 = CC;	//

R7 = 0x01;
ASTAT = R7;	// cc = 0, AZ = 1
CC |= AZ;	//
R5 = CC;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AZ = 0
CC |= AZ;	//
R6 = CC;	//

R7 = 0x21;
ASTAT = R7;	// cc = 1, AZ = 1
CC |= AZ;	//
R7 = CC;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000001;

// test CC &= AZ (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AZ = 0
CC &= AZ;	//
R4 = CC;	//

R7 = 0x01;
ASTAT = R7;	// cc = 0, AZ = 1
CC &= AZ;	//
R5 = CC;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AZ = 0
CC &= AZ;	//
R6 = CC;	//

R7 = 0x21;
ASTAT = R7;	// cc = 1, AZ = 1
CC &= AZ;	//
R7 = CC;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000001;

// test CC ^= AZ (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AZ = 0
CC ^= AZ;	//
R4 = CC;	//

R7 = 0x01;
ASTAT = R7;	// cc = 0, AZ = 1
CC ^= AZ;	//
R5 = CC;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AZ = 0
CC ^= AZ;	//
R6 = CC;	//

R7 = 0x21;
ASTAT = R7;	// cc = 1, AZ = 1
CC ^= AZ;	//
R7 = CC;	//


CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;

// test AZ = CC 0-0, 0-1, 1-0, 1-1
R7 = 0x00;
ASTAT = R7;	// cc = 0, AZ = 0
AZ = CC;	//
R0 = ASTAT;	//

R7 = 0x01;
ASTAT = R7;	// cc = 0, AZ = 1
AZ = CC;	//
R1 = ASTAT;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AZ = 0
AZ = CC;	//
R2 = ASTAT;	//

R7 = 0x21;
ASTAT = R7;	// cc = 1, AZ = 1
AZ = CC;	//
R3 = ASTAT;	//

// test AZ |= CC (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AZ = 0
AZ |= CC;	//
R4 = ASTAT;	//

R7 = 0x01;
ASTAT = R7;	// cc = 0, AZ = 1
AZ |= CC;	//
R5 = ASTAT;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AZ = 0
AZ |= CC;	//
R6 = ASTAT;	//

R7 = 0x21;
ASTAT = R7;	// cc = 1, AZ = 1
AZ |= CC;	//
R7 = ASTAT;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000021;
CHECKREG r3, 0x00000021;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000021;
CHECKREG r7, 0x00000021;

// test AZ &= CC (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AZ = 0
AZ &= CC;	//
R4 = ASTAT;	//

R7 = 0x01;
ASTAT = R7;	// cc = 0, AZ = 1
AZ &= CC;	//
R5 = ASTAT;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AZ = 0
AZ &= CC;	//
R6 = ASTAT;	//

R7 = 0x21;
ASTAT = R7;	// cc = 1, AZ = 1
AZ &= CC;	//
R7 = ASTAT;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000021;
CHECKREG r3, 0x00000021;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000020;
CHECKREG r7, 0x00000021;

// test AZ ^= CC (0-0, 0-1, 1-0, 1-1)
R7 = 0x00;
ASTAT = R7;	// cc = 0, AZ = 0
AZ ^= CC;	//
R4 = ASTAT;	//

R7 = 0x01;
ASTAT = R7;	// cc = 0, AZ = 1
AZ ^= CC;	//
R5 = ASTAT;	//

R7 = 0x20;
ASTAT = R7;	// cc = 1, AZ = 0
AZ ^= CC;	//
R6 = ASTAT;	//

R7 = 0x21;
ASTAT = R7;	// cc = 1, AZ = 1
AZ ^= CC;	//
R7 = ASTAT;	//

CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000021;
CHECKREG r3, 0x00000021;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000021;
CHECKREG r7, 0x00000020;


pass
