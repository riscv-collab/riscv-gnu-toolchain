//Original:/testcases/core/c_dsp32shiftim_af/c_dsp32shiftim_af.dsp
# mach: bfin

.include "testutils.inc"
	start


// Spec Reference: dsp32shiftimm ashift: ashift


imm32 r0, 0xa1230001;
imm32 r1, 0x1b345678;
imm32 r2, 0x23c56789;
imm32 r3, 0x34d6789a;
imm32 r4, 0x85a789ab;
imm32 r5, 0x967c9abc;
imm32 r6, 0xa789abcd;
imm32 r7, 0xb8912cde;
R0 = R0 << 0;
R1 = R1 << 3;
R2 = R2 << 7;
R3 = R3 << 8;
R4 = R4 << 15;
R5 = R5 << 24;
R6 = R6 << 31;
R7 = R7 << 20;
CHECKREG r0, 0xA1230001;
CHECKREG r1, 0xD9A2B3C0;
CHECKREG r2, 0xE2B3C480;
CHECKREG r3, 0xD6789A00;
CHECKREG r4, 0xC4D58000;
CHECKREG r5, 0xBC000000;
CHECKREG r6, 0x80000000;
CHECKREG r7, 0xCDE00000;

imm32 r0, 0xa1230001;
imm32 r1, 0x1b345678;
imm32 r2, 0x23c56789;
imm32 r3, 0x34d6789a;
imm32 r4, 0x85a789ab;
imm32 r5, 0x967c9abc;
imm32 r6, 0xa789abcd;
imm32 r7, 0xb8912cde;
R6 = R0 >>> 1;
R7 = R1 >>> 3;
R0 = R2 >>> 7;
R1 = R3 >>> 8;
R2 = R4 >>> 15;
R3 = R5 >>> 24;
R4 = R6 >>> 31;
R5 = R7 >>> 20;
CHECKREG r0, 0x00478ACF;
CHECKREG r1, 0x0034D678;
CHECKREG r2, 0xFFFF0B4F;
CHECKREG r3, 0xFFFFFF96;
CHECKREG r4, 0xFFFFFFFF;
CHECKREG r5, 0x00000036;
CHECKREG r6, 0xD0918000;
CHECKREG r7, 0x03668ACF;



pass
