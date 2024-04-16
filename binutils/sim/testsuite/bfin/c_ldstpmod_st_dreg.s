//Original:testcases/core/c_ldstpmod_st_dreg/c_ldstpmod_st_dreg.dsp
// Spec Reference: c_ldstpmod store dreg
# mach: bfin

.include "testutils.inc"
	start

// set all regs
init_i_regs 0;
init_b_regs 0;
init_l_regs 0;
init_m_regs 0;
I0 = P3;
I2 = SP;

// initial values
	imm32 r0, 0x600f5000;
	imm32 r1, 0x700e6001;
	imm32 r2, 0x800d7002;
	imm32 r3, 0x900c8003;
	imm32 r4, 0xa00b9004;
	imm32 r5, 0xb00aa005;
	imm32 r6, 0xc009b006;
	imm32 r7, 0xd008c007;
	P1 = 0x0004;
	P2 = 0x0004;
	P3 = 0x0004;
	P4 = 0x0004;
	FP = 0x0004;
	SP = 0x0008;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym p5, DATA_ADDR_5, 0x00;
	P3 = I1; SP = I3;
	[ P5 ++ P1 ] = R0;
	[ P5 ++ P1 ] = R1;
	[ P5 ++ P2 ] = R2;
	[ P5 ++ P3 ] = R3;
	[ P5 ++ P4 ] = R4;
	[ P5 ++ SP ] = R5;
	[ P5 ++ FP ] = R6;
	P1 = 0x0004;
	P2 = 0x0004;
	P3 = 0x0004;
	P4 = 0x0004;
	FP = 0x0004;
	SP = 0x0008;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym p5, DATA_ADDR_5, 0x00;
	P3 = I1; SP = I3;
	R6 = [ P5 ++ P1 ];
	R5 = [ P5 ++ P1 ];
	R4 = [ P5 ++ P2 ];
	R3 = [ P5 ++ P3 ];
	R2 = [ P5 ++ P4 ];
	R0 = [ P5 ++ SP ];
	R1 = [ P5 ++ FP ];
	CHECKREG r0, 0xB00AA005;
	CHECKREG r1, 0xC009B006;
	CHECKREG r2, 0xA00B9004;
	CHECKREG r3, 0x900C8003;
	CHECKREG r4, 0x800D7002;
	CHECKREG r5, 0x700E6001;
	CHECKREG r6, 0x600F5000;

// initial values
	imm32 r0, 0x105f50a0;
	imm32 r1, 0x204e60a1;
	imm32 r2, 0x300370a2;
	imm32 r3, 0x402c80a3;
	imm32 r4, 0x501b90a4;
	imm32 r5, 0x600aa0a5;
	imm32 r6, 0x7019b0a6;
	imm32 r7, 0xd028c0a7;
	P5 = 0x0004;
	P2 = 0x0004;
	P3 = 0x0004;
	P4 = 0x0004;
	FP = 0x0008;
	SP = 0x0004;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym p1, DATA_ADDR_1, 0x00;
	P3 = I1; SP = I3;
	[ P1 ++ P5 ] = R0;
	[ P1 ++ P5 ] = R1;
	[ P1 ++ P2 ] = R2;
	[ P1 ++ P3 ] = R3;
	[ P1 ++ P4 ] = R4;
	[ P1 ++ SP ] = R5;
	[ P1 ++ FP ] = R6;
	P5 = 0x0004;
	P2 = 0x0004;
	P3 = 0x0004;
	P4 = 0x0004;
	FP = 0x0008;
	SP = 0x0004;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym p1, DATA_ADDR_1, 0x00;
	P3 = I1; SP = I3;
	R6 = [ P1 ++ P5 ];
	R5 = [ P1 ++ P5 ];
	R4 = [ P1 ++ P2 ];
	R3 = [ P1 ++ P3 ];
	R2 = [ P1 ++ P4 ];
	R0 = [ P1 ++ SP ];
	R1 = [ P1 ++ FP ];
	CHECKREG r0, 0x600AA0A5;
	CHECKREG r1, 0x7019B0A6;
	CHECKREG r2, 0x501B90A4;
	CHECKREG r3, 0x402C80A3;
	CHECKREG r4, 0x300370A2;
	CHECKREG r5, 0x204E60A1;
	CHECKREG r6, 0x105F50A0;

