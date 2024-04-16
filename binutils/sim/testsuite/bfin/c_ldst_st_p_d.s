//Original:/testcases/core/c_ldst_st_p_d/c_ldst_st_p_d.dsp
// Spec Reference: c_ldst st_p_d
# mach: bfin

.include "testutils.inc"
	start

	imm32 r0, 0x0a231507;
	imm32 r1, 0x1b342618;
	imm32 r2, 0x2c453729;
	imm32 r3, 0x3d56483a;
	imm32 r4, 0x4e67594b;
	imm32 r5, 0x5f786a5c;
	imm32 r6, 0x60897b6d;
	imm32 r7, 0x719a8c7e;

	loadsym p5, DATA_ADDR_1;
	loadsym p1, DATA_ADDR_2;
	loadsym p2, DATA_ADDR_3;
	loadsym p4, DATA_ADDR_5;
	loadsym fp, DATA_ADDR_6;

	[ P5 ] = R0;
	[ P1 ] = R1;
	[ P2 ] = R2;
	[ P4 ] = R4;
	[ FP ] = R5;

	R0 = [ P1 ];
	R1 = [ P2 ];
	R3 = [ P4 ];
	R4 = [ P5 ];
	R5 = [ FP ];
	CHECKREG r0, 0x1B342618;
	CHECKREG r1, 0x2C453729;
	CHECKREG r3, 0x4E67594B;
	CHECKREG r4, 0x0A231507;
	CHECKREG r5, 0x5F786A5C;
	CHECKREG r7, 0x719A8C7E;

	imm32 r0, 0x1a231507;
	imm32 r1, 0x12342618;
	imm32 r2, 0x2c353729;
	imm32 r3, 0x3d54483a;
	imm32 r4, 0x4e67594b;
	imm32 r5, 0x5f78665c;
	imm32 r6, 0x60897b7d;
	imm32 r7, 0x719a8c78;
	[ P5 ] = R1;
	[ P1 ] = R2;
	[ P2 ] = R3;
	[ P4 ] = R5;
	[ FP ] = R6;
	R0 = [ P1 ];
	R1 = [ P2 ];
	R3 = [ P4 ];
	R4 = [ P5 ];
	R5 = [ FP ];
	CHECKREG r0, 0x2C353729;
	CHECKREG r1, 0x3D54483A;
	CHECKREG r3, 0x5F78665C;
	CHECKREG r4, 0x12342618;
	CHECKREG r5, 0x60897B7D;
	CHECKREG r7, 0x719A8C78;

	imm32 r0, 0x2a231507;
	imm32 r1, 0x12342618;
	imm32 r2, 0x2c253729;
	imm32 r3, 0x3d52483a;
	imm32 r4, 0x4e67294b;
	imm32 r5, 0x5f78625c;
	imm32 r6, 0x60897b2d;
	imm32 r7, 0x719a8c72;
	[ P5 ] = R2;
	[ P1 ] = R3;
	[ P2 ] = R4;
	[ P4 ] = R6;
	[ FP ] = R7;
	R0 = [ P1 ];
	R1 = [ P2 ];
	R3 = [ P4 ];
	R4 = [ P5 ];
	R5 = [ FP ];
	CHECKREG r0, 0x3D52483A;
	CHECKREG r1, 0x4E67294B;
	CHECKREG r3, 0x60897B2D;
	CHECKREG r4, 0x2C253729;
	CHECKREG r5, 0x719A8C72;
	CHECKREG r7, 0x719A8C72;

	imm32 r0, 0x3a231507;
	imm32 r1, 0x13342618;
	imm32 r2, 0x2c353729;
	imm32 r3, 0x3d53483a;
	imm32 r4, 0x4e67394b;
	imm32 r5, 0x5f78635c;
	imm32 r6, 0x60897b3d;
	imm32 r7, 0x719a8c73;
	[ P5 ] = R3;
	[ P1 ] = R4;
	[ P2 ] = R5;
	[ P4 ] = R7;
	[ FP ] = R0;
	R0 = [ P1 ];
	R1 = [ P2 ];
	R3 = [ P4 ];
	R4 = [ P5 ];
	R5 = [ FP ];
	CHECKREG r0, 0x4E67394B;
	CHECKREG r1, 0x5F78635C;
	CHECKREG r3, 0x719A8C73;
	CHECKREG r4, 0x3D53483A;
	CHECKREG r5, 0x3A231507;
	CHECKREG r7, 0x719A8C73;

	imm32 r0, 0x4a231507;
	imm32 r1, 0x14342618;
	imm32 r2, 0x2c453729;
	imm32 r3, 0x3d54483a;
	imm32 r4, 0x4e67494b;
	imm32 r5, 0x5f78645c;
	imm32 r6, 0x60897b4d;
	imm32 r7, 0x719a8c74;
	[ P5 ] = R4;
	[ P1 ] = R5;
	[ P2 ] = R6;
	[ P4 ] = R0;
	[ FP ] = R1;
	R0 = [ P1 ];
	R1 = [ P2 ];
	R3 = [ P4 ];
	R4 = [ P5 ];
	R5 = [ FP ];
	CHECKREG r0, 0x5F78645C;
	CHECKREG r1, 0x60897B4D;
	CHECKREG r3, 0x4A231507;
	CHECKREG r4, 0x4E67494B;
	CHECKREG r5, 0x14342618;
	CHECKREG r7, 0x719A8C74;

	imm32 r0, 0x5a231507;
	imm32 r1, 0x15342618;
	imm32 r2, 0x2c553729;
	imm32 r3, 0x3d55483a;
	imm32 r4, 0x4e67594b;
	imm32 r5, 0x5f78655c;
	imm32 r6, 0x60897b5d;
	imm32 r7, 0x719a8c75;
	[ P5 ] = R5;
	[ P1 ] = R6;
	[ P2 ] = R7;
	[ P4 ] = R1;
	[ FP ] = R2;
	R0 = [ P1 ];
	R1 = [ P2 ];
	R3 = [ P4 ];
	R4 = [ P5 ];
	R5 = [ FP ];
	CHECKREG r0, 0x60897B5D;
	CHECKREG r1, 0x719A8C75;
	CHECKREG r3, 0x15342618;
	CHECKREG r4, 0x5F78655C;
	CHECKREG r5, 0x2C553729;
	CHECKREG r7, 0x719A8C75;

	imm32 r0, 0x6a231507;
	imm32 r1, 0x16342618;
	imm32 r2, 0x2c653729;
	imm32 r3, 0x3d56483a;
	imm32 r4, 0x4e67694b;
	imm32 r5, 0x5f78665c;
	imm32 r6, 0x60897b6d;
	imm32 r7, 0x719a8c76;
	[ P5 ] = R6;
	[ P1 ] = R7;
	[ P2 ] = R0;
	[ P4 ] = R2;
	[ FP ] = R3;
	R0 = [ P1 ];
	R1 = [ P2 ];
	R3 = [ P4 ];
	R4 = [ P5 ];
	R5 = [ FP ];
	CHECKREG r0, 0x719A8C76;
	CHECKREG r1, 0x6A231507;
	CHECKREG r3, 0x2C653729;
	CHECKREG r4, 0x60897B6D;
	CHECKREG r5, 0x3D56483A;
	CHECKREG r7, 0x719A8C76;

	imm32 r0, 0x7a231507;
	imm32 r1, 0x17342618;
	imm32 r2, 0x2c753729;
	imm32 r3, 0x3d57483a;
	imm32 r4, 0x4e67794b;
	imm32 r5, 0x5f78675c;
	imm32 r6, 0x60897b7d;
	imm32 r7, 0x719a8c77;
	[ P5 ] = R7;
	[ P1 ] = R0;
	[ P2 ] = R1;
	[ P4 ] = R3;
	[ FP ] = R4;
	R0 = [ P1 ];
	R1 = [ P2 ];
	R3 = [ P4 ];
	R4 = [ P5 ];
	R5 = [ FP ];
	CHECKREG r0, 0x7A231507;
	CHECKREG r1, 0x17342618;
	CHECKREG r3, 0x3D57483A;
	CHECKREG r4, 0x719A8C77;
	CHECKREG r5, 0x4E67794B;
	CHECKREG r7, 0x719A8C77;

	pass

