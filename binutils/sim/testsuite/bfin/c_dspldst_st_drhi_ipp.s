//Original:testcases/core/c_dspldst_st_drhi_ipp/c_dspldst_st_drhi_ipp.dsp
// Spec Reference: c_dspldst st_drhi_ipp
# mach: bfin

.include "testutils.inc"
	start

// set all regs

INIT_I_REGS -1;
init_b_regs 0;
init_l_regs 0;
init_m_regs -1;

// Half reg 16 bit mem store

	imm32 r0, 0x0a123456;
	imm32 r1, 0x11b12345;
	imm32 r2, 0x222c1234;
	imm32 r3, 0x3344d012;
	imm32 r4, 0x5566e012;
	imm32 r5, 0x789abf01;
	imm32 r6, 0xabcd0123;
	imm32 r7, 0x01234567;

// initial values
	loadsym i0, DATA_ADDR_3;
	loadsym i1, DATA_ADDR_4;
	loadsym i2, DATA_ADDR_5;
	loadsym i3, DATA_ADDR_6;

	W [ I0 ++ ] = R0.H;
	W [ I1 ++ ] = R1.H;
	W [ I2 ++ ] = R2.H;
	W [ I3 ++ ] = R3.H;
	W [ I0 ++ ] = R1.H;
	W [ I1 ++ ] = R2.H;
	W [ I2 ++ ] = R3.H;
	W [ I3 ++ ] = R4.H;

	W [ I0 ++ ] = R3.H;
	W [ I1 ++ ] = R4.H;
	W [ I2 ++ ] = R5.H;
	W [ I3 ++ ] = R6.H;

	W [ I0 ++ ] = R4.H;
	W [ I1 ++ ] = R5.H;
	W [ I2 ++ ] = R6.H;
	W [ I3 ++ ] = R7.H;
	loadsym i0, DATA_ADDR_3;
	loadsym i1, DATA_ADDR_4;
	loadsym i2, DATA_ADDR_5;
	loadsym i3, DATA_ADDR_6;
	R0 = [ I0 ++ ];
	R1 = [ I1 ++ ];
	R2 = [ I2 ++ ];
	R3 = [ I3 ++ ];
	R4 = [ I0 ++ ];
	R5 = [ I1 ++ ];
	R6 = [ I2 ++ ];
	R7 = [ I3 ++ ];
	CHECKREG r0, 0x11B10A12;
	CHECKREG r1, 0x222C11B1;
	CHECKREG r2, 0x3344222C;
	CHECKREG r3, 0x55663344;
	CHECKREG r4, 0x55663344;
	CHECKREG r5, 0x789A5566;
	CHECKREG r6, 0xABCD789A;
	CHECKREG r7, 0x0123ABCD;

	R0 = [ I0 ++ ];
	R1 = [ I1 ++ ];
	R2 = [ I2 ++ ];
	R3 = [ I3 ++ ];
	R4 = [ I0 ++ ];
	R5 = [ I1 ++ ];
	R6 = [ I2 ++ ];
	R7 = [ I3 ++ ];
	CHECKREG r0, 0x08090A0B;
	CHECKREG r1, 0x28292A2B;
	CHECKREG r2, 0x48494A4B;
	CHECKREG r3, 0x68696A6B;
	CHECKREG r4, 0x0C0D0E0F;
	CHECKREG r5, 0x2C2D2E2F;
	CHECKREG r6, 0x4C4D4E4F;
	CHECKREG r7, 0x6C6D6E6F;

// initial values

	imm32 r0, 0x01b2c3d4;
	imm32 r1, 0x10145618;
	imm32 r2, 0xa2016729;
	imm32 r3, 0xbb30183a;
	imm32 r4, 0xdec4014b;
	imm32 r5, 0x5f7d501c;
	imm32 r6, 0x3089eb01;
	imm32 r7, 0x719abf70;

	loadsym i0, DATA_ADDR_3, 0x20;
	loadsym i1, DATA_ADDR_4, 0x20;
	loadsym i2, DATA_ADDR_5, 0x20;
	loadsym i3, DATA_ADDR_6, 0x20;

	W [ I0 -- ] = R0.H;
	W [ I1 -- ] = R1.H;
	W [ I2 -- ] = R2.H;
	W [ I3 -- ] = R3.H;
	W [ I0 -- ] = R1.H;
	W [ I1 -- ] = R2.H;
	W [ I2 -- ] = R3.H;
	W [ I3 -- ] = R4.H;

	W [ I0 -- ] = R3.H;
	W [ I1 -- ] = R4.H;
	W [ I2 -- ] = R5.H;
	W [ I3 -- ] = R6.H;
	W [ I0 -- ] = R4.H;
	W [ I1 -- ] = R5.H;
	W [ I2 -- ] = R6.H;
	W [ I3 -- ] = R7.H;
	loadsym i0, DATA_ADDR_3, 0x20;
	loadsym i1, DATA_ADDR_4, 0x20;
	loadsym i2, DATA_ADDR_5, 0x20;
	loadsym i3, DATA_ADDR_6, 0x20;
	R0 = [ I0 -- ];
	R1 = [ I1 -- ];
	R2 = [ I2 -- ];
	R3 = [ I3 -- ];
	R4 = [ I0 -- ];
	R5 = [ I1 -- ];
	R6 = [ I2 -- ];
	R7 = [ I3 -- ];
	CHECKREG r0, 0x000001B2;
	CHECKREG r1, 0x00001014;
	CHECKREG r2, 0x0000A201;
	CHECKREG r3, 0x0000BB30;
	CHECKREG r4, 0x1014BB30;
	CHECKREG r5, 0xA201DEC4;
	CHECKREG r6, 0xBB305F7D;
	CHECKREG r7, 0xDEC43089;

	R0 = [ I0 -- ];
	R1 = [ I1 -- ];
	R2 = [ I2 -- ];
	R3 = [ I3 -- ];
	R4 = [ I0 -- ];
	R5 = [ I1 -- ];
	R6 = [ I2 -- ];
	R7 = [ I3 -- ];
	CHECKREG r0, 0xDEC41A1B;
	CHECKREG r1, 0x5F7D3A3B;
	CHECKREG r2, 0x30895A5B;
	CHECKREG r3, 0x719A7A7B;
	CHECKREG r4, 0x14151617;
	CHECKREG r5, 0x34353637;
	CHECKREG r6, 0x54555657;
	CHECKREG r7, 0x74757677;

	pass

// Pre-load memory with known data
// More data is defined than will actually be used

	.data
DATA_ADDR_3:
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

DATA_ADDR_4:
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

DATA_ADDR_5:
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

DATA_ADDR_6:
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

DATA_ADDR_7:
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

DATA_ADDR_8:
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