// initial values
	imm32 r0, 0x10bf50b0;
	imm32 r1, 0x20be60b1;
	imm32 r2, 0x30bd70b2;
	imm32 r3, 0x40bc80b3;
	imm32 r4, 0x55bb90b4;
	imm32 r5, 0x60baa0b5;
	imm32 r6, 0x70b9b0b6;
	imm32 r7, 0x80b8c0b7;
	P5 = 0x0004;
	P1 = 0x0004;
	P3 = 0x0004;
	P4 = 0x0004;
	FP = 0x0004;
	SP = 0x0004;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym p2, DATA_ADDR_2, 0x00;
	P3 = I1; SP = I3;
	[ P2 ++ P5 ] = R0;
	[ P2 ++ P1 ] = R1;
	[ P2 ++ P1 ] = R2;
	[ P2 ++ P3 ] = R3;
	[ P2 ++ P4 ] = R4;
	[ P2 ++ SP ] = R5;
	[ P2 ++ FP ] = R6;
	P5 = 0x0004;
	P1 = 0x0004;
	P3 = 0x0004;
	P4 = 0x0004;
	FP = 0x0004;
	SP = 0x0004;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym p2, DATA_ADDR_2, 0x00;
	P3 = I1; SP = I3;
	R3 = [ P2 ++ P5 ];
	R4 = [ P2 ++ P1 ];
	R0 = [ P2 ++ P1 ];
	R1 = [ P2 ++ P3 ];
	R2 = [ P2 ++ P4 ];
	R5 = [ P2 ++ SP ];
	R6 = [ P2 ++ FP ];
	CHECKREG r0, 0x30BD70B2;
	CHECKREG r1, 0x40BC80B3;
	CHECKREG r2, 0x55BB90B4;
	CHECKREG r3, 0x10BF50B0;
	CHECKREG r4, 0x20BE60B1;
	CHECKREG r5, 0x60BAA0B5;
	CHECKREG r6, 0x70B9B0B6;

// initial values
	imm32 r0, 0x10cf50c0;
	imm32 r1, 0x20ce60c1;
	imm32 r2, 0x30c370c2;
	imm32 r3, 0x40cc80c3;
	imm32 r4, 0x50cb90c4;
	imm32 r5, 0x60caa0c5;
	imm32 r6, 0x70c9b0c6;
	imm32 r7, 0xd0c8c0c7;
	P5 = 0x0004;
	P1 = 0x0004;
	P2 = 0x0004;
	P4 = 0x0004;
	FP = 0x0004;
	SP = 0x0004;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym i1, DATA_ADDR_3, 0x00;
	P3 = I1; SP = I3;
	[ P3 ++ P5 ] = R0;
	[ P3 ++ P1 ] = R1;
	[ P3 ++ P2 ] = R2;
	[ P3 ++ P1 ] = R3;
	[ P3 ++ P4 ] = R4;
	[ P3 ++ SP ] = R5;
	[ P3 ++ FP ] = R6;
	P5 = 0x0004;
	P1 = 0x0004;
	P2 = 0x0004;
	P4 = 0x0004;
	FP = 0x0004;
	SP = 0x0004;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym i1, DATA_ADDR_3, 0x00;
	P3 = I1; SP = I3;
	R6 = [ P3 ++ P5 ];
	R5 = [ P3 ++ P1 ];
	R4 = [ P3 ++ P2 ];
	R3 = [ P3 ++ P1 ];
	R2 = [ P3 ++ P4 ];
	R0 = [ P3 ++ SP ];
	R1 = [ P3 ++ FP ];
	CHECKREG r0, 0x60CAA0C5;
	CHECKREG r1, 0x70C9B0C6;
	CHECKREG r2, 0x50CB90C4;
	CHECKREG r3, 0x40CC80C3;
	CHECKREG r4, 0x30C370C2;
	CHECKREG r5, 0x20CE60C1;
	CHECKREG r6, 0x10CF50C0;

