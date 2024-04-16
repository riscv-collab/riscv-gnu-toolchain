//Original:/proj/frio/dv/testcases/core/c_ccflag_dr_dr_uu/c_ccflag_dr_dr_uu.dsp
// Spec Reference: ccflags dr-dr_uu
# mach: bfin

.include "testutils.inc"
	start


imm32 r0, 0x00110022;
imm32 r1, 0x00110022;
imm32 r2, 0x00330044;
imm32 r3, 0x00550066;

imm32 r4, 0x00770088;
imm32 r5, 0x009900aa;
imm32 r6, 0x00bb00cc;
imm32 r7, 0x00000000;

ASTAT = R7;
R4 = ASTAT;

// positive dreg-1 EQUAL to positive dreg-2
CC = R0 == R1;
R5 = ASTAT;
CC = R0 < R1 (IU);
R6 = ASTAT;
CC = R0 <= R1 (IU);
R7 = ASTAT;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00001025;
CHECKREG r6, 0x00001005;
CHECKREG r7, 0x00001025;
CC = R0 < R1 (IU);
R4 = ASTAT;
CC = R0 <= R1 (IU);
R5 = ASTAT;
CHECKREG r4, 0x00001005;
CHECKREG r5, 0x00001025;

// positive dreg-1 GREATER than positive dreg-2
CC = R3 == R2;
R5 = ASTAT;
CC = R3 < R2 (IU);
R6 = ASTAT;
CC = R3 <= R2 (IU);
R7 = ASTAT;
CHECKREG r5, 0x00001004;
CHECKREG r6, 0x00001004;
CHECKREG r7, 0x00001004;
CC = R3 < R2 (IU);
R4 = ASTAT;
CC = R3 <= R2 (IU);
R5 = ASTAT;
CHECKREG r4, 0x00001004;
CHECKREG r5, 0x00001004;


// positive dreg-1 LESS than positive dreg-2
CC = R2 == R3;
R5 = ASTAT;
CC = R2 < R3 (IU);
R6 = ASTAT;
CC = R2 <= R3 (IU);
R7 = ASTAT;
CHECKREG r5, 0x00000002;
CHECKREG r6, 0x00000022;
CHECKREG r7, 0x00000022;
CC = R2 < R3 (IU);
R4 = ASTAT;
CC = R2 <= R3 (IU);
R5 = ASTAT;
CHECKREG r4, 0x00000022;
CHECKREG r5, 0x00000022;

imm32 r0, 0x01230123;
imm32 r1, 0x81230123;
imm32 r2, 0x04560456;
imm32 r3, 0x87890789;
// operate on negative number
R7 = 0;
ASTAT = R7;
R4 = ASTAT;

// positive dreg-1 GREATER than negative dreg-2
CC = R0 == R1;
R5 = ASTAT;
CC = R0 < R1 (IU);
R6 = ASTAT;
CC = R0 <= R1 (IU);
R7 = ASTAT;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000022;
CHECKREG r7, 0x00000022;

// negative dreg-1 LESS    than POSITIVE dreg-2  small
CC = R3 == R2;
R5 = ASTAT;
CC = R3 < R2 (IU);
R6 = ASTAT;
CC = R3 <= R2 (IU);
R7 = ASTAT;
CHECKREG r5, 0x00001006;
CHECKREG r6, 0x00001004;
CHECKREG r7, 0x00001004;

// negative dreg-1 GREATER than negative dreg-2
CC = R1 == R3;
R5 = ASTAT;
CC = R1 < R3 (IU);
R6 = ASTAT;
CC = R1 <= R3 (IU);
R7 = ASTAT;
CHECKREG r5, 0x00000002;
CHECKREG r6, 0x00000022;
CHECKREG r7, 0x00000022;

// negative dreg-1 LESS    than negative dreg-2
CC = R3 == R1;
R5 = ASTAT;
CC = R3 < R1 (IU);
R6 = ASTAT;
CC = R3 <= R1 (IU);
R7 = ASTAT;
CHECKREG r5, 0x00001004;
CHECKREG r6, 0x00001004;
CHECKREG r7, 0x00001004;


imm32 r0, 0x80230123;
imm32 r1, 0x00230123;
imm32 r2, 0x80560056;
imm32 r3, 0x00890089;
// operate on negative number
R7 = 0;
ASTAT = R7;
R4 = ASTAT;

// negative dreg-1 LESS    than POSITIVE dreg-2
CC = R2 == R3;
R5 = ASTAT;
CC = R2 < R3 (IU);
R6 = ASTAT;
CC = R2 <= R3 (IU);
R7 = ASTAT;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00001006;  // overflow and carry but not negative
CHECKREG r6, 0x00001004;  // cc overflow, carry and negative
CHECKREG r7, 0x00001004;


