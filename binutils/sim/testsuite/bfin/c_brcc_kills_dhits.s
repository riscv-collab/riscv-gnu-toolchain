//Original:/testcases/core/c_brcc_kills_dhits/c_brcc_kills_dhits.dsp
// Spec Reference: brcc kills  data cache hits
# mach: bfin

.include "testutils.inc"
	start

	imm32 r0, 0x00000000;
	imm32 r1, 0x00000001;
	imm32 r2, 0x00000002;
	imm32 r3, 0x00000003;
	imm32 r4, 0x00000004;
	imm32 r5, 0x00000005;
	imm32 r6, 0x00000006;
	imm32 r7, 0x00000007;
	imm32 p1, 0x00000011;
	imm32 p2, 0x00000012;
.ifndef BFIN_HOST
	imm32 p3, 0x00000013;
.endif
	imm32 p4, 0x00000014;

	loadsym P5, DATA0;
	loadsym I0, DATA1;

begin:
	ASTAT = R0;		// clear CC
	IF !CC JUMP LABEL1;	// (bp);
	CC = R4 < R5;		// CC FLAG   killed
	R1 = 21;
LABEL1:
	IF !CC JUMP LABEL2;	// (bp);
	CC = ! CC;
LABEL2:
	IF !CC JUMP LABEL3;	//  (bp);
	R2 = - R2;		// ALU2op    killed
LABEL3:
	IF !CC JUMP LABEL4;
	R3 <<= 2;		// LOGI2op   killed
LABEL4:
	IF !CC JUMP LABEL5;
	R0 = R1 + R2;		// COMP3op   killed
LABEL5:
	IF !CC JUMP LABEL6;
	R4 += 3;		// COMPI2opD killed
LABEL6:
	IF !CC JUMP LABEL7;	// (bp);
	R5 = 25;		// LDIMMHALF killed
LABEL7:
	IF !CC JUMP LABEL8;
	R6 = CC;		// CC2REG    killed
LABEL8:
	IF !CC JUMP LABEL9;
	JUMP.S BAD1;		// UJUMP     killed
LABEL9:
	IF !CC JUMP LABELCHK1;
BAD1:
	R7 = [ P5 ];		// LDST      killed

LABELCHK1:
	CHECKREG r0, 0x00000000;
	CHECKREG r1, 0x00000001;
	CHECKREG r2, 0x00000002;
	CHECKREG r3, 0x00000003;
	CHECKREG r4, 0x00000004;
	CHECKREG r5, 0x00000005;
	CHECKREG r6, 0x00000006;
	CHECKREG r7, 0x00000007;

LABEL10:
	IF !CC JUMP LABEL11;
	R1 = ( A1 += R4.L * R5.H ), A0 += R4.H * R5.L;
// DSP32MAC killed

LABEL11:
	IF !CC JUMP LABEL12;
	R2 = R2 +|+ R3;			// DSP32ALU killed

LABEL12:
	IF !CC JUMP LABEL13;
	R3 = LSHIFT R2 BY R3.L (V);	// dsp32shift killed

LABEL13:
	IF !CC JUMP LABEL14;
	R4.H = R1.L << 6;		// DSP32SHIFTIMM killed

LABEL14:
	IF !CC JUMP LABEL15;
	P2 = P1;			// REGMV PREG-PREG killed

LABEL15:
	IF !CC JUMP LABEL16;
	R5 = P1;			// REGMV Pr-to-Dr  killed

LABEL16:
	IF !CC JUMP LABEL17;
	ASTAT = R2;			// REGMV Dr-to-sys killed

LABEL17:
	IF !CC JUMP LABEL18;
	R6 = ASTAT;			// REGMV sys-to-Dr killed

LABEL18:
	IF !CC JUMP LABEL19;
	[ I0 ] = R2;			// DSPLDST  store  killed

LABEL19:
	IF !CC JUMP end;
	R7 = [ I0 ];			// DSPLDST  load   killed

end:

	CHECKREG r0, 0x00000000;
	CHECKREG r1, 0x00000001;
	CHECKREG r2, 0x00000002;
	CHECKREG r3, 0x00000003;
	CHECKREG r4, 0x00000004;
	CHECKREG r5, 0x00000005;
	CHECKREG r6, 0x00000006;
	CHECKREG r7, 0x00000007;

	pass

	.data
DATA0:
	.dd 0x000a0000
	.dd 0x000b0001
	.dd 0x000c0002
	.dd 0x000d0003
	.dd 0x000e0004

DATA1:
	.dd 0x00f00100
	.dd 0x00e00101
	.dd 0x00d00102
	.dd 0x00c00103