// initial values
	imm32 r0, 0x60df50d0;
	imm32 r1, 0x70de60d1;
	imm32 r2, 0x80dd70d2;
	imm32 r3, 0x90dc80d3;
	imm32 r4, 0xa0db90d4;
	imm32 r5, 0xb0daa0d5;
	imm32 r6, 0xc0d9b0d6;
	imm32 r7, 0xd0d8c0d7;
	P5 = 0x0004;
	P1 = 0x0004;
	P2 = 0x0004;
	P3 = 0x0004;
	FP = 0x0004;
	SP = 0x0004;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym p4, DATA_ADDR_4, 0x00;
	P3 = I1; SP = I3;
	[ P4 ++ P5 ] = R0;
	[ P4 ++ P1 ] = R1;
	[ P4 ++ P2 ] = R2;
	[ P4 ++ P3 ] = R3;
	[ P4 ++ P1 ] = R4;
	[ P4 ++ SP ] = R5;
	[ P4 ++ FP ] = R6;
	P5 = 0x0004;
	P1 = 0x0004;
	P2 = 0x0004;
	P3 = 0x0004;
	FP = 0x0004;
	SP = 0x0004;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym p4, DATA_ADDR_4, 0x00;
	P3 = I1; SP = I3;
	R5 = [ P4 ++ P5 ];
	R6 = [ P4 ++ P1 ];
	R0 = [ P4 ++ P2 ];
	R1 = [ P4 ++ P3 ];
	R2 = [ P4 ++ P1 ];
	R3 = [ P4 ++ SP ];
	R4 = [ P4 ++ FP ];
	CHECKREG r0, 0x80DD70D2;
	CHECKREG r1, 0x90DC80D3;
	CHECKREG r2, 0xA0DB90D4;
	CHECKREG r3, 0xB0DAA0D5;
	CHECKREG r4, 0xC0D9B0D6;
	CHECKREG r5, 0x60DF50D0;
	CHECKREG r6, 0x70DE60D1;

// initial values
	imm32 r0, 0x1e5f50e0;
	imm32 r1, 0x2e4e60e1;
	imm32 r2, 0x3e0370e2;
	imm32 r3, 0x4e2c80e3;
	imm32 r4, 0x5e1b90e4;
	imm32 r5, 0x6e0aa0e5;
	imm32 r6, 0x7e19b0e6;
	imm32 r7, 0xde28c0e7;
	P5 = 0x0004;
	P1 = 0x0004;
	P2 = 0x0004;
	P3 = 0x0004;
	P4 = 0x0004;
	FP = 0x0004;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym i3, DATA_ADDR_6, 0x00;
	P3 = I1; SP = I3;
	[ SP ++ P5 ] = R0;
	[ SP ++ P1 ] = R1;
	[ SP ++ P2 ] = R2;
	[ SP ++ P3 ] = R3;
	[ SP ++ P4 ] = R4;
	[ SP ++ P1 ] = R5;
	[ SP ++ FP ] = R6;
	P5 = 0x0004;
	P1 = 0x0004;
	P2 = 0x0004;
	P3 = 0x0004;
	P4 = 0x0004;
	FP = 0x0004;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym i3, DATA_ADDR_6, 0x00;
	P3 = I1; SP = I3;
	R6 = [ SP ++ P5 ];
	R5 = [ SP ++ P1 ];
	R4 = [ SP ++ P2 ];
	R3 = [ SP ++ P3 ];
	R2 = [ SP ++ P4 ];
	R0 = [ SP ++ P1 ];
	R1 = [ SP ++ FP ];
	CHECKREG r0, 0x6E0AA0E5;
	CHECKREG r1, 0x7E19B0E6;
	CHECKREG r2, 0x5E1B90E4;
	CHECKREG r3, 0x4E2C80E3;
	CHECKREG r4, 0x3E0370E2;
	CHECKREG r5, 0x2E4E60E1;
	CHECKREG r6, 0x1E5F50E0;

