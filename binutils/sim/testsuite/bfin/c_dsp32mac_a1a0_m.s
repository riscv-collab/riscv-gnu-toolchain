//Original:/testcases/core/c_dsp32mac_a1a0_m/c_dsp32mac_a1a0_m.dsp
// Spec Reference: dsp32mac a1 a0 m MNOP
# mach: bfin

.include "testutils.inc"
	start


	INIT_R_REGS 0;


	imm32 r0, 0x00000000;
	A0 = 0;
	A1 = 0;
	ASTAT = r0;

// test the MNOP  default (signed fraction : left ) rounding U=0 I=0 T=0 w32=1
	imm32 r0, 0x123c5678;
	imm32 r1, 0x2345c789;
	imm32 r2, 0x34567c9a;
	imm32 r3, 0x456789c2;
	imm32 r4, 0xc678912c;
	imm32 r5, 0x6c891234;
	imm32 r6, 0xa1c34567;
	imm32 r7, 0xc12c4567;

	A0 = 0;
	A1 = 0;

	A1 = R0.L * R1.L (M);
	R0 = A0.w;
	R1 = A1.w;
	A0 += R2.H * R3.H;
	R2 = A0.w;
	R3 = A1.w;
	A1 += R4.L * R5.H;
	R4 = A0.w;
	R5 = A1.w;
	A0 += R6.L * R7.H;
	R6 = A0.w;
	R7 = A1.w;
	CHECKREG r0, 0x00000000;
	CHECKREG r1, 0x43658E38;
	CHECKREG r2, 0x1C607934;
	CHECKREG r3, 0x43658E38;
	CHECKREG r4, 0x1C607934;
	CHECKREG r5, 0xE56C0F50;
	CHECKREG r6, 0xFA4FA29C;
	CHECKREG r7, 0xE56C0F50;

	imm32 r0, 0xd2345678;
	imm32 r1, 0x2d456789;
	imm32 r2, 0x34d6789a;
	imm32 r3, 0x456d8912;
	imm32 r4, 0x5678d123;
	imm32 r5, 0x67891d34;
	imm32 r6, 0xa12345d7;
	imm32 r7, 0xc123456d;
	A0 += R6.H * R7.L;
	R6 = A0.w;
	R7 = A1.w;
	A1 += R4.L * R5.H;
	R4 = A0.w;
	R5 = A1.w;
	A0 += R2.L * R3.L;
	R2 = A0.w;
	R3 = A1.w;
	A1 += R0.H * R1.L;
	R0 = A0.w;
	R1 = A1.w;
	CHECKREG r0, 0x56CD8212;
	CHECKREG r1, 0x9A78E46E;
	CHECKREG r2, 0x56CD8212;
	CHECKREG r3, 0xBF8410C6;
	CHECKREG r4, 0xC6DBB86A;
	CHECKREG r5, 0xBF8410C6;
	CHECKREG r6, 0xC6DBB86A;
	CHECKREG r7, 0xE56C0F50;

