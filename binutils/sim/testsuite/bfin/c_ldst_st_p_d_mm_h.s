//Original:testcases/core/c_ldst_st_p_d_mm_h/c_ldst_st_p_d_mm_h.dsp
// Spec Reference: c_ldst st_p-- h half
# mach: bfin

.include "testutils.inc"
	start

// set all regs
INIT_I_REGS -1;
INIT_R_REGS 0;
init_b_regs 0;
init_l_regs 0;
init_m_regs -1;
I0 = P3;
I2 = SP;

	imm32 r0, 0x0a231507;
	imm32 r1, 0x1b342618;
	imm32 r2, 0x2c453729;
	imm32 r3, 0x3d56483a;
	imm32 r4, 0x4e67594b;
	imm32 r5, 0x5f786a5c;
	imm32 r6, 0x60897b6d;
	imm32 r7, 0x719a8c7e;

// reset  values
	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym p5, DATA_ADDR_1, 0x20;
	loadsym p1, DATA_ADDR_2, 0x20;
	loadsym p2, DATA_ADDR_3, 0x20;
	loadsym i1, DATA_ADDR_4, 0x20;
	loadsym p4, DATA_ADDR_5, 0x20;
	loadsym fp, DATA_ADDR_6, 0x20;
	loadsym i3, DATA_ADDR_7, 0x20;
	P3 = I1; SP = I3;

	W [ P5 -- ] = R0;
	W [ P1 -- ] = R1;
	W [ P2 -- ] = R2;
	W [ P3 -- ] = R3;
	W [ P4 -- ] = R4;
	W [ FP -- ] = R5;
	W [ SP -- ] = R6;

	W [ P5 -- ] = R1;
	W [ P1 -- ] = R2;
	W [ P2 -- ] = R3;
	W [ P3 -- ] = R4;
	W [ P4 -- ] = R5;
	W [ FP -- ] = R6;
	W [ SP -- ] = R7;

	W [ P5 -- ] = R2;
	W [ P1 -- ] = R3;
	W [ P2 -- ] = R4;
	W [ P3 -- ] = R5;
	W [ P4 -- ] = R6;
	W [ FP -- ] = R7;
	W [ SP -- ] = R0;

	W [ P5 -- ] = R3;
	W [ P1 -- ] = R4;
	W [ P2 -- ] = R5;
	W [ P3 -- ] = R6;
	W [ P4 -- ] = R7;
	W [ FP -- ] = R0;
	W [ SP -- ] = R1;

	W [ P5 -- ] = R4;
	W [ P1 -- ] = R5;
	W [ P2 -- ] = R6;
	W [ P3 -- ] = R7;
	W [ P4 -- ] = R0;
	W [ FP -- ] = R1;
	W [ SP -- ] = R2;

	W [ P5 -- ] = R5;
	W [ P1 -- ] = R6;
	W [ P2 -- ] = R7;
	W [ P3 -- ] = R0;
	W [ P4 -- ] = R1;
	W [ FP -- ] = R2;
	W [ SP -- ] = R3;

	W [ P5 -- ] = R6;
	W [ P1 -- ] = R7;
	W [ P2 -- ] = R0;
	W [ P3 -- ] = R1;
	W [ P4 -- ] = R2;
	W [ FP -- ] = R3;
	W [ SP -- ] = R4;

	W [ P5 -- ] = R7;
	W [ P1 -- ] = R0;
	W [ P2 -- ] = R1;
	W [ P3 -- ] = R2;
	W [ P4 -- ] = R3;
	W [ FP -- ] = R4;
	W [ SP -- ] = R5;

	I1 = P3; P3 = I0; I3 = SP; SP = I2;
	loadsym p5, DATA_ADDR_1, 0x20;
	loadsym p1, DATA_ADDR_2, 0x20;
	loadsym p2, DATA_ADDR_3, 0x20;
	loadsym i1, DATA_ADDR_4, 0x20;
	loadsym p4, DATA_ADDR_5, 0x20;
	loadsym fp, DATA_ADDR_6, 0x20;
	loadsym i3, DATA_ADDR_7, 0x20;
	 P3 = I1; SP = I3;

	R0 = [ P1 -- ];
	R1 = [ P2 -- ];
	R2 = [ P3 -- ];
	R3 = [ P4 -- ];
	R4 = [ P5 -- ];
	R5 = [ FP -- ];
	R6 = [ SP -- ];
	CHECKREG r1, 0x00003729;
	CHECKREG r2, 0x0000483A;
	CHECKREG r3, 0x0000594B;
	CHECKREG r4, 0x00001507;
	CHECKREG r5, 0x00006A5C;
	CHECKREG r6, 0xE0E17B6D;
	CHECKREG r7, 0x719A8C7E;
	R1 = [ P1 -- ];
	R2 = [ P2 -- ];
	R3 = [ P3 -- ];
	R4 = [ P4 -- ];
	R5 = [ P5 -- ];
	R6 = [ FP -- ];
	R7 = [ SP -- ];
	CHECKREG r1, 0x3729483A;
	CHECKREG r2, 0x483A594B;
	CHECKREG r3, 0x594B6A5C;
	CHECKREG r4, 0x6A5C7B6D;
	CHECKREG r5, 0x26183729;
	CHECKREG r6, 0x7B6D8C7E;
	CHECKREG r7, 0x8C7E1507;
	R3 = [ P1 -- ];
	R4 = [ P2 -- ];
	R5 = [ P3 -- ];
	R6 = [ P4 -- ];
	R7 = [ P5 -- ];
	R0 = [ FP -- ];
	R1 = [ SP -- ];
	CHECKREG r1, 0x26183729;
	CHECKREG r2, 0x483A594B;
	CHECKREG r3, 0x594B6A5C;
	CHECKREG r4, 0x6A5C7B6D;
	CHECKREG r5, 0x7B6D8C7E;
	CHECKREG r6, 0x8C7E1507;
	CHECKREG r7, 0x483A594B;
	R3 = [ P1 -- ];
	R4 = [ P2 -- ];
	R5 = [ P3 -- ];
	R6 = [ P4 -- ];
	R7 = [ P5 -- ];
	R0 = [ FP -- ];
	R1 = [ SP -- ];
	CHECKREG r1, 0x483A594B;
	CHECKREG r2, 0x483A594B;
	CHECKREG r3, 0x7B6D8C7E;
	CHECKREG r4, 0x8C7E1507;
	CHECKREG r5, 0x15072618;
	CHECKREG r6, 0x26183729;
	CHECKREG r7, 0x6A5C7B6D;
	R4 = [ P1 -- ];
	R5 = [ P2 -- ];
	R6 = [ P3 -- ];
	R7 = [ P4 -- ];
	R0 = [ P5 -- ];
	R1 = [ FP -- ];
	R2 = [ SP -- ];
	CHECKREG r1, 0x594BB2B3;
	CHECKREG r2, 0x6A5CD2D3;
	CHECKREG r3, 0x7B6D8C7E;
	CHECKREG r4, 0x15073233;
	CHECKREG r5, 0x26185253;
	CHECKREG r6, 0x37297273;
	CHECKREG r7, 0x483A9293;
	R5 = [ P1 -- ];
	R6 = [ P2 -- ];
	R7 = [ P3 -- ];
	R0 = [ P4 -- ];
	R1 = [ P5 -- ];
	R2 = [ FP -- ];
	R3 = [ SP -- ];
	CHECKREG r1, 0x0C0D0E0F;
	CHECKREG r2, 0xACADAEAF;
	CHECKREG r3, 0xCCCDCECF;
	CHECKREG r4, 0x15073233;
	CHECKREG r5, 0x2C2D2E2F;
	CHECKREG r6, 0x4C4D4E4F;
	CHECKREG r7, 0x6C6D6E6F;
	R6 = [ P1 -- ];
	R7 = [ P2 -- ];
	R0 = [ P3 -- ];
	R1 = [ P4 -- ];
	R2 = [ P5 -- ];
	R3 = [ FP -- ];
	R0 = [ SP -- ];
	CHECKREG r1, 0x88898A8B;
	CHECKREG r2, 0x08090A0B;
	CHECKREG r3, 0xA8A9AAAB;
	CHECKREG r4, 0x15073233;
	CHECKREG r5, 0x2C2D2E2F;
	CHECKREG r6, 0x28292A2B;
	CHECKREG r7, 0x48494A4B;

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
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000

