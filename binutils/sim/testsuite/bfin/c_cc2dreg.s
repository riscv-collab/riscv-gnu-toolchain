//Original:/testcases/core/c_cc2dreg/c_cc2dreg.dsp
// Spec Reference: cc2dreg
# mach: bfin

.include "testutils.inc"
	start




imm32 r0, 0x00000000;
imm32 r1, 0x00120000;
imm32 r2, 0x00000003;
imm32 r3, 0x00000004;

imm32 r4, 0x00770088;
imm32 r5, 0x009900aa;
imm32 r6, 0x00bb00cc;
imm32 r7, 0x00000000;

ASTAT = R0;

CC = R1;
R1 = CC;
CC = R1;
CC = ! CC;
R2 = CC;
CC = R2;
CC = ! CC;
R3 = CC;
CC = R3;
CC = ! CC;
R4 = CC;
CC = R5;
R5 = CC;
CC = R6;
R6 = CC;
CC = ! CC;
R7 = CC;
R0 = CC;



CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000001;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000001;
CHECKREG r6, 0x00000001;
CHECKREG r7, 0x00000000;




pass
