//Original:/proj/frio/dv/testcases/core/c_dsp32shiftim_a0alr/c_dsp32shiftim_a0alr.dsp
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
	A0 = A0 << 0;	/* a0 = 0x00000000 */
	R1 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r1, 0x012C003E;

	R1.L = 1;
	A0.L = R2.L;
	A0.H = R2.H;
	A0 = A0 << 1;	/* a0 = 0x00000000 */
	R2 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r2, 0x026B3C48;

	R2.L = 15;
	A0.L = R3.L;
	A0.H = R3.H;
	A0 = A0 << 15;	/* a0 = 0x00000000 */
	R3 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r3, 0xCF120000;

	R3.L = 31;
	A0.L = R4.L;
	A0.H = R4.H;
	A0 = A0 << 31;	/* a0 = 0x00000000 */
	R4 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r4, 0x00000000;

	R4.L = -1;
	A0.L = R5.L;
	A0.H = R5.H;
	A0 = A0 >>> 1;	/* a0 = 0x00000000 */
	R5 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r5, 0x28B13579;

	R5.L = -16;
	A0 = 0;
	A0.L = R6.L;
	A0.H = R6.H;
	A0 = A0 >>> 16;	/* a0 = 0x00000000 */
	R6 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r6, 0x00009176;

	R6.L = -31;
	A0.L = R7.L;
	A0.H = R7.H;
	A0 = A0 >>> 31;	/* a0 = 0x00000000 */
	R0 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r0, 0x00000001;

	R7.L = -32;
	A0.L = R0.L;
	A0.H = R0.H;
	.dw 0xC683		//  .dw 0xC683		// A0 = A0 >>> 32;
	.dw 0x0100
	R7 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r7, 0x00000000;

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
	A0 = A0 << 0;	/* a0 = 0x00000000 */
	R1 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r1, 0x028C003E;

	R1.L = 1;
	A0.L = R2.L;
	A0.H = R2.H;
	A0 = A0 << 3;	/* a0 = 0x00000000 */
	R2 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r2, 0x10ACF120;

	R2.L = 15;
	A0.L = R3.L;
	A0.H = R3.H;
	A0 = A0 << 15;	/* a0 = 0x00000000 */
	R3 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r3, 0xCF120000;

	R3.L = 31;
	A0.L = R4.L;
	A0.H = R4.H;
	A0 = A0 << 31;	/* a0 = 0x00000000 */
	R4 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r4, 0x00000000;

	R4.L = -1;
	A0.L = R5.L;
	A0.H = R5.H;
	A0 = A0 >> 1;	/* a0 = 0x00000000 */
	R5 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r5, 0x29713579;

	R5.L = -16;
	A0 = 0;
	A0.L = R6.L;
	A0.H = R6.H;
	A0 = A0 >> 16;	/* a0 = 0x00000000 */
	R6 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r6, 0x00009226;

	R6.L = -31;
	A0.L = R7.L;
	A0.H = R7.H;
	A0 = A0 >> 31;	/* a0 = 0x00000000 */
	R7 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r7, 0x00000001;

	R7.L = -32;
	A0.L = R0.L;
	A0.H = R0.H;
	.dw 0xC683
	.dw 0x4100		// A0 = A0 >> 32;
	R0 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r0, 0x00000000;

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
	A0 = ROT A0 BY 0;	/* a0 = 0x00000000 */
	R1 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r1, 0x038C003E;

	R1.L = 1;
	A0.L = R2.L;
	A0.H = R2.H;
	A0 = ROT A0 BY 1;	/* a0 = 0x00000000 */
	R2 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r2, 0x062B3C48;

	R2.L = 15;
	A0.L = R3.L;
	A0.H = R3.H;
	A0 = ROT A0 BY 15;	/* a0 = 0x00000000 */
	R3 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r3, 0xCF120060;

	R3.L = 31;
	A0.L = R4.L;
	A0.H = R4.H;
	A0 = ROT A0 BY 31;	/* a0 = 0x00000000 */
	R4 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r4, 0x62B4D678;

	R4.L = -1;
	A0.L = R5.L;
	A0.H = R5.H;
	A0 = ROT A0 BY -1;	/* a0 = 0x00000000 */
	R5 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r5, 0x29F13579;

	R5.L = -16;
	A0.L = R6.L;
	A0.H = R6.H;
	A0 = ROT A0 BY -16;	/* a0 = 0x00000000 */
	R6 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r6, 0x6C9A9326;

	R6.L = -31;
	A0.L = R7.L;
	A0.H = R7.H;
	A0 = ROT A0 BY -31;	/* a0 = 0x00000000 */
	R7 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r7, 0xAFFE1ABD;

	R7.L = -32;
	A0.L = R0.L;
	A0.H = R0.H;
	A0 = ROT A0 BY -32;	/* a0 = 0x00000000 */
	R0 = A0.w;	/* r5 = 0x00000000 */
	CHECKREG r0, 0x6800018D;

	pass