DATA_ADDR_2:
	.dd 0x20212223
	.dd 0x24252627
	.dd 0x28292A2B
	.dd 0x2C2D2E2F
	.dd 0x30313233
	.dd 0x34353637
	.dd 0x38393A3B
	.dd 0x3C3D3E3F
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000

DATA_ADDR_3:
	.dd 0x40414243
	.dd 0x44454647
	.dd 0x48494A4B
	.dd 0x4C4D4E4F
	.dd 0x50515253
	.dd 0x54555657
	.dd 0x58595A5B
	.dd 0x5C5D5E5F
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000

DATA_ADDR_4:
	.dd 0x60616263
	.dd 0x64656667
	.dd 0x68696A6B
	.dd 0x6C6D6E6F
	.dd 0x70717273
	.dd 0x74757677
	.dd 0x78797A7B
	.dd 0x7C7D7E7F
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000

DATA_ADDR_5:
	.dd 0x80818283
	.dd 0x84858687
	.dd 0x88898A8B
	.dd 0x8C8D8E8F
	.dd 0x90919293
	.dd 0x94959697
	.dd 0x98999A9B
	.dd 0x9C9D9E9F
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000

DATA_ADDR_6:
	.dd 0xA0A1A2A3
	.dd 0xA4A5A6A7
	.dd 0xA8A9AAAB
	.dd 0xACADAEAF
	.dd 0xB0B1B2B3
	.dd 0xB4B5B6B7
	.dd 0xB8B9BABB
	.dd 0xBCBDBEBF
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000

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
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
	.dd 0x00000000
