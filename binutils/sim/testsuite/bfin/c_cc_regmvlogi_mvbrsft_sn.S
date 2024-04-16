//Original:/proj/frio/dv/testcases/core/c_cc_regmvlogi_mvbrsft_sn/c_cc_regmvlogi_mvbrsft_sn.dsp
// Spec Reference: cc: set (regmv & logi2op) used (ccmv & brcc & dsp32sft)
# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	INIT_I_REGS 0;
	INIT_M_REGS 0;
	INIT_L_REGS 0;
	INIT_B_REGS 0;
	INIT_R_REGS 0;
	INIT_P_REGS 0;
	ASTAT = R0;
	A0 = A1 = 0;

	imm32 r0, (_CC);	// cc=1
	imm32 r1, 0x00000000;	// cc=0
	imm32 r2, 0x62b61557;
	imm32 r3, 0x07300007;
	imm32 r4, 0x00740088;
	imm32 r5, 0x609950aa;
	imm32 r6, 0x20bb06cc;
	imm32 r7, 0x00000002;

	A0 = R4;
	A1 = R6;

	ASTAT = R0;	// cc=1  REGMV
	P2 = R2;
	R2 = R0 + R2;	// comp3op dr plus dr
	M0 = P2;	// regmv
	IF CC R1 = R3;	// ccmov
	ASTAT = R1;	// cc=0  REGMV
	R3 >>= R7;	// alu2op sft
	R3 = R0 + R2;	// comp3op dr plus dr
	I0 = R5;
	IF CC R3 = R2;	// ccmv
	CC = R0 < R1;	// ccflag
	R3.H = R1.L + R3.H (S);	// dsp32alu
	R5 = ( A1 = R7.L * R4.H ),  R4 = ( A0 = R7.H * R4.L );	// dsp32mac pair
	IF CC R4 = R5;	// ccmv
	CC = ! BITTST( R0 , 4 );	// cc = 0
	R0 = R0 +|- R1 , R6 = R0 -|+ R1 (ASR);	// dsp32alu sft
	I0 += 2;	// dagmodim
	IF CC R4 = R5;	// ccmv
	CC = BITTST ( R1 , 4 );	// cc = 0
	R7.L = R5.L << 1;	// dsp32shiftim
	R1 = R0 +|- R1 , R5 = R0 -|+ R1 (ASR);	// dsp32alu sft
	P1 = A0.w;
	IF !CC JUMP LABEL1;	// branch
	CC = ! CC;
	R1 = ( A1 = R7.L * R4.L ),  R0 = ( A0 = R7.H * R4.H )  (S2RND);	// dsp32mac pair
	I0 += M0;	// dagmodim
	P2 = A1.w;
	IF !CC JUMP LABEL2 (BP);	// branch
LABEL1:
	R2 = R0 + R2;
	JUMP.S END;
LABEL2:
	R7 = R5 - R3;
	CC = R0 < R1;	// ccflag
	R6 = R0 + R2;	// comp3op dr plus dr
	P4 = I0;
	IF CC JUMP END (BP);	// branch on
	R7 = R5 + R7;

END:

	CHECKREG r0, 0x0398000C;
	CHECKREG r1, 0x05640002;
	CHECKREG r2, 0x664E1583;
	CHECKREG r3, 0x62BD1597;
	CHECKREG r4, 0x000001D0;
	CHECKREG r5, 0xFE340009;
	CHECKREG r6, 0xFC680013;
	CHECKREG r7, 0x000003A0;
	CHECKREG p1, 0x00000000;
	CHECKREG p2, 0x62B61557;
	CHECKREG p4, 0x00000000;

	imm32 r0, (_CC);
	imm32 r1, 0x00000000;
	imm32 r2, 0x62661557;
	imm32 r3, 0x073b0007;
	imm32 r4, 0x01f49088;
	imm32 r5, 0x6e2959aa;
	imm32 r6, 0xa0b506cc;
	imm32 r7, 0xabd30002;

	A1 = A0 = 0;
	ASTAT = R0;	// cc=1  REGMV
	R2.H = R3.L + R4.L (NS);	// dsp32alu
	R2.H = ( A1 = R2.L * R3.L ), A0 = R2.H * R3.L;	// dsp32mac
	R3 = ROT R2 BY 1;	// dsp32shiftim_rot
	ASTAT = R1;	// cc=0  REGMV
	A1 += R2.L * R3.L, A0 += R2.L * R3.H;	// dsp32mac
	R2.L = R5.L << 1;	// dsp32shiftimm
	R5 = ROT R3 BY 1;	// dsp32shiftim_rot
	CC = ! BITTST( R0 , 4 );	// cc = 0
	R4.L = R5.L << 1;	// dsp32shiftimm
	R0 >>= R7;	// alu2op sft
	A0 += A1;	// dsp32alu a0 + a1
	R6 = ROT R4 BY 5;	// dsp32shiftim_rot
	CC = BITTST ( R1 , 4 );	// cc = 0
	R0 = R0 + R2;	// comp3op dr plus dr
	R5 = R3.L * R4.H, R4 = R3.H * R4.L;	// dsp32mult
	P1 = A0.w;
	IF CC R4 = R5;	// ccmov
	P1.L = 0x3000;	// ldimmhalf
	P2 = A1.w;	// regmv
	CC = BITTST ( R0 , 4 );	// cc = 1
	R7 = ROT R6 BY R7.L;

	CHECKREG r0, 0x0001B354;
	CHECKREG r1, 0x00000000;
	CHECKREG r2, 0x0001B354;
	CHECKREG r3, 0x00022AAF;
	CHECKREG r4, 0xFFFEAAF0;
	CHECKREG r5, 0x00A6BB98;
	CHECKREG r6, 0x3E955790;
	CHECKREG r7, 0xFA555E42;
	CHECKREG p1, 0x07193000;
	CHECKREG p2, 0x071EE3B4;

	pass
