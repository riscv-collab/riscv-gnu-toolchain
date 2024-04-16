//Original:/testcases/core/c_loopsetup_nested_prelc/c_loopsetup_nested_prelc.dsp
// Spec Reference: loopsetup nested preload lc0 lc1
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
R2 = 0x12;
R3 = 0x14;
R4 = 0x18;
R5 = 0x16;
R6 = 0x16;
R7 = 0x18;

LC0 = R0;
LC1 = R1;
LSETUP ( start1 , end1 ) LC0;
start1: R0 += 1;
 R1 += -2;
LSETUP ( start2 , end2 ) LC1;
start2: R4 += 4;
end2: R5 += -5;
 R3 += 1;
end1: R2 += 3;
 R3 += 4;
LC0 = R7;
LC1 = R6;
LSETUP ( start3 , end3 ) LC0;
start3: R6 += 6;
LSETUP ( start4 , end4 ) LC1;
start4: R0 += 1;
 R1 += -2;
end4: R2 += 3;
 R3 += 4;
end3: R7 += -7;
 R3 += 1;
CHECKREG r0, 0x00000037;
CHECKREG r1, 0xFFFFFFAC;
CHECKREG r2, 0x000000A8;
CHECKREG r3, 0x0000007E;
CHECKREG r4, 0x00000068;
CHECKREG r5, 0xFFFFFFB2;
CHECKREG r6, 0x000000A6;
CHECKREG r7, 0xFFFFFF70;

R0 = 0x05;
R1 = 0x10;
R2 = 0x08;
R3 = 0x0C;
R4 = 0x40 (X);
R5 = 0x50 (X);
R6 = 0x60 (X);
R7 = 0x70 (X);

LC0 = R2;
LC1 = R3;
LSETUP ( start5 , end5 ) LC0;
start5: R4 += 1;
LSETUP ( start6 , end6 ) LC1;
start6: R6 += 4;
end6: R7 += -5;
 R3 += 6;
end5: R5 += -2;
 R3 += 3;
CHECKREG r0, 0x00000005;
CHECKREG r1, 0x00000010;
CHECKREG r2, 0x00000008;
CHECKREG r3, 0x0000003F;
CHECKREG r4, 0x00000048;
CHECKREG r5, 0x00000040;
CHECKREG r6, 0x000000AC;
CHECKREG r7, 0x00000011;
LSETUP ( start7 , end7 ) LC0;
start7: R4 += 4;
end7: R5 += -5;
 R3 += 6;
CHECKREG r0, 0x00000005;
CHECKREG r1, 0x00000010;
CHECKREG r2, 0x00000008;
CHECKREG r3, 0x00000045;
CHECKREG r4, 0x0000004C;
CHECKREG r5, 0x0000003B;
CHECKREG r6, 0x000000AC;
CHECKREG r7, 0x00000011;

P1 = 12;
P2 = 14;
P3 = 16;
P4 = 18;
P5 = 12;
SP = 14;
FP = 16;

R0 = 0x05;
R1 = 0x10;
R2 = 0x14;
R3 = 0x18;
R4 = 0x16;
R5 = 0x04;
R6 = 0x30;
R7 = 0x30;

LC0 = R5;
LC1 = R4;
LSETUP ( start11 , end11 ) LC0;
start11: R0 += 1;
 R1 += -1;
LSETUP ( start15 , end15 ) LC1;
start15: R4 += 1;
end15: R5 += -1;
 R3 += 1;
end11: R2 += 1;
 R3 += 1;


LSETUP ( start13 , end13 ) LC0 = P5;
start13: R6 += 1;
LSETUP ( start12 , end12 ) LC1 = P2;
start12: R4 += 1;
end12: R5 += -1;
 R3 += 1;
end13: R7 += -1;
 R3 += 1;
CHECKREG r0, 0x00000009;
CHECKREG r1, 0x0000000C;
CHECKREG r2, 0x00000018;
CHECKREG r3, 0x0000002A;
CHECKREG r4, 0x000000D7;
CHECKREG r5, 0xFFFFFF43;
CHECKREG r6, 0x0000003C;
CHECKREG r7, 0x00000024;

R0 = 0x05;
R1 = 0x10;
R2 = 0x20;
R3 = 0x30;
R4 = 0x40 (X);
R5 = 0x50 (X);
R6 = 0x14;
R7 = 0x08;
P4 = 6;
FP = 8;

LC0 = R6;
LC1 = R7;
LSETUP ( start14 , end14 ) LC0 = P4;
start14: R0 += 1;
 R1 += -1;
LSETUP ( start16 , end16 ) LC1;
start16: R6 += 1;
end16: R7 += -1;
 R3 += 1;
LSETUP ( start17 , end17 ) LC1 = FP >> 1;
start17: R4 += 1;
end17: R5 += -1;
 R3 += 1;
end14: R2 += 1;
 R3 += 1;
CHECKREG r0, 0x0000000B;
CHECKREG r1, 0x0000000A;
CHECKREG r2, 0x00000026;
CHECKREG r3, 0x0000003D;
CHECKREG r4, 0x00000058;
CHECKREG r5, 0x00000038;
CHECKREG r6, 0x00000021;
CHECKREG r7, 0xFFFFFFFB;

pass