// Pre-load memory with known data
// More data is defined than will actually be used

	.data

DATA_ADDR_1:
	.dd 0x00010203
	.dd 0x04050607
	.dd 0x08090A0B
	.dd 0x0C0D0E0F
	.dd 0x10111213
	.dd 0x14151617
	.dd 0x18191A1B
	.dd 0x1C1D1E1F

DATA_ADDR_2:
	.dd 0x20212223
	.dd 0x24252627
	.dd 0x28292A2B
	.dd 0x2C2D2E2F
	.dd 0x30313233
	.dd 0x34353637
	.dd 0x38393A3B
	.dd 0x3C3D3E3F

DATA_ADDR_3:
	.dd 0x40414243
	.dd 0x44454647
	.dd 0x48494A4B
	.dd 0x4C4D4E4F
	.dd 0x50515253
	.dd 0x54555657
	.dd 0x58595A5B
	.dd 0x5C5D5E5F

DATA_ADDR_4:
	.dd 0x60616263
	.dd 0x64656667
	.dd 0x68696A6B
	.dd 0x6C6D6E6F
	.dd 0x70717273
	.dd 0x74757677
	.dd 0x78797A7B
	.dd 0x7C7D7E7F

DATA_ADDR_5:
	.dd 0x80818283
	.dd 0x84858687
	.dd 0x88898A8B
	.dd 0x8C8D8E8F
	.dd 0x90919293
	.dd 0x94959697
	.dd 0x98999A9B
	.dd 0x9C9D9E9F

DATA_ADDR_6:
	.dd 0xA0A1A2A3
	.dd 0xA4A5A6A7
	.dd 0xA8A9AAAB
	.dd 0xACADAEAF
	.dd 0xB0B1B2B3
	.dd 0xB4B5B6B7
	.dd 0xB8B9BABB
	.dd 0xBCBDBEBF

DATA_ADDR_7:
	.dd 0xC0C1C2C3
	.dd 0xC4C5C6C7
	.dd 0xC8C9CACB
	.dd 0xCCCDCECF
	.dd 0xD0D1D2D3
	.dd 0xD4D5D6D7
	.dd 0xD8D9DADB
	.dd 0xDCDDDEDF
	.dd 0xE0E1E2E3
	.dd 0xE4E5E6E7
	.dd 0xE8E9EAEB
	.dd 0xECEDEEEF
	.dd 0xF0F1F2F3
	.dd 0xF4F5F6F7
	.dd 0xF8F9FAFB
	.dd 0xFCFDFEFF
