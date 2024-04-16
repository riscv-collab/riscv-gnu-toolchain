//Original:/testcases/core/c_loopsetup_nested_top/c_loopsetup_nested_top.dsp
// Spec Reference: loopsetup nested top
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
LSETUP ( start1 , end1 ) LC0 = P1;
start1: R0 += 1;
 R1 += -2;
LSETUP ( start2 , end2 ) LC1 = P2;
start2: R4 += 4;
end2: R5 += -5;
 R3 += 1;
end1: R2 += 3;
 R3 += 4;
LSETUP ( start3 , end3 ) LC1 = P3;
LSETUP ( start3 , end4 ) LC0 = P4;
start3: R6 += 6;
 R0 += 1;
 R1 += -2;
end4: R2 += 3;
 R3 += 4;
end3: R7 += -7;
 R3 += 1;
CHECKREG r0, 0x00000012;
CHECKREG r1, 0xFFFFFFF6;
CHECKREG r2, 0x00000047;
CHECKREG r3, 0x0000004C;
CHECKREG r4, 0x00000070;
CHECKREG r5, 0x00000014;
CHECKREG r6, 0x0000009C;
CHECKREG r7, 0x0000004D;

R0 = 0x05;
R1 = 0x10;
R2 = 0x20;
R3 = 0x30;
R4 = 0x40 (X);
R5 = 0x50 (X);
R6 = 0x60 (X);
R7 = 0x70 (X);
LSETUP ( start5 , end5 ) LC0 = P5;
LSETUP ( start5 , end6 ) LC1 = SP >> 1;
start5: R4 += 1;
 R6 += 4;
end6: R7 += -5;
 R3 += 6;
end5: R5 += -2;
 R3 += 3;
CHECKREG r0, 0x00000005;
CHECKREG r1, 0x00000010;
CHECKREG r2, 0x00000020;
CHECKREG r3, 0x0000005D;
CHECKREG r4, 0x0000004A;
CHECKREG r5, 0x00000042;
CHECKREG r6, 0x00000088;
CHECKREG r7, 0x0000003E;
LSETUP ( start7 , end7 ) LC0 = FP;
start7: R4 += 4;
end7: R5 += -5;
 R3 += 6;
CHECKREG r0, 0x00000005;
CHECKREG r1, 0x00000010;
CHECKREG r2, 0x00000020;
CHECKREG r3, 0x00000063;
CHECKREG r4, 0x0000006E;
CHECKREG r5, 0x00000015;
CHECKREG r6, 0x00000088;
CHECKREG r7, 0x0000003E;

P1 = 8;
P2 = 10;
P3 = 12;
P4 = 14;
P5 = 16;
SP = 18;
FP = 20;

R0 = 0x05;
R1 = 0x10;
R2 = 0x20;
R3 = 0x30;
R4 = 0x40 (X);
R5 = 0x50 (X);
R6 = 0x60 (X);
R7 = 0x70 (X);
LSETUP ( start11 , end11 ) LC1 = P1 >> 1;
LSETUP ( start11 , end15 ) LC0 = P5;
start11: R0 += 1;
 R1 += -1;
 R4 += 1;
end15: R5 += -1;
 R3 += 1;
end11: R2 += 1;
 R3 += 1;
LSETUP ( start12 , end12 ) LC1 = P3 >> 1;
LSETUP ( start12 , end13 ) LC0 = P2 >> 1;
start12: R6 += 1;
 R4 += 1;
end13: R5 += -1;
 R3 += 1;
end12: R7 += -1;
 R3 += 1;
CHECKREG r0, 0x00000018;
CHECKREG r1, 0xFFFFFFFD;
CHECKREG r2, 0x00000024;
CHECKREG r3, 0x0000003C;
CHECKREG r4, 0x0000005D;
CHECKREG r5, 0x00000033;
CHECKREG r6, 0x0000006A;
CHECKREG r7, 0x0000006A;

R0 = 0x04;
R1 = 0x06;
R2 = 0x08;
R3 = 0x10;
R4 = 0x12;
R5 = 0x14;
R6 = 0x16;
R7 = 0x18;
LSETUP ( start14 , end14 ) LC0 = P4;
LSETUP ( start14 , end16 ) LC1 = SP >> 1;
start14: R0 += 1;
 R1 += -1;
 R6 += 1;
end16: R7 += -1;
 R3 += 1;
LSETUP ( start17 , end17 ) LC1 = FP >> 1;
start17: R4 += 1;
end17: R5 += -1;
 R3 += 1;
end14: R2 += 1;
 R3 += 1;
CHECKREG r0, 0x0000001A;
CHECKREG r1, 0xFFFFFFF0;
CHECKREG r2, 0x00000016;
CHECKREG r3, 0x0000002D;
CHECKREG r4, 0x0000009E;
CHECKREG r5, 0xFFFFFF88;
CHECKREG r6, 0x0000002C;
CHECKREG r7, 0x00000002;

pass
