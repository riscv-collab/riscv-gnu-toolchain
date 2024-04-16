//Original:/testcases/core/c_multi_issue_dsp_ldst_1/c_multi_issue_dsp_ldst_1.dsp
// Spec Reference: dsp32mac and 2 load/store
# mach: bfin

.include "testutils.inc"
	start

	INIT_R_REGS 0;


	imm32 r0, 0x00000000;
	A0 = 0;
	A1 = 0;
	ASTAT = r0;

	loadsym I0, DATA0;
	loadsym I1, DATA1;

	loadsym P1, DATA0;
	loadsym P2, DATA1;

// test the default (signed fraction : left )
	imm32 r0, 0x12345678;
	imm32 r1, 0x33456789;
	imm32 r2, 0x5556789a;
	imm32 r3, 0x75678912;
	imm32 r4, 0x86789123;
	imm32 r5, 0xa7891234;
	imm32 r6, 0xc1234567;
	imm32 r7, 0xf1234567;
	A1 = R0.L * R1.L, A0 = R0.L * R1.L || R0 = [ I0 ++ ] || R1 = [ I1 ++ ];
	A1 += R2.L * R3.L, A0 += R2.L * R3.H || R2 = [ I0 ++ ] || R3 = [ I1 ++ ];
	A1 += R6.H * R7.H, A0 += R6.H * R7.L || R4 = [ P1 ++ ] || [ I1 ++ ] = R5;
	R6 = A0.w;
	R7 = A1.w;
	CHECKREG r0, 0x000A0000;
	CHECKREG r1, 0x00F00100;
	CHECKREG r2, 0x000B0001;
	CHECKREG r3, 0x00E00101;
	CHECKREG r4, 0x000A0000;
	CHECKREG r5, 0xA7891234;
	CHECKREG r6, 0x92793486;
	CHECKREG r7, 0xDD2F9BAA;

	imm32 r0, 0x12245618;
	imm32 r1, 0x23256719;
	imm32 r2, 0x3426781a;
	imm32 r3, 0x45278912;
	imm32 r4, 0x56289113;
	imm32 r5, 0x67291214;
	imm32 r6, 0xa1234517;
	imm32 r7, 0xc1234517;
	A1 = R0.L * R1.L, A0 = R0.L * R1.L || R4 = [ P1 ++ ] || [ I0 ++ ] = R6;
	A1 -= R2.L * R3.L, A0 += R2.L * R3.H || R2 = [ P2 ++ ] || [ I1 ++ ] = R3;
	A1 += R4.H * R6.H, A0 -= R4.H * R6.L || [ P2 ++ ] = R5 || R7 = [ I1 ++ ];
	R6 = A0.w;
	R7 = A1.w;
	CHECKREG r0, 0x12245618;
	CHECKREG r1, 0x23256719;
	CHECKREG r2, 0x00F00100;
	CHECKREG r3, 0x45278912;
	CHECKREG r4, 0x000B0001;
	CHECKREG r5, 0x67291214;
	CHECKREG r6, 0x8634CCA2;
	CHECKREG r7, 0xB4E7420A;

	imm32 r0, 0x15245648;
	imm32 r1, 0x25256749;
	imm32 r2, 0x3526784a;
	imm32 r3, 0x45278942;
	imm32 r4, 0x55389143;
	imm32 r5, 0x65391244;
	imm32 r6, 0xa5334547;
	imm32 r7, 0xc5334547;
	A1 += R0.H * R1.H, A0 += R0.L * R1.L || R2 = [ P1 ++ ] || [ I1 -- ] = R3;
	A1 += R2.H * R3.H, A0 += R2.L * R3.H || NOP || [ I0 ++ ] = R2;
	A1 = R4.H * R5.L, A0 += R4.H * R5.L || R3 = [ P2 -- ] || R6 = [ I0 -- ];
	R6 = A0.w;
	R7 = A1.w;
	CHECKREG r0, 0x15245648;
	CHECKREG r1, 0x25256749;
	CHECKREG r2, 0xA1234517;
	CHECKREG r3, 0xA7891234;
	CHECKREG r4, 0x55389143;
	CHECKREG r5, 0x65391244;
	CHECKREG r6, 0xFD508A74;
	CHECKREG r7, 0x0C2925C0;

	imm32 r1, 0x02450789;
	imm32 r2, 0x0356089a;
	imm32 r3, 0x04670912;
	imm32 r4, 0x05780123;
	imm32 r5, 0x06890234;
	imm32 r6, 0x07230567;
	imm32 r7, 0x00230567;
	R2 = R0 +|+ R7, R4 = R0 -|- R7 (ASR) || R0 = [ I1 ++ ] || [ I0 -- ] = R2;
	R1 = R6 +|+ R3, R5 = R6 -|- R3 || R6 = [ P1 ] || [ I0 -- ] = R3;
	R5 = R4 +|+ R2, R0 = R4 -|- R2 (CO) || NOP || [ I0 ++ ] = R5;
	CHECKREG r0, 0xFA99FFDD;
	CHECKREG r1, 0x0B8A0E79;
	CHECKREG r2, 0x0AA32DD7;
	CHECKREG r3, 0x04670912;
	CHECKREG r4, 0x0A802870;
	CHECKREG r5, 0x15235647;
	CHECKREG r6, 0x0356089A;
	CHECKREG r7, 0x00230567;

	pass

	.data
