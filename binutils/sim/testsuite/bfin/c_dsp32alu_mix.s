//Original:/proj/frio/dv/testcases/core/c_dsp32alu_mix/c_dsp32alu_mix.dsp
// Spec Reference: dsp32alu mix
# mach: bfin

.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;

// ALU operations include parallel addition, subtraction, MAX, MIN, ABS on 16-bit
// and 32-bit data. If an operation use a single ALU only, it uses ALU0.

	imm32 r2, 0x44445555;
	imm32 r3, 0x66667777;
	imm32 r4, 0x88889999;
	imm32 r5, 0xaaaabbbb;
	imm32 r6, 0xccccdddd;
	imm32 r7, 0xeeeeffff;

	imm32 r0, 0x456789ab;
	imm32 r1, 0x6789abcd;
// Use only single ALU (ALU0 only), with saturation (S)
	R2 = R1 + R0 (NS);	/* 0xACF13578 */
	R3 = R2 + R0 (NS);	/* 0xACF13578 */
	CHECKREG r2, 0xACF13578;
	CHECKREG r3, 0xF258BF23;
	R2 = R1 + R0 (S);	/* 0x7FFFFFFF */
	R3 = R1 - R0 (NS);	/* 0x22222222 */
	R4.L = R1.L + R0.L (NS);	/* 0x88883578 */
	R5.L = R1.L + R0.H (NS);	/* 0xAAAAF134 */
	R6.L = R1.H + R0.L (NS);	/* 0xCCCCF134 */
	R7.L = R1.H + R0.H (NS);	/* 0xEEEEACF0 */
	CHECKREG r2, 0x7FFFFFFF;
	CHECKREG r3, 0x22222222;
	CHECKREG r4, 0x88883578;
	CHECKREG r5, 0xAAAAF134;
	CHECKREG r6, 0xCCCCF134;
	CHECKREG r7, 0xEEEEACF0;

	R4.H = R1.L + R0.L (S);	/* 0x80003578 */
	R5.H = R1.L + R0.H (S);	/* 0xF134F134 */
	R6.H = R1.H + R0.L (S);	/* 0xF134F134 */
	CHECKREG r4, 0x80003578;
	CHECKREG r5, 0xF134F134;
	CHECKREG r6, 0xF134F134;

	R4.H = R1.L + R0.L (S);	/* 0x80003578 */
	R5.H = R1.L + R0.H (S);	/* 0xF134F134 */
	R6.H = R1.H + R0.L (S);	/* 0xF134F134 */
	CHECKREG r4, 0x80003578;	/* 0x         */
	CHECKREG r5, 0xF134F134;	/* 0x         */
	CHECKREG r6, 0xF134F134;	/* 0x         */

	R4.H = R1.L + R0.L (S);	/* 0x80003578 */
	R5.H = R1.L + R0.H (S);	/* 0xF134F134 */
	R6.H = R1.H + R0.L (S);	/* 0xF134F134 */
	R7.H = R1.H + R0.H (S);	/* 0x7FFFACF0 */
	CHECKREG r4, 0x80003578;	/* 0x         */
	CHECKREG r5, 0xF134F134;	/* 0x         */
	CHECKREG r6, 0xF134F134;	/* 0x         */
	CHECKREG r7, 0x7FFFACF0;	/* 0x         */

// Dual
	R2 = R0 +|+ R1 (SCO);	/* 0x80007FFF */
	R3 = R0 +|- R1 (S);	/* 0x7FFFDDDE */
	R4 = R0 -|+ R1 (SCO);	/* 0x8000DDDE)*/
	R5 = R0 -|- R1 (SCO);	/* 0xDDDEDDDE */
	CHECKREG r2, 0x80007FFF;
	CHECKREG r3, 0x7FFFDDDE;
	CHECKREG r4, 0x8000DDDE;
	CHECKREG r5, 0xDDDEDDDE;
	R2 = R0 +|+ R1, R3 = R0 -|- R1 (SCO);	/* 0x         */
CHECKREG r2, 0x7FFF8000;
	R4 = R0 +|- R1 , R5 = R0 -|+ R1 (CO);	/* 0x         */
	R6 = R0 + R1, R7 = R0 - R1 (S);	/* 0x         */
	CHECKREG r2, 0x7FFF8000;
	CHECKREG r3, 0xDDDEDDDE;
	CHECKREG r4, 0xACF0DDDE;
	CHECKREG r5, 0x3578DDDE;
	CHECKREG r6, 0x7FFFFFFF;
	CHECKREG r7, 0xDDDDDDDE;

// Max min abs types
	R3 = MAX ( R0 , R1 );	/* 0x6789ABCD */
	R4 = MIN ( R0 , R1 );	/* 0x456789AB */
	R5 = ABS R0;	/* 0x456789AB */
	CHECKREG r3, 0x6789ABCD;
	CHECKREG r4, 0x456789AB;
	CHECKREG r5, 0x456789AB;
	R3 = MAX ( R0 , R1 ) (V);	/* 0x6789ABCD */
	R4 = MIN ( R0 , R1 ) (V);	/* 0x456789AB */
	R5 = ABS R0 (V);	/* 0x45677655 */
	CHECKREG r3, 0x6789ABCD;
	CHECKREG r4, 0x456789AB;
	CHECKREG r5, 0x45677655;

// RND types
	R2.H = R2.L = SIGN(R0.H) * R1.H + SIGN(R0.L) * R1.L;
	R3.L = R0 + R1 (RND12);	/* 0x         */
	R4.H = R0 - R1 (RND12);	/* 0x         */
	R5.L = R0 + R1 (RND20);	/* 0x         */
	R6.H = R0 - R1 (RND20);	/* 0x         */
	R7.H = R1 (RND);	/* 0x         */
	CHECKREG r2, 0xBBBCBBBC;
	CHECKREG r3, 0x67897FFF;
	CHECKREG r4, 0x800089AB;
	CHECKREG r5, 0x45670ACF;
	CHECKREG r6, 0xFDDEFFFF;
	CHECKREG r7, 0x678ADDDE;

	R7 = - R0 (V);	/* 0x         */
	CHECKREG r7, 0xBA997655;
// A0 & A1 types
	A0 = 0;
	A1 = 0;
	A0.L = R0.L;
	A0.H = R0.H;
	A0 = A1;
	A0.x = R0.L;
	A1.x = R0.L;
	R2.L = A0.x;	/* 0x         */
	R3.L = A1.x;	/* 0x         */
	R4 = ( A0 += A1 );	/* 0x         */
	R5.L = ( A0 += A1 );	/* 0x         */
	R5.H = ( A0 += A1 );	/* 0x         */
	CHECKREG r2, 0xBBBCffAB;	/* 0x         */
	CHECKREG r3, 0x6789ffAB;	/* 0x         */
	CHECKREG r4, 0x80000000;	/* 0x         */
	CHECKREG r5, 0x80008000;	/* 0x         */
	A0 += A1;
	A0 -= A1;
	R6 = A1.L + A1.H, R7 = A0.L + A0.H;	/* 0x         */
	CHECKREG r6, 0x00000000;
	CHECKREG r7, 0x00000000;

	pass
