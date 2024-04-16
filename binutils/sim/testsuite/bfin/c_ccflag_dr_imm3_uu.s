//Original:/proj/frio/dv/testcases/core/c_ccflag_dr_imm3_uu/c_ccflag_dr_imm3_uu.dsp
// Spec Reference: ccflag dr-imm3 (uu)
# mach: bfin

.include "testutils.inc"
	start


imm32 r0, 0x00000001;
imm32 r1, 0x00000002;
imm32 r2, 0x00000003;
imm32 r3, 0x00000004;

imm32 r4, 0x00770088;
imm32 r5, 0x009900aa;
imm32 r6, 0x00bb00cc;
imm32 r7, 0x00000000;

ASTAT = R7;
R4 = ASTAT;

// positive dreg EQUAL to positive imm3
CC = R0 == 1;
R5 = ASTAT;
CC = R0 < 1;
R6 = ASTAT;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00001025;
CHECKREG r6, 0x00001005;
CC = R0 <= 1;
R5 = ASTAT;
CC = R0 < 1 (IU);
R6 = ASTAT;
CC = R0 <= 1 (IU);
R7 = ASTAT;
CHECKREG r5, 0x00001025;
CHECKREG r6, 0x00001005;
CHECKREG r7, 0x00001025;

// positive dreg GREATER than to positive imm3
CC = R1 == 1;
R5 = ASTAT;
CC = R1 < 1 (IU);
R6 = ASTAT;
CC = R1 <= 1 (IU);
R7 = ASTAT;
CHECKREG r5, 0x00001004;  // carry
CHECKREG r6, 0x00001004;
CHECKREG r7, 0x00001004;

// positive dreg LESS  than to positive imm3
CC = R0 == 2;
R5 = ASTAT;
CC = R0 < 2 (IU);
R6 = ASTAT;
CC = R0 <= 2 (IU);
R7 = ASTAT;
CHECKREG r5, 0x00000002;
CHECKREG r6, 0x00000022;
CHECKREG r7, 0x00000022;

// positive dreg GREATER than to neg imm3
CC = R2 == -4;
R5 = ASTAT;
CC = R2 < 4 (IU);
R6 = ASTAT;
CC = R2 <= 4 (IU);
R7 = ASTAT;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000022;
CHECKREG r7, 0x00000022;

imm32 r0, -1;
imm32 r1, -2;
imm32 r2, -3;
imm32 r3, -4;
// negative dreg and positive imm3
R7 = 0;
ASTAT = R7;
R4 = ASTAT;

CC = R3 == 1;
R5 = ASTAT;
CC = R3 < 1 (IU);
R6 = ASTAT;
CC = R3 <= 1 (IU);
R7 = ASTAT;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00001006;
CHECKREG r6, 0x00001004;
CHECKREG r7, 0x00001004;

//  negative dreg LESS than neg imm3
CC = R2 == -1;
R4 = ASTAT;
CC = R2 < 1 (IU);
R5 = ASTAT;
CC = R2 <= 1 (IU);
R6 = ASTAT;
CHECKREG r4, 0x00000002;
CHECKREG r5, 0x00001004;
CHECKREG r6, 0x00001004;

// negative dreg GREATER neg imm3
CC = R0 == -2;
R4 = ASTAT;
CC = R0 < 4 (IU);
R5 = ASTAT;
CC = R0 <= 4 (IU);
R6 = ASTAT;
CHECKREG r4, 0x00001004;
CHECKREG r5, 0x00001004;
CHECKREG r6, 0x00001004;


imm32 r0, 0x00000000;
imm32 r1, 0x00000000;
imm32 r2, 0x00000000;
imm32 r3, 0x00000000;

imm32 r4, 0x00000001;
imm32 r5, 0x00000002;
imm32 r6, 0x00000003;
imm32 r7, 0x00000004;

ASTAT = R0;
R3 = ASTAT;

// positive dreg EQUAL to positive imm3
CC = R4 == 1;
R1 = ASTAT;
CC = R4 < 1 (IU);
R2 = ASTAT;
CC = R4 <= 1 (IU);
R3 = ASTAT;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00001025;
CHECKREG r2, 0x00001005;
CHECKREG r3, 0x00001025;

// positive dreg GREATER than to positive imm3
CC = R5 == 1;
R1 = ASTAT;
CC = R5 < 1 (IU);
R2 = ASTAT;
CC = R5 <= 1 (IU);
R3 = ASTAT;
CHECKREG r1, 0x00001004;  // carry
CHECKREG r2, 0x00001004;
CHECKREG r3, 0x00001004;

// positive dreg LESS  than to positive imm3
CC = R6 == 2;
R1 = ASTAT;
CC = R6 < 2 (IU);
R2 = ASTAT;
CC = R6 <= 2 (IU);
R3 = ASTAT;
CHECKREG r1, 0x00001004;
CHECKREG r2, 0x00001004;
CHECKREG r3, 0x00001004;

// positive dreg GREATER than to neg imm3
CC = R6 == -4;
R1 = ASTAT;
CC = R6 < 4 (IU);
R2 = ASTAT;
CC = R6 <= 4 (IU);
R3 = ASTAT;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000022;
CHECKREG r3, 0x00000022;

imm32 r4, -1;
imm32 r5, -2;
imm32 r6, -3;
imm32 r7, -4;
// negative dreg and positive imm3
R3 = 0;
ASTAT = R3;
R0 = ASTAT;

CC = R7 == 1;
R1 = ASTAT;
CC = R7 < 1 (IU);
R2 = ASTAT;
CC = R7 <= 1 (IU);
R3 = ASTAT;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00001006;
CHECKREG r2, 0x00001004;
CHECKREG r3, 0x00001004;

//  negative dreg LESS than neg imm3
CC = R6 == -1;
R0 = ASTAT;
CC = R6 < 1 (IU);
R1 = ASTAT;
CC = R6 <= 1 (IU);
R2 = ASTAT;
CHECKREG r0, 0x00000002;
CHECKREG r1, 0x00001004;
CHECKREG r2, 0x00001004;

// negative dreg GREATER neg imm3
CC = R4 == -4;
R0 = ASTAT;
CC = R4 < 4 (IU);
R1 = ASTAT;
CC = R4 <= 4 (IU);
R2 = ASTAT;
CHECKREG r0, 0x00001004;
CHECKREG r1, 0x00001004;
CHECKREG r2, 0x00001004;






pass;
