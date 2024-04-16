//Original:/testcases/core/c_loopsetup_preg_lc0/c_loopsetup_preg_lc0.dsp
// Spec Reference: loopsetup preg lc0
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
end1: R2 += 3;
 R3 += 4;
LSETUP ( start2 , end2 ) LC0 = P2;
start2: R4 += 4;
end2: R5 += -5;
 R3 += 1;
LSETUP ( start3 , end3 ) LC0 = P3;
start3: R6 += 6;
end3: R7 += -7;
 R3 += 1;
CHECKREG r0, 0x00000008;
CHECKREG r1, 0x0000000A;
CHECKREG r2, 0x00000029;
CHECKREG r3, 0x00000036;
CHECKREG r4, 0x00000050;
CHECKREG r5, 0x0000003C;
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
LSETUP ( start4 , end4 ) LC0 = P4;
start4: R0 += 1;
 R1 += -2;
end4: R2 += 3;
 R3 += 4;
LSETUP ( start5 , end5 ) LC0 = P5;
start5: R4 += 1;
end5: R5 += -2;
 R3 += 3;
LSETUP ( start6 , end6 ) LC0 = SP;
start6: R6 += 4;
end6: R7 += -5;
 R3 += 6;
CHECKREG r0, 0x0000000B;
CHECKREG r1, 0x00000004;
CHECKREG r2, 0x00000032;
CHECKREG r3, 0x0000003D;
CHECKREG r4, 0x00000047;
CHECKREG r5, 0x00000042;
CHECKREG r6, 0x00000080;
CHECKREG r7, 0x00000048;
LSETUP ( start7 , end7 ) LC0 = FP;
start7: R4 += 4;
end7: R5 += -5;
 R3 += 6;
CHECKREG r0, 0x0000000B;
CHECKREG r1, 0x00000004;
CHECKREG r2, 0x00000032;
CHECKREG r3, 0x00000043;
CHECKREG r4, 0x0000006B;
CHECKREG r5, 0x00000015;
CHECKREG r6, 0x00000080;
CHECKREG r7, 0x00000048;


pass
