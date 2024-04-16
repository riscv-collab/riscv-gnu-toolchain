//Original:/testcases/core/c_loopsetup_nested_bot/c_loopsetup_nested_bot.dsp
// Spec Reference: loopsetup nested same bottom
# mach: bfin

.include "testutils.inc"
	start


INIT_R_REGS 0;
ASTAT = r0;

//p0 = 2;
P1 = 2;
P2 = 4;
P3 = 6;
P4 = 8;
P5 = 10;
SP = 12;
FP = 14;

R0 = 0x05;
R1 = 0x10;
R2 = 0x20;
R3 = 0x32;
R4 = 0x46 (X);
R5 = 0x50 (X);
R6 = 0x68 (X);
R7 = 0x72 (X);
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
start3: R6 += 6;
LSETUP ( start4 , end3 ) LC0 = P4 >> 1;
start4: R0 += 1;
 R1 += -2;
end4: R2 += 3;
 R3 += 4;
end3: R7 += -7;
 R3 += 1;
CHECKREG r0, 0x00000010;
CHECKREG r1, 0xFFFFFFFA;
CHECKREG r2, 0x00000041;
CHECKREG r3, 0x0000005D;
CHECKREG r4, 0x00000066;
CHECKREG r5, 0x00000028;
CHECKREG r6, 0x0000008C;
CHECKREG r7, 0x00000033;

R0 = 0x05;
R1 = 0x10;
R2 = 0x14;
R3 = 0x18;
R4 = 0x20;
R5 = 0x12;
R6 = 0x24;
R7 = 0x16;
LSETUP ( start5 , end5 ) LC0 = P5;
start5: R4 += 1;
LSETUP ( start6 , end5 ) LC1 = SP >> 1;
start6: R6 += 4;
end6: R7 += -5;
 R3 += 6;
end5: R5 += -2;
 R3 += 3;
CHECKREG r0, 0x00000005;
CHECKREG r1, 0x00000010;
CHECKREG r2, 0x00000014;
CHECKREG r3, 0x00000183;
CHECKREG r4, 0x0000002A;
CHECKREG r5, 0xFFFFFF9A;
CHECKREG r6, 0x00000114;
CHECKREG r7, 0xFFFFFEEA;
LSETUP ( start7 , end7 ) LC0 = FP;
start7: R4 += 4;
end7: R5 += -5;
 R3 += 6;
CHECKREG r0, 0x00000005;
CHECKREG r1, 0x00000010;
CHECKREG r2, 0x00000014;
CHECKREG r3, 0x00000189;
CHECKREG r4, 0x00000062;
CHECKREG r5, 0xFFFFFF54;
CHECKREG r6, 0x00000114;
CHECKREG r7, 0xFFFFFEEA;

P1 = 04;
P2 = 08;
P3 = 10;
P4 = 12;
P5 = 14;
SP = 16;
FP = 18;

R0 = 0x05;
R1 = 0x10;
R2 = 0x12;
R3 = 0x20;
R4 = 0x18;
R5 = 0x14;
R6 = 0x16;
R7 = 0x28;
LSETUP ( start11 , end11 ) LC0 = P5;
start11: R0 += 1;
 R1 += -1;
LSETUP ( start15 , end15 ) LC1 = P1;
start15: R4 += 1;
end15: R5 += -1;
 R3 += 1;
end11: R2 += 1;
 R3 += 1;
LSETUP ( start13 , end12 ) LC0 = P2;
start13: R6 += 1;
LSETUP ( start12 , end12 ) LC1 = P3;
start12: R4 += 1;
end12: R5 += -1;
 R3 += 1;
end13: R7 += -1;
 R3 += 1;
CHECKREG r0, 0x00000013;
CHECKREG r1, 0x00000002;
CHECKREG r2, 0x00000020;
CHECKREG r3, 0x00000031;
CHECKREG r4, 0x0000005A;
CHECKREG r5, 0xFFFFFFD2;
CHECKREG r6, 0x00000017;
CHECKREG r7, 0x00000027;

R0 = 0x05;
R1 = 0x08;
R2 = 0x12;
R3 = 0x24;
R4 = 0x18;
R5 = 0x20;
R6 = 0x32;
R7 = 0x46 (X);
LSETUP ( start14 , end14 ) LC0 = P4;
start14: R0 += 1;
 R1 += -1;
LSETUP ( start16 , end16 ) LC1 = SP;
start16: R6 += 1;
end16: R7 += -1;
 R3 += 1;
LSETUP ( start17 , end14 ) LC1 = FP >> 1;
start17: R4 += 1;
end17: R5 += -1;
 R3 += 1;
end14: R2 += 1;
 R3 += 1;
CHECKREG r0, 0x00000011;
CHECKREG r1, 0xFFFFFFFC;
CHECKREG r2, 0x0000007E;
CHECKREG r3, 0x0000009D;
CHECKREG r4, 0x00000084;
CHECKREG r5, 0xFFFFFFB4;
CHECKREG r6, 0x000000F2;
CHECKREG r7, 0xFFFFFF86;

pass
