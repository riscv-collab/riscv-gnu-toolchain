//Original:/testcases/core/c_dsp32shiftim_ahh_s/c_dsp32shiftim_ahh_s.dsp
# mach: bfin

.include "testutils.inc"
	start


// Spec Reference: dsp32shiftimm ashift: ashift / ashift saturated



imm32 r0, 0x01230abc;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R0 = R0 << 0 (V , S);
R1 = R1 << 3 (V , S);
R2 = R2 << 5 (V , S);
R3 = R3 << 8 (V , S);
R4 = R4 << 9 (V , S);
R5 = R5 << 15 (V , S);
R6 = R6 << 7 (V , S);
R7 = R7 << 13 (V , S);
CHECKREG r0, 0x01230ABC;
CHECKREG r1, 0x7FFF7FFF;
CHECKREG r2, 0x7FFF7FFF;
CHECKREG r3, 0x7FFF7FFF;
CHECKREG r4, 0x7FFF8000;
CHECKREG r5, 0x7FFF8000;
CHECKREG r6, 0x7FFF8000;
CHECKREG r7, 0x7FFF8000;

imm32 r0, 0x01230000;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R7 = R0 >>> 1 (V, S);
R0 = R1 >>> 8 (V, S);
R1 = R2 >>> 14 (V, S);
R2 = R3 >>> 15 (V, S);
R3 = R4 >>> 11 (V, S);
R4 = R5 >>> 4 (V, S);
R5 = R6 >>> 9 (V, S);
R6 = R7 >>> 6 (V, S);
CHECKREG r0, 0x00120056;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x0008FFF1;
CHECKREG r4, 0x0567F9AB;
CHECKREG r5, 0x0033FFD5;
CHECKREG r6, 0x00020000;
CHECKREG r7, 0x00910000;




pass
