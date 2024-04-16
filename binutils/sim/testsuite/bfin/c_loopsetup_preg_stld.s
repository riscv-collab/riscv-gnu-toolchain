//Original:/testcases/core/c_loopsetup_preg_stld/c_loopsetup_preg_stld.dsp
// Spec Reference: loopsetup preg st & ld
# mach: bfin

.include "testutils.inc"
	start

	INIT_R_REGS 0;

	A0 = 0;
	A1 = 0;
	ASTAT = r0;

	P1 = 9;
	P2 = 8;
	P0 = 7;
	P4 = 6;
	P5 = 5;
	FP = 3;

	imm32 r0, 0x00200005;
	imm32 r1, 0x00300010;
	imm32 r2, 0x00500012;
	imm32 r3, 0x00600024;
	imm32 r4, 0x00700016;
	imm32 r5, 0x00900028;
	imm32 r6, 0x0a000030;
	imm32 r7, 0x00b00044;

	loadsym I0, DATA0;
	loadsym I1, DATA1;
	R0 = [ I0 ++ ];
	R1 = [ I1 ++ ];
	LSETUP ( start1 , end1 ) LC0 = P1;
start1:
	R0 += 1;
	R1 += 2;
	A1 += R0.H * R1.H, A0 += R0.L * R1.L || R0 = [ I0 ++ ] || R1 = [ I1 ++ ];
end1:
	R2 += 3;

	R3 = ( A0 += A1 );

	A0 = 0;
	A1 = 0;
	LSETUP ( start2 , end2 ) LC0 = P2;
start2:
	R4 += 4;
	A1 += R0.H * R1.H, A0 += R0.L * R1.L || R0 = [ I0 -- ] || R1 = [ I1 -- ];
end2:
	R5 += -5;
	R6 = ( A0 += A1 );
	CHECKREG r0, 0x000D0003;
	CHECKREG r1, 0x00C00103;
	CHECKREG r2, 0x0050002D;
	CHECKREG r3, 0x00010794;
	CHECKREG r4, 0x00700036;
	CHECKREG r5, 0x00900000;
	CHECKREG r6, 0x00011388;
	CHECKREG r7, 0x00B00044;

	imm32 r0, 0x01200805;
	imm32 r1, 0x02300710;
	imm32 r2, 0x03500612;
	imm32 r3, 0x04600524;
	imm32 r4, 0x05700416;
	imm32 r5, 0x06900328;
	imm32 r6, 0x0a700230;
	imm32 r7, 0x08b00044;

	loadsym I2, DATA0;
	loadsym I3, DATA1;
	[ I2 ++ ] = R0;
	[ I3 ++ ] = R1;
	LSETUP ( start3 , end3 ) LC0 = P1;
start3:
	[ I2 ++ ] = R2;
	[ I3 ++ ] = R3;
	R2 += 1;
end3:
	R3 += 1;

	A0 = 0;
	A1 = 0;
	LSETUP ( start4 , end4 ) LC0 = P2;
	R0 = [ I0 -- ];
	R1 = [ I1 -- ];
start4:
	A1 += R0.H * R1.H, A0 += R0.L * R1.L || R0 = [ I2 -- ] || R1 = [ I3 -- ];
	R4 = R4 + R0;	// comp3op
end4:
	R5 = R5 + R1;

	R6 = ( A0 += A1 );
	CHECKREG r0, 0x03500614;
	CHECKREG r1, 0x04600526;
	CHECKREG r2, 0x0350061B;
	CHECKREG r3, 0x0460052D;
	CHECKREG r4, 0x1CF02EC1;
	CHECKREG r5, 0x25602851;
	CHECKREG r6, 0x0282F220;
	CHECKREG r7, 0x08B00044;

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