// test MM=1(Mix mode), MAC1 executes a mixed mode multiplication: (one input is
// signed, the other input is unsigned
	imm32 r0, 0x12345678;
	imm32 r1, 0x33456789;
	imm32 r2, 0x5556789a;
	imm32 r3, 0x75678912;
	imm32 r4, 0x86789123;
	imm32 r5, 0xa7891234;
	imm32 r6, 0xc1234567;
	imm32 r7, 0xf1234567;
	A1 += R0.L * R1.L (M), A0 = R0.L * R1.L;
	R0 = A0.w;
	R1 = A1.w;
	A1 = R2.L * R3.L (M), A0 += R2.L * R3.H;
	R2 = A0.w;
	R3 = A1.w;
	A1 += R4.L * R5.L (M), A0 = R4.H * R5.L;
	R4 = A0.w;
	R5 = A1.w;
	A1 = R6.L * R7.L (M), A0 = R6.H * R7.H;
	R6 = A0.w;
	R7 = A1.w;
	CHECKREG r0, 0x45F11C70;
	CHECKREG r1, 0xBD7172A6;
	CHECKREG r2, 0xB48EEC5C;
	CHECKREG r3, 0x4092E4D4;
	CHECKREG r4, 0xEEB780C0;
	CHECKREG r5, 0x38B0D5F0;
	CHECKREG r6, 0x074CB592;
	CHECKREG r7, 0x12D0AF71;

	imm32 r0, 0x12245618;
	imm32 r1, 0x23256719;
	imm32 r2, 0x3426781a;
	imm32 r3, 0x45278912;
	imm32 r4, 0x56289113;
	imm32 r5, 0x67291214;
	imm32 r6, 0xa1234517;
	imm32 r7, 0xc1234517;
	A1 += R0.L * R1.H (M), A0 = R0.L * R1.L;
	R0 = A0.w;
	R1 = A1.w;
	A1 += R2.L * R3.H (M), A0 = R2.L * R3.H;
	R2 = A0.w;
	R3 = A1.w;
	A1 += R4.L * R5.H (M), A0 = R4.H * R5.L;
	R4 = A0.w;
	R5 = A1.w;
	A1 += R6.L * R7.H (M), A0 += R6.H * R7.H;
	R6 = A0.w;
	R7 = A1.w;
	CHECKREG r0, 0x455820B0;
	CHECKREG r1, 0x1EA268E9;
	CHECKREG r2, 0x40E29BEC;
	CHECKREG r3, 0x3F13B6DF;
	CHECKREG r4, 0x0C2B1640;
	CHECKREG r5, 0x126097EA;
	CHECKREG r6, 0x3AC1EBD2;
	CHECKREG r7, 0x4680610F;

	imm32 r0, 0x15245648;
	imm32 r1, 0x25256749;
	imm32 r2, 0x3526784a;
	imm32 r3, 0x45278942;
	imm32 r4, 0x55389143;
	imm32 r5, 0x65391244;
	imm32 r6, 0xa5334547;
	imm32 r7, 0xc5334547;
	A1 = R0.H * R1.H (M), A0 = R0.L * R1.L;
	R0 = A0.w;
	R1 = A1.w;
	A1 += R2.H * R3.H (M), A0 += R2.L * R3.H;
	R2 = A0.w;
	R3 = A1.w;
	A1 = R4.H * R5.H (M), A0 = R4.H * R5.L;
	R4 = A0.w;
	R5 = A1.w;
	A1 = R6.H * R7.H (M), A0 = R6.H * R7.H;
	R6 = A0.w;
	R7 = A1.w;
	CHECKREG r0, 0x459F2510;
	CHECKREG r1, 0x03114234;
	CHECKREG r2, 0x869BAF9C;
	CHECKREG r3, 0x116C98FE;
	CHECKREG r4, 0x0C2925C0;
	CHECKREG r5, 0x21B21178;
	CHECKREG r6, 0x29B65052;
	CHECKREG r7, 0xBA0E2829;

	imm32 r0, 0x13245628;
	imm32 r1, 0x23256729;
	imm32 r2, 0x3326782a;
	imm32 r3, 0x43278922;
	imm32 r4, 0x56389123;
	imm32 r5, 0x67391224;
	imm32 r6, 0xa1334527;
	imm32 r7, 0xc1334527;
	A1 = R0.H * R1.L (M), A0 = R0.L * R1.L;
	R0 = A0.w;
	R1 = A1.w;
	A1 += R2.H * R3.L (M), A0 = R2.L * R3.H;
	R2 = A0.w;
	R3 = A1.w;
	A1 = R4.H * R5.L (M), A0 = R4.H * R5.L;
	R4 = A0.w;
	R5 = A1.w;
	A1 += R6.H * R7.L (M), A0 = R6.H * R7.H;
	R6 = A0.w;
	R7 = A1.w;
	CHECKREG r0, 0x456FC8D0;
	CHECKREG r1, 0x07B68CC4;
	CHECKREG r2, 0x3F0A98CC;
	CHECKREG r3, 0x231CADD0;
	CHECKREG r4, 0x0C381FC0;
	CHECKREG r5, 0x061C0FE0;
	CHECKREG r6, 0x2E832052;
	CHECKREG r7, 0xEC805DA5;