// initial values
	imm32 r0, 0x10ff50f0;
	imm32 r1, 0x20fe60f1;
	imm32 r2, 0x30fd70f2;
	imm32 r3, 0x40fc80f3;
	imm32 r4, 0x55fb90f4;
	imm32 r5, 0x60faa0f5;
	imm32 r6, 0x70f9b0f6;
	imm32 r7, 0x80f8c0f7;
	P5 = 0x0004;
	P1 = 0x0004;
	P2 = 0x0004;
	P3 = 0x0004;
	P4 = 0x0004;
	FP = 0x1004 (X);
	SP = 0x0004;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym fp, DATA_ADDR_7, 0x00;
	P3 = I1; SP = I3;
	[ FP ++ P5 ] = R0;
	[ FP ++ P1 ] = R1;
	[ FP ++ P2 ] = R2;
	[ FP ++ P3 ] = R3;
	[ FP ++ P4 ] = R4;
	[ FP ++ SP ] = R5;
	[ FP ++ P1 ] = R6;
	P5 = 0x0004;
	P1 = 0x0004;
	P2 = 0x0004;
	P3 = 0x0004;
	P4 = 0x0004;
	SP = 0x0004;
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym fp, DATA_ADDR_7, 0x00;
	P3 = I1; SP = I3;
	R3 = [ FP ++ P5 ];
	R4 = [ FP ++ P1 ];
	R0 = [ FP ++ P2 ];
	R1 = [ FP ++ P3 ];
	R2 = [ FP ++ P4 ];
	R5 = [ FP ++ SP ];
	R6 = [ FP ++ P1 ];
	CHECKREG r0, 0x30FD70F2;
	CHECKREG r1, 0x40FC80F3;
	CHECKREG r2, 0x55FB90F4;
	CHECKREG r3, 0x10FF50F0;
	CHECKREG r4, 0x20FE60F1;
	CHECKREG r5, 0x60FAA0F5;
	CHECKREG r6, 0x70F9B0F6;

	P3 = I0; SP = I2;
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
	.dd 0x11223344
	.dd 0x55667788
	.dd 0x99717273
	.dd 0x74757677
	.dd 0x82838485
	.dd 0x86878889
	.dd 0x80818283
	.dd 0x84858687
	.dd 0x01020304
	.dd 0x05060708
	.dd 0x09101112
	.dd 0x14151617
	.dd 0x18192021
	.dd 0x22232425
	.dd 0x26272829
	.dd 0x30313233
	.dd 0x34353637
	.dd 0x38394041
	.dd 0x42434445
	.dd 0x46474849
	.dd 0x50515253
	.dd 0x54555657
	.dd 0x58596061
	.dd 0x62636465
	.dd 0x66676869
	.dd 0x74555657
	.dd 0x78596067
	.dd 0x72636467
	.dd 0x76676867

DATA_ADDR_2:
	.dd 0x20212223
	.dd 0x24252627
	.dd 0x28292A2B
	.dd 0x2C2D2E2F
	.dd 0x30313233
	.dd 0x34353637
	.dd 0x38393A3B
	.dd 0x3C3D3E3F
	.dd 0x91929394
	.dd 0x95969798
	.dd 0x99A1A2A3
	.dd 0xA5A6A7A8
	.dd 0xA9B0B1B2
	.dd 0xB3B4B5B6
	.dd 0xB7B8B9C0
	.dd 0x70717273
	.dd 0x74757677
	.dd 0x78798081
	.dd 0x82838485
	.dd 0x86C283C4
	.dd 0x81C283C4
	.dd 0x82C283C4
	.dd 0x83C283C4
	.dd 0x84C283C4
	.dd 0x85C283C4
	.dd 0x86C283C4
	.dd 0x87C288C4
	.dd 0x88C283C4
	.dd 0x89C283C4
	.dd 0x80C283C4
	.dd 0x81C283C4
	.dd 0x82C288C4
	.dd 0x94555659
	.dd 0x98596069
	.dd 0x92636469
	.dd 0x96676869

DATA_ADDR_3:
	.dd 0x40414243
	.dd 0x44454647
	.dd 0x48494A4B
	.dd 0x4C4D4E4F
	.dd 0x50515253
	.dd 0x54555657
	.dd 0x58595A5B
	.dd 0xC5C6C7C8
	.dd 0xC9CACBCD
	.dd 0xCFD0D1D2
	.dd 0xD3D4D5D6
	.dd 0xD7D8D9DA
	.dd 0xDBDCDDDE
	.dd 0xDFE0E1E2
	.dd 0xE3E4E5E6
	.dd 0x91E899EA
	.dd 0x92E899EA
	.dd 0x93E899EA
	.dd 0x94E899EA
	.dd 0x95E899EA
	.dd 0x96E899EA
	.dd 0x97E899EA
	.dd 0x98E899EA
	.dd 0x99E899EA
	.dd 0x91E899EA
	.dd 0x92E899EA
	.dd 0x93E899EA
	.dd 0x94E899EA
	.dd 0x95E899EA
	.dd 0x96E899EA
	.dd 0x977899EA
	.dd 0xa455565a
	.dd 0xa859606a
	.dd 0xa263646a
	.dd 0xa667686a

