//Original:/proj/frio/dv/testcases/core/c_dsp32alu_alhwx/c_dsp32alu_alhwx.dsp
// Spec Reference: dsp32alu alhwx
# mach: bfin

.include "testutils.inc"
	start

	R0 = 0;
	ASTAT = R0;
	A1 = A0 = 0;

	imm32 r0, 0xa5678911;
	imm32 r1, 0xaa89ab1d;
	imm32 r2, 0xd4b45515;
	imm32 r3, 0xf66e7717;
	imm32 r4, 0xe567f91b;
	imm32 r5, 0x6789ae1d;
	imm32 r6, 0xb4445515;
	imm32 r7, 0x8666a7d7;
	A0.L = R0.L;
	A0.H = R0.H;
	A0.x = R1.L;
	R7 = A0.w;
	R6 = A0.x;
	R5.L = A0.x;
	A1.L = R4.L;
	A1.H = R4.H;
	A1.x = R3.L;
	R0 = A1.w;
	R1 = A1.x;
	R2.L = A1.x;
	CHECKREG r0, 0xE567F91B;
	CHECKREG r1, 0x00000017;
	CHECKREG r2, 0xD4B40017;
	CHECKREG r3, 0xF66E7717;
	CHECKREG r4, 0xE567F91B;
	CHECKREG r5, 0x6789001D;
	CHECKREG r6, 0x0000001D;
	CHECKREG r7, 0xA5678911;

	imm32 r0, 0xe5678911;
	imm32 r1, 0xaa89ab1d;
	imm32 r2, 0xdfb45515;
	imm32 r3, 0xf66e7717;
	imm32 r4, 0xe5d7f91b;
	imm32 r5, 0x67e9ae1d;
	imm32 r6, 0xb4445515;
	imm32 r7, 0x866aa7b7;
	A0.L = R1.L;
	A0.H = R1.H;
	A0.x = R2.L;
	R5 = A0.w;
	R7 = A0.x;
	R6.L = A0.x;
	A1.L = R3.L;
	A1.H = R3.H;
	A1.x = R4.L;
	R1 = A1.w;
	R2 = A1.x;
	R0.L = A1.x;
	CHECKREG r0, 0xE567001B;
	CHECKREG r1, 0xF66E7717;
	CHECKREG r2, 0x0000001B;
	CHECKREG r3, 0xF66E7717;
	CHECKREG r4, 0xE5D7F91B;
	CHECKREG r5, 0xAA89AB1D;
	CHECKREG r6, 0xB4440015;
	CHECKREG r7, 0x00000015;

	imm32 r0, 0x35678911;
	imm32 r1, 0xa489ab1d;
	imm32 r2, 0xd4545515;
	imm32 r3, 0xf6667717;
	imm32 r4, 0x9567f91b;
	imm32 r5, 0x6a89ae1d;
	imm32 r6, 0xb4445515;
	imm32 r7, 0x8666a7d7;
	A0.L = R3.L;
	A0.H = R3.H;
	A0.x = R4.L;
	R0 = A0.w;
	R1 = A0.x;
	R2.L = A0.x;
	A1.L = R5.L;
	A1.H = R6.H;
	A1.x = R7.L;
	R7 = A1.w;
	R5 = A1.x;
	R5.L = A1.x;
	CHECKREG r0, 0xF6667717;
	CHECKREG r1, 0x0000001B;
	CHECKREG r2, 0xD454001B;
	CHECKREG r3, 0xF6667717;
	CHECKREG r4, 0x9567F91B;
	CHECKREG r5, 0xffffffD7;
	CHECKREG r6, 0xB4445515;
	CHECKREG r7, 0xB444AE1D;

	imm32 r0, 0xd5678911;
	imm32 r1, 0x2a89ab1d;
	imm32 r2, 0xd3b45515;
	imm32 r3, 0xf66e7717;
	imm32 r4, 0xe5d7f91b;
	imm32 r5, 0x67e9ae1d;
	imm32 r6, 0xb4445515;
	imm32 r7, 0x889aa7b7;
	A0.L = R4.L;
	A0.H = R5.H;
	A0.x = R6.L;
	R1 = A0.w;
	R2 = A0.x;
	R3.L = A0.x;
	A1.L = R0.L;
	A1.H = R0.H;
	A1.x = R7.L;
	R4 = A1.w;
	R5 = A1.x;
	R6.L = A1.x;
	CHECKREG r0, 0xD5678911;
	CHECKREG r1, 0x67E9F91B;
	CHECKREG r2, 0x00000015;
	CHECKREG r3, 0xF66E0015;
	CHECKREG r4, 0xD5678911;
	CHECKREG r5, 0xffffffB7;
	CHECKREG r6, 0xB444ffB7;
	CHECKREG r7, 0x889AA7B7;

	pass
