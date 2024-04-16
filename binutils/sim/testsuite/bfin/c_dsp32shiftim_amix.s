//Original:/testcases/core/c_dsp32shiftim_amix/c_dsp32shiftim_amix.dsp
# mach: bfin

.include "testutils.inc"
	start


// Spec Reference: dsp32shiftimm ashift: mix



imm32 r4, 0x00000000;
imm32 r5, 0x00000000;
imm32 r6, 0x00000000;
imm32 r7, 0x00000000;

// Ashift : positive data, count (+)=left (half reg)
imm32 r0, 0x00010001;
imm32 r1, 1;
imm32 r2, 0x00020002;
imm32 r3, 2;
R4.H = R0.H << 1;
R4.L = R0.L << 1; /* r4 = 0x00020002 */
R5.H = R2.H << 2;
R5.L = R2.L << 2; /* r5 = 0x00080008 */
R6 = R0 << 1 (V); /* r6 = 0x00020002 */
R7 = R2 << 2 (V); /* r7 = 0x00080008 */
CHECKREG r4, 0x00020002;
CHECKREG r5, 0x00080008;
CHECKREG r6, 0x00020002;
CHECKREG r7, 0x00080008;

imm32 r1, 3;
imm32 r3, 4;
R6 = R0 << 3; /* r6 = 0x00080010 */
R7 = R2 << 4;
CHECKREG r6, 0x00080008; /* r7 = 0x00100010 */
CHECKREG r7, 0x00200020;

A0 = 0;
A0.L = R0.L;
A0.H = R0.H;
A0 = A0 << 3; /* a0 = 0x00080008 */
R5 = A0.w; /* r5 = 0x00080008 */
CHECKREG r5, 0x00080008;

imm32 r4, 0x30000003;
imm32 r1, 1;
R5 = R4 << 1; /* r5 = 0x60000006 */

imm32 r1, 2;
R6 = ASHIFT R4 BY R1.L; /* r5 = 0xc000000c like LSHIFT */
CHECKREG r5, 0x60000006;
CHECKREG r6, 0xc000000c;


// Ashift : count (-)=right (half reg)
imm32 r0, 0x10001000;
imm32 r1, -1;
imm32 r2, 0x10001000;
imm32 r3, -2;
R4.H = R0.H >>> 1;
R4.L = R0.L >>> 1; /* r4 = 0x08000800 */
R5.H = R2.H >>> 2;
R5.L = R2.L >>> 2; /* r4 = 0x04000400 */
R6 = R0 >>> 1 (V); /* r4 = 0x08000800 */
R7 = R2 >>> 2 (V); /* r4 = 0x04000400 */
CHECKREG r4, 0x08000800;
CHECKREG r5, 0x04000400;
CHECKREG r6, 0x08000800;
CHECKREG r7, 0x04000400;

// Ashift : (full reg)
imm32 r1, -3;
imm32 r3, -4;
R6 = R0 >>> 3; /* r6 = 0x02000200 */
R7 = R2 >>> 4; /* r7 = 0x01000100 */
CHECKREG r6, 0x02000200;
CHECKREG r7, 0x01000100;

// NEGATIVE
// Ashift : NEGATIVE data, count (+)=left (half reg)
imm32 r0, 0xc00f800f;
imm32 r1, 1;
imm32 r2, 0xe00fe00f;
imm32 r3, 2;
R4.H = R0.H << 1;
R4.L = R0.L << 1 (S); /* r4 = 0x801e801e */
R5.H = R2.H << 2;
R5.L = R2.L << 2; /* r4 = 0x803c803c */
CHECKREG r4, 0x801e8000;
CHECKREG r5, 0x803c803c;

imm32 r0, 0xc80fe00f;
imm32 r2, 0xe40fe00f;
imm32 r1, 4;
imm32 r3, 5;
R6 = R0 << 4; /* r6 = 0x80fe00f0 */
R7 = R2 << 5; /* r7 = 0x81fc01e0 */
CHECKREG r6, 0x80fe00f0;
CHECKREG r7, 0x81fc01e0;

imm32 r0, 0xf80fe00f;
imm32 r2, 0xfc0fe00f;
R6 = R0 << 4 (S); /* r6 = 0x80fe00f0 */
R7 = R2 << 5 (S); /* r7 = 0x81fc01e0 */
CHECKREG r6, 0x80fe00f0;
CHECKREG r7, 0x81fc01e0;

imm32 r0, 0xc80fe00f;
imm32 r2, 0xe40fe00f;
R6 = R0 << 4 (S); /* r6 = 0x80000000 zero bubble tru MSB */
R7 = R2 << 5 (S); /* r7 = 0x80000000 */
CHECKREG r6, 0x80000000;
CHECKREG r7, 0x80000000;

imm32 r0, 0xFFFFFFF4;
imm32 r2, 0xFFF00001;
R6 = R0 << 31 (S); /* r6 = 0x80000000 */
R7 = R2 << 31 (S); /* r7 = 0x80000000 */
CHECKREG r6, 0x80000000;
CHECKREG r7, 0x80000000;


// Ashift : NEGATIVE data, count (-)=right (half reg) Working ok
imm32 r0, 0x80f080f0;
imm32 r1, -1;
imm32 r2, 0x80f080f0;
imm32 r3, -2;
R4.H = R0.H >>> 1;
R4.L = R0.L >>> 1; /* r4 = 0xc078c078 */
R5.H = R2.H >>> 2;
R5.L = R2.L >>> 2; /* r4 = 0xe03ce03c */
CHECKREG r4, 0xc078c078;
CHECKREG r5, 0xe03ce03c;
R6 = R0 >>> 1 (V); /* r6 = 0xc078c078 */
R7 = R2 >>> 2 (V); /* r7 = 0xe03ce03c */
CHECKREG r6, 0xc078c078;
CHECKREG r7, 0xe03ce03c;

imm32 r1, -3;
imm32 r3, -4;
R6 = R0 >>> 3; /* r6 = 0xf01e101e */
R7 = R2 >>> 4; /* r7 = 0xf80f080f */
CHECKREG r6, 0xf01e101e;
CHECKREG r7, 0xf80f080f;


pass