DATA_ADDR_4:
	.dd 0x60616263
	.dd 0x64656667
	.dd 0x68696A6B
	.dd 0x6C6D6E6F
	.dd 0x70717273
	.dd 0x74757677
	.dd 0x78797A7B
	.dd 0x7C7D7E7F
	.dd 0xEBECEDEE
	.dd 0xF3F4F5F6
	.dd 0xF7F8F9FA
	.dd 0xFBFCFDFE
	.dd 0xFF000102
	.dd 0x03040506
	.dd 0x0708090A
	.dd 0x0B0CAD0E
	.dd 0xAB0CAD01
	.dd 0xAB0CAD02
	.dd 0xAB0CAD03
	.dd 0xAB0CAD04
	.dd 0xAB0CAD05
	.dd 0xAB0CAD06
	.dd 0xAB0CAA07
	.dd 0xAB0CAD08
	.dd 0xAB0CAD09
	.dd 0xA00CAD1E
	.dd 0xA10CAD2E
	.dd 0xA20CAD3E
	.dd 0xA30CAD4E
	.dd 0xA40CAD5E
	.dd 0xA50CAD6E
	.dd 0xA60CAD7E
	.dd 0xB455565B
	.dd 0xB859606B
	.dd 0xB263646B
	.dd 0xB667686B

DATA_ADDR_5:
	.dd 0x80818283
	.dd 0x84858687
	.dd 0x88898A8B
	.dd 0x8C8D8E8F
	.dd 0x90919293
	.dd 0x94959697
	.dd 0x98999A9B
	.dd 0x9C9D9E9F
	.dd 0x0F101213
	.dd 0x14151617
	.dd 0x18191A1B
	.dd 0x1C1D1E1F
	.dd 0x20212223
	.dd 0x24252627
	.dd 0x28292A2B
	.dd 0x2C2D2E2F
	.dd 0xBC0DBE21
	.dd 0xBC1DBE22
	.dd 0xBC2DBE23
	.dd 0xBC3DBE24
	.dd 0xBC4DBE65
	.dd 0xBC5DBE27
	.dd 0xBC6DBE28
	.dd 0xBC7DBE29
	.dd 0xBC8DBE2F
	.dd 0xBC9DBE20
	.dd 0xBCADBE21
	.dd 0xBCBDBE2F
	.dd 0xBCCDBE23
	.dd 0xBCDDBE24
	.dd 0xBCFDBE25
	.dd 0xC455565C
	.dd 0xC859606C
	.dd 0xC263646C
	.dd 0xC667686C
	.dd 0xCC0DBE2C

DATA_ADDR_6:
	.dd 0x00010203
	.dd 0x04050607
	.dd 0x08090A0B
	.dd 0x0C0D0E0F
	.dd 0x10111213
	.dd 0x14151617
	.dd 0x18191A1B
	.dd 0x1C1D1E1F
	.dd 0x20212223
	.dd 0x24252627
	.dd 0x28292A2B
	.dd 0x2C2D2E2F
	.dd 0x30313233
	.dd 0x34353637
	.dd 0x38393A3B
	.dd 0x3C3D3E3F
	.dd 0x40414243
	.dd 0x44454647
	.dd 0x48494A4B
	.dd 0x4C4D4E4F
	.dd 0x50515253
	.dd 0x54555657
	.dd 0x58595A5B
	.dd 0x5C5D5E5F
	.dd 0x60616263
	.dd 0x64656667
	.dd 0x68696A6B
	.dd 0x6C6D6E6F
	.dd 0x70717273
	.dd 0x74757677
	.dd 0x78797A7B
	.dd 0x7C7D7E7F

DATA_ADDR_7:
	.dd 0x80818283
	.dd 0x84858687
	.dd 0x88898A8B
	.dd 0x8C8D8E8F
	.dd 0x90919293
	.dd 0x94959697
	.dd 0x98999A9B
	.dd 0x9C9D9E9F
	.dd 0xA0A1A2A3
	.dd 0xA4A5A6A7
	.dd 0xA8A9AAAB
	.dd 0xACADAEAF
	.dd 0xB0B1B2B3
	.dd 0xB4B5B6B7
	.dd 0xB8B9BABB
	.dd 0xBCBDBEBF
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
