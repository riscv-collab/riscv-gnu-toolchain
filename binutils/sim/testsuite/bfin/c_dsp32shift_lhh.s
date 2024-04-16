//Original:/testcases/core/c_dsp32shift_lhh/c_dsp32shift_lhh.dsp
// Spec Reference: dsp32shift lshift/lshift
# mach: bfin

.include "testutils.inc"
	start



// lshift/lshift : = (half reg)
// d_reg = lshift/lshift (d BY d_lo)
// Rx by RLx
imm32 r0, 0x01230000;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R1 = LSHIFT R0 BY R0.L (V);
R2 = LSHIFT R1 BY R0.L (V);
R3 = LSHIFT R2 BY R0.L (V);
R4 = LSHIFT R3 BY R0.L (V);
R5 = LSHIFT R4 BY R0.L (V);
R6 = LSHIFT R5 BY R0.L (V);
R7 = LSHIFT R6 BY R0.L (V);
R0 = LSHIFT R7 BY R0.L (V);

imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R1.L = 5;
R2 = LSHIFT R0 BY R1.L (V);
R3 = LSHIFT R1 BY R1.L (V);
R4 = LSHIFT R2 BY R1.L (V);
R5 = LSHIFT R3 BY R1.L (V);
R6 = LSHIFT R4 BY R1.L (V);
R7 = LSHIFT R5 BY R1.L (V);
R0 = LSHIFT R6 BY R1.L (V);
R1 = LSHIFT R7 BY R1.L (V);

imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R2 = 15;
R3 = LSHIFT R0 BY R2.L (V);
R4 = LSHIFT R1 BY R2.L (V);
R5 = LSHIFT R2 BY R2.L (V);
R6 = LSHIFT R3 BY R2.L (V);
R7 = LSHIFT R4 BY R2.L (V);
R0 = LSHIFT R5 BY R2.L (V);
R1 = LSHIFT R6 BY R2.L (V);
R2 = LSHIFT R7 BY R2.L (V);

imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R3.L = 16;
R4 = LSHIFT R0 BY R3.L (V);
R5 = LSHIFT R1 BY R3.L (V);
R6 = LSHIFT R2 BY R3.L (V);
R7 = LSHIFT R3 BY R3.L (V);
R0 = LSHIFT R4 BY R3.L (V);
R1 = LSHIFT R5 BY R3.L (V);
R2 = LSHIFT R6 BY R3.L (V);
R3 = LSHIFT R7 BY R3.L (V);

imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R4.L = -1;
R0 = LSHIFT R0 BY R4.L (V);
R1 = LSHIFT R1 BY R4.L (V);
R2 = LSHIFT R2 BY R4.L (V);
R3 = LSHIFT R3 BY R4.L (V);
R4 = LSHIFT R4 BY R4.L (V);
R5 = LSHIFT R5 BY R4.L (V);
R6 = LSHIFT R6 BY R4.L (V);
R7 = LSHIFT R7 BY R4.L (V);

imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R5.L = -6;
R6 = LSHIFT R0 BY R5.L (V);
R7 = LSHIFT R1 BY R5.L (V);
R0 = LSHIFT R2 BY R5.L (V);
R1 = LSHIFT R3 BY R5.L (V);
R2 = LSHIFT R4 BY R5.L (V);
R3 = LSHIFT R5 BY R5.L (V);
R4 = LSHIFT R6 BY R5.L (V);
R5 = LSHIFT R7 BY R5.L (V);

imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R6.L = -15;
R7 = LSHIFT R0 BY R6.L (V);
R0 = LSHIFT R1 BY R6.L (V);
R1 = LSHIFT R2 BY R6.L (V);
R2 = LSHIFT R3 BY R6.L (V);
R3 = LSHIFT R4 BY R6.L (V);
R4 = LSHIFT R5 BY R6.L (V);
R5 = LSHIFT R6 BY R6.L (V);
R6 = LSHIFT R7 BY R6.L (V);

imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R7.L = -16;
R0 = LSHIFT R0 BY R7.L (V);
R1 = LSHIFT R1 BY R7.L (V);
R2 = LSHIFT R2 BY R7.L (V);
R3 = LSHIFT R3 BY R7.L (V);
R4 = LSHIFT R4 BY R7.L (V);
R5 = LSHIFT R5 BY R7.L (V);
R6 = LSHIFT R6 BY R7.L (V);
R7 = LSHIFT R7 BY R7.L (V);

imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R0.L = 4;
//r0 = lshift/lshift (r0 by rl0);
R1 = LSHIFT R1 BY R0.L (V);
R2 = LSHIFT R2 BY R0.L (V);
R3 = LSHIFT R3 BY R0.L (V);
R4 = LSHIFT R4 BY R0.L (V);
R5 = LSHIFT R5 BY R0.L (V);
R6 = LSHIFT R6 BY R0.L (V);
R7 = LSHIFT R7 BY R0.L (V);

imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R1.L = 6;
R0 = LSHIFT R0 BY R1.L (V);
//r1 = lshift/lshift (r1 by rl1);
R2 = LSHIFT R2 BY R1.L (V);
R3 = LSHIFT R3 BY R1.L (V);
R4 = LSHIFT R4 BY R1.L (V);
R5 = LSHIFT R5 BY R1.L (V);
R6 = LSHIFT R6 BY R1.L (V);
R7 = LSHIFT R7 BY R1.L (V);


imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R2.L = 15;
R0 = LSHIFT R0 BY R2.L (V);
R1 = LSHIFT R1 BY R2.L (V);
//r2 = lshift/lshift (r2 by rl2);
R3 = LSHIFT R3 BY R2.L (V);
R4 = LSHIFT R4 BY R2.L (V);
R5 = LSHIFT R5 BY R2.L (V);
R6 = LSHIFT R6 BY R2.L (V);
R7 = LSHIFT R7 BY R2.L (V);

imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R3.L = 16;
R0 = LSHIFT R0 BY R3.L (V);
R1 = LSHIFT R1 BY R3.L (V);
R2 = LSHIFT R2 BY R3.L (V);
//r3 = lshift/lshift (r3 by rl3);
R4 = LSHIFT R4 BY R3.L (V);
R5 = LSHIFT R5 BY R3.L (V);
R6 = LSHIFT R6 BY R3.L (V);
R7 = LSHIFT R7 BY R3.L (V);

imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R4.L = -9;
R0 = LSHIFT R0 BY R4.L (V);
R1 = LSHIFT R1 BY R4.L (V);
R2 = LSHIFT R2 BY R4.L (V);
R3 = LSHIFT R3 BY R4.L (V);
//r4 = lshift/lshift (r4 by rl4);
R5 = LSHIFT R5 BY R4.L (V);
R6 = LSHIFT R6 BY R4.L (V);
R7 = LSHIFT R7 BY R4.L (V);

imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R5.L = -14;
R0 = LSHIFT R0 BY R5.L (V);
R1 = LSHIFT R1 BY R5.L (V);
R2 = LSHIFT R2 BY R5.L (V);
R3 = LSHIFT R3 BY R5.L (V);
R4 = LSHIFT R4 BY R5.L (V);
//r5 = lshift/lshift (r5 by rl5);
R6 = LSHIFT R6 BY R5.L (V);
R7 = LSHIFT R7 BY R5.L (V);


imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R6.L = -15;
R0 = LSHIFT R0 BY R6.L (V);
R1 = LSHIFT R1 BY R6.L (V);
R2 = LSHIFT R2 BY R6.L (V);
R3 = LSHIFT R3 BY R6.L (V);
R4 = LSHIFT R4 BY R6.L (V);
R5 = LSHIFT R5 BY R6.L (V);
//r6 = lshift/lshift (r6 by rl6);
R7 = LSHIFT R7 BY R6.L (V);

imm32 r0, 0x01230002;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R7.L = -16;
R0 = LSHIFT R0 BY R7.L (V);
R1 = LSHIFT R1 BY R7.L (V);
R2 = LSHIFT R2 BY R7.L (V);
R3 = LSHIFT R3 BY R7.L (V);
R4 = LSHIFT R4 BY R7.L (V);
R5 = LSHIFT R5 BY R7.L (V);
R6 = LSHIFT R6 BY R7.L (V);
R7 = LSHIFT R7 BY R7.L (V);
CHECKREG r0, 0x00000000;
CHECKREG r1, 0x00000000;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x00000000;
CHECKREG r4, 0x00000000;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0x00000000;
CHECKREG r7, 0x00000000;
CHECKREG r7, 0x00000000;

pass