DATA0:
	.dd 0x000a0000
	.dd 0x000b0001
	.dd 0x000c0002
	.dd 0x000d0003
	.dd 0x000e0004
	.dd 0x000f0005
	.dd 0x00100006
	.dd 0x00200007
	.dd 0x00300008
	.dd 0x00400009
	.dd 0x0050000a
	.dd 0x0060000b
	.dd 0x0070000c
	.dd 0x0080000d
	.dd 0x0090000e
	.dd 0x0100000f
	.dd 0x02000010
	.dd 0x03000011
	.dd 0x04000012
	.dd 0x05000013
	.dd 0x06000014
	.dd 0x001a0000
	.dd 0x001b0001
	.dd 0x001c0002
	.dd 0x001d0003
	.dd 0x00010004
	.dd 0x00010005
	.dd 0x02100006
	.dd 0x02200007
	.dd 0x02300008
	.dd 0x02200009
	.dd 0x0250000a
	.dd 0x0260000b
	.dd 0x0270000c
	.dd 0x0280000d
	.dd 0x0290000e
	.dd 0x2100000f
	.dd 0x22000010
	.dd 0x22000011
	.dd 0x24000012
	.dd 0x25000013
	.dd 0x26000014

DATA1:
	.dd 0x00f00100
	.dd 0x00e00101
	.dd 0x00d00102
	.dd 0x00c00103
	.dd 0x00b00104
	.dd 0x00a00105
	.dd 0x00900106
	.dd 0x00800107
	.dd 0x00100108
	.dd 0x00200109
	.dd 0x0030010a
	.dd 0x0040010b
	.dd 0x0050011c
	.dd 0x0060010d
	.dd 0x0070010e
	.dd 0x0080010f
	.dd 0x00900110
	.dd 0x01000111
	.dd 0x02000112
	.dd 0x03000113
	.dd 0x04000114
	.dd 0x05000115
	.dd 0x03f00100
	.dd 0x03e00101
	.dd 0x03d00102
	.dd 0x03c00103
	.dd 0x03b00104
	.dd 0x03a00105
	.dd 0x03900106
	.dd 0x03800107
	.dd 0x03100108
	.dd 0x03200109
	.dd 0x0330010a
	.dd 0x0330010b
	.dd 0x0350011c
	.dd 0x0360010d
	.dd 0x0370010e
	.dd 0x0380010f
	.dd 0x03900110
	.dd 0x31000111
	.dd 0x32000112
	.dd 0x33000113
	.dd 0x34000114