// test the MNOP  default (signed fraction : left ) rounding U=0 I=0 T=0 w32=1
	imm32 r0, 0x123c5678;
	imm32 r1, 0x2345c789;
	imm32 r2, 0x34567c9a;
	imm32 r3, 0x456789c2;
	imm32 r4, 0xc678912c;
	imm32 r5, 0x6c891234;
	imm32 r6, 0xa1c34567;
	imm32 r7, 0xc12c4567;

	A0 = 0;
	A1 = 0;

	A1 += R0.L * R1.L (M);
	R0 = A0.w;
	R1 = A1.w;
	A0 += R2.H * R3.H;
	R2 = A0.w;
	R3 = A1.w;
	A1 = R4.L * R5.H (M);
	R4 = A0.w;
	R5 = A1.w;
	A0 += R6.L * R7.H;
	R6 = A0.w;
	R7 = A1.w;
	CHECKREG r0, 0x00000000;
	CHECKREG r1, 0x43658E38;
	CHECKREG r2, 0x1C607934;
	CHECKREG r3, 0x43658E38;
	CHECKREG r4, 0x1C607934;
	CHECKREG r5, 0xD103408C;
	CHECKREG r6, 0xFA4FA29C;
	CHECKREG r7, 0xD103408C;

	imm32 r0, 0xd2345678;
	imm32 r1, 0x2d456789;
	imm32 r2, 0x34d6789a;
	imm32 r3, 0x456d8912;
	imm32 r4, 0x5678d123;
	imm32 r5, 0x67891d34;
	imm32 r6, 0xa12345d7;
	imm32 r7, 0xc123456d;
	A0 = R6.H * R7.L;
	R6 = A0.w;
	R7 = A1.w;
	A1 = R4.L * R5.H (M);
	R4 = A0.w;
	R5 = A1.w;
	A0 = R2.L * R3.L;
	R2 = A0.w;
	R3 = A1.w;
	A1 += R0.H * R1.L (M);
	R0 = A0.w;
	R1 = A1.w;
	CHECKREG r0, 0x8FF1C9A8;
	CHECKREG r1, 0xDA866A8F;
	CHECKREG r2, 0x8FF1C9A8;
	CHECKREG r3, 0xED0C00BB;
	CHECKREG r4, 0xCC8C15CE;
	CHECKREG r5, 0xED0C00BB;
	CHECKREG r6, 0xCC8C15CE;
	CHECKREG r7, 0xD103408C;

	imm32 r0, 0x123c5678;
	imm32 r1, 0x2345c789;
	imm32 r2, 0x34567c9a;
	imm32 r3, 0x456789c2;
	imm32 r4, 0xc678912c;
	imm32 r5, 0x6c891234;
	imm32 r6, 0xa1c34567;
	imm32 r7, 0xc12c4567;

	A0 = 0;
	A1 = 0;

	A1 -= R0.L * R1.L (M);
	R0 = A0.w;
	R1 = A1.w;
	A0 -= R2.H * R3.H;
	R2 = A0.w;
	R3 = A1.w;
	A1 -= R4.L * R5.H (M);
	R4 = A0.w;
	R5 = A1.w;
	A0 -= R6.L * R7.H;
	R6 = A0.w;
	R7 = A1.w;
	CHECKREG r0, 0x00000000;
	CHECKREG r1, 0xBC9A71C8;
	CHECKREG r2, 0xE39F86CC;
	CHECKREG r3, 0xBC9A71C8;
	CHECKREG r4, 0xE39F86CC;
	CHECKREG r5, 0xEB97313C;
	CHECKREG r6, 0x05B05D64;
	CHECKREG r7, 0xEB97313C;

	imm32 r0, 0xd2345678;
	imm32 r1, 0x2d456789;
	imm32 r2, 0x34d6789a;
	imm32 r3, 0x456d8912;
	imm32 r4, 0x5678d123;
	imm32 r5, 0x67891d34;
	imm32 r6, 0xa12345d7;
	imm32 r7, 0xc123456d;
	A0 -= R6.H * R7.L;
	R6 = A0.w;
	R7 = A1.w;
	A1 -= R4.L * R5.H (M);
	R4 = A0.w;
	R5 = A1.w;
	A0 -= R2.L * R3.L;
	R2 = A0.w;
	R3 = A1.w;
	A1 -= R0.H * R1.L (M);
	R0 = A0.w;
	R1 = A1.w;
	CHECKREG r0, 0xA9327DEE;
	CHECKREG r1, 0x1110C6AD;
	CHECKREG r2, 0xA9327DEE;
	CHECKREG r3, 0xFE8B3081;
	CHECKREG r4, 0x39244796;
	CHECKREG r5, 0xFE8B3081;
	CHECKREG r6, 0x39244796;
	CHECKREG r7, 0xEB97313C;

	pass

	.data
DATA0:
	.dd 0x000a0000
	.dd 0x000b0001
	.dd 0x000c0002
	.dd 0x000d0003
	.dd 0x000e0004
	.dd 0x000f0005

DATA1:
	.dd 0x00f00100
	.dd 0x00e00101
	.dd 0x00d00102
	.dd 0x00c00103
	.dd 0x00b00104
	.dd 0x00a00105
