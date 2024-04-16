//Original:/proj/frio/dv/testcases/core/c_dsp32shift_a0alr/c_dsp32shift_a0alr.dsp
// Spec Reference: dsp32shift a0 ashift, lshift, rot
# mach: bfin

.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;

	imm32 r0, 0x11140000;
	imm32 r1, 0x012C003E;
	imm32 r2, 0x81359E24;
	imm32 r3, 0x81459E24;
	imm32 r4, 0xD159E268;
	imm32 r5, 0x51626AF2;
	imm32 r6, 0x9176AF36;
	imm32 r7, 0xE18BFF86;

	R0.L = 0;
	A0 = 0;
	A0.L = R1.L;
	A0.H = R1.H;
	A0 = ASHIFT A0 BY R0.L;	/* a0 = 0x00000000 */
	R2 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r2, 0x012C003E;

	R1.L = 1;
	A0.L = R2.L;
	A0.H = R2.H;
	A0 = ASHIFT A0 BY R1.L;	/* a0 = 0x00000000 */
	R3 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r3, 0x0258007C;

	R2.L = 15;
	A0.L = R3.L;
	A0.H = R3.H;
	A0 = ASHIFT A0 BY R2.L;	/* a0 = 0x00000000 */
	R4 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r4, 0x003E0000;

	R3.L = 31;
	A0.L = R4.L;
	A0.H = R4.H;
	A0 = ASHIFT A0 BY R3.L;	/* a0 = 0x00000000 */
	R5 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r5, 0x00000000;

	R4.L = -1;
	A0.L = R5.L;
	A0.H = R5.H;
	A0 = ASHIFT A0 BY R4.L;	/* a0 = 0x00000000 */
	R6 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r6, 0x00000000;

	R5.L = -16;
	A0 = 0;
	A0.L = R6.L;
	A0.H = R6.H;
	A0 = ASHIFT A0 BY R5.L;	/* a0 = 0x00000000 */
	R7 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r7, 0x00000000;

	R6.L = -31;
	A0.L = R7.L;
	A0.H = R7.H;
	A0 = ASHIFT A0 BY R6.L;	/* a0 = 0x00000000 */
	R0 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r0, 0x00000000;

	R7.L = -32;
	A0.L = R0.L;
	A0.H = R0.H;
	A0 = ASHIFT A0 BY R7.L;	/* a0 = 0x00000000 */
	R1 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r1, 0x00000000;

	imm32 r0, 0x12340000;
	imm32 r1, 0x028C003E;
	imm32 r2, 0x82159E24;
	imm32 r3, 0x82159E24;
	imm32 r4, 0xD259E268;
	imm32 r5, 0x52E26AF2;
	imm32 r6, 0x9226AF36;
	imm32 r7, 0xE26BFF86;

	R0.L = 0;
	A0 = 0;
	A0.L = R1.L;
	A0.H = R1.H;
	A0 = LSHIFT A0 BY R0.L;	/* a0 = 0x00000000 */
	R2 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r2, 0x028C003E;

	R1.L = 1;
	A0.L = R2.L;
	A0.H = R2.H;
	A0 = LSHIFT A0 BY R1.L;	/* a0 = 0x00000000 */
	R3 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r3, 0x0518007C;

	R2.L = 15;
	A0.L = R3.L;
	A0.H = R3.H;
	A0 = LSHIFT A0 BY R2.L;	/* a0 = 0x00000000 */
	R4 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r4, 0x003E0000;

	R3.L = 31;
	A0.L = R4.L;
	A0.H = R4.H;
	A0 = LSHIFT A0 BY R3.L;	/* a0 = 0x00000000 */
	R5 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r5, 0x00000000;

	R4.L = -1;
	A0.L = R5.L;
	A0.H = R5.H;
	A0 = LSHIFT A0 BY R4.L;	/* a0 = 0x00000000 */
	R6 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r6, 0x00000000;

	R5.L = -16;
	A0 = 0;
	A0.L = R6.L;
	A0.H = R6.H;
	A0 = LSHIFT A0 BY R5.L;	/* a0 = 0x00000000 */
	R7 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r7, 0x00000000;

	R6.L = -31;
	A0.L = R7.L;
	A0.H = R7.H;
	A0 = LSHIFT A0 BY R6.L;	/* a0 = 0x00000000 */
	R0 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r0, 0x00000000;

	R7.L = -32;
	A0.L = R0.L;
	A0.H = R0.H;
	A0 = LSHIFT A0 BY R7.L;	/* a0 = 0x00000000 */
	R1 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r1, 0x00000000;

	imm32 r0, 0x13340000;
	imm32 r1, 0x038C003E;
	imm32 r2, 0x83159E24;
	imm32 r3, 0x83159E24;
	imm32 r4, 0xD359E268;
	imm32 r5, 0x53E26AF2;
	imm32 r6, 0x9326AF36;
	imm32 r7, 0xE36BFF86;

	R0.L = 0;
	A0 = 0;
	A0.L = R1.L;
	A0.H = R1.H;
	A0 = ROT A0 BY R0.L;	/* a0 = 0x00000000 */
	R2 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r2, 0x038C003E;

	R1.L = 1;
	A0.L = R2.L;
	A0.H = R2.H;
	A0 = ROT A0 BY R1.L;	/* a0 = 0x00000000 */
	R3 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r3, 0x0718007C;

	R2.L = 15;
	A0.L = R3.L;
	A0.H = R3.H;
	A0 = ROT A0 BY R2.L;	/* a0 = 0x00000000 */
	R4 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r4, 0x003E0001;

	R3.L = 31;
	A0.L = R4.L;
	A0.H = R4.H;
	A0 = ROT A0 BY R3.L;	/* a0 = 0x00000000 */
	R5 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r5, 0xE3000F80;

	R4.L = -1;
	A0.L = R5.L;
	A0.H = R5.H;
	A0 = ROT A0 BY R4.L;	/* a0 = 0x00000000 */
	R6 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r6, 0x718007C0;

	R5.L = -16;
	A0.L = R6.L;
	A0.H = R6.H;
	A0 = ROT A0 BY R5.L;	/* a0 = 0x00000000 */
	R7 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r7, 0x80007180;

	R6.L = -31;
	A0.L = R7.L;
	A0.H = R7.H;
	A0 = ROT A0 BY R6.L;	/* a0 = 0x00000000 */
	R0 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r0, 0x01C6001F;

	R7.L = -32;
	A0.L = R0.L;
	A0.H = R0.H;
	A0 = ROT A0 BY R7.L;	/* a0 = 0x00000000 */
	R1 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r1, 0x8C003E00;

	pass
