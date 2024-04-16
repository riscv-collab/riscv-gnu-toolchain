//Original:/testcases/core/c_loopsetup_preg_lc1/c_loopsetup_preg_lc1.dsp
// Spec Reference: loopsetup preg lc1
# mach: bfin

.include "testutils.inc"
	start


INIT_R_REGS 0;

ASTAT = r0;

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
R6 = 0x60 (X);
R7 = 0x70 (X);
LSETUP ( start11 , end11 ) LC1 = P1;
start11: R0 += 1;
 R1 += -1;
end11: R2 += 1;
 R3 += 1;
LSETUP ( start12 , end12 ) LC1 = P2;
start12: R4 += 1;
end12: R5 += -1;
 R3 += 1;
LSETUP ( start13 , end13 ) LC1 = P3;
start13: R6 += 1;
end13: R7 += -1;
 R3 += 1;
CHECKREG r0, 0x00000011;
CHECKREG r1, 0x00000004;
CHECKREG r2, 0x0000002C;
CHECKREG r3, 0x00000033;
CHECKREG r4, 0x0000004E;
CHECKREG r5, 0x00000042;
CHECKREG r6, 0x00000070;
CHECKREG r7, 0x00000060;

R0 = 0x05;
R1 = 0x10;
R2 = 0x20;
R3 = 0x30;
R4 = 0x40 (X);
R5 = 0x50 (X);
R6 = 0x60 (X);
R7 = 0x70 (X);
LSETUP ( start14 , end14 ) LC1 = P4;
start14: R0 += 1;
 R1 += -1;
end14: R2 += 1;
 R3 += 1;
LSETUP ( start15 , end15 ) LC1 = P5;
start15: R4 += 1;
end15: R5 += -1;
 R3 += 1;
LSETUP ( start16 , end16 ) LC1 = SP;
start16: R6 += 1;
end16: R7 += -1;
 R3 += 1;
CHECKREG r0, 0x00000017;
CHECKREG r1, 0xFFFFFFFE;
CHECKREG r2, 0x00000032;
CHECKREG r3, 0x00000033;
CHECKREG r4, 0x00000054;
CHECKREG r5, 0x0000003c;
CHECKREG r6, 0x00000076;
CHECKREG r7, 0x0000005A;
LSETUP ( start17 , end17 ) LC1 = FP;
start17: R4 += 1;
end17: R5 += -1;
 R3 += 1;
CHECKREG r0, 0x00000017;
CHECKREG r1, 0xFFFFFFFE;
CHECKREG r2, 0x00000032;
CHECKREG r3, 0x00000034;
CHECKREG r4, 0x0000006c;
CHECKREG r5, 0x00000024;
CHECKREG r6, 0x00000076;
CHECKREG r7, 0x0000005A;

pass
