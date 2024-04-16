//Original:/testcases/core/c_loopsetup_prelc/c_loopsetup_prelc.dsp
// Spec Reference: loopsetup preload lc0 lc1
# mach: bfin

.include "testutils.inc"
	start


INIT_R_REGS 0;

ASTAT = r0;

//p0 = 2;
P1 = 3;
P2 = 4;
P3 = 5;
P4 = 6;
P5 = 7;
SP = 8;
FP = 9;

R0 = 0x05;
R1 = 0x10;
R2 = 0x20;
R3 = 0x30;
R4 = 0x40 (X);
R5 = 0x50 (X);
R6 = 0x60 (X);
R7 = 0x70 (X);

LC0 = R0;
LC1 = R1;

LSETUP ( start1 , end1 ) LC0;
start1: R0 += 1;
 R1 += -2;
end1: R2 += 3;
 R3 += 4;
LSETUP ( start2 , end2 ) LC1;
start2: R4 += 4;
end2: R5 += -5;
 R3 += 1;
LSETUP ( start3 , end3 ) LC0 = P3;
start3: R6 += 6;
end3: R7 += -7;
 R3 += 1;
CHECKREG r0, 0x0000000a;
CHECKREG r1, 0x00000006;
CHECKREG r2, 0x0000002f;
CHECKREG r3, 0x00000036;
CHECKREG r4, 0x00000080;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x0000007E;
CHECKREG r7, 0x0000004D;

R0 = 0x05;
R1 = 0x10;
R2 = 0x20;
R3 = 0x30;
R4 = 0x40 (X);
R5 = 0x50 (X);
R6 = 0x60 (X);
R7 = 0x70 (X);

LC0 = R2;
LC1 = R3;

LSETUP ( start4 , end4 ) LC0;
start4: R0 += 1;
 R1 += -2;
end4: R2 += 3;
 R3 += 4;
LSETUP ( start5 , end5 ) LC1;
start5: R4 += 1;
end5: R5 += -2;
 R3 += 3;

LSETUP ( start6 , end6 ) LC0 = P2;
start6: R6 += 4;
end6: R7 += -5;
 R3 += 6;
CHECKREG r0, 0x00000025;
CHECKREG r1, 0xFFFFFFD0;
CHECKREG r2, 0x00000080;
CHECKREG r3, 0x0000003D;
CHECKREG r4, 0x00000070;
CHECKREG r5, 0xFFFFFFF0;
CHECKREG r6, 0x00000070;
CHECKREG r7, 0x0000005C;
LSETUP ( start7 , end7 ) LC1;
start7: R4 += 4;
end7: R5 += -5;
 R3 += 6;
CHECKREG r0, 0x00000025;
CHECKREG r1, 0xFFFFFFD0;
CHECKREG r2, 0x00000080;
CHECKREG r3, 0x00000043;
CHECKREG r4, 0x00000074;
CHECKREG r5, 0xFFFFFFEB;
CHECKREG r6, 0x00000070;
CHECKREG r7, 0x0000005C;

P1 = 12;
P2 = 14;
P3 = 16;
P4 = 18;
P5 = 20;
SP = 22;
FP = 24;

R0 = 0x05;
R1 = 0x10;
R2 = 0x20;
R3 = 0x30;
R4 = 0x40 (X);
R5 = 0x50 (X);
R6 = 0x25;
R7 = 0x32;

LC0 = R6;
LC1 = R7;
LSETUP ( start11 , end11 ) LC0;
start11: R0 += 1;
 R1 += -1;
end11: R2 += 1;
 R3 += 1;
LSETUP ( start12 , end12 ) LC1;
start12: R4 += 1;
end12: R5 += -1;
 R3 += 1;
LSETUP ( start13 , end13 ) LC1 = P4;
start13: R6 += 1;
end13: R7 += -1;
 R3 += 1;
CHECKREG r0, 0x0000002A;
CHECKREG r1, 0xFFFFFFEB;
CHECKREG r2, 0x00000045;
CHECKREG r3, 0x00000033;
CHECKREG r4, 0x00000072;
CHECKREG r5, 0x0000001E;
CHECKREG r6, 0x00000037;
CHECKREG r7, 0x00000020;


pass