imm32 r4, 0x44444444;
imm32 r5, 0x55555555;
imm32 r6, 0x66666666;
imm32 r7, 0x77777777;

imm32 r0, 0x00000000;
imm32 r1, 0x11111111;
imm32 r2, 0x22222222;
imm32 r3, 0x33333333;

ASTAT = R0;
R3 = ASTAT;
NOP;
CHECKREG r3, 0x00000000;

// positive dreg-1 EQUAL to positive dreg-2
CC = R4 == R5;
R0 = ASTAT;
CC = R4 < R5 (IU);
R1 = ASTAT;
CC = R4 <= R5 (IU);
R2 = ASTAT;
CC = R4 < R5 (IU);
R3 = ASTAT;
CHECKREG r0, 0x00000002;
CHECKREG r1, 0x00000022;
CHECKREG r2, 0x00000022;
CHECKREG r3, 0x00000022;
CC = R4 <= R5 (IU);
R0 = ASTAT;
NOP;
CHECKREG r0, 0x00000022;

// positive dreg-1 GREATER than positive dreg-2
CC = R7 == R6;
R0 = ASTAT;
CC = R7 < R6 (IU);
R1 = ASTAT;
CC = R7 <= R6 (IU);
R2 = ASTAT;
CC = R7 < R6 (IU);
R3 = ASTAT;
CHECKREG r0, 0x00001004;
CHECKREG r1, 0x00001004;
CHECKREG r2, 0x00001004;
CHECKREG r3, 0x00001004;
CC = R7 <= R6 (IU);
R0 = ASTAT;
NOP;
CHECKREG r0, 0x00001004;


// positive dreg-1 LESS than positive dreg-2
CC = R6 == R7;
R0 = ASTAT;
CC = R6 < R7 (IU);
R1 = ASTAT;
CC = R6 <= R7 (IU);
R2 = ASTAT;
CC = R6 < R7 (IU);
R3 = ASTAT;
CHECKREG r0, 0x00000002;
CHECKREG r1, 0x00000022;
CHECKREG r2, 0x00000022;
CHECKREG r3, 0x00000022;
CC = R6 <= R7 (IU);
R0 = ASTAT;
NOP;
CHECKREG r0, 0x00000022;

imm32 r4, 0x01230123;
imm32 r5, 0x81230123;
imm32 r6, 0x04560456;
imm32 r7, 0x87890789;
// operate on negative number
R0 = 0;
ASTAT = R0;
R3 = ASTAT;
CHECKREG r3, 0x00000000;

// positive dreg-1 GREATER than negative dreg-2
CC = R4 == R5;
R1 = ASTAT;
CC = R4 < R5 (IU);
R2 = ASTAT;
CC = R4 <= R5 (IU);
R3 = ASTAT;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000022;
CHECKREG r3, 0x00000022;

// negative dreg-1 LESS    than POSITIVE dreg-2  small
CC = R7 == R6;
R0 = ASTAT;
CC = R7 < R6 (IU);
R1 = ASTAT;
CC = R7 <= R6 (IU);
R2 = ASTAT;
CHECKREG r0, 0x00001006;
CHECKREG r1, 0x00001004;
CHECKREG r2, 0x00001004;

// negative dreg-1 GREATER than negative dreg-2
CC = R5 == R7;
R0 = ASTAT;
CC = R5 < R7 (IU);
R1 = ASTAT;
CC = R5 <= R7 (IU);
R2 = ASTAT;
CHECKREG r0, 0x00000002;
CHECKREG r1, 0x00000022;
CHECKREG r2, 0x00000022;

// negative dreg-1 LESS    than negative dreg-2
CC = R7 == R5;
R1 = ASTAT;
CC = R7 < R5 (IU);
R2 = ASTAT;
CC = R7 <= R5 (IU);
R3 = ASTAT;
CHECKREG r1, 0x00001004;
CHECKREG r2, 0x00001004;
CHECKREG r3, 0x00001004;


imm32 r4, 0x80230123;
imm32 r5, 0x00230123;
imm32 r6, 0x80560056;
imm32 r7, 0x00890089;
// operate on negative number
R3 = 0;
ASTAT = R3;
R0 = ASTAT;

// negative dreg-1 LESS    than POSITIVE dreg-2
CC = R6 == R7;
R1 = ASTAT;
CC = R6 < R7 (IU);
R2 = ASTAT;
CC = R6 <= R7 (IU);
R3 = ASTAT;
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00001006;  // overflow and carry but not negative
CHECKREG r2, 0x00001004;  // cc overflow, carry and negative
CHECKREG r3, 0x00001004;


pass;
