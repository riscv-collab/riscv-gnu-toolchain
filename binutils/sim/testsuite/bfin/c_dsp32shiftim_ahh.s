//Original:/testcases/core/c_dsp32shiftim_ahh/c_dsp32shiftim_ahh.dsp
# mach: bfin

.include "testutils.inc"
	start


// Spec Reference: dsp32shiftimm ashift: ashift / ashift



imm32 r0, 0x01230abc;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R0 = R0 << 0 (V);
R1 = R1 << 3 (V);
R2 = R2 << 5 (V);
R3 = R3 << 8 (V);
R4 = R4 << 9 (V);
R5 = R5 << 15 (V);
R6 = R6 << 7 (V);
R7 = R7 << 13 (V);
CHECKREG r0, 0x01230ABC;
CHECKREG r1, 0x91A0B3C0;
CHECKREG r2, 0x68A0F120;
CHECKREG r3, 0x56009A00;
CHECKREG r4, 0xCE005600;
CHECKREG r5, 0x00000000;
CHECKREG r6, 0xC480E680;
CHECKREG r7, 0x4000C000;

imm32 r0, 0x01230000;
imm32 r1, 0x12345678;
imm32 r2, 0x23456789;
imm32 r3, 0x3456789a;
imm32 r4, 0x456789ab;
imm32 r5, 0x56789abc;
imm32 r6, 0x6789abcd;
imm32 r7, 0x789abcde;
R7 = R0 >>> 1 (V);
R0 = R1 >>> 8 (V);
R1 = R2 >>> 14 (V);
R2 = R3 >>> 15 (V);
R3 = R4 >>> 11 (V);
R4 = R5 >>> 4 (V);
R5 = R6 >>> 9 (V);
R6 = R7 >>> 6 (V);
CHECKREG r0, 0x00120056;
CHECKREG r1, 0x00000001;
CHECKREG r2, 0x00000000;
CHECKREG r3, 0x0008FFF1;
CHECKREG r4, 0x0567F9AB;
CHECKREG r5, 0x0033FFD5;
CHECKREG r6, 0x00020000;
CHECKREG r7, 0x00910000;




pass
