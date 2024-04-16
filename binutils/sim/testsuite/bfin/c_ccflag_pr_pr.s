//Original:/proj/frio/dv/testcases/core/c_ccflag_pr_pr/c_ccflag_pr_pr.dsp
// Spec Reference: ccflag pr-pr
# mach: bfin

.include "testutils.inc"
	start

INIT_P_REGS 0;
INIT_R_REGS 0;


//imm32 p0, 0x00110022;
imm32 p1, 0x00110022;
imm32 p2, 0x00330044;
imm32 p3, 0x00550066;

imm32 p4, 0x00770088;
imm32 p5, 0x009900aa;
imm32 fp, 0x00bb00cc;
imm32 sp, 0x00000000;

R0 = 0;
ASTAT = R0;
R4 = ASTAT;

// positive preg-1 EQUAL to positive preg-2
CC = P2 == P1;
R5 = ASTAT;
P5 = R5;
CC = P2 < P1;
R6 = ASTAT;
CC = P2 <= P1;
R7 = ASTAT;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000000;

// positive preg-1 GREATER than positive preg-2
CC = P3 == P2;
R5 = ASTAT;
CC = P3 < P2;
R6 = ASTAT;
CC = P3 <= P2;
R7 = ASTAT;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000000;
// positive preg-1 LESS than positive preg-2
CC = P2 == P3;
R5 = ASTAT;
CC = P2 < P3;
R6 = ASTAT;
CC = P2 <= P3;
R7 = ASTAT;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000020;
CHECKREG r7, 0x00000020;

//imm32 p0, 0x01230123;
imm32 p1, 0x81230123;
imm32 p2, 0x04560456;
imm32 p3, 0x87890789;
// operate on negative number
R0 = 0;
ASTAT = R0;
R4 = ASTAT;

// positive preg-1 GREATER than negative preg-2
CC = P2 == P1;
R5 = ASTAT;
CC = P2 < P1;
R6 = ASTAT;
CC = P2 <= P1;
R7 = ASTAT;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000000;

// negative preg-1 LESS    than POSITIVE preg-2  small
CC = P3 == P2;
R5 = ASTAT;
CC = P3 < P2;
R6 = ASTAT;
CC = P3 <= P2;
R7 = ASTAT;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000020;
CHECKREG r7, 0x00000020;

// negative preg-1 GREATER than negative preg-2
CC = P1 == P3;
R5 = ASTAT;
CC = P1 < P3;
R6 = ASTAT;
CC = P1 <= P3;
R7 = ASTAT;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000020;
CHECKREG r7, 0x00000020;

// negative preg-1 LESS    than negative preg-2
CC = P3 == P1;
R5 = ASTAT;
CC = P3 < P1;
R6 = ASTAT;
CC = P3 <= P1;
R7 = ASTAT;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000000;


//imm32 p0, 0x80230123;
imm32 p1, 0x00230123;
imm32 p2, 0x80560056;
imm32 p3, 0x00890089;
// operate on negative number
R0 = 0;
ASTAT = R0;
R4 = ASTAT;

// negative preg-1 LESS    than POSITIVE preg-2
CC = P2 == P3;
R5 = ASTAT;
CC = P2 < P3;
R6 = ASTAT;
CC = P2 <= P3;
R7 = ASTAT;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;  // overflow and carry but not negative
CHECKREG r6, 0x00000020;  // cc overflow, carry and negative
CHECKREG r7, 0x00000020;


imm32 p4, 0x44444444;
imm32 p5, 0x55555555;
imm32 fp, 0x66666666;
imm32 sp, 0x77777777;

//imm32 p0, 0x00000000;
imm32 p1, 0x11111111;
imm32 p2, 0x00000000;
imm32 p3, 0x33333333;

ASTAT = R0;
R3 = ASTAT;
CHECKREG r3, 0x00000000;

// positive preg-1 EQUAL to positive preg-2
CC = P4 == P5;
R0 = ASTAT;
CC = P4 < P5;
R1 = ASTAT;
CC = P4 <= P5;
R2 = ASTAT;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000020;
CHECKREG r2, 0x00000020;

// positive preg-1 GREATER than positive preg-2
CC = SP == FP;
R0 = ASTAT;
CC = SP < FP;
R1 = ASTAT;
CC = SP <= FP;
R2 = ASTAT;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000000;


// positive preg-1 LESS than positive preg-2
CC = FP == SP;
R0 = ASTAT;
CC = FP < SP;
R1 = ASTAT;
CC = FP <= SP;
R2 = ASTAT;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000020;
CHECKREG r2, 0x00000020;

imm32 p4, 0x01230123;
imm32 p5, 0x81230123;
imm32 fp, 0x04560456;
imm32 sp, 0x87890789;
// operate on negative number
R0 = 0;
ASTAT = R0;
R3 = ASTAT; // nop;
CHECKREG r3, 0x00000000;

// positive preg-1 GREATER than negative preg-2
CC = P4 == P5;
R1 = ASTAT;
CC = P4 < P5;
R2 = ASTAT;
CC = P4 <= P5;
R3 = ASTAT;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000000;

// negative preg-1 LESS    than POSITIVE preg-2  small
CC = SP == FP;
R0 = ASTAT;
CC = SP < FP;
R1 = ASTAT;
CC = SP <= FP;
R2 = ASTAT;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000020;
CHECKREG r2, 0x00000020;

// negative preg-1 GREATER than negative preg-2
CC = P5 == SP;
R0 = ASTAT;
CC = P5 < SP;
R1 = ASTAT;
CC = P5 <= SP;
R2 = ASTAT;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000020;
CHECKREG r2, 0x00000020;

// negative preg-1 LESS    than negative preg-2
CC = SP == P5;
R1 = ASTAT;
CC = SP < P5;
R2 = ASTAT;
CC = SP <= P5;
R3 = ASTAT;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000000;


imm32 p4, 0x80230123;
imm32 p5, 0x00230123;
imm32 fp, 0x80560056;
imm32 sp, 0x00890089;
// operate on negative number
P3 = 0;
ASTAT = P3;
R0 = ASTAT;

// negative preg-1 LESS    than POSITIVE preg-2
CC = R6 == R7;
R1 = ASTAT;
CC = R6 < R7;
R2 = ASTAT;
CC = R6 <= R7;
R3 = ASTAT;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00001025;  // overflow and carry but not negative
CHECKREG r2, 0x00001005;  // cc overflow, carry and negative
CHECKREG r3, 0x00001025;


pass;
