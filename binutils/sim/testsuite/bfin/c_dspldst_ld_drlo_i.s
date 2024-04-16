//Original:/testcases/core/c_dspldst_ld_drlo_i/c_dspldst_ld_drlo_i.dsp
// Spec Reference: c_dspldst ld_drlo_i
# mach: bfin

.include "testutils.inc"
	start

	INIT_R_REGS 0;

	loadsym i0, DATA_ADDR_3;
	loadsym i1, DATA_ADDR_4;
	loadsym i2, DATA_ADDR_5;
	loadsym i3, DATA_ADDR_6;

// Load Lower half of Dregs
	R0.L = W [ I0 ];
	R1.L = W [ I1 ];
	R2.L = W [ I2 ];
	R3.L = W [ I3 ];
	R4.L = W [ I0 ];
	R5.L = W [ I1 ];
	R6.L = W [ I2 ];
	R7.L = W [ I3 ];
	CHECKREG r0, 0x00000203;
	CHECKREG r1, 0x00002223;
	CHECKREG r2, 0x00004243;
	CHECKREG r3, 0x00006263;
	CHECKREG r4, 0x00000203;
	CHECKREG r5, 0x00002223;
	CHECKREG r6, 0x00004243;
	CHECKREG r7, 0x00006263;

	R1.L = W [ I0 ];
	R2.L = W [ I1 ];
	R3.L = W [ I2 ];
	R4.L = W [ I3 ];
	R5.L = W [ I0 ];
	R6.L = W [ I1 ];
	R7.L = W [ I2 ];
	R0.L = W [ I3 ];
	CHECKREG r0, 0x00006263;
	CHECKREG r1, 0x00000203;
	CHECKREG r2, 0x00002223;
	CHECKREG r3, 0x00004243;
	CHECKREG r4, 0x00006263;
	CHECKREG r5, 0x00000203;
	CHECKREG r6, 0x00002223;
	CHECKREG r7, 0x00004243;

	R2.L = W [ I0 ];
	R3.L = W [ I1 ];
	R4.L = W [ I2 ];
	R5.L = W [ I3 ];
	R6.L = W [ I0 ];
	R7.L = W [ I1 ];
	R0.L = W [ I2 ];
	R1.L = W [ I3 ];
	CHECKREG r0, 0x00004243;
	CHECKREG r1, 0x00006263;
	CHECKREG r2, 0x00000203;
	CHECKREG r3, 0x00002223;
	CHECKREG r4, 0x00004243;
	CHECKREG r5, 0x00006263;
	CHECKREG r6, 0x00000203;
	CHECKREG r7, 0x00002223;

	R3.L = W [ I0 ];
	R4.L = W [ I1 ];
	R5.L = W [ I2 ];
	R6.L = W [ I3 ];
	R7.L = W [ I0 ];
	R0.L = W [ I1 ];
	R1.L = W [ I2 ];
	R2.L = W [ I3 ];
	CHECKREG r0, 0x00002223;
	CHECKREG r1, 0x00004243;
	CHECKREG r2, 0x00006263;
	CHECKREG r3, 0x00000203;
	CHECKREG r4, 0x00002223;
	CHECKREG r5, 0x00004243;
	CHECKREG r6, 0x00006263;
	CHECKREG r7, 0x00000203;

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

DATA_ADDR_4:
	.dd 0x20212223
	.dd 0x24252627
	.dd 0x28292A2B
	.dd 0x2C2D2E2F
	.dd 0x30313233
	.dd 0x34353637
	.dd 0x38393A3B
	.dd 0x3C3D3E3F

DATA_ADDR_5:
	.dd 0x40414243
	.dd 0x44454647
	.dd 0x48494A4B
	.dd 0x4C4D4E4F
	.dd 0x50515253
	.dd 0x54555657
	.dd 0x58595A5B
	.dd 0x5C5D5E5F

DATA_ADDR_6:
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
