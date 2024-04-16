//Original:/testcases/core/c_loopsetup_overlap/c_loopsetup_overlap.dsp
// Spec Reference: loopsetup overlap
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
start3: R6 += 6;
LSETUP ( start4 , end4 ) LC0 = P4 >> 1;
start4: R0 += 1;
 R1 += -2;
end3: R2 += 3;
 R3 += 4;
end4: R7 += -7;
 R3 += 1;
CHECKREG r0, 0x0000000F;
CHECKREG r1, 0xFFFFFFFC;
CHECKREG r2, 0x0000003E;
CHECKREG r3, 0x00000044;
CHECKREG r4, 0x00000070;
CHECKREG r5, 0x00000014;
CHECKREG r6, 0x0000007E;
CHECKREG r7, 0x0000005B;

R0 = 0x05;
R1 = 0x10;
R2 = 0x20;
R3 = 0x30;
R4 = 0x40 (X);
R5 = 0x50 (X);
R6 = 0x60 (X);
R7 = 0x70 (X);
LSETUP ( start5 , end5 ) LC0 = P5;
start5: R4 += 1;
LSETUP ( start6 , end6 ) LC1 = SP >> 1;
start6: R6 += 4;
end5: R7 += -5;
 R3 += 6;
end6: R5 += -2;
 R3 += 3;
CHECKREG r0, 0x00000005;
CHECKREG r1, 0x00000010;
CHECKREG r2, 0x00000020;
CHECKREG r3, 0x0000004B;
CHECKREG r4, 0x00000047;
CHECKREG r5, 0x00000048;
CHECKREG r6, 0x00000088;
CHECKREG r7, 0x0000003E;
LSETUP ( start7 , end7 ) LC0 = FP;
start7: R4 += 4;
end7: R5 += -5;
 R3 += 6;
CHECKREG r0, 0x00000005;
CHECKREG r1, 0x00000010;
CHECKREG r2, 0x00000020;
CHECKREG r3, 0x00000051;
CHECKREG r4, 0x0000006B;
CHECKREG r5, 0x0000001B;
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
LSETUP ( start11 , end11 ) LC1 = P1;
start11: R0 += 1;
 R1 += -1;
LSETUP ( start15 , end15 ) LC0 = P5;
start15: R4 += 5;
end11: R5 += -14;
 R3 += 1;
end15: R2 += 17;
 R3 += 12;
LSETUP ( start13 , end13 ) LC1 = P3;
start13: R6 += 1;
LSETUP ( start12 , end12 ) LC0 = P2;
start12: R4 += 22;
end13: R5 += -11;
 R3 += 13;
end12: R7 += -1;
 R3 += 14;
CHECKREG r0, 0x0000000D;
CHECKREG r1, 0x00000008;
CHECKREG r2, 0x00000130;
CHECKREG r3, 0x000000DC;
CHECKREG r4, 0x00000281;
CHECKREG r5, 0xFFFFFE27;
CHECKREG r6, 0x0000006C;
CHECKREG r7, 0x00000066;

R0 = 0x05;
R1 = 0x10;
R2 = 0x20;
R3 = 0x30;
R4 = 0x40 (X);
R5 = 0x50 (X);
R6 = 0x60 (X);
R7 = 0x70 (X);
LSETUP ( start14 , end14 ) LC0 = P4;
start14: R0 += 21;
 R1 += -11;
LSETUP ( start16 , end16 ) LC1 = SP;
start16: R6 += 10;
end16: R7 += -12;
 R3 += 1;
LSETUP ( start17 , end17 ) LC1 = FP >> 1;
start17: R4 += 31;
end14: R5 += -1;
 R3 += 11;
end17: R2 += 41;
 R3 += 1;
CHECKREG r0, 0x0000012B;
CHECKREG r1, 0xFFFFFF76;
CHECKREG r2, 0x000001BA;
CHECKREG r3, 0x000000AD;
CHECKREG r4, 0x00000309;
CHECKREG r5, 0x00000039;
CHECKREG r6, 0x00000A38;
CHECKREG r7, 0xFFFFF4A0;

pass
